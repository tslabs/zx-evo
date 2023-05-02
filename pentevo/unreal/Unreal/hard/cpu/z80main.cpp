#include "std.h"
#include "emul.h"
#include "funcs.h"
#include "vars.h"
#include "draw.h"
#include "hard/memory.h"
#include "sound/sound.h"
#include "hard/tsconf.h"
#include "z80main.h"
#include "z80main_fn.h"
#include "sound/saa1099.h"
#include "sound/dev_moonsound.h"


u8 Rm(u32 addr)
{
	return rm<false>(addr);
}

void Wm(u32 addr, u8 val)
{
	wm<false>(addr, val);
}

u8 DbgRm(u32 addr)
{
	return rm<true>(addr);
}

void DbgWm(u32 addr, u8 val)
{
	wm<true>(addr, val);
}

void reset_sound(void)
{
	ay[0].reset();
	ay[1].reset();
	Saa1099.reset();
	zxmmoonsound.reset();

	if (conf.sound.ay_scheme == ay_scheme::chrv)
		out(0xfffd, 0xff);

#ifdef MOD_GS
	if (conf.sound.gsreset)
		reset_gs();
#endif
}

void reset(rom_mode mode)
{
	comp.pEFF7 &= conf.EFF7_mask;
	comp.pEFF7 |= EFF7_GIGASCREEN; // [vv] disable turbo
	{
		conf.frame = frametime;
		cpu.set_tpi(conf.frame);
		//                if ((conf.mem_model == MM_PENTAGON)&&(comp.pEFF7 & EFF7_GIGASCREEN))conf.frame = 71680; //removed 0.37
		apply_sound();
	} //Alone Coder 0.36.4
	comp.t_states = 0; comp.frame_counter = 0;
	comp.p7FFD = 0;

	comp.ulaplus_mode = 0;
	comp.ulaplus_reg = 0;

	tsinit();

	if (conf.mem_model == MM_TSL)
		set_clk();		// turbo 2x (7MHz) for TS-Conf
	else
		TURBO(1);		// turbo 1x (3.5MHz) for all other clones

	switch (mode)
	{
	case rom_mode::sys: {comp.ts.memconf = 4; break; }
	case rom_mode::dos: {comp.ts.memconf = 0; break; }
	case rom_mode::s128: {comp.ts.memconf = 0; break; }
	case rom_mode::sos: {comp.ts.memconf = 0; break; }
	default: ;
	}

	comp.flags = 0;
	comp.active_ay = 0;

	cpu.reset();
	reset_tape();
	reset_sound();

	load_spec_colors();

	comp.ide_hi_byte_r = 0;
	comp.ide_hi_byte_w = 0;
	comp.ide_hi_byte_w1 = 0;
	hdd.reset();
	input.buffer.Enable(false);

	if ((!conf.trdos_present && mode == rom_mode::dos) ||
		(!conf.cache && mode == rom_mode::cache))
		mode = rom_mode::sos;

	set_mode(mode);
}

void set_clk(void)
{
	switch (comp.ts.zclk)
	{
	case 0: TURBO(1); break;
	case 1: TURBO(2); break;
	case 2: TURBO(4); break;
	case 3: TURBO(4); break;
	default: ;
	}
	comp.ts.intctrl.frame_len = (conf.intlen * cpu.rate) >> 8;
}
