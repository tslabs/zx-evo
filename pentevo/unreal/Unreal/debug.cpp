#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dx.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgreg.h"
#include "dbgtrace.h"
#include "dbgmem.h"
#include "dbgoth.h"
#include "dbglabls.h"
#include "dbgbpx.h"
#include "dbgcmd.h"
#include "util.h"

#ifdef MOD_MONITOR

u8 trace_labels;

unsigned show_scrshot;
unsigned user_watches[3] = { 0x4000, 0x8000, 0xC000 };

unsigned mem_sz = 8;
unsigned mem_disk, mem_track, mem_max;
u8 mem_ascii;
u8 mem_dump;
u8 editor = ED_MEM;

unsigned regs_curs;
unsigned dbg_extport;
u8 dgb_extval; // extended memory port like 1FFD or DFFD

unsigned ripper; // ripper mode (none/read/write)

DBGWND activedbg = WNDTRACE;

void debugscr();
unsigned find1dlg(unsigned start);
unsigned find2dlg(unsigned start);

/*
#include "dbgpaint.cpp"
#include "dbglabls.cpp"
#include "z80asm.cpp"
#include "dbgreg.cpp"
#include "dbgmem.cpp"
#include "dbgtrace.cpp"
#include "dbgrwdlg.cpp"
#include "dbgcmd.cpp"
#include "dbgbpx.cpp"
#include "dbgoth.cpp"
*/

void debugscr()
{
   memset(txtscr, BACKGR_CH, sizeof txtscr/2);
   memset(txtscr+sizeof txtscr/2, BACKGR, sizeof txtscr/2);
   nfr = 0;

   showregs();
   showtrace();
   showmem();
   showwatch();
   showstack();
   show_ay();
   showbanks();
   showports();
   showdos();

#if 1
   show_time();
#else
   tprint(copy_x, copy_y, "\x1A", 0x9C);
   tprint(copy_x+1, copy_y, "UnrealSpeccy " VERS_STRING, 0x9E);
   tprint(copy_x+20, copy_y, "by SMT", 0x9D);
   tprint(copy_x+26, copy_y, "\x1B", 0x9C);
   frame(copy_x, copy_y, 27, 1, 0x0A);
#endif
}

void handle_mouse()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned mx = ((mousepos & 0xFFFF)-temp.gx)/8,
            my = (((mousepos >> 16) & 0x7FFF)-temp.gy)/16;
   if (my >= trace_y && my < trace_y+trace_size && mx >= trace_x && mx < trace_x+32)
   {
      needclr++; activedbg = WNDTRACE;
      cpu.trace_curs = cpu.trpc[my - trace_y];
      if (mx - trace_x < cs[1][0]) cpu.trace_mode = 0;
      else if (mx - trace_x < cs[2][0]) cpu.trace_mode = 1;
      else cpu.trace_mode = 2;
   }
   if (my >= mem_y && my < mem_y+mem_size && mx >= mem_x && mx < mem_x+37)
   {
      needclr++; activedbg = WNDMEM;
      unsigned dx = mx-mem_x;
      if (mem_dump)
      {
         if (dx >= 5)
             cpu.mem_curs = cpu.mem_top + (dx-5) + (my-mem_y)*32;
      }
      else
      {
         unsigned mem_se = (dx-5)%3;
         if (dx >= 29) cpu.mem_curs = cpu.mem_top + (dx-29) + (my-mem_y)*8, mem_ascii=1;
         if (dx >= 5 && mem_se != 2 && dx < 29)
            cpu.mem_curs = cpu.mem_top + (dx-5)/3 + (my-mem_y)*8,
            cpu.mem_second = mem_se, mem_ascii=0;
      }
   }
   if (mx >= regs_x && my >= regs_y && mx < regs_x+32 && my < regs_y+4) {
      needclr++; activedbg = WNDREGS;
      for (unsigned i = 0; i < regs_layout_count; i++) {
         unsigned delta = 1;
         if (regs_layout[i].width == 16) delta = 4;
         if (regs_layout[i].width == 8) delta = 2;
         if (my-regs_y == regs_layout[i].y && mx-regs_x-regs_layout[i].x < delta) regs_curs = i;
      }
   }
   if (mousepos & 0x80000000) { // right-click
      enum { IDM_BPX=1, IDM_SOME_OTHER };
      HMENU menu = CreatePopupMenu();
      if (activedbg == WNDTRACE) {
         AppendMenu(menu, MF_STRING, IDM_BPX, "breakpoint");
      } else {
         AppendMenu(menu, MF_STRING, 0, "I don't know");
         AppendMenu(menu, MF_STRING, 0, "what to place");
         AppendMenu(menu, MF_STRING, 0, "to menu, so");
         AppendMenu(menu, MF_STRING, 0, "No Stuff Here");
      }
      int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN,
         (mousepos & 0xFFFF) + temp.client.left,
         ((mousepos>>16) & 0x7FFF) + temp.client.top, 0, wnd, 0);
      DestroyMenu(menu);
      if (cmd == IDM_BPX) cbpx();
      //if (cmd == IDM_SOME_OTHER) some_other();
      //needclr++;
   }
   mousepos = 0;
}

