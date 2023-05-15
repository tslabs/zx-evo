#include "std.h"
#include "dxrcopy.h"

// #define QUAD_BUFFER  // tests show that this variant is slower, even in noflic mode
#if 0
void line32_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*4; x += 32) {
      u8 byte = *src;
      unsigned *t1 = tab + src[1];
      u8 byt1 = src[rb2_offs];
      unsigned *t2 = tab + src[rb2_offs+1];
      src += 2;

      *(unsigned*)(dst+x)    = t1[(byte << 1) & 0x100] +
                               t2[(byt1 << 1) & 0x100];
      *(unsigned*)(dst+x+4)  = t1[(byte << 2) & 0x100] +
                               t2[(byt1 << 2) & 0x100];
      *(unsigned*)(dst+x+8)  = t1[(byte << 3) & 0x100] +
                               t2[(byt1 << 3) & 0x100];
      *(unsigned*)(dst+x+12) = t1[(byte << 4) & 0x100] +
                               t2[(byt1 << 4) & 0x100];
      *(unsigned*)(dst+x+16) = t1[(byte << 5) & 0x100] +
                               t2[(byt1 << 5) & 0x100];
      *(unsigned*)(dst+x+20) = t1[(byte << 6) & 0x100] +
                               t2[(byt1 << 6) & 0x100];
      *(unsigned*)(dst+x+24) = t1[(byte << 7) & 0x100] +
                               t2[(byt1 << 7) & 0x100];
      *(unsigned*)(dst+x+28) = t1[(byte << 8) & 0x100] +
                               t2[(byt1 << 8) & 0x100];
   }
}

void line32d_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*8; x += 64) {
      u8 byte = *src;
      unsigned *t1 = tab + src[1];
      u8 byt1 = src[rb2_offs];
      unsigned *t2 = tab + src[rb2_offs+1];
      src += 2;

      *(unsigned*)(dst+x)    =
      *(unsigned*)(dst+x+4)  =
                               t1[(byte << 1) & 0x100] +
                               t2[(byt1 << 1) & 0x100];
      *(unsigned*)(dst+x+8)  =
      *(unsigned*)(dst+x+12) =
                               t1[(byte << 2) & 0x100] +
                               t2[(byt1 << 2) & 0x100];
      *(unsigned*)(dst+x+16)  =
      *(unsigned*)(dst+x+20)  =
                               t1[(byte << 3) & 0x100] +
                               t2[(byt1 << 3) & 0x100];
      *(unsigned*)(dst+x+24) =
      *(unsigned*)(dst+x+28) =
                               t1[(byte << 4) & 0x100] +
                               t2[(byt1 << 4) & 0x100];
      *(unsigned*)(dst+x+32) =
      *(unsigned*)(dst+x+36) =
                               t1[(byte << 5) & 0x100] +
                               t2[(byt1 << 5) & 0x100];
      *(unsigned*)(dst+x+40) =
      *(unsigned*)(dst+x+44) =
                               t1[(byte << 6) & 0x100] +
                               t2[(byt1 << 6) & 0x100];
      *(unsigned*)(dst+x+48) =
      *(unsigned*)(dst+x+52) =
                               t1[(byte << 7) & 0x100] +
                               t2[(byt1 << 7) & 0x100];
      *(unsigned*)(dst+x+56) =
      *(unsigned*)(dst+x+60) =
                               t1[(byte << 8) & 0x100] +
                               t2[(byt1 << 8) & 0x100];
   }
}

void line32t_nf(u8 *dst, u8 *src, unsigned *tab)
{
   u32 *d = (u32 *)dst;
   for (unsigned x = 0,  i = 0; x < temp.scx*3; x += 24, i += 2)
   {
      u8 byte1 = src[i+0];
      unsigned *t1 = tab + src[i+1];
      u8 byte2 = src[i+rb2_offs];
      unsigned *t2 = tab + src[i+rb2_offs+1];

      u32 paper1 = t1[0];
      u32 ink1 = t1[0x100];

      u32 paper2 = t2[0];
      u32 ink2 = t2[0x100];

      d[x+0]  = 
      d[x+1]  = 
      d[x+2]  = ((byte1 & 0x80) ? ink1 : paper1) + ((byte2 & 0x80) ? ink2 : paper2);

      d[x+3]  = 
      d[x+4]  = 
      d[x+5]  = ((byte1 & 0x40) ? ink1 : paper1) + ((byte2 & 0x40) ? ink2 : paper2);

      d[x+6]  = 
      d[x+7]  = 
      d[x+8]  = ((byte1 & 0x20) ? ink1 : paper1) + ((byte2 & 0x20) ? ink2 : paper2);

      d[x+9]  = 
      d[x+10] = 
      d[x+11] = ((byte1 & 0x10) ? ink1 : paper1) + ((byte2 & 0x10) ? ink2 : paper2);

      d[x+12] = 
      d[x+13] = 
      d[x+14] = ((byte1 & 0x08) ? ink1 : paper1) + ((byte2 & 0x08) ? ink2 : paper2);

      d[x+15] = 
      d[x+16] = 
      d[x+17] = ((byte1 & 0x04) ? ink1 : paper1) + ((byte2 & 0x04) ? ink2 : paper2);

      d[x+18] = 
      d[x+19] = 
      d[x+20] = ((byte1 & 0x02) ? ink1 : paper1) + ((byte2 & 0x02) ? ink2 : paper2);

      d[x+21] = 
      d[x+22] = 
      d[x+23] = ((byte1 & 0x01) ? ink1 : paper1) + ((byte2 & 0x01) ? ink2 : paper2);
   }
}

