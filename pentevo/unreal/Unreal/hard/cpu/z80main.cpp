#include "std.h"
#include "emul.h"
#include "funcs.h"
#include "vars.h"
#include "draw.h"
#include "hard/memory.h"
#include "sound/sound.h"
#include "hard/tsconf.h"
#include "z80main.h"

#include "util.h"
#include "z80main_fn.h"
#include "emulator/debugger/dbgoth.h"
#include "sound/saa1099.h"
#include "sound/dev_moonsound.h"

static constexpr t_mem_if fast_mem_if = { rm<false>, wm<false> };
static constexpr t_mem_if dbg_mem_if = { rm<true>, wm<true> };

debug_context_t main_z80_dbg_cntx{};

TMainZ80 cpu(0, BankNames, step, delta, SetLastT, membits, &fast_mem_if, &dbg_mem_if, main_z80_dbg_cntx);

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


u8* TMainZ80::direct_mem(unsigned addr) const
{
	return am_r(addr);
}

void TMainZ80::direct_wm(unsigned addr, u8 val)
{
	*direct_mem(addr) = val;
	const u16 cache_pointer = addr & 0x1FF;
	tscache_addr[cache_pointer] = -1; // write invalidates flag
}

u8 TMainZ80::rd(u32 addr)
{
	const u8 tempbyte = mem_if_->rm(addr);

	// Align 14MHz CPU memory request to 7MHz DRAM cycle
	// request can be satisfied only in the next DRAM cycle
	if (comp.ts.cache_miss && rate == 0x40)
		tt += (tt & 0x40) ? 0x40 * 6 : 0x40 * 5;
	else
		tt += rate * 3;

	return tempbyte;
}

u8 TMainZ80::m1_cycle()
{
	dbg_context_.pc_hist_ptr++;
	if (dbg_context_.pc_hist_ptr >= z80_pc_history_size)
		dbg_context_.pc_hist_ptr = 0;

	dbg_context_.pc_hist[dbg_context_.pc_hist_ptr].addr = pc;
	dbg_context_.pc_hist[dbg_context_.pc_hist_ptr].page = comp.ts.page[(cpu.pc >> 14) & 3];

	r_low++;
	const u8 tempbyte = mem_if_->rm(pc++);

	// Align 14MHz CPU memory request to 7MHz DRAM cycle
	// request can be satisfied only in the next DRAM cycle
	if (comp.ts.cache_miss && rate == 0x40)
		tt = (tt + 0x40 * 7) & 0xFFFFFF80;
	else
		tt += rate * 4;

	cpu.opcode = tempbyte;

	return tempbyte;
}

u8 TMainZ80::in(unsigned port)
{
	return ::in(port);
}

void TMainZ80::out(unsigned port, u8 val)
{
	::out(port, val);
}

u8 TMainZ80::int_vec()
{

	tt += rate * 3; // pass 3 tacts before read INT vector

	if (conf.mem_model == MM_TSL)
	{
		// check status of frame INT
		ts_frame_int(comp.ts.vdos || comp.ts.vdos_m1);

		if (comp.ts.intctrl.frame_pend)
			return comp.ts.im2vect[INT_FRAME];

		if (comp.ts.intctrl.line_pend)
			return comp.ts.im2vect[INT_LINE];

		if (comp.ts.intctrl.dma_pend)
			return comp.ts.im2vect[INT_DMA];


		return 0xFF;
	}

	return (comp.flags & CF_Z80FBUS) ? u8(rdtsc() & 0xFF) : 0xFF;
}

void TMainZ80::check_next_frame()
{
	if (t >= conf.frame)
	{
		comp.t_states += conf.frame;
		t -= conf.frame;
		eipos -= conf.frame;
		comp.frame_counter++;
		//int_pend = true;
		if (conf.mem_model == MM_TSL)
		{
			comp.ts.intctrl.line_t = comp.ts.intline ? 0 : conf.t_line;
			comp.ts.intctrl.last_cput -= conf.frame;
		}
	}
}

void TMainZ80::retn()
{
	nmi_in_progress = false;
	set_banks();
}

void TMainZ80::handle_int(Z80& cpu, u8 vector)
{
	Z80::handle_int(cpu, vector);

	if (conf.mem_model == MM_TSL)
	{
		if (comp.ts.intctrl.frame_pend) comp.ts.intctrl.frame_pend = 0;
		else
			if (comp.ts.intctrl.line_pend)  comp.ts.intctrl.line_pend = 0;
			else
				if (comp.ts.intctrl.dma_pend)   comp.ts.intctrl.dma_pend = 0;
	}
}
