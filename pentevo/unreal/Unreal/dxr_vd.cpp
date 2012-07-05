#include "std.h"

// SAM style video drive (only 8 bit, double-size screen)

#ifdef MOD_VID_VD

void rend_vd8dbl(unsigned char *dst, unsigned pitch)
{
   unsigned char *dst2 = dst + (temp.ox-512)*temp.obpp/16;
   dst2 += (temp.scy-192)/2*(pitch*2);

   if (conf.fast_sl) {

      pitch *= 2;
      rend_frame_8d1(dst, pitch);
      dst = dst2;
      for (unsigned y = 0; y < 192; y++) {
         unsigned char *src = (unsigned char*)vdmem + t.scrtab[y];
         for (unsigned x = 0; x < 32; x++) {
            __m64 d =          t.vdtab[0][0][src[0x2000*0+x]];
            d = _mm_add_pi8(d, t.vdtab[0][1][src[0x2000*1+x]]);
            d = _mm_add_pi8(d, t.vdtab[0][2][src[0x2000*2+x]]);
            d = _mm_add_pi8(d, t.vdtab[0][3][src[0x2000*3+x]]);

            *(__m64*)(dst+x*16+0) = _mm_unpacklo_pi8(d,d);
            *(__m64*)(dst+x*16+8) = _mm_unpackhi_pi8(d,d);
         }
         dst += pitch;
      }

   } else {

      rend_frame_8d(dst, pitch);
      dst = dst2;
      for (unsigned y = 0; y < 192; y++) {
         unsigned char *src = (unsigned char*)vdmem + t.scrtab[y];
         for (unsigned pass = 0; pass < 2; pass++) {
            for (unsigned x = 0; x < 32; x++) {
               __m64 d =          t.vdtab[pass][0][src[0x2000*0+x]];
               d = _mm_add_pi8(d, t.vdtab[pass][1][src[0x2000*1+x]]);
               d = _mm_add_pi8(d, t.vdtab[pass][2][src[0x2000*2+x]]);
               d = _mm_add_pi8(d, t.vdtab[pass][3][src[0x2000*3+x]]);

               *(__m64*)(dst+x*16+0) = _mm_unpacklo_pi8(d,d);
               *(__m64*)(dst+x*16+8) = _mm_unpackhi_pi8(d,d);
            }
            dst += pitch;
         }
      }
   }
   _mm_empty();
}

#endif