void line32q_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*16; x += 128) {
      u8 byte = *src;
      unsigned *t1 = tab + src[1];
      u8 byt1 = src[rb2_offs];
      unsigned *t2 = tab + src[rb2_offs+1];
      src += 2;

      *(unsigned*)(dst+x+0x00) =
      *(unsigned*)(dst+x+0x04) =
      *(unsigned*)(dst+x+0x08) =
      *(unsigned*)(dst+x+0x0C) =
                               t1[(byte << 1) & 0x100] +
                               t2[(byt1 << 1) & 0x100];
      *(unsigned*)(dst+x+0x10) =
      *(unsigned*)(dst+x+0x14) =
      *(unsigned*)(dst+x+0x18) =
      *(unsigned*)(dst+x+0x1C) =
                               t1[(byte << 2) & 0x100] +
                               t2[(byt1 << 2) & 0x100];
      *(unsigned*)(dst+x+0x20) =
      *(unsigned*)(dst+x+0x24) =
      *(unsigned*)(dst+x+0x28) =
      *(unsigned*)(dst+x+0x2C) =
                               t1[(byte << 3) & 0x100] +
                               t2[(byt1 << 3) & 0x100];
      *(unsigned*)(dst+x+0x30) =
      *(unsigned*)(dst+x+0x34) =
      *(unsigned*)(dst+x+0x38) =
      *(unsigned*)(dst+x+0x3C) =
                               t1[(byte << 4) & 0x100] +
                               t2[(byt1 << 4) & 0x100];
      *(unsigned*)(dst+x+0x40) =
      *(unsigned*)(dst+x+0x44) =
      *(unsigned*)(dst+x+0x48) =
      *(unsigned*)(dst+x+0x4C) =
                               t1[(byte << 5) & 0x100] +
                               t2[(byt1 << 5) & 0x100];
      *(unsigned*)(dst+x+0x50) =
      *(unsigned*)(dst+x+0x54) =
      *(unsigned*)(dst+x+0x58) =
      *(unsigned*)(dst+x+0x5C) =
                               t1[(byte << 6) & 0x100] +
                               t2[(byt1 << 6) & 0x100];
      *(unsigned*)(dst+x+0x60) =
      *(unsigned*)(dst+x+0x64) =
      *(unsigned*)(dst+x+0x68) =
      *(unsigned*)(dst+x+0x6C) =
                               t1[(byte << 7) & 0x100] +
                               t2[(byt1 << 7) & 0x100];
      *(unsigned*)(dst+x+0x70) =
      *(unsigned*)(dst+x+0x74) =
      *(unsigned*)(dst+x+0x78) =
      *(unsigned*)(dst+x+0x7C) =
                               t1[(byte << 8) & 0x100] +
                               t2[(byt1 << 8) & 0x100];
   }
}

#ifdef MOD_SSE2
void line32(u8 *dst, u8 *src, unsigned *tab)
{
   __m128i *d = (__m128i *)dst;
   __m128i m1, m2;
   m1 = _mm_set_epi32(0x10, 0x20, 0x40, 0x80);
   m2 = _mm_set_epi32(0x1, 0x2, 0x4, 0x8);

   for (unsigned x = 0,  i = 0; x < temp.scx / 4; x += 2,  i += 2)
   {
      unsigned byte = src[i];
      unsigned attr = src[i+1];
      unsigned ink = tab[attr + 0x100];
      unsigned paper = tab[attr];

      __m128i b, b1, b2;
      __m128i r1, r2;
      __m128i iv, pv;
      __m128i im1, pm1, im2, pm2;
      __m128i vr1, vr2;

      b = _mm_set1_epi32(byte);
      iv = _mm_set1_epi32(ink);
      pv = _mm_set1_epi32(paper);

      b1 = _mm_and_si128(b, m1);
      r1 = _mm_cmpeq_epi32(b1, m1);
      im1 = _mm_and_si128(r1, iv);
      pm1 = _mm_andnot_si128(r1, pv);
      vr1 = _mm_or_si128(im1, pm1);
      _mm_store_si128(&d[x], vr1);

      b2 = _mm_and_si128(b, m2);
      r2 = _mm_cmpeq_epi32(b2, m2);
      im2 = _mm_and_si128(r2, iv);
      pm2 = _mm_andnot_si128(r2, pv);
      vr2 = _mm_or_si128(im2, pm2);
      _mm_store_si128(&d[x+1], vr2);
   }
}
#else
void line32(u8 *dst, u8 *src, unsigned *tab)
{
   unsigned *d = (unsigned *)dst;
   for (unsigned x = 0,  i = 0; x < temp.scx; x += 8,  i += 2)
   {
      unsigned byte = src[i];
      unsigned attr = src[i+1];
      unsigned ink = tab[attr + 0x100];
      unsigned paper = tab[attr];

      d[x]   = (byte & 0x80) ? ink : paper; // 7
      d[x+1] = (byte & 0x40) ? ink : paper; // 6
      d[x+2] = (byte & 0x20) ? ink : paper; // 5
      d[x+3] = (byte & 0x10) ? ink : paper; // 4

      d[x+4] = (byte & 0x08) ? ink : paper; // 3
      d[x+5] = (byte & 0x04) ? ink : paper; // 2
      d[x+6] = (byte & 0x02) ? ink : paper; // 1
      d[x+7] = (byte & 0x01) ? ink : paper; // 0
   }
}
#endif

