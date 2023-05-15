#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "dx/dx.h"
#include "util.h"

u8 wavhdr[]= {
   0x52,0x49,0x46,0x46,0xcc,0xf6,0x3e,0x00,
   0x57,0x41,0x56,0x45,0x66,0x6d,0x74,0x20,
   0x10,0x00,0x00,0x00,0x01,0x00,0x02,0x00,
   0x22,0x56,0x00,0x00,0x88,0x58,0x01,0x00,
   0x04,0x00,0x10,0x00,0x64,0x61,0x74,0x61,
   0xa8,0xf6,0x3e,0x00
};
#pragma pack(1)
static struct
{
   u16 sig;
   u8 stereo;
   u16 start;
   unsigned ayfq;
   u8 intfq;
   u16 year;
   unsigned rawsize;
} vtxheader;
#pragma pack()
bool silence(unsigned pos)
{
   return !(vtxbuf[pos+8] | vtxbuf[pos+9] | vtxbuf[pos+10]) ||
          (vtxbuf[pos+7] & 0x3F) == 0x3F;
}
INT_PTR CALLBACK VtxDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp);
unsigned vtxyear, vtxchip;
char vtxname[200], vtxauthor[200], vtxsoft[200], vtxtracker[200], vtxcomm[200];

void savesnddialog()
{
   sound_stop(); //Alone Coder
   unsigned end; //Alone Coder 0.36.7
   if (savesndtype) {
      if (savesndtype == 1) { // wave
         unsigned fsize = ftell(savesnd);
         fseek(savesnd, 0, SEEK_SET);
         fsize -= sizeof wavhdr;
         *(unsigned*)(wavhdr+4) = fsize+0x2c-8;
         *(unsigned*)(wavhdr+0x28) = fsize;
         fwrite(wavhdr, 1, sizeof wavhdr, savesnd);
         MessageBox(wnd, "WAV save done", "Save sound", MB_ICONINFORMATION);
      } else { // vtx
         savesndtype = 0;
         u8 *newb = (u8*)malloc(vtxbuffilled);
         for (/*unsigned*/ end = 0; end < (int)vtxbuffilled && silence(end); end += 14);
         vtxbuffilled -= end; memcpy(vtxbuf, vtxbuf+end, vtxbuffilled);
         for (end = vtxbuffilled; end && silence(end-14); end -= 14);
         vtxbuffilled = end;
         int nrec = vtxbuffilled/14;
         for (int i = 0; i < nrec; i++)
            for (int j = 0; j < 14; j++)
               newb[j*nrec+i] = vtxbuf[i*14+j];
         free(vtxbuf);
         FILE *ff = fopen("vtx.tmp", "wb");
         if (!ff) return;
         fwrite(newb, 1, vtxbuffilled, ff);
         fclose(ff);
         STARTUPINFO si = { sizeof si };
         si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE;
         PROCESS_INFORMATION pi;
         char Parh[] = "lha a vtx.lzh vtx.tmp";
         if (CreateProcess(0, Parh, 0, 0, 0, 0, 0, 0, &si, &pi))
         {
            WaitForSingleObject(pi.hProcess, 5000);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            DeleteFile("vtx.tmp");
         }
         else
         {
            DeleteFile("vtx.tmp");
            MessageBox(wnd, "LHA.EXE not found in %PATH%", 0, MB_ICONERROR);
            return;
         }
         ff = fopen("vtx.lzh", "rb"); if (!ff) return;
         fseek(ff, 0x22, SEEK_SET);
         unsigned packed = fread(newb, 1, vtxbuffilled, ff)-1;
         fclose(ff); DeleteFile("vtx.lzh");
         DialogBox(hIn, MAKEINTRESOURCE(IDD_VTX), wnd, VtxDlg);
         vtxheader.sig = (vtxchip & 1) ? WORD2('y','m') : WORD2('a','y');
         static u8 ste[] = { 1, 2, 0 };
         vtxheader.stereo = ste[vtxchip/2];
         vtxheader.ayfq = conf.sound.ayfq;
         vtxheader.intfq = 50;
         vtxheader.year = vtxyear;
         vtxheader.rawsize = vtxbuffilled;
         fwrite(&vtxheader, 1, 0x10, savesnd);
         fwrite(vtxname, 1, strlen(vtxname)+1, savesnd);
         fwrite(vtxauthor, 1, strlen(vtxauthor)+1, savesnd);
         fwrite(vtxsoft, 1, strlen(vtxsoft)+1, savesnd);
         fwrite(vtxtracker, 1, strlen(vtxtracker)+1, savesnd);
         fwrite(vtxcomm, 1, strlen(vtxcomm)+1, savesnd);
         fwrite(newb, 1, packed, savesnd);
      }
      fclose(savesnd);
      savesndtype = 0;
   } else {
      OPENFILENAME ofn = { 0 };
      char sndsavename[0x200]; *sndsavename = 0;

      ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
      ofn.lpstrFilter = "All sound (WAV)\0*.wav\0AY sound (VTX)\0*.vtx\0";
      ofn.lpstrFile = sndsavename; ofn.nMaxFile = sizeof sndsavename;
      ofn.lpstrTitle = "Save Sound";
      ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
      ofn.hwndOwner = wnd;
      ofn.nFilterIndex = 1;
      if (GetSaveFileName(&ofn)) {
         char *name = sndsavename; for (char *x = name; *x; x++) if (*x == '\\') name = x+1;
         if (!strchr(name, '.')) {
            if (ofn.nFilterIndex == 1) strcat(sndsavename, ".wav");
            else strcat(sndsavename, ".vtx");
         }
         savesnd = fopen(ofn.lpstrFile, "wb");
         if (!savesnd) MessageBox(wnd, "Can't create file", 0, MB_ICONERROR);
         else if (ofn.nFilterIndex == 2) { // vtx
            savesndtype = 2;
            vtxbuf = 0;
         } else { // wave. all params, except fq are fixed: 16bit,stereo
            *(unsigned*)(wavhdr+0x18) = conf.sound.fq; // fq
            *(unsigned*)(wavhdr+0x1C) = conf.sound.fq*4; // bitrate
            fwrite(wavhdr, 1, 44, savesnd); // header
            savesndtype = 1;
         }
      }
   }
   eat();
   sound_play(); //Alone Coder
}

