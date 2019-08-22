#include "std.h"
#include "dbgwidgets.h"
#include "dbgtsconf.h"
#include "emul.h"
#include "vars.h"

auto tsconf_regs = dbg_canvas(tsconf_base_x, tsconf_base_y);

const char *d_vmode[] = { "ZX  ", "16c ", "256c", "text" };
const char *d_rres[] = { "256x192", "320x200", "320x240", "360x288" };
const char *d_clk[] = { "3.5M", "7M", "14M", "unk" };
const char *d_lock[] = { "512", "128", "aut", "1MB" };
const char *d_dma[] =
{
	"unk    ",
	"unk    ",
	"RAM-RAM",
	"BLT-RAM",
	"SPI-RAM",
	"RAM-SPI",
	"IDE-RAM",
	"RAM-IDE",
	"FIL-RAM",
	"RAM-CRM",
	"unk    ",
	"RAM-SFL",
	"unk    ",
	"unk    ",
	"unk    ",
	"unk    "
};

const char *d_rw[] =
{
	"WRITE",
	"READ"
};

const char *d_dmast[] =
{
	"RAM  ",
	"BLT  ",
	"SPI_R",
	"SPI_W",
	"IDE_R",
	"IDE_W",
	"FILL ",
	"CRAM ",
	"SFILE",
	"INIT ",
	"NOP  "
};

class vconfig_control: public dbg_control
{
public:
	explicit vconfig_control() : dbg_control(6) { }

	void on_paint() override
	{
		draw_reg_frame("VConfig", &comp.ts.vconf);
		draw_bit("RRES", 76, d_rres, comp.ts.rres);
		draw_bit("NOGFX", 5, comp.ts.nogfx);
		draw_bit("NOTSU", 4, comp.ts.notsu);
		draw_bit("GFXOVR", 3, comp.ts.gfxovr);
		draw_bit("FT_EN", 2, comp.ts.ft_en);
		draw_bit("VMODE", 10, d_vmode, comp.ts.vmode);
	}
};

class tsconfig_control : public dbg_control
{
public:
	explicit tsconfig_control() : dbg_control(6) { }

	void on_paint() override
	{
		draw_reg_frame("TSConfig", &comp.ts.tsconf);
		draw_bit("S_EN", 7, comp.ts.s_en);
		draw_bit("T1_EN", 6, comp.ts.t1_en);
		draw_bit("T0_EN", 5, comp.ts.t0_en);
		draw_bit("T1Z_EN", 3, comp.ts.t1z_en);
		draw_bit("T0Z_EN", 2, comp.ts.t0z_en);
		draw_bit("TS_EXT", 0, comp.ts.tsconf);
	}
};

class sysconfig_control : public dbg_control
{
public:
	explicit sysconfig_control() : dbg_control(2) { }

	void on_paint() override
	{
		draw_reg_frame("SysConfig", &comp.ts.sysconf);
		draw_bit("CACHE_EN", 2, comp.ts.sysconf >> 2);
		draw_bit("ZCLK", 10, d_clk, comp.ts.zclk);
	}
};

class cache_config_control : public dbg_control
{
public:
	explicit cache_config_control() : dbg_control(4) { }

	void on_paint() override
	{
		draw_reg_frame("CacheConfig", &comp.ts.cacheconf);
		draw_bit("EN_C000", 4, comp.ts.cacheconf >> 3);
		draw_bit("EN_8000", 4, comp.ts.cacheconf >> 2);
		draw_bit("EN_4000", 4, comp.ts.cacheconf >> 1);
		draw_bit("EN_0000", 4, comp.ts.cacheconf >> 0);
	}
};

class memconfig_control : public dbg_control
{
public:
	explicit memconfig_control() : dbg_control(5) { }

	void on_paint() override
	{
		draw_reg_frame("MemConfig", &comp.ts.memconf);
		draw_bit("LCK128", 76, comp.ts.s_en);
		draw_bit("W0_RAM", 3, comp.ts.w0_ram ^ 1);
		draw_bit("W0_MAP", 2, comp.ts.w0_map_n ^ 1);
		draw_bit("W0_WE", 1, comp.ts.w0_we);
		draw_bit("ROM128", 0, comp.ts.rom128);
	}
};