#ifdef MOD_SSE2
void line32d(u8 *dst, u8 *src, unsigned *tab)
{
   __m128i *d = (__m128i *)dst;
   __m128i m1, m2;
   m1 = _mm_set_epi32(0x10, 0x20, 0x40, 0x80);
   m2 = _mm_set_epi32(0x1, 0x2, 0x4, 0x8);

   for (unsigned x = 0,  i = 0; x < temp.scx / 2; x += 4,  i += 2)
   {
      unsigned byte = src[i];
      unsigned attr = src[i+1];
      unsigned ink = tab[attr + 0x100];
      unsigned paper = tab[attr];

      __m128i b, b1, b2;
      __m128i r1, r2;
      __m128i iv, pv;
      __m128i im1, pm1, im2, pm2;
      __m128i vr1, vr2;
      __m128i l1, l2;
      __m128i h1, h2;

      b = _mm_set1_epi32(byte);
      iv = _mm_set1_epi32(ink);
      pv = _mm_set1_epi32(paper);

      b1 = _mm_and_si128(b, m1);
      r1 = _mm_cmpeq_epi32(b1, m1);
      im1 = _mm_and_si128(r1, iv);
      pm1 = _mm_andnot_si128(r1, pv);
      vr1 = _mm_or_si128(im1, pm1);

      l1 = _mm_unpacklo_epi32(vr1, vr1);
      _mm_store_si128(&d[x], l1);
      h1 = _mm_unpackhi_epi32(vr1, vr1);
      _mm_store_si128(&d[x+1], h1);

      b2 = _mm_and_si128(b, m2);
      r2 = _mm_cmpeq_epi32(b2, m2);
      im2 = _mm_and_si128(r2, iv);
      pm2 = _mm_andnot_si128(r2, pv);
      vr2 = _mm_or_si128(im2, pm2);

      l2 = _mm_unpacklo_epi32(vr2, vr2);
      _mm_store_si128(&d[x+2], l2);
      h2 = _mm_unpackhi_epi32(vr2, vr2);
      _mm_store_si128(&d[x+3], h2);
   }
}
#else
void line32d(u8 *dst, u8 *src, unsigned *tab)
{
   unsigned *d = (unsigned *)dst;
   for (unsigned x = 0, i = 0; x < temp.scx * 2; x += 16, i+= 2)
   {
      // [vv] Такой порядок записи позволяет icl генерировать cmovcc вместо jcc
      u8 byte = src[i];
      u8 attr = src[i+1];
      unsigned ink = tab[attr + 0x100];
      unsigned paper = tab[attr];

      d[x]    = d[x+1]  = (byte & 0x80) ? ink : paper; // 7
      d[x+2]  = d[x+3]  = (byte & 0x40) ? ink : paper; // 6
      d[x+4]  = d[x+5]  = (byte & 0x20) ? ink : paper; // 5
      d[x+6]  = d[x+7]  = (byte & 0x10) ? ink : paper; // 4
      d[x+8]  = d[x+9]  = (byte & 0x08) ? ink : paper; // 3
      d[x+10] = d[x+11] = (byte & 0x04) ? ink : paper; // 2
      d[x+12] = d[x+13] = (byte & 0x02) ? ink : paper; // 1
      d[x+14] = d[x+15] = (byte & 0x01) ? ink : paper; // 0
   }
}
#endif

void line32t(u8 *dst, const u8 *src, const unsigned *tab)
{
   unsigned *d = (unsigned *)dst;
   for (unsigned x = 0, i = 0; x < temp.scx * 3; x += 3*8,  i += 2)
   {
      u8 byte = src[i];
      unsigned attr = src[i + 1];
      unsigned ink = tab[attr + 0x100];
      unsigned paper = tab[attr];

      d[x]      = d[x + 1]  = d[x + 2]  = (byte & 0x80) ? ink : paper;
      d[x + 3]  = d[x + 4]  = d[x + 5]  = (byte & 0x40) ? ink : paper;
      d[x + 6]  = d[x + 7]  = d[x + 8]  = (byte & 0x20) ? ink : paper;
      d[x + 9]  = d[x + 10] = d[x + 11] = (byte & 0x10) ? ink : paper;
      d[x + 12] = d[x + 13] = d[x + 14] = (byte & 0x08) ? ink : paper;
      d[x + 15] = d[x + 16] = d[x + 17] = (byte & 0x04) ? ink : paper;
      d[x + 18] = d[x + 19] = d[x + 20] = (byte & 0x02) ? ink : paper;
      d[x + 21] = d[x + 22] = d[x + 23] = (byte & 0x01) ? ink : paper;
   }
}

