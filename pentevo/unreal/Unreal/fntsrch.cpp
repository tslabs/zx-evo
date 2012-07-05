#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "dxr_text.h"
#include "util.h"

#ifdef MOD_SETTINGS
unsigned font_maxmem = 0xFFFF;
unsigned char r21=1, r30=1, r41=1, r61=1, r80=1, rae=1, rf0=0, roth=0;
unsigned char linear = 0, right = 1, x100 = 1;
unsigned char rmask[0x100];
unsigned font_address;
unsigned char fontdata2[0x900*2];
unsigned fontsize = 8;
static unsigned block_font_dialog = 0;
unsigned char *font_base;


void update_rmask()
{
   memset(rmask, 0, sizeof rmask);
   if (r21) memset(rmask+0x21, 1, 0x0F);
   if (r30) memset(rmask+0x30, 1, 10);
   if (r41) memset(rmask+0x41, 1, 26);
   if (r61) memset(rmask+0x61, 1, 26);
   if (r80) memset(rmask+0x80, 1, 32);
   if (rae) memset(rmask+0xA0, 1, 16), memset(rmask+0xE0, 1, 16);
   if (rf0) rmask[0xF0] = rmask[0xF1] = 1;
   if (roth)
       memset(rmask+1, 1, 0x1F), memset(rmask+0xB0, 1, 0x30),
       memset(rmask+0xF2, 1, 13), memset(rmask+0x3A, 1, 7),
       memset(rmask+0x5B, 1, 6), memset(rmask+0x7B, 1, 5);
}

void get_ranges(HWND dlg)
{
   char ln[64]; GetDlgItemText(dlg, IDE_ADDRESS, ln, sizeof ln);
   sscanf(ln, "%X", &font_address); font_address &= font_maxmem;

   linear = (IsDlgButtonChecked(dlg, IDC_LINEAR) == BST_CHECKED)? 1 : 0;
   right = (IsDlgButtonChecked(dlg, IDC_RIGHT) == BST_CHECKED)? 1 : 0;
   x100 = (IsDlgButtonChecked(dlg, IDC_100) == BST_CHECKED)? 1 : 0;

   r21 = IsDlgButtonChecked(dlg, IDC_R212F) == BST_CHECKED;
   r30 = IsDlgButtonChecked(dlg, IDC_R3039) == BST_CHECKED;
   r41 = IsDlgButtonChecked(dlg, IDC_R415A) == BST_CHECKED;
   r61 = IsDlgButtonChecked(dlg, IDC_R617A) == BST_CHECKED;
   r80 = IsDlgButtonChecked(dlg, IDC_R809F) == BST_CHECKED;
   rae = IsDlgButtonChecked(dlg, IDC_RA0AF) == BST_CHECKED;
   rf0 = IsDlgButtonChecked(dlg, IDC_RF0F1) == BST_CHECKED;
   roth = IsDlgButtonChecked(dlg, IDC_ROTH2) == BST_CHECKED;
   fontsize = 5 + SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_GETCURSEL, 0, 0);
   update_rmask();
}

