#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgtrace.h"
#include "util.h"

/*
     dialogs design

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from TR-DOS file  ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³file:  12345678 C      ³
³start: E000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from TR-DOS sector³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³trk (00-9F): 00        ³
³sec (00-0F): 08        ³
³start: E000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read RAW sectors       ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³drive: A               ³
³cyl (00-4F): 00 side: 0³
³sec (00-0F): 08 num: 01³
³start: E000            ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
³Read from host file    ³
ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´
³file: 12345678.bin     ³
³start: C000 end: FFFF  ³
ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ

*/

enum FILEDLG_MODE { FDM_LOAD = 0, FDM_SAVE, FDM_DISASM };

unsigned addr = 0, end = 0xFFFF;
u8 *memdata;
unsigned rw_drive, rw_trk, rw_cyl, rw_tsec, rw_rsec, rw_side;
char fname[20] = "", trdname[9] = "12345678", trdext[2] = "C";

const int FILEDLG_X = 6;
const int FILEDLG_Y = 10;
const int FILEDLG_DX = 25;

static void rw_err(const char *msg)
{
   MessageBox(wnd, msg, "Error", MB_OK | MB_ICONERROR);
}

char query_file_addr(FILEDLG_MODE mode)
{
   filledframe(FILEDLG_X, FILEDLG_Y, FILEDLG_DX, 5);
   char ln[64];
   static const char *titles[] = { " Read from binary file   ",
                                   " Write to binary file    ",
                                   " Disasm to text file     " };
   tprint(FILEDLG_X, FILEDLG_Y, titles[mode], FRM_HEADER);
   tprint(FILEDLG_X+1, FILEDLG_Y+2, "file:", FFRAME_INSIDE);
   sprintf(ln, (mode != FDM_LOAD)? "start: %04X end: %04X" : "start: %04X", addr, end);
   tprint(FILEDLG_X+1, FILEDLG_Y+3, ln, FFRAME_INSIDE);
   strcpy(str, fname);
   for (;;)
   {
      if (!inputhex(FILEDLG_X+7,FILEDLG_Y+2,16, false))
          return 0;
      if (mode != FDM_LOAD)
          break;
      if (GetFileAttributes(str) != INVALID_FILE_ATTRIBUTES)
          break;
   }
   strcpy(fname, str);
   sprintf(ln, "%-16s", fname);
   fillattr(FILEDLG_X+7,FILEDLG_Y+2,16);
   unsigned a1 = input4(FILEDLG_X+8,FILEDLG_Y+3,addr);
   if (a1 == -1)
       return 0;
   addr = a1;
   fillattr(FILEDLG_X+8,FILEDLG_Y+3,4);
   if (mode == FDM_LOAD)
       return 1;
   for (;;)
   {
      unsigned e1 = input4(FILEDLG_X+18,FILEDLG_Y+3,end);
      if (e1 == -1)
          return 0;
      if (e1 < addr)
          continue;
      end = e1;
          return 1;
   }
}

void write_mem()
{
   Z80 &cpu = CpuMgr.Cpu();
   u8 *ptr = memdata;
   for (unsigned a1 = addr; a1 <= end; a1++)
      *cpu.DirectMem(a1) = *ptr++;
}

void read_mem()
{
   Z80 &cpu = CpuMgr.Cpu();
   u8 *ptr = memdata;
   for (unsigned a1 = addr; a1 <= end; a1++)
     *ptr++ = cpu.DirectRm(a1);
}

char rw_select_drive()
{
   tprint(FILEDLG_X+1, FILEDLG_Y+2, "drive:", FFRAME_INSIDE);
   for (;;) {
      *(unsigned*)str = 'A' + rw_drive;
      if (!inputhex(FILEDLG_X+8, FILEDLG_Y+2, 1, true)) return 0;
      fillattr(FILEDLG_X+8, FILEDLG_Y+2, 1);
      unsigned disk = *str - 'A';
      if (disk > 3) continue;
      if (!comp.wd.fdd[disk].rawdata) continue;
      rw_drive = disk; return 1;
   }
}

