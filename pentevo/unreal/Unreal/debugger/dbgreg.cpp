#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgreg.h"

const t_reg_layout regs_layout[] =
{
   { offsetof(TZ80State, a)     ,  8,  3, 0, 0, 1, 0, 2 }, //  0 a
   { offsetof(TZ80State, f)     ,  8,  5, 0, 0, 5, 1, 2 }, //  1 f
   { offsetof(TZ80State, bc)    , 16,  3, 1, 2, 6, 0, 3 }, //  2 bc
   { offsetof(TZ80State, de)    , 16,  3, 2, 3, 7, 2, 4 }, //  3 de
   { offsetof(TZ80State, hl)    , 16,  3, 3, 4, 8, 3, 4 }, //  4 hl
   { offsetof(TZ80State, alt.af), 16, 11, 0, 1, 9, 5, 6 }, //  5 af'
   { offsetof(TZ80State, alt.bc), 16, 11, 1, 2,10, 5, 7 }, //  6 bc'
   { offsetof(TZ80State, alt.de), 16, 11, 2, 3,11, 6, 8 }, //  7 de'
   { offsetof(TZ80State, alt.hl), 16, 11, 3, 4,12, 7, 8 }, //  8 hl'
   { offsetof(TZ80State, sp)    , 16, 19, 0, 5,13, 9,10 }, //  9 sp
   { offsetof(TZ80State, pc)    , 16, 19, 1, 6,10, 9,11 }, // 10 pc
   { offsetof(TZ80State, ix)    , 16, 19, 2, 7,15,10,12 }, // 11 ix
   { offsetof(TZ80State, iy)    , 16, 19, 3, 8,18,11,12 }, // 12 iy
   { offsetof(TZ80State, i)     ,  8, 28, 0, 9,14,13,16 }, // 13 i
   { offsetof(TZ80State, r_low) ,  8, 30, 0,13,14,14,17 }, // 14 r
   { offsetof(TZ80State, im)    ,  2, 26, 2,11,16,13,20 }, // 15 im
   { offsetof(TZ80State, iff1)  ,  1, 30, 2,15,17,13,24 }, // 16 iff1
   { offsetof(TZ80State, iff2)  ,  1, 31, 2,16,17,14,25 }, // 17 iff2
   { offsetof(TZ80State, f)     , 37, 24, 3,12,19,15,18 }, // 18 SF
   { offsetof(TZ80State, f)     , 36, 25, 3,18,20,15,19 }, // 19 ZF
   { offsetof(TZ80State, f)     , 35, 26, 3,19,21,15,20 }, // 20 F5
   { offsetof(TZ80State, f)     , 34, 27, 3,20,22,15,21 }, // 21 HF
   { offsetof(TZ80State, f)     , 33, 28, 3,21,23,15,22 }, // 22 F3
   { offsetof(TZ80State, f)     , 32, 29, 3,22,24,16,23 }, // 23 PV
   { offsetof(TZ80State, f)     , 31, 30, 3,23,25,16,24 }, // 24 NF
   { offsetof(TZ80State, f)     , 30, 31, 3,24,25,17,25 }, // 25 CF
};

const size_t regs_layout_count = _countof(regs_layout);

void showregs()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	const auto& prevcpu = t_cpu_mgr::prev_cpu();

	const u8 atr = (activedbg == wndregs) ? w_sel : w_norm;
	char line[40];
	tprint(regs_x, regs_y + 0, "af:**** af'**** sp:**** ir: ****", atr);
	tprint(regs_x, regs_y + 1, "bc:**** bc'**** pc:**** t:******", atr);
	tprint(regs_x, regs_y + 2, "de:**** de'**** ix:**** im?,i:**", atr);
	tprint(regs_x, regs_y + 3, "hl:**** hl'**** iy:**** ########", atr);

	if (cpu.halted && !cpu.iff1)
	{
		tprint(regs_x + 26, regs_y + 1, "DiHALT", (activedbg == wndregs) ? w_dihalt1 : w_dihalt2);
	}
	else
	{
		sprintf(line, "%6d", cpu.t);
		tprint(regs_x + 26, regs_y + 1, line, atr);
	}

	cpu.r_low = (cpu.r_low & 0x7F) + cpu.r_hi;
	for (unsigned i = 0; i < regs_layout_count; i++)
	{
		const unsigned mask = (1 << regs_layout[i].width) - 1;
		const unsigned val = mask & *reinterpret_cast<unsigned*>(PCHAR(static_cast<TZ80State*>(&cpu)) + regs_layout[i].offs);
		auto atr1 = atr;
		if (activedbg == wndregs && i == regs_curs)
			atr1 = w_curs;
		if (val != (mask & *reinterpret_cast<unsigned*>(PCHAR(&prevcpu) + regs_layout[i].offs)))
			atr1 |= 0x08;

		char bf[16];
		switch (regs_layout[i].width)
		{
		case  8: sprintf(bf, "%02X", val); break;
		case 16: sprintf(bf, "%04X", val); break;
		case  1:
		case  2: sprintf(bf, "%X", val); break;
		default: *bf = 0;
		}
		tprint(regs_x + regs_layout[i].x, regs_y + regs_layout[i].y, bf, atr1);
	}
	static const char flg[] = "SZ5H3PNCsz.h.pnc";
	for (u8 q = 0; q < 8; q++)
	{
		unsigned ln;
		auto atr1 = atr;
		if (activedbg == wndregs && regs_curs == unsigned(q + 18)) atr1 = w_curs;
		ln = flg[q + ((cpu.af & (0x80 >> q)) ? 0 : 8)];
		if ((0x80 >> q)&(cpu.f^prevcpu.f)) atr1 |= 0x08;
		tprint(regs_x + 24 + q, regs_y + 3, reinterpret_cast<char*>(&ln), atr1);
	}
	tprint(regs_x, regs_y - 1, "regs", w_title);
	frame(regs_x, regs_y, 32, 4, FRAME);
}

