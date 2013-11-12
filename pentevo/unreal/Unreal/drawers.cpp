
// Drawers - render graphics from internal resources into 32 bit truecolor buffer 896x320

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];
extern u8 fontatm2[2048];

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
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u32 a = ((vid.ygctr & 0xF8) << 2) + (vid.xctr & 0x1F) + 0x1800;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++, a++)
	{
		u32 p0, p1;
		u8 p = scr[g];	// pixels
		u8 c = scr[a];	// attributes
		vcyc++;

		if (conf.ulaplus && comp.ulaplus_mode)
		{
			const u32 cr[8] = { 0, 2359296, 4784128, 7143424, 9568256, 11927552, 14352384, 16711680 };
			const u32 cg[8] = { 0, 9216, 18688, 27904, 37376, 46592, 56064, 65280 };
			const u32 cb[4] = { 0, 85, 170, 255 };
			u32 psel = (c & 0xC0) >> 2;
			u32 ink = comp.ulaplus_cram[psel + (c & 7)];
			u32 paper = comp.ulaplus_cram[psel + ((c >> 3) & 7) + 8];
			p0 = cr[(paper & 0x1C) >> 2] | cg[(paper & 0xE0) >> 5] | cb[paper & 0x03];
			p1 = cr[(ink & 0x1C) >> 2] | cg[(ink & 0xE0) >> 5] | cb[ink & 0x03];
		}

		else
		{
			if ((c & 0x80) && (comp.frame_counter & 0x10))
				p ^= 0xFF; // flash
			u32 b = (c & 0x40) >> 3;
			p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
			p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		}

		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Pentagon multicolor
void draw_pmc(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];			// pixels
		u8 c = scr[g + 0x2000];	// attributes
		vcyc++;
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// AlCo 384x304
void draw_p384(int n)
{
	u8 *scr = page_ram(comp.ts.vpage & 0xFE);
	u32 g = ((vid.ygctr & 0x07) << 8) | ((vid.ygctr & 0x38) << 2);	// 'raw' values, page4, only 64 lines addressing, no column address
	u32 a = ((vid.ygctr & 0x38) << 2) | (vid.xctr & 0x1F);
	u32 ogx, oax, ogy, oay;

	// select Y segment
	if (vid.ygctr < 64)
		ogy = 0, oay = 0x1000;
	else if (vid.ygctr < 256)
		ogy = 0x4000 | (((vid.ygctr-64) & 0xC0) << 5), oay = 0x5800 | (((vid.ygctr-64) & 0xC0) << 2);
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
			ogx = 0x2000 | ((vid.xctr-8) & 0x18), oax = 0x2000 | ((vid.xctr-8) & 0x18);
		else
			ogx = 8, oax = 0x0008;

		u8 p = scr[g + ogx + ogy + o];		// pixels
		u8 c = scr[a + oax + oay + o];		// attributes
		vcyc++;
		u32 b = (c & 0x40) >> 3;
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | b | ((c >> 3) & 0x07)];	// color for 'PAPER'
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | b | (c & 0x07)];			// color for 'INK'
		sinc_draw
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Sinclair double screen (debug)
void draw_zxw(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u32 a = ((vid.ygctr & 0xF8) << 2) + (vid.xctr & 0x1F) + 0x1800;
	u8 *scr = page_ram(comp.ts.vpage & 0xFD);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++, a++)
	{
		u8 p = scr[g];	// pixels
		u8 c = scr[a];	// attributes
		vcyc++;
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
	vid.memvidcyc[vid.line] = vcyc;
	if (vid.xctr & 0x20) vid.vptr += 256;
}

// Pentagon 16c
void draw_p16(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];
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
		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// ATM 16c
void draw_atm16(int n)
{
	u32 g = vid.ygctr * 40 + vid.xctr;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];
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
		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// ATM HiRes
void draw_atmhr(int n)
{
	u32 g = vid.ygctr * 40 + vid.xctr;
	u8 *scr = page_ram(comp.ts.vpage);
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 4, vid.t_next += 4, vid.xctr++, g++)
	{
		u8 p = scr[g];				// page5, #0000
		u8 c = scr[g - PAGE*4];		// page1, #0000
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x80) >> 4) | ((c >> 3) & 0x07)];
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x40) >> 3) | (c & 0x07)];
		hires_draw

		p = scr[g + PAGE/2];				// page5, #0000
		c = scr[g - PAGE*4 + PAGE/2];		// page1, #0000
		p0 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x80) >> 4) | ((c >> 3) & 0x07)];	// true color for bit=0 (PAPER)
		p1 = vid.clut[(comp.ts.gpal << 4) | ((c & 0x40) >> 3) | (c & 0x07)];       	// true color for bit=1 (INK)
		hires_draw

		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// TS text
void draw_tstx(int n)
{
	vid.xctr &= 0x7F;
	u8 *scr = page_ram(comp.ts.vpage);			// video memory address
	u8 *fnt = page_ram(comp.ts.vpage ^ 0x01);	// font address
	u32 s = ((vid.ygctr & 0x1F8) << 5);			// row address offset
	u32 vptr = vid.vptr;						// address in videobuffer
	u16 vcyc = vid.memvidcyc[vid.line];
	u8 line = vid.ygctr & 7;					// symbol line (0-7)

	for (; n > 0; n -= 2, vid.t_next += 2)
	{
		u8 sym = scr[s + vid.xctr];				// symbol code
		u8 atr = scr[s + vid.xctr + 0x80];		// symbol attribute
		u8 p = fnt[line + (sym << 3)];			// pixels
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((atr >> 4) & 0x0F)];	// true color for bit=0 (PAPER)
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | (atr & 0x0F)];			// true color for bit=1 (INK)
		hires_draw
		vid.xctr = (vid.xctr + 1) & 0x7F;
		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// ATM2 text
