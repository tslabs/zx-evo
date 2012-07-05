#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "drawnomc.h"
#include "dx.h"
#include "dxr_text.h"
#include "dxr_rsm.h"
#include "dxr_advm.h"
#include "memory.h"
#include "config.h"
#include "util.h"

#ifdef CACHE_ALIGNED
CACHE_ALIGNED unsigned char rbuf[sizeof_rbuf];
#else // __declspec(align) not available, force QWORD align with old method
__int64 rbuf__[sizeof_rbuf/sizeof(__int64)];
unsigned char * const rbuf = (unsigned char*)rbuf__;
#endif

unsigned char * const rbuf_s = rbuf + rb2_offs; // frames to mix with noflic and resampler filters
unsigned char * const save_buf = rbuf_s + rb2_offs*MAX_BUFFERS; // used in monitor

T t;

videopoint *vcurr;
videopoint video[4*MAX_HEIGHT];
unsigned vmode;  // what are drawing: 0-not visible, 1-border, 2-screen
unsigned prev_t; // last drawn pixel
unsigned *atrtab;

unsigned char colortab[0x100];// map zx attributes to pc attributes
// colortab shifted to 8 and 24
unsigned colortab_s8[0x100];
unsigned colortab_s24[0x100];

/*
#include "drawnomc.cpp"
#include "draw_384.cpp"
*/

PALETTEENTRY pal0[0x100]; // emulator palette

void AtmVideoController::PrepareFrameATM2(int VideoMode)
{
    for (int y=0; y<256; y++)
    {
        if ( VideoMode == 6 )
        {
            // смещени€ в текстовом видеорежиме
            Scanlines[y].Offset = 64*(y/8);
        } else {
            // смещени€ в растровом видеорежиме
            Scanlines[y].Offset = (y<56) ? 0 : 40*(y-56);
        }
        Scanlines[y].VideoMode = VideoMode;
    }
    CurrentRayLine = 0;
    IncCounter_InRaster = 0;
    IncCounter_InBorder = 0;
}

void AtmVideoController::PrepareFrameATM1(int VideoMode)
{
    for (int y=56; y<256; y++)
    {
        Scanlines[y].Offset = 40*(y-56);
        Scanlines[y].VideoMode = VideoMode;
    }
}


AtmVideoController AtmVideoCtrl;

void video_permanent_tables()
{
   // pixel doubling table
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < 0x100; i++) {
      unsigned res = 0;
      for (int j = 0x80; j; j/=2) {
         res <<= 2; if (i & j) res |= 3;
      }
      t.dbl[i] = res;
   }

   for (i = 0; i < 0x100; i++) {
      unsigned r1 = 0, r2 = 0;
      if (i & 0x01) r1++,        r2 += 1;
      if (i & 0x02) r1++,        r2 += 1;
      if (i & 0x04) r1++,        r2 += 0x100;
      if (i & 0x08) r1++,        r2 += 0x100;
      if (i & 0x10) r1 += 0x100, r2 += 0x10000;
      if (i & 0x20) r1 += 0x100, r2 += 0x10000;
      if (i & 0x40) r1 += 0x100, r2 += 0x1000000;
      if (i & 0x80) r1 += 0x100, r2 += 0x1000000;
      // low byte of settab - number of pixels in low nibble of i
      // high byte of low word of settab - number of pixels in high nibble of i
      t.settab[i] = r1;
      t.settab2[i] = r2*4; // *4 - convert square 2x2 to 4x4
   }

   i = 0; // calc screen addresses
   for (int p = 0; p < 4; p++)
      for (int y = 0; y < 8; y++)
         for (int o = 0; o < 8; o++, i++)
            t.scrtab[i] = p*0x800 + y*0x20 + o*0x100,
            t.atrtab_hwmc[i] = t.scrtab[i] + 0x2000,
            t.atrtab[i] = 0x1800 + (p*8+y)*32;

   // alco table
   static unsigned disp_0[] = { 0x0018, 0x2000, 0x2008, 0x2010, 0x2018, 0x0008 };
   static unsigned base_s[] = { 0x10000, 0x14000, 0x14800, 0x15000, 0x11800 };
   static unsigned base_a[] = { 0x11000, 0x15800, 0x15900, 0x15A00, 0x11300 };
   for (unsigned y = 0; y < 304; y++)
      for (unsigned x = 0; x < 6; x++) {
         unsigned disp = disp_0[x] + (y & 0x38)*4;
         ::t.alco[y][x].a = memory + base_a[y/64] + disp;
         ::t.alco[y][x].s = memory + base_s[y/64] + disp + (y & 7)*0x100;
      }

   #ifdef MOD_VID_VD
   // this code is only for ygrbYGRB palette
   for (unsigned byte = 0; byte < 0x100; byte++)
      for (int bit = 0; bit < 8; bit++)
         t.vdtab[0][0][byte].m64_u8[7-bit] = (byte & (1 << bit))? 0x11 : 0;
   for (int pl = 1; pl < 4; pl++)
      for (unsigned byte = 0; byte < 0x100; byte++)
         t.vdtab[0][pl][byte] = _mm_slli_pi32(t.vdtab[0][0][byte], pl);
   for (i = 0; i < sizeof t.vdtab[0]; i++)
      ((unsigned char*)t.vdtab[1])[i] = ((unsigned char*)t.vdtab[0])[i] & 0x0F;
   _mm_empty();
   #endif

   temp.offset_vscroll_prev = 0;
   temp.offset_vscroll = 0;
   temp.offset_hscroll_prev = 0;
   temp.offset_hscroll = 0;
}

