#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgtrace.h"
#include "dbglabls.h"
#include "dbgpaint.h"
#include "dbgcmd.h"
#include "memory.h"
#include "z80asm.h"
#include "op_system.h"
#include "util.h"

int disasm_line(unsigned addr, char *line)
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned char dbuf[16+129/*Alone Code 0.36.7*/];
   int i; //Alone Coder 0.36.7
   for (/*int*/ i = 0; i < 16; i++) dbuf[i] = cpu.DirectRm(addr+i);
   sprintf(line, "%04X ", addr); int ptr = 5;
   int len = disasm(dbuf, addr, trace_labels) - dbuf;
   //8000 ..DDCB0106 rr (ix+1)
   if (trace_labels)
   {
      char *lbl = mon_labels.find(am_r(addr));
//      if (lbl) for (int k = 0; k < 10 && lbl[k]; line[ptr++] = lbl[k++]); //Alone Coder
      if (lbl) for (int k = 0; (k < 10) && lbl[k]; )line[ptr++] = lbl[k++]; //Alone Coder
   }
   else
   {
      int len1 = len;
      if (len > 4) len1 = 4, *(short*)(line+ptr) = WORD2('.','.'), ptr+=2;
      for (i = len-len1; i < len; i++)
         sprintf(line+ptr, "%02X", dbuf[i]), ptr += 2;
   }

   while (ptr < 16) line[ptr++] = ' ';
   strcpy(line+ptr, asmbuf);
   return len;
}

#define TWF_BRANCH  0x010000
#define TWF_BRADDR  0x020000
#define TWF_LOOPCMD 0x040000
#define TWF_CALLCMD 0x080000
#define TWF_BLKCMD  0x100000
unsigned tracewndflags()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned readptr = cpu.pc, base = cpu.hl;
   unsigned char opcode = 0; unsigned char ed = 0;
   for (;;)
   {
      opcode = cpu.DirectRm(readptr++);
      if (opcode == 0xDD)
          base = cpu.ix;
      else if (opcode == 0xFD)
          base = cpu.iy;
      else if (opcode == 0xED)
          ed = 1;
      else
          break;
   }

   unsigned fl = 0;
   if (opcode == 0x76) // halt
   {
       return TWF_BLKCMD;
   }

   if (ed)
   {
      if ((opcode & 0xF4) == 0xB0) // ldir/lddr | cpir/cpdr | inir/indr | otir/otdr
          return TWF_BLKCMD;

      if ((opcode & 0xC7) != 0x45)
          return 0; // reti/retn

 ret:
      return (cpu.DirectRm(cpu.sp) | (cpu.DirectRm(cpu.sp+1) << 8U)) | TWF_BRANCH | TWF_BRADDR;
   }

   if (opcode == 0xC9) // ret
       goto ret;
   if (opcode == 0xC3) // jp
   {
       jp: return (cpu.DirectRm(readptr) | (cpu.DirectRm(readptr+1) << 8U)) | TWF_BRANCH | fl;
   }
   if (opcode == 0xCD) // call
   {
       fl = TWF_CALLCMD;
       goto jp;
   }

   static const unsigned char flags[] = { ZF,CF,PV,SF };

   if ((opcode & 0xC1) == 0xC0)
   {
      unsigned char flag = flags[(opcode >> 4) & 3];
      unsigned char res = cpu.f & flag;
      if (!(opcode & 0x08))
          res ^= flag;
      if (!res)
          return 0;
      if ((opcode & 0xC7) == 0xC0) // ret cc
          goto ret;
      if ((opcode & 0xC7) == 0xC4) // call cc
      {
          fl = TWF_CALLCMD;
          goto jp;
      }
      if ((opcode & 0xC7) == 0xC2) // jp cc
      {
          fl = TWF_LOOPCMD;
          goto jp;
      }
   }

   if (opcode == 0xE9)
       return base | TWF_BRANCH | TWF_BRADDR; // jp (hl/ix/iy)

   if ((opcode & 0xC7) == 0xC7)
       return (opcode & 0x38) | TWF_CALLCMD | TWF_BRANCH; // rst #xx

   if ((opcode & 0xC7) == 0x00)
   {
      if (!opcode || opcode == 0x08)
          return 0;
      int offs = (signed char)cpu.DirectRm(readptr++);
      unsigned addr = (offs + readptr) | TWF_BRANCH;
      if (opcode == 0x18)
          return addr; // jr
      if (opcode == 0x10)
          return (cpu.b==1)? 0 : addr | TWF_LOOPCMD; // djnz

      unsigned char flag = flags[(opcode >> 4) & 1]; // jr cc
      unsigned char res = cpu.f & flag;
      if (!(opcode & 0x08))
          res ^= flag;
      return res? addr | TWF_LOOPCMD : 0;
   }
   return 0;
}

