#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "drawers.h"
#include "dx.h"
#include "config.h"
#include "util.h"
#include "leds.h"

const RASTER raster[R_MAX] = {
	{ R_256_192, 80, 272, 70, 70+128, 198 },
	//{ R_256_192, 80, 272, 58, 186, 198 },
	{ R_320_200, 76, 276, 54, 214, 214 },
	{ R_320_240, 56, 296, 54, 214, 214 },
	{ R_360_288, 32, 320, 44, 224, 0 },
	{ R_384_304, 16, 320, 32, 224, 0 },
	{ R_512_240, 56, 296, 70, 198, 0 },
};

// Default color table: 0RRrrrGG gggBBbbb
u16 spec_colors[16] = {
	0x0000,
	0x0010,
	0x4000,
	0x4010,
	0x0200,
	0x0210,
	0x4200,
	0x4210,
	0x0000,
	0x0018,
	0x6000,
	0x6018,
	0x0300,
	0x0318,
	0x6300,
	0x6318
};

#ifdef CACHE_ALIGNED
CACHE_ALIGNED u8 rbuf[sizeof_rbuf];
#else // __declspec(align) not available, force u64 align with old method
__int64 rbuf__[sizeof_rbuf/sizeof(__int64)];
u8 * const rbuf = (u8*)rbuf__;
#endif

CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];
VCTR vid;

u8 * const rbuf_s = rbuf + rb2_offs; // frames to mix with noflic and resampler filters
u8 * const save_buf = rbuf_s + rb2_offs*MAX_BUFFERS; // used in monitor

T t;

videopoint *vcurr;
videopoint video[4*MAX_HEIGHT];
unsigned vmode;  // what are drawing: 0-not visible, 1-border, 2-screen
unsigned prev_t; // last drawn pixel
unsigned *atrtab;

u8 colortab[0x100];// map zx attributes to pc attributes
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
            // смещения в текстовом видеорежиме
            Scanlines[y].Offset = 64*(y/8);
        } else {
            // смещения в растровом видеорежиме
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

   palette_options *pl = &pals[conf.pal];
   u8 brights[4] = { pl->ZZ, pl->ZN, pl->NN, pl->BB };
   u8 brtab[16] =
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
      u8 ink = a & 7;
      u8 paper = (a >> 3) & 7;
      u8 bright = (a >> 6) & 1;
      u8 flash = (a >> 7) & 1;

      if (ink)
          ink |= bright << 3; // no bright for 0th color

      if (paper)
          paper |= (conf.flashcolor ? flash : bright) << 3; // no bright for 0th color

      if (flash_active && flash)
      {
          u8 t = ink;
          ink = paper;
          paper = t;
      }

      const u8 color = (paper << 4) | ink;

      colortab[a] = color;
      colortab_s8[a] = color << 8;
      colortab_s24[a] = color << 24;
   }
#if 0
   if (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_ATM450)
       atm_zc_tables(); // update with new flash bit