class bitmap_control : public dbg_control
{
public:
	explicit bitmap_control() : dbg_control(3) { }

	void on_paint() override
	{
		set_xy(19, 0);
		draw_led("EN", !comp.ts.nogfx);

		draw_reg_frame("Bitmap");
		draw_port("VPage", comp.ts.vpage);
		draw_hl_port('X', comp.ts.g_xoffsh, comp.ts.g_xoffsl, comp.ts.g_xoffs);
		draw_hl_port('Y', comp.ts.g_yoffsh, comp.ts.g_yoffsl, comp.ts.g_yoffs);
	}
};

class sprites_control : public dbg_control
{
public:
	explicit sprites_control() : dbg_control(1) { }

	void on_paint() override
	{
		draw_reg_frame("Sprites");
		draw_port("SGPage", comp.ts.sgpage);
	}
};

class tiles0_control : public dbg_control
{
public:
	explicit tiles0_control() : dbg_control(3) { }

	void on_paint() override
	{
		draw_reg_frame("Tiles0");
		set_xy(14, 0);
		draw_led("Z_EN", comp.ts.t0z_en);
		draw_led("EN", comp.ts.t0_en);
		next_row();

		draw_port("T0GPage", comp.ts.t0gpage[2]);
		draw_hex16("X", 16, comp.ts.t0_xoffs);
		draw_hex16("Y", 16, comp.ts.t0_yoffs);
	}
};

class tiles1_control : public dbg_control
{
public:
	explicit tiles1_control() : dbg_control(3) { }

	void on_paint() override
	{
		draw_reg_frame("Tiles1");

		set_xy(14, 0);
		draw_led("Z_EN", comp.ts.t1z_en);
		draw_led("EN", comp.ts.t1_en);
		next_row();

		draw_port("T1GPage", comp.ts.t1gpage[2]);
		draw_hex16("X", 16, comp.ts.t1_xoffs);
		draw_hex16("Y", 16, comp.ts.t1_yoffs);
	}
};

class palsel_control : public dbg_control
{
public:
	explicit palsel_control() : dbg_control(3) { }

	void on_paint() override
	{
		draw_reg_frame("PalSel", &comp.ts.palsel);
		draw_bit_h("T1PAL", 76, comp.ts.t1pal << 2);
		draw_bit_h("T0PAL", 54, comp.ts.t0pal << 2);
		draw_bit_h("GPAL",  30, comp.ts.gpal);
	}
};

class fmaddr_control : public dbg_control
{
public:
	explicit fmaddr_control() : dbg_control(2) { }

	void on_paint() override
	{
		draw_reg_frame("FMAddr", &comp.ts.fmaddr);
		draw_bit("FM_EN", 4, comp.ts.fm_en);
		draw_hex16("FM_MAPS", 30, comp.ts.fm_addr << 12);
	}
};

class misc_control : public dbg_control
{
public:
	explicit misc_control() : dbg_control(3) { }

	void on_paint() override
	{
		draw_reg_frame("Misc");
		set_xy(6, 0);

		draw_led("FDD", comp.ts.fddvirt >> 3);
		draw_led("FDC", comp.ts.fddvirt >> 2);
		draw_led("FDB", comp.ts.fddvirt >> 1);
		draw_led("FDA", comp.ts.fddvirt >> 0);
		
		next_row();
		draw_port("   TMPage", comp.ts.tmpage);
		draw_port("   Border", comp.ts.border);
		draw_port("   FDDVirt", comp.ts.fddvirt);
	}
};

class mempages_control : public dbg_control
{
public:
	explicit mempages_control() : dbg_control(4) { }

	void on_paint() override
	{
		draw_reg_frame("MemPages");
		draw_port("   Page0", comp.ts.page[0]);
		draw_port("   Page1", comp.ts.page[1]);
		draw_port("   Page2", comp.ts.page[2]);
		draw_port("   Page3", comp.ts.page[3]);
	}
};

class dma_control : public dbg_control
{
public:
	explicit dma_control() : dbg_control(15) { }