unsigned getYUY2(unsigned r, unsigned g, unsigned b)
{
   int y = (int)(0.29*r + 0.59*g + 0.14*b);
   int u = (int)(128.0 - 0.14*r - 0.29*g + 0.43*b);
   int v = (int)(128.0 + 0.36*r - 0.29*g - 0.07*b);
   if (y < 0) y = 0; if (y > 255) y = 255;
   if (u < 0) u = 0; if (u > 255) u = 255;
   if (v < 0) v = 0; if (v > 255) v = 255;
   return WORD4(y,u,y,v);
}

void create_palette()
{
   if ((temp.rflags & RF_8BPCH) && temp.obpp == 8) temp.rflags |= RF_GRAY, conf.flashcolor = 0;

   PALETTE_OPTIONS *pl = &pals[conf.pal];
   unsigned char brights[4] = { pl->ZZ, pl->ZN, pl->NN, pl->BB };
   unsigned char brtab[16] =
      //  ZZ      NN      ZZ      BB
      { pl->ZZ, pl->ZN, pl->ZZ, pl->ZB,    // ZZ
        pl->ZN, pl->NN, pl->ZN, pl->NB,    // NN
        pl->ZZ, pl->ZN, pl->ZZ, pl->ZB,    // ZZ (bright=1,ink=0)
        pl->ZB, pl->NB, pl->ZB, pl->BB };  // BB

   for (unsigned i = 0; i < 0x100; i++) {
      unsigned r0, g0, b0;
      if (temp.rflags & RF_GRAY) { // grayscale palette
         r0 = g0 = b0 = i;
      } else if (temp.rflags & RF_PALB) { // palette index: gg0rr0bb
         b0 = brights[i & 3];
         r0 = brights[(i >> 3) & 3];
         g0 = brights[(i >> 6) & 3];
      } else { // palette index: ygrbYGRB
         b0 = brtab[((i>>0)&1)+((i>>2)&2)+((i>>2)&4)+((i>>4)&8)]; // brtab[ybYB]
         r0 = brtab[((i>>1)&1)+((i>>2)&2)+((i>>3)&4)+((i>>4)&8)]; // brtab[yrYR]
         g0 = brtab[((i>>2)&1)+((i>>2)&2)+((i>>4)&4)+((i>>4)&8)]; // brtab[ygYG]
      }

      // transform with current settings
      unsigned r = 0xFF & ((r0 * pl->r11 + g0 * pl->r12 + b0 * pl->r13) / 0x100);
      unsigned g = 0xFF & ((r0 * pl->r21 + g0 * pl->r22 + b0 * pl->r23) / 0x100);
      unsigned b = 0xFF & ((r0 * pl->r31 + g0 * pl->r32 + b0 * pl->r33) / 0x100);

      // prepare palette in bitmap header for GDI renderer
      gdibmp.header.bmiColors[i].rgbRed   = pal0[i].peRed   = r;
      gdibmp.header.bmiColors[i].rgbGreen = pal0[i].peGreen = g;
      gdibmp.header.bmiColors[i].rgbBlue  = pal0[i].peBlue  = b;
   }
   memcpy(syspalette + 10, pal0 + 10, (246-9) * sizeof *syspalette);
}

void atm_zc_tables();//forward

