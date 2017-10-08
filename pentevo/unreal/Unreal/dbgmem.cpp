#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgcmd.h"
#include "dbgmem.h"
#include "wd93crc.h"
#include "util.h"

TRKCACHE edited_track;

unsigned sector_offset, sector;

void findsector(unsigned addr)
{
   for (sector_offset = sector = 0; sector < edited_track.s; sector++, sector_offset += edited_track.hdr[sector].datlen)
      if (addr >= sector_offset && addr < sector_offset + edited_track.hdr[sector].datlen)
         return;
   errexit("internal diskeditor error");
}

u8 *editam(unsigned addr)
{
   Z80 &cpu = CpuMgr.Cpu();
   if (editor == ED_CMOS) return &cmos[addr & (sizeof(cmos)-1)];
   if (editor == ED_NVRAM) return &nvram[addr & (sizeof(nvram)-1)];
   if (editor == ED_MEM) return cpu.DirectMem(addr);
   if (!edited_track.trkd) return 0;
   if (editor == ED_PHYS) return edited_track.trkd + addr;
   // editor == ED_LOG
   findsector(addr); return edited_track.hdr[sector].data + addr - sector_offset;
}

void editwm(unsigned addr, u8 byte)
{
   if (editrm(addr) == byte) return;
   u8 *ptr = editam(addr);
   if (!ptr) return; *ptr = byte;
   if (editor == ED_MEM || editor == ED_CMOS || editor == ED_NVRAM) return;
   if (editor == ED_PHYS) { comp.wd.fdd[mem_disk].optype |= 2; return; }
   comp.wd.fdd[mem_disk].optype |= 1;
   // recalc sector checksum
   findsector(addr);
   *(u16*)(edited_track.hdr[sector].data + edited_track.hdr[sector].datlen) =
      wd93_crc(edited_track.hdr[sector].data - 1, edited_track.hdr[sector].datlen + 1);
}

unsigned memadr(unsigned addr)
{
   if (editor == ED_MEM) return (addr & 0xFFFF);
   if (editor == ED_CMOS) return (addr & (sizeof(cmos)-1));
   if (editor == ED_NVRAM) return (addr & (sizeof(nvram)-1));
   // else if (editor == ED_PHYS || editor == ED_LOG)
   if (!mem_max) return 0;
   while ((int)addr < 0) addr += mem_max;
   while (addr >= mem_max) addr -= mem_max;
   return addr;
}

