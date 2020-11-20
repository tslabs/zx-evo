#include "std.h"
#include "emul.h"
#include "vars.h"
#include "draw.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dbgmem.h"
#include "dbgrwdlg.h"
#include "dbgoth.h"
#include "memory.h"
#include "gui.h"
#include "util.h"

void out(unsigned port, u8 val);

unsigned find1dlg(unsigned start)
{
	static char ftext[12] = "";
	strcpy(str, ftext);
	filledframe(10, 10, 16, 4);
	tprint(10, 10, "  find string   ", frm_header);
	tprint(11, 12, "text:", fframe_inside);
	if (!inputhex(17, 12, 8, false)) return -1;
	strcpy(ftext, str);
	const auto len = strlen(ftext);
	unsigned i; //Alone Coder 0.36.7
	for (auto ptr = memadr(start + 1); ptr != start; ptr = memadr(ptr + 1)) {
		for (/*unsigned*/ i = 0; i < len; i++)
			if (editrm(memadr(ptr + i)) != ftext[i]) break;
		if (i == len) return ptr;
	}
	tprint(11, 12, "  not found   ", fframe_error);
	debugflip();
	while (!process_msgs());
	return -1;
}

unsigned find2dlg(unsigned start)
{
	static unsigned code = 0xF3, mask = 0xFF; char ln[64];
	filledframe(10, 10, 16, 5);
	tprint(10, 10, "   find data    ", frm_header);
	sprintf(ln, "code: %08lX", _byteswap_ulong(code)); tprint(11, 12, ln, fframe_inside);
	sprintf(ln, "mask: %08lX", _byteswap_ulong(mask)); tprint(11, 13, ln, fframe_inside);
	sprintf(str, "%08lX", _byteswap_ulong(code));
	if (!inputhex(17, 12, 8, true)) return -1;
	sscanf(str, "%x", &code); code = _byteswap_ulong(code);
	tprint(17, 12, str, fframe_inside);
	sprintf(str, "%08lX", _byteswap_ulong(mask));
	if (!inputhex(17, 13, 8, true)) return -1;
	sscanf(str, "%x", &mask); mask = _byteswap_ulong(mask);
	unsigned i; //Alone Coder 0.36.7
	for (auto ptr = memadr(start + 1); ptr != start; ptr = memadr(ptr + 1)) {
		const auto cd = reinterpret_cast<u8*>(&code);
		const auto ms = reinterpret_cast<u8*>(&mask);
		for (/*unsigned*/ i = 0; i < 4; i++)
			if ((editrm(memadr(ptr + i)) & ms[i]) != (cd[i] & ms[i])) break;
		if (i == 4) return ptr;
	}
	tprint(11, 12, "  not found   ", fframe_error);
	tprint(11, 13, "              ", fframe_error);
	debugflip();
	while (!process_msgs());
	return -1;
}

void mon_fill()
{
	filledframe(6, 10, 26, 5);
	char ln[64]; sprintf(ln, "start: %04X end: %04X", addr, end);
	tprint(6, 10, "    fill memory block     ", frm_header);
	tprint(7, 12, "pattern (hex):", fframe_inside);
	tprint(7, 13, ln, fframe_inside);

	static char fillpattern[10] = "00";

	u8 pattern[4];
	unsigned fillsize = 0;

	strcpy(str, fillpattern);
	if (!inputhex(22, 12, 8, true)) return;
	strcpy(fillpattern, str);

	if (!fillpattern[0])
		strcpy(fillpattern, "00");

	for (fillsize = 0; fillpattern[2 * fillsize]; fillsize++) {
		if (!fillpattern[2 * fillsize + 1]) fillpattern[2 * fillsize + 1] = '0', fillpattern[2 * fillsize + 2] = 0;
		pattern[fillsize] = hex(fillpattern + 2 * fillsize);
	}
	tprint(22, 12, "        ", fframe_inside);
	tprint(22, 12, fillpattern, fframe_inside);

	auto a1 = input4(14, 13, addr); if (a1 == unsigned(-1)) return;
	addr = a1; tprint(14, 13, str, fframe_inside);
	a1 = input4(24, 13, end); if (a1 == unsigned(-1)) return;
	end = a1;

	unsigned pos = 0;
	for (a1 = addr; a1 <= end; a1++) {
		cpu.DirectWm(a1, pattern[pos]);
		if (++pos == fillsize) pos = 0;
	}
}

// Выход из монитора
void mon_emul()
{
	for (unsigned i = 0; i < t_cpu_mgr::get_count(); i++)
	{
		auto& cpu = t_cpu_mgr::get_cpu(i);
		cpu.dbgchk = isbrk(cpu);
		cpu.dbgbreak = 0;
	}
	dbgbreak = 0;
}

void mon_scr(char alt)
{
	/*temp.scale = temp.mon_scale;
	apply_video();

	memcpy(save_buf, rbuf, rb2_offs);
	paint_scr(alt);
	if (conf.bordersize == 5)
	  show_memcycles();
	flip(); if (conf.noflic) flip();
	memcpy(rbuf, save_buf, rb2_offs);

	while (!process_msgs()) Sleep(20);
	temp.rflags = RF_MONITOR;
	temp.mon_scale = temp.scale;
	temp.scale = 1;
	set_video();*/
}

void mon_scr0() {
    if (show_scrshot != 1) {
        show_scrshot = 1;
        scrshot_page_mask = 0;
    } else
        scrshot_page_mask ^= 0x08;
}