void paint_font(HWND dlg, int paint=0)
{
   const int sz = 340;
   char *buf = (char*)malloc(sz*sz);
   if (!buf) return;
   RECT rc; GetWindowRect(GetDlgItem(dlg, IDC_FRAME), &rc);
   RECT r2; GetWindowRect(dlg, &r2);
   rc.top -= r2.top, rc.bottom -= r2.top, rc.left -= r2.left, rc.right -= r2.left;
   static struct {
      BITMAPINFO header;
      RGBQUAD waste[0x100];
   } gdibmp = { { sizeof(BITMAPINFOHEADER), sz, -sz, 1, 8, BI_RGB } };
   static RGBQUAD cl[] = { {0xFF,0,0},{0xC0,0xC0,0xC0},{0,0,0} };
   memcpy(gdibmp.header.bmiColors, cl, sizeof(cl));
   memset(buf, 0, sz*sz);

   unsigned next_pixel, next_char;
   if (linear) next_pixel = 1, next_char = 8;
   else next_pixel = 0x100, next_char = 1;

   unsigned t1[4], t2[4];
   for (unsigned j = 0; j < 4; j++) {
      unsigned mask = (j >> 1)*0xFFFF + (j & 1)*0xFFFF0000;
      t1[j] = mask & 0x01010101; t2[j] = mask & 0x02020202;
   }

   for (unsigned ch = 0; ch < 0x100; ch++) {
      unsigned x = ch & 0x0F, y = ch / 0x10;
      x = x*20 + ((x>>2)&3) * 2;
      y = y*20 + ((y>>2)&3) * 2;
      x += 10, y += 10;
      for (unsigned i = 0; i < fontsize; i++) {
         unsigned char byte = font_base[(font_address + i*next_pixel + ch*next_char) & font_maxmem];
         unsigned *t0 = t1;
         if (right || !rmask[ch]) t0 = t2;
         *(unsigned*)(buf+sz*(y+2*i) + x) = t0[byte >> 6];
         *(unsigned*)(buf+sz*(y+2*i) + x + 4) = t0[(byte >> 4) & 3];
         *(unsigned*)(buf+sz*(y+2*i+1) + x) = t0[byte >> 6];
         *(unsigned*)(buf+sz*(y+2*i+1) + x + 4) = t0[(byte >> 4) & 3];

         t0 = t1;
         if (!right || !rmask[ch]) t0 = t2;
         *(unsigned*)(buf+sz*(y+2*i) + x + 8) = t0[(byte >> 2) & 3];
         *(unsigned*)(buf+sz*(y+2*i) + x + 12) = t0[byte & 3];
         *(unsigned*)(buf+sz*(y+2*i+1) + x + 8) = t0[(byte >> 2) & 3];
         *(unsigned*)(buf+sz*(y+2*i+1) + x + 12) = t0[byte & 3];
      }
   }

   PAINTSTRUCT ps;
   if (paint) BeginPaint(dlg, &ps); else ps.hdc = GetDC(dlg);
   SetDIBitsToDevice(ps.hdc, rc.left + (int)(rc.right-rc.left-sz)/2, rc.top + (int)(rc.bottom-rc.top-sz)/2, sz, sz, 0, 0, 0, sz, buf, &gdibmp.header, DIB_RGB_COLORS);
   if (paint) EndPaint(dlg, &ps); else ReleaseDC(dlg, ps.hdc);
   free(buf);
   char ln[64];
   if (font_base == RAM_BASE_M)
      sprintf(ln, "bank #%02X, offset %04X", (font_address & font_maxmem) >> 14, font_address & 0x3FFF);
   else
      sprintf(ln, "file offset %04X", font_address);
   SetDlgItemText(dlg, IDC_ADDRESS, ln);
}

unsigned char pattern[12][8];

void kill_pattern(unsigned x, unsigned y, unsigned mode)
{
   unsigned sx[64], sy[64], sp = 1;
   sx[0] = x, sy[0] = y;
   pattern[y][x] = 0;
   while (sp--) {
      x = sx[sp], y = sy[sp];
      if (pattern[y-1][x]) pattern[y-1][x] = 0, sx[sp] = x, sy[sp] = y-1, sp++;
      if (pattern[y+1][x]) pattern[y+1][x] = 0, sx[sp] = x, sy[sp] = y+1, sp++;
      if (pattern[y][x-1]) pattern[y][x-1] = 0, sx[sp] = x-1, sy[sp] = y, sp++;
      if (pattern[y][x+1]) pattern[y][x+1] = 0, sx[sp] = x+1, sy[sp] = y, sp++;
      if (mode) {
         if (pattern[y-1][x+1]) pattern[y-1][x+1] = 0, sx[sp] = x+1, sy[sp] = y-1, sp++;
         if (pattern[y+1][x+1]) pattern[y+1][x+1] = 0, sx[sp] = x+1, sy[sp] = y+1, sp++;
         if (pattern[y-1][x-1]) pattern[y-1][x-1] = 0, sx[sp] = x-1, sy[sp] = y-1, sp++;
         if (pattern[y+1][x-1]) pattern[y+1][x-1] = 0, sx[sp] = x-1, sy[sp] = y+1, sp++;
      }
   }
}

