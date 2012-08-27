#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrend.h"
#include "dxrcopy.h"
#include "dxrframe.h"
#include "dxr_advm.h"

// http://scale2x.sourceforge.net/algorithm.html

inline void line_8_any(unsigned char *dst, unsigned char *src)
{
   if (conf.noflic) line8_nf(dst, src, t.sctab8[0]);
   else line8(dst, src, t.sctab8[0]);
}

inline void line_32_any(unsigned char *dst, unsigned char *src)
{
   if (conf.noflic) line32_nf(dst, src, t.sctab32[0]);
   else line32(dst, src, t.sctab32[0]);
}

#if 1   // switch between vectorized and branched code

#ifdef MOD_SSE2

void lines_scale2(const unsigned char *src, unsigned y, unsigned char *dst1, unsigned char *dst2, unsigned nPix)
{
   const unsigned char
      *u = src + ((y-1) & 7)*sc2lines_width,
      *m = src + ((y+0) & 7)*sc2lines_width,
      *l = src + ((y+1) & 7)*sc2lines_width;

   for (unsigned i = 0; i < nPix; i += 8) {

      __m64 uu = *(__m64*)(u+i);
      __m64 ll = *(__m64*)(l+i);
      __m64 cmp = _mm_cmpeq_pi8(uu,ll);

      if (_mm_movemask_pi8(cmp) != 0xFF) {

         __m128i mm = _mm_loadu_si128((__m128i*)(m+i-4));
         __m128i uu = _mm_loadu_si128((__m128i*)(u+i-4));
         __m128i ll = _mm_loadu_si128((__m128i*)(l+i-4));

         __m128i md = _mm_slli_si128(mm,1);
         __m128i mf = _mm_srli_si128(mm,1);
         __m128i maskall = _mm_or_si128(_mm_cmpeq_epi8(md,mf), _mm_cmpeq_epi8(uu,ll));

         __m128i e0, e1, v1, v2, v3;

         e0 = _mm_cmpeq_epi8(md,uu);
         e0 = _mm_andnot_si128(maskall, e0);
         e0 = _mm_srli_si128(e0,4);
         e0 = _mm_unpacklo_epi8(e0, _mm_setzero_si128());

         e1 = _mm_cmpeq_epi8(mf,uu);
         e1 = _mm_andnot_si128(maskall, e1);
         e1 = _mm_srli_si128(e1,4);
         e1 = _mm_unpacklo_epi8(_mm_setzero_si128(), e1);

         e0 = _mm_or_si128(e0, e1);

         v1 = _mm_srli_si128(mm,4);
         v1 = _mm_unpacklo_epi8(v1,v1);
         v2 = _mm_srli_si128(uu,4);
         v2 = _mm_unpacklo_epi8(v2,v2);

         _mm_store_si128((__m128i*)(dst1 + 2*i), _mm_or_si128( _mm_and_si128(e0,v2), _mm_andnot_si128(e0,v1) ) );

         e0 = _mm_cmpeq_epi8(md,ll);
         e0 = _mm_andnot_si128(maskall, e0);
         e0 = _mm_srli_si128(e0,4);
         e0 = _mm_unpacklo_epi8(e0, _mm_setzero_si128());

         e1 = _mm_cmpeq_epi8(mf,ll);
         e1 = _mm_andnot_si128(maskall, e1);
         e1 = _mm_srli_si128(e1,4);
         e1 = _mm_unpacklo_epi8(_mm_setzero_si128(), e1);

         e0 = _mm_or_si128(e0, e1);

         v3 = _mm_srli_si128(ll,4);
         v3 = _mm_unpacklo_epi8(v3,v3);

         _mm_store_si128((__m128i*)(dst2 + 2*i), _mm_or_si128( _mm_and_si128(e0,v3), _mm_andnot_si128(e0,v1) ) );

      } else {

         __m64 v0 = *(__m64*)(m+i);
         __m128i v1 = _mm_movpi64_epi64(v0);
         v1 = _mm_unpacklo_epi8(v1,v1);
         _mm_store_si128((__m128i*)(dst1 + 2*i), v1);
         _mm_store_si128((__m128i*)(dst2 + 2*i), v1);
      }
   }
}

#else // MMX vectorized