//void mon_scr0() { mon_scr(0); }
void mon_scr1() { mon_scr(1); }
void mon_scray() { mon_scr(2); }

void mon_exitsub()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	cpu.dbgchk = 1;
	cpu.dbgbreak = 0;
	dbgbreak = 0;
	cpu.dbg_stophere = cpu.DirectRm(cpu.sp) + 0x100 * cpu.DirectRm(cpu.sp + 1);
}

void editbank()
{
	const auto x = input2(ports_x + 5, ports_y + 1, comp.p7FFD);
	if (x != UINT_MAX)
	{
		comp.p7FFD = x;
        comp.ts.vpage = comp.ts.vpage_d = (x & 8) ? 7 : 5;
		set_banks();
	}
}

void editextbank()
{
	if (dbg_extport == UINT_MAX)
		return;
	const auto x = input2(ports_x + 5, ports_y + 2, dgb_extval);
	if (x != UINT_MAX)
		out(dbg_extport, u8(x));
}

void mon_aux()
{
	switch (activedbg)
	{
	case wndbanks:
		showbank = true;
		break;

	default:
		showbank = false;
		break;
	}
}

void mon_nxt()
{
	activedbg = (activedbg == wndmem) ? wndbanks : dbgwnd(int(activedbg) - 1);
	mon_aux();
}

void mon_prv()
{
	activedbg = (activedbg == wndbanks) ? wndmem : dbgwnd(int(activedbg) + 1);
	mon_aux();
}

void mon_dump()
{
	mem_dump = (mem_dump + 1) & 1;
	mem_sz = mem_dump ? 32 : 8;
}

void mon_switch_dump()
{
	static const unsigned dump_modes[] = { ed_mem, ed_phys, ed_log, ed_cmos, ed_nvram };
	static unsigned idx = 0;
	++idx;
	idx %= ed_max;
	editor = dump_modes[idx];
}

constexpr auto tool_x = 18;
constexpr auto tool_y = 12;

void mon_tool()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	static u8 unref = 0xCF;
	if (ripper) {
		OPENFILENAME ofn = { 0 };
		char savename[0x200]; *savename = 0;
		ofn.lStructSize = (WinVerMajor < 5) ? OPENFILENAME_SIZE_VERSION_400 : sizeof(OPENFILENAME);
		ofn.lpstrFilter = "Memory dump\0*.bin\0";
		ofn.lpstrFile = savename; ofn.nMaxFile = sizeof savename;
		ofn.lpstrTitle = "Save ripped data";
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
		ofn.hwndOwner = wnd;
		ofn.lpstrDefExt = "bin";
		ofn.nFilterIndex = 1;
		if (GetSaveFileName(&ofn)) {
			for (unsigned i = 0; i < 0x10000; i++)
				snbuf[i] = (cpu.membits[i] & ripper) ? cpu.DirectRm(i) : unref;
			const auto ff = fopen(savename, "wb");
			if (ff) fwrite(snbuf, 1, 0x10000, ff), fclose(ff);
		}
		ripper = 0;
	}
	else {
		filledframe(tool_x, tool_y, 17, 6);
		tprint(tool_x, tool_y, "  ripper's tool  ", frm_header);
		tprint(tool_x + 1, tool_y + 2, "trace reads:", fframe_inside);
		*reinterpret_cast<unsigned*>(str) = 'Y';
		if (!inputhex(tool_x + 15, tool_y + 2, 1, false)) return;
		tprint(tool_x + 15, tool_y + 2, str, fframe_inside);
		if (*str == 'Y' || *str == 'y' || *str == '1') ripper |= MEMBITS_R;
		*reinterpret_cast<unsigned*>(str) = 'N';
		tprint(tool_x + 1, tool_y + 3, "trace writes:", fframe_inside);
		if (!inputhex(tool_x + 15, tool_y + 3, 1, false)) { ripper = 0; return; }
		tprint(tool_x + 15, tool_y + 3, str, fframe_inside);
		if (*str == 'Y' || *str == 'y' || *str == '1') ripper |= MEMBITS_W;
		tprint(tool_x + 1, tool_y + 4, "unref. byte:", fframe_inside);
		unsigned ub;
		if ((ub = input2(tool_x + 14, tool_y + 4, unref)) == UINT_MAX) { ripper = 0; return; }
		unref = u8(ub);
		if (ripper)
			for (unsigned i = 0; i < 0x10000; i++)
				cpu.membits[i] &= ~(MEMBITS_R | MEMBITS_W);
	}
}

void mon_setup_dlg()
{
#ifdef MOD_SETTINGS
	setup_dlg();
	temp.rflags = RF_MONITOR;
	set_video();
#endif
}

void mon_scrshot() { show_scrshot++; if (show_scrshot == 2) show_scrshot = 0; if (show_scrshot == 1) scrshot_page_mask = 0;}
void mon_switchscr() { scrshot_page_mask ^= 0x08; }

void mon_switch_cpu()
{
	//    CpuMgr.CopyToPrev();
	auto& cpu0 = t_cpu_mgr::get_cpu();
	cpu0.dbgbreak = 0;
	t_cpu_mgr::switch_cpu();
	auto& cpu1 = t_cpu_mgr::get_cpu();

	if (cpu1.trace_curs == UINT_MAX)
		cpu1.trace_curs = cpu1.pc;
	if (cpu1.trace_top == UINT_MAX)
		cpu1.trace_top = cpu1.pc;
	if (cpu1.trace_mode == UINT_MAX)
		cpu1.trace_mode = 0;

	debugscr();
	debugflip();
}
