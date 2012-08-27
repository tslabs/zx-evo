#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxr_rsm.h"

RSM_DATA rsm;

void RSM_DATA::prepare_line_32(unsigned char *src0)
{
   unsigned i;
   for (i = 0; i < line_size_d; i++)
       line_buffer_d[i] = bias;

   unsigned line_size = line_size_d / 2, frame = 0;
   __m64 *tab = colortab;
   for (;;)
   {
      unsigned char *src = src0;
      for (i = 0; i < line_size; )
      {
         unsigned s = *(unsigned*)src, attr = (s >> 6) & 0x3FC;
         line_buffer[i+0] = _mm_add_pi8(line_buffer[i+0], tab[((s >> 6) & 3) + attr]);
         line_buffer[i+1] = _mm_add_pi8(line_buffer[i+1], tab[((s >> 4) & 3) + attr]);
         line_buffer[i+2] = _mm_add_pi8(line_buffer[i+2], tab[((s >> 2) & 3) + attr]);
         line_buffer[i+3] = _mm_add_pi8(line_buffer[i+3], tab[((s >> 0) & 3) + attr]);
         attr = (s >> 22) & 0x3FC;
         line_buffer[i+4] = _mm_add_pi8(line_buffer[i+4], tab[((s >>22) & 3) + attr]);
         line_buffer[i+5] = _mm_add_pi8(line_buffer[i+5], tab[((s >>20) & 3) + attr]);
         line_buffer[i+6] = _mm_add_pi8(line_buffer[i+6], tab[((s >>18) & 3) + attr]);
         line_buffer[i+7] = _mm_add_pi8(line_buffer[i+7], tab[((s >>16) & 3) + attr]);
         i += 8, src += 4;
      }
      if (++frame == mix_frames)
          break;
      src0 += rb2_offs;
      if (src0 >= rbuf_s + rb2_offs * mix_frames)
          src0 -= rb2_offs * mix_frames;
      tab += 0x100*4;
   }
}

void RSM_DATA::prepare_line_16(unsigned char *src0)
{
   unsigned i;
   for (i = 0; i < line_size_d; i++)
       line_buffer_d[i] = bias;

   unsigned line_size = line_size_d / 2, frame = 0;
   __m64 *tab = colortab;
   for (;;) {
      unsigned char *src = src0;
      for (i = 0; i < line_size; ) {
         unsigned s = *(unsigned*)src, attr = (s >> 4) & 0xFF0;
         line_buffer[i+0] = _mm_add_pi8(line_buffer[i+0], tab[((s >> 4)  & 0xF) + attr]);
         line_buffer[i+1] = _mm_add_pi8(line_buffer[i+1], tab[((s >> 0)  & 0xF) + attr]);
         attr = (s >> 20) & 0xFF0;
         line_buffer[i+2] = _mm_add_pi8(line_buffer[i+2], tab[((s >> 20) & 0xF) + attr]);
         line_buffer[i+3] = _mm_add_pi8(line_buffer[i+3], tab[((s >> 16) & 0xF) + attr]);
         src += 4; i += 4;
      }
      if (++frame == mix_frames) break;
      src0 += rb2_offs;
      if (src0 >= rbuf_s + rb2_offs * mix_frames) src0 -= rb2_offs * mix_frames;
      tab += 0x100*16;
   }
}

void RSM_DATA::prepare_line_8(unsigned char *src0)
{
   unsigned i;
   for (i = 0; i < line_size_d; i++)
       line_buffer_d[i] = bias;

   unsigned frame = 0;
   DWORD *tab = (DWORD*)colortab;
   for (;;) {
      unsigned char *src = src0;
      for (i = 0; i < line_size_d; ) {
         unsigned s = *(unsigned*)src, attr = (s >> 4) & 0xFF0;
         line_buffer_d[i+0] += tab[((s >> 4)  & 0xF) + attr];
         line_buffer_d[i+1] += tab[((s >> 0)  & 0xF) + attr];
         attr = (s >> 20) & 0xFF0;
         line_buffer_d[i+2] += tab[((s >> 20) & 0xF) + attr];
         line_buffer_d[i+3] += tab[((s >> 16) & 0xF) + attr];
         s = *(unsigned*)(src+4), attr = (s >> 4) & 0xFF0;
         line_buffer_d[i+4] += tab[((s >> 4)  & 0xF) + attr];
         line_buffer_d[i+5] += tab[((s >> 0)  & 0xF) + attr];
         attr = (s >> 20) & 0xFF0;
         line_buffer_d[i+6] += tab[((s >> 20) & 0xF) + attr];
         line_buffer_d[i+7] += tab[((s >> 16) & 0xF) + attr];
         src += 8, i += 8;
      }
      if (++frame == mix_frames) break;
      src0 += rb2_offs;
      if (src0 >= rbuf_s + rb2_offs * mix_frames) src0 -= rb2_offs * mix_frames;
      tab += 0x100*16;
   }
}