unsigned kill_raw(unsigned x, unsigned y, unsigned mode)
{
   unsigned result = 0;
   if (pattern[y][x])   kill_pattern(x, y, mode), result++;
   if (pattern[y][x+1]) kill_pattern(x+1, y, mode), result++;
   if (pattern[y][x+2]) kill_pattern(x+2, y, mode), result++;
   if (pattern[y][x+3]) kill_pattern(x+3, y, mode), result++;
   return result;
}

unsigned count_lnk(unsigned mode)
{
   unsigned result = 0;
   for (;;) {
      unsigned r1 = result;
      for (unsigned line = 1; line < 11; line++) {
         if (*(unsigned*)&(pattern[line][0])) result += kill_raw(0, line, mode);
         if (*(unsigned*)&(pattern[line][4])) result += kill_raw(4, line, mode);
      }
      if (result == r1) break;
   }
   return result;
}

static union { unsigned v32; unsigned char v8[4]; } c_map0[16];
static union { unsigned v32; unsigned char v8[4]; } c_map1[16];
static union { unsigned v32; unsigned char v8[4]; } c_map2[16];
static union { unsigned v32; unsigned char v8[4]; } c_map3[16];
static union { unsigned v32; unsigned char v8[4]; } c_map4[16];

void create_maps()
{
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < 16; i++)
      for (unsigned j = 0; j < 4; j++)
         c_map0[i].v8[3-j] = (i >> j) & 1;
   for (i = 0; i < 16; i++) {
      c_map1[i].v32 = c_map0[i >> 1].v32,
      c_map2[i].v32 = c_map0[(i&1) << 3].v32;
      c_map3[i].v32 = c_map0[(((~i)>>2) | 4) & 0x07].v32,
      c_map4[i].v32 = c_map0[(((~i)<<2) | 2) & 0x0E].v32;
   }
}

unsigned linked_cells(unsigned sym)
{
   unsigned pix = 0x100, shift = right? 0 : 4;
   if (linear) sym *= 8, pix = 1;
   sym += font_address;
//   *(unsigned*)&(pattern[0][0]) = *(unsigned*)&(pattern[0][4]) = 0;
   for (unsigned i = 0; i < fontsize; i++, sym += pix)
      *(unsigned*)&(pattern[i+1][0]) = c_map1[(font_base[sym & font_maxmem] >> shift) & 0x0F].v32,
      *(unsigned*)&(pattern[i+1][4]) = c_map2[(font_base[sym & font_maxmem] >> shift) & 0x0F].v32;
   *(unsigned*)&(pattern[fontsize+1][0]) = *(unsigned*)&(pattern[fontsize+1][4]) = 0;
   *(unsigned*)&(pattern[fontsize+2][0]) = *(unsigned*)&(pattern[fontsize+2][4]) = 0;
//   *(unsigned*)&(pattern[fontsize+3][0]) = *(unsigned*)&(pattern[fontsize+3][4]) = 0;
   return count_lnk(1);
}

unsigned linked_empties(unsigned sym)
{
   unsigned pix = 0x100, shift = right? 0 : 4;
   if (linear) sym *= 8, pix = 1;
   sym += font_address;
//   *(unsigned*)&(pattern[0][0]) = *(unsigned*)&(pattern[0][4]) = 0;
   *(unsigned*)&(pattern[1][0]) = WORD4(0,1,1,1),
   *(unsigned*)&(pattern[1][4]) = WORD4(1,1,1,0);
   for (unsigned i = 0; i < fontsize; i++, sym += pix)
      *(unsigned*)&(pattern[i+2][0]) = c_map3[(font_base[sym & font_maxmem] >> shift) & 0x0F].v32,
      *(unsigned*)&(pattern[i+2][4]) = c_map4[(font_base[sym & font_maxmem] >> shift) & 0x0F].v32;
   *(unsigned*)&(pattern[fontsize+2][0]) = WORD4(0,1,1,1),
   *(unsigned*)&(pattern[fontsize+2][4]) = WORD4(1,1,1,0);
//   *(unsigned*)&(pattern[fontsize+3][0]) = *(unsigned*)&(pattern[fontsize+3][4]) = 0;
   return count_lnk(0);
}