void line32q(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*16; x += 128) {
      u8 byte = *src++;
      unsigned *t = tab + *src++;
      *(unsigned*)(dst+x+0x00) =
      *(unsigned*)(dst+x+0x04) =
      *(unsigned*)(dst+x+0x08) =
      *(unsigned*)(dst+x+0x0C) =
                               t[(byte << 1) & 0x100];
      *(unsigned*)(dst+x+0x10) =
      *(unsigned*)(dst+x+0x14) =
      *(unsigned*)(dst+x+0x18) =
      *(unsigned*)(dst+x+0x1C) =
                               t[(byte << 2) & 0x100];
      *(unsigned*)(dst+x+0x20) =
      *(unsigned*)(dst+x+0x24) =
      *(unsigned*)(dst+x+0x28) =
      *(unsigned*)(dst+x+0x2C) =
                               t[(byte << 3) & 0x100];
      *(unsigned*)(dst+x+0x30) =
      *(unsigned*)(dst+x+0x34) =
      *(unsigned*)(dst+x+0x38) =
      *(unsigned*)(dst+x+0x3C) =
                               t[(byte << 4) & 0x100];
      *(unsigned*)(dst+x+0x40) =
      *(unsigned*)(dst+x+0x44) =
      *(unsigned*)(dst+x+0x48) =
      *(unsigned*)(dst+x+0x4C) =
                               t[(byte << 5) & 0x100];
      *(unsigned*)(dst+x+0x50) =
      *(unsigned*)(dst+x+0x54) =
      *(unsigned*)(dst+x+0x58) =
      *(unsigned*)(dst+x+0x5C) =
                               t[(byte << 6) & 0x100];
      *(unsigned*)(dst+x+0x60) =
      *(unsigned*)(dst+x+0x64) =
      *(unsigned*)(dst+x+0x68) =
      *(unsigned*)(dst+x+0x6C) =
                               t[(byte << 7) & 0x100];
      *(unsigned*)(dst+x+0x70) =
      *(unsigned*)(dst+x+0x74) =
      *(unsigned*)(dst+x+0x78) =
      *(unsigned*)(dst+x+0x7C) =
                               t[(byte << 8) & 0x100];
   }
}

void line16_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*2; x += 32) {
      unsigned s = *(unsigned*)src, attr = (s >> 6) & 0x3FC;
      unsigned r = *(unsigned*)(src + rb2_offs), atr2 = (r >> 6) & 0x3FC;
      *(unsigned*)(dst+x)   = (tab[((s >> 6) & 3) + attr]) +
                              (tab[((r >> 6) & 3) + atr2]);
      *(unsigned*)(dst+x+4) = (tab[((s >> 4) & 3) + attr]) +
                              (tab[((r >> 4) & 3) + atr2]);
      *(unsigned*)(dst+x+8) = (tab[((s >> 2) & 3) + attr]) +
                              (tab[((r >> 2) & 3) + atr2]);
      *(unsigned*)(dst+x+12)= (tab[((s >> 0) & 3) + attr]) +
                              (tab[((r >> 0) & 3) + atr2]);
      attr = (s >> 22) & 0x3FC; atr2 = (r >> 22) & 0x3FC;
      *(unsigned*)(dst+x+16)= (tab[((s >>22) & 3) + attr]) +
                              (tab[((r >>22) & 3) + atr2]);
      *(unsigned*)(dst+x+20)= (tab[((s >>20) & 3) + attr]) +
                              (tab[((r >>20) & 3) + atr2]);
      *(unsigned*)(dst+x+24)= (tab[((s >>18) & 3) + attr]) +
                              (tab[((r >>18) & 3) + atr2]);
      *(unsigned*)(dst+x+28)= (tab[((s >>16) & 3) + attr]) +
                              (tab[((r >>16) & 3) + atr2]);
      src += 4;
   }
}

#define line16d_nf line32_nf

#define line16q line32d
#define line16q_nf line32d_nf

void line16t(u8 *dst, u8 *src, unsigned *tab)
{
   u16 *d = (u16 *)dst;
   for (unsigned x = 0; x < temp.scx*3; x += 24)
   {
      u8 byte = *src++;
      unsigned *t = tab + *src++;
      u16 paper_yu = t[0];
      u16 paper_yv = t[0] >> 16;
      u16 ink_yu = t[0x100];
      u16 ink_yv = t[0x100] >> 16;

      d[x+0]  = (byte & 0x80) ? ink_yu : paper_yu;
      d[x+1]  = (byte & 0x80) ? ink_yv : paper_yv;
      d[x+2]  = (byte & 0x80) ? ink_yu : paper_yu;

      d[x+3]  = (byte & 0x40) ? ink_yv : paper_yv;
      d[x+4]  = (byte & 0x40) ? ink_yu : paper_yu;
      d[x+5]  = (byte & 0x40) ? ink_yv : paper_yv;

      d[x+6]  = (byte & 0x20) ? ink_yu : paper_yu;
      d[x+7]  = (byte & 0x20) ? ink_yv : paper_yv;
      d[x+8]  = (byte & 0x20) ? ink_yu : paper_yu;

      d[x+9]  = (byte & 0x10) ? ink_yv : paper_yv;
      d[x+10] = (byte & 0x10) ? ink_yu : paper_yu;
      d[x+11] = (byte & 0x10) ? ink_yv : paper_yv;

      d[x+12] = (byte & 0x08) ? ink_yu : paper_yu;
      d[x+13] = (byte & 0x08) ? ink_yv : paper_yv;
      d[x+14] = (byte & 0x08) ? ink_yu : paper_yu;

      d[x+15] = (byte & 0x04) ? ink_yv : paper_yv;
      d[x+16] = (byte & 0x04) ? ink_yu : paper_yu;
      d[x+17] = (byte & 0x04) ? ink_yv : paper_yv;

      d[x+18] = (byte & 0x02) ? ink_yu : paper_yu;
      d[x+19] = (byte & 0x02) ? ink_yv : paper_yv;
      d[x+20] = (byte & 0x02) ? ink_yu : paper_yu;

      d[x+21] = (byte & 0x01) ? ink_yv : paper_yv;
      d[x+22] = (byte & 0x01) ? ink_yu : paper_yu;
      d[x+23] = (byte & 0x01) ? ink_yv : paper_yv;
   }
}

