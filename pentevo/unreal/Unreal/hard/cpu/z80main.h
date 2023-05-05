#pragma once
#include "sysdefs.h"

void reset(rom_mode mode);
void reset_sound(void);
void set_clk(void);

constexpr auto ts_cache_size = 512;
constexpr auto z80_pc_history_size = 32;

struct debug_context_t
{
	pc_history_t pc_hist[z80_pc_history_size];
	u32 pc_hist_ptr;
};

class TMainZ80 : public Z80
{
private:
	debug_context_t& dbg_context_;


public:
	u32 tscache_addr[ts_cache_size]{};
	u8 tscache_data[ts_cache_size]{};

	TMainZ80(u32 Idx, t_bank_names BankNames, step_fn Step, delta_fn Delta, set_lastt_fn SetLastT, u8* membits, const t_mem_if* FastMemIf, const t_mem_if* DbgMemIf, debug_context_t &debug_context) :
		Z80(Idx, BankNames, Step, Delta, SetLastT, membits, FastMemIf, DbgMemIf), dbg_context_(debug_context)
	{
	}

	u8* direct_mem(unsigned addr) const override; // get direct memory pointer in debuger
	void direct_wm(unsigned addr, u8 val) override;

	u8 rd(u32 addr) override;

	u8 m1_cycle() override;
	u8 in(unsigned port) override;
	void out(unsigned port, u8 val) override;
	u8 int_vec() override;
	void check_next_frame() override;
	void retn() override;

	void handle_int(Z80& cpu, u8 vector) override;

};

extern  debug_context_t main_z80_dbg_cntx;
extern TMainZ80 cpu;