void lines_scale2(const unsigned char *src, unsigned y, unsigned char *dst1, unsigned char *dst2, unsigned nPix)
{
   const unsigned char
      *u = src + ((y-1) & 7)*sc2lines_width,
      *m = src + ((y+0) & 7)*sc2lines_width,
      *l = src + ((y+1) & 7)*sc2lines_width;

   for (unsigned i = 0; i < nPix; i += 4) {

      if (*(unsigned*)(u+i) ^ *(unsigned*)(l+i)) {

         __m64 mm = *(__m64*)(m+i-2);
         __m64 uu = *(__m64*)(u+i-2);
         __m64 ll = *(__m64*)(l+i-2);
         __m64 md = _mm_slli_si64(mm,8);
         __m64 mf = _mm_srli_si64(mm,8);
         __m64 maskall = _mm_or_si64(_mm_cmpeq_pi8(md,mf), _mm_cmpeq_pi8(uu,ll));

         __m64 e0, e1, v1, v2;

         e0 = _mm_cmpeq_pi8(md,uu);
         e0 = _mm_andnot_si64(maskall, e0);
         e0 = _mm_srli_si64(e0,16);
         e0 = _mm_unpacklo_pi8(e0, _mm_setzero_si64());

         e1 = _mm_cmpeq_pi8(mf,uu);
         e1 = _mm_andnot_si64(maskall, e1);
         e1 = _mm_srli_si64(e1,16);
         e1 = _mm_unpacklo_pi8(_mm_setzero_si64(), e1);

         e0 = _mm_or_si64(e0, e1);

         v1 = _m_from_int(*(unsigned*)(m+i));
         v2 = _m_from_int(*(unsigned*)(u+i));
         v1 = _mm_unpacklo_pi8(v1,v1);
         v2 = _mm_unpacklo_pi8(v2,v2);

         *(__m64*)(dst1 + 2*i) = _mm_or_si64( _mm_and_si64(e0,v2), _mm_andnot_si64(e0,v1) );

         e0 = _mm_cmpeq_pi8(md,ll);
         e0 = _mm_andnot_si64(maskall, e0);
         e0 = _mm_srli_si64(e0,16);
         e0 = _mm_unpacklo_pi8(e0, _mm_setzero_si64());

         e1 = _mm_cmpeq_pi8(mf,ll);
         e1 = _mm_andnot_si64(maskall, e1);
         e1 = _mm_srli_si64(e1,16);
         e1 = _mm_unpacklo_pi8(_mm_setzero_si64(), e1);

         e0 = _mm_or_si64(e0, e1);

         v1 = _m_from_int(*(unsigned*)(m+i));
         v2 = _m_from_int(*(unsigned*)(l+i));
         v1 = _mm_unpacklo_pi8(v1,v1);
         v2 = _mm_unpacklo_pi8(v2,v2);

         *(__m64*)(dst2 + 2*i) = _mm_or_si64( _mm_and_si64(e0,v2), _mm_andnot_si64(e0,v1) );

      } else {

         __m64 v1 = _m_from_int(*(unsigned*)(m+i));
         v1 = _mm_unpacklo_pi8(v1,v1);
         *(__m64*)(dst1 + 2*i) = v1;
         *(__m64*)(dst2 + 2*i) = v1;

      }

   }
}

#endif // SSE2

#else // MMX branched
// src       dst
// ABC       e0e1
// DEF       e2e3
// GHI

/*
if (B != H && D != F)
{                               E0 = E;
    E0 = D == B ? D : E;        E1 = E;
    E1 = B == F ? F : E;        E2 = E;
    E2 = D == H ? D : E;        E3 = E;
    E3 = H == F ? F : E;        if (B != H) continue;
}                          =>   if (D != F)
else                            {
{                                   E0 = D == B ? D : E;
    E0 = E;                         E1 = B == F ? F : E;
    E1 = E;                         E2 = D == H ? D : E;
    E2 = E;                         E3 = H == F ? F : E;
    E3 = E;                     }
}
*/