void rleft() { regs_curs = regs_layout[regs_curs].lf; }
void rright() { regs_curs = regs_layout[regs_curs].rt; }
void rup() { regs_curs = regs_layout[regs_curs].up; }
void rdown() { regs_curs = regs_layout[regs_curs].dn; }
void renter()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	debugscr();
	debugflip();
	const auto sz = regs_layout[regs_curs].width;
	auto val = ((1 << sz) - 1) & *reinterpret_cast<unsigned*>(PCHAR(static_cast<TZ80State*>(&cpu)) + regs_layout[regs_curs].offs);
	const auto ptr = PUCHAR(static_cast<TZ80State*>(&cpu)) + regs_layout[regs_curs].offs;
	if ((sz == 8 || sz == 16) && ((input.lastkey >= '0' && input.lastkey <= '9') || (input.lastkey >= 'A' && input.lastkey <= 'F')))
		PostThreadMessage(GetCurrentThreadId(), WM_KEYDOWN, input.lastkey, 1);
	switch (sz)
	{
	case 8:
		val = input2(regs_x + regs_layout[regs_curs].x, regs_y + regs_layout[regs_curs].y, val);
		if (val != UINT_MAX)
			*ptr = val;
		break;
	case 16:
		val = input4(regs_x + regs_layout[regs_curs].x, regs_y + regs_layout[regs_curs].y, val);
		if (val != UINT_MAX)
			*reinterpret_cast<u16*>(ptr) = val;
		break;
	case 1:
		*ptr ^= 1; break;
	case 2:
		*ptr = (*ptr + 1) % 3; break;
	default: // flags
		*ptr ^= (1 << (sz - 30));
	}
	cpu.r_hi = cpu.r_low & 0x80;
}
void ra() { regs_curs = 0; input.lastkey = 0; renter(); }
void rf() { regs_curs = 1; input.lastkey = 0; renter(); }
void rbc() { regs_curs = 2; input.lastkey = 0; renter(); }
void rde() { regs_curs = 3; input.lastkey = 0; renter(); }
void rhl() { regs_curs = 4; input.lastkey = 0; renter(); }
void rsp() { regs_curs = 9; input.lastkey = 0; renter(); }
void rpc() { regs_curs = 10; input.lastkey = 0; renter(); }
void rix() { regs_curs = 11; input.lastkey = 0; renter(); }
void riy() { regs_curs = 12; input.lastkey = 0; renter(); }
void ri() { regs_curs = 13; input.lastkey = 0; renter(); }
void rr() { regs_curs = 14; input.lastkey = 0; renter(); }
void rm() { regs_curs = 15; renter(); }
void r_1() { regs_curs = 16; renter(); }
void r_2() { regs_curs = 17; renter(); }
void rSF() { regs_curs = 18; renter(); }
void rZF() { regs_curs = 19; renter(); }
void rF5() { regs_curs = 20; renter(); }
void rHF() { regs_curs = 21; renter(); }
void rF3() { regs_curs = 22; renter(); }
void rPF() { regs_curs = 23; renter(); }
void rNF() { regs_curs = 24; renter(); }
void rCF() { regs_curs = 25; renter(); }

void rcodejump()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (regs_layout[regs_curs].width == 16)
	{
		activedbg = wndtrace;
		cpu.trace_curs = cpu.trace_top = *reinterpret_cast<u16*>(PCHAR(static_cast<TZ80State*>(&cpu)) + regs_layout[regs_curs].offs);
	}
}
void rdatajump()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (regs_layout[regs_curs].width == 16)
	{
		activedbg = wndmem;
		editor = ed_mem;
		cpu.mem_curs = *reinterpret_cast<u16*>(PCHAR(static_cast<TZ80State*>(&cpu)) + regs_layout[regs_curs].offs);
	}
}

char dispatch_regs()
{
	if ((input.lastkey >= '0' && input.lastkey <= '9') || (input.lastkey >= 'A' && input.lastkey <= 'F'))
	{
		renter();
		return 1;
	}
	return 0;
}