void draw_atm2tx(int n)
{
	u8 *scrs = page_ram(comp.ts.vpage);		// video memory symbols address
	u8 *scra = page_ram(comp.ts.vpage-4);	// video memory attrs address
	u8 *fnt = fontatm2;						// font address
	u32 vptr = vid.vptr;					// address in videobuffer
	u16 vcyc = vid.memvidcyc[vid.line];
	u32 y = vid.ygctr >> 3;					// row
	u8 line = vid.ygctr & 7;				// line (0-7)

	for (; n > 0; n -= 2, vid.t_next += 2)
	{
		u32 ss = (vid.xctr & 1) ? 0x21C0 : 0x01C0;	// left/right symbol offset
		u32 sa = (vid.xctr & 1) ? 0x01C1 : 0x21C0;	// left/right attr offset
		u32 x = vid.xctr >> 1;						// pair column
		u8 sym = scrs[(y * 64) + x + ss];			// symbol code
		u8 atr = scra[(y * 64) + x + sa];			// symbol attribute
		u8 p = fnt[(line << 8) + sym];				// pixels
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((atr & 0x80) >> 4) | ((atr >> 3) & 0x07)];	// true color for bit=0 (PAPER)
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | ((atr & 0x40) >> 3) | (atr & 0x07)];       	// true color for bit=1 (INK)
		hires_draw
		vid.xctr = (vid.xctr + 1) & 0x7F;
		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// ATM3 text
void draw_atm3tx(int n)
{
	u8 *scr = page_ram(comp.ts.vpage & 2 | 8);		// video memory address
	u8 *fnt = fontatm2;						// font address
	u32 y = vid.ygctr >> 3;					// row
	u8 line = vid.ygctr & 7;				// line (0-7)
	u32 vptr = vid.vptr;					// address in videobuffer
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 2, vid.t_next += 2)
	{
		u32 ss = (vid.xctr & 1) ? 0x11C0 : 0x01C0;	// left/right symbol offset
		u32 sa = (vid.xctr & 1) ? 0x21C1 : 0x31C0;	// left/right attr offset
		u32 x = vid.xctr >> 1;						// pair column
		u8 sym = scr[(y * 64) + x + ss];			// symbol code
		u8 atr = scr[(y * 64) + x + sa];			// symbol attribute
		u8 p = fnt[(line << 8) + sym];				// pixels
		u32 p0 = vid.clut[(comp.ts.gpal << 4) | ((atr & 0x80) >> 4) | ((atr >> 3) & 0x07)];	// true color for bit=0 (PAPER)
		u32 p1 = vid.clut[(comp.ts.gpal << 4) | ((atr & 0x40) >> 3) | (atr & 0x07)];       	// true color for bit=1 (INK)
		hires_draw
		vid.xctr = (vid.xctr + 1) & 0x7F;
		vcyc += 2;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// Pentagon 512x192
void draw_phr(int n)
{
	u32 g = ((vid.ygctr & 0x07) << 8) + ((vid.ygctr & 0x38) << 2) + ((vid.ygctr & 0xC0) << 5) + (vid.xctr & 0x1F);
	u8 *scr = page_ram(comp.ts.vpage);
	u32 p0 = vid.clut[(comp.ts.gpal << 4)];		// color for 'PAPER'
	u32 p1 = vid.clut[(comp.ts.gpal << 4) + 7];	// color for 'INK'
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
void draw_ts16(int n)
{
static int subt = 0;

	u32 s = (vid.ygctr << 8);
	u8 *scr = page_ram(comp.ts.vpage & 0xF8);
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
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p1; vptr += 2;
		subt++;
		if (subt > 3)
		{
			subt = 0; vcyc++;
		}

	}

	for (; n > 0; n--, vid.t_next++)
	{
		p = scr[s + t++]; t &= 0xFF;
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		p1 = vid.clut[(comp.ts.gpal << 4) | (p & 0x0F)];
		vbuf[vid.buf][vptr  ] = vbuf[vid.buf][vptr+1] = p0;
		vbuf[vid.buf][vptr+2] = vbuf[vid.buf][vptr+3] = p1;
		vptr += 4;
		subt += 2;
		if (subt > 3)
		{
			subt -= 4; vcyc++;
		}
	}

	if (comp.ts.g_xoffs & 1)		// odd offset - right pixel
	{
		p = scr[s + t];
		p0 = vid.clut[(comp.ts.gpal << 4) | ((p >> 4) & 0x0F)];
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p0; vptr += 2;
		subt++;
		if (subt > 3)
		{
			subt = 0; vcyc++;
		}
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
}

// TS 256c
void draw_ts256(int n)
{
	u32 s = (vid.ygctr << 9);
	u8 *scr = page_ram(comp.ts.vpage & 0xF0);
	u32 t = (vid.xctr + comp.ts.g_xoffs) & 0x1FF;
	vid.xctr += n * 2;
	u32 vptr = vid.vptr;
	u16 vcyc = vid.memvidcyc[vid.line];

	for (; n > 0; n -= 1, vid.t_next++)
	{
		u32 p = vid.clut[scr[s + t++]];  t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p; vptr += 2;
		p = vid.clut[scr[s + t++]]; t &= 0x1FF;
		vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = p; vptr += 2;
		vcyc++;
	}
	vid.vptr = vptr;
	vid.memvidcyc[vid.line] = vcyc;
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
	for (int i = 0; i < (s_pix); i++)
	{
		if (vid.tsline[i] & 0xF)	// if pixel is not transparent
			vbuf[vid.buf][vptr] = vbuf[vid.buf][vptr+1] = vid.clut[vid.tsline[i]];
		vptr += 2;
	}
}