// make colortab: zx-attr -> pc-attr
void make_colortab(char flash_active)
{
   if (conf.flashcolor)
       flash_active = 0;

   for (unsigned a = 0; a < 0x100; a++)
   {
      unsigned char ink = a & 7;
      unsigned char paper = (a >> 3) & 7;
      unsigned char bright = (a >> 6) & 1;
      unsigned char flash = (a >> 7) & 1;

      if (ink)
          ink |= bright << 3; // no bright for 0th color

      if (paper)
          paper |= (conf.flashcolor ? flash : bright) << 3; // no bright for 0th color

      if (flash_active && flash)
      {
          unsigned char t = ink;
          ink = paper;
          paper = t;
      }

      u8 color = (paper << 4) | ink;

      colortab[a] = color;
      colortab_s8[a] = color << 8;
      colortab_s24[a] = color << 24;
   }

   if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_ATM450)
       atm_zc_tables(); // update with new flash bit
}

// make attrtab: pc-attr + 0x100*pixel -> palette index
void attr_tables()
{
   unsigned char flashcolor = (temp.rflags & RF_MON)? 0 : conf.flashcolor;
   for (unsigned a = 0; a < 0x100; a++)
   {
      unsigned char ink = (a & 0x0F), paper = (a >> 4);
      if (flashcolor)
          paper = (paper & 7) + (ink & 8); // paper_bright from ink

      if (temp.rflags & RF_GRAY)
      { // grayscale palette
         t.attrtab[a] = paper*16;
         t.attrtab[a+0x100] = ink*16;
      }
      else if (temp.rflags & RF_COMPPAL)
      { //------ for ATM palette - direct values from palette registers
         t.attrtab[a] = comp.comp_pal[a >> 4];
         t.attrtab[a+0x100] = comp.comp_pal[a & 0x0F];
      }
      else if (temp.rflags & RF_PALB)
      { //----------------------------- for bilinear
         unsigned char b0,b1, r0,r1, g0,g1;
         b0 = (paper >> 0) & 1, r0 = (paper >> 1) & 1, g0 = (paper >> 2) & 1;
         b1 = (ink >> 0) & 1, r1 = (ink >> 1) & 1, g1 = (ink >> 2) & 1;

         if (flashcolor && (a & 0x80))
         {
            b1 += b0, r1 += r0, g1 += g0;
            r0 = b0 = g0 = 0;
         }
         else
         {
            b0 *= 2, r0 *= 2, g0 *=2,
            b1 *= 2, r1 *= 2, g1 *=2;
         }

         unsigned char br1 = (ink >> 3) & 1;
         if (r1) r1 += br1;
         if (g1) g1 += br1;
         if (b1) b1 += br1;

         unsigned char br0 = (paper >> 3) & 1;
         if (r0) r0 += br0;
         if (g0) g0 += br0;
         if (b0) b0 += br0;

         // palette index: gg0rr0bb
         t.attrtab[a+0x100]  = (g1 << 6) + (r1 << 3) + b1;
         t.attrtab[a]        = (g0 << 6) + (r0 << 3) + b0;
      }
      else //------------------------------------ all others
      {
         // palette index: ygrbYGRB
         if (flashcolor && (a & 0x80))
         {
             t.attrtab[a] = 0;
             t.attrtab[a+0x100] = ink+(paper<<4);
         }
         else
         {
             t.attrtab[a] = paper * 0x11;
             t.attrtab[a+0x100] = ink * 0x11;
         }
      }
   }
}

void p4bpp_tables()
{
   for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned bt = 0; bt < 0x100; bt++) {
         unsigned lf = ((bt >> 3) & 7) + ((bt >> 4) & 8);
         unsigned rt = (bt & 7) + ((bt >> 3) & 8);
         if (temp.obpp == 8) {
            t.p4bpp8[pass][bt] = (t.sctab8[pass][0x0F+0x10*rt] & 0xFFFF) +
                                   (t.sctab8[pass][0x0F+0x10*lf] & 0xFFFF0000);
         } else if (temp.obpp == 16) {
            t.p4bpp16[pass][bt*2+0] = t.sctab16[pass][0x03+4*rt],
            t.p4bpp16[pass][bt*2+1] = t.sctab16[pass][0x03+4*lf];
         } else /* if (temp.obpp == 32) */ {
            t.p4bpp32[pass][bt*2+0] = t.sctab32[pass][0x100+rt],
            t.p4bpp32[pass][bt*2+1] = t.sctab32[pass][0x100+lf];
         }
      }
   }
}

