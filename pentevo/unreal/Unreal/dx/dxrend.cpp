#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dxrend.h"
#include "draw.h"

// 1x 32bit
void render_1x(u8 *dst, u32 stride_u8)
{
  RGB32 *src = (RGB32*)vbuf[vid.buf];
  src += conf.framex*2;
  src += conf.framey * VID_WIDTH*2;

  for (u32 i=0; i<conf.frameysize; i++)
  {
    RGB32 *src1 = src;
    for (u32 j=0; j<(stride_u8/4); j++)
    {
      RGB32 p1 = *src1++; RGB32 p2 = *src1++;
      *dst++ = (p1.b + p2.b) >> 1;
      *dst++ = (p1.g + p2.g) >> 1;
      *dst++ = (p1.r + p2.r) >> 1;
      dst++;
    }
    src += VID_WIDTH * 2;
  }
}

// 2x 32bit
void render_2x(u8 *dst, u32 stride_u8)
{
  if (!conf.noflic)
  {
    u32 *src = (u32*)vbuf[vid.buf];
    src += conf.framex * 2;
    src += conf.framey * VID_WIDTH * 2;

    for (u32 i=0; i<conf.frameysize; i++)
    {
      memcpy (dst, src, stride_u8); dst += stride_u8;
      memcpy (dst, src, stride_u8); dst += stride_u8;
      src += VID_WIDTH * 2;
    }
  }
  else  // noflic
  {
    RGB32 *src1 = (RGB32*)vbuf[vid.buf];
    src1 += conf.framex * 2;
    src1 += conf.framey * VID_WIDTH * 2;
    RGB32 *src2 = (RGB32*)vbuf[vid.buf ^ 1];
    src2 += conf.framex * 2;
    src2 += conf.framey * VID_WIDTH * 2;

    for (u32 i=0; i<conf.frameysize; i++)
    {
      RGB32 *src11 = src1;
      RGB32 *src22 = src2;
      u8 *dst1 = dst;
      for (u32 j=0; j<(stride_u8/4); j++)
      {
        RGB32 p1 = *src11++; RGB32 p2 = *src22++;
        *dst++ = (p1.b + p2.b) >> 1;
        *dst++ = (p1.g + p2.g) >> 1;
        *dst++ = (p1.r + p2.r) >> 1;
        dst++;
      }
      memcpy (dst, dst1, stride_u8); dst += stride_u8;
      src1 += VID_WIDTH * 2;
      src2 += VID_WIDTH * 2;
    }
  }
}

// 2x 32bit Scanline
void render_2xs(u8 *dst, u32 stride_u8)
{
  RGB32 *src = (RGB32*)vbuf[vid.buf];
  src += conf.framex * 2;
  src += conf.framey * VID_WIDTH * 2;

  for (u32 i=0; i<conf.frameysize; i++)
  {
    RGB32 *src1 = src;
    u8 *dst1 = dst;
    memcpy (dst, src, stride_u8); dst += stride_u8;
    for (u32 j=0; j < (stride_u8 / 4); j++)
    {
      RGB32 p = *src1++;
      *dst++ = p.b >> 1;
      *dst++ = p.g >> 1;
      *dst++ = p.r >> 1;
      dst++;
    }
    src += VID_WIDTH * 2;
  }
}

