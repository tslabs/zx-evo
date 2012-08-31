#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dxrend.h"
#include "draw.h"
#include "util.h"

// 1x 32bit
void rend_1x_32(u8 *dst, u32 pitch)
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
void rend_2x_32(u8 *dst, u32 pitch)
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

// 2x 32bit NoFlic
void rend_2x_32_nf(u8 *dst, u32 pitch)
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

// 2x 32bit Scanline
void rend_2xs_32(u8 *dst, u32 pitch)
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
void rend_3x_32(u8 *dst, u32 pitch)
{
	RGB32 *src = (RGB32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		RGB32 *src1 = src;
		u8 *dst1 = dst;
		for (u32 j=0; j<(pitch/12); j++)
		{
			RGB32 p1 = *src1++; RGB32 p2 = *src1++;
			*dst++ = p1.b;
			*dst++ = p1.g;
			*dst++ = p1.r;
			dst++;
			*dst++ = (p1.b + p2.b) >> 1;
			*dst++ = (p1.g + p2.g) >> 1;
			*dst++ = (p1.r + p2.r) >> 1;
			dst++;
			*dst++ = p2.b;
			*dst++ = p2.g;
			*dst++ = p2.r;
			dst++;
		}
		memcpy (dst, dst1, pitch); dst += pitch;
		memcpy (dst, dst1, pitch); dst += pitch;
		src += VID_WIDTH * 2;
	}
}

// 4x 32bit
void rend_4x_32(u8 *dst, u32 pitch)
{
	u32 *src = (u32*)vbuf[vid.buf];
	src += conf.framex*2;
	src += conf.framey * VID_WIDTH*2;

	for (u32 i=0; i<conf.frameysize; i++)
	{
		u32 *src1 = src;
		u32 *dst1 = (u32*)dst;
		for (u32 j=0; j<(pitch/8); j++)
			*dst1++ = *src1, *dst1++ = *src1++;
		memcpy ((u8*)dst1, dst, pitch); dst1 += pitch/4;
		memcpy ((u8*)dst1, dst, pitch); dst1 += pitch/4;
		memcpy ((u8*)dst1, dst, pitch);
		
		dst += pitch*4;
		src += VID_WIDTH * 2;
	}
}

// Bitness selectors
// 1x
void render_1x(u8 *dst, u32 pitch)
{
	if (conf.noflic)    {
		if (temp.obpp == 8)  { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_1x_32(dst, pitch); return; }
	} else {
		if (temp.obpp == 8)  { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_1x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_1x_32(dst, pitch); return; }
	}
}

// 2x
void render_2x(u8 *dst, u32 pitch)
{
	if (conf.noflic) {
		if (temp.obpp == 8)  { rend_2x_32_nf(dst, pitch); return; }
		if (temp.obpp == 16) { rend_2x_32_nf(dst, pitch); return; }
		if (temp.obpp == 32) { rend_2x_32_nf(dst, pitch); return; }
	} else {
		if (temp.obpp == 8)  { rend_2x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_2x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_2x_32(dst, pitch); return; }
	}
}

// 2x Scanline
void render_2xs(u8 *dst, u32 pitch)
{
	if (conf.noflic) {
		if (temp.obpp == 8)  { rend_2xs_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_2xs_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_2xs_32(dst, pitch); return; }
   } else {
		if (temp.obpp == 8)  { rend_2xs_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_2xs_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_2xs_32(dst, pitch); return; }
   }
}

// 3x
void render_3x(u8 *dst, u32 pitch)
{
   if (conf.noflic) {
		if (temp.obpp == 8)  { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_3x_32(dst, pitch); return; }
   } else {
		if (temp.obpp == 8)  { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_3x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_3x_32(dst, pitch); return; }
   }
}

// 4x
void render_4x(u8 *dst, u32 pitch)
{
   if (conf.noflic) {
		if (temp.obpp == 8)  { rend_4x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_4x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_4x_32(dst, pitch); return; }
	} else {
		if (temp.obpp == 8)  { rend_4x_32(dst, pitch); return; }
		if (temp.obpp == 16) { rend_4x_32(dst, pitch); return; }
		if (temp.obpp == 32) { rend_4x_32(dst, pitch); return; }
	}
}
