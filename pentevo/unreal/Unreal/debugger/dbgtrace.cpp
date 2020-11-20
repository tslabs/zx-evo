#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgtrace.h"
#include "dbglabls.h"
#include "dbgpaint.h"
#include "dbgcmd.h"
#include "memory.h"
#include "z80asm.h"
#include "op_system.h"
#include "util.h"
#include "draw.h"
#include "emulkeys.h"

extern VCTR vid;

int disasm_line(unsigned addr, char *line)
{
	auto& cpu = t_cpu_mgr::get_cpu();
	u8 dbuf[16 + 129/*Alone Code 0.36.7*/];
	int i; //Alone Coder 0.36.7
	for (/*int*/ i = 0; i < 16; i++) dbuf[i] = cpu.DirectRm(addr + i);
	sprintf(line, "%04X ", addr);
	auto ptr = 5;
	const int len = disasm(dbuf, addr, trace_labels) - dbuf;
	//8000 ..DDCB0106 rr (ix+1)
	if (trace_labels)
	{
		const auto lbl = mon_labels.find(am_r(addr));
		//      if (lbl) for (int k = 0; k < 10 && lbl[k]; line[ptr++] = lbl[k++]); //Alone Coder
		if (lbl) for (int k = 0; (k < 10) && lbl[k]; )line[ptr++] = lbl[k++]; //Alone Coder
	}
	else
	{
		int len1 = len;
		if (len > 4) len1 = 4, *reinterpret_cast<short*>(line + ptr) = WORD2('.', '.'), ptr += 2;
		for (i = len - len1; i < len; i++)
			sprintf(line + ptr, "%02X", dbuf[i]), ptr += 2;
	}

	while (ptr < 16) line[ptr++] = ' ';
	strcpy(line + ptr, asmbuf);
	return len;
}

#define TWF_BRANCH  0x010000
#define TWF_BRADDR  0x020000
#define TWF_LOOPCMD 0x040000
#define TWF_CALLCMD 0x080000
#define TWF_BLKCMD  0x100000
unsigned tracewndflags()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	auto readptr = cpu.pc, base = cpu.hl;
	u8 opcode; u8 ed = 0;
	for (;;)
	{
		opcode = cpu.DirectRm(readptr++);
		if (opcode == 0xDD)
			base = cpu.ix;
		else if (opcode == 0xFD)
			base = cpu.iy;
		else if (opcode == 0xED)
			ed = 1;
		else
			break;
	}

	unsigned fl = 0;
	if (opcode == 0x76) // halt
	{
		return TWF_BLKCMD;
	}

	if (ed)
	{
		if ((opcode & 0xF4) == 0xB0) // ldir/lddr | cpir/cpdr | inir/indr | otir/otdr
			return TWF_BLKCMD;

		if ((opcode & 0xC7) != 0x45)
			return 0; // reti/retn

	ret:
		return (cpu.DirectRm(cpu.sp) | (cpu.DirectRm(cpu.sp + 1) << 8U)) | TWF_BRANCH | TWF_BRADDR;
	}

	if (opcode == 0xC9) // ret
		goto ret;
	if (opcode == 0xC3) // jp
	{
	jp: return (cpu.DirectRm(readptr) | (cpu.DirectRm(readptr + 1) << 8U)) | TWF_BRANCH | fl;
	}
	if (opcode == 0xCD) // call
	{
		fl = TWF_CALLCMD;
		goto jp;
	}

	static const u8 flags[] = { ZF,CF,PV,SF };

	if ((opcode & 0xC1) == 0xC0)
	{
		const auto flag = flags[(opcode >> 4) & 3];
		u8 res = cpu.f & flag;
		if (!(opcode & 0x08))
			res ^= flag;
		if (!res)
			return 0;
		if ((opcode & 0xC7) == 0xC0) // ret cc
			goto ret;
		if ((opcode & 0xC7) == 0xC4) // call cc
		{
			fl = TWF_CALLCMD;
			goto jp;
		}
		if ((opcode & 0xC7) == 0xC2) // jp cc
		{
			fl = TWF_LOOPCMD;
			goto jp;
		}
	}

	if (opcode == 0xE9)
		return base | TWF_BRANCH | TWF_BRADDR; // jp (hl/ix/iy)

	if ((opcode & 0xC7) == 0xC7)
		return (opcode & 0x38) | TWF_CALLCMD | TWF_BRANCH; // rst #xx

	if ((opcode & 0xC7) == 0x00)
	{
		if (!opcode || opcode == 0x08)
			return 0;
		const int offs = static_cast<char>(cpu.DirectRm(readptr++));
		const unsigned addr = (offs + readptr) | TWF_BRANCH;
		if (opcode == 0x18)
			return addr; // jr
		if (opcode == 0x10)
			return (cpu.b == 1) ? 0 : addr | TWF_LOOPCMD; // djnz

		const auto flag = flags[(opcode >> 4) & 1]; // jr cc
		u8 res = cpu.f & flag;
		if (!(opcode & 0x08))
			res ^= flag;
		return res ? addr | TWF_LOOPCMD : 0;
	}
	return 0;
}