void lines_scale2(const unsigned char *src, unsigned y, unsigned char *dst1, unsigned char *dst2, unsigned nPix)
{
   const unsigned char
      *u = src + ((y-1) & 7)*sc2lines_width,
      *m = src + ((y+0) & 7)*sc2lines_width,
      *l = src + ((y+1) & 7)*sc2lines_width;

   // process 4pix per iteration
   for (unsigned i = 0; i < nPix; i += 4)
   {
      unsigned dw = *(unsigned*)(m+i);
      __m64 v1 = _mm_cvtsi32_si64(dw); // v1   =     0|    0|    0|    0|dw[3]|dw[2]|dw[1]|dw[0]
      v1 = _mm_unpacklo_pi8(v1,v1);    // v1   = dw[3]|dw[3]|dw[2]|dw[2]|dw[1]|dw[1]|dw[0]|dw[0]
      *(__m64*)(dst1 + 2*i) = v1;      // e0e1 = dw[3]|dw[3]|dw[2]|dw[2]|dw[1]|dw[1]|dw[0]|dw[0]
      *(__m64*)(dst2 + 2*i) = v1;      // e2e3 = dw[3]|dw[3]|dw[2]|dw[2]|dw[1]|dw[1]|dw[0]|dw[0]

      dw = *(unsigned*)(u+i) ^ *(unsigned*)(l+i);
      if (!dw)
          continue; // u == l

   #define process_pix(n)                                       \
      if ((dw & (0xFF << (8*n))) && m[i+n-1] != m[i+n+1])       \
      {                                                         \
         if (u[i+n] == m[i+n-1])                                \
             dst1[2*(i+n)] = u[i+n];                            \
         if (u[i+n] == m[i+n+1])                                \
             dst1[2*(i+n)+1] = u[i+n];                          \
         if (l[i+n] == m[i+n-1])                                \
             dst2[2*(i+n)] = l[i+n];                            \
         if (l[i+n] == m[i+n+1])                                \
             dst2[2*(i+n)+1] = l[i+n];                          \
      }

      process_pix(0);
      process_pix(1);
      process_pix(2);
      process_pix(3);
   #undef process_pix
   }
}

#endif // MMX branched

void lines_scale2_32(const unsigned char *src, unsigned y, unsigned char *dst1, unsigned char *dst2, unsigned nPix)
{
   const u32 *s = (u32 *)src;
   const u32 *u = s + ((y-1) & 7)*sc2lines_width;
   const u32 *m = s + ((y+0) & 7)*sc2lines_width;
   const u32 *l = s + ((y+1) & 7)*sc2lines_width;
   u32 *d1 = (u32 *)dst1;
   u32 *d2 = (u32 *)dst2;

   for (unsigned i = 0; i < nPix; i++)
   {
      d1[2*i] = d1[2*i+1] = d2[2*i] = d2[2*i+1] = m[i];

      if (u[i] != l[i] && m[i-1] != m[i+1])
      {
         if (u[i] == m[i+1])
             d1[2*i] = u[i];
         if (u[i] == m[i+1])
             d1[2*i+1] = u[i];
         if (l[i] == m[i-1])
             d2[2*i] = l[i];
         if (l[i] == m[i+1])
             d2[2*i+1] = l[i];
      }
   }
}

// 8bpp
void render_scale2(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf; unsigned delta = temp.scx/4;
   line_8_any(t.scale2buf[0], src);
   // assume 'above' screen line same as line 0
   memcpy(t.scale2buf[(0-1) & 7], t.scale2buf[0], temp.scx);
   for (unsigned y = 0; y < temp.scy; y++)
   {
      src += delta;
      line_8_any(t.scale2buf[(y+1) & 7], src);
      lines_scale2(t.scale2buf[0], y, dst, dst+pitch, temp.scx);
      dst += 2*pitch;
   }
}

// 32bpp
void render_scale2_32(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf;
   unsigned delta = temp.scx/4;
   line_32_any((u8 *)t.scale2buf32[0], src);

   // assume 'above' screen line same as line 0
   memcpy(t.scale2buf32[(0-1) & 7], t.scale2buf32[0], temp.scx);
   for (unsigned y = 0; y < temp.scy; y++)
   {
      src += delta;
      line_32_any((u8 *)t.scale2buf32[(y+1) & 7], src);
      lines_scale2_32((u8 *)t.scale2buf32[0], y, dst, dst+pitch, temp.scx);
      dst += 2*pitch;
   }
}

