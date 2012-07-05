#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "dxrcopy.h"
#include "dxr_prof.h"

void line8_prof(unsigned char *dst, unsigned char *src, unsigned *tab0)
{
   for (unsigned x = 0; x < 512; x += 64) {
      unsigned s = *(unsigned*)(src+0x2000);
      unsigned t = *(unsigned*)src;
      unsigned as = *(unsigned*)(src + 0x2000 + 0x34*PAGE);
      unsigned at = *(unsigned*)(src + 0x34*PAGE);
      unsigned *tab = tab0 + ((as << 4) & 0xFF0);
      *(unsigned*)(dst+x)    = tab[((s >> 4)  & 0xF)];
      *(unsigned*)(dst+x+4)  = tab[((s >> 0)  & 0xF)];
      tab = tab0 + ((at << 4) & 0xFF0);
      *(unsigned*)(dst+x+8)  = tab[((t >> 4)  & 0xF)];
      *(unsigned*)(dst+x+12) = tab[((t >> 0)  & 0xF)];
      tab = tab0 + ((as >> 4) & 0xFF0);
      *(unsigned*)(dst+x+16) = tab[((s >> 12) & 0xF)];
      *(unsigned*)(dst+x+20) = tab[((s >> 8)  & 0xF)];
      tab = tab0 + ((at >> 4) & 0xFF0);
      *(unsigned*)(dst+x+24) = tab[((t >> 12) & 0xF)];
      *(unsigned*)(dst+x+28) = tab[((t >> 8)  & 0xF)];
      tab = tab0 + ((as >> 12) & 0xFF0);
      *(unsigned*)(dst+x+32) = tab[((s >> 20) & 0xF)];
      *(unsigned*)(dst+x+36) = tab[((s >> 16) & 0xF)];
      tab = tab0 + ((at >> 12) & 0xFF0);
      *(unsigned*)(dst+x+40) = tab[((t >> 20) & 0xF)];
      *(unsigned*)(dst+x+44) = tab[((t >> 16) & 0xF)];
      tab = tab0 + ((as >> 20) & 0xFF0);
      *(unsigned*)(dst+x+48) = tab[((s >> 28) & 0xF)];
      *(unsigned*)(dst+x+52) = tab[((s >> 24) & 0xF)];
      tab = tab0 + ((at >> 20) & 0xFF0);
      *(unsigned*)(dst+x+56) = tab[((t >> 28) & 0xF)];
      *(unsigned*)(dst+x+60) = tab[((t >> 24) & 0xF)];
      src += 4;
   }
}

void line16_prof(unsigned char *dst, unsigned char *src, unsigned *tab0)
{
   for (unsigned x = 0; x < 1024; x += 128) {
      unsigned s = *(unsigned*)(src+0x2000);
      unsigned t = *(unsigned*)src;
      unsigned as = *(unsigned*)(src + 0x2000 + 0x34*PAGE);
      unsigned at = *(unsigned*)(src + 0x34*PAGE);
      unsigned *tab = tab0 + ((as << 2) & 0x3FC);
      *(unsigned*)(dst+x+0x00) = tab[((s >> 6)  & 0x03)];
      *(unsigned*)(dst+x+0x04) = tab[((s >> 4)  & 0x03)];
      *(unsigned*)(dst+x+0x08) = tab[((s >> 2)  & 0x03)];
      *(unsigned*)(dst+x+0x0C) = tab[((s >> 0)  & 0x03)];
      tab = tab0 + ((at << 2) & 0x3FC);
      *(unsigned*)(dst+x+0x10) = tab[((t >> 6)  & 0x03)];
      *(unsigned*)(dst+x+0x14) = tab[((t >> 4)  & 0x03)];
      *(unsigned*)(dst+x+0x18) = tab[((t >> 2)  & 0x03)];
      *(unsigned*)(dst+x+0x1C) = tab[((t >> 0)  & 0x03)];
      tab = tab0 + ((as >> 6) & 0x3FC);
      *(unsigned*)(dst+x+0x20) = tab[((s >> 14) & 0x03)];
      *(unsigned*)(dst+x+0x24) = tab[((s >> 12) & 0x03)];
      *(unsigned*)(dst+x+0x28) = tab[((s >> 10) & 0x03)];
      *(unsigned*)(dst+x+0x2C) = tab[((s >> 8)  & 0x03)];
      tab = tab0 + ((at >> 6) & 0x3FC);
      *(unsigned*)(dst+x+0x30) = tab[((t >> 14) & 0x03)];
      *(unsigned*)(dst+x+0x34) = tab[((t >> 12) & 0x03)];
      *(unsigned*)(dst+x+0x38) = tab[((t >> 10) & 0x03)];
      *(unsigned*)(dst+x+0x3C) = tab[((t >> 8)  & 0x03)];
      tab = tab0 + ((as >> 14) & 0x3FC);
      *(unsigned*)(dst+x+0x40) = tab[((s >> 22) & 0x03)];
      *(unsigned*)(dst+x+0x44) = tab[((s >> 20) & 0x03)];
      *(unsigned*)(dst+x+0x48) = tab[((s >> 18) & 0x03)];
      *(unsigned*)(dst+x+0x4C) = tab[((s >> 16) & 0x03)];
      tab = tab0 + ((at >> 14) & 0x3FC);
      *(unsigned*)(dst+x+0x50) = tab[((t >> 22) & 0x03)];
      *(unsigned*)(dst+x+0x54) = tab[((t >> 20) & 0x03)];
      *(unsigned*)(dst+x+0x58) = tab[((t >> 18) & 0x03)];
      *(unsigned*)(dst+x+0x5C) = tab[((t >> 16) & 0x03)];
      tab = tab0 + ((as >> 22) & 0x3FC);
      *(unsigned*)(dst+x+0x60) = tab[((s >> 30) & 0x03)];
      *(unsigned*)(dst+x+0x64) = tab[((s >> 28) & 0x03)];
      *(unsigned*)(dst+x+0x68) = tab[((s >> 26) & 0x03)];
      *(unsigned*)(dst+x+0x6C) = tab[((s >> 24) & 0x03)];
      tab = tab0 + ((at >> 22) & 0x3FC);
      *(unsigned*)(dst+x+0x70) = tab[((t >> 30) & 0x03)];
      *(unsigned*)(dst+x+0x74) = tab[((t >> 28) & 0x03)];
      *(unsigned*)(dst+x+0x78) = tab[((t >> 26) & 0x03)];
      *(unsigned*)(dst+x+0x7C) = tab[((t >> 24) & 0x03)];
      src += 4;
   }
}

