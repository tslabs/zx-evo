#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "fontdata.h"
#include "font8.h"
#include "font14.h"
#include "font16.h"
#include "init.h"
#include "util.h"

static void *root_tab;
const unsigned char *fontdata = fontdata1;

static void recognize_text(const unsigned char *st, unsigned char *d[])
{
    void **dst = (void **)d;
    const u32 *start = (u32 *)st;
    for (unsigned i = 0; i < 64; i++)
    {
        dst[i] = root_tab;
    }

    for (unsigned j = conf.fontsize; j != 0; j--, start += temp.scx >> 4)
    {
        for (unsigned i = 0; i < 64 / 4; i++)
        {
            dst[4*i+0] = ((void **)dst[4*i+0])[(start[i] >> 4) & 0xF];
            dst[4*i+1] = ((void **)dst[4*i+1])[(start[i] & 0xF)];
            dst[4*i+2] = ((void **)dst[4*i+2])[(start[i] >> 20) & 0xF];
            dst[4*i+3] = ((void **)dst[4*i+3])[(start[i] >> 16) & 0xF];
        }
    }
}

#define line_a64_8_2 line_a64_8 // scanlines выглядит очень плохо с шрифтом 8x16
#define line_a64_8_1 line_a64_8 // поэтому scanlines не поддеpжан
#define line_a64_16_2 line_a64_16
#define line_a64_16_1 line_a64_16
#define line_a64_32_2 line_a64_32
#define line_a64_32_1 line_a64_32

void line_a64_8(unsigned char *dst, unsigned char *src, unsigned char *chars[], unsigned line)
{
   for (unsigned x = 0; x < 512; x += 32) {
      unsigned s = *(unsigned*)src;
      unsigned attr = (s >> 6) & 0x3FC;
      unsigned char *ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+ 0) = t.sctab8d[0][((s >> 6) & 3) + attr];
         *(unsigned*)(dst+x+ 4) = t.sctab8d[0][((s >> 4) & 3) + attr];
      } else {
         unsigned attr = 0xF0*4;
         *(unsigned*)(dst+x+ 0) = t.sctab8[0][((ptr[line] >> 4) & 0xF) + attr*4];
         *(unsigned*)(dst+x+ 4) = t.sctab8[0][((ptr[line]     ) & 0xF) + attr*4];
      }
      ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+ 8) = t.sctab8d[0][((s >> 2) & 3) + attr];
         *(unsigned*)(dst+x+12) = t.sctab8d[0][((s >> 0) & 3) + attr];
      } else {
         unsigned attr = 0xF0*4;
         *(unsigned*)(dst+x+ 8) = t.sctab8[0][((ptr[line] >> 4) & 0xF) + attr*4];
         *(unsigned*)(dst+x+12) = t.sctab8[0][((ptr[line]     ) & 0xF) + attr*4];
      }
      ptr = *chars++;
      attr = (s >> 22) & 0x3FC;
      if (!ptr) {
         *(unsigned*)(dst+x+16) = t.sctab8d[0][((s >>22) & 3) + attr];
         *(unsigned*)(dst+x+20) = t.sctab8d[0][((s >>20) & 3) + attr];
      } else {
         unsigned attr = 0xF0*4;
         *(unsigned*)(dst+x+16) = t.sctab8[0][((ptr[line] >> 4) & 0xF) + attr*4];
         *(unsigned*)(dst+x+20) = t.sctab8[0][((ptr[line]     ) & 0xF) + attr*4];
      }
      ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+24) = t.sctab8d[0][((s >>18) & 3) + attr];
         *(unsigned*)(dst+x+28) = t.sctab8d[0][((s >>16) & 3) + attr];
      } else {
         unsigned attr = 0xF0*4;
         *(unsigned*)(dst+x+24) = t.sctab8[0][((ptr[line] >> 4) & 0xF) + attr*4];
         *(unsigned*)(dst+x+28) = t.sctab8[0][((ptr[line]     ) & 0xF) + attr*4];
      }

      src += 4;
   }
}