unsigned trcurs_y;
unsigned asmii;
char asmpc[64], dumppc[12];
const unsigned cs[3][2] = { {0,4}, {5,10}, {16,16} };

void showtrace()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	//   char line[40]; //Alone Coder 0.36.7
	char line[16 + 129]; //Alone Coder 0.36.7

	cpu.trace_curs &= 0xFFFF;
	cpu.trace_top &= 0xFFFF;
	cpu.pc &= 0xFFFF;
	cpu.trace_mode = (cpu.trace_mode + 3) % 3;

	cpu.pc_trflags = tracewndflags();
	cpu.nextpc = (cpu.pc + disasm_line(cpu.pc, line)) & 0xFFFF;
	auto pc = cpu.trace_top;
	asmii = -1;
	const u8 atr0 = (activedbg == wndtrace) ? w_sel : w_norm;
	unsigned ii; //Alone Coder 0.36.7
	for (/*unsigned*/ ii = 0; ii < trace_size; ii++)
	{
		pc &= 0xFFFF; cpu.trpc[ii] = pc;
		const auto len = disasm_line(pc, line);
		auto ptr = line + strlen(line);
		while (ptr < line + 32) *ptr++ = ' '; line[32] = 0;

		u8 atr = (pc == cpu.pc) ? w_tracepos : atr0;
		if (cpu.membits[pc] & MEMBITS_BPX) atr = (atr&~7) | 2;
		tprint(trace_x, trace_y + ii, line, atr);

		if (pc == cpu.trace_curs)
		{
			asmii = ii;
			if (activedbg == wndtrace)
				for (unsigned q = 0; q < cs[cpu.trace_mode][1]; q++)
					txtscr[debug_text_width * debug_text_height + (trace_y + ii) * debug_text_width + trace_x + cs[cpu.trace_mode][0] + q] = w_curs;
		}

		if (cpu.pc_trflags & TWF_BRANCH)
		{
			if (pc == cpu.pc)
			{
				const auto addr = cpu.pc_trflags & 0xFFFF;
				unsigned arr = (addr <= cpu.pc) ? 0x18 : 0x19; // up/down arrow
				const u8 color = (pc == cpu.trace_curs && activedbg == wndtrace && cpu.trace_mode == 2) ? w_trace_jinfo_curs_fg : w_trace_jinfo_nocurs_fg;
				if (cpu.pc_trflags & TWF_BRADDR) sprintf(line, "%04X%c", addr, arr), tprint_fg(trace_x + 32 - 5, trace_y + ii, line, color);
				else tprint_fg(trace_x + 32 - 1, trace_y + ii, reinterpret_cast<char*>(&arr), color);
			}

			if (pc == (cpu.pc_trflags & 0xFFFF))
			{
				unsigned arr = 0x11; // left arrow
				tprint_fg(trace_x + 32 - 1, trace_y + ii, reinterpret_cast<char*>(&arr), w_trace_jarrow_foregr);
			}
		}

		pc += len;
	}
	cpu.trpc[ii] = pc;

	u8 dbuf[16];
	int i; //Alone Coder
	for (/*int*/ i = 0; i < 16; i++) dbuf[i] = cpu.DirectRm(cpu.trace_curs + i);
	const int len = disasm(dbuf, cpu.trace_curs, 0) - dbuf; strcpy(asmpc, asmbuf);
	for (/*int*/ i = 0; i < len && i < 5; i++)
		sprintf(dumppc + i * 2, "%02X", cpu.DirectRm(cpu.trace_curs + i));

	char cpu_num[10];
	_snprintf(cpu_num, sizeof(cpu_num), "Z80(%d)", t_cpu_mgr::get_current_cpu());
	tprint(trace_x, trace_y - 1, cpu_num, w_title);

	char lbr[5];
	_snprintf(lbr, sizeof(lbr), "%04hX", cpu.last_branch);
	tprint(trace_x + 8, trace_y - 1, lbr, w_title);
	frame(trace_x, trace_y, 32, trace_size, FRAME);
}