void showmem()
{
   Z80 &cpu = CpuMgr.Cpu();
   char line[DEBUG_TEXT_WIDTH]; unsigned ii;
   unsigned cursor_found = 0;
   if (mem_dump) mem_ascii = 1, mem_sz = 32; else mem_sz = 8;

   if (editor == ED_LOG || editor == ED_PHYS)
   {
      edited_track.clear();
      edited_track.seek(comp.wd.fdd+mem_disk, mem_track/2, mem_track & 1, (editor == ED_LOG)? LOAD_SECTORS : JUST_SEEK);
      if (!edited_track.trkd) { // no track
         for (ii = 0; ii < mem_size; ii++) {
            sprintf(line, (ii == mem_size/2)?
               "          track not found            ":
               "                                     ");
            tprint(mem_x, mem_y+ii, line, (activedbg == WNDMEM) ? W_SEL : W_NORM);
         }
         mem_max = 0;
         goto title;
      }
      mem_max = edited_track.trklen;
      if (editor == ED_LOG)
         for (mem_max = ii = 0; ii < edited_track.s; ii++)
            mem_max += edited_track.hdr[ii].datlen;
   }
   else if (editor == ED_MEM)
       mem_max = 0x10000;
   else if (editor == ED_CMOS)
       mem_max = sizeof(cmos);
   else if (editor == ED_NVRAM)
       mem_max = sizeof(nvram);

redraw:
   if (mem_dump == 2)
   {	 // tsconf
#define tprx(a,b,c,d,e) sprintf(line, "%s: %02X:%04X", (c), (d), (e)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tpre(a,b,c,d,e) sprintf(line, "%s: %d:%d", (c), (d), (e)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprh(a,b,c,d) sprintf(line, "%s: %02X", (c), (d)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprd(a,b,c,d) sprintf(line, "%s: %d", (c), (d)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprs(a,b,c,d) sprintf(line, "%s: %s", (c), (d)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprc(a,b,c,d) sprintf(line, "%s:%s", (c), (d)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprl(a,b,c) sprintf(line, "%s", (c)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);
#define tprn(a,b,c) sprintf(line, "%d", (c)); tprint(mem_x + (a), mem_y + (b), line, W_NORM);

	   const char d_onoff[2][4] = {"dis", "ena"};
	   const char d_vmode[4][5] = {"ZX  ", "16c ", "256c", "text"};
	   const char d_rres[4][8] = {"256x192", "320x200", "320x240", "360x288"};
	   const char d_clk[4][7] = {"3.5MHz", "7MHz  ", "14MHz ", "unk   "};
	   const char d_lock[4][5] = {"512", "128", "aut", "1MB"};
	   const char d_dma[16][8] =
	   {
	   "unk    ",
	   "unk    ",
	   "RAM-RAM",
	   "BLT-RAM",
	   "SPI-RAM",
	   "RAM-SPI",
	   "IDE-RAM",
	   "RAM-IDE",
	   "FIL-RAM",
	   "RAM-CRM",
	   "unk    ",
	   "RAM-SFL",
	   "unk    ",
	   "unk    ",
	   "unk    ",
	   "unk    "
   };

   const char d_dmast[11][6] =
   {
	   "RAM  ",
	   "BLT  ",
	   "SPI_R",
	   "SPI_W",
	   "IDE_R",
	   "IDE_W",
	   "FILL ",
	   "CRAM ",
	   "SFILE",
	   "INIT ",
	   "NOP  "
   };

   tprs(0,  0, "VMod", d_vmode[comp.ts.vmode]);
        tprd(0,  1, "RRes", comp.ts.rres);
        tprs(0,  2, "Gfx ", d_onoff[!comp.ts.nogfx]);
        tprs(0,  3, "TSU ", d_onoff[!comp.ts.notsu]);
        tprh(0,  4, "Vpg ", comp.ts.vpage);
        tprh(0,  5, "Spg ", comp.ts.sgpage);
        tprh(0,  6, "T0pg", comp.ts.t0gpage[2]);
        tprh(0,  7, "T1pg", comp.ts.t1gpage[2]);
        tprh(0,  8, "TMpg", comp.ts.tmpage);
        tprh(0,  9, "Gpal", comp.ts.gpal);
        tprh(0, 10, "T0pl", comp.ts.t0pal << 2);
        tprh(0, 11, "T1pl", comp.ts.t1pal << 2);

        tprd(12,  0, "HSI", comp.ts.hsint);
        tprd(12,  1, "VSI", comp.ts.vsint);
        tprl(10,  2, "DMA LIN FRM");
        tprn(11,  3, comp.ts.intdma);
        tprn(15,  3, comp.ts.intline);
        tprn(19,  3, comp.ts.intframe);

        tprd(9,  5, "GX ", comp.ts.g_xoffs);
        tprd(9,  6, "GY ", comp.ts.g_yoffs);
        tprd(9,  7, "T0X", comp.ts.t0_xoffs);
        tprd(9,  8, "T0Y", comp.ts.t0_yoffs);
        tprd(9,  9, "T1X", comp.ts.t1_xoffs);
        tprd(9, 10, "T1Y", comp.ts.t1_yoffs);
        tprh(9, 11, "Brd", comp.ts.border);

        tprl(21,  0, "SP T1 T0 T1Z T0Z");
        tprn(22,  1, comp.ts.s_en);
        tprn(25,  1, comp.ts.t1_en);
        tprn(28,  1, comp.ts.t0_en);
        tprn(31,  1, comp.ts.t1z_en);
        tprn(34,  1, comp.ts.t0z_en);

        tprs(23,  3, "CLK", d_clk[comp.ts.zclk]);

        tprl(23,  4, "Cache: ");
        tprn(30,  4, comp.ts.cache_win0);
        tprn(32,  4, comp.ts.cache_win1);
        tprn(34,  4, comp.ts.cache_win2);
        tprn(36,  4, comp.ts.cache_win3);

        tprh(23,  5, "FMAddr", comp.ts.fmaddr);

        tprc(18,  7, "RAM0", d_onoff[comp.ts.w0_ram]);
        tprc(18,  8, "MAP0", d_onoff[!comp.ts.w0_map_n]);
        tprc(19,  9, "WE0", d_onoff[comp.ts.w0_we]);
        tprc(19, 10, "Lck", d_lock[comp.ts.lck128]);

        tprs(27,  7, "DMA", "");
        tprx(27,  8, "S", comp.ts.dma.saddr >> 14, comp.ts.dma.saddr & 0x3FFF);
        tprx(27,  9, "D", comp.ts.dma.daddr >> 14, comp.ts.dma.daddr & 0x3FFF);
        tpre(27, 10, "L", comp.ts.dma.num, comp.ts.dma.len * 2);
        tprs(27, 11, "St", d_dmast[comp.ts.dma.state]);
    }

   else
   {
	cpu.mem_curs = memadr(cpu.mem_curs);
	cpu.mem_top = memadr(cpu.mem_top);
	for (ii = 0; ii < mem_size; ii++)
	{
	   unsigned ptr = memadr(cpu.mem_top + ii*mem_sz);
	   sprintf(line, "%04X ", ptr);
	   unsigned cx = 0;
	   if (!mem_dump)
	   {  // 0000 11 22 33 44 55 66 77 88 abcdefgh
	      for (unsigned dx = 0; dx < 8; dx++)
	      {
	         if (ptr == cpu.mem_curs) cx = (mem_ascii) ? dx+29 : dx*3 + 5 + cpu.mem_second;
	         u8 c = editrm(ptr); ptr = memadr(ptr+1);
	         sprintf(line+5+3*dx, "%02X", c); line[7+3*dx] = ' ';
	         line[29+dx] = c ? c : '.';
	      }
	   }
	   else
	   {  // 0000 0123456789ABCDEF0123456789ABCDEF
	      for (unsigned dx = 0; dx < 32; dx++)
	      {
	         if (ptr == cpu.mem_curs) cx = dx+5;
	         u8 c = editrm(ptr); ptr = memadr(ptr+1);
	         line[dx+5] = c ? c : '.';
	      }
	   }

	   line[37] = 0;
	   tprint(mem_x, mem_y+ii, line, (activedbg == WNDMEM) ? W_SEL : W_NORM);
	   cursor_found |= cx;
	   if (cx && (activedbg == WNDMEM))
	      txtscr[(mem_y+ii) * DEBUG_TEXT_WIDTH + mem_x + cx + DEBUG_TEXT_WIDTH * DEBUG_TEXT_HEIGHT] = W_CURS;
	}
   }

   if (!cursor_found) { cursor_found=1; cpu.mem_top=cpu.mem_curs & ~(mem_sz-1); goto redraw; }
title:
   const char *MemName = 0;
   if (editor == ED_MEM)
       MemName = "memory";
   else if (editor == ED_CMOS)
       MemName = "cmos";
   else if (editor == ED_NVRAM)
       MemName = "nvram";

   if (editor == ED_MEM || editor == ED_CMOS || editor == ED_NVRAM)
   {
       sprintf(line, "%s: %04X gsdma: %06X", MemName, cpu.mem_curs & 0xFFFF, temp.gsdmaaddr);
   }
   else if (editor == ED_PHYS)
       sprintf(line, "disk %c, trk %02X, offs %04X", mem_disk+'A', mem_track, cpu.mem_curs);
   else
   { // ED_LOG
      if (mem_max)
          findsector(cpu.mem_curs);
      sprintf(line, "disk %c, trk %02X, sec %02X[%02X], offs %04X",
        mem_disk+'A', mem_track, sector, edited_track.hdr[sector].n,
        cpu.mem_curs-sector_offset);
   }
   tprint(mem_x, mem_y-1, line, W_TITLE);
   frame(mem_x,mem_y,37,mem_size,FRAME);
}

      /* ------------------------------------------------------------- */
