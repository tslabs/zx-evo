#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dxrend.h"
#include "draw.h"
#include "util.h"

// 1x 32bit
void render_1x(u8 *dst, u32 pitch)
{
	RGB32 *src = (RGB32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		RGB32 *src1 = src;
		for (u32 j=0; j<(pitch/4); j++)
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
void render_2x(u8 *dst, u32 pitch)
{
	if (!conf.noflic)
	{
		u32 *src = (u32*)vbuf[vid.buf];
		src += conf.framex * 2;
		src += conf.framey * VID_WIDTH * 2;
	
		for (u32 i=0; i<conf.frameysize; i++)
		{
			memcpy (dst, src, pitch); dst += pitch;
			memcpy (dst, src, pitch); dst += pitch;
			src += VID_WIDTH * 2;
		}
	}

	else	// noflic
	{
		RGB32 *src1 = (RGB32*)vbuf[vid.buf];
		src1 += conf.framex * 2;
		src1 += conf.framey * VID_WIDTH * 2;
		RGB32 *src2 = (RGB32*)vbuf[vid.buf^1];
		src2 += conf.framex * 2;
		src2 += conf.framey * VID_WIDTH * 2;
		
		for (u32 i=0; i<conf.frameysize; i++)
		{
			RGB32 *src11 = src1;
			RGB32 *src22 = src2;
			u8 *dst1 = dst;
			for (u32 j=0; j<(pitch/4); j++)
			{
				RGB32 p1 = *src11++; RGB32 p2 = *src22++;
				*dst++ = (p1.b + p2.b) >> 1;
				*dst++ = (p1.g + p2.g) >> 1;
				*dst++ = (p1.r + p2.r) >> 1;
				dst++;
			}
			memcpy (dst, dst1, pitch); dst += pitch;
			src1 += VID_WIDTH * 2;
			src2 += VID_WIDTH * 2;
		}
	}
}

// 2x 32bit Scanline
void render_2xs(u8 *dst, u32 pitch)
{
	RGB32 *src = (RGB32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		RGB32 *src1 = src;
		u8 *dst1 = dst;
		memcpy (dst, src, pitch); dst += pitch;
		for (u32 j=0; j<(pitch/4); j++)
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
void render_3x(u8 *dst, u32 pitch)
{
	RGB32 *src = (RGB32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
    for (u8 k=0; k<3; k++)
    {
		  RGB32 *src1 = src;
		  u8 *dst1 = dst;
		  for (u32 j=0; j<(pitch/12); j++)
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
      dst += pitch;
    }
		src += VID_WIDTH * 2;
	}
}

// 4x 32bit
void render_4x(u8 *dst, u32 pitch)
{
	u32 *src = (u32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
    for (u8 k=0; k<4; k++)
    {
		  u32 *src1 = src;
		  u32 *dst1 = (u32*)dst;
		  for (u32 j=0; j<(pitch/8); j++)
        *dst1++ = *src1, *dst1++ = *src1++;
		
		  dst += pitch;
    }
		src += VID_WIDTH * 2;
	}
}
