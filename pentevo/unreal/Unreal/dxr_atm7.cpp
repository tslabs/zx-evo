#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "dxr_atmf.h"
#include "dxr_atm7.h"
#include "fontatm2.h"

const int text_ofs = 0x1840;

void line_atm7_8(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font)
{
   unsigned i = 0;
   for (unsigned x = 0; x < 512; x += 0x10) {
      unsigned chr = src[(i+1) & 0x1F]; i++;
      unsigned *tab = tab0 + (chr << 2);
      chr = font[chr];
      *(unsigned*)(dst+x+0x00) = tab[(chr >> 6) & 0x03];
      *(unsigned*)(dst+x+0x04) = tab[(chr >> 4) & 0x03];
      *(unsigned*)(dst+x+0x08) = tab[(chr >> 2) & 0x03];
      *(unsigned*)(dst+x+0x0C) = tab[chr & 0x03];
   }
}

void line_atm7_16(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font)
{
   unsigned i = 0;
   for (unsigned x = 0; x < 512*2; x += 0x20) {
      unsigned chr = src[(i+1) & 0x1F]; i++;
      unsigned *tab = tab0+chr;
      chr = font[chr];
      *(unsigned*)(dst+x+0x00) = tab[((chr << 1) & 0x100)];
      *(unsigned*)(dst+x+0x04) = tab[((chr << 2) & 0x100)];
      *(unsigned*)(dst+x+0x08) = tab[((chr << 3) & 0x100)];
      *(unsigned*)(dst+x+0x0C) = tab[((chr << 4) & 0x100)];
      *(unsigned*)(dst+x+0x10) = tab[((chr << 5) & 0x100)];
      *(unsigned*)(dst+x+0x14) = tab[((chr << 6) & 0x100)];
      *(unsigned*)(dst+x+0x18) = tab[((chr << 7) & 0x100)];
      *(unsigned*)(dst+x+0x1C) = tab[((chr << 8) & 0x100)];
   }
}

void line_atm7_32(unsigned char *dst, unsigned char *src, unsigned *tab0, unsigned char *font)
{
   unsigned i = 0;
   for (unsigned x = 0; x < 512*4; x += 0x40) {
      unsigned chr = src[(i+1) & 0x1F]; i++;
      unsigned *tab = tab0+chr;
      chr = font[chr];
      *(unsigned*)(dst+x+0x00) = *(unsigned*)(dst+x+0x04) = tab[((chr << 1) & 0x100)];
      *(unsigned*)(dst+x+0x08) = *(unsigned*)(dst+x+0x0C) = tab[((chr << 2) & 0x100)];
      *(unsigned*)(dst+x+0x10) = *(unsigned*)(dst+x+0x14) = tab[((chr << 3) & 0x100)];
      *(unsigned*)(dst+x+0x18) = *(unsigned*)(dst+x+0x1C) = tab[((chr << 4) & 0x100)];
      *(unsigned*)(dst+x+0x20) = *(unsigned*)(dst+x+0x24) = tab[((chr << 5) & 0x100)];
      *(unsigned*)(dst+x+0x28) = *(unsigned*)(dst+x+0x2C) = tab[((chr << 6) & 0x100)];
      *(unsigned*)(dst+x+0x30) = *(unsigned*)(dst+x+0x34) = tab[((chr << 7) & 0x100)];
      *(unsigned*)(dst+x+0x38) = *(unsigned*)(dst+x+0x3C) = tab[((chr << 8) & 0x100)];
   }
}

void r_atm7_8s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_8(dst, temp.base + text_ofs + y, t.zctab8ad[0], fontatm2 + v*0x100); dst += pitch;
         }
}

void r_atm7_8d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_8(dst, temp.base + text_ofs + y, t.zctab8ad[0], fontatm2 + v*0x100); dst += pitch;
            line_atm7_8(dst, temp.base + text_ofs + y, t.zctab8ad[1], fontatm2 + v*0x100); dst += pitch;
         }
}

void r_atm7_16s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_16(dst, temp.base + text_ofs + y, t.zctab16ad[0], fontatm2 + v*0x100); dst += pitch;
         }
}

void r_atm7_16d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_16(dst, temp.base + text_ofs + y, t.zctab16ad[0], fontatm2 + v*0x100); dst += pitch;
            line_atm7_16(dst, temp.base + text_ofs + y, t.zctab16ad[1], fontatm2 + v*0x100); dst += pitch;
         }
}

void r_atm7_32s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_32(dst, temp.base + text_ofs + y, t.zctab32ad[0], fontatm2 + v*0x100); dst += pitch;
         }
}

void r_atm7_32d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y += 32)
      for (unsigned s = 0; s < 4; s++)
         for (unsigned v = 0; v < 8; v++) {
            line_atm7_32(dst, temp.base + text_ofs + y, t.zctab32ad[0], fontatm2 + v*0x100); dst += pitch;
            line_atm7_32(dst, temp.base + text_ofs + y, t.zctab32ad[1], fontatm2 + v*0x100); dst += pitch;
         }
}

void rend_atm7(unsigned char *dst, unsigned pitch)
{
   unsigned char *dst2 = dst + (temp.ox-512)*temp.obpp/16;
   if (temp.scy > 200) dst2 += (temp.scy-192)/2*pitch * ((temp.oy > temp.scy)?2:1);
   if (temp.oy > temp.scy && conf.fast_sl) pitch *= 2;

   if (temp.obpp == 8)  { if (conf.fast_sl) rend_frame_8d1(dst, pitch), r_atm7_8s (dst2, pitch); else rend_frame_8d (dst, pitch), r_atm7_8d (dst2, pitch); return; }
   if (temp.obpp == 16) { if (conf.fast_sl) rend_frame_16d1(dst, pitch), r_atm7_16s(dst2, pitch); else rend_frame_16d(dst, pitch), r_atm7_16d(dst2, pitch); return; }
   if (temp.obpp == 32) { if (conf.fast_sl) rend_frame_32d1(dst, pitch), r_atm7_32s(dst2, pitch); else rend_frame_32d(dst, pitch), r_atm7_32d(dst2, pitch); return; }
}