void line16t_nf(u8 *dst, u8 *src, unsigned *tab)
{
   u16 *d = (u16 *)dst;
   for (unsigned x = 0,  i = 0; x < temp.scx*3; x += 24, i += 2)
   {
      u8 byte1 = src[i+0];
      unsigned *t1 = tab + src[i+1];
      u8 byte2 = src[i+rb2_offs];
      unsigned *t2 = tab + src[i+rb2_offs+1];

      u16 paper_yu1 = t1[0];
      u16 paper_yv1 = t1[0] >> 16;
      u16 ink_yu1 = t1[0x100];
      u16 ink_yv1 = t1[0x100] >> 16;

      u16 paper_yu2 = t2[0];
      u16 paper_yv2 = t2[0] >> 16;
      u16 ink_yu2 = t2[0x100];
      u16 ink_yv2 = t2[0x100] >> 16;

      d[x+0]  = ((byte1 & 0x80) ? ink_yu1 : paper_yu1) + ((byte2 & 0x80) ? ink_yu2 : paper_yu2);
      d[x+1]  = ((byte1 & 0x80) ? ink_yv1 : paper_yv1) + ((byte2 & 0x80) ? ink_yv2 : paper_yv2);
      d[x+2]  = ((byte1 & 0x80) ? ink_yu1 : paper_yu1) + ((byte2 & 0x80) ? ink_yu2 : paper_yu2);

      d[x+3]  = ((byte1 & 0x40) ? ink_yv1 : paper_yv1) + ((byte2 & 0x40) ? ink_yv2 : paper_yv2);
      d[x+4]  = ((byte1 & 0x40) ? ink_yu1 : paper_yu1) + ((byte2 & 0x40) ? ink_yu2 : paper_yu2);
      d[x+5]  = ((byte1 & 0x40) ? ink_yv1 : paper_yv1) + ((byte2 & 0x40) ? ink_yv2 : paper_yv2);

      d[x+6]  = ((byte1 & 0x20) ? ink_yu1 : paper_yu1) + ((byte2 & 0x20) ? ink_yu2 : paper_yu2);
      d[x+7]  = ((byte1 & 0x20) ? ink_yv1 : paper_yv1) + ((byte2 & 0x20) ? ink_yv2 : paper_yv2);
      d[x+8]  = ((byte1 & 0x20) ? ink_yu1 : paper_yu1) + ((byte2 & 0x20) ? ink_yu2 : paper_yu2);

      d[x+9]  = ((byte1 & 0x10) ? ink_yv1 : paper_yv1) + ((byte2 & 0x10) ? ink_yv2 : paper_yv2);
      d[x+10] = ((byte1 & 0x10) ? ink_yu1 : paper_yu1) + ((byte2 & 0x10) ? ink_yu2 : paper_yu2);
      d[x+11] = ((byte1 & 0x10) ? ink_yv1 : paper_yv1) + ((byte2 & 0x10) ? ink_yv2 : paper_yv2);

      d[x+12] = ((byte1 & 0x08) ? ink_yu1 : paper_yu1) + ((byte2 & 0x08) ? ink_yu2 : paper_yu2);
      d[x+13] = ((byte1 & 0x08) ? ink_yv1 : paper_yv1) + ((byte2 & 0x08) ? ink_yv2 : paper_yv2);
      d[x+14] = ((byte1 & 0x08) ? ink_yu1 : paper_yu1) + ((byte2 & 0x08) ? ink_yu2 : paper_yu2);

      d[x+15] = ((byte1 & 0x04) ? ink_yv1 : paper_yv1) + ((byte2 & 0x04) ? ink_yv2 : paper_yv2);
      d[x+16] = ((byte1 & 0x04) ? ink_yu1 : paper_yu1) + ((byte2 & 0x04) ? ink_yu2 : paper_yu2);
      d[x+17] = ((byte1 & 0x04) ? ink_yv1 : paper_yv1) + ((byte2 & 0x04) ? ink_yv2 : paper_yv2);

      d[x+18] = ((byte1 & 0x02) ? ink_yu1 : paper_yu1) + ((byte2 & 0x02) ? ink_yu2 : paper_yu2);
      d[x+19] = ((byte1 & 0x02) ? ink_yv1 : paper_yv1) + ((byte2 & 0x02) ? ink_yv2 : paper_yv2);
      d[x+20] = ((byte1 & 0x02) ? ink_yu1 : paper_yu1) + ((byte2 & 0x02) ? ink_yu2 : paper_yu2);

      d[x+21] = ((byte1 & 0x01) ? ink_yv1 : paper_yv1) + ((byte2 & 0x01) ? ink_yv2 : paper_yv2);
      d[x+22] = ((byte1 & 0x01) ? ink_yu1 : paper_yu1) + ((byte2 & 0x01) ? ink_yu2 : paper_yu2);
      d[x+23] = ((byte1 & 0x01) ? ink_yv1 : paper_yv1) + ((byte2 & 0x01) ? ink_yv2 : paper_yv2);
   }
}