char rw_trdos_sectors(FILEDLG_MODE mode)
{
   filledframe(FILEDLG_X, FILEDLG_Y, FILEDLG_DX, 7);
   const char *title = (mode == FDM_LOAD)? " Read from TR-DOS sectors" :
                                     " Write to TR-DOS sectors ";
   tprint(FILEDLG_X, FILEDLG_Y, title, FRM_HEADER);

   char ln[64]; unsigned t;

   sprintf(ln, "trk (00-9F): %02X", rw_trk);
   tprint(FILEDLG_X+1, FILEDLG_Y+3, ln, FFRAME_INSIDE);

   sprintf(ln, "sec (00-0F): %02X", rw_tsec);
   tprint(FILEDLG_X+1, FILEDLG_Y+4, ln, FFRAME_INSIDE);

   sprintf(ln, "start: %04X end: %04X", addr, end);
   tprint(FILEDLG_X+1, FILEDLG_Y+5, ln, FFRAME_INSIDE);

   if (!rw_select_drive()) return 0;
   FDD *fdd = &comp.wd.fdd[rw_drive];
   // if (fdd->sides != 2) { rw_err("single-side TR-DOS disks are not supported"); return 0; }

   t = input2(FILEDLG_X+14, FILEDLG_Y+3, rw_trk);
   if (t == -1) return 0; else rw_trk = t;
   fillattr(FILEDLG_X+14, FILEDLG_Y+3, 2);

   t = input2(FILEDLG_X+14, FILEDLG_Y+4, rw_tsec);
   if (t == -1) return 0; else rw_tsec = t;
   fillattr(FILEDLG_X+14, FILEDLG_Y+4, 2);

   t = input4(FILEDLG_X+8,FILEDLG_Y+5,addr);
   if (t == -1) return 0; else addr = t;
   fillattr(FILEDLG_X+8,FILEDLG_Y+5,4);

   for (;;) {
      t = input4(FILEDLG_X+18,FILEDLG_Y+5,end);
      fillattr(FILEDLG_X+18,FILEDLG_Y+5,4);
      if (t == -1) return 0;
      if (t < addr) continue;
      end = t; break;
   }

   unsigned offset = 0;
   if (mode == FDM_SAVE) read_mem();

   unsigned trk = rw_trk, sec = rw_tsec;

   TRKCACHE tc; tc.clear();
   for (;;) {
      int left = end+1 - (addr+offset);
      if (left <= 0) break;
      if (left > 0x100) left = 0x100;

      tc.seek(fdd, trk/2, trk & 1, LOAD_SECTORS);
      if (!tc.trkd) { sprintf(ln, "track #%02X not found", trk); rw_err(ln); break; }
      const SECHDR *hdr = tc.get_sector(sec+1);
      if (!hdr || !hdr->data) { sprintf(ln, "track #%02X, sector #%02X not found", trk, sec); rw_err(ln); break; }
      if (hdr->l != 1) { sprintf(ln, "track #%02X, sector #%02X is not 256 bytes", trk, sec); rw_err(ln); break; }

      if (mode == FDM_LOAD) {
         memcpy(memdata+offset, hdr->data, left);
      } else {
         tc.write_sector(sec+1, memdata+offset);
         fdd->optype |= 1;
      }

      offset += left;
      if (++sec == 0x10) trk++, sec = 0;
   }

   end = addr + offset - 1;
   if (mode == FDM_LOAD) write_mem();
   return 1;
}