void atm_zc_tables() // atm,profi screens (use normal zx-flash)
{
   for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned at = 0; at < 0x100; at++) {
         unsigned pc_attr = colortab[at];
         if (temp.obpp == 8)
            for (unsigned j = 0; j < 4; j++)
               t.zctab8ad[pass][at*4+j] = t.sctab8d[pass][pc_attr*4+j];
         else if (temp.obpp == 16)
            t.zctab16ad[pass][at] = t.sctab16d[pass][pc_attr],
            t.zctab16ad[pass][at+0x100] = t.sctab16d[pass][pc_attr+0x100];
         else /* if (temp.obpp == 32) */
            t.zctab32ad[pass][at] = t.sctab32[pass][pc_attr],
            t.zctab32ad[pass][at+0x100] = t.sctab32[pass][pc_attr+0x100];
      }
   }

   // atm palette mapping (port out to palette index)
   for (unsigned i = 0; i < 0x100; i++) {
      unsigned v = i ^ 0xFF, dst;
      if (conf.mem_model == MM_ATM450)
         dst = // ATM1: --grbGRB => Gg0Rr0Bb
               ((v & 0x20) << 1) | // g
               ((v & 0x10) >> 1) | // r
               ((v & 0x08) >> 3) | // b
               ((v & 0x04) << 5) | // G
               ((v & 0x02) << 3) | // R
               ((v & 0x01) << 1);  // B
      else
         dst = // ATM2: grbG--RB => Gg0Rr0Bb
               ((v & 0x80) >> 1) | // g
               ((v & 0x40) >> 3) | // r
               ((v & 0x20) >> 5) | // b
               ((v & 0x10) << 3) | // G
               ((v & 0x02) << 3) | // R
               ((v & 0x01) << 1);  // B
      t.atm_pal_map[i] = dst;
   }
}

void hires_sc_tables()  // atm,profi screens (use zx-attributes & flash -> paper_bright)
{
   for (unsigned pass = 0; pass < 2; pass++) {
      for (unsigned at = 0; at < 0x100; at++) {
         unsigned pc_attr = (at & 0x80) + (at & 0x38)*2 + (at & 0x40)/8 + (at & 7);
         if (temp.obpp == 8)
            for (unsigned j = 0; j < 16; j++)
               t.zctab8[pass][at*0x10+j] = t.sctab8[pass][pc_attr*0x10+j];
         else if (temp.obpp == 16)
            for (unsigned j = 0; j < 4; j++)
               t.zctab16[pass][at*4+j] = t.sctab16[pass][pc_attr*4+j];
         else /* if (temp.obpp == 32) */
            for (unsigned j = 0; j < 2; j++)
               t.zctab32[pass][at+0x100*j] = t.sctab32[pass][pc_attr+0x100*j];
      }
   }
}

void calc_noflic_16_32()
{
   unsigned at, pass;
   if (temp.obpp == 16) {
      for (pass = 0; pass < 2; pass++) {
         for (at = 0; at < 2*0x100; at++)
            t.sctab16d_nf[pass][at] = (t.sctab16d[pass][at] & temp.shift_mask)/2;
         for (at = 0; at < 4*0x100; at++)
            t.sctab16_nf[pass][at] = (t.sctab16[pass][at] & temp.shift_mask)/2;
         for (at = 0; at < 2*0x100; at++)
            t.p4bpp16_nf[pass][at] = (t.p4bpp16[pass][at] & temp.shift_mask)/2;
      }
   }
   if (temp.obpp == 32) {
      unsigned shift_mask = 0xFEFEFEFE;
      for (pass = 0; pass < 2; pass++) {
         for (at = 0; at < 2*0x100; at++)
            t.sctab32_nf[pass][at] = (t.sctab32[pass][at] & shift_mask)/2;
         for (at = 0; at < 2*0x100; at++)
            t.p4bpp32_nf[pass][at] = (t.p4bpp32[pass][at] & shift_mask)/2;
      }
   }
}

