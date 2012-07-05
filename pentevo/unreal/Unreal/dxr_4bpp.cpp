#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "dxrframe.h"
#include "dxr_4bpp.h"

// AlCo 4bpp mode
static const int p4bpp_ofs[] =
{
  0x00000000 - PAGE, 0x00004000 - PAGE, 0x00002000 - PAGE, 0x00006000 - PAGE,
  0x00000001 - PAGE, 0x00004001 - PAGE, 0x00002001 - PAGE, 0x00006001 - PAGE,
  0x00000002 - PAGE, 0x00004002 - PAGE, 0x00002002 - PAGE, 0x00006002 - PAGE,
  0x00000003 - PAGE, 0x00004003 - PAGE, 0x00002003 - PAGE, 0x00006003 - PAGE,
  0x00000004 - PAGE, 0x00004004 - PAGE, 0x00002004 - PAGE, 0x00006004 - PAGE,
  0x00000005 - PAGE, 0x00004005 - PAGE, 0x00002005 - PAGE, 0x00006005 - PAGE,
  0x00000006 - PAGE, 0x00004006 - PAGE, 0x00002006 - PAGE, 0x00006006 - PAGE,
  0x00000007 - PAGE, 0x00004007 - PAGE, 0x00002007 - PAGE, 0x00006007 - PAGE,
  0x00000008 - PAGE, 0x00004008 - PAGE, 0x00002008 - PAGE, 0x00006008 - PAGE,
  0x00000009 - PAGE, 0x00004009 - PAGE, 0x00002009 - PAGE, 0x00006009 - PAGE,
  0x0000000A - PAGE, 0x0000400A - PAGE, 0x0000200A - PAGE, 0x0000600A - PAGE,
  0x0000000B - PAGE, 0x0000400B - PAGE, 0x0000200B - PAGE, 0x0000600B - PAGE,
  0x0000000C - PAGE, 0x0000400C - PAGE, 0x0000200C - PAGE, 0x0000600C - PAGE,
  0x0000000D - PAGE, 0x0000400D - PAGE, 0x0000200D - PAGE, 0x0000600D - PAGE,
  0x0000000E - PAGE, 0x0000400E - PAGE, 0x0000200E - PAGE, 0x0000600E - PAGE,
  0x0000000F - PAGE, 0x0000400F - PAGE, 0x0000200F - PAGE, 0x0000600F - PAGE,
  0x00000010 - PAGE, 0x00004010 - PAGE, 0x00002010 - PAGE, 0x00006010 - PAGE,
  0x00000011 - PAGE, 0x00004011 - PAGE, 0x00002011 - PAGE, 0x00006011 - PAGE,
  0x00000012 - PAGE, 0x00004012 - PAGE, 0x00002012 - PAGE, 0x00006012 - PAGE,
  0x00000013 - PAGE, 0x00004013 - PAGE, 0x00002013 - PAGE, 0x00006013 - PAGE,
  0x00000014 - PAGE, 0x00004014 - PAGE, 0x00002014 - PAGE, 0x00006014 - PAGE,
  0x00000015 - PAGE, 0x00004015 - PAGE, 0x00002015 - PAGE, 0x00006015 - PAGE,
  0x00000016 - PAGE, 0x00004016 - PAGE, 0x00002016 - PAGE, 0x00006016 - PAGE,
  0x00000017 - PAGE, 0x00004017 - PAGE, 0x00002017 - PAGE, 0x00006017 - PAGE,
  0x00000018 - PAGE, 0x00004018 - PAGE, 0x00002018 - PAGE, 0x00006018 - PAGE,
  0x00000019 - PAGE, 0x00004019 - PAGE, 0x00002019 - PAGE, 0x00006019 - PAGE,
  0x0000001A - PAGE, 0x0000401A - PAGE, 0x0000201A - PAGE, 0x0000601A - PAGE,
  0x0000001B - PAGE, 0x0000401B - PAGE, 0x0000201B - PAGE, 0x0000601B - PAGE,
  0x0000001C - PAGE, 0x0000401C - PAGE, 0x0000201C - PAGE, 0x0000601C - PAGE,
  0x0000001D - PAGE, 0x0000401D - PAGE, 0x0000201D - PAGE, 0x0000601D - PAGE,
  0x0000001E - PAGE, 0x0000401E - PAGE, 0x0000201E - PAGE, 0x0000601E - PAGE,
  0x0000001F - PAGE, 0x0000401F - PAGE, 0x0000201F - PAGE, 0x0000601F - PAGE
};

#define p4bpp8_nf p4bpp8

static int buf4bpp_shift = 0;

static void line_p4bpp_8(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   u8 *d = (u8 *)dst;
   for (unsigned x = 0, i = 0; x < 256; x += 2, i++)
   {
       unsigned tmp = tab[src[p4bpp_ofs[(i+temp.offset_hscroll) & 0x7f]]];
       d[x]   = tmp;
       d[x+1] = tmp >> 16;
   }
}

static void line_p4bpp_8d(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   for (unsigned x = 0; x < 128; x++)
   {
       *(unsigned*)(dst) = tab[src[p4bpp_ofs[(x+temp.offset_hscroll) & 0x7f]]];
       dst+=4;
   }
}