unsigned trcurs_y;
unsigned asmii;
char asmpc[64], dumppc[12];
const unsigned cs[3][2] = { {0,4}, {5,10}, {16,16} };

void showtrace()
{
   Z80 &cpu = CpuMgr.Cpu();
//   char line[40]; //Alone Coder 0.36.7
   char line[16+129]; //Alone Coder 0.36.7

   cpu.trace_curs &= 0xFFFF;
   cpu.trace_top &= 0xFFFF;
   cpu.pc &= 0xFFFF;
   cpu.trace_mode = (cpu.trace_mode+3) % 3;

   cpu.pc_trflags = tracewndflags();
   cpu.nextpc = (cpu.pc + disasm_line(cpu.pc, line)) & 0xFFFF;
   unsigned pc = cpu.trace_top;
   asmii = -1;
   unsigned char atr0 = (activedbg == WNDTRACE) ? W_SEL : W_NORM;
   unsigned ii; //Alone Coder 0.36.7
   for (/*unsigned*/ ii = 0; ii < trace_size; ii++)
   {
      pc &= 0xFFFF; cpu.trpc[ii] = pc;
      int len = disasm_line(pc, line);
      char *ptr = line+strlen(line);
      while (ptr < line+32) *ptr++ = ' '; line[32] = 0;

      unsigned char atr = (pc == cpu.pc)? W_TRACEPOS : atr0;
      if (cpu.membits[pc] & MEMBITS_BPX) atr = (atr&~7)|2;
      tprint(trace_x, trace_y+ii, line, atr);

      if (pc == cpu.trace_curs)
      {
         asmii = ii;
         if (activedbg == WNDTRACE)
            for (unsigned q = 0; q < cs[cpu.trace_mode][1]; q++)
               txtscr[80*30 + (trace_y+ii)*80 + trace_x + cs[cpu.trace_mode][0] + q] = W_CURS;
      }

      if (cpu.pc_trflags & TWF_BRANCH)
      {
         if (pc == cpu.pc)
         {
            unsigned addr = cpu.pc_trflags & 0xFFFF;
            unsigned arr = (addr <= cpu.pc)? 0x18 : 0x19; // up/down arrow
            unsigned char color = (pc == cpu.trace_curs && activedbg == WNDTRACE && cpu.trace_mode == 2)? W_TRACE_JINFO_CURS_FG : W_TRACE_JINFO_NOCURS_FG;
            if (cpu.pc_trflags & TWF_BRADDR) sprintf(line, "%04X%c", addr, arr), tprint_fg(trace_x+32-5, trace_y+ii, line, color);
            else tprint_fg(trace_x+32-1, trace_y+ii, (char*)&arr, color);
         }

         if (pc == (cpu.pc_trflags & 0xFFFF))
         {
            unsigned arr = 0x11; // left arrow
            tprint_fg(trace_x+32-1, trace_y+ii, (char*)&arr, W_TRACE_JARROW_FOREGR);
         }
      }

      pc += len;
   }
   cpu.trpc[ii] = pc;

   unsigned char dbuf[16];
   int i; //Alone Coder
   for (/*int*/ i = 0; i < 16; i++) dbuf[i] = cpu.DirectRm(cpu.trace_curs+i);
   int len = disasm(dbuf, cpu.trace_curs, 0) - dbuf; strcpy(asmpc, asmbuf);
   for (/*int*/ i = 0; i < len && i < 5; i++)
      sprintf(dumppc + i*2, "%02X", cpu.DirectRm(cpu.trace_curs+i));

   char cpu_num[10];
   _snprintf(cpu_num, sizeof(cpu_num), "Z80(%d)", CpuMgr.GetCurrentCpu());
   tprint(trace_x, trace_y-1, cpu_num, W_TITLE);

   char lbr[5];
   _snprintf(lbr, sizeof(lbr), "%04hX", cpu.last_branch);
   tprint(trace_x+8, trace_y-1, lbr,  W_TITLE);
   frame(trace_x,trace_y,32,trace_size,FRAME);
}