// pal.index => raw video data, shadowed with current scanline pass
unsigned raw_data(unsigned index, unsigned pass, unsigned bpp)
{
   if (bpp == 8)
   {

      if (pass)
      {
         if (!conf.scanbright)
             return 0;
         // palette too small to realize noflic/atari with shaded scanlines
         if (conf.scanbright < 100 && !conf.noflic && !conf.atariset[0])
         {
            if (temp.rflags & RF_PALB)
                index = (index & (index << 1) & 0x92) | ((index ^ 0xFF) & (index >> 1) & 0x49);
            else
                index &= 0x0F;
         }
      }
      return index * 0x01010101;
   }

   unsigned r = pal0[index].peRed, g = pal0[index].peGreen, b = pal0[index].peBlue;
   if (pass)
   {
       r = r * conf.scanbright / 100;
       g = g * conf.scanbright / 100;
       b = b * conf.scanbright / 100;
   }

   if (bpp == 32)
       return WORD4(b,g,r,0);

   // else (bpp == 16)
   if (temp.hi15==0)
       return ((b/8) + ((g/4)<<5) + ((r/8)<<11)) * 0x10001;
   if (temp.hi15==1)
       return ((b/8) + ((g/8)<<5) + ((r/8)<<10)) * 0x10001;
   if (temp.hi15==2)
       return getYUY2(r,g,b);
   return 0;
}

unsigned atari_to_raw(unsigned at, unsigned pass)
{
   unsigned c1 = at/0x10, c2 = at & 0x0F;
   unsigned raw0 = raw_data(t.attrtab[c1+0x100], pass, temp.obpp);
   unsigned raw1 = raw_data(t.attrtab[c2+0x100], pass, temp.obpp);
   if (raw0 == raw1) return raw1;

   if (temp.obpp == 8)
      return (temp.rflags & RF_PALB)? (0x49494949 & ((raw0&raw1)^((raw0^raw1)>>1))) |
                                      (0x92929292 & ((raw0&raw1)|((raw0|raw1)&((raw0&raw1)<<1))))
                                    : (0x0F0F0F0F & raw0) | (0xF0F0F0F0 & raw1);

   return (raw0 & temp.shift_mask)/2 + (raw1 & temp.shift_mask)/2;
}

void pixel_tables()
{
   attr_tables();
   for (unsigned pass = 0; pass < 2; pass++)
   {
      for (unsigned at = 0; at < 0x100; at++)
      {
         unsigned px0 = t.attrtab[at];
         unsigned px1 = t.attrtab[at+0x100];
         unsigned p0 = raw_data(px0, pass, temp.obpp);
         unsigned p1 = raw_data(px1, pass, temp.obpp);

         // sctab32 required for frame resampler in 16-bit mode, so temp.obpp=16 here
         t.sctab32[pass][at] = raw_data(px0, pass, 32);
         t.sctab32[pass][at+0x100] = raw_data(px1, pass, 32);

         // 8 bit
         unsigned j;
         for (j = 0; j < 0x10; j++)
         {
            unsigned mask = (j >> 3)*0xFF + (j & 0x04)*(0xFF00/4) +
                            (j & 0x02)*(0xFF0000/2) + (j & 1)*0xFF000000;
            t.sctab8[pass][j + at*0x10] = (mask & p1) + (~mask & p0);
         }
         for (j = 0; j < 4; j++)
         {
            unsigned mask = (j >> 1)*0xFFFF + (j & 1)*0xFFFF0000;
            t.sctab8d[pass][j+at*4] = (mask & p1) + (~mask & p0);
         }
         t.sctab8q[at] = p0, t.sctab8q[at+0x100] = p1;

         // 16 bit
         for (j = 0; j < 4; j++)
         {
            unsigned mask = (j >> 1)*0xFFFF + (j & 1)*0xFFFF0000;
            t.sctab16[pass][j+at*4] = (mask & p1) + (~mask & p0);
         }
         t.sctab16d[pass][at] = p0, t.sctab16d[pass][at+0x100] = p1;

         unsigned atarimode;
         if (!(temp.rflags & RF_MON) && (atarimode = temp.ataricolors[at]))
         {
            unsigned rawdata[4], i;
            for (i = 0; i < 4; i++) rawdata[i] = atari_to_raw((atarimode >> (8*i)) & 0xFF, pass);
            for (i = 0; i < 16; i++) t.sctab8[pass][at*0x10+i] = rawdata[i/4] + 16*rawdata[i & 3];
            for (i = 0; i < 4; i++) t.sctab8d[pass][at*4+i] = rawdata[i];
            for (i = 0; i < 4; i++) t.sctab16[pass][at*4+i] = rawdata[i];

         }
      }
   }

   p4bpp_tables(); // used for ATM2+ mode0 and Pentagon-4bpp

   if (temp.obpp > 8 && conf.noflic) calc_noflic_16_32();

   if ((temp.rflags & (RF_DRIVER|RF_2X|RF_USEFONT))==(RF_DRIVER|RF_2X) && // render="double"
       (conf.mem_model == MM_ATM450 || conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_PROFI))
      hires_sc_tables();
}