void rend_rsm_8(unsigned char *dst, unsigned pitch, unsigned char *src)
{
   unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      rsm.prepare_line_8(src);
      for (unsigned i = 0; i < rsm.line_size_d; i++)
         *(unsigned*)(dst + i*4) = rsm.line_buffer_d[i];
      dst += pitch; src += delta;
   }
}

void rend_rsm_16(unsigned char *dst, unsigned pitch, unsigned char *src)
{
   unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      rsm.prepare_line_32(src);
      // pack truecolor pixel to 16 bit
      if (temp.hi15 == 0)
         for (unsigned i = 0; i < rsm.line_size_d; i+=2) {
            unsigned c1 = rsm.line_buffer_d[i];
            unsigned c2 = rsm.line_buffer_d[i+1];
            *(unsigned*)(dst + i*2) =
              ((c1 >> 3) & 0x1F) + ((c1 >> 5) & 0x07E0) + ((c1 >> 8) & 0xF800) +
              ((c2 << 13) & 0x1F0000) + ((c2 << 11) & 0x07E00000) + ((c2 << 8) & 0xF8000000);
         }
      else /* if (temp.hi15 == 1) */
         for (unsigned i = 0; i < rsm.line_size_d; i+=2) {
            unsigned c1 = rsm.line_buffer_d[i];
            unsigned c2 = rsm.line_buffer_d[i+1];
            *(unsigned*)(dst + i*2) =
              ((c1 >> 3) & 0x1F) + ((c1 >> 6) & 0x03E0) + ((c1 >> 9) & 0x7C00) +
              ((c2 << 13) & 0x1F0000) + ((c2 << 10) & 0x03E00000) + ((c2 << 7) & 0x7C000000);
         }

      dst += pitch; src += delta;
   }
}

void rend_rsm_16o(unsigned char *dst, unsigned pitch, unsigned char *src)
{
   unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      rsm.prepare_line_16(src);
      for (unsigned i = 0; i < rsm.line_size_d; i++)
         *(unsigned*)(dst + i*4) = rsm.line_buffer_d[i];

      dst += pitch; src += delta;
   }
}

void rend_rsm_32(unsigned char *dst, unsigned pitch, unsigned char *src)
{
   unsigned delta = temp.scx/4;
   for (unsigned y = 0; y < temp.scy; y++) {
      rsm.prepare_line_32(src);
      for (unsigned i = 0; i < rsm.line_size_d; i++)
         *(unsigned*)(dst + i*4) = rsm.line_buffer_d[i];
      dst += pitch; src += delta;
   }
}

void __fastcall render_rsm(u8 *dst, u32 pitch)
{
   rsm.colortab = (__m64*)((int)rsm.tables + rsm.frame * rsm.frame_table_size);
   unsigned char *src = rbuf_s + rb2_offs * rsm.rbuf_dst;

   if (temp.obpp ==  8) rend_rsm_8 (dst, pitch, src);
   if (temp.obpp == 16) { if (rsm.mode == 0) rend_rsm_16(dst, pitch, src); else rend_rsm_16o(dst, pitch, src); }
   if (temp.obpp == 32) rend_rsm_32(dst, pitch, src);

   _mm_empty(); // EMMS
}

unsigned gcd(unsigned x, unsigned y)
{
   while (x != y) if (x > y) x -= y; else y -= x;
   return x;
}

unsigned lcm(unsigned x, unsigned y)
{
   return x*y / gcd(x,y);
}

