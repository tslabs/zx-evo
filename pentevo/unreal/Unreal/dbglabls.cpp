#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbglabls.h"
#include "memory.h"
#include "config.h"
#include "util.h"

MON_LABELS mon_labels;

void MON_LABELS::start_watching_labels()
{
   char dir[FILENAME_MAX] = "\0";
   strncat(dir, userfile, strrchr(userfile, '\\') - userfile);
   hNewUserLabels = FindFirstChangeNotification(dir, 0, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE);
}

void MON_LABELS::stop_watching_labels()
{
   if (!hNewUserLabels || hNewUserLabels == INVALID_HANDLE_VALUE) return;
   CloseHandle(hNewUserLabels);
   hNewUserLabels = INVALID_HANDLE_VALUE;
}

void MON_LABELS::notify_user_labels()
{
   if (hNewUserLabels == INVALID_HANDLE_VALUE) return;
   // load labels at first check
   if (!hNewUserLabels) { start_watching_labels(); import_file(); return; }

   if (WaitForSingleObject(hNewUserLabels, 0) != WAIT_OBJECT_0) return;

   import_file();
   FindNextChangeNotification(hNewUserLabels);
}

unsigned MON_LABELS::add_name(char *name)
{
   unsigned len = strlen(name)+1, new_size = names_size + len;
   if (new_size > align_by(names_size, 4096))
      names = (char*)realloc(names, align_by(new_size, 4096));
   unsigned result = names_size;
   memcpy(names + result, name, len);
   names_size = new_size;
   return result;
}

void MON_LABELS::clear(unsigned char *start, unsigned size)
{
   unsigned dst = 0;
   for (unsigned src = 0; src < n_pairs; src++)
      if ((unsigned)(pairs[src].address - start) > size)
         pairs[dst++] = pairs[src];
   n_pairs = dst;
   // pack `names'
   char *pnames = names; names = 0; names_size = 0;
   for (unsigned l = 0; l < n_pairs; l++)
      pairs[l].name_offs = add_name(pnames + pairs[l].name_offs);
   free(pnames);
}

int __cdecl labels_sort_func(const void *e1, const void *e2)
{
   const MON_LABEL *a = (MON_LABEL*)e1, *b = (MON_LABEL*)e2;
   return a->address - b->address;
}

void MON_LABELS::sort()
{
   qsort(pairs, n_pairs, sizeof(MON_LABEL), labels_sort_func);
}

void MON_LABELS::add(unsigned char *address, char *name)
{
   if (n_pairs >= align_by(n_pairs, 1024))
      pairs = (MON_LABEL*)realloc(pairs, sizeof(MON_LABEL) * align_by(n_pairs+1, 1024));
   pairs[n_pairs].address = address;
   pairs[n_pairs].name_offs = add_name(name);
   n_pairs++;
}

char *MON_LABELS::find(unsigned char *address)
{
   unsigned l = 0, r = n_pairs;
   for (;;) {
      if (l >= r) return 0;
      unsigned m = (l+r)/2;
      if (pairs[m].address == address) return names + pairs[m].name_offs;
      if (pairs[m].address < address) l = m+1; else r = m;
   }
}