void mstl()
{
    Z80 &cpu = CpuMgr.Cpu();
    if (mem_max) cpu.mem_curs &= ~(mem_sz-1), cpu.mem_second = 0;
}
void mendl()
{
    Z80 &cpu = CpuMgr.Cpu();
    if (mem_max) cpu.mem_curs |= (mem_sz-1), cpu.mem_second = 1;
}
void mup()
{
    Z80 &cpu = CpuMgr.Cpu();
    if (mem_max) cpu.mem_curs -= mem_sz;
}
void mpgdn()
{
    Z80 &cpu = CpuMgr.Cpu();
    if (mem_max) cpu.mem_curs += mem_size*mem_sz, cpu.mem_top += mem_size*mem_sz;
}

void mpgup()
{
    Z80 &cpu = CpuMgr.Cpu();
    if (mem_max) cpu.mem_curs -= mem_size*mem_sz, cpu.mem_top -= mem_size*mem_sz;
}

void mdown()
{
   if (!mem_max) return;

   Z80 &cpu = CpuMgr.Cpu();
   cpu.mem_curs += mem_sz;
   if (((cpu.mem_curs - cpu.mem_top + mem_max) % mem_max) / mem_sz >= mem_size) cpu.mem_top += mem_sz;
}

void mleft()
{
   if (!mem_max) return;
   Z80 &cpu = CpuMgr.Cpu();
   if (mem_ascii || !cpu.mem_second) cpu.mem_curs--;
   if (!mem_ascii) cpu.mem_second ^= 1;
}

