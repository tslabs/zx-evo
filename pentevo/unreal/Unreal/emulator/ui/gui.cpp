#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "config.h"
#include "draw.h"
#include "dx/dx.h"
#include "tape.h"
#include "snapshot.h"
#include "leds.h"
#include "util.h"
#include "hard/tsconf.h"

void setcheck(unsigned ID, u8 state = 1)
{
   CheckDlgButton(dlg, ID, state ? BST_CHECKED : BST_UNCHECKED);
}

u8 getcheck(unsigned ID)
{
   return (IsDlgButtonChecked(dlg, ID) == BST_CHECKED);
}


#ifdef MOD_SETTINGS

CONFIG c1;
char dlgok = 0;

const char *lastpage;

char rset_list[0x800];

char compare_rset(char *rname)
{
   CONFIG c2; load_romset(&c2, rname);
   if (stricmp(c2.sos_rom_path, c1.sos_rom_path)) return 0;
   if (stricmp(c2.dos_rom_path, c1.dos_rom_path)) return 0;
   if (stricmp(c2.sys_rom_path, c1.sys_rom_path)) return 0;
   if (stricmp(c2.zx128_rom_path, c1.zx128_rom_path)) return 0;
   return 1;
}

void find_romset()
{
   HWND box = GetDlgItem(dlg, IDC_ROMSET); int cur = -1, i = 0;
   for (char *dst = rset_list; *dst; dst += strlen(dst)+1, i++)
      if (compare_rset(dst)) cur = i;
   SendMessage(box, CB_SETCURSEL, cur, 0);
}

char select_romfile(char *dstname)
{
   char fname[FILENAME_MAX];
   fname[0] = 0;
/*
   strcpy(fname, dstname);
   char *x = strrchr(fname+2, ':');
   if (x)
       *x = 0;
*/
   OPENFILENAME ofn = { 0 };
   ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
   ofn.hwndOwner = dlg;
   ofn.lpstrFilter = "ROM image (*.ROM)\0*.ROM\0All files\0*.*\0";
   ofn.lpstrFile = fname;
   ofn.nMaxFile = _countof(fname);
   ofn.lpstrTitle = "Select ROM";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
   ofn.lpstrInitialDir   = temp.RomDir;
   if (!GetOpenFileName(&ofn))
       return 0;
   strcpy(dstname, fname);
   strcpy(temp.RomDir, ofn.lpstrFile);
   char *Ptr = strrchr(temp.RomDir, '\\');
   if (Ptr)
    *Ptr = 0;
   return 1;
}

char *MemDlg_get_bigrom()
{
   if (c1.mem_model == MM_PENTAGON) return c1.pent_rom_path;
   if (c1.mem_model == MM_TSL) return c1.tsl_rom_path;
   return 0;
}

void change_rompage(int dx, int reload)
{
   int x = SendDlgItemMessage(dlg, IDC_ROMPAGE, CB_GETCURSEL, 0, 0);
   static char *pgs[] = { c1.sos_rom_path, c1.zx128_rom_path, c1.dos_rom_path, c1.sys_rom_path };
   char *ptr = pgs[x];
   if (reload)
       select_romfile(ptr);
   if (dx) {
      char *x = strrchr(ptr+2, ':');
      unsigned pg = 0;
      if (!x) x = ptr + strlen(ptr); else { *x = 0; pg = atoi(x+1); }
      FILE *ff = fopen(ptr, "rb");
      unsigned sz = 0;
      if (ff) fseek(ff, 0, SEEK_END), sz = ftell(ff)/PAGE, fclose(ff);
      if ((unsigned)(pg+dx) < sz) {
         pg += dx;
         SendDlgItemMessage(dlg, IDC_ROMSET, CB_SETCURSEL, 0, 0);
      }
      sprintf(x, ":%d", pg);
   }
   SendDlgItemMessage(dlg, IDE_ROMNAME, WM_SETTEXT, 0, (LPARAM)ptr);
   find_romset();
}

void change_rombank(int dx, int reload)
{
   char *romname = MemDlg_get_bigrom();

   char line[512];

   strcpy(line, romname);

   char *x = strrchr(line+2, ':');

   unsigned pg = 0;
   if (!x)
       x = line + strlen(line);
   else
   {
       *x = 0;
       pg = atoi(x+1);
   }

   if (reload)
   {
       if (!select_romfile(line))
           return;
       x = line + strlen(line);
   }

   FILE *ff = fopen(line, "rb");
   unsigned sz = 0;
   if (ff)
   {
       fseek(ff, 0, SEEK_END);
       sz = ftell(ff);
       fclose(ff);
   }

   if (!sz || (sz & 0xFFFF))
   {
       err: MessageBox(dlg, "Invalid ROM size", "error", MB_ICONERROR | MB_OK);
       return;
   }

   sz /= 1024;

   if ((unsigned)(pg+dx) < sz/256)
       pg += dx;
   if (sz > 256)
       sprintf(x, ":%d", pg);
   strcpy(romname, line);
   SendDlgItemMessage(dlg, IDE_BIGROM, WM_SETTEXT, 0, (LPARAM)romname);

   sprintf(line, "Loaded ROM size: %dK", sz);
   SetDlgItemText(dlg, IDC_TOTAL_ROM, line);
   ShowWindow(GetDlgItem(dlg, IDC_TOTAL_ROM), SW_SHOW);
}

void reload_roms()
{
   unsigned i = 0, n = SendDlgItemMessage(dlg, IDC_ROMSET, CB_GETCURSEL, 0, 0);
   char *dst; //Alone Coder 0.36.7
   for (/*char * */dst = rset_list; *dst && i < n; i++, dst += strlen(dst)+1);
   if (!*dst) return;
   load_romset(&c1, dst);
   change_rompage(0,0);
}

void MemDlg_set_visible()
{
   int vis = !c1.use_romset? SW_SHOW : SW_HIDE;
   ShowWindow(GetDlgItem(dlg, IDE_BIGROM), vis);
   ShowWindow(GetDlgItem(dlg, IDB_ROMSEL_S), vis);
   ShowWindow(GetDlgItem(dlg, IDC_FILEBANK), vis);
   vis = c1.use_romset? SW_SHOW : SW_HIDE;
   ShowWindow(GetDlgItem(dlg, IDC_ROMSET), vis);
   ShowWindow(GetDlgItem(dlg, IDC_ROMPAGE), vis);
   ShowWindow(GetDlgItem(dlg, IDE_ROMNAME), vis);
   ShowWindow(GetDlgItem(dlg, IDC_FILEPAGE), vis);
   ShowWindow(GetDlgItem(dlg, IDB_ROMSEL_P), vis);
   ShowWindow(GetDlgItem(dlg, IDC_TOTAL_ROM), SW_HIDE);
}

void mem_set_sizes()
{
   unsigned mems = mem_model[c1.mem_model].availRAMs;
   unsigned best = mem_model[c1.mem_model].defaultRAM;

   EnableWindow(GetDlgItem(dlg, IDC_RAM128),  (mems & RAM_128)?  1:0);
   EnableWindow(GetDlgItem(dlg, IDC_RAM256),  (mems & RAM_256)?  1:0);
   EnableWindow(GetDlgItem(dlg, IDC_RAM512),  (mems & RAM_512)?  1:0);
   EnableWindow(GetDlgItem(dlg, IDC_RAM1024), (mems & RAM_1024)? 1:0);
   EnableWindow(GetDlgItem(dlg, IDC_RAM2048), (mems & RAM_2048)? 1:0);
   EnableWindow(GetDlgItem(dlg, IDC_RAM4096), (mems & RAM_4096)? 1:0);

   char ok = 1;
   if (getcheck(IDC_RAM128) && !(mems & RAM_128))  ok = 0;
   if (getcheck(IDC_RAM256) && !(mems & RAM_256))  ok = 0;
   if (getcheck(IDC_RAM512) && !(mems & RAM_512))  ok = 0;
   if (getcheck(IDC_RAM1024)&& !(mems & RAM_1024)) ok = 0;
   if (getcheck(IDC_RAM2048)&& !(mems & RAM_2048)) ok = 0;
   if (getcheck(IDC_RAM4096)&& !(mems & RAM_4096)) ok = 0;

   if (!ok) {
      setcheck(IDC_RAM128, 0);
      setcheck(IDC_RAM256, 0);
      setcheck(IDC_RAM512, 0);
      setcheck(IDC_RAM1024,0);
	  setcheck(IDC_RAM2048,0);
      setcheck(IDC_RAM4096,0);
      if (best == 128) setcheck(IDC_RAM128);
      if (best == 256) setcheck(IDC_RAM256);
      if (best == 512) setcheck(IDC_RAM512);
      if (best == 1024)setcheck(IDC_RAM1024);
	  if (best == 2048)setcheck(IDC_RAM2048);
      if (best == 4096)setcheck(IDC_RAM4096);
   }

   char *romname = MemDlg_get_bigrom();
   EnableWindow(GetDlgItem(dlg, IDC_SINGLE_ROM), romname? 1 : 0);
   if (romname) SetDlgItemText(dlg, IDE_BIGROM, romname);
   else c1.use_romset = 1, setcheck(IDC_CUSTOM_ROM,1), setcheck(IDC_SINGLE_ROM,0);

   int cache_ok = 1;
   EnableWindow(GetDlgItem(dlg, IDC_CACHE0), cache_ok);
   EnableWindow(GetDlgItem(dlg, IDC_CACHE16), cache_ok);
   EnableWindow(GetDlgItem(dlg, IDC_CACHE32), cache_ok);

   MemDlg_set_visible();
}