unsigned MON_LABELS::load(char *filename, unsigned char *base, unsigned size)
{
   FILE *in = fopen(filename, "rt");
   if (!in)
   {
       errmsg("can't find label file %s", filename);
       return 0;
   }

   clear(base, size);
   unsigned l_counter = 0, loaded = 0; char *txt = 0;
   int l; //Alone Coder 0.36.7
   while (!feof(in)) {
      char line[64];
      if (!fgets(line, sizeof(line), in)) break;
      l_counter++;
      for (/*int*/ l = strlen(line); l && line[l-1] <= ' '; l--); line[l] = 0;
      if (!l) continue;
      unsigned val = 0, offset = 0;
      if (l >= 6 && line[4] == ' ')
      { // адрес без номера банка xxxx label
         for (l = 0; l < 4; l++)
         {
            if (!ishex(line[l]))
                goto ll_err;
            val = (val * 0x10) + hex(line[l]);
         }
         txt = line+5;
      }
      else if (l >= 9 && line[2] == ':' && line[7] == ' ')
      { // адрес сномером банка bb:xxxx label
         for (l = 0; l < 2; l++)
         {
            if (!ishex(line[l]))
                goto ll_err;
            val = (val * 0x10) + hex(line[l]);
         }
         for (l = 3; l < 7; l++)
         {
            if (!ishex(line[l]))
                goto ll_err;
            offset = (offset * 0x10) + hex(line[l]);
         }
         val = val*PAGE + (offset & (PAGE-1));
         txt = line+8;
      }
      else
      {
   ll_err:
         color(CONSCLR_ERROR);
         printf("error in %s, line %d\n", filename, l_counter);
         continue;
      }

      if (val < size)
      {
          add(base+val, txt);
          loaded++;
      }
   }
   fclose(in);
   sort();
   return loaded;
}

unsigned MON_LABELS::alasm_chain_len(unsigned char *page, unsigned offset, unsigned &end)
{
   unsigned count = 0;
   for (;;) {
      if (offset >= 0x3FFC) return 0;
      unsigned s1 = page[offset], sz = s1 & 0x3F;
      if (!s1 || offset == 0x3E00) { end = offset+1; return count; }
      if (sz < 6) return 0;
      unsigned char sym = page[offset+sz-1];
      if (sym >= '0' && sym <= '9') return 0;
      for (unsigned ptr = 5; ptr < sz; ptr++)
         if (!alasm_valid_char[page[offset+ptr]]) return 0;
      if (!(s1 & 0xC0)) count++;
      offset += sz;
   }
}

void MON_LABELS::find_alasm()
{
   static const char label_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz@$_";
   memset(alasm_valid_char, 0, sizeof alasm_valid_char);
   for (const char *lbl = label_chars; *lbl; lbl++) alasm_valid_char[*lbl] = 1;

   alasm_found_tables = 0;
   for (unsigned page = 0; page < conf.ramsize*1024; page += PAGE) {
      for (unsigned offset = 0; offset < PAGE; offset++) {
         unsigned end, count = alasm_chain_len(RAM_BASE_M + page, offset, end);
         if (count < 2) continue;
         alasm_count[alasm_found_tables] = count;
         alasm_offset[alasm_found_tables] = page + offset;
         offset = end; alasm_found_tables++;
         if (alasm_found_tables == MAX_ALASM_LTABLES) return;
      }
   }
}


void MON_LABELS::import_alasm(unsigned offset, char *caption)
{
   clear_ram();
   unsigned char *base = RAM_BASE_M + offset;
   for (;;) { // #FE00/FF00/FFFC - end of labels?
      unsigned char sz = *base; if (!sz) break;
      if (!(sz & 0xC0)) {
         char lbl[64]; unsigned ptr = 0;
         for (unsigned k = sz; k > 5;) k--, lbl[ptr++] = base[k]; lbl[ptr] = 0;
         unsigned val = *(unsigned short*)(base+1);
         unsigned char *bs;
         switch (val & 0xC000) {
            case 0x4000: bs = RAM_BASE_M+5*PAGE; break;
            case 0x8000: bs = RAM_BASE_M+2*PAGE; break;
            case 0xC000: bs = RAM_BASE_M+0*PAGE; break;
            default: bs = 0;
         }
         if (bs) add(bs+(val & 0x3FFF), lbl);
      }
      base += (sz & 0x3F);
   }
   sort();
}

void MON_LABELS::find_xas()
{
   char look_page_6 = 0;
   const char *err = "XAS labels not found in bank #06";
   if (conf.mem_model == MM_PENTAGON && conf.ramsize > 128)
      err = "XAS labels not found in banks #06,#46", look_page_6 = 1;
   xaspage = 0;
   if (look_page_6 && RAM_BASE_M[PAGE*14+0x3FFF] == 5 && RAM_BASE_M[PAGE*14+0x1FFF] == 5) xaspage = 0x46;
   if (!xaspage && RAM_BASE_M[PAGE*6+0x3FFF] == 5 && RAM_BASE_M[PAGE*6+0x1FFF] == 5) xaspage = 0x06;
   if (!xaspage) strcpy(xas_errstr, err);
   else sprintf(xas_errstr, "XAS labels from bank #%02X", xaspage);
}