void TCpuMgr::CopyToPrev()
{
    for (unsigned i = 0; i < Count; i++)
        PrevCpus[i] = *Cpus[i];
}

/* ------------------------------------------------------------- */
void debug(Z80 *cpu)
{
   sound_stop();
   temp.mon_scale = temp.scale;
   temp.scale = 1;
   temp.rflags = RF_MONITOR;
   needclr = 1;
   dbgbreak = 1;
   //set_video();
   ShowWindow( debug_wnd, SW_SHOW );

   CpuMgr.SetCurrentCpu(cpu->GetIdx());
   TZ80State *prevcpu = &CpuMgr.PrevCpu(cpu->GetIdx());
   cpu->trace_curs = cpu->pc;
   cpu->dbg_stopsp = cpu->dbg_stophere = -1;
   cpu->dbg_loop_r1 = 0, cpu->dbg_loop_r2 = 0xFFFF;
   mousepos = 0;

   while (dbgbreak) // debugger event loop
   {
      if (trace_labels)
         mon_labels.notify_user_labels();

      cpu = &CpuMgr.Cpu();
      prevcpu = &CpuMgr.PrevCpu(cpu->GetIdx());
repaint_dbg:
      cpu->trace_top &= 0xFFFF;
      cpu->trace_curs &= 0xFFFF;

      debugscr();
      if (cpu->trace_curs < cpu->trace_top || cpu->trace_curs >= cpu->trpc[trace_size] || asmii==-1)
      {
         cpu->trace_top = cpu->trace_curs;
         debugscr();
      }
      
      debugflip();

sleep:
      while (!dispatch(0))
      {
         if (mousepos)
             handle_mouse();
         if (needclr)
         {
             needclr--;
             goto repaint_dbg;
         }
		 if (!dbgbreak)
			 goto leave_dbg;	/* ugh... too much gotos... */
         Sleep(20);
      }
      if (activedbg == WNDREGS && dispatch_more(ac_regs) > 0)
      {
          continue;
      }
      if (activedbg == WNDTRACE && dispatch_more(ac_trace) > 0)
      {
          continue;
      }
      if (activedbg == WNDMEM && dispatch_more(ac_mem) > 0)
      {
          continue;
      }
      if (activedbg == WNDREGS && dispatch_regs())
      {
          continue;
      }
      if (activedbg == WNDTRACE && dispatch_trace())
      {
          continue;
      }
      if (activedbg == WNDMEM && dispatch_mem())
      {
          continue;
      }
      if (needclr)
      {
          needclr--;
          continue;
      }
      goto sleep;
   }

leave_dbg:
   *prevcpu = *cpu;
//   CpuMgr.CopyToPrev();
   cpu->SetLastT();
   temp.scale = temp.mon_scale;
   temp.rflags = RF_GDI;
   //apply_video();
   ShowWindow( debug_wnd, SW_HIDE );
   sound_play();
}

void debug_cond_check(Z80 *cpu)
{
	if (cpu->cbpn)
	{
		cpu->r_low = (cpu->r_low & 0x7F) + cpu->r_hi;
		for (unsigned i = 0; i < cpu->cbpn; i++)
		{
			if (calc(cpu, cpu->cbp[i]))
			{
				cpu->dbgbreak |= 1;
				dbgbreak |= 1;
			}
		}
	}
}