void video_color_tables()
{
   temp.shift_mask = 0xFEFEFEFE; // 32bit, 16bit YUY2
   if (temp.obpp == 16 && temp.hi15==0) temp.shift_mask = 0xF7DEF7DE;
   if (temp.obpp == 16 && temp.hi15==1) temp.shift_mask = 0x7BDE7BDE;

   create_palette();
   pixel_tables();
   make_colortab(0);

   if (temp.rflags & (RF_USEC32 | RF_USE32AS16)) {
      for (unsigned at = 0; at < 0x100; at++) {
         for (unsigned vl = 0; vl <= 0x10; vl++) {
            unsigned br = (at & 0x40) ? 0xFF : 0xBF;
            unsigned c1, c2, res;
            c1 = (at & 1) >> 0, c2 = (at & 0x08) >> 3;
            unsigned b = (c1*vl + c2*(0x10-vl))*br/0x10;
            c1 = (at & 2) >> 1, c2 = (at & 0x10) >> 4;
            unsigned r = (c1*vl + c2*(0x10-vl))*br/0x10;
            c1 = (at & 4) >> 2, c2 = (at & 0x20) >> 5;
            unsigned g = (c1*vl + c2*(0x10-vl))*br/0x10;
            if (temp.rflags & RF_USE32AS16) {
               if (temp.hi15==0) res = (b/8) + ((g/4)<<5) + ((r/8)<<11);
               if (temp.hi15==1) res = (b/8) + ((g/8)<<5) + ((r/8)<<10);
               if (temp.hi15==2) res = getYUY2(r,g,b);
               else res *= 0x10001; // for hi15=0,1
            } else res =  WORD4(b,g,r,0);
            t.c32tab[at][vl] = res;
         }
      }
   }
   setpal(0);
}

void video_timing_tables()
{
   if (conf.frame < 2000)
   {
       conf.frame = 2000;
       cpu.SetTpi(conf.frame);
   }
   if (conf.t_line < 128) conf.t_line = 128;
   conf.nopaper &= 1;
   atrtab = (comp.pEFF7 & EFF7_HWMC) ? t.atrtab_hwmc : t.atrtab;

//   conf.bordersize=2;
//   temp.scx = 384, temp.scy = 300;

   unsigned width = temp.scx/4;
   //temp.vidbufsize = temp.scx*temp.scy/4;

   // make video table
   temp.b_bottom = temp.b_top = 24, temp.b_left = 32;
   unsigned mid_lines = 192, buf_mid = 256;

   if (conf.bordersize==0) temp.b_top = temp.b_left = 0;
   if (conf.bordersize==2) temp.b_top = temp.b_left = 64;

   temp.b_right = temp.scx - buf_mid - temp.b_left;
   temp.b_bottom = temp.scy - mid_lines - temp.b_top;

   if (conf.nopaper) temp.b_bottom += mid_lines, mid_lines=0;
   int inx = 0;

   unsigned i;
   #define ts(t) (((int)(t) < 0) ? 0 : t)
   unsigned t = conf.paper - temp.b_top*conf.t_line;
   video[inx++].next_t = ts(t - temp.b_left/2);
   for (i = 0; i < temp.b_top; i++)
   { // top border
      video[inx].next_t = ts(t + (buf_mid+temp.b_right)/2);
      video[inx].screen_ptr = rbuf+width*i;
      video[inx].nextvmode = 0;
      inx++; t += conf.t_line;
      video[inx++].next_t = ts(t - temp.b_left/2);
   }
   for (i = 0; i < mid_lines; i++)
   { // screen+border
      video[inx].next_t = ts(t);
      video[inx].screen_ptr = rbuf+width*(i+temp.b_top);
      video[inx].nextvmode = 2; inx++;
      video[inx].next_t = ts(t + buf_mid/2);
      video[inx].screen_ptr = rbuf+width*(i+temp.b_top)+temp.b_left/4;
      video[inx].scr_offs = ::t.scrtab[i];
      video[inx].atr_offs = atrtab[i];
      inx++;
      video[inx].next_t = ts(t + (buf_mid+temp.b_right)/2);
      video[inx].screen_ptr = rbuf+width*(i+temp.b_top)+(buf_mid+temp.b_left)/4;
      video[inx].nextvmode = 0;
      inx++; t += conf.t_line;
      video[inx++].next_t = ts(t - temp.b_left/2);
   }
   for (i = 0; i < temp.b_bottom; i++)
   { // bottom border
      video[inx].next_t = ts(t + (buf_mid+temp.b_right)/2);
      video[inx].screen_ptr = rbuf+width*(i+temp.b_top+mid_lines);
      video[inx].nextvmode = 0;
      inx++; t += conf.t_line;
      video[inx++].next_t = ts(t - temp.b_left/2);
   }
   video[inx-1].next_t = 0x7FFFFFFF;

   temp.evenM1_C0 = conf.even_M1 ? 0xC0 : 0x00;
   temp.border_add = conf.border_4T ? 6 : 0;
   temp.border_and = conf.border_4T ? 0xFFFFFFFC : 0xFFFFFFFF;

   for (i = 0; i < NUM_LEDS; i++)
   {
      unsigned z = *(&conf.led.ay + i);
      int x = (signed short)(z & 0xFFFF);
      int y = (signed short)(((z >> 16) & 0x7FFF) + ((z >> 15) & 0x8000));
      if (x < 0) x += width*8;
      if (y < 0) y += temp.scy;
      *(&temp.led.ay+i) = (z & 0x80000000) ? rbuf + ((x>>2)&0xFE) + y*width : 0;
   }

   if (temp.rflags & RF_USEFONT)
       create_font_tables();

   needclr = 2;
}