void mright()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (!mem_max) return;
   if (mem_ascii || cpu.mem_second) cpu.mem_curs++;
   if (!mem_ascii) cpu.mem_second ^= 1;
   if (((cpu.mem_curs - cpu.mem_top + mem_max) % mem_max) / mem_sz >= mem_size) cpu.mem_top += mem_sz;
}

char dispatch_mem()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (!mem_max)
       return 0;
   if (mem_ascii)
   {
      u8 Kbd[256];
      GetKeyboardState(Kbd);
      u16 k;
      if (ToAscii(input.lastkey,0,Kbd,&k,0) != 1)
          return 0;
      k &= 0xFF;
      if (k < 0x20 || k >= 0x80)
          return 0;
      editwm(cpu.mem_curs, (u8)k);
      mright();
      return 1;
   }
   else
   {
      if ((input.lastkey >= '0' && input.lastkey <= '9') || (input.lastkey >= 'A' && input.lastkey <= 'F'))
      {
         u8 k = (input.lastkey >= 'A') ? input.lastkey-'A'+10 : input.lastkey-'0';
         u8 c = editrm(cpu.mem_curs);
         if (cpu.mem_second) editwm(cpu.mem_curs, (c & 0xF0) | k);
         else editwm(cpu.mem_curs, (c & 0x0F) | (k << 4));
         mright();
         return 1;
      }
   }
   return 0;
}

void mtext()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned rs = find1dlg(cpu.mem_curs);
   if (rs != -1) cpu.mem_curs = rs;
}

void mcode()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned rs = find2dlg(cpu.mem_curs);
   if (rs != -1) cpu.mem_curs = rs;
}

void mgoto()
{
   Z80 &cpu = CpuMgr.Cpu();
   unsigned v = input4(mem_x, mem_y, cpu.mem_top);
   if (v != -1) cpu.mem_top = (v & ~(mem_sz-1)), cpu.mem_curs = v;
}

void mswitch() { mem_ascii ^= 1; }

void msp()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.sp;
}
void mpc()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.pc;
}

void mbc()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.bc;
}

void mde()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.de;
}

void mhl()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.hl;
}

void mix()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.ix;
}

void miy()
{
    Z80 &cpu = CpuMgr.Cpu();
    cpu.mem_curs = cpu.iy;
}

void mmodemem() { editor = ED_MEM; }
void mmodephys() { editor = ED_PHYS; }
void mmodelog() { editor = ED_LOG; }

void mdiskgo()
{
   Z80 &cpu = CpuMgr.Cpu();
   if (editor == ED_MEM) return;
   for (;;) {
      *(unsigned*)str = mem_disk + 'A';
      if (!inputhex(mem_x+5, mem_y-1, 1, true)) return;
      if (*str >= 'A' && *str <= 'D') break;
   }
   mem_disk = *str-'A'; showmem();
   unsigned x = input2(mem_x+12, mem_y-1, mem_track);
   if (x == -1) return;
   mem_track = x;
   if (editor == ED_PHYS) return;
   showmem();
   // enter sector
   for (;;) {
      findsector(cpu.mem_curs); x = input2(mem_x+20, mem_y-1, sector);
      if (x == -1) return; if (x < edited_track.s) break;
   }
   for (cpu.mem_curs = 0; x; x--) cpu.mem_curs += edited_track.hdr[x-1].datlen;
}