void c_lbl_import()
{
	mon_labels.import_menu();
}

/* ------------------------------------------------------------- */
unsigned save_pos[8] = { UINT_MAX };
unsigned save_cur[8] = { UINT_MAX };
unsigned stack_pos[32] = { UINT_MAX }, stack_cur[32] = { UINT_MAX };

void push_pos()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	memmove(&stack_pos[1], &stack_pos[0], sizeof stack_pos - sizeof *stack_pos);
	memmove(&stack_cur[1], &stack_cur[0], sizeof stack_cur - sizeof *stack_cur);
	stack_pos[0] = cpu.trace_top; stack_cur[0] = cpu.trace_curs;
}

unsigned cpu_up(unsigned ip)
{
	auto& cpu = t_cpu_mgr::get_cpu();
	u8 buf1[0x10];
	const auto p1 = (ip > sizeof buf1) ? ip - sizeof buf1 : 0;
	for (unsigned i = 0; i < sizeof buf1; i++) buf1[i] = cpu.DirectRm(p1 + i);
	u8 *dispos = buf1, *prev;
	do {
		prev = dispos;
		dispos = disasm(dispos, 0, 0);
	} while (unsigned(dispos - buf1 + p1) < ip);
	return prev - buf1 + p1;
}

void cgoto()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	const auto v = input4(trace_x, trace_y, cpu.trace_top);
	if (v != UINT_MAX)
		cpu.trace_top = cpu.trace_curs = v;
}

void csetpc()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.pc = cpu.trace_curs;
}

void center()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (!cpu.trace_mode)
		sprintf(str, "%04X", cpu.trace_curs);
	else if (cpu.trace_mode == 1)
		strcpy(str, dumppc);
	else
		strcpy(str, asmpc);

	if (input.lastkey != VK_RETURN)
	{
		*str = 0;
		PostThreadMessage(GetCurrentThreadId(), WM_KEYDOWN, input.lastkey, 1);
	}

	for (;;)
	{
		if (!inputhex(trace_x + cs[cpu.trace_mode][0], trace_y + trcurs_y + asmii, cs[cpu.trace_mode][1], cpu.trace_mode < 2))
			break;
		if (!cpu.trace_mode)
		{
			push_pos();
			sscanf(str, "%X", &cpu.trace_top);
			cpu.trace_curs = cpu.trace_top;
			for (unsigned i = 0; i < asmii; i++)
				cpu.trace_top = cpu_up(cpu.trace_top);
			break;
		}
		else if (cpu.trace_mode == 1)
		{
			char *p; //Alone Coder 0.36.7
			for (/*char * */p = str + strlen(str) - 1; p >= str && *p == ' '; *p-- = 0) {}
			u8 dump[8]; unsigned i;
			for (p = str, i = 0; ishex(*p) && ishex(p[1]); p += 2)
				dump[i++] = hex(p);
			if (*p) continue;
			for (unsigned j = 0; j < i; j++)
				cpu.DirectWm(cpu.trace_curs + j, dump[j]);
			break;
		}
		else
		{
			const unsigned sz = assemble_cmd(reinterpret_cast<u8*>(str), cpu.trace_curs);
			if (sz)
			{
				for (unsigned i = 0; i < sz; i++)
					cpu.DirectWm(cpu.trace_curs + i, asmresult[i]);
				showtrace();
				cdown();
				break;
			}
		}
	}
}

char dispatch_trace()
{
	if (input.lastkey >= 'A' && input.lastkey < 'Z')
	{
		center();
		return 1;
	}
	return 0;
}