void set_video()
{
   set_vidmode();
   video_color_tables();
}

void apply_video()
{
   load_ula_preset();
   temp.rflags = renders[conf.render].flags;
   if (conf.use_comp_pal && (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_ATM450 || conf.mem_model == MM_PROFI))
   {
      temp.rflags |= RF_COMPPAL | RF_PALB;
      // disable palette noflic, only if it is really used
      if (temp.obpp == 8 && (temp.rflags & (RF_DRIVER | RF_USEFONT | RF_8BPCH)) == RF_DRIVER)
      conf.noflic = 0;
   }

   if (renders[conf.render].func == render_rsm)
       conf.flip = 1; // todo: revert back after returning from frame resampler //Alone Coder

   if (renders[conf.render].func == render_advmame)
   {
      if (conf.videoscale == 2)
          temp.rflags |= RF_2X;
      if (conf.videoscale == 3)
          temp.rflags |= RF_3X;
      if (conf.videoscale == 4)
          temp.rflags |= RF_4X;
   } //Alone Coder

   set_video();
   calc_rsm_tables();
   video_timing_tables();
}

__inline unsigned char *raypointer()
{
   if (prev_t > conf.frame)
       return rbuf + rb2_offs;
   if (!vmode)
       return vcurr[1].screen_ptr;
   unsigned offs = (prev_t - vcurr[-1].next_t) / 4;
   return vcurr->screen_ptr + (offs+1) * 2;
}

__inline void clear_until_ray()
{
   unsigned char *dst = raypointer();
   while (dst < rbuf + rb2_offs) *dst++ = 0, *dst++ = 0x55;
}

void paint_scr(char alt) // alt=0/1 - main/alt screen, alt=2 - ray-painted
{
   if (alt == 2) {
      update_screen();
      clear_until_ray();
   } else {
      if (alt) comp.p7FFD ^= 8, set_banks();
      draw_screen();
      if (alt) comp.p7FFD ^= 8, set_banks();
   }
}