void debug_events(Z80 *cpu)
{
   unsigned pc = cpu->pc & 0xFFFF;
   u8 *membit = cpu->membits + pc;
   *membit |= MEMBITS_X;
   cpu->dbgbreak |= (*membit & MEMBITS_BPX);
   dbgbreak |= (*membit & MEMBITS_BPX);

   if (pc == cpu->dbg_stophere)
   {
       cpu->dbgbreak = 1;
       dbgbreak = 1;
   }

   if ((cpu->sp & 0xFFFF) == cpu->dbg_stopsp)
   {
      if (pc > cpu->dbg_stophere && pc < cpu->dbg_stophere + 0x100)
      {
          cpu->dbgbreak = 1;
          dbgbreak = 1;
      }
      if (pc < cpu->dbg_loop_r1 || pc > cpu->dbg_loop_r2)
      {
          cpu->dbgbreak = 1;
          dbgbreak = 1;
      }
   }

   debug_cond_check(cpu);
   brk_port_in = brk_port_out = -1; // reset only when breakpoints active
   brk_mem_rd = brk_mem_wr = -1;	// reset only when breakpoints active

   if (cpu->dbgbreak)
       debug(cpu);
}

#endif // MOD_MONITOR

u8 isbrk(const Z80 &cpu) // is there breakpoints active or any other reason to use debug z80 loop?
{
#ifndef MOD_DEBUGCORE
   return 0;
#else

   #ifdef MOD_MEMBAND_LED
   if (conf.led.memband & 0x80000000)
       return 1;
   #endif

   if (conf.mem_model == MM_PROFSCORP)
       return 1; // breakpoint on read ROM switches ROM bank

   #ifdef MOD_MONITOR
   if (cpu.cbpn)
       return 1;
   u8 res = 0;
   for (int i = 0; i < 0x10000; i++)
       res |= cpu.membits[i];
   return (res & (MEMBITS_BPR | MEMBITS_BPW | MEMBITS_BPX));
   #endif

#endif
}


/* ===================== */

static LRESULT APIENTRY DebugWndProc(HWND hwnd,UINT uMessage,WPARAM wparam,LPARAM lparam)
{
	PAINTSTRUCT ps; 
    HDC hdc;

	if (uMessage == WM_CLOSE)
	{
		mon_emul();
		return 0;
	}

	if (uMessage == WM_PAINT)
	{
		u8 * const bptr = debug_gdibuf;
		hdc = BeginPaint(hwnd, &ps); 
		SetDIBitsToDevice(hdc, 0, 0, 640, 480, 0, 0, 0, 480, bptr, &debug_gdibmp.header, DIB_RGB_COLORS);
		EndPaint(hwnd, &ps);
		return 0L;
	}

	return DefWindowProc(hwnd, uMessage, wparam, lparam);
}

void init_debug()
{
	WNDCLASS  wc = { 0 };
	RECT ClRect;
	DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	wc.lpfnWndProc = (WNDPROC)DebugWndProc;
	wc.hInstance = hIn;
	wc.lpszClassName = "DEBUG_WND";
	wc.hIcon = NULL;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	RegisterClass(&wc);

	debug_wnd = CreateWindow("DEBUG_WND", "UnrealSpeccy debugger", dwStyle,
                    CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0, hIn, NULL);
    
	ClRect.left = 0;
	ClRect.top = 0;
	ClRect.right = 640 - 1;
	ClRect.bottom = 480 - 1;
	AdjustWindowRect( &ClRect, dwStyle, FALSE );
	SetWindowPos( debug_wnd, NULL, 0, 0, ClRect.right - ClRect.left + 1, ClRect.bottom - ClRect.top + 1, SWP_NOMOVE );

	for (unsigned i = 0; i < 0x100; i++)
	{
		unsigned y = (i & 8) ? 0xFF : 0xC0;
		unsigned r = (i & 2) ? y : 0;
		unsigned g = (i & 4) ? y : 0;
		unsigned b = (i & 1) ? y : 0;

		debug_gdibmp.header.bmiColors[i].rgbRed   = r;
		debug_gdibmp.header.bmiColors[i].rgbGreen = g;
		debug_gdibmp.header.bmiColors[i].rgbBlue  = b;
	}
}