INT_PTR CALLBACK MemDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg; char bf[0x800];
   static char lock = 0;

   if (msg == WM_INITDIALOG) {
      HWND box = GetDlgItem(dlg, IDC_MEM);
      for (unsigned i = 0; i < N_MM_MODELS; i++)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)mem_model[i].fullname);

      box = GetDlgItem(dlg, IDC_ROMPAGE);
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"BASIC48");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"BASIC128");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"TR-DOS");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"SERVICE");
      SendMessage(box, CB_SETCURSEL, 0, 0);

      GetPrivateProfileSectionNames(bf, sizeof bf, ininame);
      box = GetDlgItem(dlg, IDC_ROMSET);
      char *dst = rset_list;
      for (char *p = bf; *p; p += strlen(p)+1) {
         if ((*(unsigned*)p | 0x20202020) != WORD4('r','o','m','.')) continue;
         strcpy(dst, p+4); dst += strlen(dst)+1;
         char line[128]; GetPrivateProfileString(p, "title", p+4, line, sizeof line, ininame);
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)line);
      }
      *dst = 0;
   }
   if (!lock && msg == WM_COMMAND) {
      unsigned id = LOWORD(wp), code = HIWORD(wp);
      if (code == BN_CLICKED) {
         if (id == IDC_SINGLE_ROM) c1.use_romset = 0, MemDlg_set_visible();
         if (id == IDC_CUSTOM_ROM) c1.use_romset = 1, MemDlg_set_visible();
         if (id == IDB_ROMSEL_P) change_rompage(0,1);
         if (id == IDB_ROMSEL_S) change_rombank(0,1);
      }
      if (code == CBN_SELCHANGE) {
         if (id == IDC_ROMSET) reload_roms();
         if (id == IDC_ROMPAGE) change_rompage(0,0);
         if (id == IDC_MEM)
            c1.mem_model = (MEM_MODEL)SendDlgItemMessage(dlg, IDC_MEM, CB_GETCURSEL, 0, 0),
            lock=1, mem_set_sizes(), lock=0;
      }
      return 1;
   }
   if (msg != WM_NOTIFY) return 0;
   NM_UPDOWN *nud = (NM_UPDOWN*)lp;
   if (nud->hdr.code == UDN_DELTAPOS) {
      if (wp == IDC_FILEPAGE) change_rompage(nud->iDelta > 0 ? 1 : -1, 0);
      if (wp == IDC_FILEBANK) change_rombank(nud->iDelta > 0 ? 1 : -1, 0);
      return TRUE; // don't chage up-down state
   }
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      if (getcheck(IDC_CACHE0)) c1.cache = 0;
      if (getcheck(IDC_CACHE16)) c1.cache = 16;
      if (getcheck(IDC_CACHE32)) c1.cache = 32;

      if (getcheck(IDC_CMOS_NONE)) c1.cmos = 0;
      if (getcheck(IDC_CMOS_DALLAS)) c1.cmos = 1;
      if (getcheck(IDC_CMOS_RUS)) c1.cmos = 2;

      if (getcheck(IDC_RAM128)) c1.ramsize = 128;
      if (getcheck(IDC_RAM256)) c1.ramsize = 256;
      if (getcheck(IDC_RAM512)) c1.ramsize = 512;
      if (getcheck(IDC_RAM1024))c1.ramsize = 1024;
	  if (getcheck(IDC_RAM2048))c1.ramsize = 2048;
      if (getcheck(IDC_RAM4096))c1.ramsize = 4096;

      c1.smuc = getcheck(IDC_SMUC);
   }
   if (nm->code == PSN_SETACTIVE) {
      lock = 1;
      SendDlgItemMessage(dlg, IDC_MEM, CB_SETCURSEL, c1.mem_model, 0);
      setcheck(IDC_RAM128, (c1.ramsize == 128));
      setcheck(IDC_RAM256, (c1.ramsize == 256));
      setcheck(IDC_RAM512, (c1.ramsize == 512));
      setcheck(IDC_RAM1024,(c1.ramsize == 1024));
	  setcheck(IDC_RAM2048,(c1.ramsize == 2048));
      setcheck(IDC_RAM4096,(c1.ramsize == 4096));
      setcheck(IDC_SINGLE_ROM, !c1.use_romset);
      setcheck(IDC_CUSTOM_ROM, c1.use_romset);
      find_romset();

      setcheck(IDC_CACHE0,  (c1.cache == 0));
      setcheck(IDC_CACHE16, (c1.cache == 16));
      setcheck(IDC_CACHE32, (c1.cache == 32));

      setcheck(IDC_CMOS_NONE, (c1.cmos == 0));
      setcheck(IDC_CMOS_DALLAS, (c1.cmos == 1));
      setcheck(IDC_CMOS_RUS, (c1.cmos == 2));

      setcheck(IDC_SMUC, c1.smuc);

      mem_set_sizes();
      lock = 0;

      lastpage = "MEMORY";
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

int getint(unsigned ID) {
   HWND wnd = GetDlgItem(dlg, ID);
   char bf[64]; SendMessage(wnd, WM_GETTEXT, sizeof bf, (LPARAM)bf);
   return atoi(bf);
}
void setint(unsigned ID, int num) {
   HWND wnd = GetDlgItem(dlg, ID);
   char bf[64]; sprintf(bf, "%d", num);
   SendMessage(wnd, WM_SETTEXT, 0, (LPARAM)bf);
}

INT_PTR CALLBACK UlaDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   NMHDR *nm = (NMHDR*)lp;
   volatile static char block=0;
   if (msg == WM_INITDIALOG) {
      HWND box = GetDlgItem(dlg, IDC_ULAPRESET);
      for (unsigned i = 0; i < num_ula; i++)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)ulapreset[i]);
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"<custom>");
   }
   if (msg == WM_COMMAND && !block) {
      unsigned id = LOWORD(wp), code = HIWORD(wp);
      if ((code == EN_CHANGE && (id==IDE_FRAME || id==IDE_LINE || id==IDE_INT || id==IDE_INT_LEN || id==IDE_INTSTART))
          || (code == BN_CLICKED && (id==IDC_EVENM1 || id==IDC_4TBORDER || id==IDC_FLOAT_BUS || id==IDC_FLOAT_DOS || id==IDC_PORT_FF)))
      {
         c1.ula_preset = -1;
         SendDlgItemMessage(dlg, IDC_ULAPRESET, CB_SETCURSEL, num_ula, 0);
      }
      if (code == CBN_SELCHANGE) {
         if (id == IDC_ULAPRESET) {
            unsigned pre = SendDlgItemMessage(dlg, IDC_ULAPRESET, CB_GETCURSEL, 0, 0);
            if (pre == num_ula) pre = -1;
            c1.ula_preset = (u8)pre;
            if (pre == -1) return 1;
            CONFIG tmp = conf;
            conf.ula_preset = (u8)pre; load_ula_preset();
            c1.frame = /*conf.frame*/frametime/*Alone Coder*/, c1.intfq = conf.intfq, c1.intlen = conf.intlen, c1.t_line = conf.t_line,
            c1.intstart = conf.intstart, c1.even_M1 = conf.even_M1, c1.border_4T = conf.border_4T;
            c1.floatbus = conf.floatbus, c1.floatdos = conf.floatdos;
            c1.portff = conf.portff;
            conf = tmp;
            goto refresh;
         }
      }
      return 1;
   }
   if (msg != WM_NOTIFY) return 0;
   if (nm->code == PSN_KILLACTIVE) {
      c1.frame = getint(IDE_FRAME);
      c1.t_line = getint(IDE_LINE);
      c1.intstart = getint(IDE_INTSTART);
      c1.intfq = getint(IDE_INT);
      c1.intlen = getint(IDE_INT_LEN);
      c1.nopaper = getcheck(IDC_NOPAPER);
      c1.even_M1 = getcheck(IDC_EVENM1);
      c1.border_4T = getcheck(IDC_4TBORDER);
      c1.floatbus = getcheck(IDC_FLOAT_BUS);
      c1.floatdos = getcheck(IDC_FLOAT_DOS);
      c1.portff = getcheck(IDC_PORT_FF) != 0;
   }
   if (nm->code == PSN_SETACTIVE) {
refresh:
      SendDlgItemMessage(dlg, IDC_ULAPRESET, CB_SETCURSEL, c1.ula_preset<num_ula? c1.ula_preset : num_ula, 0);
      block=1;
      setint(IDE_FRAME, c1.frame);
      setint(IDE_LINE, c1.t_line);
      setint(IDE_INTSTART, c1.intstart);
      setint(IDE_INT, c1.intfq);
      setint(IDE_INT_LEN, c1.intlen);
      setcheck(IDC_NOPAPER, c1.nopaper);
      setcheck(IDC_EVENM1, c1.even_M1);
      setcheck(IDC_4TBORDER, c1.border_4T);
      setcheck(IDC_FLOAT_BUS, c1.floatbus);
      setcheck(IDC_FLOAT_DOS, c1.floatdos);
      setcheck(IDC_PORT_FF, c1.portff);

      block=0;
      lastpage = "ULA";
      return 1;
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

void HddDlg_set_active()
{
	const int enable = (c1.ide_scheme != ide_scheme::none);
   EnableWindow(GetDlgItem(dlg, IDB_HDD0), enable);
   EnableWindow(GetDlgItem(dlg, IDE_HDD0_CHS), enable);
   EnableWindow(GetDlgItem(dlg, IDE_HDD0_LBA), enable);
   EnableWindow(GetDlgItem(dlg, IDC_HDD0_RO), enable);
   EnableWindow(GetDlgItem(dlg, IDB_HDD1), enable);
   EnableWindow(GetDlgItem(dlg, IDE_HDD1_CHS), enable);
   EnableWindow(GetDlgItem(dlg, IDE_HDD1_LBA), enable);
   EnableWindow(GetDlgItem(dlg, IDC_HDD1_RO), enable);
}

void HddDlg_show_info(int device)
{
   unsigned c = c1.ide[device].c, h = c1.ide[device].h, s = c1.ide[device].s, l = c1.ide[device].lba;
   DWORD readonly = 0;
   if (!*c1.ide[device].image) readonly = 1;
   if (*c1.ide[device].image == '<') {
      unsigned drive = find_hdd_device(c1.ide[device].image);
      if (drive < max_phys_hd_drives + max_phys_cd_drives) {
         c = ((u16*)phys[drive].idsector)[1];
         h = ((u16*)phys[drive].idsector)[3];
         s = ((u16*)phys[drive].idsector)[6];
         l = *(unsigned*)(phys[drive].idsector+0x78);
         if (!l) l = c*h*s;
         readonly = 1;
      }
   }
   HWND edit_l = GetDlgItem(dlg, device? IDE_HDD1_LBA : IDE_HDD0_LBA);
   HWND edit_c = GetDlgItem(dlg, device? IDE_HDD1_CHS : IDE_HDD0_CHS);
   SendMessage(edit_l, EM_SETREADONLY, readonly, 0);
   SendMessage(edit_c, EM_SETREADONLY, readonly, 0);

   SetDlgItemText(dlg, device? IDE_HDD1 : IDE_HDD0, c1.ide[device].image);
   char textbuf[512];
   *textbuf = 0; if (*c1.ide[device].image) sprintf(textbuf, "%d", l);
   SetWindowText(edit_l, textbuf);
   *textbuf = 0; if (*c1.ide[device].image) sprintf(textbuf, "%d/%d/%d", c,h,s);
   SetWindowText(edit_c, textbuf);
}

void HddDlg_select_image(int device)
{
   HMENU selmenu = CreatePopupMenu();
   AppendMenu(selmenu, MF_STRING, 1, "Select image file...");
   AppendMenu(selmenu, MF_STRING, 2, "Remove device");
   int max, drive; char textbuf[512];
   for (max = drive = 0; drive < n_phys; drive++) {

      if (phys[drive].type == ata_devtype_t::nthdd)
         sprintf(textbuf, "HDD %d: %s, %d Mb", phys[drive].spti_id, phys[drive].viewname, phys[drive].hdd_size / (2*1024));

      else if (phys[drive].type == ata_devtype_t::spti_cd)
         sprintf(textbuf, "CD-ROM %d: %s", phys[drive].spti_id, phys[drive].viewname);

      else if (phys[drive].type == ata_devtype_t::aspi_cd)
         sprintf(textbuf, "CD-ROM %d.%d: %s", phys[drive].adapterid, phys[drive].targetid, phys[drive].viewname);

      else continue;

      if (!max) AppendMenu(selmenu, MF_SEPARATOR, 0, 0);
      max++, AppendMenu(selmenu, MF_STRING, drive+8, textbuf);
   }

   RECT rc; GetWindowRect(GetDlgItem(dlg, device? IDB_HDD1 : IDB_HDD0), &rc);
   int code = TrackPopupMenu(selmenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, rc.left, rc.bottom, 0, dlg, 0);
   DestroyMenu(selmenu);
   if (!code) return;

   if (code == 2) { // remove
      *c1.ide[device].image = 0;
      HddDlg_show_info(device);
      return;
   }

   if (code >= 8)
   { // physical device
      if (MessageBox(dlg, "All volumes on drive will be dismounted\n", "Warning",
          MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
          return;
      strcpy(c1.ide[device].image, phys[code-8].viewname);
      HddDlg_show_info(device);
      return;
   }

   // open HDD image
   OPENFILENAME fn = { 0 };
/*
   strcpy(textbuf, c1.ide[device].image);
   if (textbuf[0] == '<') *textbuf = 0;
*/
   textbuf[0] = 0;
   fn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
   fn.hwndOwner = dlg;
   fn.lpstrFilter = "Hard disk drive image (*.HDD)\0*.HDD\0";
   fn.lpstrFile = textbuf;
   fn.nMaxFile = _countof(textbuf);
   fn.lpstrTitle = "Select image file for HDD emulator";
   fn.Flags = OFN_CREATEPROMPT | OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST;
   fn.lpstrInitialDir   = temp.HddDir;
   if (!GetOpenFileName(&fn))
       return;
   strcpy(temp.HddDir, fn.lpstrFile);
   char *Ptr = strrchr(temp.HddDir, '\\');
   if (Ptr)
    *Ptr = 0;

   int file = open(textbuf, O_RDONLY | O_BINARY, S_IREAD);
   if (file < 0)
       return;
   __int64 sz = _filelengthi64(file);
   close(file);

   strcpy(c1.ide[device].image, textbuf);
   c1.ide[device].c = 0;
   c1.ide[device].h = 0;
   c1.ide[device].s = 0;
   c1.ide[device].lba = unsigned(sz / 512);
   HddDlg_show_info(device);
}

void HddDlg_show_size(unsigned id, unsigned sectors)
{
   unsigned __int64 sz = ((unsigned __int64)sectors) << 9;
   char num[64]; int ptr = 0, tri = 0;
   for (;;) {
      num[ptr++] = (u8)(sz % 10) + '0';
      sz /= 10; if (!sz) break;
      if (++tri == 3) num[ptr++] = ',', tri = 0;
   }
   char dst[64]; dst[0] = '-'; dst[1] = ' ';
   int k; //Alone Coder 0.36.7
   for (/*int*/ k = 2; ptr; k++) dst[k] = num[--ptr];
   strcpy(dst+k, " bytes");
   SetDlgItemText(dlg, id, dst);
}

INT_PTR CALLBACK HddDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   NMHDR *nm = (NMHDR*)lp;
   volatile static char block=0;
   if (msg == WM_INITDIALOG)
   {
      HWND box = GetDlgItem(dlg, IDC_IDESCHEME);
      ComboBox_AddString(box, "NONE");
      ComboBox_AddString(box, "NEMO");
      ComboBox_AddString(box, "NEMO (A8)");
      ComboBox_SetItemData(box, 0, (LPARAM)ide_scheme::none);
      ComboBox_SetItemData(box, 1, (LPARAM)ide_scheme::nemo);
      ComboBox_SetItemData(box, 2, (LPARAM)ide_scheme::nemo_a8);
   }
   if (msg == WM_COMMAND && !block)
   {
	   const unsigned id = LOWORD(wp);
      const unsigned code = HIWORD(wp);
      if (code == CBN_SELCHANGE && id == IDC_IDESCHEME)
      {
	      const HWND box = GetDlgItem(dlg, IDC_IDESCHEME);
	      const int Idx = ComboBox_GetCurSel(box);
         c1.ide_scheme = (ide_scheme)ComboBox_GetItemData(box, Idx);
         HddDlg_set_active();
      }
      if (id == IDB_HDD0) HddDlg_select_image(0);
      if (id == IDB_HDD1) HddDlg_select_image(1);

      if (code == EN_CHANGE)
      {
         char bf[64]; unsigned c=0, h=0, s=0, l=0;
         GetWindowText((HWND)lp, bf, sizeof bf);
         sscanf(bf, "%d/%d/%d", &c, &h, &s);
         sscanf(bf, "%d", &l);
         switch (id)
         {
            case IDE_HDD0_CHS: HddDlg_show_size(IDS_HDD0_CHS, c*h*s); break;
            case IDE_HDD0_LBA: HddDlg_show_size(IDS_HDD0_LBA, l); break;
            case IDE_HDD1_CHS: HddDlg_show_size(IDS_HDD1_CHS, c*h*s); break;
            case IDE_HDD1_LBA: HddDlg_show_size(IDS_HDD1_LBA, l); break;
         }
      }

      return 1;
   }
   if (msg != WM_NOTIFY) return 0;
   if (nm->code == PSN_KILLACTIVE) {
      // ide_scheme read in CBN_SELCHANGE
      // image read in 'select drive/image' button click
      c1.ide[0].readonly = getcheck(IDC_HDD0_RO);
      c1.ide[1].readonly = getcheck(IDC_HDD1_RO);
      for (unsigned device = 0; device < 2; device++)
         if (*c1.ide[device].image && *c1.ide[device].image != '<') {
            char textbuf[64]; unsigned c=0, h=0, s=0, l=0;
            GetDlgItemText(dlg, device? IDE_HDD1_LBA : IDE_HDD0_LBA, textbuf, sizeof textbuf);
            sscanf(textbuf, "%d", &c1.ide[device].lba);
            GetDlgItemText(dlg, device? IDE_HDD1_CHS : IDE_HDD0_CHS, textbuf, sizeof textbuf);
            sscanf(textbuf, "%d/%d/%d", &c1.ide[device].c, &c1.ide[device].h, &c1.ide[device].s);
         }

   }
   if (nm->code == PSN_SETACTIVE)
   {
      block=1;
      const HWND box = GetDlgItem(dlg, IDC_IDESCHEME);
      const int cnt = ComboBox_GetCount(box);
      for (int i = 0; i < cnt; i++)
      {
	      auto data = (ULONG_PTR)ComboBox_GetItemData(box, i);
          if (static_cast<ide_scheme>(data) == c1.ide_scheme)
          {
              ComboBox_SetCurSel(box, i);
              break;
          }
      }
      HddDlg_set_active();
      block=0;
      setcheck(IDC_HDD0_RO, c1.ide[0].readonly);
      setcheck(IDC_HDD1_RO, c1.ide[1].readonly);
      HddDlg_show_info(0);
      HddDlg_show_info(1);
      lastpage = "HDD";
      return 1;
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK EFF7Dlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   NMHDR *nm = (NMHDR*)lp;
   if (msg != WM_NOTIFY) return 0;
   static int bits[] = { IDC_BIT0, IDC_BIT1, IDC_BIT2, IDC_BIT3,
                         IDC_BIT4, IDC_BIT5, IDC_BIT6, IDC_BIT7 };
   static int lock[] = { IDC_LOCK0, IDC_LOCK1, IDC_LOCK2, IDC_LOCK3,
                         IDC_LOCK4, IDC_LOCK5, IDC_LOCK6, IDC_LOCK7 };
   if (nm->code == PSN_KILLACTIVE) {
      unsigned mask = 0, eff7 = 0;
      for (unsigned i = 0; i < 8; i++) {
         if (getcheck(lock[i])) mask |= (1<<i);
         if (getcheck(bits[i])) eff7 |= (1<<i);
      }
      c1.EFF7_mask = mask, comp.pEFF7 = eff7;
   }
   if (nm->code == PSN_SETACTIVE) {
      for (unsigned i = 0; i < 8; i++) {
         setcheck(lock[i], c1.EFF7_mask & (1<<i));
         setcheck(bits[i], comp.pEFF7 & (1<<i));
      }
      lastpage = "EFF7";
      return 1;
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK ChipDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   if (msg == WM_INITDIALOG) {
      unsigned i; HWND aybox;

      aybox = GetDlgItem(dlg, IDC_CHIP_BUS);
      for (i = 0; i < SNDCHIP::CHIP_MAX; i++)
         SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)SNDCHIP::get_chipname((SNDCHIP::CHIP_TYPE)i));

      aybox = GetDlgItem(dlg, IDC_CHIP_SCHEME);
      for (i = 0; i < (int)ay_scheme::max_value; i++)
         SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)ay_schemes[i]);

      aybox = GetDlgItem(dlg, IDC_CHIP_VOL);
      for (u8 UCi = 0; UCi < num_ayvols; UCi++) //Alone Coder
         SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)ayvols[UCi]); //Alone Coder

      aybox = GetDlgItem(dlg, IDC_CHIP_STEREO);
      for (i = 0; i < num_aystereo; i++)
         SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)aystereo[i]);

      aybox = GetDlgItem(dlg, IDC_CHIP_CLK);
      SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)"1774400");
      SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)"3500000");
      SendMessage(aybox, CB_ADDSTRING, 0, (LPARAM)"1750000");
   }
   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      c1.sound.ayfq = getint(IDC_CHIP_CLK);
      c1.sound.ay_chip = (u8)SendDlgItemMessage(dlg, IDC_CHIP_BUS, CB_GETCURSEL, 0, 0);
      c1.sound.ay_scheme = (ay_scheme)SendDlgItemMessage(dlg, IDC_CHIP_SCHEME, CB_GETCURSEL, 0, 0);
      c1.sound.ay_vols = (u8)SendDlgItemMessage(dlg, IDC_CHIP_VOL, CB_GETCURSEL, 0, 0);
      c1.sound.ay_stereo = (u8)SendDlgItemMessage(dlg, IDC_CHIP_STEREO, CB_GETCURSEL, 0, 0);
      c1.sound.ay_samples = getcheck(IDC_CHIP_DIGITAL);
   }
   if (nm->code == PSN_SETACTIVE) {
      setint(IDC_CHIP_CLK, c1.sound.ayfq);
      SendDlgItemMessage(dlg, IDC_CHIP_BUS, CB_SETCURSEL, c1.sound.ay_chip, 0);
      SendDlgItemMessage(dlg, IDC_CHIP_SCHEME, CB_SETCURSEL, (int)c1.sound.ay_scheme, 0);
      SendDlgItemMessage(dlg, IDC_CHIP_VOL, CB_SETCURSEL, c1.sound.ay_vols, 0);
      SendDlgItemMessage(dlg, IDC_CHIP_STEREO, CB_SETCURSEL, c1.sound.ay_stereo, 0);
      setcheck(IDC_CHIP_DIGITAL, c1.sound.ay_samples);
      lastpage = "AY";
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK VideoDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg; unsigned id, code;
   int i;
   if (msg == WM_INITDIALOG)
   {
      HWND box = GetDlgItem(dlg, IDC_VIDEOFILTER);

      for (i = 0; renders[i].func; i++)   // !!! i < countof(drivers)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)renders[i].name);
      SendMessage(box, CB_SETCURSEL, c1.render, 0);

      box = GetDlgItem(dlg, IDC_BORDERSIZE);
      for (i = 0; bordersizes[i].name; i++)   // !!! i < countof(drivers)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)bordersizes[i].name);
      SendMessage(box, CB_SETCURSEL, c1.bordersize, 0);

      box = GetDlgItem(dlg, IDC_RENDER);
      for (i = 0; i < countof(drivers); i++)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)drivers[i].name);
      SendMessage(box, CB_SETCURSEL, c1.driver, 0);

      box = GetDlgItem(dlg, IDC_PALETTE);
      for (i = 0; i < (int)c1.num_pals; i++)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)pals[i].name);
      SendMessage(box, CB_SETCURSEL, c1.pal, 0);

      box = GetDlgItem(dlg, IDC_VDAC);
      for (i = 0; ts_vdac_names[i].name; i++)   // !!! i < countof(drivers)
         SendMessage(box, CB_ADDSTRING, 0, (LPARAM)ts_vdac_names[i].name);
      SendMessage(box, CB_SETCURSEL, comp.ts.vdac, 0);

      /*box = GetDlgItem(dlg, IDC_FONTHEIGHT);
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"5pix, scroll");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"6pix, scroll");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"7pix, scroll");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"8pix, scroll");
      SendMessage(box, CB_ADDSTRING, 0, (LPARAM)"8pix, fixed");*/
      unsigned index = c1.fontsize - 5;
      if (!c1.pixelscroll && index == 3) index++;
      SendMessage(box, CB_SETCURSEL, index, 0);

      for (int i = 0; i < sizeof(sshot_ext) / sizeof(sshot_ext[0]); i++)
      {
          SendDlgItemMessage(dlg, IDC_SCRSHOT, CB_ADDSTRING, 0, (LPARAM)sshot_ext[i]);
      }
      SendDlgItemMessage(dlg, IDC_SCRSHOT, CB_SETCURSEL, (int)conf.scrshot, 0);

      goto filter_changed;
   }
   if (msg == WM_COMMAND) {
      id = LOWORD(wp), code = HIWORD(wp);
      //if (id == IDC_FONT) { font_setup(dlg); return 1; }
      if ((id == IDC_NOFLIC /*|| id == IDC_FAST_SL*/) && code == BN_CLICKED) goto filter_changed;
      if (code == CBN_SELCHANGE && id == IDC_BORDERSIZE) {
         c1.bordersize = (u8)SendDlgItemMessage(dlg, IDC_BORDERSIZE, CB_GETCURSEL, 0, 0);
         goto filter_changed;
	  }
	  if (code == CBN_SELCHANGE && id == IDC_VDAC) {
          comp.ts.vdac = ts_vdac_names[(u8)SendDlgItemMessage(dlg, IDC_VDAC, CB_GETCURSEL, 0, 0)].value;
          goto filter_changed;
	  }
      if (code == CBN_SELCHANGE && id == IDC_VIDEOFILTER) {
   filter_changed:
         unsigned filt_n = SendDlgItemMessage(dlg, IDC_VIDEOFILTER, CB_GETCURSEL, 0, 0);
         DWORD f = renders[filt_n].flags;
         render_func rend = renders[filt_n].func;

		 DWORD sh;
         /*sh = (f & (RF_USE32AS16 | RF_USEC32)) ? SW_SHOW : SW_HIDE;
         ShowWindow(GetDlgItem(dlg, IDC_CH_TITLE), sh);
         ShowWindow(GetDlgItem(dlg, IDC_CH2), sh);
         ShowWindow(GetDlgItem(dlg, IDC_CH4), sh);
         ShowWindow(GetDlgItem(dlg, IDC_CH_AUTO), sh);
         sh = !sh;
         ShowWindow(GetDlgItem(dlg, IDC_B_TITLE), sh);
         ShowWindow(GetDlgItem(dlg, IDC_B0), sh);
         ShowWindow(GetDlgItem(dlg, IDC_B1), sh);
         ShowWindow(GetDlgItem(dlg, IDC_B2), sh);*/

         sh = (f & RF_BORDER)? SW_HIDE : SW_SHOW;
         ShowWindow(GetDlgItem(dlg, IDC_FLASH), sh);

         //if (!(f & RF_2X) || getcheck(IDC_FAST_SL) || !getcheck(IDC_NOFLIC)) sh = SW_HIDE;
         ShowWindow(GetDlgItem(dlg, IDC_ALT_NOFLIC), sh);

         /*sh = (f & (RF_USEFONT)) ? SW_SHOW : SW_HIDE;
         ShowWindow(GetDlgItem(dlg, IDC_FNTTITLE), sh);
         ShowWindow(GetDlgItem(dlg, IDC_FONTHEIGHT), sh);
         ShowWindow(GetDlgItem(dlg, IDC_FONT), sh);*/

         sh = (f & RF_DRIVER)? SW_SHOW : SW_HIDE;
         ShowWindow(GetDlgItem(dlg, IDC_REND_TITLE), sh);
         ShowWindow(GetDlgItem(dlg, IDC_RENDER), sh);

         sh = (f & RF_2X) && (f & (RF_DRIVER | RF_USEC32))? SW_SHOW : SW_HIDE;
         //ShowWindow(GetDlgItem(dlg, IDC_FAST_SL), sh);

         // update CLUT
         for (i = 0; i < 256; i++)
           update_clut(i);
      }
      return 1;
   }

   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;

   if (nm->code == PSN_KILLACTIVE)
   {
      /*unsigned index = SendDlgItemMessage(dlg, IDC_FONTHEIGHT, CB_GETCURSEL, 0, 0);
      c1.pixelscroll = (index == 4)? 0 : 1;
      c1.fontsize = (index == 4)? 8 : index + 5;*/
      c1.render = SendDlgItemMessage(dlg, IDC_VIDEOFILTER, CB_GETCURSEL, 0, 0);
      c1.driver = SendDlgItemMessage(dlg, IDC_RENDER, CB_GETCURSEL, 0, 0);
      c1.frameskip = getint(IDE_SKIP1);
      c1.minres = getint(IDE_MINX);
      c1.frameskipmax = getint(IDE_SKIP2);
      c1.scanbright = getint(IDE_SCBRIGHT);
      //c1.fast_sl = getcheck(IDC_FAST_SL);
      c1.scrshot = (sshot_format)SendDlgItemMessage(dlg, IDC_SCRSHOT, CB_GETCURSEL, 0, 0);
      c1.flip = getcheck(IDC_FLIP);
      //c1.updateb = getcheck(IDC_UPDB);
      c1.pal = SendDlgItemMessage(dlg, IDC_PALETTE, CB_GETCURSEL, 0, 0);
      c1.flashcolor = getcheck(IDC_FLASH);
      c1.noflic = getcheck(IDC_NOFLIC);
      c1.alt_nf = getcheck(IDC_ALT_NOFLIC);
      //c1.videoscale = (u8)(SendDlgItemMessage(dlg, IDC_VIDEOSCALE, TBM_GETPOS, 0, 0));
   }

   if (nm->code == PSN_SETACTIVE)
   {
      setint(IDE_SKIP1, c1.frameskip);
      setint(IDE_SKIP2, c1.frameskipmax);
      setint(IDE_MINX, c1.minres);
      setint(IDE_SCBRIGHT, c1.scanbright);

      SendDlgItemMessage(dlg, IDC_SCRSHOT, CB_SETCURSEL, (int)c1.scrshot, 0);

      setcheck(IDC_FLIP, c1.flip);
      //setcheck(IDC_UPDB, c1.updateb);
      setcheck(IDC_FLASH, c1.flashcolor);
      setcheck(IDC_NOFLIC, c1.noflic);
      setcheck(IDC_ALT_NOFLIC, c1.alt_nf);
      //setcheck(IDC_FAST_SL, c1.fast_sl);

      /*SendDlgItemMessage(dlg, IDC_VIDEOSCALE, TBM_SETRANGE, 0, MAKELONG(1,4));
      SendDlgItemMessage(dlg, IDC_VIDEOSCALE, TBM_SETPOS, 1, c1.videoscale);*/

      lastpage = "VIDEO";
      goto filter_changed;
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

static struct
{
   unsigned ID;
   int *value;
} slider[] = {
   { IDC_SND_BEEPER,		&c1.sound.beeper_vol		},
   { IDC_SND_MICOUT,		&c1.sound.micout_vol		},
   { IDC_SND_MICIN,			&c1.sound.micin_vol			},
   { IDC_SND_AY,			&c1.sound.ay_vol			},
   { IDC_SND_SAA,			&c1.sound.saa1099_vol		},
   { IDC_SND_COVOXFB,		&c1.sound.covoxFB_vol		},
   { IDC_SND_COVOXDD,		&c1.sound.covoxDD_vol		},
   { IDC_SND_COVOXPROFI,	&c1.sound.covoxProfi_vol	},
   { IDC_SND_SD,			&c1.sound.sd_vol			},
   { IDC_SND_BASS,			&c1.sound.bass_vol			},
   { IDC_SND_GS,			&c1.sound.gs_vol			},
   { IDC_SND_MOONSOUND,		&c1.sound.moonsound_vol		},
};

INT_PTR CALLBACK SoundDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   if (msg == WM_INITDIALOG)
   {
      unsigned savemod = 0, reset = 0, fx_vol = 0, bass_vol = 0;
//      unsigned here_soundfilter = 1; //Alone Coder 0.36.4
      #ifdef MOD_GS
      if (c1.gs_type)
      {
         reset = 1;

         #ifdef MOD_GSZ80
         if (c1.gs_type == 1) fx_vol = 1;
         #endif

         #ifdef MOD_GSBASS
         if (c1.gs_type == 2) {
            fx_vol = bass_vol = 1;
            if (gs.mod && gs.modsize) savemod = 1;
         }
         #endif
      }
      #endif

//      EnableWindow(GetDlgItem(dlg, IDC_SOUNDFILTER), here_soundfilter); //Alone Coder 0.36.4

      EnableWindow(GetDlgItem(dlg, IDC_GSRESET), reset);
      EnableWindow(GetDlgItem(dlg, IDB_SAVEMOD), savemod);
      EnableWindow(GetDlgItem(dlg, IDC_GS_TITLE), fx_vol);
      EnableWindow(GetDlgItem(dlg, IDC_SND_GS), fx_vol);
      EnableWindow(GetDlgItem(dlg, IDC_SND_BASS), bass_vol);
      EnableWindow(GetDlgItem(dlg, IDC_BASS_TITLE), bass_vol);
   }
   if (msg == WM_COMMAND && LOWORD(wp) == IDC_NOSOUND) {
      c1.sound.enabled = !getcheck(IDC_NOSOUND);
upd:  for (int i = 0; i < sizeof slider/sizeof*slider; i++) {
         SendDlgItemMessage(dlg, slider[i].ID, TBM_SETRANGE, 0, MAKELONG(0,8192));
         SendDlgItemMessage(dlg, slider[i].ID, TBM_SETPOS, 1, c1.sound.enabled ? *slider[i].value : 0);
         SendDlgItemMessage(dlg, slider[i].ID, WM_ENABLE, c1.sound.enabled, 0);
      }
      return 1;
   }

   #ifdef MOD_GSBASS
   if (msg == WM_COMMAND && LOWORD(wp) == IDB_SAVEMOD) {
      OPENFILENAME ofn = { 0 };
      char fname[0x200]; strncpy(fname, (char*)gs.mod, 20); fname[20] = 0;
      for (char *ptr = fname; *ptr; ptr++)
         if (*ptr == '|' || *ptr == '<' || *ptr == '>' ||
             *ptr == '?' || *ptr == '/' || *ptr == '\\' ||
             *ptr == '"' || *ptr == ':' || *ptr == '*' || *(u8*)ptr < ' ')
            *ptr = ' ';
      ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
      ofn.lpstrFilter = "Amiga music module (MOD)\0*.mod\0";
      ofn.lpstrFile = fname; ofn.nMaxFile = sizeof fname;
      ofn.lpstrTitle = "Save music from GS";
      ofn.lpstrDefExt = "mod";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
      ofn.hwndOwner = dlg;
      if (GetSaveFileName(&ofn)) {
         FILE *ff = fopen(fname, "wb");
         if (ff) {
            fwrite(gs.mod, 1, gs.modsize, ff);
            fclose(ff);
         }
      }
      return 1;
   }
   #endif

   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      if (c1.sound.enabled = (IsDlgButtonChecked(dlg, IDC_NOSOUND) != BST_CHECKED))
         for (int i = 0; i < sizeof slider/sizeof*slider; i++)
            *slider[i].value = SendDlgItemMessage(dlg, slider[i].ID, TBM_GETPOS, 0, 0);
      c1.sound.gsreset = getcheck(IDC_GSRESET);
      c1.soundfilter = getcheck(IDC_SOUNDFILTER); //Alone Coder 0.36.4
   }
   if (nm->code == PSN_SETACTIVE) {
      setcheck(IDC_NOSOUND, !c1.sound.enabled);
      setcheck(IDC_GSRESET, c1.sound.gsreset);
      setcheck(IDC_SOUNDFILTER, c1.soundfilter); //Alone Coder 0.36.4
      lastpage = "SOUND";
      goto upd;
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK TapeDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   if (msg == WM_INITDIALOG) {
      find_tape_index();
      for (unsigned i = 0; i < tape_infosize; i++)
         SendDlgItemMessage(dlg, IDC_TAPE, LB_ADDSTRING, 0, (LPARAM)tapeinfo[i].desc);
   }
   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      comp.tape.index = SendDlgItemMessage(dlg, IDC_TAPE, LB_GETCURSEL, 0, 0);
      c1.tape_autostart = getcheck(IDC_TAPE_AUTOSTART);
	  c1.tape_traps = getcheck(IDC_TAPE_TRAPS);
   }
   if (nm->code == PSN_SETACTIVE) {
      SendDlgItemMessage(dlg, IDC_TAPE, LB_SETCURSEL, comp.tape.index, 0);
      setcheck(IDC_TAPE_AUTOSTART, c1.tape_autostart);
	  setcheck(IDC_TAPE_TRAPS, c1.tape_traps);
      lastpage = "TAPE";
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}


