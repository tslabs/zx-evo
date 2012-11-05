#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "tsconf.h"

extern VCTR vid;

const u8 pwm[] = { 0,10,21,31,42,53,63,74,85,95,106,117,127,138,149,159,170,181,191,202,213,223,234,245,255 };

// convert CRAM data to precalculated renderer tables
void update_clut(u8 addr)
{
	u16 t = comp.cram[addr];
	u8 r = (t >> 10) & 0x1F;
	u8 g = (t >> 5) & 0x1F;
	u8 b = t & 0x1F;

	// video PWM correction
	r = (r > 24) ? 24 : r;
	g = (g > 24) ? 24 : g;
	b = (b > 24) ? 24 : b;

	// coerce to TrueÚ Color values
	r = pwm[r];
	g = pwm[g];
	b = pwm[b];

	vid.clut[addr] = (r << 16) | (g <<8) | b;
}

// DMA procedures
#define ss_inc	ss = ctrl.s_algn ? ((ss & m1) | ((ss + 2) & m2)) : ((ss + 2) & 0x3FFFFF)
#define dd_inc	dd = ctrl.d_algn ? ((dd & m1) | ((dd + 2) & m2)) : ((dd + 2) & 0x3FFFFF)

void dma (u8 val)
{
	DMACTRL_t ctrl;
	ctrl.ctrl = val;

	u8 tmp;
	int i, j;
	u16 *s, *d;
	u32 ss, dd;
	u32 m1 = ctrl.asz ? 0x3FFE00 : 0x3FFF00;
	u32 m2 = ctrl.asz ? 0x1FF : 0xFF;

	for (i=0; i<(comp.ts.dmanum + 1); i++)
	{
		ss = comp.ts.dmasaddr;
		dd = comp.ts.dmadaddr;

		switch (ctrl.dev)
		{
			// RAM to RAM
			case DMA_RAM:
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
				s = (u16*)(ss + RAM_BASE_M);
				d = (u16*)(dd + RAM_BASE_M);
				*d = *s;
				ss_inc; dd_inc;
				}
				break;
			}
			// RAM to CRAM (palette)
			case DMA_CRAM:
			{
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
					s = (u16*)(ss + RAM_BASE_M);
					tmp = (dd >> 1) & 0xFF;
					comp.cram[tmp] = *s;
					update_clut(tmp);
					ss_inc; dd_inc;
				}
				break;
			}
		}

		if (ctrl.s_algn)
			comp.ts.dmasaddr += ctrl.asz ? 512 : 256;
		else
			comp.ts.dmasaddr = ss;

		if (ctrl.d_algn)
			comp.ts.dmadaddr += ctrl.asz ? 512 : 256;
		else
			comp.ts.dmadaddr = dd;
	}
}

// TS Engine

void render_tile(u8 page, u32 tnum, u8 line, u32 x, u8 pal, u8 xf, u8 n)
{
	u8 *g = page_ram(page) + ((tnum & 0xFC0) << 5) + (line << 8);
	x += (xf ? (n * 8 - 1) : 0) + 64;
	pal <<= 4;
	i8 a = xf ? -1 : 1;
	u8 c;
	u32 ox = ((tnum & 0x3F) << 2);
	for (u32 i=0; i<n; i++)
	{
		if (c = g[ox + 0] & 0xF0) vid.tsline[x] = pal | (c >> 4); x += a;
		if (c = g[ox + 0] & 0x0F) vid.tsline[x] = pal | c; x += a;
		if (c = g[ox + 1] & 0xF0) vid.tsline[x] = pal | (c >> 4); x += a;
		if (c = g[ox + 1] & 0x0F) vid.tsline[x] = pal | c; x += a;
		if (c = g[ox + 2] & 0xF0) vid.tsline[x] = pal | (c >> 4); x += a;
		if (c = g[ox + 2] & 0x0F) vid.tsline[x] = pal | c; x += a;
		if (c = g[ox + 3] & 0xF0) vid.tsline[x] = pal | (c >> 4); x += a;
		if (c = g[ox + 3] & 0x0F) vid.tsline[x] = pal | c; x += a;
		ox = (ox + (a * 4)) & 0xFF;
	}
}

SPRITE_t *spr = (SPRITE_t*)comp.sfile;
u32 snum;

void render_tile_layer(u8 layer)
{
	u32 y = (vid.yctr + (layer ? comp.ts.t1_offsy : comp.ts.t0_offsy)) & 0x1FF;
	u32 x = (layer ? comp.ts.t1_offsx : comp.ts.t0_offsx);
	TILE_t *tmap = (TILE_t*)(page_ram(comp.ts.tmpage) + ((y & 0x1F8) << 5));
	u32 ox = ((x & 0x1F8) >> 3) + (layer << 6);

	for (u32 i=0; i<46; i++)
	{
		TILE_t t = tmap[(ox + i) & 0x7F];
		if ((layer ? comp.ts.t1z_en : comp.ts.t0z_en) || t.tnum)
			render_tile(
				(layer ? comp.ts.t1gpage : comp.ts.t0gpage),				// page
				t.tnum,														// tile number
				(y ^ (t.yflp ? 7 : 0)) & 7,									// line offset (3 bit)
				(i << 3) - (x % 8),											// x coordinate in buffer (masked 0x1FF in func)
				((layer ? comp.ts.t1pal : comp.ts.t0pal) << 2) | t.pal,		// palette
				t.xflp, 1													// x flip, x size
			);
	}
}

void render_sprite()
{
	SPRITE_t s = spr[snum];
	u8 ys = ((s.ys + 1) << 3);
	i32 l = vid.yctr - s.y;

	if ((l >= 0) && (l < ys))
		render_tile(
			comp.ts.sgpage,					// page
			s.tnum,							// tile number
			(u8)(s.yflp ? (ys - l) : l),		// line offset (3 bit)
			s.x,							// x coordinate in buffer (masked 0x1FF in func)
			s.pal,							// palette
			s.xflp, s.xs + 1				// x flip, x size
		);
}

void render_sprite_layer()
{
	for (; snum < 85; snum++)
	{
		if (spr[snum].act)
			render_sprite();
		if (spr[snum].leap)
			{ snum++; break; }
	}
}

void render_ts()
{
	memset(vid.tsline+64, 0, 360);
	snum = 0;

	for (u32 layer=0; layer < 5; layer++)
	{
		if (layer == 0 || layer == 2 || layer == 4)
			if (comp.ts.s_en)
				{ render_sprite_layer(); continue; }
		if (layer == 1)
			if (comp.ts.t0_en)
				{ render_tile_layer(0); continue; }
		if (layer == 3)
			if (comp.ts.t1_en)
				{ render_tile_layer(1); continue; }
	}
}
