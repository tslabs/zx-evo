#pragma once
#include "sysdefs.h"

u8 Rm(u32 addr);
void Wm(u32 addr, u8 val);
u8 DbgRm(u32 addr);
void DbgWm(u32 addr, u8 val);

void reset(rom_mode mode);
void reset_sound(void);
void set_clk(void);

class TMainZ80 : public Z80
{
public:
	TMainZ80(u32 Idx,
		t_bank_names BankNames, step_fn Step, delta_fn Delta,
		set_lastt_fn SetLastT, u8* membits, const t_mem_if* FastMemIf, const t_mem_if* DbgMemIf) :
		Z80(Idx, BankNames, Step, Delta, SetLastT, membits, FastMemIf, DbgMemIf) { }

	u8* direct_mem(unsigned addr) const override; // get direct memory pointer in debuger

	u8 rd(u32 addr) override;

	u8 m1_cycle() override;
	u8 in(unsigned port) override;
	void out(unsigned port, u8 val) override;
	u8 int_vec() override;
	void check_next_frame() override;
	void retn() override;

	void handle_int(Z80& cpu, u8 vector) override;
};