// MMX-vectorized version is not ready yet :(
// 8bpp
void lines_scale3(unsigned y, unsigned char *dst, unsigned pitch)
{

   const unsigned char
      *u = t.scale2buf[(y-1) & 3],
      *m = t.scale2buf[(y+0) & 3],
      *l = t.scale2buf[(y+1) & 3];

   for (unsigned i = 0; i < temp.scx; i += 4)
   {
      unsigned char c;

      c = m[i];
      dst[3*i+0+0*pitch+ 0] = dst[3*i+1+0*pitch+ 0] = dst[3*i+2+0*pitch+ 0] = c;
      dst[3*i+0+1*pitch+ 0] = dst[3*i+1+1*pitch+ 0] = dst[3*i+2+1*pitch+ 0] = c;
      dst[3*i+0+2*pitch+ 0] = dst[3*i+1+2*pitch+ 0] = dst[3*i+2+2*pitch+ 0] = c;

      c = m[i+1];
      dst[3*i+0+0*pitch+ 3] = dst[3*i+1+0*pitch+ 3] = dst[3*i+2+0*pitch+ 3] = c;
      dst[3*i+0+1*pitch+ 3] = dst[3*i+1+1*pitch+ 3] = dst[3*i+2+1*pitch+ 3] = c;
      dst[3*i+0+2*pitch+ 3] = dst[3*i+1+2*pitch+ 3] = dst[3*i+2+2*pitch+ 3] = c;

      c = m[i+2];
      dst[3*i+0+0*pitch+ 6] = dst[3*i+1+0*pitch+ 6] = dst[3*i+2+0*pitch+ 6] = c;
      dst[3*i+0+1*pitch+ 6] = dst[3*i+1+1*pitch+ 6] = dst[3*i+2+1*pitch+ 6] = c;
      dst[3*i+0+2*pitch+ 6] = dst[3*i+1+2*pitch+ 6] = dst[3*i+2+2*pitch+ 6] = c;

      c = m[i+3];
      dst[3*i+0+0*pitch+ 9] = dst[3*i+1+0*pitch+ 9] = dst[3*i+2+0*pitch+ 9] = c;
      dst[3*i+0+1*pitch+ 9] = dst[3*i+1+1*pitch+ 9] = dst[3*i+2+1*pitch+ 9] = c;
      dst[3*i+0+2*pitch+ 9] = dst[3*i+1+2*pitch+ 9] = dst[3*i+2+2*pitch+ 9] = c;

      unsigned dw = *(unsigned*)(u+i) ^ *(unsigned*)(l+i);
      if (!dw) continue;

   #define process_pix(n)                                                                              \
      if ((dw & (0xFF << (8*n))) && m[i+n-1] != m[i+n+1])                                              \
      {                                                                                                \
         if (u[i+n] == m[i+n-1])                                                                       \
             dst[0*pitch+3*(i+n)] = u[i+n];                                                            \
         if ((u[i+n] == m[i+n-1] && m[i+n] != u[i+n+1]) || (u[i+n] == m[i+n+1] && m[i+n] != u[i+n-1])) \
             dst[0*pitch+3*(i+n)+1] = u[i+n];                                                          \
         if (u[i+n] == m[i+n+1])                                                                       \
             dst[0*pitch+3*(i+n)+2] = u[i+n];                                                          \
         if ((u[i+n] == m[i+n-1] && m[i+n] != l[i+n-1]) || (l[i+n] == m[i+n-1] && m[i+n] != u[i+n-1])) \
             dst[1*pitch+3*(i+n)+0] = m[i+n-1];                                                        \
         if ((u[i+n] == m[i+n+1] && m[i+n] != l[i+n+1]) || (l[i+n] == m[i+n+1] && m[i+n] != u[i+n+1])) \
             dst[1*pitch+3*(i+n)+2] = m[i+n+1];                                                        \
         if (l[i+n] == m[i+n-1])                                                                       \
             dst[2*pitch+3*(i+n)] = l[i+n];                                                            \
         if ((l[i+n] == m[i+n-1] && m[i+n] != l[i+n+1]) || (l[i+n] == m[i+n+1] && m[i+n] != l[i+n-1])) \
             dst[2*pitch+3*(i+n)+1] = l[i+n];                                                          \
         if (l[i+n] == m[i+n+1])                                                                       \
             dst[2*pitch+3*(i+n)+2] = l[i+n];                                                          \
      }

      process_pix(0);
      process_pix(1);
      process_pix(2);
      process_pix(3);
   #undef process_pix
   }
}

// 8bpp
void render_scale3(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf; unsigned delta = temp.scx/4;
   line_8_any(t.scale2buf[0], src);
   // assume 'above' screen line same as line 0
   memcpy(t.scale2buf[(0-1) & 3], t.scale2buf[0], temp.scx);
   for (unsigned y = 0; y < temp.scy; y++) {
      src += delta;
      line_8_any(t.scale2buf[(y+1) & 3], src);
      lines_scale3(y, dst, pitch);
      dst += 3*pitch;
   }
}