void c_lbl_import()
{
   mon_labels.import_menu();
}

      /* ------------------------------------------------------------- */
unsigned save_pos[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };
unsigned save_cur[8] = { -1,-1,-1,-1,-1,-1,-1,-1 };
unsigned stack_pos[32] = { -1 }, stack_cur[32] = { -1 };

void push_pos()
{
   Z80 &cpu = CpuMgr.Cpu();
   memmove(&stack_pos[1], &stack_pos[0], sizeof stack_pos - sizeof *stack_pos);
   memmove(&stack_cur[1], &stack_cur[0], sizeof stack_cur - sizeof *stack_cur);
   stack_pos[0] = cpu.trace_top; stack_cur[0] = cpu.trace_curs;
}

unsigned cpu_up(unsigned ip)
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned char buf1[0x10];
   unsigned p1 = (ip > sizeof buf1) ? ip - sizeof buf1 : 0;
   for (unsigned i = 0; i < sizeof buf1; i++) buf1[i] = cpu.DirectRm(p1+i);
   unsigned char *dispos = buf1, *prev;
   do {
      prev = dispos;
      dispos = disasm(dispos, 0, 0);
   } while ((unsigned)(dispos-buf1+p1) < ip);
   return prev-buf1+p1;
}

void cgoto()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned v = input4(trace_x, trace_y, cpu.trace_top);
   if (v != -1)
       cpu.trace_top = cpu.trace_curs = v;
}

void csetpc()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.pc = cpu.trace_curs;
}

void center()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (!cpu.trace_mode)
       sprintf(str, "%04X", cpu.trace_curs);
   else if (cpu.trace_mode == 1)
       strcpy(str, dumppc);
   else
       strcpy(str, asmpc);

   if (input.lastkey != VK_RETURN)
   {
       *str = 0;
        PostThreadMessage(GetCurrentThreadId(), WM_KEYDOWN, input.lastkey, 1);
   }

   for (;;)
   {
      if (!inputhex(trace_x+cs[cpu.trace_mode][0], trace_y + trcurs_y + asmii, cs[cpu.trace_mode][1], cpu.trace_mode < 2))
          break;
      if (!cpu.trace_mode)
      {
         push_pos();
         sscanf(str, "%X", &cpu.trace_top);
         cpu.trace_curs = cpu.trace_top;
         for (unsigned i = 0; i < asmii; i++)
             cpu.trace_top = cpu_up(cpu.trace_top);
         break;
      }
      else if (cpu.trace_mode == 1)
      {
         char *p; //Alone Coder 0.36.7
         for (/*char * */p = str+strlen(str)-1; p >= str && *p == ' '; *p-- = 0);
         unsigned char dump[8]; unsigned i;
         for (p = str, i = 0; ishex(*p) && ishex(p[1]); p+=2)
            dump[i++] = hex(p);
         if (*p) continue;
         for (unsigned j = 0; j < i; j++)
            cpu.DirectWm(cpu.trace_curs+j, dump[j]);
         break;
      }
      else
      {
         unsigned sz = assemble_cmd((unsigned char*)str, cpu.trace_curs);
         if (sz)
         {
            for (unsigned i = 0; i < sz; i++)
                cpu.DirectWm(cpu.trace_curs+i, asmresult[i]);
            showtrace();
            void cdown();
            cdown();
            break;
         }
      }
   }
}