void FillModemList(HWND box)
{
   ComboBox_AddString(box, "NONE");
   for (unsigned port = 1; port < 256; port++)
   {
      HANDLE hPort;
      if (zf232.rs_open_port == port)
          hPort = zf232.rs_h_port;
      else
      {
         char portName[11];
         _snprintf(portName, _countof(portName), "\\\\.\\COM%d", port);

         hPort = CreateFile(portName, 0, 0, 0, OPEN_EXISTING, 0, 0);
         if (hPort == INVALID_HANDLE_VALUE)
             continue;
      }

      struct
      {
         COMMPROP comm;
         char xx[4000];
      } b;

      b.comm.wPacketLength = sizeof(b);
      b.comm.dwProvSpec1 = COMMPROP_INITIALIZED;
      if (GetCommProperties(hPort, &b.comm) && b.comm.dwProvSubType == PST_MODEM)
      {
         MODEMDEVCAPS *mc = (MODEMDEVCAPS*)&b.comm.wcProvChar;
         char vendor[0x100], model[0x100];

         unsigned vsize = mc->dwModemManufacturerSize / sizeof(WCHAR);
         WideCharToMultiByte(CP_ACP, 0, (WCHAR*)(PCHAR(mc) + mc->dwModemManufacturerOffset), vsize, vendor, sizeof vendor, 0, 0);
         vendor[vsize] = 0;

         unsigned msize = mc->dwModemModelSize / sizeof(WCHAR);
         WideCharToMultiByte(CP_ACP, 0, (WCHAR*)(PCHAR(mc) + mc->dwModemModelOffset), msize, model, sizeof model, 0, 0);
         model[msize] = 0;
         char line[0x200];
         _snprintf(line, _countof(line), "COM%d: %s %s", port, vendor, model);
         ComboBox_AddString(box, line);
      }
      else
      {
         char portName[11];
         _snprintf(portName, _countof(portName), "COM%d:", port);
         ComboBox_AddString(box, portName);
      }
      if (zf232.rs_open_port != port)
          CloseHandle(hPort);
   }
}