void MON_LABELS::import_xas()
{
   if (!xaspage) return;
   unsigned base = (xaspage == 0x46)? 0x0E*PAGE : (unsigned)xaspage*PAGE;

   clear_ram(); unsigned count = 0;
   int i; //Alone Coder 0.36.7
   for (int k = 0; k < 2; k++) {
      unsigned char *ptr = RAM_BASE_M + base + (k? 0x3FFD : 0x1FFD);
      for (;;) {
         if (ptr[2] < 5 || (ptr[2] & 0x80)) break;
         char lbl[16]; for (/*int*/ i = 0; i < 7; i++) lbl[i] = ptr[i-7];
         for (i = 7; i && lbl[i-1]==' '; i--); lbl[i] = 0;
         unsigned val = *(unsigned short*)ptr;
         unsigned char *bs;
         switch (val & 0xC000) {
            case 0x4000: bs = RAM_BASE_M+5*PAGE; break;
            case 0x8000: bs = RAM_BASE_M+2*PAGE; break;
            case 0xC000: bs = RAM_BASE_M+0*PAGE; break;
            default: bs = 0;
         }
         if (bs) add(bs+(val & 0x3FFF), lbl), count++;
         ptr -= 9; if (ptr < RAM_BASE_M+base+9) break;
      }
   }
   sort();
   char ln[64]; sprintf(ln, "imported %d labels", count);
   MessageBox(GetForegroundWindow(), ln, xas_errstr, MB_OK | MB_ICONINFORMATION);
}

void MON_LABELS::import_menu()
{
   find_xas();
   find_alasm();

   MENUITEM items[MAX_ALASM_LTABLES+4] = { 0 };
   unsigned menuptr = 0;

   items[menuptr].text = xas_errstr;
   items[menuptr].flags = xaspage? (MENUITEM::FLAGS)0 : MENUITEM::DISABLED;
   menuptr++;

   char alasm_text[MAX_ALASM_LTABLES][64];
   if (!alasm_found_tables) {
      sprintf(alasm_text[0], "No ALASM labels in whole %dK memory", conf.ramsize);
      items[menuptr].text = alasm_text[0];
      items[menuptr].flags = MENUITEM::DISABLED;
      menuptr++;
   } else {
      for (unsigned i = 0; i < alasm_found_tables; i++) {
         sprintf(alasm_text[i], "%d ALASM labels in page %d, offset #%04X", alasm_count[i], alasm_offset[i]/PAGE, (alasm_offset[i] & 0x3FFF) | 0xC000);
         items[menuptr].text = alasm_text[i];
         items[menuptr].flags = (MENUITEM::FLAGS)0;
         menuptr++;
      }
   }

   items[menuptr].text = nil;
   items[menuptr].flags = MENUITEM::DISABLED;
   menuptr++;

   items[menuptr].text = "CANCEL";
   items[menuptr].flags = MENUITEM::CENTER;
   menuptr++;

   MENUDEF menu = { items, menuptr, "import labels" };
   if (!handle_menu(&menu)) return;
   if (menu.pos == 0) import_xas();
   menu.pos--;
   if ((unsigned)menu.pos < alasm_found_tables) import_alasm(alasm_offset[menu.pos], alasm_text[menu.pos]);
}

void MON_LABELS::import_file()
{
   FILE *ff = fopen(userfile, "rb"); if (!ff) return; fclose(ff);
   unsigned count = load(userfile, RAM_BASE_M, conf.ramsize * 1024);
   if (!count) return;
   char tmp[0x200];
   sprintf(tmp, "loaded %d labels from\r\n%s", count, userfile);
   puts(tmp);
   //MessageBox(GetForegroundWindow(), tmp, "unreal discovered changes in user labels", MB_OK | MB_ICONINFORMATION);//removed by Alone Coder
}

