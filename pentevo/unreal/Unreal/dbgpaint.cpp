#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dx.h"
#include "draw.h"
#include "dxrframe.h"
#include "font16.h"
#include "util.h"

u8 txtscr[80*30*2];

static struct
{
   u8 x,y,dx,dy,c;
} frames[20];

unsigned nfr;

void debugflip()
{
   if (!active)
       return;
   setpal(0);

   u8 * const bptr = gdibuf;

   if (show_scrshot) {
      memcpy(save_buf, rbuf, rb2_offs);
      paint_scr((show_scrshot == 1)? 0 : 2);
      u8 *dst = bptr + wat_y*16*640+wat_x*8;
      u8 *src = rbuf+temp.scx/4*(temp.b_top+192/2-wat_sz*16/2);
      src += temp.scx/8-37/2*2;
      for (unsigned y = 0; y < wat_sz*16; y++) {
         for (unsigned x = 0; x < 37; x++) {
            *(unsigned*)(dst+x*8+0) = t.sctab8[0][(src[x*2] >>  4) + src[2*x+1]*16];
            *(unsigned*)(dst+x*8+4) = t.sctab8[0][(src[x*2] & 0xF) + src[2*x+1]*16];
         }
         src += temp.scx/4, dst += 640;
      }
      memcpy(rbuf, save_buf, rb2_offs);
   }

   // print text
   int x,y;
   u8 *tptr = txtscr;
   for (y = 0; y < 16*30*640; y+=16*640)
   {
      for (x = 0; x < 80; x++, tptr++)
      {
         unsigned ch = *tptr, at = tptr[80*30];
         if (at == 0xFF) continue; // transparent color
         const u8 *fnt = &font16[ch*16];
         at <<= 4;
         for (int yy = 0; yy < 16; yy++, fnt++)
         {
            *(unsigned*)(bptr+y+640*yy+x*8+0) = t.sctab8[0][(*fnt >>  4) + at];
            *(unsigned*)(bptr+y+640*yy+x*8+4) = t.sctab8[0][(*fnt & 0xF) + at];
         }
      }
   }

   // show frames
   for (unsigned i = 0; i < nfr; i++)
   {
      u8 a1 = (frames[i].c | 0x08) * 0x11;
      y = frames[i].y*16-1;
      for (x = 8*frames[i].x-1; x < (frames[i].x+frames[i].dx)*8; x++) bptr[y*640+x] = a1;
      y = (frames[i].y+frames[i].dy)*16;
      for (x = 8*frames[i].x-1; x < (frames[i].x+frames[i].dx)*8; x++) bptr[y*640+x] = a1;
      x = frames[i].x*8-1;
      for (y = 16*frames[i].y; y < (frames[i].y+frames[i].dy)*16; y++) bptr[y*640+x] = a1;
      x = (frames[i].x+frames[i].dx)*8;
      for (y = 16*frames[i].y; y < (frames[i].y+frames[i].dy)*16; y++) bptr[y*640+x] = a1;
   }

   gdibmp.header.bmiHeader.biBitCount = 8;
   if (needclr)
       gdi_frame();
   SetDIBitsToDevice(temp.gdidc, temp.gx, temp.gy, 640, 480, 0, 0, 0, 480, bptr, &gdibmp.header, DIB_RGB_COLORS);
   gdibmp.header.bmiHeader.biBitCount = temp.obpp;
}

void frame(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 attr)
{
   frames[nfr].x = x;
   frames[nfr].y = y;
   frames[nfr].dx = dx;
   frames[nfr].dy = dy;
   frames[nfr].c = attr;
   nfr++;
}

void tprint(unsigned x, unsigned y, const char *str, u8 attr)
{
   for (unsigned ptr = y*80 + x; *str; str++, ptr++) {
      txtscr[ptr] = *str; txtscr[ptr+80*30] = attr;
   }
}

void tprint_fg(unsigned x, unsigned y, const char *str, u8 attr)
{
   for (unsigned ptr = y*80 + x; *str; str++, ptr++) {
      txtscr[ptr] = *str; txtscr[ptr+80*30] = (txtscr[ptr+80*30] & 0xF0) + attr;
   }
}

void filledframe(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 color)
{
   for (unsigned yy = y; yy < (y+dy); yy++)
      for (unsigned xx = x; xx < (x+dx); xx++)
         txtscr[yy*80+xx] = ' ',
         txtscr[yy*80+xx+30*80] = color;
   nfr = 0; // delete other frames while dialog
   frame(x,y,dx,dy,FFRAME_FRAME);
}

void fillattr(unsigned x, unsigned y, unsigned dx, u8 color)
{
   for (unsigned xx = x; xx < (x+dx); xx++)
      txtscr[y*80+xx+30*80] = color;
}