#endif
}
#if 0
// make attrtab: pc-attr + 0x100*pixel -> palette index
void attr_tables()
{
   u8 flashcolor = (temp.rflags & RF_MON)? 0 : conf.flashcolor;
   for (unsigned a = 0; a < 0x100; a++)
   {
      u8 ink = (a & 0x0F), paper = (a >> 4);
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
         u8 b0,b1, r0,r1, g0,g1;
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

         u8 br1 = (ink >> 3) & 1;
         if (r1) r1 += br1;
         if (g1) g1 += br1;
         if (b1) b1 += br1;

         u8 br0 = (paper >> 3) & 1;
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
#endif
void set_video()
{
   set_vidmode();
   //video_color_tables();
}

void apply_video()
{
   conf.framex = bordersizes[conf.bordersize].x;
   conf.framexsize = bordersizes[conf.bordersize].xsize;
   conf.framey = bordersizes[conf.bordersize].y;
   conf.frameysize = bordersizes[conf.bordersize].ysize;

   load_ula_preset();
   temp.rflags = renders[conf.render].flags;
   if (conf.use_comp_pal && (conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3 || conf.mem_model == MM_ATM450 || conf.mem_model == MM_PROFI))
   {
      temp.rflags |= RF_COMPPAL | RF_PALB;
      // disable palette noflic, only if it is really used
      if (temp.obpp == 8 && (temp.rflags & (RF_DRIVER | RF_USEFONT | RF_8BPCH)) == RF_DRIVER)
      conf.noflic = 0;
   }

   set_video();
//   video_timing_tables();

   // Console info about selected video option
   //printf("\n");
   //color(CONSCLR_HARDINFO);
   //printf("Display: %ux%u\n", conf.framexsize, conf.frameysize);
   //printf("Render: %s\n", renders[conf.render].name);
   //printf("Driver: %s\n", drivers[conf.driver].name);
   //printf("Bits per pixel: %u\n", temp.obpp);
   //printf("Window dimensions: %ux%u\n", temp.ox, temp.oy);
}

__inline u8 *raypointer()
{
//   if (prev_t > conf.frame)
//       return rbuf + rb2_offs;
//   if (!vmode)
//       return vcurr[1].screen_ptr;
//   unsigned offs = (prev_t - vcurr[-1].next_t) / 4;
//   return vcurr->screen_ptr + (offs+1) * 2;
}

__inline void clear_until_ray()
{
//   u8 *dst = raypointer();
//   while (dst < rbuf + rb2_offs) *dst++ = 0, *dst++ = 0x55;
}

void paint_scr(char alt) // alt=0/1 - main/alt screen, alt=2 - ray-painted
{
   /*if (alt == 2) {
      clear_until_ray();
   } else {
	   // !!! here need to handle comp.ts.vpage somehow, now it's unhandled
      if (alt) comp.p7FFD ^= 8, set_banks();
		draw_screen();
      if (alt) comp.p7FFD ^= 8, set_banks();
   }*/
}

u32 get_free_memcycles(int dram_t)
{
  if (vid.memcyc_lcmd >= dram_t)
    return 0;

  u32 memcycles = vid.memcpucyc[vid.line] + vid.memvidcyc[vid.line] + vid.memtstcyc[vid.line] + vid.memtsscyc[vid.line] + vid.memdmacyc[vid.line];
  
  if (memcycles >= MEM_CYCLES)
    return 0;

  u32 free_t = dram_t - vid.memcyc_lcmd;
  free_t = min(free_t, MEM_CYCLES - memcycles);

  return free_t;
}

void update_screen()
{
  // Get tact of cpu state in current frame
  u32 cput = (cpu.t>= conf.frame) ? (VID_TACTS * VID_LINES) : cpu.t;

  while (vid.t_next < cput)
  {
    // Calculate tacts for drawing in current video line
    int n = min(cput - vid.t_next, (u32)VID_TACTS - vid.line_pos);
    int dram_t = n << 1;

    // Start of new video line
    if (!vid.line_pos)
    {
      if (comp.ts.vconf != comp.ts.vconf_d)
      {
        comp.ts.vconf = comp.ts.vconf_d;
        init_raster();
      }

      comp.ts.g_xoffs = comp.ts.g_xoffs_d;  // GFX X offset
      comp.ts.vpage   = comp.ts.vpage_d;    // Video Page
      comp.ts.palsel  = comp.ts.palsel_d;   // Palette Selector

      comp.ts.t0gpage[2] = comp.ts.t0gpage[1];
      comp.ts.t0gpage[1] = comp.ts.t0gpage[0];
      comp.ts.t1gpage[2] = comp.ts.t1gpage[1];
      comp.ts.t1gpage[1] = comp.ts.t1gpage[0];
      comp.ts.t0_xoffs_d = comp.ts.t0_xoffs;
      comp.ts.t1_xoffs_d = comp.ts.t1_xoffs;

      vid.ts_pos = 0;

      // set new task for tsu
      comp.ts.tsu.state = TSS_INIT;
    }

    // Render upper and bottom border
    if (vid.line < vid.raster.u_brd || vid.line >= vid.raster.d_brd)
    {
      draw_border(n);
      vid.line_pos += n;
    }
    else
    {
      // Start of new video line
      if (!vid.line_pos)
      {
        vid.xctr = 0; // clear X video counter
        vid.yctr++;   // increment Y video counter

        if (!comp.ts.g_yoffs_updated) // was Y-offset updated?
        { // no - just increment old
          vid.ygctr++;
          vid.ygctr &= 0x1FF;
        }
        else
        { // yes - reload Y-offset
          vid.ygctr = comp.ts.g_yoffs;
          comp.ts.g_yoffs_updated = 0;
        }
      }

      // Render left border
      if (vid.line_pos < vid.raster.l_brd)
      {
        u32 m = min((u32)n, vid.raster.l_brd - vid.line_pos);
        draw_border(m); n -= m;
        vid.line_pos += (u16)m;
      }
      // Render pixel graphics
      if (n > 0 && vid.line_pos < vid.raster.r_brd)
      {
        u32 m = min((u32)n, vid.raster.r_brd - vid.line_pos);
        u32 t = vid.t_next; // store tact of video controller
        u32 vptr = vid.vptr;
        drawers[vid.mode].func(m);
        if (conf.mem_model == MM_TSL) draw_ts(vptr);
        t = vid.t_next - t; // calculate tacts used by drawers func
        n -= t; vid.line_pos += (u16)t;
      }
      // Render right border
      if (n > 0)
      {
        u32 m = min(n, VID_TACTS - vid.line_pos);
        draw_border(m); n -= m;
        vid.line_pos += (u16)m;
      }
    }
    u32 free_t = get_free_memcycles(dram_t); // get free memcyc of last command
    free_t = render_ts(free_t);
    dma(free_t);

    // calculate busy tacts for the next line
    vid.memcyc_lcmd = (vid.memcyc_lcmd > dram_t) ? (vid.memcyc_lcmd - dram_t) : 0;

    // if line is full, then go to the next line
    if (vid.line_pos == VID_TACTS)
      vid.line_pos = 0, vid.line++;
  }
}

void update_raypos(bool showleds) {
    // update debug state
    vbuf[vid.buf][vid.vptr] ^= 0xFFFFFF;    // invert raypos
    if (conf.bordersize == 5)
        show_memcycles(0, vid.line);
}

void init_raster()
{
	// TSconf
	if (conf.mem_model == MM_TSL)
	{
		vid.raster = raster[comp.ts.rres];
		EnterCriticalSection(&tsu_toggle_cr); // wbcbz7 note: huhuhuhuhuhuh...dirty code :)
		if ((comp.ts.nogfx) || (!comp.ts.tsu.toggle.gfx)) { vid.mode = M_BRD; LeaveCriticalSection(&tsu_toggle_cr); return; }
		if (comp.ts.vmode == 0) { vid.mode = M_ZX; LeaveCriticalSection(&tsu_toggle_cr); return; }
		if (comp.ts.vmode == 1) { vid.mode = M_TS16; LeaveCriticalSection(&tsu_toggle_cr); return; }
		if (comp.ts.vmode == 2) { vid.mode = M_TS256; LeaveCriticalSection(&tsu_toggle_cr); return; }
		if (comp.ts.vmode == 3) { vid.mode = M_TSTX; LeaveCriticalSection(&tsu_toggle_cr); return; }
	}

	u8 m = EFF7_4BPP | EFF7_HWMC;

	// ATM 1
	if ((conf.mem_model == MM_ATM450) && (((comp.aFE >> 5) & 3) != FF77_ZX))
	{
		vid.raster = raster[R_320_200];
		if (((comp.aFE >> 5) & 3) == aFE_16) { vid.mode = M_ATM16; return; }
		if (((comp.aFE >> 5) & 3) == aFE_MC) { vid.mode = M_ATMHR; return; }
		vid.mode = M_NUL; return;
	}

	// ATM 2 & 3
	if ((conf.mem_model == MM_ATM710 || conf.mem_model == MM_ATM3) && ((comp.pFF77 & 7) != FF77_ZX))
	{
		vid.raster = raster[R_320_200];
		if (conf.mem_model == MM_ATM3 && (comp.pEFF7 & m)) { vid.mode = M_NUL; return; }	// EFF7 AlCo bits must be 00, or invalid mode
		if ((comp.pFF77 & 7) == FF77_16) { vid.mode = M_ATM16; return; }
		if ((comp.pFF77 & 7) == FF77_MC) { vid.mode = M_ATMHR; return; }
		if ((comp.pFF77 & 7) == FF77_TX) { vid.mode = M_ATMTX; return; }
		if (conf.mem_model == MM_ATM3 && (comp.pFF77 & 7) == FF77_TL) { vid.mode = M_ATMTL; return; }
		vid.mode = M_NUL; return;
	}

	vid.raster = raster[R_256_192];

	// ATM 3 AlCo modes
	if (conf.mem_model == MM_ATM3 && (comp.pEFF7 & m))
	{
		if ((comp.pEFF7 & m) == EFF7_4BPP) { vid.mode = M_P16; return; }
		if ((comp.pEFF7 & m) == EFF7_HWMC) { vid.mode = M_PMC; return; }
		vid.mode = M_NUL; return;
	}

	// Pentagon AlCo modes
	m = EFF7_4BPP | EFF7_512 | EFF7_384 | EFF7_HWMC;
	if (conf.mem_model == MM_PENTAGON && (comp.pEFF7 & m))
	{
		if ((comp.pEFF7 & m) == EFF7_4BPP) { vid.mode = M_P16; return; }
		if ((comp.pEFF7 & m) == EFF7_HWMC) { vid.mode = M_PMC; return; }
		if ((comp.pEFF7 & m) == EFF7_512) { vid.mode = M_PHR; return; }
		if ((comp.pEFF7 & m) == EFF7_384) { vid.raster = raster[R_384_304]; vid.mode = M_P384; return; }
		vid.mode = M_NUL; return;
	}

	if (conf.mem_model == MM_PROFI && (comp.pDFFD & 0x80))
	{
		vid.raster = raster[R_512_240];
		vid.mode = M_PROFI; return;
	}

	if (conf.mem_model == MM_GMX && (comp.p7EFD & 0x08))
	{
		vid.raster = raster[R_320_200];
		vid.mode = M_GMX; return;
	}

	// Sinclair
	vid.mode = M_ZX;
}

void init_frame()
{
   // draw on single buffer if noflic is not active
   if (conf.noflic) vid.buf ^= 1;

   switch (conf.ray_paint_mode) {
   case RAYDRAW_CLEAR:
       memset(vbuf[vid.buf], 0xFF000000, sizeof(u32)*sizeof_vbuf);          // alpha fix (doubt if it's really need)
       break;

   case RAYDRAW_DIM:
       {
           // TODO: rewirte with SSE2
           auto *p = vbuf[vid.buf];
           for (auto i = 0; i < sizeof_vbuf; i++) {
               *p = ((*p >> 2) & 0x3F3F3F) + ((0x808080 >> 2) & 0x3F3F3F) | 0xFF000000;
               p++;
           }
       }
       break;

       default:
           break;
   }


   vid.t_next = 0;
   vid.vptr = 0;
   vid.yctr = 0;
   vid.ygctr = comp.ts.g_yoffs - 1;
   vid.line = 0;
   vid.line_pos = 0;
   comp.ts.g_yoffs_updated = 0;
   vid.flash = comp.frame_counter & 0x10;
   init_raster();
   init_memcycles();
}

void load_spec_colors()
{
	for (int i=0xF0; i<0x100; i++)
	{
		comp.cram[i] = spec_colors[i-0xF0];
		update_clut(i);
	}
}