void load_labels(char *filename, unsigned char *base, unsigned size)
{
   mon_labels.load(filename, base, size);
}

char curlabel[64]; unsigned lcount;

void ShowLabels()
{
   SetDlgItemText(dlg, IDC_LABEL_TEXT, curlabel);
   HWND list = GetDlgItem(dlg, IDC_LABELS);

   while (SendMessage(list, LB_GETCOUNT, 0, 0))
      SendMessage(list, LB_DELETESTRING, 0, 0);

   unsigned ln = strlen(curlabel); lcount = 0;
   char *s; //Alone Coder 0.36.7
   for (unsigned p = 0; p < 4; p++)
   {
      unsigned char *base = am_r(p*PAGE);
      for (unsigned i = 0; i < mon_labels.n_pairs; i++)
      {
         unsigned char *label = mon_labels.pairs[i].address;
         if (label < base || label >= base + PAGE)
             continue;
         char *name = mon_labels.pairs[i].name_offs + mon_labels.names;
         if (ln)
         {
            // unfortunately, strstr() is case sensitive, use loop
            for (/*char * */s = name; *s; s++)
               if (!strnicmp(s, curlabel, ln)) break;
            if (!*s) continue;
         }
         char zz[0x400];
         sprintf(zz, "%04X %s", (label - base) + (p * PAGE), name);
         SendMessage(list, LB_ADDSTRING, 0, (LPARAM)zz); lcount++;
      }
   }
   SendMessage(list, LB_SETCURSEL, 0, 0);
   SetFocus(list);
}

INT_PTR CALLBACK LabelsDlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   ::dlg = dlg;
   if (msg == WM_INITDIALOG)
   {
      *curlabel = 0;
      ShowLabels();
      return 1;
   }

   if (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE) EndDialog(dlg, 0);

   if (msg == WM_VKEYTOITEM)
   {
      unsigned sz = strlen(curlabel);
      wp = LOWORD(wp);
      if (wp == VK_BACK) {
         if (sz) curlabel[sz-1] = 0, ShowLabels();
         else { deadkey: Beep(300, 100); }
      } else if ((unsigned)(wp-'0') < 10 || (unsigned)(wp-'A') < 26 || wp == '_') {
         if (sz == sizeof(curlabel)-1) goto deadkey;
         curlabel[sz] = wp, curlabel[sz+1] = 0, ShowLabels();
         if (!lcount) { curlabel[sz] = 0, ShowLabels(); goto deadkey; }
      } else return -1;
      return -2;
   }

   if (msg != WM_COMMAND) return 0;

   unsigned id = LOWORD(wp), code = HIWORD(wp);
   if (id == IDCANCEL || id == IDOK) EndDialog(dlg, 0);

   if (id == IDOK || (id == IDC_LABELS && code == LBN_DBLCLK))
   {
      HWND list = GetDlgItem(dlg, IDC_LABELS);
      unsigned n = SendMessage(list, LB_GETCURSEL, 0, 0);
      if (n >= lcount) return 0;
      char zz[0x400]; SendMessage(list, LB_GETTEXT, n, (LPARAM)zz);
      unsigned address; sscanf(zz, "%X", &address);

      void push_pos(); push_pos();
      CpuMgr.Cpu().trace_curs = CpuMgr.Cpu().trace_top = address;
      activedbg = WNDTRACE;

      EndDialog(dlg, 1);
      return 1;
   }

   return 0;
}

void mon_show_labels()
{
   DialogBox(hIn, MAKEINTRESOURCE(IDD_LABELS), wnd, LabelsDlg);
}

void init_labels(char* filename)
{
    if (filename)
    {
        addpath(mon_labels.userfile, filename);
        color(CONSCLR_DEFAULT); printf("lbl: ");
        color(CONSCLR_INFO);    printf("%s\n", mon_labels.userfile);
    }
    else addpath(mon_labels.userfile, "user.l");
}