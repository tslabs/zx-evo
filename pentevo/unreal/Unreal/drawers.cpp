
// Drawers - render graphics from internal resources into 32 bit truecolor buffer 896x320

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

#define sinc_draw \
	vbuf[vid.buf][vptr   ] = vbuf[vid.buf][vptr+ 1] = ((p << 1) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+ 2] = vbuf[vid.buf][vptr+ 3] = ((p << 2) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+ 4] = vbuf[vid.buf][vptr+ 5] = ((p << 3) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+ 6] = vbuf[vid.buf][vptr+ 7] = ((p << 4) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+ 8] = vbuf[vid.buf][vptr+ 9] = ((p << 5) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+10] = vbuf[vid.buf][vptr+11] = ((p << 6) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+12] = vbuf[vid.buf][vptr+13] = ((p << 7) & 0x100) ? p1 : p0;	\
	vbuf[vid.buf][vptr+14] = vbuf[vid.buf][vptr+15] = ((p << 8) & 0x100) ? p1 : p0; \
	vptr += 16;

#define p16c_draw \
	p0 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x40) >> 3) | (c & 0x07)]; \
	p1 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x80) >> 4) | ((c >> 3) & 0x07)]; \
	vbuf[vid.buf][vptr  ] = vbuf[vid.buf][vptr+1] = p0; \
	vbuf[vid.buf][vptr+2] = vbuf[vid.buf][vptr+3] = p1; \
	vptr += 4;

#define hires_draw \
	vbuf[vid.buf][vptr  ] = ((p << 1) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+1] = ((p << 2) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+2] = ((p << 3) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+3] = ((p << 4) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+4] = ((p << 5) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+5] = ((p << 6) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+6] = ((p << 7) & 0x100) ? p1 : p0; \
	vbuf[vid.buf][vptr+7] = ((p << 8) & 0x100) ? p1 : p0; \
	vptr += 8;

// Sinclair
void draw_zx(int n)
{
	u32 g = ((vid.yctr & 0x07) << 8) + ((vid.yctr & 0x38) << 2) + ((vid.yctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u32 a = ((vid.yctr & 0xF8) << 2) + (vid.xctr & 0x1F) + 0x1800;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++, a++)
	{
		u8 p = scr[g];	// pixels
		u8 c = scr[a];	// attributes
		if ((c & 0x80) && (comp.frame_counter & 8))
			p ^= 0xFF; // flash
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
}

// Pentagon multicolor
void draw_pmc(int n)
{
	u32 g = ((vid.yctr & 0x07) << 8) + ((vid.yctr & 0x38) << 2) + ((vid.yctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];			// pixels
		u8 c = scr[g + 0x2000];	// attributes
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
}

// AlCo 384x304
void draw_p384(int n)
{
	u8 *scr = page_ram(comp.ts.vpage & 0xFE);
	u32 g = ((vid.yctr & 0x07) << 8) | ((vid.yctr & 0x38) << 2);	// 'raw' values, page4, only 64 lines addressing, no column address
	u32 a = ((vid.yctr & 0x38) << 2) | (vid.xctr & 0x1F);
	u32 vptr = vid.vptr;
	u32 ogx, oax, ogy, oay;

	// select Y segment
	if (vid.yctr < 64)
		ogy = 0, oay = 0x1000;
	else if (vid.yctr < 256)
		ogy = 0x4000 | (((vid.yctr-64) & 0xC0) << 5), oay = 0x5800 | (((vid.yctr-64) & 0xC0) << 2);
	else
		ogy = 0x1800, oay = 0x1300;

	for (u32 o = 0; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, o = (o + 1) & 7)
	{
		// select X segment
		if (vid.xctr < 8)
			ogx = 0x18, oax = 0x0018;
		else if (vid.xctr < 40)
			ogx = 0x2000 | ((vid.xctr-8) & 0x18), oax = 0x2000 | ((vid.xctr-8) & 0x18);
		else
			ogx = 8, oax = 0x0008;

		u8 p = scr[g + ogx + ogy + o];		// pixels
		u8 c = scr[a + oax + oay + o];		// attributes
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
}

// Sinclair double screen (debug)
void draw_zxw(int n)
{
	u32 g = ((vid.yctr & 0x07) << 8) + ((vid.yctr & 0x38) << 2) + ((vid.yctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u32 a = ((vid.yctr & 0xF8) << 2) + (vid.xctr & 0x1F) + 0x1800;
	u8 *scr = page_ram(comp.ts.vpage & 0xFD);
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++, a++)
	{
		u8 p = scr[g];	// pixels
		u8 c = scr[a];	// attributes
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		hires_draw
		vptr += 248;

		p = scr[g + PAGE*2];	// pixels
		c = scr[a + PAGE*2];	// attributes
		b = (c & 0x40) >> 3;
		p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		hires_draw
		vptr -= 256;
	}
	vid.vptr = vptr;
	if (vid.xctr & 0x20) vid.vptr += 256;
}

// Pentagon 16c
void draw_p16(int n)
{
	u32 g = ((vid.yctr & 0x07) << 8) + ((vid.yctr & 0x38) << 2) + ((vid.yctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u32 p0, p1;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 c = scr[g - PAGE];		// page4, #0000
		p16c_draw
		c = scr[g];					// page5, #0000
		p16c_draw
		c = scr[g - PAGE + PAGE/2];	// page4, #2000
		p16c_draw
		c = scr[g + PAGE/2];		// page5, #2000
		p16c_draw
	}
	vid.vptr = vptr;
}

// ATM 16c
void draw_atm16(int n)
{
	u32 g = vid.yctr * 40 + vid.xctr;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u32 p0, p1;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 c = scr[g - PAGE*4];		// page1, #0000
		p16c_draw
		c = scr[g];					// page5, #0000
		p16c_draw
		c = scr[g - PAGE*4 + PAGE/2];		// page1, #2000
		p16c_draw
		c = scr[g + PAGE/2];		// page5, #2000
		p16c_draw
	}
	vid.vptr = vptr;
}

// ATM HiRes
void draw_atmhr(int n)
{
	u32 g = vid.yctr * 40 + vid.xctr;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];				// page5, #0000
		u8 c = scr[g - PAGE*4];		// page1, #0000
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x80) >> 4) | ((c >> 3) & 0x07)];
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x40) >> 3) | (c & 0x07)];
		hires_draw

		p = scr[g + PAGE/2];				// page5, #0000
		c = scr[g - PAGE*4 + PAGE/2];		// page1, #0000
		p0 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x80) >> 4) | ((c >> 3) & 0x07)];
		p1 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x40) >> 3) | (c & 0x07)];
		hires_draw
	}
	vid.vptr = vptr;
}