unsigned char is_font()
{
   const int max_err = 2;
   int err = 0;
   #define RET_ERR { if (++err > max_err) return 0; }
   if (r21) {
      if (linked_cells('!') != 2) RET_ERR
      if (linked_empties('!') != 1) RET_ERR
      if (linked_cells('*') != 1) RET_ERR
      if (linked_empties('*') != 1) RET_ERR
      if (linked_cells('(') != 1) RET_ERR
      if (linked_empties('(') != 1) RET_ERR
      if (linked_cells(')') != 1) RET_ERR
      if (linked_empties(')') != 1) RET_ERR
      if (linked_cells('$') != 1) RET_ERR
   }
   if (r30) {
      if (linked_cells('1') != 1) RET_ERR
      if (linked_empties('1') != 1) RET_ERR
      if (linked_cells('6') != 1) RET_ERR
      if (linked_empties('6') != 2) RET_ERR
      if (linked_cells('8') != 1) RET_ERR
      if (linked_empties('8') != 3) RET_ERR
      if (linked_cells('9') != 1) RET_ERR
      if (linked_empties('9') != 2) RET_ERR
      if (linked_cells('0') != 1) RET_ERR
      int e = linked_empties('0');
      if (e != 2 && e != 3) RET_ERR
   }
   if (r41) {
      if (linked_cells('A') != 1) RET_ERR
      if (linked_empties('A') != 2) RET_ERR
      if (linked_cells('O') != 1) RET_ERR
      if (linked_empties('O') != 2) RET_ERR
      if (linked_cells('S') != 1) RET_ERR
      if (linked_empties('S') != 1) RET_ERR
      if (linked_cells('T') != 1) RET_ERR
      if (linked_empties('T') != 1) RET_ERR
      if (linked_cells('Z') != 1) RET_ERR
      if (linked_empties('Z') != 1) RET_ERR
   }
   if (r61) {
      if (linked_cells('b') != 1) RET_ERR
      if (linked_empties('b') != 2) RET_ERR
      if (linked_cells('o') != 1) RET_ERR
      if (linked_empties('o') != 2) RET_ERR
      if (linked_cells('s') != 1) RET_ERR
      if (linked_empties('s') != 1) RET_ERR
      if (linked_cells('t') != 1) RET_ERR
      if (linked_empties('t') != 1) RET_ERR
      if (linked_cells('z') != 1) RET_ERR
      if (linked_empties('z') != 1) RET_ERR
   }
   if (r80) {
      if (linked_cells(0x80) != 1) RET_ERR // A
      if (linked_empties(0x80) != 2) RET_ERR
      if (linked_cells(0x81) != 1) RET_ERR // Å
      if (linked_empties(0x81) != 2) RET_ERR
      if (linked_cells(0x83) != 1) RET_ERR // É
      if (linked_empties(0x83) != 1) RET_ERR
      if (linked_cells(0x9F) != 1) RET_ERR // ü
      if (linked_empties(0x9F) != 2) RET_ERR
   }
   if (rae) {
      if (linked_cells(0xAE) != 1) RET_ERR // o
      if (linked_empties(0xAE) != 2) RET_ERR
      if (linked_cells(0xE1) != 1) RET_ERR // c
      if (linked_empties(0xE1) != 1) RET_ERR
      if (linked_cells(0xE2) != 1) RET_ERR // T
      if (linked_empties(0xE2) != 1) RET_ERR
   }
   if (rf0) {
      // if (linked_cells(0xF0) != 1) RET_ERR // 
   }
   if (roth) {
      if (linked_cells(':') != 2) RET_ERR
      if (linked_empties(':') != 1) RET_ERR
      if (linked_cells(';') != 2) RET_ERR
      if (linked_empties(';') != 1) RET_ERR
   }
   #undef RET_ERR
   return 1;
}