void SelectModem(HWND box)
{
   if (!c1.modem_port)
   {
       ComboBox_SetCurSel(box, 0);
       return;
   }

   char line[0x200];
   int Cnt = ComboBox_GetCount(box);
   for (int i = 0; i < Cnt; i++)
   {
      ComboBox_GetLBText(box, i, line);
      int Port = 0;
      sscanf(line, "COM%d", &Port);
      if (Port == c1.modem_port)
      {
         SendMessage(box, CB_SETCURSEL, i, 0);
         ComboBox_SetCurSel(box, i);
         return;
      }
   }
}

int GetModemPort(HWND box)
{
   int index = ComboBox_GetCurSel(box);
   if (!index)
       return 0;

   char line[0x200];
   ComboBox_GetLBText(box, index, line);
   int Port = 0;
   sscanf(line, "COM%d", &Port);
   return Port;
}

INT_PTR CALLBACK InputDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg; char names[0x2000];
   if (msg == WM_INITDIALOG) {
      zxkeymap *active_zxk = conf.input.active_zxk;
      for (unsigned i = 0; i < active_zxk->zxk_size; i++)
         SendDlgItemMessage(dlg, IDC_FIREKEY, CB_ADDSTRING, 0, (LPARAM)active_zxk->zxk[i].name);
      GetPrivateProfileSectionNames(names, sizeof names, ininame);
      for (char *ptr = names; *ptr; ptr += strlen(ptr)+1)
         if (!strnicmp(ptr, "ZX.KEYS.", sizeof("ZX.KEYS.")-1)) {
            char line[0x200]; GetPrivateProfileString(ptr, "Name", ptr, line, sizeof line, ininame);
            SendDlgItemMessage(dlg, IDC_KLAYOUT, CB_ADDSTRING, 0, (LPARAM)line);
         }
      FillModemList(GetDlgItem(dlg, IDC_MODEM));
   }
   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      if (getcheck(IDC_MOUSE_NONE)) c1.input.mouse = 0;
      if (getcheck(IDC_MOUSE_KEMPSTON)) c1.input.mouse = 1;
      if (getcheck(IDC_MOUSE_AY)) c1.input.mouse = 2;
      if (getcheck(IDC_WHEEL_NONE)) c1.input.mousewheel = mouse_wheel_mode::none;
      if (getcheck(IDC_WHEEL_KEYBOARD)) c1.input.mousewheel = mouse_wheel_mode::keyboard;
      if (getcheck(IDC_WHEEL_KEMPSTON)) c1.input.mousewheel = mouse_wheel_mode::kempston;
      c1.input.keybpcmode = getcheck(IDC_PC_LAYOUT);
      c1.input.mouseswap = getcheck(IDC_MOUSESWAP);
      c1.input.kjoy = getcheck(IDC_KJOY);
      c1.input.keymatrix = getcheck(IDC_KEYMATRIX);
      c1.input.mousescale = (char)(SendDlgItemMessage(dlg, IDC_MOUSESCALE, TBM_GETPOS, 0, 0) - 3);
      c1.input.joymouse = getcheck(IDC_JOYMOUSE);
      c1.input.firenum = SendDlgItemMessage(dlg, IDC_FIREKEY, CB_GETCURSEL, 0, 0);
      c1.input.fire = getcheck(IDC_AUTOFIRE);
      c1.input.firedelay = getint(IDE_FIRERATE);
      c1.input.altlock = getcheck(IDC_ALTLOCK);
      c1.input.paste_hold = getint(IDE_HOLD_DELAY);
      c1.input.paste_release = getint(IDE_RELEASE_DELAY);
      c1.input.paste_newline = getint(IDE_NEWLINE_DELAY);
      c1.modem_port = GetModemPort(GetDlgItem(dlg, IDC_MODEM));
      GetPrivateProfileSectionNames(names, sizeof names, ininame);
      int n = SendDlgItemMessage(dlg, IDC_KLAYOUT, CB_GETCURSEL, 0, 0), i = 0;
      for (char *ptr = names; *ptr; ptr += strlen(ptr)+1)
         if (!strnicmp(ptr, "ZX.KEYS.", sizeof("ZX.KEYS.")-1)) {
            if (i == n) strcpy(c1.keyset, ptr+sizeof("ZX.KEYS.")-1);
            i++;
         }
   }
   if (nm->code == PSN_SETACTIVE) {
      setcheck(IDC_MOUSE_NONE, c1.input.mouse == 0);
      setcheck(IDC_MOUSE_KEMPSTON, c1.input.mouse == 1);
      setcheck(IDC_MOUSE_AY, c1.input.mouse == 2);
      setcheck(IDC_PC_LAYOUT, c1.input.keybpcmode);
      setcheck(IDC_WHEEL_NONE, c1.input.mousewheel == mouse_wheel_mode::none);
      setcheck(IDC_WHEEL_KEYBOARD, c1.input.mousewheel == mouse_wheel_mode::keyboard);
      setcheck(IDC_WHEEL_KEMPSTON, c1.input.mousewheel == mouse_wheel_mode::kempston);
      setcheck(IDC_MOUSESWAP, c1.input.mouseswap);
      setcheck(IDC_KJOY, c1.input.kjoy);
      setcheck(IDC_KEYMATRIX, c1.input.keymatrix);
      setcheck(IDC_JOYMOUSE, c1.input.joymouse);
      setcheck(IDC_AUTOFIRE, c1.input.fire);
      setcheck(IDC_ALTLOCK, c1.input.altlock);
      SendDlgItemMessage(dlg, IDC_MOUSESCALE, TBM_SETRANGE, 0, MAKELONG(0,6));
      SendDlgItemMessage(dlg, IDC_MOUSESCALE, TBM_SETPOS, 1, c1.input.mousescale+3);
      SendDlgItemMessage(dlg, IDC_FIREKEY, CB_SETCURSEL, c1.input.firenum, 0);
      setint(IDE_FIRERATE, c1.input.firedelay);
      setint(IDE_HOLD_DELAY, c1.input.paste_hold);
      setint(IDE_RELEASE_DELAY, c1.input.paste_release);
      setint(IDE_NEWLINE_DELAY, c1.input.paste_newline);
      SelectModem(GetDlgItem(dlg, IDC_MODEM));
      GetPrivateProfileSectionNames(names, sizeof names, ininame);
      int i = 0;
      for (char *ptr = names; *ptr; ptr += strlen(ptr)+1)
         if (!strnicmp(ptr, "ZX.KEYS.", sizeof("ZX.KEYS.")-1)) {
            if (!strnicmp(c1.keyset, ptr+sizeof("ZX.KEYS.")-1, strlen(c1.keyset)))
               SendDlgItemMessage(dlg, IDC_KLAYOUT, CB_SETCURSEL, i, 0);
            i++;
         }
      lastpage = "INPUT";
   }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK LedsDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   static int ids[NUM_LEDS][3] = {
     { IDC_LED_AY, IDC_LED_AY_X, IDC_LED_AY_Y },
     { IDC_LED_PERF, IDC_LED_PERF_X, IDC_LED_PERF_Y },
     { IDC_LED_LOAD, IDC_LED_ROM_X, IDC_LED_ROM_Y },
     { IDC_LED_INPUT, IDC_LED_INPUT_X, IDC_LED_INPUT_Y },
     { IDC_LED_TIME, IDC_LED_TIME_X, IDC_LED_TIME_Y },
     { IDC_LED_DEBUG, IDC_LED_DEBUG_X, IDC_LED_DEBUG_Y },
     { IDC_LED_MEMBAND, IDC_LED_MEMBAND_X, IDC_LED_MEMBAND_Y }
   };
   volatile static char block=0;
   ::dlg = dlg;
   if (msg == WM_USER || (!block && msg == WM_COMMAND && (HIWORD(wp)==EN_CHANGE || HIWORD(wp)==BN_CLICKED)))
   {
      u8 ld_on = getcheck(IDC_LED_ON);
      c1.led.enabled = ld_on;
      c1.led.perf_t = getcheck(IDC_PERF_T);
      c1.led.flash_ay_kbd = getcheck(IDC_LED_AYKBD);
      for (unsigned i = 0; i < NUM_LEDS; i++) {
         char b1[16], b2[16];
         SendDlgItemMessage(dlg, ids[i][1], WM_GETTEXT, sizeof b1, (LPARAM)b1);
         SendDlgItemMessage(dlg, ids[i][2], WM_GETTEXT, sizeof b2, (LPARAM)b2);
         if (!*b1 || !*b2) continue; // skip first notification with empty controls
         unsigned a = (atoi(b1) & 0xFFFF) + ((atoi(b2) & 0x7FFF) << 16);
         if (IsDlgButtonChecked(dlg, ids[i][0]) == BST_CHECKED) a |= 0x80000000;
         u8 x = ld_on && (a & 0x80000000);
         EnableWindow(GetDlgItem(dlg, ids[i][0]), ld_on);
         EnableWindow(GetDlgItem(dlg, ids[i][1]), x);
         EnableWindow(GetDlgItem(dlg, ids[i][2]), x);
         (&c1.led.ay)[i] = a;
      }
      EnableWindow(GetDlgItem(dlg, IDC_LED_AYKBD), (ld_on && hndKbdDev));
      EnableWindow(GetDlgItem(dlg, IDC_PERF_T), ld_on && (c1.led.perf & 0x80000000));
      EnableWindow(GetDlgItem(dlg, IDC_LED_BPP), ld_on && (c1.led.memband & 0x80000000));

      #ifndef MOD_MONITOR
      c1.led.osw &= 0x7FFFFFFF;
      EnableWindow(GetDlgItem(dlg, IDC_LED_DEBUG), 0);
      #endif

      #ifndef MOD_MEMBAND_LED
      c1.led.memband &= 0x7FFFFFFF;
      EnableWindow(GetDlgItem(dlg, IDC_LED_MEMBAND), 0);
      #endif
   }
   if (msg != WM_NOTIFY) return 0;
   NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      unsigned pos = SendDlgItemMessage(dlg, IDC_LED_BPP, TBM_GETPOS, 0, 0);
      if (pos == 0) c1.led.bandBpp = 64;
      else if (pos == 1) c1.led.bandBpp = 128;
      else if (pos == 2) c1.led.bandBpp = 256;
      else c1.led.bandBpp = 512;
   }
   if (nm->code == PSN_SETACTIVE) {
      block = 1;
      setcheck(IDC_LED_ON, c1.led.enabled);
      setcheck(IDC_LED_AYKBD, c1.led.flash_ay_kbd);
      setcheck(IDC_PERF_T, c1.led.perf_t);
      unsigned pos = 3;
      if (c1.led.bandBpp == 64) pos = 0;
      if (c1.led.bandBpp == 128) pos = 1;
      if (c1.led.bandBpp == 256) pos = 2;
      SendDlgItemMessage(dlg, IDC_LED_BPP, TBM_SETRANGE, 0, MAKELONG(0,3));
      SendDlgItemMessage(dlg, IDC_LED_BPP, TBM_SETPOS, 1, pos);
      for (unsigned i = 0; i < NUM_LEDS; i++) {
         unsigned a = (&c1.led.ay)[i];
         char bf[16]; sprintf(bf, "%d", (i16)(a & 0xFFFF));
         SendDlgItemMessage(dlg, ids[i][1], WM_SETTEXT, 0, (LPARAM)bf);
         sprintf(bf, "%d", (i16)(((a >> 16) & 0x7FFF) + ((a >> 15) & 0x8000)));
         SendDlgItemMessage(dlg, ids[i][2], WM_SETTEXT, 0, (LPARAM)bf);
         setcheck(ids[i][0], a >> 31);
      }
      LedsDlg(dlg, WM_USER, 0, 0);
      block = 0;
      lastpage = "LEDS";
   }

   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;
}