// ¬ызываетс€ при записи в видеопам€ть нового значени€
void update_screen()
{
   unsigned last_t = (cpu.t + temp.border_add) & temp.border_and;
   unsigned t = prev_t;
   if (t >= last_t)
       return;

   unsigned char b = comp.border_attr;
   b = (b<<4)+b;

   if (vmode == 1)
       goto mode1;
   if (vmode == 2)
       goto mode2;

mode0: // not visible
   {
      vmode = 1;
      t = vcurr->next_t;
      vcurr++;
      if (t >= last_t)
      {
      done:
          prev_t = t;
          return;
      }
   }
mode1: // border
   {
      unsigned offs = (t - vcurr[-1].next_t);
      unsigned char *ptr = vcurr->screen_ptr + offs/2;
      ptr = (unsigned char*)(ULONG_PTR(ptr) & ~ULONG_PTR(1));
      if (offs & 3)
      {
         *ptr++ = ((unsigned)0xFF00 >> ((offs & 3)*2));
         t += 4 -(offs & 3);
         *ptr = (*ptr & 0x0F) + (b & 0xF0);
         ptr++;
      }
      unsigned end = min(vcurr->next_t, last_t);
      for (; t < end; t+=4)
      {
         *(volatile unsigned char*)ptr;
         *ptr++ = 0; *ptr++ = b;
      }
      t = end;
      if (t == vcurr->next_t)
      {
          vmode = vcurr->nextvmode;
          vcurr++;
      }
      if (t == last_t)
          goto done;
      if (!vmode)
          goto mode0;
   }
mode2: // screen
   {
      unsigned offs = (t - vcurr[-1].next_t)/4;
      unsigned char *scr = temp.base + vcurr->scr_offs + offs;
      unsigned char *atr = temp.base + vcurr->atr_offs + offs;
      unsigned char *ptr = vcurr->screen_ptr + offs*2;
      unsigned end = min(last_t, vcurr->next_t);
      for (int i = 0; t < end; t += 4, i++)
      {
         *(volatile unsigned char*)(ptr+2*i);
         ptr[2*i] = scr[i];
         ptr[2*i+1] = colortab[atr[i]];
      }
      t = end;
      if (t == vcurr->next_t)
      {
          vmode = 1;
          vcurr++;
      }
      if (t == last_t)
          goto done;
      goto mode1;
   }
}

void init_frame()
{
   // recreate colors with flash attribute
   unsigned char frame = (unsigned char)comp.frame_counter;
   if (!(frame & 15) /* && !conf.flashcolor */ )
       make_colortab(frame & 16);

   prev_t = -1; // block MCR
   temp.base_2 = 0; // block paper trace

   if (temp.vidblock)
       return;

/* [vv] ќтключен, т.к. этот бит использетс€ дл€ DDp scroll
   // AlCo384 - no border/paper rendering
   if (comp.pEFF7 & EFF7_384)
       return;
*/
   // GIGASCREEN - no paper rendering
//   if (comp.pEFF7 & EFF7_GIGASCREEN) goto allow_border; //Alone Coder

   // disable multicolors, border still works
   if ((temp.rflags & RF_BORDER) || // chunk/etc filter
       (conf.mem_model == MM_PROFI && (comp.pDFFD & 0x80)) ||   // profi hires screen
       ((conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3)&& (comp.pFF77 & 7) != 3) ||  // ATM-2 hires screen
       (conf.mem_model == MM_ATM450 && (comp.aFE & 0x60) != 0x60)) // ATM-1 hires screen
   {
       if ((conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3))
       {
           // ATM2, один из расширенных видеорежимов
           AtmVideoCtrl.PrepareFrameATM2(comp.pFF77 & 7);
       }

       if (conf.mem_model == MM_ATM450)
       {
           // ATM1, один из расширенных видеорежимов
           AtmVideoCtrl.PrepareFrameATM1( (comp.aFE >> 5) & 3 );

       }
       
      // if border update disabled, dont show anything on zx-screen
      if (!conf.updateb)
          return;
   }

   // paper + border
   temp.base_2 = temp.base;
//allow_border:
   prev_t = vmode = 0;
   vcurr = video;
}

void flush_frame()
{
   if (temp.vidblock)
       return;
   if (prev_t != -1)
   { // MCR on
      if (prev_t)
      {  // paint until end of frame
         // paint until screen bottom, even if n_lines*t_line < cpu.t (=t_frame)
         unsigned t = cpu.t;
         cpu.t = 0x7FFF0000;
         update_screen();
         cpu.t = t;
//         if (comp.pEFF7 & EFF7_GIGASCREEN) draw_gigascreen_no_border(); //Alone Coder
      }
      else
      { // MCR on, but no screen updates in last frame - use fast painter
         if (temp.base_2 /*|| (comp.pEFF7 & EFF7_GIGASCREEN)*/ /*Alone Coder*/)
             draw_screen();
         else
             draw_border();
      }
      return;
   }
   if (comp.pEFF7 & EFF7_384)
       draw_alco();
}

void load_spec_colors()
{
   // spectrum colors -> palette indexes (RF_PALB - gg0rr0bb format)
   static unsigned char comp_pal[16] =
      { 0x00, 0x02, 0x10, 0x12, 0x80, 0x82, 0x90, 0x92,
        0x00, 0x03, 0x18, 0x1B, 0xC0, 0xC3, 0xD8, 0xDB };
   memcpy(comp.comp_pal, comp_pal, sizeof comp.comp_pal);
   temp.comp_pal_changed = 1;
}