void line32_prof(unsigned char *dst, unsigned char *src, unsigned *tab0)
{
   for (unsigned x = 0; x < 512*4; x += 64) {
      unsigned s = src[0x2000], *tab = tab0 + src[0x2000 + 0x34*PAGE];
      *(unsigned*)(dst+x)    = tab[((s << 1) & 0x100)];
      *(unsigned*)(dst+x+4)  = tab[((s << 2) & 0x100)];
      *(unsigned*)(dst+x+8)  = tab[((s << 3) & 0x100)];
      *(unsigned*)(dst+x+12) = tab[((s << 4) & 0x100)];
      *(unsigned*)(dst+x+16) = tab[((s << 5) & 0x100)];
      *(unsigned*)(dst+x+20) = tab[((s << 6) & 0x100)];
      *(unsigned*)(dst+x+24) = tab[((s << 7) & 0x100)];
      *(unsigned*)(dst+x+28) = tab[((s << 8) & 0x100)];
      s = src[0]; tab = tab0 + src[0x34*PAGE];
      *(unsigned*)(dst+x+32) = tab[((s << 1) & 0x100)];
      *(unsigned*)(dst+x+36) = tab[((s << 2) & 0x100)];
      *(unsigned*)(dst+x+40) = tab[((s << 3) & 0x100)];
      *(unsigned*)(dst+x+44) = tab[((s << 4) & 0x100)];
      *(unsigned*)(dst+x+48) = tab[((s << 5) & 0x100)];
      *(unsigned*)(dst+x+52) = tab[((s << 6) & 0x100)];
      *(unsigned*)(dst+x+56) = tab[((s << 7) & 0x100)];
      *(unsigned*)(dst+x+60) = tab[((s << 8) & 0x100)];
      src++;
   }
}

void r_profi_8(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line8_prof(dst, base+t.scrtab[y], t.zctab8[0]);
      dst += pitch;
   }
}

void r_profi_8d(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line8_prof(dst, base+t.scrtab[y], t.zctab8[0]); dst += pitch;
      line8_prof(dst, base+t.scrtab[y], t.zctab8[1]); dst += pitch;
   }
}

void r_profi_16(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line16_prof(dst, base+t.scrtab[y], t.zctab16[0]);
      dst += pitch;
   }
}

void r_profi_16d(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line16_prof(dst, base+t.scrtab[y], t.zctab16[0]); dst += pitch;
      line16_prof(dst, base+t.scrtab[y], t.zctab16[1]); dst += pitch;
   }
}

void r_profi_32(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line32_prof(dst, base+t.scrtab[y], t.zctab32[0]);
      dst += pitch;
   }
}

void r_profi_32d(unsigned char *dst, unsigned pitch, unsigned char *base)
{
   unsigned max = min(240, temp.scy);
   for (unsigned y = 0; y < max; y++) {
      line32_prof(dst, base+t.scrtab[y], t.zctab32[0]); dst += pitch;
      line32_prof(dst, base+t.scrtab[y], t.zctab32[1]); dst += pitch;
   }
}

void rend_profi(unsigned char *dst, unsigned pitch)
{
   static unsigned char dffd = -1;
   if ((comp.pDFFD ^ dffd) & 0x80)
   {
      video_color_tables();
      dffd = comp.pDFFD;
      needclr = 2;
   }

   if (temp.comp_pal_changed)
   {
      pixel_tables();
      temp.comp_pal_changed = 0;
   }

   unsigned char *dst2 = dst + (temp.ox-512)*temp.obpp/16;
   if (temp.scy > 240)
       dst2 += (temp.scy-240)/2*pitch * ((temp.oy > temp.scy)?2:1);
   if (temp.oy > temp.scy && conf.fast_sl)
       pitch *= 2;

   unsigned char *base = memory + ((comp.p7FFD & 0x08) ? 6 * PAGE : 4 * PAGE);
   if (temp.obpp == 8)
   {
       if (conf.fast_sl)
           r_profi_8 (dst2, pitch, base);
       else
           r_profi_8d (dst2, pitch, base);
       return;
   }
   if (temp.obpp == 16)
   {
       if (conf.fast_sl)
           r_profi_16(dst2, pitch, base);
       else
           r_profi_16d(dst2, pitch, base);
       return;
   }
   if (temp.obpp == 32)
   {
       if (conf.fast_sl)
           r_profi_32(dst2, pitch, base);
       else
           r_profi_32d(dst2, pitch, base);
       return;
   }
}