INT_PTR CALLBACK VtxDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   if (msg == WM_INITDIALOG) {
      static char chips[] = "ABC AY\0ABC YM\0ACB AY\0ACB YM\0MONO AY\0MONO YM\0";
      for (char *str = chips; *str; str += strlen(str)+1)
         SendDlgItemMessage(dlg, IDC_VTXCHIP, CB_ADDSTRING, 0, (LPARAM)str);
      unsigned c = ((conf.sound.ay_voltab[8] == conf.sound.ay_voltab[9])?0:1) /* + (conf.ay_preset>2?0:conf.ay_preset*2) */;
      SendDlgItemMessage(dlg, IDC_VTXCHIP, CB_SETCURSEL, c, 0);
      SetFocus(GetDlgItem(dlg, IDE_VTXNAME));
      return 1;
   }
   if ((msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE) ||
       (msg == WM_COMMAND && LOWORD(wp) == IDOK))
   {
      SendDlgItemMessage(dlg, IDE_VTXNAME, WM_GETTEXT, sizeof vtxname, (LPARAM)vtxname);
      SendDlgItemMessage(dlg, IDE_VTXAUTH, WM_GETTEXT, sizeof vtxauthor, (LPARAM)vtxauthor);
      SendDlgItemMessage(dlg, IDE_VTXSOFT, WM_GETTEXT, sizeof vtxsoft, (LPARAM)vtxsoft);
      SendDlgItemMessage(dlg, IDE_VTXTRACK, WM_GETTEXT, sizeof vtxtracker, (LPARAM)vtxtracker);
      SendDlgItemMessage(dlg, IDE_VTXCOMM, WM_GETTEXT, sizeof vtxcomm, (LPARAM)vtxcomm);
      vtxchip = SendDlgItemMessage(dlg, IDC_VTXCHIP, CB_GETCURSEL, 0, 0);
      char xx[20]; SendDlgItemMessage(dlg, IDE_VTXYEAR, WM_GETTEXT, sizeof xx, (LPARAM)xx);
      vtxyear = atoi(xx);
      EndDialog(dlg, 1);
   }
   return 0;
}

int dopoke(int really)
{
   for (u8 *ptr = snbuf; *ptr; ) {
      while (*ptr == ' ' || *ptr == ':' || *ptr == ';' || *ptr == ',') ptr++;
      unsigned num = 0;
      while (isdigit(*ptr)) num = num*10 + (*ptr++ - '0');
      if (num < 0x4000 || num > 0xFFFF) return ptr-snbuf+1;
      while (*ptr == ' ' || *ptr == ':' || *ptr == ';' || *ptr == ',') ptr++;
      unsigned val = 0;
      while (isdigit(*ptr)) val = val*10 + (*ptr++ - '0');
      if (val > 0xFF) return ptr-snbuf+1;
      while (*ptr == ' ' || *ptr == ':' || *ptr == ';' || *ptr == ',') ptr++;
      if (really)
          cpu.direct_wm(num, val);
   }
   return 0;
}

INT_PTR CALLBACK pokedlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   if (msg == WM_INITDIALOG) {
      SetFocus(GetDlgItem(dlg, IDE_POKE));
      return 1;
   }
   if ((msg == WM_COMMAND && wp == IDCANCEL) ||
       (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE)) EndDialog(dlg, 0);
   if (msg == WM_COMMAND && LOWORD(wp) == IDOK)
   {
      SendDlgItemMessage(dlg, IDE_POKE, WM_GETTEXT, /*sizeof snbuf*/640*480*4, (LPARAM)snbuf); //Alone Coder 0.36.5
          int r = dopoke(0);
      if (r) MessageBox(dlg, "Incorrect format", 0, MB_ICONERROR),
             SendDlgItemMessage(dlg, IDE_POKE, EM_SETSEL, r, r);
      else dopoke(1), EndDialog(dlg, 0);
   }
   return 0;
}