void cfindtext()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	const auto oldmode = editor; editor = ed_mem;
	const auto rs = find1dlg(cpu.trace_curs);
	editor = oldmode;
	if (rs != UINT_MAX)
		cpu.trace_top = cpu.trace_curs = rs;
}
void cfindcode()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	const auto oldmode = editor; editor = ed_mem;
	const auto rs = find2dlg(cpu.trace_curs);
	editor = oldmode;
	if (rs != UINT_MAX)
		cpu.trace_top = cpu.trace_curs = rs;
}

void cbpx()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.membits[cpu.trace_curs] ^= MEMBITS_BPX;
}

void cfindpc()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.trace_top = cpu.trace_curs = cpu.pc;
}

void cup()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (cpu.trace_curs > cpu.trace_top)
	{
		for (unsigned i = 1; i < trace_size; i++)
			if (cpu.trpc[i] == cpu.trace_curs)
				cpu.trace_curs = cpu.trpc[i - 1];
	}
	else
		cpu.trace_top = cpu.trace_curs = cpu_up(cpu.trace_curs);
}

void cdown()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	for (unsigned i = 0; i < trace_size; i++)
		if (cpu.trpc[i] == cpu.trace_curs)
		{
			cpu.trace_curs = cpu.trpc[i + 1];
			if (i + 1 == trace_size)
				cpu.trace_top = cpu.trpc[1];
			break;
		}
}
void cleft() { t_cpu_mgr::get_cpu().trace_mode--; }
void cright() { t_cpu_mgr::get_cpu().trace_mode++; }
void chere()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.dbgbreak = 0;
	dbgbreak = 0;
	cpu.dbgchk = 1;

	cpu.dbg_stophere = cpu.trace_curs;
}

void cpgdn()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	unsigned curs = 0;
	for (unsigned i = 0; i < trace_size; i++)
		if (cpu.trace_curs == cpu.trpc[i]) curs = i;
	cpu.trace_top = cpu.trpc[trace_size];
	showtrace();
	cpu.trace_curs = cpu.trpc[curs];
}

void cpgup()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	unsigned curs = 0;
	for (auto i = 0; i < trace_size; i++)
		if (cpu.trace_curs == cpu.trpc[i]) curs = i;
	for (auto i = 0; i < trace_size; i++)
		cpu.trace_top = cpu_up(cpu.trace_top);

	showtrace();
	cpu.trace_curs = cpu.trpc[curs];
}

void pop_pos()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (stack_pos[0] == UINT_MAX)
		return;

	cpu.trace_curs = stack_cur[0];
	cpu.trace_top = stack_pos[0];
	memcpy(&stack_pos[0], &stack_pos[1], sizeof stack_pos - sizeof *stack_pos);
	memcpy(&stack_cur[0], &stack_cur[1], sizeof stack_cur - sizeof *stack_cur);
	stack_pos[(sizeof stack_pos / sizeof *stack_pos) - 1] = -1;
}

void cjump()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	char *ptr = nullptr;

	for (auto p = asmpc; *p; p++)
		if (ishex(p[0]) & ishex(p[1]) & ishex(p[2]) & ishex(p[3])) ptr = p;
	if (!ptr) return;
	push_pos();
	unsigned addr;
	sscanf(ptr, "%04X", &addr);
	cpu.trace_curs = cpu.trace_top = addr;
}

void cdjump()
{
	char *ptr = 0;
	for (auto p = asmpc; *p; p++)
		if (ishex(p[0]) & ishex(p[1]) & ishex(p[2]) & ishex(p[3])) ptr = p;
	if (!ptr) return;
	unsigned addr; sscanf(ptr, "%04X", &addr);
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.mem_curs = addr; activedbg = wndmem; editor = ed_mem;
}