void line8(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx; x += 32) {
      unsigned src0 = *(unsigned*)src, attr = (src0 >> 4) & 0xFF0;
      *(unsigned*)(dst+x)    = tab[((src0 >> 4)  & 0xF) + attr];
      *(unsigned*)(dst+x+4)  = tab[((src0 >> 0)  & 0xF) + attr];
      attr = (src0 >> 20) & 0xFF0;
      *(unsigned*)(dst+x+8)  = tab[((src0 >> 20) & 0xF) + attr];
      *(unsigned*)(dst+x+12) = tab[((src0 >> 16) & 0xF) + attr];
      src0 = *(unsigned*)(src+4), attr = (src0 >> 4) & 0xFF0;
      *(unsigned*)(dst+x+16) = tab[((src0 >> 4)  & 0xF) + attr];
      *(unsigned*)(dst+x+20) = tab[((src0 >> 0)  & 0xF) + attr];
      attr = (src0 >> 20) & 0xFF0;
      *(unsigned*)(dst+x+24) = tab[((src0 >> 20) & 0xF) + attr];
      *(unsigned*)(dst+x+28) = tab[((src0 >> 16) & 0xF) + attr];
      src += 8;
   }
}

void line8_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx; x += 32) {
      unsigned s = *(unsigned*)src, attr = (s >> 4) & 0xFF0;
      unsigned r = *(unsigned*)(src + rb2_offs), atr2 = (r >> 4) & 0xFF0;
      *(unsigned*)(dst+x)    = (tab[((s >> 4)  & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 4)  & 0xF) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+4)  = (tab[((s >> 0)  & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 0)  & 0xF) + atr2] & 0xF0F0F0F0);
      attr = (s >> 20) & 0xFF0; atr2 = (r >> 20) & 0xFF0;
      *(unsigned*)(dst+x+8)  = (tab[((s >> 20) & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 20) & 0xF) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+12) = (tab[((s >> 16) & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 16) & 0xF) + atr2] & 0xF0F0F0F0);
      s = *(unsigned*)(src+4), attr = (s >> 4) & 0xFF0;
      r = *(unsigned*)(src+rb2_offs+4), atr2 = (r >> 4) & 0xFF0;
      *(unsigned*)(dst+x+16) = (tab[((s >> 4)  & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 4)  & 0xF) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+20) = (tab[((s >> 0)  & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 0)  & 0xF) + atr2] & 0xF0F0F0F0);
      attr = (s >> 20) & 0xFF0; atr2 = (r >> 20) & 0xFF0;
      *(unsigned*)(dst+x+24) = (tab[((s >> 20) & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 20) & 0xF) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+28) = (tab[((s >> 16) & 0xF) + attr] & 0x0F0F0F0F) +
                               (tab[((r >> 16) & 0xF) + atr2] & 0xF0F0F0F0);
      src += 8;
   }
}

void line8d(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*2; x += 32) {
      unsigned s = *(unsigned*)src, attr = (s >> 6) & 0x3FC;
      *(unsigned*)(dst+x)   = tab[((s >> 6) & 3) + attr];
      *(unsigned*)(dst+x+4) = tab[((s >> 4) & 3) + attr];
      *(unsigned*)(dst+x+8) = tab[((s >> 2) & 3) + attr];
      *(unsigned*)(dst+x+12)= tab[((s >> 0) & 3) + attr];
      attr = (s >> 22) & 0x3FC;
      *(unsigned*)(dst+x+16)= tab[((s >>22) & 3) + attr];
      *(unsigned*)(dst+x+20)= tab[((s >>20) & 3) + attr];
      *(unsigned*)(dst+x+24)= tab[((s >>18) & 3) + attr];
      *(unsigned*)(dst+x+28)= tab[((s >>16) & 3) + attr];
      src += 4;
   }
}


void line8t(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*3; x += 24)
   {
      u8 byte = *src++;
      unsigned *t = tab + *src++;
      dst[x+0]  = dst[x+1]  = dst[x+2]  = t[(byte << 1) & 0x100];
      dst[x+3]  = dst[x+4]  = dst[x+5]  = t[(byte << 2) & 0x100];
      dst[x+6]  = dst[x+7]  = dst[x+8]  = t[(byte << 3) & 0x100];
      dst[x+9]  = dst[x+10] = dst[x+11] = t[(byte << 4) & 0x100];
      dst[x+12] = dst[x+13] = dst[x+14] = t[(byte << 5) & 0x100];
      dst[x+15] = dst[x+16] = dst[x+17] = t[(byte << 6) & 0x100];
      dst[x+18] = dst[x+19] = dst[x+20] = t[(byte << 7) & 0x100];
      dst[x+21] = dst[x+22] = dst[x+23] = t[(byte << 8) & 0x100];
   }
}

void line8q(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*4; x += 32) {
      u8 byte = *src++;
      unsigned *t = tab + *src++;
      *(unsigned*)(dst+x+0x00) = t[(byte << 1) & 0x100];
      *(unsigned*)(dst+x+0x04) = t[(byte << 2) & 0x100];
      *(unsigned*)(dst+x+0x08) = t[(byte << 3) & 0x100];
      *(unsigned*)(dst+x+0x0C) = t[(byte << 4) & 0x100];
      *(unsigned*)(dst+x+0x10) = t[(byte << 5) & 0x100];
      *(unsigned*)(dst+x+0x14) = t[(byte << 6) & 0x100];
      *(unsigned*)(dst+x+0x18) = t[(byte << 7) & 0x100];
      *(unsigned*)(dst+x+0x1C) = t[(byte << 8) & 0x100];
   }
}