char wr_trdos_file()
{
   filledframe(FILEDLG_X, FILEDLG_Y, FILEDLG_DX, 6);
   const char *title = " Write to TR-DOS file    ";
   tprint(FILEDLG_X, FILEDLG_Y, title, FRM_HEADER);

   char ln[64]; unsigned t;

   sprintf(ln, "file:  %-8s %s", trdname, trdext);
   tprint(FILEDLG_X+1, FILEDLG_Y+3, ln, FFRAME_INSIDE);

   sprintf(ln, "start: %04X end: %04X", addr, end);
   tprint(FILEDLG_X+1, FILEDLG_Y+4, ln, FFRAME_INSIDE);

   if (!rw_select_drive()) return 0;
   FDD *fdd = &comp.wd.fdd[rw_drive];
   // if (fdd->sides != 2) { rw_err("single-side TR-DOS disks are not supported"); return 0; }

   strcpy(str, trdname);
   if (!inputhex(FILEDLG_X+8,FILEDLG_Y+3,8,false)) return 0;
   fillattr(FILEDLG_X+8,FILEDLG_Y+3,8);
   strcpy(trdname, str);
   for (int ptr = strlen(trdname); ptr < 8; trdname[ptr++] = ' ');
   trdname[8] = 0;

   strcpy(str, trdext);
   if (!inputhex(FILEDLG_X+17,FILEDLG_Y+3,1,false)) return 0;
   fillattr(FILEDLG_X+17,FILEDLG_Y+3,1);
   trdext[0] = str[0]; trdext[1] = 0;

   t = input4(FILEDLG_X+8,FILEDLG_Y+4,addr);
   if (t == -1) return 0; else addr = t;
   fillattr(FILEDLG_X+8,FILEDLG_Y+4,4);

   for (;;) {
      t = input4(FILEDLG_X+18,FILEDLG_Y+4,end);
      fillattr(FILEDLG_X+18,FILEDLG_Y+4,4);
      if (t == -1) return 0;
      if (t < addr) continue;
      end = t; break;
   }

   read_mem();

   u8 hdr[16];
   memcpy(hdr, trdname, 8);
   hdr[8] = *trdext;

   unsigned sz = end-addr+1;
   *(u16*)(hdr+9) = addr;
   *(u16*)(hdr+11) = sz;
   hdr[13] = u8(align_by(sz, 0x100) / 0x100); // sector size

   fdd->optype |= 1;
   if (!fdd->addfile(hdr, memdata)) { rw_err("write error"); return 0; }
   return 1;
}

void mon_load()
{
   static MENUITEM items[] =
      { { "from binary file", MENUITEM::LEFT },
        { "from TR-DOS file", MENUITEM::LEFT },
        { "from TR-DOS sectors", MENUITEM::LEFT },
        { "from raw sectors of FDD image", MENUITEM::LEFT } };
   static MENUDEF menu = { items, 3, "Load data to memory...", 0 };

   if (!handle_menu(&menu))
       return;

   u8 bf[0x10000];
   memdata = bf;

   switch (menu.pos)
   {
      case 0:
      {
         if (!query_file_addr(FDM_LOAD))
             return;
         FILE *ff = fopen(fname, "rb");
         if (!ff)
             return;
         unsigned sz = fread(bf, 1, sizeof(bf), ff);
         fclose(ff);
         end = addr + sz - 1;
         end &= 0xFFFF;
         write_mem();
         return;
      }

      case 1:
      {
         rw_err("file selector\r\nis not implemented");
         return;
      }

      case 2:
      {
         rw_trdos_sectors(FDM_LOAD);
         return;
      }

      case 3:
      {
         return;
      }
   }
}

void mon_save()
{
   static MENUITEM items[] =
      { { "to binary file", MENUITEM::LEFT },
        { "to TR-DOS file", MENUITEM::LEFT },
        { "to TR-DOS sectors", MENUITEM::LEFT },
        { "as Z80 disassembly", MENUITEM::LEFT },
        { "to raw sectors of FDD image", (MENUITEM::FLAGS)(MENUITEM::LEFT | MENUITEM::DISABLED) } };
   static MENUDEF menu = { items, 4, "Save data from memory...", 0 };

   if (!handle_menu(&menu)) return;

   u8 bf[0x10000]; memdata = bf;

   switch (menu.pos)
   {
      case 0:
      {
         if (!query_file_addr(FDM_SAVE)) return;
         read_mem();
         FILE *ff = fopen(fname, "wb");
         if (!ff) return;
         fwrite(bf, 1, end+1-addr, ff);
         fclose(ff);
         return;
      }

      case 1:
      {
         wr_trdos_file();
         return;
      }

      case 2:
      {
         rw_trdos_sectors(FDM_SAVE);
         return;
      }

      case 3:
      {
         if (!query_file_addr(FDM_DISASM)) return;
         FILE *ff = fopen(fname, "wt");
         if (!ff) return;
         for (unsigned a = addr; a <= end; ) {
//            char line[64]; //Alone Coder 0.36.7
            char line[16+129]; //Alone Coder 0.36.7
            a += disasm_line(a, line);
            fprintf(ff, "%s\n", line);
         }
         fclose(ff);
         return;
      }

      case 4:
      {
         return;
      }
   }
}