char dispatch_trace()
{
   if (input.lastkey >= 'A' && input.lastkey < 'Z')
   {
       center();
       return 1;
   }
   return 0;
}

void cfindtext()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned char oldmode = editor; editor = ED_MEM;
   unsigned rs = find1dlg(cpu.trace_curs);
   editor = oldmode;
   if (rs != -1)
       cpu.trace_top = cpu.trace_curs = rs;
}
void cfindcode()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned char oldmode = editor; editor = ED_MEM;
   unsigned rs = find2dlg(cpu.trace_curs);
   editor = oldmode;
   if (rs != -1)
       cpu.trace_top = cpu.trace_curs = rs;
}

void cbpx()
{
   Z80 &cpu = CpuMgr.Cpu();
   cpu.membits[cpu.trace_curs] ^= MEMBITS_BPX;
}

void cfindpc()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.trace_top = cpu.trace_curs = cpu.pc;
}

void cup()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (cpu.trace_curs > cpu.trace_top)
   {
      for (unsigned i = 1; i < trace_size; i++)
         if (cpu.trpc[i] == cpu.trace_curs)
             cpu.trace_curs = cpu.trpc[i-1];
   }
   else
       cpu.trace_top = cpu.trace_curs = cpu_up(cpu.trace_curs);
}

void cdown()
{
   Z80 &cpu = CpuMgr.Cpu();
   for (unsigned i = 0; i < trace_size; i++)
      if (cpu.trpc[i] == cpu.trace_curs)
      {
         cpu.trace_curs = cpu.trpc[i+1];
         if (i+1 == trace_size)
             cpu.trace_top = cpu.trpc[1];
         break;
      }
}
void cleft()  { CpuMgr.Cpu().trace_mode--; }
void cright() { CpuMgr.Cpu().trace_mode++; }
void chere()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.dbgbreak = 0;
    dbgbreak = 0;
    cpu.dbgchk = 1;
 
    cpu.dbg_stophere = cpu.trace_curs;
}

void cpgdn()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned curs = 0;
   for (unsigned i = 0; i < trace_size; i++)
      if (cpu.trace_curs == cpu.trpc[i]) curs = i;
   cpu.trace_top = cpu.trpc[trace_size];
   showtrace();
   cpu.trace_curs = cpu.trpc[curs];
}

void cpgup()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned curs = 0;
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < trace_size; i++)
      if (cpu.trace_curs == cpu.trpc[i]) curs = i;
   for (i = 0; i < trace_size; i++)
       cpu.trace_top = cpu_up(cpu.trace_top);
   showtrace();
   cpu.trace_curs = cpu.trpc[curs];
}

void pop_pos()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (stack_pos[0] == -1)
       return;
   cpu.trace_curs = stack_cur[0];
   cpu.trace_top = stack_pos[0];
   memcpy(&stack_pos[0], &stack_pos[1], sizeof stack_pos - sizeof *stack_pos);
   memcpy(&stack_cur[0], &stack_cur[1], sizeof stack_cur - sizeof *stack_cur);
   stack_pos[(sizeof stack_pos / sizeof *stack_pos)-1] = -1;
}

void cjump()
{
   Z80 &cpu = CpuMgr.Cpu();
   char *ptr = 0;
   for (char *p = asmpc; *p; p++)
      if (ishex(p[0]) & ishex(p[1]) & ishex(p[2]) & ishex(p[3])) ptr = p;
   if (!ptr) return;
   push_pos();
   unsigned addr;
   sscanf(ptr, "%04X", &addr);
   cpu.trace_curs = cpu.trace_top = addr;
}

