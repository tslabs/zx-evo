#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "gui.h"
#include "util.h"

struct FILEPREVIEWINFO
{
   OPENFILENAME *ofn;
   struct { HWND h; int dx,dy; } list;
   struct { HWND h; } dlg;
   struct { HWND h; } innerdlg;

   void OnResize();
   void OnChange();

   void PreviewTRD(char *filename);
   void PreviewSCL(char *filename);
   void Preview(u8 *cat);

} FilePreviewInfo;

void FILEPREVIEWINFO::OnResize()
{
   const int dlgbase = 280;
   const int listbase = 163;

   RECT dlgrc; GetWindowRect(dlg.h, &dlgrc);
   list.dy = (dlgrc.bottom - dlgrc.top) - dlgbase + listbase;

   SetWindowPos(list.h, 0, 0, 0, list.dx, list.dy,
                  SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
}

void FILEPREVIEWINFO::OnChange()
{
   char filename[512];
   int r = SendMessage(dlg.h, CDM_GETFILEPATH, sizeof(filename), (LPARAM) filename);
   SendMessage(list.h, LVM_DELETEALLITEMS, 0, 0);
   if (r < 0 || (GetFileAttributes(filename) & FILE_ATTRIBUTE_DIRECTORY)) return;

   #if 0 // too slow for every file
   TRKCACHE t;
   FDD TestDrive;
   u8 type = what_is(filename);
   if (type < snSCL) return;
   TestDrive.emptydisk();
   if (!TestDrive.read(type)) return;
   #endif

   char *ext = strrchr(filename, '.');
   if (!ext) return;
   ext++;

   if (!stricmp(ext, "trd")) PreviewTRD(filename);
   if (!stricmp(ext, "scl")) PreviewSCL(filename);
}

void FILEPREVIEWINFO::Preview(u8 *cat)
{
   ::dlg = innerdlg.h;
   u8 bas = getcheck(IDC_PREVIEW_BASIC);
   u8 del = getcheck(IDC_PREVIEW_ERASED);

   int count = 0;
   char fn[10];

   LVITEM item;
   item.mask = LVIF_TEXT;
   item.pszText = fn;

   for (unsigned p = 0; p < 0x800; p += 0x10) {
      if (!cat[p]) break;
      if (!del && cat[p] == 1) continue;
      if (bas && cat[p+8] != 'B') continue;

      memcpy(fn, cat+p, 8); fn[8] = 0;
      item.iItem = count++;
      item.iSubItem = 0;
      item.iItem = SendMessage(list.h, LVM_INSERTITEM, 0, (LPARAM) &item);

      fn[0] = cat[p+8]; fn[1] = 0;
      item.iSubItem = 1;
      SendMessage(list.h, LVM_SETITEM, 0, (LPARAM) &item);

      sprintf(fn, "%d", cat[p+13]);
      item.iSubItem = 2;
      SendMessage(list.h, LVM_SETITEM, 0, (LPARAM) &item);
   }
}

void FILEPREVIEWINFO::PreviewTRD(char *filename)
{
   u8 cat[0x800];
   FILE *ff = fopen(filename, "rb");
   int sz = fread(cat, 1, 0x800, ff);
   fclose(ff);
   if (sz != 0x800) return;
   Preview(cat);
}

void FILEPREVIEWINFO::PreviewSCL(char *filename)
{
   u8 cat[0x800] = { 0 };
   u8 hdr[16];

   FILE *ff = fopen(filename, "rb");
   unsigned sz = fread(hdr, 1, 9, ff), count = 0;

   if (sz == 9 && !memcmp(hdr, "SINCLAIR", 8)) {
      unsigned max = hdr[8]; sz = max*14;
      u8 *cat1 = (u8*)alloca(sz);
      if (fread(cat1, 1, sz, ff) == sz) {
         for (unsigned i = 0; i < sz; i += 14) {
            memcpy(cat+count*0x10, cat1+i, 14);
            count++; if (count == 0x80) break;
         }
      }
   }

   fclose(ff);
   if (count) Preview(cat);
}

UINT_PTR CALLBACK PreviewDlgProc(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   switch (msg)
   {
      case WM_INITDIALOG:
      {
         FilePreviewInfo.ofn = (OPENFILENAME*)lp;
         FilePreviewInfo.innerdlg.h = dlg;
         FilePreviewInfo.dlg.h = GetParent(dlg);
         FilePreviewInfo.list.h = GetDlgItem(dlg, IDC_PREVIEW_BOX);

         unsigned exflags = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
         SendMessage(FilePreviewInfo.list.h, LVM_SETEXTENDEDLISTVIEWSTYLE, exflags, exflags);

         LVCOLUMN sizeCol;
         sizeCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
         sizeCol.fmt = LVCFMT_LEFT;

         sizeCol.cx = 80;
         sizeCol.pszText = PSTR("Filename");
         SendMessage(FilePreviewInfo.list.h, LVM_INSERTCOLUMN, 0, (LPARAM)&sizeCol);

         sizeCol.cx = 40;
         sizeCol.pszText = PSTR("Ext");
         SendMessage(FilePreviewInfo.list.h, LVM_INSERTCOLUMN, 1, (LPARAM)&sizeCol);

         sizeCol.cx = 50;
         sizeCol.pszText = PSTR("Size");
         SendMessage(FilePreviewInfo.list.h, LVM_INSERTCOLUMN, 2, (LPARAM)&sizeCol);

         HFONT fnt = (HFONT)GetStockObject(OEM_FIXED_FONT);
         SendMessage(FilePreviewInfo.list.h, WM_SETFONT, (WPARAM)fnt, 0);

         RECT rc; GetWindowRect(FilePreviewInfo.list.h, &rc);
         FilePreviewInfo.list.dx = rc.right - rc.left;
         FilePreviewInfo.list.dy = rc.bottom - rc.top;

         break;
      }

      case WM_COMMAND:
         if (LOWORD(wp) == IDC_PREVIEW_BASIC || LOWORD(wp) == IDC_PREVIEW_ERASED)
            FilePreviewInfo.OnChange();
         break;

      case WM_SIZE:
         FilePreviewInfo.OnResize();
         break;

      case WM_NOTIFY:
         if (((OFNOTIFY*)lp)->hdr.code == CDN_SELCHANGE)
            FilePreviewInfo.OnChange();
         break;

   }
   return 0;
}

int GetSnapshotFileName(OPENFILENAME *ofn, int save)
{
   ofn->Flags |= save? OFN_PATHMUSTEXIST : OFN_FILEMUSTEXIST;
   ofn->Flags |= OFN_HIDEREADONLY | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_ENABLESIZING;
   ofn->Flags |= OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;

   ofn->hwndOwner = GetForegroundWindow();
   ofn->hInstance = hIn;
   ofn->lpstrTitle = save? "Save Snapshot / Disk / Tape as" : "Load Snapshot / Disk / Tape";

   ofn->lpfnHook          = PreviewDlgProc;
   ofn->lpTemplateName    = MAKEINTRESOURCE(IDD_FILEPREVIEW);
   ofn->lpstrInitialDir   = temp.SnapDir;

   BOOL res = save? GetSaveFileName(ofn) : GetOpenFileName(ofn);

   if (res)
   {
       strcpy(temp.SnapDir, ofn->lpstrFile);
       char *Ptr = strrchr(temp.SnapDir, '\\');
       if (Ptr)
        *Ptr = 0;
       return res;
   }
   DWORD errcode = CommDlgExtendedError();
   if (!errcode) return 0;

   color(CONSCLR_ERROR);
   printf("Error while selecting file. Code is 0x%08X\n", errcode);
   return 0;
}