void line8d_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*2; x += 32) {
      unsigned s = *(unsigned*)src, attr = (s >> 6) & 0x3FC;
      unsigned r = *(unsigned*)(src + rb2_offs), atr2 = (r >> 6) & 0x3FC;
      *(unsigned*)(dst+x)   = (tab[((s >> 6) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >> 6) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+4) = (tab[((s >> 4) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >> 4) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+8) = (tab[((s >> 2) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >> 2) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+12)= (tab[((s >> 0) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >> 0) & 3) + atr2] & 0xF0F0F0F0);
      attr = (s >> 22) & 0x3FC; atr2 = (r >> 22) & 0x3FC;
      *(unsigned*)(dst+x+16)= (tab[((s >>22) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >>22) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+20)= (tab[((s >>20) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >>20) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+24)= (tab[((s >>18) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >>18) & 3) + atr2] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+28)= (tab[((s >>16) & 3) + attr] & 0x0F0F0F0F) +
                              (tab[((r >>16) & 3) + atr2] & 0xF0F0F0F0);
      src += 4;
   }
}

void line8t_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0, i = 0; x < temp.scx*3; x += 24, i += 2)
   {
      u32 byte1 = src[i+0];
      u32 byte2 = src[i+rb2_offs+0];
      unsigned *t1 = tab + src[i+1];
      unsigned *t2 = tab + src[i+rb2_offs+1];
      u8 ink1 = u8(t1[0x100] & 0x0F);
      u8 ink2 = u8(t2[0x100] & 0xF0);
      u8 paper1 = u8(t1[0] & 0x0F);
      u8 paper2 = u8(t2[0] & 0xF0);

      dst[x+0]  = dst[x+1]  = dst[x+2]  = ((byte1 & 0x80) ? ink1 : paper1) + ((byte2 & 0x80) ? ink2 : paper2);
      dst[x+3]  = dst[x+4]  = dst[x+5]  = ((byte1 & 0x40) ? ink1 : paper1) + ((byte2 & 0x40) ? ink2 : paper2);
      dst[x+6]  = dst[x+7]  = dst[x+8]  = ((byte1 & 0x20) ? ink1 : paper1) + ((byte2 & 0x20) ? ink2 : paper2);
      dst[x+9]  = dst[x+10] = dst[x+11] = ((byte1 & 0x10) ? ink1 : paper1) + ((byte2 & 0x10) ? ink2 : paper2);
      dst[x+12] = dst[x+13] = dst[x+14] = ((byte1 & 0x08) ? ink1 : paper1) + ((byte2 & 0x08) ? ink2 : paper2);
      dst[x+15] = dst[x+16] = dst[x+17] = ((byte1 & 0x04) ? ink1 : paper1) + ((byte2 & 0x04) ? ink2 : paper2);
      dst[x+18] = dst[x+19] = dst[x+20] = ((byte1 & 0x02) ? ink1 : paper1) + ((byte2 & 0x02) ? ink2 : paper2);
      dst[x+21] = dst[x+22] = dst[x+23] = ((byte1 & 0x01) ? ink1 : paper1) + ((byte2 & 0x01) ? ink2 : paper2);
   }
}

void line8q_nf(u8 *dst, u8 *src, unsigned *tab)
{
   for (unsigned x = 0; x < temp.scx*4; x += 32) {
      u8 byte1 = src[0], byte2 = src[rb2_offs+0];
      unsigned *t1 = tab + src[1], *t2 = tab + src[rb2_offs+1];
      src += 2;

      *(unsigned*)(dst+x+0x00) = (t1[(byte1 << 1) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 1) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x04) = (t1[(byte1 << 2) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 2) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x08) = (t1[(byte1 << 3) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 3) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x0C) = (t1[(byte1 << 4) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 4) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x10) = (t1[(byte1 << 5) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 5) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x14) = (t1[(byte1 << 6) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 6) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x18) = (t1[(byte1 << 7) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 7) & 0x100] & 0xF0F0F0F0);
      *(unsigned*)(dst+x+0x1C) = (t1[(byte1 << 8) & 0x100] & 0x0F0F0F0F) + (t2[(byte2 << 8) & 0x100] & 0xF0F0F0F0);
   }
}


void rend_copy32_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line32_nf(dst, src, t.sctab32_nf[0]);
      dst += pitch; src += delta;
   }
}

void rend_copy32(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf;
   unsigned delta = temp.scx / 4;
   for (unsigned y = 0; y < temp.scy; y++)
   {
      line32(dst, src, t.sctab32[0]);
      dst += pitch;
      src += delta;
   }
}

void rend_copy32d1_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line32d_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy32d_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   if (conf.alt_nf) {
      int offset = rb2_offs;
      if (comp.frame_counter & 1) src += rb2_offs, offset = -offset;
      for (unsigned y = 0; y < temp.scy; y++) {
         line32d(dst, src, t.sctab32[0]); dst += pitch;
         line32d(dst, src+offset, t.sctab32[0]); dst += pitch;
         src += delta;
      }
   } else {
      for (unsigned y = 0; y < temp.scy; y++) {
         line32d_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
         line32d_nf(dst, src, t.sctab32_nf[1]); dst += pitch;
         src += delta;
      }
   }
}

void rend_copy32t_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line32t_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      line32t_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      line32t_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy32q_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
#ifdef QUAD_BUFFER
      u8 buffer[MAX_WIDTH*4*sizeof(DWORD)];
      line32q_nf(buffer, src, t.sctab32_nf[0]);
      for (int i = 0; i < 4; i++) {
         memcpy(dst, buffer, temp.scx*16);
         dst += pitch;
      }
#else
      line32q_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      line32q_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      line32q_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
      line32q_nf(dst, src, t.sctab32_nf[0]); dst += pitch;
#endif
      src += delta;
   }
}

