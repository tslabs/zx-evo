#include "std.h"
#include "sysdefs.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "tsconf.h"
#include "sdcard.h"
#include "zc.h"

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
	static int iter1st;

	u8 tmp;
	int i, j;
	u16 *s, *d;
	u32 ss, dd;
	u32 m1 = ctrl.asz ? 0x3FFE00 : 0x3FFF00;
	u32 m2 = ctrl.asz ? 0x1FF : 0xFF;
	iter1st = 1;

	u16 sv = *(u16*)(comp.ts.dmasaddr + RAM_BASE_M);
	
	for (i=0; i<(comp.ts.dmanum + 1); i++)
	{
		ss = comp.ts.dmasaddr;
		dd = comp.ts.dmadaddr;

		switch (ctrl.dev)
		{
			case DMA_RAM:
			
				/* Blitter */
				if (ctrl.rw)
				{
					for (j=0; j<(comp.ts.dmalen + 1); j++)
					{
						s = (u16*)(ss + RAM_BASE_M);
						d = (u16*)(dd + RAM_BASE_M);
						u16 ds = *s;
						u16 dr = *d;
						u16 dw = 0;

						if (ctrl.asz)
						{
							dw |= (ds & 0xFF00) ? (ds & 0xFF00) : (dr & 0xFF00);
							dw |= (ds & 0x00FF) ? (ds & 0x00FF) : (dr & 0x00FF);
						}

						else
						{
							dw |= (ds & 0xF000) ? (ds & 0xF000) : (dr & 0xF000);
							dw |= (ds & 0x0F00) ? (ds & 0x0F00) : (dr & 0x0F00);
							dw |= (ds & 0x00F0) ? (ds & 0x00F0) : (dr & 0x00F0);
							dw |= (ds & 0x000F) ? (ds & 0x000F) : (dr & 0x000F);
						}

						*d = dw;
						ss_inc; dd_inc;
					}
				}

				/* RAM to RAM */
				else
				{
					for (j=0; j<(comp.ts.dmalen + 1); j++)
					{
						s = (u16*)(ss + RAM_BASE_M);
						d = (u16*)(dd + RAM_BASE_M);
						*d = *s;
						ss_inc; dd_inc;
					}
				}
				
				break;

			case DMA_SPI:
			
				/* RAM to SPI and SPI to RAM */
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
					s = (u16*)(ss + RAM_BASE_M);
					d = (u16*)(dd + RAM_BASE_M);
					if (ctrl.rw)	// RAM to SPI
					{
						Zc.Wr(0x57, *s & 0xFF);
						Zc.Wr(0x57, *s >> 8);
					}
					else		// SPI to RAM
					{
						u8 d1 = Zc.Rd(0x57);
						*d = (Zc.Rd(0x57) << 8) | d1;
					}
					ss_inc; dd_inc;
				}
				break;

			case DMA_IDE:
			
				/* RAM to IDE and IDE to RAM */
				for (j=0; j<(comp.ts.dmalen + 1); j++)
				{
					s = (u16*)(ss + RAM_BASE_M);
					d = (u16*)(dd + RAM_BASE_M);
					if (ctrl.rw)	// RAM to IDE
					{
						hdd.write_data(*s);
					}
					else	// IDE to RAM
					{
						*d = (u16)hdd.read_data();
					}
					ss_inc; dd_inc;
				}
				break;

			// RAM to CRAM or RAM filler
			case DMA_CRAM:
			
				/* CRAM */
				if (ctrl.rw)
				{
					for (j=0; j<(comp.ts.dmalen + 1); j++)
					{
						s = (u16*)(ss + RAM_BASE_M);
						tmp = (dd >> 1) & 0xFF;
						comp.cram[tmp] = *s;
						update_clut(tmp);
						ss_inc; dd_inc;
					}
				}
				
				/* RAM filler */
				else
				{
					if (iter1st)
					{
						ss_inc;
						iter1st = 0;
					}

					for (j=0; j<(comp.ts.dmalen + 1); j++)
					{
						d = (u16*)(dd + RAM_BASE_M);
						*d = sv;
						dd_inc;
					}
				}
				
				break;

			case DMA_SFILE:
			
				/* SFILE */
				if (ctrl.rw)
				{
					for (j=0; j<(comp.ts.dmalen + 1); j++)
					{
						s = (u16*)(ss + RAM_BASE_M);
						tmp = (dd >> 1) & 0xFF;
						comp.sfile[tmp] = *s;
						ss_inc; dd_inc;
					}
				}
				break;
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

u8 render_tile(u8 page, u32 tnum, u8 line, u32 x, u8 pal, u8 xf, u8 n)
{
	u8 cnt = 0;		// used to calculate RAM cycles usage

	/* check if number of allowed DRAM cycles per line (448) not exceeded */
	if ((vid.memcpucyc[vid.line - 1] + vid.memvidcyc[vid.line - 1] + vid.memtsscyc[vid.line - 1] + vid.memtstcyc[vid.line - 1]) > 448)
		return 0;

	u8 *g = page_ram(page & 0xF8) + ((tnum & 0xFC0) << 5) + (line << 8);
	x += (xf ? (n * 8 - 1) : 0);
	pal <<= 4;
	i8 a = xf ? -1 : 1;
	u8 c;
	u32 ox = (tnum & 0x3F) << 2;
	for (u32 i=0; i<n; i++)		// draw 8 pixels per iteration
	{
		//if (!((vid.tsline[x & 0x1FF] &
		//	vid.tsline[(x + a) & 0x1FF] &
		//	vid.tsline[(x + 2*a) & 0x1FF] &
		//	vid.tsline[(x + 3*a) & 0x1FF])
		//	& 0x0F))
			cnt++;
		//if (!((vid.tsline[(x + 4*a) & 0x1FF] &
		//	vid.tsline[(x + 5*a) & 0x1FF] &
		//	vid.tsline[(x + 6*a) & 0x1FF] &
		//	vid.tsline[(x + 7*a) & 0x1FF])
		//	& 0x0F))
			cnt++;
		
		if (c = g[ox + 0] & 0xF0) vid.tsline[x & 0x1FF] = pal | (c >> 4); x += a;
		if (c = g[ox + 0] & 0x0F) vid.tsline[x & 0x1FF] = pal | c; x += a;
		if (c = g[ox + 1] & 0xF0) vid.tsline[x & 0x1FF] = pal | (c >> 4); x += a;
		if (c = g[ox + 1] & 0x0F) vid.tsline[x & 0x1FF] = pal | c; x += a;
		if (c = g[ox + 2] & 0xF0) vid.tsline[x & 0x1FF] = pal | (c >> 4); x += a;
		if (c = g[ox + 2] & 0x0F) vid.tsline[x & 0x1FF] = pal | c; x += a;
		if (c = g[ox + 3] & 0xF0) vid.tsline[x & 0x1FF] = pal | (c >> 4); x += a;
		if (c = g[ox + 3] & 0x0F) vid.tsline[x & 0x1FF] = pal | c; x += a;
		ox = (ox + 4) & 0xFF;
	}

	return cnt;
}

SPRITE_t *spr = (SPRITE_t*)comp.sfile;
u32 snum;

void render_tile_layer(u8 layer)
{
	u32 y = (vid.yctr + (layer ? comp.ts.t1_yoffs : comp.ts.t0_yoffs)) & 0x1FF;
	u32 x = (layer ? comp.ts.t1_xoffs_d[1] : comp.ts.t0_xoffs_d[1]);
	TILE_t *tmap = (TILE_t*)(page_ram(comp.ts.tmpage) + ((y & 0x1F8) << 5));
	u32 ox = (x >> 3) & 0x3F;
	u32 l = (layer << 6);

	for (u32 i=0; i<46; i++)
	{
		TILE_t t = tmap[(ox + i) & 0x3F | l];
		if ((layer ? comp.ts.t1z_en : comp.ts.t0z_en) || t.tnum)
		{
			vid.memtstcyc[vid.line - 1] += render_tile(
				(layer ? comp.ts.t1gpage[2] : comp.ts.t0gpage[2]),			// page
				t.tnum,														// tile number
				(y ^ (t.yflp ? 7 : 0)) & 7,									// line offset (3 bit)
				(i << 3) - (x % 8),											// x coordinate in buffer (masked 0x1FF in func)
				((layer ? comp.ts.t1pal : comp.ts.t0pal) << 2) | t.pal,		// palette
				t.xflp, 1													// x flip, x size
			);
		}
	}
}

void render_sprite()
{
	//sfile_dump();
	SPRITE_t s = spr[snum];
	u8 ys = ((s.ys + 1) << 3);
	u32 l = (vid.yctr - s.y) & 0x1FF;

	if (l < ys)
	{
		vid.memtsscyc[vid.line - 1] += render_tile(
			comp.ts.sgpage,					// page
			s.tnum,							// tile number
			(u8)(s.yflp ? (ys - l - 1) : l),	// line offset (3 bit)
			s.x,							// x coordinate in buffer (masked 0x1FF in func)
			s.pal,							// palette
			s.xflp, s.xs + 1				// x flip, x size
		);
	}
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
	memset(vid.tsline, 0, 360);
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

// This used to debug SFILE operations
void sfile_dump()
{
	FILE *f;

	f = fopen("dump.bin", "wb");
	fwrite(comp.sfile, 1, 512, f);
	fclose(f);

	f = fopen("dump.txt", "w");
	fprintf(f, "sgpage:%d\n\n", comp.ts.sgpage);
	for(int i=0; i<85; i++)
	{
		SPRITE_t s = spr[i];
		fprintf(f, "%d\tx:%d\ty:%d\tact:%d\tleap:%d\txs:%d\tys:%d\ttnum:%5d\txf:%d\tyf:%d\tpal:%d\r", i, s.x, s.y, s.act, s.leap, (s.xs+1)*8, (s.ys+1)*8, s.tnum, s.xflp, s.yflp, s.pal);
	}
	fclose(f);
}

// TS-Config init
void tsinit(void)
{
	comp.ts.page[0] = 0;
	comp.ts.page[1] = 5;
	comp.ts.page[2] = 2;
	comp.ts.page[3] = 0;

	comp.ts.fmaddr = 0;
	comp.ts.im2vect = 255;
	comp.ts.fddvirt = 0;
	comp.ts.vdos = 0;
  comp.ts.vdos_m1 = 0;

	comp.ts.sysconf = 1;		// turbo 7MHz for TS-Conf
	comp.ts.memconf = 0;

	comp.ts.hsint = 2;
	comp.ts.vsint = 0;

	comp.ts.vpage = comp.ts.vpage_d = 5;
	comp.ts.vconf = comp.ts.vconf_d = 0;
	comp.ts.tsconf = 0;
	comp.ts.palsel = comp.ts.palsel_d = 15;
	comp.ts.g_xoffs = 0;
	comp.ts.g_yoffs = 0;
	
	comp.intpos = 1;
}