void cfliplabels()
{
	trace_labels = !trace_labels;
    if (trace_labels) mon_labels.import_file(); // explicit load on labels show
    showtrace();
}
void csave(unsigned n)
{
	auto& cpu = t_cpu_mgr::get_cpu();
	save_pos[n] = cpu.trace_top;
	save_cur[n] = cpu.trace_curs;
}
void crest(unsigned n)
{
	auto& cpu = t_cpu_mgr::get_cpu();
	if (save_pos[n] == UINT_MAX)
		return;
	push_pos();
	cpu.trace_top = save_pos[n];
	cpu.trace_curs = save_cur[n];
}
void csave1() { csave(0); }
void csave2() { csave(1); }
void csave3() { csave(2); }
void csave4() { csave(3); }
void csave5() { csave(4); }
void csave6() { csave(5); }
void csave7() { csave(6); }
void csave8() { csave(7); }
void crest1() { crest(0); }
void crest2() { crest(1); }
void crest3() { crest(2); }
void crest4() { crest(3); }
void crest5() { crest(4); }
void crest6() { crest(5); }
void crest7() { crest(6); }
void crest8() { crest(7); }

namespace z80dbg
{
	void __cdecl SetLastT()
	{
		cpu.debug_last_t = comp.t_states + cpu.t;
	}
}

void mon_step()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	auto& prevcpu = t_cpu_mgr::prev_cpu();

	cpu.SetLastT();
	prevcpu = TZ80State(cpu);

	vid.memcyc_lcmd = 0; // new command, start accumulate number of busy memcycles

	cpu.Step();
	cpu.CheckNextFrame();

	// Baseconf NMI trap
	if (conf.mem_model == MM_ATM3 && (comp.pBF & 0x10) && (cpu.pc == comp.pBD))
		nmi_pending = 1;

	// NMI processing
	if (nmi_pending)
	{
		if (conf.mem_model == MM_ATM3)
		{
			nmi_pending = 0;
			cpu.nmi_in_progress = true;
			set_banks();
			m_nmi(RM_NOCHANGE);
		}
		else if (conf.mem_model == MM_PROFSCORP || conf.mem_model == MM_SCORP)
		{
			nmi_pending--;
			if (cpu.pc > 0x4000)
			{
				m_nmi(RM_DOS);
				nmi_pending = 0;
			}
		}
		else
			nmi_pending = 0;
	} // end if (nmi_pending)

	// Baseconf NMI
	if (comp.pBE)
	{
		if (conf.mem_model == MM_ATM3 && comp.pBE == 1)
		{
			cpu.nmi_in_progress = false;
			set_banks();
		}
		comp.pBE--;
	}

	// INT processing
	if (conf.mem_model == MM_TSL)
	{
		const bool vdos = comp.ts.vdos || comp.ts.vdos_m1;

		ts_frame_int(vdos);
		ts_line_int(vdos);
		ts_dma_int(vdos);

		cpu.int_pend = comp.ts.intctrl.pend && !vdos;
	} // Reset INT
	else
	{
		const unsigned int_start = conf.intstart;
		unsigned int_end = conf.intstart + conf.intlen;

		cpu.int_pend = false;
		if ((cpu.t >= int_start) && (cpu.t < int_end))
			cpu.int_pend = true;
		else if (int_end >= conf.frame)
		{
			int_end -= conf.frame;
			if ((cpu.t >= int_start) || (cpu.t < int_end))
				cpu.int_pend = true;
		}
	}

	if (cpu.int_pend && cpu.iff1 && cpu.t != cpu.eipos && cpu.int_gate)
	{
		handle_int(&cpu, cpu.IntVec());
	}
	update_screen(); // update screen, TSU, DMA
    update_raypos();
    flip_from_debug();

	cpu.trace_curs = cpu.pc;
}

void mon_stepover()
{
	Z80 &cpu = t_cpu_mgr::get_cpu();
	u8 trace = 1;

	// call,rst
	if (cpu.pc_trflags & TWF_CALLCMD)
	{
		cpu.dbg_stopsp = cpu.sp & 0xFFFF;
		cpu.dbg_stophere = cpu.nextpc;
		trace = 0;
	}
	else if (cpu.pc_trflags & TWF_BLKCMD) // ldir/lddr|cpir/cpdr|otir/otdr|inir/indr
	{
		trace = 0;
		cpu.dbg_stophere = cpu.nextpc;
	}

	if (trace)
	{
		mon_step();
	}
	else
	{
		cpu.dbgbreak = 0;
		dbgbreak = 0;
		cpu.dbgchk = 1;
	}
}
