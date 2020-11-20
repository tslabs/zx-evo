#include "std.h"
#include "drawnomc.h"

#if 0
void draw_border1()
{
   unsigned br = comp.border_attr * 0x11001100;
   for (unsigned i = 0; i < temp.scx*temp.scy/4; i+=4)
      *(unsigned*)(rbuf + i) = br;
}

void draw_alco();
void draw_gigascreen_no_border();

void draw_screen()
{
/* [vv] Отключен, т.к. этот бит использется для DDp scroll
   if (comp.pEFF7 & EFF7_384)
   {
       draw_alco();
       return;
   }
*/

   if (conf.nopaper)
   {
       draw_border1();
       return;
   }
//   if (comp.pEFF7 & EFF7_GIGASCREEN) { draw_border1(); draw_gigascreen_no_border(); return; } //Alone Coder

   u8 *dst = rbuf;
   unsigned br = comp.border_attr * 0x11001100;

   unsigned i;
   for (i = temp.b_top * temp.scx / 16; i; i--)
   {
      *(unsigned*)dst = br;
      dst += 4;
   }

   for (int y = 0; y < 192; y++)
   {
//[vv] *(volatile unsigned*)dst;

      unsigned x;
      for (x = temp.b_left; x; x -= 16)
      {
         *(unsigned*)dst = br;
         dst += 4;
      }

      for (x = 0; x < 32; x++)
      {
//[vv]   *(volatile u8*)dst;
//         *dst++ = temp.base[t.scrtab[y] + x];
//         *dst++ = colortab[temp.base[atrtab[y] + x]];
      }

//[vv]      *(volatile unsigned*)dst;
      for (x = temp.b_right; x; x -= 16)
      {
         *(unsigned*)dst = br;
         dst += 4;
      }
   }

   for (i = temp.b_bottom*temp.scx / 16; i; i--)
   {
      *(unsigned*)dst = br;
      dst += 4;
   }
}

void draw_gigascreen_no_border()
{
   u8 *dst = rbuf + (temp.b_top * temp.scx + temp.b_left) / 4;
   u8 * const screen = page_ram(5);
   unsigned offset = (comp.frame_counter & 1)? 0 : 2*PAGE;
   for (int y = 0; y < 192; y++) {
      *(volatile u8*)dst;
      for (unsigned x = 0; x < 32; x++) {
         dst[2*x+0] = screen[t.scrtab[y] + x + offset];
         dst[2*x+1] = colortab[screen[atrtab[y] + x + offset]];
      }
      offset ^= 2*PAGE;
      dst += temp.scx / 4;
   }
}
#endif