INT_PTR CALLBACK BetaDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   unsigned ID = LOWORD(wp);
   if (msg == WM_INITDIALOG)
   {
      setcheck(IDC_DISK_TRAPS, c1.trdos_traps);
   }
   if (msg == WM_COMMAND)
   {
      int disk;
      switch (ID)
      {
         case IDB_INS_A: disk = 0; goto load;
         case IDB_INS_B: disk = 1; goto load;
         case IDB_INS_C: disk = 2; goto load;
         case IDB_INS_D: disk = 3; goto load;
         load:
            if (!comp.wd.fdd[disk].test())
                return 1;
            opensnap(disk+1);
            c1.trdos_wp[disk] = conf.trdos_wp[disk];
            goto reload;

         case IDB_REM_A: disk = 0; goto remove;
         case IDB_REM_B: disk = 1; goto remove;
         case IDB_REM_C: disk = 2; goto remove;
         case IDB_REM_D: disk = 3; goto remove;
         remove:
            if (!comp.wd.fdd[disk].test())
                return 1;
            comp.wd.fdd[disk].free();
            c1.trdos_wp[disk] = conf.trdos_wp[disk];
            goto reload;

         case IDB_SAVE_A: savesnap(0); goto reload;
         case IDB_SAVE_B: savesnap(1); goto reload;
         case IDB_SAVE_C: savesnap(2); goto reload;
         case IDB_SAVE_D: savesnap(3); goto reload;

         case IDC_BETA128:
            c1.trdos_present = getcheck(IDC_BETA128);
            goto reload;

         case IDC_DISK_TRAPS:
            c1.trdos_traps = getcheck(IDC_DISK_TRAPS); break;

         case IDC_DISK_NODELAY:
            c1.wd93_nodelay = getcheck(IDC_DISK_NODELAY); break;
      }
   }
   if (msg != WM_NOTIFY) return 0;
   {NMHDR *nm = (NMHDR*)lp;
   if (nm->code == PSN_KILLACTIVE) {
      c1.trdos_present = getcheck(IDC_BETA128);
      c1.trdos_traps = getcheck(IDC_DISK_TRAPS);
      c1.wd93_nodelay = getcheck(IDC_DISK_NODELAY);
      c1.trdos_wp[0] = getcheck(IDC_WPA);
      c1.trdos_wp[1] = getcheck(IDC_WPB);
      c1.trdos_wp[2] = getcheck(IDC_WPC);
      c1.trdos_wp[3] = getcheck(IDC_WPD);
   }
   if (nm->code == PSN_SETACTIVE) { lastpage = "Beta128"; goto reload; }
   if (nm->code == PSN_APPLY) dlgok = 1;
   if (nm->code == PSN_RESET) dlgok = 0;
   return 1;}