void line_a64_16(unsigned char *dst, unsigned char *src, unsigned char *chars[], unsigned line)
{
   for (unsigned x = 0; x < 1024; x += 32) {
      unsigned char s = *src++;
      unsigned attr = *src++;
      unsigned *tab1 = t.sctab16d[0] + attr;
      unsigned *tab2 = t.sctab16 [0] + attr*4;
      unsigned char *ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+ 0) = tab1[(s << 1) & 0x100];
         *(unsigned*)(dst+x+ 4) = tab1[(s << 2) & 0x100];
         *(unsigned*)(dst+x+ 8) = tab1[(s << 3) & 0x100];
         *(unsigned*)(dst+x+12) = tab1[(s << 4) & 0x100];
      } else {
         unsigned s = ptr[line];
         *(unsigned*)(dst+x)   = tab2[(s >> 6) & 3];
         *(unsigned*)(dst+x+4) = tab2[(s >> 4) & 3];
         *(unsigned*)(dst+x+8) = tab2[(s >> 2) & 3];
         *(unsigned*)(dst+x+12)= tab2[(s >> 0) & 3];
      }
      ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+16) = tab1[(s << 5) & 0x100];
         *(unsigned*)(dst+x+20) = tab1[(s << 6) & 0x100];
         *(unsigned*)(dst+x+24) = tab1[(s << 7) & 0x100];
         *(unsigned*)(dst+x+28) = tab1[(s << 8) & 0x100];
      } else {
         unsigned s = ptr[line];
         *(unsigned*)(dst+x+16) = tab2[(s >> 6) & 3];
         *(unsigned*)(dst+x+20) = tab2[(s >> 4) & 3];
         *(unsigned*)(dst+x+24) = tab2[(s >> 2) & 3];
         *(unsigned*)(dst+x+28) = tab2[(s >> 0) & 3];
      }
   }
}

void line_a64_32(unsigned char *dst, unsigned char *src, unsigned char *chars[], unsigned line)
{
   for (unsigned x = 0; x < 2048; x += 64) {
      unsigned char s = *src++;
      unsigned attr = *src++;
      unsigned *tab = t.sctab32[0] + attr;
      unsigned char *ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+ 0) =
         *(unsigned*)(dst+x+ 4) = tab[(s << 1) & 0x100];
         *(unsigned*)(dst+x+ 8) =
         *(unsigned*)(dst+x+12) = tab[(s << 2) & 0x100];
         *(unsigned*)(dst+x+16) =
         *(unsigned*)(dst+x+20) = tab[(s << 3) & 0x100];
         *(unsigned*)(dst+x+24) =
         *(unsigned*)(dst+x+28) = tab[(s << 4) & 0x100];
      } else {
         unsigned s = ptr[line];
         *(unsigned*)(dst+x+ 0) = tab[(s << 1) & 0x100];
         *(unsigned*)(dst+x+ 4) = tab[(s << 2) & 0x100];
         *(unsigned*)(dst+x+ 8) = tab[(s << 3) & 0x100];
         *(unsigned*)(dst+x+12) = tab[(s << 4) & 0x100];
         *(unsigned*)(dst+x+16) = tab[(s << 5) & 0x100];
         *(unsigned*)(dst+x+20) = tab[(s << 6) & 0x100];
         *(unsigned*)(dst+x+24) = tab[(s << 7) & 0x100];
         *(unsigned*)(dst+x+28) = tab[(s << 8) & 0x100];
      }
      ptr = *chars++;
      if (!ptr) {
         *(unsigned*)(dst+x+32) =
         *(unsigned*)(dst+x+36) = tab[(s << 5) & 0x100];
         *(unsigned*)(dst+x+40) =
         *(unsigned*)(dst+x+44) = tab[(s << 6) & 0x100];
         *(unsigned*)(dst+x+48) =
         *(unsigned*)(dst+x+52) = tab[(s << 7) & 0x100];
         *(unsigned*)(dst+x+56) =
         *(unsigned*)(dst+x+60) = tab[(s << 8) & 0x100];
      } else {
         unsigned s = ptr[line];
         *(unsigned*)(dst+x+32) = tab[(s << 1) & 0x100];
         *(unsigned*)(dst+x+36) = tab[(s << 2) & 0x100];
         *(unsigned*)(dst+x+40) = tab[(s << 3) & 0x100];
         *(unsigned*)(dst+x+44) = tab[(s << 4) & 0x100];
         *(unsigned*)(dst+x+48) = tab[(s << 5) & 0x100];
         *(unsigned*)(dst+x+52) = tab[(s << 6) & 0x100];
         *(unsigned*)(dst+x+56) = tab[(s << 7) & 0x100];
         *(unsigned*)(dst+x+60) = tab[(s << 8) & 0x100];
      }
   }
}

