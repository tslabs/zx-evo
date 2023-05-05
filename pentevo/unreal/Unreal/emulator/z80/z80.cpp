#include "std.h"
#include "z80.h"

void Z80::reset()
{
	int_flags = ir_ = pc = 0;
	im = 0;
	last_branch = 0;
	int_pend = false;
	int_gate = true;
}

Z80::Z80(u32 idx, t_bank_names BankNames, step_fn Step, delta_fn Delta, set_lastt_fn SetLastT, u8* membits,
         const t_mem_if* FastMemIf, const t_mem_if* DbgMemIf): z80_state_t(),
	       bank_names(BankNames),
	       step(Step),
	       delta(Delta), set_last_t(SetLastT), membits(membits),
	       idx_(idx),
	       fast_mem_if_(FastMemIf), dbg_mem_if_(DbgMemIf)
{
	mem_if_ = FastMemIf;
	tpi = 0;
	rate = (1 << 8);
	dbgbreak = 0;
	dbgchk = 0;
	debug_last_t = 0;
	trace_curs = trace_top = (unsigned)-1;
	trace_mode = 0;
	mem_curs = mem_top = 0;
	pc_trflags = nextpc = 0;
	dbg_stophere = dbg_stopsp = (unsigned)-1;
	dbg_loop_r1 = 0;
	dbg_loop_r2 = 0xFFFF;
	int_pend = false;
	int_gate = true;
	nmi_in_progress = false;
}

u32 Z80::get_idx() const
{
	return idx_;
}

void Z80::set_tpi(u32 Tpi)
{
	tpi = Tpi;
}

void Z80::set_fast_mem_if()
{
	mem_if_ = fast_mem_if_;
}

void Z80::set_dbg_mem_if()
{
	mem_if_ = dbg_mem_if_;
}

u8 Z80::direct_rm(unsigned addr) const
{
	return *direct_mem(addr);
}

void Z80::direct_wm(unsigned addr, u8 val)
{
	*direct_mem(addr) = val;
}

u8 Z80::rd(const u32 addr)
{
	tt += rate * 3;
	return mem_if_->rm(addr);
}

void Z80::wd(const u32 addr, const u8 val)
{
	tt += rate * 3;
	mem_if_->wm(addr, val);
}

void Z80::handle_int(Z80& cpu, u8 vector)
{
	unsigned intad;
	if (cpu.im < 2) {
		intad = 0x38;
	}
	else { // im2
		const unsigned vec = vector + cpu.i * 0x100;
		intad = cpu.mem_if_->rm(vec) + 0x100 * cpu.mem_if_->rm(vec + 1);
	}

	if (cpu.direct_rm(cpu.pc) == 0x76) // int on halt command
		cpu.pc++;

	CPUTACT(((cpu.im < 2) ? 13 : 19) - 3);
	cpu.mem_if_->wm(--cpu.sp, cpu.pch);
	cpu.mem_if_->wm(--cpu.sp, cpu.pcl);
	cpu.pc = intad;
	cpu.memptr = intad;
	cpu.halted = 0;
	cpu.iff1 = cpu.iff2 = 0;
	cpu.int_pend = false;
}