	void on_paint() override
	{
		draw_reg_frame("DMA");

		set_xy(15, 0);
		draw_led("ACTIVE", comp.ts.dma.state != DMA_ST_NOP);
		next_row();

		// draw_xhl_port('S', comp.ts.saddrx, comp.ts.saddrh, comp.ts.saddrl);
		draw_hex24("     SRC", -1, comp.ts.saddr);
		draw_hex24("CURR SRC", -1, comp.ts.dma.saddr);
		next_row();

		// draw_xhl_port('D', comp.ts.daddrx, comp.ts.daddrh, comp.ts.daddrl);
		draw_hex24("     DST", -1, comp.ts.daddr);
		draw_hex24("CURR DST", -1, comp.ts.dma.daddr);
		next_row();

		draw_hex8_inline("     NUM", comp.ts.dmanum);
		draw_hex8_inline(" LEN", comp.ts.dmalen);
		next_row();
		draw_hex8_inline("CURR NUM", u8(comp.ts.dma.num));
		draw_hex8_inline(" LEN", u8(comp.ts.dma.len));
		next_row();
		next_row();
		draw_port("CTRL", comp.ts.dma.ctrl);
		draw_bit("OPT", 6, comp.ts.dma.opt);
		draw_bit("S_ALIGN", 5, comp.ts.dma.s_algn);
		draw_bit("D_ALIGN", 4, comp.ts.dma.d_algn);
		draw_bit("A_SZ", 3, comp.ts.dma.asz);
		draw_bit("DDEV", 20, d_dma, (comp.ts.dma.dev << 1) + comp.ts.dma.rw);
	}
};

class interrupt_control : public dbg_control
{
public:
	explicit interrupt_control() : dbg_control(5) { }

	void on_paint() override
	{
		draw_reg_frame("Interrupt");
		draw_port("HSINT", comp.ts.hsint);

		draw_dec_inline(" h", comp.ts.hsint << 1);
		next_row();

		draw_port("VSINTH", comp.ts.vsinth);
		draw_port("VSINTL", comp.ts.vsintl);

		draw_dec_inline(" v", comp.ts.vsint);
		draw_dec_inline(" inc", comp.ts.vsint >> 4);
	}
};

class intmask_control : public dbg_control
{
public:
	explicit intmask_control() : dbg_control(3) { }

	void on_paint() override
	{
		draw_reg_frame("IntMask");
		draw_bit("DMA", 2, comp.ts.intdma);
		draw_bit("LINE", 1, comp.ts.intline);
		draw_bit("FRAME", 0, comp.ts.intframe);
	}
};

void init_regs_page()
{
	auto &col_0 = tsconf_regs.create_column(23);
	auto &col_1 = tsconf_regs.create_column(23);
	auto &col_2 = tsconf_regs.create_column(23);

	col_0.add_item(new vconfig_control());
	col_0.add_item(new tsconfig_control());
	col_0.add_item(new sysconfig_control());
	col_0.add_item(new cache_config_control());
	col_0.add_item(new memconfig_control());

	col_1.add_item(new bitmap_control());
	col_1.add_item(new tiles0_control());
	col_1.add_item(new tiles1_control());
	col_1.add_item(new palsel_control());
	col_1.add_item(new misc_control());
	col_1.add_item(new fmaddr_control());
	col_1.add_item(new mempages_control());

	col_2.add_item(new sprites_control());
	col_2.add_item(new dma_control());
	col_2.add_item(new interrupt_control());
	col_2.add_item(new intmask_control());
}

void init_tsconf()
{
	init_regs_page();
}

bool on_mouse_tsconf(int mx, int my)
{
	if (mx >= tsconf_regs.get_x())
		return tsconf_regs.handle_mouse(mx - tsconf_regs.get_x(), my);

	auto need_repaint = false;
	tsconf_regs.iterate([&need_repaint](dbg_control* control)
	{
		if (control->is_active) {
			need_repaint = true;
			control->is_active = false;
		}
	});

	return need_repaint;
}

void show_tsconf()
{
	tsconf_regs.paint();
}