static void line_p4bpp_8d_nf(unsigned char *dst, unsigned char *src1, unsigned char *src2, unsigned *tab)
{
   for (unsigned x = 0; x < 128; x++)
   {
       *(unsigned*)(dst) =
        (tab[src1[p4bpp_ofs[(x+temp.offset_hscroll     ) & 0x7f]]] & 0x0F0F0F0F) +
        (tab[src2[p4bpp_ofs[(x+temp.offset_hscroll_prev) & 0x7f]]] & 0xF0F0F0F0);
       dst+=4;
   }
}

static void line_p4bpp_16(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   u16 *d = (u16 *)dst;
   for (unsigned x = 0, i = 0; x < 256; x +=2, i++)
   {
       unsigned tmp = 2*src[p4bpp_ofs[(i+temp.offset_hscroll) & 0x7f]];
       d[x]   = tab[0+tmp];
       d[x+1] = tab[1+tmp];
   }
}

static void line_p4bpp_16d(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   unsigned tmp;
   for (unsigned x = 0; x < 128; x++)
   {
       tmp = 2*src[p4bpp_ofs[(x+temp.offset_hscroll) & 0x7f]];
       *(unsigned*)dst = tab[0+tmp];
       dst+=4;
       *(unsigned*)dst = tab[1+tmp];
       dst+=4;
   }
}

static void line_p4bpp_16d_nf(unsigned char *dst, unsigned char *src1, unsigned char *src2, unsigned *tab)
{
   unsigned tmp1;
   unsigned tmp2;
   for (unsigned x = 0; x < 128; x++)
   {
       tmp1 = p4bpp_ofs[(x+temp.offset_hscroll     ) & 0x7f];
       tmp2 = p4bpp_ofs[(x+temp.offset_hscroll_prev) & 0x7f];
       *(unsigned*)dst = tab[0+2*src1[tmp1]] + tab[0+2*src2[tmp2]];
       dst+=4;
       *(unsigned*)dst = tab[1+2*src1[tmp1]] + tab[1+2*src2[tmp2]];
       dst+=4;
   }
}

static void line_p4bpp_32(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   u32 *d = (u32 *)dst;
   for (unsigned x = 0, i = 0; x < 256; x += 2, i++)
   {
       unsigned tmp = 2*src[p4bpp_ofs[(i+temp.offset_hscroll) & 0x7f]];
       d[x]   = tab[0+tmp];
       d[x+1] = tab[1+tmp];
   }
}

static void line_p4bpp_32d(unsigned char *dst, unsigned char *src, unsigned *tab)
{
   unsigned tmp;
   for (unsigned x = 0; x < 128; x++)
   {
       tmp = 2*src[p4bpp_ofs[(x+temp.offset_hscroll) & 0x7f]];
       *(unsigned*)dst = *(unsigned*)(dst+4) = tab[0+tmp];
       dst+=8;
       *(unsigned*)dst = *(unsigned*)(dst+4) = tab[1+tmp];
       dst+=8;
   }
}

static void line_p4bpp_32d_nf(unsigned char *dst, unsigned char *src1, unsigned char *src2, unsigned *tab)
{
   unsigned tmp1;
   unsigned tmp2;
   for (unsigned x = 0; x < 128; x++)
   {
       tmp1 = p4bpp_ofs[(x+temp.offset_hscroll     ) & 0x7f];
       tmp2 = p4bpp_ofs[(x+temp.offset_hscroll_prev) & 0x7f];
       *(unsigned*)dst = *(unsigned*)(dst+4) = tab[0+2*src1[tmp1]] + tab[0+2*src2[tmp2]];
       dst+=8;
       *(unsigned*)dst = *(unsigned*)(dst+4) = tab[1+2*src1[tmp1]] + tab[1+2*src2[tmp2]];
       dst+=8;
   }
}

static void r_p4bpp_8(unsigned char *dst, unsigned pitch)
{
    for (unsigned y = 0; y < 192; y++)
    {
       unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
       line_p4bpp_8(dst, src, t.p4bpp8[0]); dst += pitch;
    }
}

static void r_p4bpp_8d1(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_8d_nf(dst, src1, src2, t.p4bpp8_nf[0]); dst += pitch;
      }

   }
   else
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_8d(dst, src, t.p4bpp8[0]); dst += pitch;
      }
   }
}

static void r_p4bpp_8d(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {

      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_8d_nf(dst, src1, src2, t.p4bpp8_nf[0]); dst += pitch;
         line_p4bpp_8d_nf(dst, src1, src2, t.p4bpp8_nf[1]); dst += pitch;
      }

   }
   else
   {

      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_8d(dst, src, t.p4bpp8[0]); dst += pitch;
         line_p4bpp_8d(dst, src, t.p4bpp8[1]); dst += pitch;
      }

   }
}

static void r_p4bpp_16(unsigned char *dst, unsigned pitch)
{
    for (unsigned y = 0; y < 192; y++)
    {
       unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
       line_p4bpp_16(dst, src, t.p4bpp16[0]);
       dst += pitch;
    }
}