reload:
   SendDlgItemMessage(dlg, IDE_DISK_A, WM_SETTEXT, 0, (LPARAM)comp.wd.fdd[0].name);
   SendDlgItemMessage(dlg, IDE_DISK_B, WM_SETTEXT, 0, (LPARAM)comp.wd.fdd[1].name);
   SendDlgItemMessage(dlg, IDE_DISK_C, WM_SETTEXT, 0, (LPARAM)comp.wd.fdd[2].name);
   SendDlgItemMessage(dlg, IDE_DISK_D, WM_SETTEXT, 0, (LPARAM)comp.wd.fdd[3].name);
   setcheck(IDC_BETA128, c1.trdos_present);
   setcheck(IDC_DISK_TRAPS, c1.trdos_traps);
   setcheck(IDC_DISK_NODELAY, c1.wd93_nodelay);
   setcheck(IDC_WPA, c1.trdos_wp[0]);
   setcheck(IDC_WPB, c1.trdos_wp[1]);
   setcheck(IDC_WPC, c1.trdos_wp[2]);
   setcheck(IDC_WPD, c1.trdos_wp[3]);
   unsigned on = getcheck(IDC_BETA128);
   EnableWindow(GetDlgItem(dlg, IDC_DISK_TRAPS), on);
   EnableWindow(GetDlgItem(dlg, IDC_DISK_NODELAY), on);

   EnableWindow(GetDlgItem(dlg, IDB_INS_A), on);
   EnableWindow(GetDlgItem(dlg, IDB_INS_B), on);
   EnableWindow(GetDlgItem(dlg, IDB_INS_C), on);
   EnableWindow(GetDlgItem(dlg, IDB_INS_D), on);

   EnableWindow(GetDlgItem(dlg, IDB_REM_A), on);
   EnableWindow(GetDlgItem(dlg, IDB_REM_B), on);
   EnableWindow(GetDlgItem(dlg, IDB_REM_C), on);
   EnableWindow(GetDlgItem(dlg, IDB_REM_D), on);

   EnableWindow(GetDlgItem(dlg, IDB_SAVE_A), on && comp.wd.fdd[0].rawdata);
   EnableWindow(GetDlgItem(dlg, IDB_SAVE_B), on && comp.wd.fdd[1].rawdata);
   EnableWindow(GetDlgItem(dlg, IDB_SAVE_C), on && comp.wd.fdd[2].rawdata);
   EnableWindow(GetDlgItem(dlg, IDB_SAVE_D), on && comp.wd.fdd[3].rawdata);

   ShowWindow(GetDlgItem(dlg, IDC_MODA), comp.wd.fdd[0].optype? SW_SHOW : SW_HIDE);
   ShowWindow(GetDlgItem(dlg, IDC_MODB), comp.wd.fdd[1].optype? SW_SHOW : SW_HIDE);
   ShowWindow(GetDlgItem(dlg, IDC_MODC), comp.wd.fdd[2].optype? SW_SHOW : SW_HIDE);
   ShowWindow(GetDlgItem(dlg, IDC_MODD), comp.wd.fdd[3].optype? SW_SHOW : SW_HIDE);
   return 1;
}

