#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "emulator/ui/gui.h"
#include "dx/dx.h"
#include "util.h"

struct CHEATDLG
{
   HWND dlg{};
   HWND resBox{};
   HWND resLabel{};

   void ShowResults();
   void SetControls();
   void Search();

   u8 *lastsnap;
   u8 *bitmask;
   unsigned searchSize{};
   unsigned nFound;
   u8 wordmode{}, hex{};

   CHEATDLG() { lastsnap = nullptr; bitmask = nullptr; nFound = -1; mode = S_NEW; }
   ~CHEATDLG() { free(lastsnap); free(bitmask);  }

   enum SR_MODE { S_NEW, S_VAL, S_INC, S_DEC } mode;

} CheatDlg;

void CHEATDLG::Search()
{
   if (nFound == -1) searchSize = conf.ramsize*1024;
   searchSize = min(searchSize, conf.ramsize*1024);

   bitmask = (u8*)realloc(bitmask, searchSize/8);
   if (nFound == -1) memset(bitmask, 0xFF, searchSize/8);

   unsigned i;

   switch (mode) {

      case S_NEW:
      {
         memset(bitmask, 0xFF, searchSize/8);
         nFound = -1;
         break;
      }

      case S_VAL:
      {
         char str[16]; GetDlgItemText(dlg, IDE_VALUE, str, sizeof(str));
         unsigned val; sscanf(str, hex? "%X" : "%d", &val);
         if (wordmode) {
            for (i = 0; i < searchSize-1; i++)
               if (*(WORD*)(memory+i) != (WORD)val)
                  bitmask[i/8] &= ~(1 << (i & 7));
         } else {
            for (i = 0; i < searchSize; i++)
               if (memory[i] != (BYTE)val)
                  bitmask[i/8] &= ~(1 << (i & 7));
         }
         break;
      }

      case S_INC:
      case S_DEC:
      {
         u8 *ptr1, *ptr2;
         if (mode == S_INC) ptr1 = memory, ptr2 = lastsnap;
         else ptr2 = memory, ptr1 = lastsnap;

         if (wordmode) {
            for (i = 0; i < searchSize-1; i++)
               if (*(WORD*)(ptr1+i) <= *(WORD*)(ptr2+i))
                  bitmask[i/8] &= ~(1 << (i & 7));
         } else {
            for (i = 0; i < searchSize; i++)
               if (ptr1[i] <= ptr2[i])
                  bitmask[i/8] &= ~(1 << (i & 7));
         }
         break;
      }

   }

   if (wordmode) bitmask[(searchSize-1)/8] &= 0x7F;

   if (mode != S_NEW) {
      for (nFound = i = 0; i < searchSize/8; i++) {
         if (!bitmask[i]) continue;
         if (bitmask[i] & 0x01) nFound++;
         if (bitmask[i] & 0x02) nFound++;
         if (bitmask[i] & 0x04) nFound++;
         if (bitmask[i] & 0x08) nFound++;
         if (bitmask[i] & 0x10) nFound++;
         if (bitmask[i] & 0x20) nFound++;
         if (bitmask[i] & 0x40) nFound++;
         if (bitmask[i] & 0x80) nFound++;
      }
   }

   lastsnap = (u8*)realloc(lastsnap, searchSize);
   memcpy(lastsnap, memory, searchSize);
   ShowResults();
}

void CHEATDLG::SetControls()
{
   int enabled;

   setcheck(IDC_HEX, hex);
   setcheck(IDC_BYTE, !wordmode);
   setcheck(IDC_WORD, wordmode);

   enabled = lastsnap && (nFound > 0);
   EnableWindow(GetDlgItem(dlg, IDC_DEC), enabled);
   EnableWindow(GetDlgItem(dlg, IDC_INC), enabled);
   if (!enabled && (mode == S_DEC || mode == S_INC)) mode = S_NEW;

   enabled = (nFound > 0);
   EnableWindow(GetDlgItem(dlg, IDC_EXACT), enabled);
   if (!enabled && mode == S_VAL) mode = S_NEW;

   setcheck(IDC_NEW, (mode == S_NEW));
   setcheck(IDC_EXACT, (mode == S_VAL));
   setcheck(IDC_INC, (mode == S_INC));
   setcheck(IDC_DEC, (mode == S_DEC));

   enabled = (nFound == -1);// || (mode == S_NEW);
   EnableWindow(GetDlgItem(dlg, IDC_BYTE), enabled);
   EnableWindow(GetDlgItem(dlg, IDC_WORD), enabled);

   enabled = (mode == S_VAL);
   EnableWindow(GetDlgItem(dlg, IDE_VALUE), enabled);
   EnableWindow(GetDlgItem(dlg, IDC_HEX), enabled);
}