static void r_p4bpp_16d1(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_16d_nf(dst, src1, src2, t.p4bpp16_nf[0]); dst += pitch;
      }

   }
   else
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_16d(dst, src, t.p4bpp16[0]); dst += pitch;
      }

   }
}

static void r_p4bpp_16d(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_16d_nf(dst, src1, src2, t.p4bpp16_nf[0]); dst += pitch;
         line_p4bpp_16d_nf(dst, src1, src2, t.p4bpp16_nf[1]); dst += pitch;
      }

   }
   else
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_16d(dst, src, t.p4bpp16[0]); dst += pitch;
         line_p4bpp_16d(dst, src, t.p4bpp16[1]); dst += pitch;
      }
   }
}

static void r_p4bpp_32(unsigned char *dst, unsigned pitch)
{
    for (unsigned y = 0; y < 192; y++)
    {
       unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
       line_p4bpp_32(dst, src, t.p4bpp32[0]);
       dst += pitch;
    }
}

static void r_p4bpp_32d1(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_32d_nf(dst, src1, src2, t.p4bpp32_nf[0]); dst += pitch;
      }

   }
   else
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_32d(dst, src, t.p4bpp32[0]); dst += pitch;
      }
   }
}

static void r_p4bpp_32d(unsigned char *dst, unsigned pitch)
{
   if (conf.noflic)
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src1 = temp.base                 + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         unsigned char *src2 = temp.base + buf4bpp_shift + t.scrtab[(unsigned char)(y+temp.offset_vscroll_prev)];
         line_p4bpp_32d_nf(dst, src1, src2, t.p4bpp32_nf[0]); dst += pitch;
         line_p4bpp_32d_nf(dst, src1, src2, t.p4bpp32_nf[1]); dst += pitch;
      }
   }
   else
   {
      for (unsigned y = 0; y < 192; y++)
      {
         unsigned char *src = temp.base + t.scrtab[(unsigned char)(y+temp.offset_vscroll)];
         line_p4bpp_32d(dst, src, t.p4bpp32[0]); dst += pitch;
         line_p4bpp_32d(dst, src, t.p4bpp32[1]); dst += pitch;
      }
   }
}

void rend_p4bpp_small(unsigned char *dst, unsigned pitch)
{
   unsigned char *dst2 = dst + (temp.ox-256)*temp.obpp/16;
   dst2 += temp.b_top * pitch * ((temp.oy > temp.scy)?2:1);

   if (temp.oy > temp.scy && conf.fast_sl)
       pitch *= 2;

   if (conf.noflic)
       buf4bpp_shift = (rbuf_s+PAGE) - temp.base;

   if (temp.obpp == 8) 
   {
       rend_frame8(dst, pitch);
       r_p4bpp_8(dst2, pitch);
   }
   if (temp.obpp == 16)
   {
       rend_frame16(dst, pitch);
       r_p4bpp_16(dst2, pitch);
   }
   if (temp.obpp == 32)
   {
       rend_frame32(dst, pitch);
       r_p4bpp_32(dst2, pitch);
   }

   if (conf.noflic)
       memcpy(rbuf_s, temp.base-PAGE, 2*PAGE);

   temp.offset_vscroll_prev = temp.offset_vscroll;
   temp.offset_vscroll = 0;
   temp.offset_hscroll_prev = temp.offset_hscroll;
   temp.offset_hscroll = 0;
}

void rend_p4bpp(unsigned char *dst, unsigned pitch)
{
   unsigned char *dst2 = dst + (temp.ox-512)*temp.obpp/16;
   dst2 += temp.b_top * pitch * ((temp.oy > temp.scy)?2:1);

   if (temp.oy > temp.scy && conf.fast_sl) pitch *= 2;

   if (conf.noflic)
       buf4bpp_shift = (rbuf_s+PAGE) - temp.base;

   if (temp.obpp == 8) 
   {
       if (conf.fast_sl)
       {
           rend_frame_8d1(dst, pitch);
           r_p4bpp_8d1(dst2, pitch);
       }
       else
       {
           rend_frame_8d(dst, pitch);
           r_p4bpp_8d(dst2, pitch);
       }
   }
   if (temp.obpp == 16)
   {
       if (conf.fast_sl)
       {
           rend_frame_16d1(dst, pitch);
           r_p4bpp_16d1(dst2, pitch);
       }
       else
       {
           rend_frame_16d(dst, pitch);
           r_p4bpp_16d(dst2, pitch);
       }
   }
   if (temp.obpp == 32)
   {
       if (conf.fast_sl)
       {
           rend_frame_32d1(dst, pitch);
           r_p4bpp_32d1(dst2, pitch);
       }
       else
       {
           rend_frame_32d(dst, pitch);
           r_p4bpp_32d(dst2, pitch);
       }
   }

   if (conf.noflic)
       memcpy(rbuf_s, temp.base-PAGE, 2*PAGE);

   temp.offset_vscroll_prev = temp.offset_vscroll;
   temp.offset_vscroll = 0;
   temp.offset_hscroll_prev = temp.offset_hscroll;
   temp.offset_hscroll = 0;
}