void setup_dlg()
{
   PROPSHEETPAGE psp[16] = { 0 };
   PROPSHEETPAGE *ps = psp;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_MEM);
   ps->pszTitle      = "MEMORY";
   ps->pfnDlgProc    = MemDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_VIDEO);
   ps->pszTitle      = "VIDEO";
   ps->pfnDlgProc    = VideoDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_ULA);
   ps->pszTitle      = "ULA";
   ps->pfnDlgProc    = UlaDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_HDD);
   ps->pszTitle      = "HDD";
   ps->pfnDlgProc    = HddDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_EFF7);
   ps->pszTitle      = "EFF7";
   ps->pfnDlgProc    = EFF7Dlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_CHIP);
   ps->pszTitle      = "AY";
   ps->pfnDlgProc    = ChipDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_SOUND);
   ps->pszTitle      = "SOUND";
   ps->pfnDlgProc    = SoundDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_INPUT);
   ps->pszTitle      = "INPUT";
   ps->pfnDlgProc    = InputDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_TAPE);
   ps->pszTitle      = "TAPE";
   ps->pfnDlgProc    = TapeDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_BETA128);
   ps->pszTitle      = "Beta128";
   ps->pfnDlgProc    = BetaDlg;
   ps++;

   ps->pszTemplate   = MAKEINTRESOURCE(IDD_LEDS);
   ps->pszTitle      = "LEDS";
   ps->pfnDlgProc    = LedsDlg;
   ps++;

   PROPSHEETHEADER psh = { sizeof(PROPSHEETHEADER) };
   psh.dwFlags          = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW | (lastpage ? PSH_USEPSTARTPAGE : 0);
   psh.hwndParent       = wnd;
   psh.hInstance        = hIn;
   psh.pszIcon          = MAKEINTRESOURCE(IDI_MAIN);
   psh.pszCaption       = "Emulation Settings";
   psh.ppsp             = (LPCPROPSHEETPAGE)&psp;
   psh.pStartPage       = lastpage;
   psh.nPages           = ps - psp;

   for (unsigned i = 0; i < psh.nPages; i++) {
      psp[i].dwSize = sizeof(PROPSHEETPAGE);
      psp[i].hInstance = hIn;
      psp[i].dwFlags = PSP_USETITLE;
   }

   // temp.rflags = RF_MONITOR; set_video();
   sound_stop();

   c1 = conf; PropertySheet(&psh);
   if (dlgok) {
           if (conf.render != c1.render)
               temp.scale = 1;
           conf = c1;
           frametime = conf.frame; //Alone Coder 0.36.5
   };

   eat();
   SendMessage(wnd, WM_SETFOCUS, (WPARAM)wnd, 0); // show cursor for 'kempston on mouse'
   applyconfig();
   sound_play();
}

#endif