inline int pretest_font(unsigned pix, unsigned chr, unsigned shift)
{
   unsigned i; //Alone Coder 0.36.7
   // check space
   for (/*unsigned*/ i = 0; i < fontsize; i++)
      if ((font_base[(font_address + i*pix + chr * 0x20) & font_maxmem] >> shift) & 0x0F) return 0;
   // check non-spaces
   for (i = 0; i < 0x100; i++) {
      if (!rmask[i]) continue;
      unsigned char s = 0;
      for (unsigned k = 0; k < fontsize; k++)
         s += (font_base[(font_address + k*pix + i*chr) & font_maxmem] >> shift) & 0x0F;
      if (!s) return 0;
   }
   return 1;
}

void set_data(HWND dlg)
{
   unsigned prev = block_font_dialog;
   block_font_dialog = 1;
   char ln[64]; sprintf(ln, "0%04X", font_address);
   SetDlgItemText(dlg, IDE_ADDRESS, ln);

   if (linear)
      CheckDlgButton(dlg, IDC_PLANES, BST_UNCHECKED),
      CheckDlgButton(dlg, IDC_LINEAR, BST_CHECKED);
   else
      CheckDlgButton(dlg, IDC_PLANES, BST_CHECKED),
      CheckDlgButton(dlg, IDC_LINEAR, BST_UNCHECKED);

   if (right)
      CheckDlgButton(dlg, IDC_LEFT, BST_UNCHECKED),
      CheckDlgButton(dlg, IDC_RIGHT, BST_CHECKED);
   else
      CheckDlgButton(dlg, IDC_LEFT, BST_CHECKED),
      CheckDlgButton(dlg, IDC_RIGHT, BST_UNCHECKED);
   block_font_dialog = prev;
}

void fnt_search(HWND dlg)
{
   create_maps();
   memset(pattern, 0, sizeof pattern);

   unsigned start = font_address,
            st_linear = linear, st_right = right;

   unsigned pix = linear? 1 : 0x100,
            chr = linear? 8 : 1,
            shift = right? 0 : 4;

   for (;;) {
      font_address = (font_address+1) & font_maxmem;
      if (font_address == start) {
         right ^= 1; shift = right? 0 : 4;
         if (right == st_right) {
            linear ^= 1; pix = linear? 1 : 0x100, chr = linear? 8 : 1;
            if (linear == st_linear) {
               MessageBox(dlg, "font not found", "font searcher", MB_OK | MB_ICONWARNING);
               return;
            }
         }
      }
      if (!pretest_font(pix, chr, shift)) continue;
      if (is_font()) break;
   }
   paint_font(dlg);
   set_data(dlg);
}

void save_font()
{
   unsigned chr = 1, line = 0x100, shift = 4;
   if (linear) chr = 8, line = 1;
   if (right) shift = 0;
   unsigned char *dst = fontdata2;

   unsigned j; //Alone Coder 0.36.7
   for (unsigned i = 0; i < 0x100; i++) {
      if (!i || i == 0x20) continue;
      if (!rmask[i]) continue;
      unsigned chardata = font_address + i*chr;
      unsigned char sum = 0;
      for (/*unsigned*/ j = 0; j < fontsize; j++) sum |= font_base[(chardata + j*line) & font_maxmem];
      if (!((sum >> shift) & 0x0F)) continue;
      *dst++ = (unsigned char)i;
      for (j = 0; j < 8; j++)
         *dst++ = j < fontsize?
                   (font_base[(chardata + j*line) & font_maxmem] >> shift) & 0x0F : 0;
   }
   *dst = 0;
   fontdata = fontdata2;
}