static unsigned char *zero64[64] = { 0 };

void rend_anti64_8s(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];

   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_8_1(dst, src, chars, s); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_8_1(dst, src, chars, line); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_8_1(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

void rend_anti64_8d(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];
   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_8_1(dst, src, chars, s*2); dst += pitch;
      line_a64_8_2(dst, src, chars, s*2+1); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_8_1(dst, src, chars, line*2); dst += pitch;
         line_a64_8_2(dst, src, chars, line*2+1); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_8_1(dst, src, zero64, 0); dst += pitch;
      line_a64_8_2(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

void rend_anti64_16s(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];
   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_16_1(dst, src, chars, s); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_16_1(dst, src, chars, line); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_16_1(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

void rend_anti64_16d(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];
   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_16_1(dst, src, chars, s*2); dst += pitch;
      line_a64_16_2(dst, src, chars, s*2+1); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_16_1(dst, src, chars, line*2); dst += pitch;
         line_a64_16_2(dst, src, chars, line*2+1); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_16_1(dst, src, zero64, 0); dst += pitch;
      line_a64_16_2(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

void rend_anti64_32s(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];
   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_32_1(dst, src, chars, s); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_32_1(dst, src, chars, line); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_32_1(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

void rend_anti64_32d(unsigned char *dst, unsigned pitch, unsigned char *src, unsigned scroll = 0)
{
   ATTR_ALIGN(16) unsigned char *chars[64];
   unsigned delta = temp.scx/4;
   if (scroll) for (unsigned y = 0; y < 64; y++) chars[y] = 0;
   else recognize_text(src, chars), scroll = conf.fontsize;
   for (unsigned s = 0; s < scroll; s++) {
      line_a64_32_1(dst, src, chars, s*2); dst += pitch;
      line_a64_32_2(dst, src, chars, s*2+1); dst += pitch;
      src += delta;
   }
   for (unsigned y = conf.fontsize; y < 192; y+=conf.fontsize) {
      recognize_text(src, chars);
      for (unsigned line = 0; line < conf.fontsize; line++) {
         line_a64_32_1(dst, src, chars, line*2); dst += pitch;
         line_a64_32_2(dst, src, chars, line*2+1); dst += pitch;
         src += delta;
      }
   }
   for (unsigned line = scroll; line < conf.fontsize; line++) {
      line_a64_32_1(dst, src, zero64, 0); dst += pitch;
      line_a64_32_2(dst, src, zero64, 0); dst += pitch;
      src += delta;
   }
}

unsigned detect_scroll(unsigned char *src)
{
   unsigned char *chars[64];
   unsigned delta = temp.scx/4, delta2 = delta * conf.fontsize;
   unsigned max = 0, scroll = 0;
   for (unsigned line = 0; line < conf.fontsize; line++)
   {
      unsigned found = 0; unsigned char *src_pos = src; src += delta;
      for (unsigned pos = line; pos < 192; pos += conf.fontsize)
      {
         recognize_text(src_pos, chars);
         for (unsigned i = 0; i < 64; i++)
             found += (unsigned)(chars[i]) >> 16;
         src_pos += delta2;
      }
      if (found > max)
          max = found, scroll = line;
   }
   return scroll;
}

void __fastcall render_text(u8 *dst, u32 pitch)
{
   unsigned char *dst2 = dst + temp.b_left*temp.obpp/4 +
                       temp.b_top*pitch * ((temp.oy > temp.scy)?2:1);
   if (temp.oy > temp.scy && conf.fast_sl) pitch *= 2;
   unsigned char *src = rbuf + (temp.b_top*temp.scx+temp.b_left)/4;
   unsigned scroll = conf.pixelscroll? detect_scroll(src) : 0;

   if (temp.obpp == 8)  { if (conf.fast_sl) rend_frame_8d1(dst, pitch), rend_anti64_8s (dst2, pitch, src, scroll); else rend_frame_8d(dst, pitch), rend_anti64_8d (dst2, pitch, src, scroll); return; }
   if (temp.obpp == 16) { if (conf.fast_sl) rend_frame_16d1(dst, pitch), rend_anti64_16s(dst2, pitch, src, scroll); else rend_frame_16d(dst, pitch), rend_anti64_16d(dst2, pitch, src, scroll); return; }
   if (temp.obpp == 32) { if (conf.fast_sl) rend_frame_32d1(dst, pitch), rend_anti64_32s(dst2, pitch, src, scroll); else rend_frame_32d(dst, pitch), rend_anti64_32d(dst2, pitch, src, scroll); return; }

}

void *alloc_table(void **&tptr, void *startval)
{
   void *res = (void *)tptr;
   for (unsigned i = 0; i < 16; i++)
       tptr[i] = startval;
   tptr += 16;
   return res;
}

//
//    dt[i-1] -> dt[i] -> dt[i+1]
//               |        dt[i+1]
//               |
//               dt[i] -> dt[i+1]
//               |        dt[i+1]
//               |
//               dt[i] -> dt[i+1]
//                        dt[i+1]

// l1[] = { 0, 0, 0 };
// l2[] = { l1, l1, l1 };
// l3[] = { l2, l2, l2 };

void create_font_tables()
{
   const unsigned char *fontbase = conf.fast_sl ? font8 : font16;
   unsigned fonth = conf.fast_sl ? 8 : 16;

   if (conf.fontsize < 8)
   {
       fontbase = conf.fast_sl ? font8 : font14;
       fonth = conf.fast_sl ? 8 : 14;
   }

   void *dummy_tab[8];
   void **ts = (void **)t.font_tables;
   dummy_tab[conf.fontsize-1] = alloc_table(ts, 0);
   for (unsigned i = conf.fontsize-1; i; i--)
      dummy_tab[i-1] = alloc_table(ts, dummy_tab[i]);
   root_tab = dummy_tab[0];

   // todo: collapse tree to graph
   // (last bytes of same char with
   // different first bytes may be same)
   for (unsigned invert = 0; invert < 0x100; invert += 0xFF)
   {
      for (const unsigned char *ptr = fontdata; *ptr; ptr += 8)
      {
         unsigned code = *ptr++;
         void **tab = (void **)root_tab;

         for (unsigned level = 0; ; level++)
         {
            unsigned bits = (ptr[level] ^ (unsigned char)invert) & 0x0F;
            if (level == conf.fontsize-1)
            {
               if (tab[bits])
               {
#if 0 // 1 - debug
                  color(CONSCLR_ERROR);
                  printf("duplicate char %d (%02X)! - font table may corrupt\n", (ptr-1-fontdata)/9, code);
                  exit();
#endif
               }
               else
               {
                  unsigned *newchar;
                  unsigned *basechar = (unsigned*)(fontbase + fonth*code); // Шрифт 8xh, h=8,14,16
                  if (invert)
                  {
                     newchar = (unsigned *)ts;
                     ts += 4;
                     newchar[0] = ~basechar[0];
                     newchar[1] = ~basechar[1];
                     newchar[2] = ~basechar[2];
                     newchar[3] = ~basechar[3];
                  }
                  else
                     newchar = basechar;

                  tab[bits] = newchar;
               }
               break;
            }
            void *next = tab[bits];
            if (next == dummy_tab[level+1])
               tab[bits] = next = alloc_table(ts, (level==conf.fontsize-2)? 0 : dummy_tab[level+2]);
            tab = (void **)next;
         }
      }
   }

   size_t usedsize = ((u8 *)ts) - ((u8 *)t.font_tables);
   if (usedsize > sizeof(t.font_tables))
   {
      color(CONSCLR_ERROR);
      printf("font table overflow: size=%u (0x%X) of 0x%X\n", usedsize, usedsize, sizeof t.font_tables);
      exit();
   }
}