char str[0x80];
unsigned inputhex(unsigned x, unsigned y, unsigned sz, bool hex)
{
   unsigned cr = 0;
   mousepos = 0;

   for (;;)
   {
      str[sz] = 0;

      unsigned i;
      for (i = strlen(str); i < sz; i++)
          str[i] = ' ';
      for (i = 0; i < sz; i++)
      {
         unsigned vl = (u8)str[i];
         tprint(x+i,y,(char*)&vl,(i==cr) ? W_INPUTCUR : W_INPUTBG);
      }

      debugflip();

      unsigned key;
      for (;;Sleep(20))
      {
         key = process_msgs();
         needclr = 0;
         debugflip();

         if (mousepos)
             return 0;
         if (key)
             break;
      }

      switch(key)
      {
      case VK_ESCAPE: return 0;
      case VK_RETURN:
         for (char *ptr = str+sz-1; *ptr == ' ' && ptr >= str; *ptr-- = 0);
         return 1;
      case VK_LEFT:
          if (cr)
              cr--;
          continue;
      case VK_BACK:
          if (cr)
          {
              for (i = cr; i < sz; i++)
                  str[i-1]=str[i];
              str[sz-1] = ' ';
              --cr;
          }
          continue;
      case VK_RIGHT:
          if (cr != sz-1)
              cr++;
          continue;
      case VK_HOME:
          cr=0;
          continue;
      case VK_END:
          for (cr=sz-1; cr && str[cr]==' ' && str[cr-1] == ' '; cr--);
          continue;
      case VK_DELETE:
          for (i = cr; i < sz-1; i++)
              str[i]=str[i+1];
          str[sz-1] = ' ';
          continue;
      case VK_INSERT:
          for (i = sz-1; i > cr; i--)
              str[i]=str[i-1];
          str[cr] = ' ';
          continue;
      }

      if (hex)
      {
         if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F'))
             str[cr++] = (u8)key;
      }
      else
      {
         u8 Kbd[256];
         GetKeyboardState(Kbd);
         u16 k;
         if (ToAscii(key, 0, Kbd, &k, 0) == 1)
         {
             char m;
             if (CharToOemBuff((char *)&k, &m, 1))
                 str[cr++] = m;
         }
      }
      if (cr == sz)
          cr--;
   }
}

unsigned input4(unsigned x, unsigned y, unsigned val)
{
   sprintf(str, "%04X", val);
   if (inputhex(x,y,4,true))
   {
       sscanf(str, "%x", &val);
       return val;
   }
   return -1;
}

unsigned input2(unsigned x, unsigned y, unsigned val)
{
   sprintf(str, "%02X", val);
   if (inputhex(x,y,2,true))
   {
       sscanf(str, "%x", &val);
       return val;
   }
   return -1;
}


void format_item(char *dst, unsigned width, const char *text, MENUITEM::FLAGS flags)
{
   memset(dst, ' ', width+2); dst[width+2] = 0;
   unsigned sz = strlen(text), left = 0;
   if (sz > width) sz = width;
   if (flags & MENUITEM::RIGHT) left = width - sz;
   else if (flags & MENUITEM::CENTER) left = (width - sz)/2;
   memcpy(dst+left+1, text, sz);
}

void paint_items(MENUDEF *menu)
{
   char ln[80]; unsigned item;

   unsigned maxlen = strlen(menu->title);
   for (item = 0; item < menu->n_items; item++) {
      unsigned sz = strlen(menu->items[item].text);
      maxlen = max(maxlen, sz);
   }
   unsigned menu_dx = maxlen+2, menu_dy = menu->n_items + 3;
   unsigned menu_x = (80 - menu_dx)/2, menu_y = (30 - menu_dy)/2;
   filledframe(menu_x, menu_y, menu_dx, menu_dy, MENU_INSIDE);
   format_item(ln, maxlen, menu->title, MENUITEM::CENTER);
   tprint(menu_x, menu_y, ln, MENU_HEADER);

   for (/*unsigned*/ item = 0; item < menu->n_items; item++) {
      u8 color = MENU_ITEM;
      if (menu->items[item].flags & MENUITEM::DISABLED) color = MENU_ITEM_DIS;
      else if (item == menu->pos) color = MENU_CURSOR;
      format_item(ln, maxlen, menu->items[item].text, menu->items[item].flags);
      tprint(menu_x, menu_y+item+2, ln, color);
   }
}

void menu_move(MENUDEF *menu, int dir)
{
   unsigned start = menu->pos;
   for (;;) {
      menu->pos += dir;
      if ((int)menu->pos == -1) menu->pos = menu->n_items-1;
      if (menu->pos >= menu->n_items) menu->pos = 0;
      if (!(menu->items[menu->pos].flags & MENUITEM::DISABLED)) return;
      if (menu->pos == start) return;
   }
}

char handle_menu(MENUDEF *menu)
{
   if (menu->items[menu->pos].flags & MENUITEM::DISABLED)
      menu_move(menu, 1);
   for (;;)
   {
      paint_items(menu);
      debugflip();

      unsigned key;
      for (;;Sleep(20))
      {
         key = process_msgs();
         needclr =  0;
         debugflip();

         if (mousepos)
             return 0;
         if (key)
             break;
      }
      if (key == VK_ESCAPE)
          return 0;
      if (key == VK_RETURN || key == VK_SPACE)
          return 1;
      if (key == VK_UP || key == VK_LEFT)
          menu_move(menu, -1);
      if (key == VK_DOWN || key == VK_RIGHT)
          menu_move(menu, 1);
      if (key == VK_HOME || key == VK_PRIOR)
      {
          menu->pos = -1;
          menu_move(menu, 1);
      }
      if (key == VK_END || key == VK_NEXT)
      {
          menu->pos = menu->n_items;
          menu_move(menu, -1);
      }
   }
}