void FontFromFile(HWND dlg)
{
   OPENFILENAME ofn = { 0 };
   char fname[0x200]; *fname = 0;

   ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
   ofn.hwndOwner = dlg;
   ofn.lpstrFilter = "font files (*.FNT,*.FNX)\0*.fnt;*.fnx\0All files\0*.*\0";
   ofn.lpstrFile = fname; ofn.nMaxFile = sizeof fname;
   ofn.lpstrTitle = "Load font from file";
   ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;
   if (!GetOpenFileName(&ofn)) return;

   FILE *ff = fopen(fname, "rb"); if (!ff) return;
   memset(font_base = snbuf, 0, sizeof snbuf);
   unsigned size = fread(snbuf, 1, sizeof snbuf, ff);
   fclose(ff);

   for (font_maxmem = 0x800; font_maxmem < size; font_maxmem *= 2);
   font_maxmem--;

   SetDlgItemText(dlg, IDE_ADDRESS, "0");
   get_ranges(dlg);
   paint_font(dlg);
}

INT_PTR CALLBACK fonts_dlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
   if (msg == WM_INITDIALOG) {
      block_font_dialog = 1;
      font_maxmem = conf.ramsize * 1024 - 1;
      font_base = RAM_BASE_M;
      CheckDlgButton(dlg, IDC_100, x100? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_R212F, r21? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_R3039, r30? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_R415A, r41? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_R617A, r61? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_R809F, r80? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_RA0AF, rae? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_RF0F1, rf0? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(dlg, IDC_ROTH2, roth? BST_CHECKED : BST_UNCHECKED);
      SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)"5 pixels");
      SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)"6 pixels");
      SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)"7 pixels");
      SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_ADDSTRING, 0, (LPARAM)"8 pixels");
      SendDlgItemMessage(dlg, IDC_FONTSIZE, CB_SETCURSEL, fontsize-5, 0);
      set_data(dlg);
      update_rmask();
      block_font_dialog = 0;
      return 1;
   }

   if (msg == WM_PAINT) { paint_font(dlg, 1); return 1; }
   if (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE) EndDialog(dlg, 0);

   if (block_font_dialog) return 0;

   if (msg == WM_NOTIFY && wp == IDC_SPIN) {
      char ln[64]; GetDlgItemText(dlg, IDE_ADDRESS, ln, sizeof ln);
      sscanf(ln, "%X", &font_address);
      font_address += ((LPNMUPDOWN)lp)->iDelta * (x100? 0x100 : 1);
      font_address &= font_maxmem;
      sprintf(ln, "0%04X", font_address); SetDlgItemText(dlg, IDE_ADDRESS, ln);
      paint_font(dlg);
      return 0;
   }

   if (msg != WM_COMMAND) return 0;
   unsigned id = LOWORD(wp), code = HIWORD(wp);

   if ((id == IDE_ADDRESS && code == EN_CHANGE) ||
       (id == IDC_FONTSIZE && code == CBN_SELCHANGE) ||
       ((id == IDC_LINEAR || id == IDC_PLANES ||
         id == IDC_LEFT || id == IDC_RIGHT || id == IDC_100 ||
         id == IDC_R212F || id == IDC_R3039 || id == IDC_R415A ||
         id == IDC_R617A || id == IDC_R809F || id == IDC_RA0AF ||
         id == IDC_RF0F1 || id == IDC_ROTH2) && code == BN_CLICKED))
   {
      get_ranges(dlg);
      paint_font(dlg);
   }

   if (id == IDC_FILE) FontFromFile(dlg);
   if (id == IDC_FIND) fnt_search(dlg);
   if (id == IDCANCEL) EndDialog(dlg, 0);
   if (id == IDOK) save_font(), EndDialog(dlg, 0);

   return 0;
}

void font_setup(HWND dlg)
{
   DialogBox(hIn, MAKEINTRESOURCE(IDD_FONTS), dlg, fonts_dlg);
}
#endif // MOD_SETTINGS