// 32bpp
void lines_scale3_32(unsigned y, unsigned char *dst, unsigned pitch)
{
   const u32 *u = t.scale2buf32[(y-1) & 3];
   const u32 *m = t.scale2buf32[(y+0) & 3];
   const u32 *l = t.scale2buf32[(y+1) & 3];
   u32 *d = (u32 *)dst;
   pitch /= sizeof(u32);

   for (unsigned i = 0; i < temp.scx; i++)
   {
      d[0*pitch+3*i+0] = d[0*pitch+3*i+1] = d[0*pitch+3*i+2] = m[i];
      d[1*pitch+3*i+0] = d[1*pitch+3*i+1] = d[1*pitch+3*i+2] = m[i];
      d[2*pitch+3*i+0] = d[2*pitch+3*i+1] = d[2*pitch+3*i+2] = m[i];

      if (u[i] != l[i] && m[i-1] != m[i+1])
      {
         if (u[i] == m[i-1])
             d[0*pitch+3*i+0] = u[i];
         if ((u[i] == m[i-1] && m[i] != u[i+1]) || (u[i] == m[i+1] && m[i] != u[i-1]))
             d[0*pitch+3*i+1] = u[i];
         if (u[i] == m[i+1])
             d[0*pitch+3*i+2] = u[i];
         if ((u[i] == m[i-1] && m[i] != l[i-1]) || (l[i] == m[i-1] && m[i] != u[i-1]))
             d[1*pitch+3*i+0] = m[i-1];
         if ((u[i] == m[i+1] && m[i] != l[i+1]) || (l[i] == m[i+1] && m[i] != u[i+1]))
             d[1*pitch+3*i+2] = m[i+1];
         if (l[i] == m[i-1])
             d[2*pitch+3*i+0] = l[i];
         if ((l[i] == m[i-1] && m[i] != l[i+1]) || (l[i] == m[i+1] && m[i] != l[i-1]))
             d[2*pitch+3*i+1] = l[i];
         if (l[i] == m[i+1])
             d[2*pitch+3*i+2] = l[i];
      }
   }
}

// 32bpp
void render_scale3_32(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf; unsigned delta = temp.scx/4;
   line_32_any((u8 *)t.scale2buf32[0], src);
   // assume 'above' screen line same as line 0
   memcpy(t.scale2buf32[(0-1) & 3], t.scale2buf32[0], temp.scx);
   for (unsigned y = 0; y < temp.scy; y++)
   {
      src += delta;
      line_32_any((u8 *)t.scale2buf32[(y+1) & 3], src);
      lines_scale3_32(y, dst, pitch);
      dst += 3*pitch;
   }
}

void render_scale4(unsigned char *dst, unsigned pitch)
{
   unsigned char *src = rbuf; unsigned delta = temp.scx/4;

   line_8_any(t.scale2buf[0], src); src += delta;
   line_8_any(t.scale2buf[1], src); src += delta;
   // assume 'above' screen line same as line 0
   memcpy(t.scale2buf[(0-1) & 7], t.scale2buf[0], temp.scx);
   lines_scale2(t.scale2buf[0], 0, t.scale4buf[0], t.scale4buf[1], temp.scx);

   for (unsigned y = 0; y < temp.scy; y++) {

      line_8_any(t.scale2buf[(y+2) & 7], src); src += delta;

      unsigned char *dst1 = t.scale4buf[(2*y+2) & 7];
      unsigned char *dst2 = t.scale4buf[(2*y+3) & 7];
      lines_scale2(t.scale2buf[0], y+1, dst1, dst2, temp.scx);

      lines_scale2(t.scale4buf[0], 2*y,   dst+0*pitch, dst+1*pitch, temp.scx*2);
      lines_scale2(t.scale4buf[0], 2*y+1, dst+2*pitch, dst+3*pitch, temp.scx*2);

      dst += 4*pitch;
   }
}

void __fastcall render_advmame(unsigned char *dst, unsigned pitch)
{
   switch (conf.videoscale)
   {
      case 2:
          if (temp.obpp == 8) render_scale2(dst, pitch);
          else if (temp.obpp == 32) render_scale2_32(dst, pitch);
      break;
      case 3:
          if (temp.obpp == 8) render_scale3(dst, pitch);
          else if (temp.obpp == 32) render_scale3_32(dst, pitch);
      break;
      case 4: render_scale4(dst, pitch); break;
      default: render_1x(dst, pitch); return; // skip noflic test
   }
   if (conf.noflic)
       memcpy(rbuf_s, rbuf, temp.scy*temp.scx/4);
   _mm_empty();
}
