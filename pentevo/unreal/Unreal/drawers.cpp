
// Drawers - render graphics from internal resources into 32 bit truecolor buffer 896x320

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

// ULA+ color models:
//
// val  red/grn     blue1       blue2
// 0    00000000    00000000    00000000
// 1    00100100
// 2    01001001
// 3    01101101    01101101    01101101
// 4    10010010                10010010
// 5    10110110    10110110
// 6    11011011
// 7    11111111    11111111    11111111

// ULA+ palette cell select:
// bit5 - FLASH
// bit4 - BRIGHT
// bit3 - 0 - INK / 1 - PAPER
// bits0..2 - INK / PAPER

#define col_def(a) (((a) << 5) | ((a) << 2) |  ((a) >> 1))
#define col_r(a) (col_def(a) << 16)
#define col_g(a) (col_def(a) << 8)
#define col_b(a) (col_def(a))

constexpr u32 cr[8] =
{
	col_r(0),
	col_r(1),
	col_r(2),
	col_r(3),
	col_r(4),
	col_r(5),
	col_r(6),
	col_r(7)
};

const u32 cg[8] =
{
	col_g(0),
	col_g(1),
	col_g(2),
	col_g(3),
	col_g(4),
	col_g(5),
	col_g(6),
	col_g(7)
};