// TS text
void draw_tstx(int n)
{
	vid.xctr &= 0x7F;
	u32 s = ((vid.yctr & 0x1F8) << 5);
	u8 *scr = page_ram(comp.ts.vpage);
	u8 *fnt = page_ram(comp.ts.vpage ^ 0x01);
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 2, vid.t_next += 2)
	{
		u8 a = scr[s + vid.xctr + 0x80];
		u8 p = fnt[(vid.yctr & 7) + (scr[s + vid.xctr++] << 3)]; vid.xctr &= 0x7F;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((a >> 4) & 0x0F)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | (a & 0x0F)];			// color for 'INK'
		hires_draw
	}
	vid.vptr = vptr;
}

// Pentagon 512x192
void draw_phr(int n)
{
	u32 g = ((vid.yctr & 0x07) << 8) + ((vid.yctr & 0x38) << 2) + ((vid.yctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u32 p0 = vid.clut[(comp.ts.gpal << 4)];		// color for 'PAPER'
	u32 p1 = vid.clut[(comp.ts.gpal << 4) + 7];	// color for 'INK'

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];
		hires_draw
		p = scr[g + 0x2000];
		hires_draw
	}
	vid.vptr = vptr;
}

// TS 16c
void draw_ts16(int n)
{
	u32 s = (vid.yctr << 8);
	u8 *scr = page_ram(comp.ts.vpage & 0xF8);
	u32 t = (vid.xctr + (comp.ts.g_offsx >> 1)) & 0xFF;
	vid.xctr += n;
	u32 p0, p1;
	u8 p;
	u32 vptr = vid.vptr;

	if (comp.ts.g_offsx & 1)		// odd offset - left pixel
	{
		n--; vid.t_next++;
		p = scr[s + t++]; t &= 0xFF;
		p1 = vid.clut[(comp.ts.gpal << 4) | (p & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p1; vptr += 2;
	}

	for (; n > 0; n -= 1, vid.t_next++)
	{
		p = scr[s + t++]; t &= 0xFF;
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		p1 = vid.clut[(comp.ts.gpal << 4) | (p & 0x0F)];
		vbuf[vid.buf][vptr  ] = vbuf[vid.buf][vptr+1] = p0;
		vbuf[vid.buf][vptr+2] = vbuf[vid.buf][vptr+3] = p1;
		vptr += 4;
	}

	if (comp.ts.g_offsx & 1)		// odd offset - right pixel
	{
		p = scr[s + t];
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p1; vptr += 2;
	}
	vid.vptr = vptr;
}

// TS 256c
void draw_ts256(int n)
{
	u32 s = (vid.yctr << 9);
	u8 *scr = page_ram(comp.ts.vpage & 0xF0);
	u32 t = (vid.xctr + comp.ts.g_offsx) & 0x1FF;
	vid.xctr += n * 2;
	u32 vptr = vid.vptr;

	for (; n > 0; n -= 1, vid.t_next++)
	{
		u32 p = vid.clut[scr[s + t++]];  t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p; vptr += 2;
		p = vid.clut[scr[s + t++]]; t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p; vptr += 2;
	}
	vid.vptr = vptr;
}

// Null
void draw_nul(int n)
{
	u32 vptr = vid.vptr;
	RGB32 p;

	for (; n > 0; n -= 1, vid.t_next++)
	{
		p.r = p.g = p.b = rand() >> 8;
		vbuf[vid.buf][vptr  ] = vbuf[vid.buf][vptr+ 1] = p.p;
		p.r = p.g = p.b = rand() >> 8;
		vbuf[vid.buf][vptr+2] = vbuf[vid.buf][vptr+ 3] = p.p;
		vptr += 4;
	}
	vid.vptr = vptr;
}

// Border
void draw_border(int n)
{
	vid.t_next += n;
	u32 vptr = vid.vptr;

	for (; n > 0; n--)
	{
		u32 p = vid.clut[comp.ts.border];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = vbuf[vid.buf][vptr+2] = vbuf[vid.buf][vptr+3] = p;
		vptr += 4;
	}
	vid.vptr = vptr;
}

// TS Line
void draw_ts()
// called at the end of pixel line, vptr points on 1st pix of the right border
{
	int s_pix = (vid.raster.r_brd - vid.raster.l_brd) * 2;
	u32 vptr = vid.vptr - s_pix * 2;
	for (int i = 64; i < (s_pix + 64); i++)
	{
		if (vid.tsline[i] & 0xF)	// if pixel is not transparent
			vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = vid.clut[vid.tsline[i]];
		vptr += 2;
	}
}