void CHEATDLG::ShowResults()
{
   if (nFound > 0 && nFound <= 100) {

      ShowWindow(resLabel, SW_HIDE);
      ShowWindow(resBox, SW_SHOW);
      SendMessage(resBox, LVM_DELETEALLITEMS, 0, 0);

      char fn[10];
      LVITEM item; int count = 0;
      item.mask = LVIF_TEXT;
      item.pszText = fn;

      for (unsigned i = 0; i < searchSize; i++) {
         if (!(bitmask[i/8] & (1 << (i & 7)))) continue;

         unsigned base = 0xC000;
         unsigned page = i / PAGE;
         if (page == 2) base = 0x8000;
         if (page == 5) base = 0x4000;

         sprintf(fn, "%04X", base + (i & (PAGE-1)));
         item.iItem = count++;
         item.iSubItem = 0;
         item.iItem = SendMessage(resBox, LVM_INSERTITEM, 0, (LPARAM) &item);

         sprintf(fn, "%02X", page);
         item.iSubItem = 1;
         SendMessage(resBox, LVM_SETITEM, 0, (LPARAM) &item);

         sprintf(fn, "%04X", i & (PAGE-1));
         item.iSubItem = 2;
         SendMessage(resBox, LVM_SETITEM, 0, (LPARAM) &item);

         if (wordmode) sprintf(fn, "%04X", *(WORD*)(memory+i));
         else sprintf(fn, "%02X", memory[i]);
         item.iSubItem = 3;
         SendMessage(resBox, LVM_SETITEM, 0, (LPARAM) &item);

      }
   } else {
      ShowWindow(resBox, SW_HIDE);
      ShowWindow(resLabel, SW_SHOW);
      char str[128];
      if (!lastsnap) strcpy(str, "no active search");
      else if (nFound == -1) strcpy(str, "new search started");
      else if (!nFound) strcpy(str, "found nothing");
      else sprintf(str, "result list too large (%d entries)", nFound);
      SetWindowText(resLabel, str);
   }
}

INT_PTR CALLBACK cheatdlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   if (msg == WM_INITDIALOG) {

      ::dlg = CheatDlg.dlg = dlg;
      CheatDlg.resLabel = GetDlgItem(dlg, IDC_STATUS);
      CheatDlg.resBox = GetDlgItem(dlg, IDC_RESULTS);

      unsigned exflags = LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT;
      SendMessage(CheatDlg.resBox, LVM_SETEXTENDEDLISTVIEWSTYLE, exflags, exflags);

      LVCOLUMN sizeCol;
      sizeCol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
      sizeCol.fmt = LVCFMT_LEFT;

      sizeCol.cx = 50;
      sizeCol.pszText = LPSTR("Address");
      SendMessage(CheatDlg.resBox, LVM_INSERTCOLUMN, 0, (LPARAM)&sizeCol);

      sizeCol.cx = 40;
      sizeCol.pszText = LPSTR("Page");
      SendMessage(CheatDlg.resBox, LVM_INSERTCOLUMN, 1, (LPARAM)&sizeCol);

      sizeCol.cx = 50;
      sizeCol.pszText = LPSTR("Offset");
      SendMessage(CheatDlg.resBox, LVM_INSERTCOLUMN, 2, (LPARAM)&sizeCol);

      sizeCol.cx = 50;
      sizeCol.pszText = LPSTR("Value");
      SendMessage(CheatDlg.resBox, LVM_INSERTCOLUMN, 3, (LPARAM)&sizeCol);

      // SetFocus(GetDlgItem(dlg, IDE_VALUE));
      SendDlgItemMessage(dlg, IDE_VALUE, EM_LIMITTEXT, 5, 0);
      CheatDlg.SetControls();
      CheatDlg.ShowResults();
      return 1;
   }

   if ((msg == WM_COMMAND && wp == IDCANCEL) ||
       (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE)) EndDialog(dlg, 0);

   if (msg == WM_COMMAND && HIWORD(wp) == BN_CLICKED)
   {
      DWORD id = LOWORD(wp);
      if (id == IDC_NEW) CheatDlg.mode = CHEATDLG::S_NEW;
      else if (id == IDC_EXACT) CheatDlg.mode = CHEATDLG::S_VAL;
      else if (id == IDC_INC) CheatDlg.mode = CHEATDLG::S_INC;
      else if (id == IDC_DEC) CheatDlg.mode = CHEATDLG::S_DEC;
      else if (id == IDC_HEX) CheatDlg.hex = getcheck(id);
      else if (id == IDC_BYTE) CheatDlg.wordmode = 0;
      else if (id == IDC_WORD) CheatDlg.wordmode = 1;
      else if (id == IDB_SEARCH) CheatDlg.Search();
      else id = 0;

      if (id) CheatDlg.SetControls();
   }

   return 0;
}

void main_cheat()
{
   sound_stop();
   DialogBox(hIn, MAKEINTRESOURCE(IDD_CHEAT), wnd, cheatdlg);
   eat(); sound_play();
}