void cdjump()
{
   char *ptr = 0;
   for (char *p = asmpc; *p; p++)
      if (ishex(p[0]) & ishex(p[1]) & ishex(p[2]) & ishex(p[3])) ptr = p;
   if (!ptr) return;
   unsigned addr; sscanf(ptr, "%04X", &addr);
   Z80 &cpu = CpuMgr.Cpu();
   cpu.mem_curs = addr; activedbg = WNDMEM; editor = ED_MEM;
}

void cfliplabels()
{
   trace_labels = !trace_labels; showtrace();
}
void csave(unsigned n)
{
   Z80 &cpu = CpuMgr.Cpu();
   save_pos[n] = cpu.trace_top;
   save_cur[n] = cpu.trace_curs;
}
void crest(unsigned n)
{
   Z80 &cpu = CpuMgr.Cpu();
   if (save_pos[n] == -1)
       return;
   push_pos();
   cpu.trace_top = save_pos[n];
   cpu.trace_curs = save_cur[n];
}
void csave1() { csave(0); }
void csave2() { csave(1); }
void csave3() { csave(2); }
void csave4() { csave(3); }
void csave5() { csave(4); }
void csave6() { csave(5); }
void csave7() { csave(6); }
void csave8() { csave(7); }
void crest1() { crest(0); }
void crest2() { crest(1); }
void crest3() { crest(2); }
void crest4() { crest(3); }
void crest5() { crest(4); }
void crest6() { crest(5); }
void crest7() { crest(6); }
void crest8() { crest(7); }

namespace z80dbg
{
void __cdecl SetLastT()
{
   cpu.debug_last_t = comp.t_states + cpu.t;
}
}

void mon_step()
{
   Z80 &cpu = CpuMgr.Cpu();
   TZ80State &prevcpu = CpuMgr.PrevCpu();
   
   cpu.SetLastT();
   prevcpu = cpu;
//   CpuMgr.CopyToPrev();
   if (cpu.t >= conf.intlen)
       cpu.int_pend = false;
   cpu.Step();
   if (cpu.int_pend && cpu.iff1 && cpu.t != cpu.eipos && // int enabled in CPU not issued after EI
       cpu.int_gate) // int enabled by ATM hardware
   {
      handle_int(&cpu, cpu.IntVec());
   }

   cpu.CheckNextFrame();
   cpu.trace_curs = cpu.pc;
}

void mon_stepover()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned char trace = 1;

   // call,rst
   if (cpu.pc_trflags & TWF_CALLCMD)
   {
       cpu.dbg_stopsp = cpu.sp & 0xFFFF;
       cpu.dbg_stophere = cpu.nextpc;
       trace = 0;
   }
   else if (cpu.pc_trflags & TWF_BLKCMD) // ldir/lddr|cpir/cpdr|otir/otdr|inir/indr
   {
       trace = 0;
       cpu.dbg_stophere = cpu.nextpc;
   }

/* [vv]
   // jr cc,$-xx, jp cc,$-xx
   else if ((cpu.pc_trflags & TWF_LOOPCMD) && (cpu.pc_trflags & 0xFFFF) < (cpu.pc & 0xFFFF))
   {
      cpu.dbg_stopsp = cpu.sp & 0xFFFF;
      cpu.dbg_stophere = cpu.nextpc,
      cpu.dbg_loop_r1 = cpu.pc_trflags & 0xFFFF;
      cpu.dbg_loop_r2 = cpu.pc & 0xFFFF;
      trace = 0;
   }
*/

/* [vv]
   else if (cpu.pc_trflags & TWF_BRANCH)
       trace = 1;
   else
   {
       trace = 1;
       cpu.dbg_stophere = cpu.nextpc;
   }
*/

   if (trace)
   {
       mon_step();
   }
   else
   {
       cpu.dbgbreak = 0;
       dbgbreak = 0;
       cpu.dbgchk = 1;
   }
}