void rend_copy32d1(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf;
   unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++)
   {
      line32d(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy32d(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf;
   unsigned delta = temp.scx / 4;
   for (unsigned y = 0; y < temp.scy; y++)
   {
      line32d(dst, src, t.sctab32[0]); dst += pitch; // Четные строки
      line32d(dst, src, t.sctab32[1]); dst += pitch; // Нечетные строки
      src += delta;
   }
}

void rend_copy32t(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf;
   unsigned delta = temp.scx / 4;
   for (unsigned y = 0; y < temp.scy; y++)
   {
      line32t(dst, src, t.sctab32[0]); dst += pitch;
      line32t(dst, src, t.sctab32[0]); dst += pitch;
      line32t(dst, src, t.sctab32[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy32q(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
#ifdef QUAD_BUFFER
      u8 buffer[MAX_WIDTH*4*sizeof(DWORD)];
      line32q(buffer, src, t.sctab32[0]);
      for (int i = 0; i < 4; i++) {
         memcpy(dst, buffer, temp.scx*16);
         dst += pitch;
      }
#else
      line32q(dst, src, t.sctab32[0]); dst += pitch;
      line32q(dst, src, t.sctab32[0]); dst += pitch;
      line32q(dst, src, t.sctab32[0]); dst += pitch;
      line32q(dst, src, t.sctab32[0]); dst += pitch;
#endif
      src += delta;
   }
}

void rend_copy16(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16(dst, src, t.sctab16[0]);
      dst += pitch, src += delta;
   }
}

void rend_copy16_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16_nf(dst, src, t.sctab16_nf[0]);
      dst += pitch, src += delta;
   }
}

void rend_copy16d1(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy16d(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16d(dst, src, t.sctab16d[0]); dst += pitch;
      line16d(dst, src, t.sctab16d[1]); dst += pitch;
      src += delta;
   }
}

void rend_copy16t(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16t(dst, src, t.sctab16d[0]); dst += pitch;
      line16t(dst, src, t.sctab16d[0]); dst += pitch;
      line16t(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy16q(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16q(dst, src, t.sctab16d[0]); dst += pitch;
      line16q(dst, src, t.sctab16d[0]); dst += pitch;
      line16q(dst, src, t.sctab16d[0]); dst += pitch;
      line16q(dst, src, t.sctab16d[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy16d1_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16d_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy16d_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   if (conf.alt_nf) {
      int offset = rb2_offs;
      if (comp.frame_counter & 1) src += rb2_offs, offset = -offset;
      for (unsigned y = 0; y < temp.scy; y++) {
         line16d(dst, src, t.sctab16d[0]); dst += pitch;
         line16d(dst, src+offset, t.sctab16d[0]); dst += pitch;
         src += delta;
      }
   } else {
      u8 *src = rbuf; unsigned delta = temp.scx/4;
      for (unsigned y = 0; y < temp.scy; y++) {
         line16d_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
         line16d_nf(dst, src, t.sctab16d_nf[1]); dst += pitch;
         src += delta;
      }
   }
}

void rend_copy16t_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16t_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      line16t_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      line16t_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy16q_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line16q_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      line16q_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      line16q_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      line16q_nf(dst, src, t.sctab16d_nf[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy8(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8(dst, src, t.sctab8[0]);
      dst += pitch, src += delta;
   }
}

void rend_copy8_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8_nf(dst, src, t.sctab8[0]);
      dst += pitch, src += delta;
   }
}

void rend_copy8d1(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy8d(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8d(dst, src, t.sctab8d[0]); dst += pitch;
      line8d(dst, src, t.sctab8d[1]); dst += pitch;
      src += delta;
   }
}

void rend_copy8t(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8t(dst, src, t.sctab8q); dst += pitch;
      line8t(dst, src, t.sctab8q); dst += pitch;
      line8t(dst, src, t.sctab8q); dst += pitch;
      src += delta;
   }
}

void rend_copy8q(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8q(dst, src, t.sctab8q); dst += pitch;
      line8q(dst, src, t.sctab8q); dst += pitch;
      line8q(dst, src, t.sctab8q); dst += pitch;
      line8q(dst, src, t.sctab8q); dst += pitch;
      src += delta;
   }
}

void rend_copy8d1_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8d_nf(dst, src, t.sctab8d[0]); dst += pitch;
      src += delta;
   }
}

void rend_copy8d_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   if (conf.alt_nf) {
      int offset = rb2_offs;
      if (comp.frame_counter & 1) src += rb2_offs, offset = -offset;
      for (unsigned y = 0; y < temp.scy; y++) {
         line8d(dst, src, t.sctab8d[0]); dst += pitch;
         line8d(dst, src+offset, t.sctab8d[0]); dst += pitch;
         src += delta;
      }
   } else {
      for (unsigned y = 0; y < temp.scy; y++) {
         line8d_nf(dst, src, t.sctab8d[0]); dst += pitch;
         line8d_nf(dst, src, t.sctab8d[1]); dst += pitch;
         src += delta;
      }
   }
}

void rend_copy8t_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8t_nf(dst, src, t.sctab8q); dst += pitch;
      line8t_nf(dst, src, t.sctab8q); dst += pitch;
      line8t_nf(dst, src, t.sctab8q); dst += pitch;
      src += delta;
   }
}

void rend_copy8q_nf(u8 *dst, unsigned pitch)
{
   u8 *src = rbuf; unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      line8q_nf(dst, src, t.sctab8q); dst += pitch;
      line8q_nf(dst, src, t.sctab8q); dst += pitch;
      line8q_nf(dst, src, t.sctab8q); dst += pitch;
      line8q_nf(dst, src, t.sctab8q); dst += pitch;
      src += delta;
   }
}
#endif