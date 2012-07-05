#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "dxr_512.h"

void line8_512(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   for (unsigned x = 0; x < 512; x += 64) {
      unsigned s = *(unsigned*)src;
      unsigned t = *(unsigned*)(src+0x2000);
      *(unsigned*)(dst+x)    = tab[((s >> 4)  & 0xF)];
      *(unsigned*)(dst+x+4)  = tab[((s >> 0)  & 0xF)];
      *(unsigned*)(dst+x+8)  = tab[((t >> 4)  & 0xF)];
      *(unsigned*)(dst+x+12) = tab[((t >> 0)  & 0xF)];
      *(unsigned*)(dst+x+16) = tab[((s >> 12) & 0xF)];
      *(unsigned*)(dst+x+20) = tab[((s >> 8)  & 0xF)];
      *(unsigned*)(dst+x+24) = tab[((t >> 12) & 0xF)];
      *(unsigned*)(dst+x+28) = tab[((t >> 8)  & 0xF)];
      *(unsigned*)(dst+x+32) = tab[((s >> 20) & 0xF)];
      *(unsigned*)(dst+x+36) = tab[((s >> 16) & 0xF)];
      *(unsigned*)(dst+x+40) = tab[((t >> 20) & 0xF)];
      *(unsigned*)(dst+x+44) = tab[((t >> 16) & 0xF)];
      *(unsigned*)(dst+x+48) = tab[((s >> 28) & 0xF)];
      *(unsigned*)(dst+x+52) = tab[((s >> 24) & 0xF)];
      *(unsigned*)(dst+x+56) = tab[((t >> 28) & 0xF)];
      *(unsigned*)(dst+x+60) = tab[((t >> 24) & 0xF)];
      src += 4;
   }
}
void line16_512(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   for (unsigned x = 0; x < 1024; x += 128) {
      unsigned s = *(unsigned*)src;
      unsigned t = *(unsigned*)(src+0x2000);
      *(unsigned*)(dst+x+0x00) = tab[((s >> 6)  & 0x03)];
      *(unsigned*)(dst+x+0x04) = tab[((s >> 4)  & 0x03)];
      *(unsigned*)(dst+x+0x08) = tab[((s >> 2)  & 0x03)];
      *(unsigned*)(dst+x+0x0C) = tab[((s >> 0)  & 0x03)];
      *(unsigned*)(dst+x+0x10) = tab[((t >> 6)  & 0x03)];
      *(unsigned*)(dst+x+0x14) = tab[((t >> 4)  & 0x03)];
      *(unsigned*)(dst+x+0x18) = tab[((t >> 2)  & 0x03)];
      *(unsigned*)(dst+x+0x1C) = tab[((t >> 0)  & 0x03)];
      *(unsigned*)(dst+x+0x20) = tab[((s >> 14) & 0x03)];
      *(unsigned*)(dst+x+0x24) = tab[((s >> 12) & 0x03)];
      *(unsigned*)(dst+x+0x28) = tab[((s >> 10) & 0x03)];
      *(unsigned*)(dst+x+0x2C) = tab[((s >> 8)  & 0x03)];
      *(unsigned*)(dst+x+0x30) = tab[((t >> 14) & 0x03)];
      *(unsigned*)(dst+x+0x34) = tab[((t >> 12) & 0x03)];
      *(unsigned*)(dst+x+0x38) = tab[((t >> 10) & 0x03)];
      *(unsigned*)(dst+x+0x3C) = tab[((t >> 8)  & 0x03)];
      *(unsigned*)(dst+x+0x40) = tab[((s >> 22) & 0x03)];
      *(unsigned*)(dst+x+0x44) = tab[((s >> 20) & 0x03)];
      *(unsigned*)(dst+x+0x48) = tab[((s >> 18) & 0x03)];
      *(unsigned*)(dst+x+0x4C) = tab[((s >> 16) & 0x03)];
      *(unsigned*)(dst+x+0x50) = tab[((t >> 22) & 0x03)];
      *(unsigned*)(dst+x+0x54) = tab[((t >> 20) & 0x03)];
      *(unsigned*)(dst+x+0x58) = tab[((t >> 18) & 0x03)];
      *(unsigned*)(dst+x+0x5C) = tab[((t >> 16) & 0x03)];
      *(unsigned*)(dst+x+0x60) = tab[((s >> 30) & 0x03)];
      *(unsigned*)(dst+x+0x64) = tab[((s >> 28) & 0x03)];
      *(unsigned*)(dst+x+0x68) = tab[((s >> 26) & 0x03)];
      *(unsigned*)(dst+x+0x6C) = tab[((s >> 24) & 0x03)];
      *(unsigned*)(dst+x+0x70) = tab[((t >> 30) & 0x03)];
      *(unsigned*)(dst+x+0x74) = tab[((t >> 28) & 0x03)];
      *(unsigned*)(dst+x+0x78) = tab[((t >> 26) & 0x03)];
      *(unsigned*)(dst+x+0x7C) = tab[((t >> 24) & 0x03)];
      src += 4;
   }
}
void line32_512(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   for (unsigned x = 0; x < 512*4; x += 64) {
      unsigned s = src[0];
      *(unsigned*)(dst+x)    = tab[((s << 1) & 0x100)];
      *(unsigned*)(dst+x+4)  = tab[((s << 2) & 0x100)];
      *(unsigned*)(dst+x+8)  = tab[((s << 3) & 0x100)];
      *(unsigned*)(dst+x+12) = tab[((s << 4) & 0x100)];
      *(unsigned*)(dst+x+16) = tab[((s << 5) & 0x100)];
      *(unsigned*)(dst+x+20) = tab[((s << 6) & 0x100)];
      *(unsigned*)(dst+x+24) = tab[((s << 7) & 0x100)];
      *(unsigned*)(dst+x+28) = tab[((s << 8) & 0x100)];
      s = src[0x2000];
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

#define ATTR_512 0x07
void r512_8s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line8_512(dst, temp.base+t.scrtab[y], t.sctab8[0]+ATTR_512*0x10);
      dst += pitch;
   }
}
void r512_8d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line8_512(dst, temp.base+t.scrtab[y], t.sctab8[0]+ATTR_512*0x10); dst += pitch;
      line8_512(dst, temp.base+t.scrtab[y], t.sctab8[1]+ATTR_512*0x10); dst += pitch;
   }
}
void r512_16s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line16_512(dst, temp.base+t.scrtab[y], t.sctab16[0]+ATTR_512*4);
      dst += pitch;
   }
}
void r512_16d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line16_512(dst, temp.base+t.scrtab[y], t.sctab16[0]+ATTR_512*4); dst += pitch;
      line16_512(dst, temp.base+t.scrtab[y], t.sctab16[1]+ATTR_512*4); dst += pitch;
   }
}
void r512_32s(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line32_512(dst, temp.base+t.scrtab[y], t.sctab32[0]+ATTR_512);
      dst += pitch;
   }
}
void r512_32d(unsigned char *dst, unsigned pitch)
{
   for (unsigned y = 0; y < 192; y++) {
      line32_512(dst, temp.base+t.scrtab[y], t.sctab32[0]+ATTR_512); dst += pitch;
      line32_512(dst, temp.base+t.scrtab[y], t.sctab32[1]+ATTR_512); dst += pitch;
   }
}

void rend_512(unsigned char *dst, unsigned pitch)
{
   unsigned char *dst2 = dst + temp.b_left*temp.obpp/4 +
                       temp.b_top*pitch * ((temp.oy > temp.scy)?2:1);
   if (temp.oy > temp.scy && conf.fast_sl) pitch *= 2;

   if (temp.obpp == 8)  { if (conf.fast_sl) rend_frame_8d1 (dst, pitch), r512_8s (dst2, pitch); else rend_frame_8d (dst, pitch), r512_8d (dst2, pitch); return; }
   if (temp.obpp == 16) { if (conf.fast_sl) rend_frame_16d1(dst, pitch), r512_16s(dst2, pitch); else rend_frame_16d(dst, pitch), r512_16d(dst2, pitch); return; }
   if (temp.obpp == 32) { if (conf.fast_sl) rend_frame_32d1(dst, pitch), r512_32s(dst2, pitch); else rend_frame_32d(dst, pitch), r512_32d(dst2, pitch); return; }
}

