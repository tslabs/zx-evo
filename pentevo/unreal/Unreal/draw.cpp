#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "drawers.h"
#include "dx/dx.h"
#include "config.h"
#include "util.h"
#include "leds.h"

const RASTER raster[R_MAX] = {
	{ R_256_192, 80, 272, 70, 70 + 128, 198 },
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
__int64 rbuf__[sizeof_rbuf / sizeof(__int64)];
u8* const rbuf = (u8*)rbuf__;
#endif

CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];
VCTR vid;

u8* const rbuf_s = rbuf + rb2_offs; // frames to mix with noflic and resampler filters
u8* const save_buf = rbuf_s + rb2_offs * MAX_BUFFERS; // used in monitor

T t;

videopoint* vcurr;
videopoint video[4 * MAX_HEIGHT];
unsigned vmode;  // what are drawing: 0-not visible, 1-border, 2-screen
unsigned prev_t; // last drawn pixel
unsigned* atrtab;

u8 colortab[0x100];// map zx attributes to pc attributes
// colortab shifted to 8 and 24
unsigned colortab_s8[0x100];
unsigned colortab_s24[0x100];

/*
#include "drawnomc.cpp"
#include "draw_384.cpp"
*/

PALETTEENTRY pal0[0x100]; // emulator palette

unsigned getYUY2(unsigned r, unsigned g, unsigned b)
{
	int y = (int)(0.29 * r + 0.59 * g + 0.14 * b);
	int u = (int)(128.0 - 0.14 * r - 0.29 * g + 0.43 * b);
	int v = (int)(128.0 + 0.36 * r - 0.29 * g - 0.07 * b);
	if (y < 0) y = 0; if (y > 255) y = 255;
	if (u < 0) u = 0; if (u > 255) u = 255;
	if (v < 0) v = 0; if (v > 255) v = 255;
	return WORD4(y, u, y, v);
}

void create_palette()
{
	if ((temp.rflags & RF_8BPCH) && temp.obpp == 8) temp.rflags |= RF_GRAY, conf.flashcolor = 0;

	palette_options* pl = &pals[conf.pal];
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
		}
		else if (temp.rflags & RF_PALB) { // palette index: gg0rr0bb
			b0 = brights[i & 3];
			r0 = brights[(i >> 3) & 3];
			g0 = brights[(i >> 6) & 3];
		}
		else { // palette index: ygrbYGRB
			b0 = brtab[((i >> 0) & 1) + ((i >> 2) & 2) + ((i >> 2) & 4) + ((i >> 4) & 8)]; // brtab[ybYB]
			r0 = brtab[((i >> 1) & 1) + ((i >> 2) & 2) + ((i >> 3) & 4) + ((i >> 4) & 8)]; // brtab[yrYR]
			g0 = brtab[((i >> 2) & 1) + ((i >> 2) & 2) + ((i >> 4) & 4) + ((i >> 4) & 8)]; // brtab[ygYG]
		}

		// transform with current settings
		unsigned r = 0xFF & ((r0 * pl->r11 + g0 * pl->r12 + b0 * pl->r13) / 0x100);
		unsigned g = 0xFF & ((r0 * pl->r21 + g0 * pl->r22 + b0 * pl->r23) / 0x100);
		unsigned b = 0xFF & ((r0 * pl->r31 + g0 * pl->r32 + b0 * pl->r33) / 0x100);

		// prepare palette in bitmap header for GDI renderer
		gdibmp.header.bmiColors[i].rgbRed = pal0[i].peRed = r;
		gdibmp.header.bmiColors[i].rgbGreen = pal0[i].peGreen = g;
		gdibmp.header.bmiColors[i].rgbBlue = pal0[i].peBlue = b;
	}
	memcpy(syspalette + 10, pal0 + 10, (246 - 9) * sizeof * syspalette);
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

}

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

	set_video();
}

u32 get_free_memcycles(int dram_t)
{
	if (vid.memcyc_lcmd >= dram_t)
		return 0;

	const u32 memcycles = vid.memcpucyc[vid.line] + vid.memvidcyc[vid.line] + vid.memtstcyc[vid.line] + vid.memtsscyc[vid.line] + vid.memdmacyc[vid.line];

	if (memcycles >= MEM_CYCLES)
		return 0;

	u32 free_t = dram_t - vid.memcyc_lcmd;
	free_t = min(free_t, MEM_CYCLES - memcycles);

	return free_t;
}

void update_screen()
{
	// Get tact of cpu state in current frame
	const u32 cput = (cpu.t >= conf.frame) ? (VID_TACTS * VID_LINES) : cpu.t;

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
			comp.ts.vpage = comp.ts.vpage_d;    // Video Page
			comp.ts.palsel = comp.ts.palsel_d;   // Palette Selector

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
				const u32 m = min((u32)n, vid.raster.r_brd - vid.line_pos);
				u32 t = vid.t_next; // store tact of video controller
				const u32 vptr = vid.vptr;
				drawers[vid.mode].func(m);
				if (conf.mem_model == MM_TSL) draw_ts(vptr);
				t = vid.t_next - t; // calculate tacts used by drawers func
				n -= t; vid.line_pos += (u16)t;
			}
			// Render right border
			if (n > 0)
			{
				const u32 m = min(n, VID_TACTS - vid.line_pos);
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

	vid.raster = raster[R_256_192];

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

	// Sinclair
	vid.mode = M_ZX;
}

void init_frame()
{
	// draw on single buffer if noflic is not active
	if (conf.noflic) vid.buf ^= 1;

	switch (conf.ray_paint_mode) {
	case ray_paint_mode::clear:
		memset(vbuf[vid.buf], 0xFF000000, sizeof(u32) * sizeof_vbuf);          // alpha fix (doubt if it's really need)
		break;

	case ray_paint_mode::dim:
	{
		// TODO: rewirte with SSE2
		auto* p = vbuf[vid.buf];
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
	for (int i = 0xF0; i < 0x100; i++)
	{
		comp.cram[i] = spec_colors[i - 0xF0];
		update_clut(i);
	}
}