const u32 cb[2][4] =
{
	col_b(0),
	col_b(3),
	col_b(5),
	col_b(7),

	col_b(0),
	col_b(3),
	col_b(4),
	col_b(7)
};

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
static void draw_zx(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u32 a = ((vid.ygctr & 0xF8) << 2) + (vid.xctr & 0x1F) + 0x1800;
	const u8* scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];
	const ulaplus upmod = conf.ulaplus;
	const u8 tsgpal = comp.ts.gpal << 4;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++, a++)
	{
		u32 p0, p1;
		u8 p = scr[g];	// pixels
		u8 c = scr[a];	// attributes
		vcyc++;
		vid.memcyc_lcmd++;

		if ((upmod != ulaplus::none) && comp.ulaplus_mode)
		{
			const u32 psel = (c & 0xC0) >> 2;
			const u32 ink = comp.ulaplus_cram[psel + (c & 7)];
			const u32 paper = comp.ulaplus_cram[psel + ((c >> 3) & 7) + 8];
			p0 = cr[(paper & 0x1C) >> 2] | cg[(paper & 0xE0) >> 5] | cb[(int)upmod][paper & 0x03];
			p1 = cr[(ink & 0x1C) >> 2] | cg[(ink & 0xE0) >> 5] | cb[(int)upmod][ink & 0x03];
		}

		else
		{
			if ((c & 0x80) && (comp.frame_counter & 0x10))
				p ^= 0xFF; // flash
			u32 b = (c & 0x40) >> 3;
			p0 = vid.clut[tsgpal | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
			p1 = vid.clut[tsgpal | b | (c & 0x07)];			// color for 'INK'
		}

		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Pentagon multicolor
static void draw_pmc(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	const u8* scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		const u8 p = scr[g];			// pixels
		const u8 c = scr[g + 0x2000];	// attributes
		vcyc++;
		const u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// AlCo 384x304
static void draw_p384(int n)
{
	const u8* scr = page_ram(comp.ts.vpage & 0xFE);
	const u32 g = ((vid.ygctr & 0x07) << 8) | ((vid.ygctr & 0x38) << 2);	// 'raw' values, page4, only 64 lines addressing, no column address
	const u32 a = ((vid.ygctr & 0x38) << 2) | (vid.xctr & 0x1F);
	u32 ogx, oax, ogy, oay;

	// select Y segment
	if (vid.ygctr < 64)
		ogy = 0, oay = 0x1000;
	else if (vid.ygctr < 256)
		ogy = 0x4000 | (((vid.ygctr - 64) & 0xC0) << 5), oay = 0x5800 | (((vid.ygctr - 64) & 0xC0) << 2);
	else
		ogy = 0x1800, oay = 0x1300;

	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (u32 o = 0; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, o = (o + 1) & 7)
	{
		// select X segment
		if (vid.xctr < 8)
			ogx = 0x18, oax = 0x0018;
		else if (vid.xctr < 40)
			ogx = 0x2000 | ((vid.xctr - 8) & 0x18), oax = 0x2000 | ((vid.xctr - 8) & 0x18);
		else
			ogx = 8, oax = 0x0008;

		const u8 p = scr[g + ogx + ogy + o];		// pixels
		const u8 c = scr[a + oax + oay + o];		// attributes
		vcyc++;
		const u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Pentagon 16c
static void draw_p16(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	const u8* scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];
	u32 p0, p1;

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 c = scr[g - PAGE];		// page4, #0000
		p16c_draw
			c = scr[g];					// page5, #0000
		p16c_draw
			c = scr[g - PAGE + PAGE / 2];	// page4, #2000
		p16c_draw
			c = scr[g + PAGE / 2];		// page5, #2000
		p16c_draw
			vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// TS text
static void draw_tstx(int n)
{
	vid.xctr &= 0x7F;
	const u8* scr = page_ram(comp.ts.vpage);			// video memory address
	const u8* fnt = page_ram(comp.ts.vpage ^ 0x01);	// font address
	const u32 s = ((vid.ygctr & 0x1F8) << 5);			// row address offset
	u32 vptr = vid.vptr;						// address in videobuffer
	u16 vcyc = vid.memvidcyc[vid.line];
	const u8 line = vid.ygctr & 7;					// symbol line (0-7)

	for (; n > 0; n -= 2, vid.t_next += 2)
	{
		const u8 sym = scr[s + vid.xctr];				// symbol code
		const u8 atr = scr[s + vid.xctr + 0x80];		// symbol attribute
		const u8 p = fnt[line + (sym << 3)];			// pixels
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((atr >> 4) & 0x0F)];	// true color for bit=0 (PAPER)
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | (atr & 0x0F)];			// true color for bit=1 (INK)
		hires_draw
			vid.xctr = (vid.xctr + 1) & 0x7F;
		vcyc += 2;
		vid.memcyc_lcmd += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Pentagon 512x192
static void draw_phr(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	const u8* scr = page_ram(comp.ts.vpage);
	const u32 p0 = vid.clut[(comp.ts.gpal << 4)];		// color for 'PAPER'
	const u32 p1 = vid.clut[(comp.ts.gpal << 4) + 7];	// color for 'INK'
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];
		hires_draw
			p = scr[g + 0x2000];
		vcyc++;
		hires_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// TS 16c
static void draw_ts16(int n)
{
	static int subt = 0;

	const u32 s = (vid.ygctr << 8);
	const u8* scr = page_ram(comp.ts.vpage & 0xF8);
	u32 t = (vid.xctr + (comp.ts.g_xoffs >> 1)) & 0xFF;
	vid.xctr += n;
	u32 p0, p1;
	u8 p;
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	if (comp.ts.g_xoffs & 1)		// odd offset - left pixel
	{
		n--; vid.t_next++;
		p = scr[s + t++]; t &= 0xFF;
		p1 = vid.clut[(comp.ts.gpal << 4) | (p & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p1; vptr += 2;
		subt++;
		if (subt > 3)
		{
			subt = 0; vcyc++; vid.memcyc_lcmd++;
		}

	}

	for (; n > 0; n--, vid.t_next++)
	{
		p = scr[s + t++]; t &= 0xFF;
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		p1 = vid.clut[(comp.ts.gpal << 4) | (p & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p0;
		vbuf[vid.buf][vptr + 2] = vbuf[vid.buf][vptr + 3] = p1;
		vptr += 4;
		subt += 2;
		if (subt > 3)
		{
			subt -= 4; vcyc++; vid.memcyc_lcmd++;
		}
	}

	if (comp.ts.g_xoffs & 1)		// odd offset - right pixel
	{
		p = scr[s + t];
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p0; vptr += 2;
		subt++;
		if (subt > 3)
		{
			subt = 0; vcyc++; vid.memcyc_lcmd++;
		}
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// TS 256c
static void draw_ts256(int n)
{
	const u32 s = (vid.ygctr << 9);
	const u8* scr = page_ram(comp.ts.vpage & 0xF0);
	u32 t = (vid.xctr + comp.ts.g_xoffs) & 0x1FF;
	vid.xctr += n * 2;
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 1, vid.t_next++)
	{
		u32 p = vid.clut[scr[s + t++]];  t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p; vptr += 2;
		p = vid.clut[scr[s + t++]]; t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p; vptr += 2;
		vcyc++;
		vid.memcyc_lcmd++;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Null
static void draw_nul(int n)
{
	u32 vptr = vid.vptr;
	RGB32 p;

	for (; n > 0; n -= 1, vid.t_next++)
	{
		p.r = p.g = p.b = rand() >> 8;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = p.p;
		p.r = p.g = p.b = rand() >> 8;
		vbuf[vid.buf][vptr + 2] = vbuf[vid.buf][vptr + 3] = p.p;
		vptr += 4;
	}
	vid.vptr = vptr;
}

// Border
void draw_border(int n)
{
	vid.t_next += n;
	u32 vptr = vid.vptr;
	u32 p;

	// ULAplus border mode is active only for Sinclair video mode
	if ((vid.mode == M_ZX) && (conf.ulaplus != ulaplus::none) && comp.ulaplus_mode) {
		const u8 idx = comp.ulaplus_cram[(comp.ts.border & 7) + 8];
		p = cr[(idx & 0x1C) >> 2] | cg[(idx & 0xE0) >> 5] | cb[(int)conf.ulaplus][idx & 0x03];
	}
	else p = vid.clut[comp.ts.border];

	for (; n > 0; n--)
	{
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = vbuf[vid.buf][vptr + 2] = vbuf[vid.buf][vptr + 3] = p;
		vptr += 4;
	}
	vid.vptr = vptr;
}

// TS Line
void draw_ts(u32 vptr)
{
	const u8 n = (vid.line ^ 1) & 1;
	const u32 max = (vid.vptr - vptr) >> 1;
	for (u32 i = 0; i < max; i++, vptr += 2, vid.ts_pos++)
	{
		if (vid.tsline[n][vid.ts_pos] & 0x0F)
			vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr + 1] = vid.clut[vid.tsline[n][vid.ts_pos]];
	}
}

DRAWER drawers[] = {
	{ draw_border	},	// Border only
	{ draw_nul		},	// Non-existing mode
	{ draw_zx		},	// Sinclair
	{ draw_pmc 		},	// Pentagon Multicolor
	{ draw_p16 		},	// Pentagon 16c
	{ draw_p384		},	// Pentagon 384x304
	{ draw_phr		},	// Pentagon HiRes
	{ draw_ts16		},	// TS 16c
	{ draw_ts256	},	// TS 256c
	{ draw_tstx		},	// TS Text
};