void calc_rsm_tables()
{
   rsm.rbuf_dst = rsm.frame = 0;

   if (renders[conf.render].func != render_rsm) {
      rsm.mix_frames = rsm.period = 1;
      static unsigned char one = 1;
      rsm.needframes = &one; // rsm.needframes[0]=1
      return;
   }

   rsm.mode = (temp.obpp == 8)? 2 : 0;
   if (temp.obpp == 16 && temp.hi15 == 2) rsm.mode = 1;

   rsm.line_size_d = (temp.scx >> rsm.mode);

   unsigned fmax = lcm(conf.intfq, temp.ofq);
   rsm.period = fmax / conf.intfq;
   unsigned step = fmax / temp.ofq;

   rsm.mix_frames = (conf.rsm.mode == RSM_SIMPLE)? 2 : conf.rsm.mix_frames;

   rsm.frame_table_size = rsm.mix_frames * 0x100;
   if (rsm.mode == 0) rsm.frame_table_size *= 4*sizeof(__m64);
   if (rsm.mode == 1) rsm.frame_table_size *= 16*sizeof(__m64);
   if (rsm.mode == 2) rsm.frame_table_size *= 16*sizeof(DWORD);

   rsm.data = (unsigned char*)realloc(rsm.data, rsm.period * (rsm.frame_table_size + 1));
   rsm.tables = (__m64*)rsm.data;
   rsm.needframes = rsm.data + rsm.frame_table_size * rsm.period;

   double *weights = (double*)alloca(rsm.period * rsm.mix_frames * sizeof(double));
   double *dst = weights;

   unsigned low_bias = 0, dynamic_range = 0xFF;

   if (conf.rsm.mode != RSM_SIMPLE) {
      unsigned fsize = rsm.period * rsm.mix_frames;
      double *flt = (double*)alloca((fsize+1) * sizeof(double));

      double cutoff = 0.9;
      if (conf.rsm.mode == RSM_FIR1) cutoff = 0.5;
      if (conf.rsm.mode == RSM_FIR2) cutoff = 0.33333333;

      double pi = 4*atan(1.0);
      cutoff *= 1 / (double)rsm.period; // cutoff scale = inftq/maxfq = 1/rsm.period
      double c1 = 0.54 / pi, c2 = 0.46 / pi;
      for (unsigned i = 0; i <= fsize; i++) {
         if (i == fsize/2) flt[i] = cutoff;
         else flt[i] = sin(pi*cutoff*((double)i - fsize/2)) * (c1 - c2*cos(2*pi*(double)i/fsize)) / ((double)i - fsize/2);
      }

      double low_b = 0, high_b = 0;
      for (unsigned frame = 0; frame < rsm.period; frame++)
      {
         unsigned pos = frame * step, srcframe = pos / rsm.period;
         if (frame) srcframe++; // (pos % rsm.period) != 0

         unsigned nextpos = pos + step, nextframe = nextpos / rsm.period;
         if (frame+1 != rsm.period) nextframe++; // (nextpos % rsm.period) != 0

         rsm.needframes[frame] = (nextframe - srcframe);
         double low = 0, high = 0;
         unsigned offset = (srcframe * rsm.period) - pos;
         for (unsigned ch = 0; ch < rsm.mix_frames; ch++) {
            double weight = flt[offset] * rsm.period;
            if (weight < 0) low += weight; else high += weight;
            *dst++ = weight;
            offset += rsm.period;
         }
         if (low < low_b) low_b = low;
         if (high > high_b) high_b = high;
      }
      low_bias = (unsigned)((-low_b)*0xFF);
      dynamic_range = (0xFF - low_bias);
   } else { // RSM_SIMPLE

      double div = 1 / (double)step;
      for (unsigned frame = 0; frame < rsm.period; frame++)
      {
         unsigned pos = frame * step, srcframe = pos / rsm.period;
         unsigned nextpos = pos + step, nextframe = nextpos / rsm.period;
         unsigned offset = (srcframe == nextframe)? step : (nextpos - nextframe*rsm.period);
         rsm.needframes[frame] = (nextframe - srcframe);
         *dst++ = offset * div;
         *dst++ = (step - offset) * div;
      }
   }

   rsm.bias = 0x01010101 * low_bias;

   unsigned char *dst32 = (unsigned char*)rsm.tables;
   for (unsigned frame = 0; frame < rsm.period; frame++)
      for (unsigned ch = 0; ch < rsm.mix_frames; ch++)
      {
         unsigned char *start_frame = dst32;
         switch (rsm.mode)
         {
            case 0: // rgb 16/32bit - reorder table, MMX processes 2 truecolor pixels at one op
               for (unsigned a = 0; a < 0x100; a++)
                  for (unsigned pix = 0; pix < 4; pix++) {
                     *(DWORD*)(dst32+0) = t.sctab32[0][(pix >>1)*0x100 + a];
                     *(DWORD*)(dst32+4) = t.sctab32[0][(pix & 1)*0x100 + a];
                     dst32 += 8;
                  }
               break;
            case 1: // YUY2 overlay - reorder table, MMX processes 4 overlay pixels at one op
               for (unsigned a = 0; a < 0x100; a++)
                  for (unsigned pix = 0; pix < 16; pix++) {
                     *(DWORD*)(dst32+0) = t.sctab16[0][a*4 + (pix >> 2)];
                     *(DWORD*)(dst32+4) = t.sctab16[0][a*4 + (pix &  3)];
                     dst32 += 8;
                  }
               break;
            case 2:
               memcpy(dst32, t.sctab8[0], 0x100*16*sizeof(DWORD)), dst32 += 0x100*16*sizeof(DWORD);
               break;
            default:
               __assume(0);
         }

         double scale = *weights++ * dynamic_range * (1/256.0);
         for (unsigned char *ptr = start_frame; ptr < dst32; ptr++) {
            double color = scale * *ptr;
            if (color > 255) color = 255;
            if (color < 0) color += 256; // color = 0;
            *ptr = (unsigned char)color;
         }
      }
}