// 3x 32bit
void render_3x(u8 *dst, u32 stride_u8)
{
  u32 stride_u32 = stride_u8 / 4;
  
  if (!conf.noflic)
  {
    RGB32 *src = (RGB32*)vbuf[vid.buf];
    src += conf.framex * 2;
    src += conf.framey * VID_WIDTH * 2;

    for (u32 i = 0; i < conf.frameysize; i++)
    {
      RGB32 *src1 = src;
      u8 *dst1 = dst;

      for (u32 j = 0; j < (stride_u32 / 3); j++)
      {
        RGB32 p1 = *src1++; RGB32 p2 = *src1++;
        *dst1++ = p1.b;
        *dst1++ = p1.g;
        *dst1++ = p1.r;
        dst1++;
        *dst1++ = (p1.b + p2.b) >> 1;
        *dst1++ = (p1.g + p2.g) >> 1;
        *dst1++ = (p1.r + p2.r) >> 1;
        dst1++;
        *dst1++ = p2.b;
        *dst1++ = p2.g;
        *dst1++ = p2.r;
        dst1++;
      }

      memcpy(dst1, dst, stride_u8);
      dst1 += stride_u8;
      memcpy(dst1, dst, stride_u8);
      dst += stride_u8 * 3;
      src += VID_WIDTH * 2;
    }
  }
  else
  {
    RGB32 *src1 = (RGB32*)vbuf[vid.buf];
    src1 += conf.framex * 2;
    src1 += conf.framey * VID_WIDTH * 2;
    RGB32 *src2 = (RGB32*)vbuf[vid.buf ^ 1];
    src2 += conf.framex * 2;
    src2 += conf.framey * VID_WIDTH * 2;

    for (u32 i = 0; i < conf.frameysize; i++)
    {
      RGB32 *src11 = src1;
      RGB32 *src22 = src2;
      u8 *dst1 = dst;

      for (u32 j = 0; j < (stride_u8 / 12); j++)
      {
        RGB32 p11 = *src11++; RGB32 p12 = *src11++;
        RGB32 p21 = *src22++; RGB32 p22 = *src22++;
        *dst1++ = (p11.b + p21.b) >> 1;
        *dst1++ = (p11.g + p21.g) >> 1;
        *dst1++ = (p11.r + p21.r) >> 1;
        dst1++;
        *dst1++ = (p11.b + p21.b + p12.b + p22.b) >> 2;
        *dst1++ = (p11.g + p21.g + p12.g + p22.g) >> 2;
        *dst1++ = (p11.r + p21.r + p12.r + p22.r) >> 2;
        dst1++;
        *dst1++ = (p12.b + p22.b) >> 1;
        *dst1++ = (p12.g + p22.g) >> 1;
        *dst1++ = (p12.r + p22.r) >> 1;
        dst1++;
      }

      memcpy(dst1, dst, stride_u8); dst1 += stride_u8;
      memcpy(dst1, dst, stride_u8);
      dst += stride_u8 * 3;
      src1 += VID_WIDTH * 2;
      src2 += VID_WIDTH * 2;
    }
  }
}

// 4x 32bit
void render_4x(u8 *dest, u32 stride_u8)
{
  u32 stride_u32 = stride_u8 / 4;
  
  if (!conf.noflic)
  {
    u32 *dst = (u32*)dest;
    u32 *src = (u32*)vbuf[vid.buf];
    src += conf.framex * 2;
    src += conf.framey * VID_WIDTH * 2;

    for (u32 i = 0; i < conf.frameysize; i++)
    {
      u32 *src1 = src;
      u32 *dst1 = dst;

      for (u32 j = 0; j < (stride_u32 / 2); j++)
      {
        *dst1++ = *src1;
        *dst1++ = *src1++;
      }

      dst1 = dst + stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst1 += stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst1 += stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst += stride_u32 * 4;
      src += VID_WIDTH * 2;
    }
  }
  else
  {
    u32 *dst = (u32*)dest;
    u32 *src1 = (u32*)vbuf[vid.buf] + (conf.framex * 2) + (conf.framey * VID_WIDTH * 2);
    u32 *src2 = (u32*)vbuf[vid.buf ^ 1] + (conf.framex * 2) + (conf.framey * VID_WIDTH * 2);

    for (u32 i = 0; i < conf.frameysize; i++)
    {
      u32 *src11 = src1;
      u32 *src22 = src2;
      u32 *dst1 = dst;

      for (u32 j = 0; j < (stride_u32 / 2); j++)
      {
        *dst1++ = (*src11 + *src22) >> 1;
        *dst1++ = (*src11++ + *src22++) >> 1;
      }

      dst1 = dst + stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst1 += stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst1 += stride_u32;
      memcpy(dst1, dst, stride_u8);
      dst += stride_u32 * 4;
      src1 += VID_WIDTH * 2;
      src2 += VID_WIDTH * 2;
    }
  }
}
