#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "util.h"
#include "hard/memory.h" // for benter()


void show_time()
{
	const Z80 &cpu = t_cpu_mgr::get_cpu();
	tprint(time_x, time_y, "time delta:", w_otheroff);
	char text[32];
	sprintf(text, "%14I64d", cpu.delta());
	tprint(time_x + 11, time_y, text, w_other);
	tprint(time_x + 25, time_y, "t", w_otheroff);
	frame(time_x, time_y, 26, 1, FRAME);
}

static void wtline(const char *name, unsigned ptr, unsigned y)
{
	char line[40];
	if (name)
		sprintf(line, "%3s: ", name);
	else
		sprintf(line, "%04X ", ptr);

	auto& cpu = t_cpu_mgr::get_cpu();
	for (unsigned dx = 0; dx < 8; dx++)
	{
		const auto c = cpu.direct_rm(ptr++);
		sprintf(line + 5 + 3 * dx, "%02X", c);
		line[7 + 3 * dx] = ' ';
		line[29 + dx] = c ? c : '.';
	}

	line[37] = 0;
	tprint(wat_x, wat_y + y, line, w_other);
}

void showwatch()
{
	if (show_scrshot)
	{
		for (unsigned y = 0; y < wat_sz_y; y++)
			for (unsigned x = 0; x < wat_sz_x; x++)
				txtscr[debug_text_width * debug_text_height + (wat_y + y) * debug_text_width + (wat_x + x)] = 0xFF;
	}
	else
	{
		auto& cpu = t_cpu_mgr::get_cpu();
		wtline("PC", cpu.pc, 0);
		wtline("SP", cpu.sp, 1);
		wtline("BC", cpu.bc, 2);
		wtline("DE", cpu.de, 3);
		wtline("HL", cpu.hl, 4);
		wtline("IX", cpu.ix, 5);
		wtline("IY", cpu.iy, 6);
		wtline("BC'", cpu.alt.bc, 7);
		wtline("DE'", cpu.alt.de, 8);
		wtline("HL'", cpu.alt.hl, 9);
		wtline(nullptr, user_watches[0], 10);
		wtline(nullptr, user_watches[1], 11);
		wtline(nullptr, user_watches[2], 12);
	}

	const char *text = "watches";
    if (show_scrshot == 1) {
        text = scrshot_page_mask ? "screen memory (alt)" : "screen memory";
    }

	//if (show_scrshot == 2) text = "ray-painted";  // ray-painted is already available in main emu window
    
	tprint(wat_x, wat_y - 1, text, w_title);

	if (comp.flags & CF_DOSPORTS)
		tprint(wat_x + 34, wat_y - 1, "DOS", w_dos);
	frame(wat_x, wat_y, 37, wat_sz, FRAME);
}

void mon_setwatch()
{
	if (show_scrshot) show_scrshot = 0;
	for (unsigned i = 0; i < 3; i++) {
		debugscr();
		const unsigned addr = input4(wat_x, wat_y + wat_sz - 3 + i, user_watches[i]);
		if (addr == UINT_MAX) return;
		user_watches[i] = addr;
	}
}

void showstack()
{
	Z80 &cpu = t_cpu_mgr::get_cpu();
	for (unsigned i = 0; i < stack_size; i++)
	{
		char xx[10]; //-2:1234
					 //SP:1234
					 //+2:
		if (!i) *reinterpret_cast<unsigned*>(xx) = WORD2('-', '2');
		else if (i == 1) *reinterpret_cast<unsigned*>(xx) = WORD2('S', 'P');
		else sprintf(xx, (i > 8) ? "%X" : "+%X", (i - 1) * 2);
		sprintf(xx + 2, ":%02X%02X", cpu.direct_rm(cpu.sp + (i - 1) * 2 + 1), cpu.direct_rm(cpu.sp + (i - 1) * 2));
		tprint(stack_x, stack_y + i, xx, w_other);
	}
	tprint(stack_x, stack_y - 1, "stack", w_title);
	frame(stack_x, stack_y, 7, stack_size, FRAME);
}

void show_ay()
{
	if (conf.sound.ay_scheme == ay_scheme::none) 
		return;

	const char *ayn = comp.active_ay ? "AY1" : "AY0";
	if (conf.sound.ay_scheme < ay_scheme::quadro) ayn = "AY:", comp.active_ay = 0;
	tprint(ay_x - 3, ay_y, ayn, w_title);
	const auto chip = &ay[comp.active_ay];
	char line[32];
	for (auto i = 0; i < 16; i++) {
		line[0] = "0123456789ABCDEF"[i]; line[1] = 0;
		tprint(ay_x + i * 3, ay_y, line, w_aynum);
		sprintf(line, "%02X", chip->get_reg(i));
		tprint(ay_x + i * 3 + 1, ay_y, line, i == (chip->get_activereg()) ? w_ayon : w_ayoff);
	}
	frame(ay_x, ay_y, 48, 1, FRAME);
}

void mon_switchay()
{
	comp.active_ay ^= 1;
}

void __cdecl BankNames(int i, char *name)
{
	unsigned rom_bank = 0;
	unsigned ram_bank = 0;

	const bool is_ram = (RAM_BASE_M <= bankr[i]) && (bankr[i] < page_ram(MAX_RAM_PAGES));
	const bool is_rom = (ROM_BASE_M <= bankr[i]) && (bankr[i] < page_rom(MAX_ROM_PAGES));

	if (is_ram)
		ram_bank = ULONG((bankr[i] - RAM_BASE_M) / PAGE);

	if (is_rom)
		rom_bank = ULONG((bankr[i] - ROM_BASE_M) / PAGE);

	if (is_ram)
		sprintf(name, "RAM%2X", ram_bank);

	if (is_rom)
		sprintf(name, "ROM%2X", rom_bank);

	if (bankr[i] == base_sos_rom)
		strcpy(name, "BASIC");
	if (bankr[i] == base_dos_rom)
		strcpy(name, "TRDOS");
	if (bankr[i] == base_128_rom)
		strcpy(name, "B128K");

	if (bankr[i] == CACHE_M)
		strcpy(name, (conf.cache != 32) ? "CACHE" : "CACH0");
	if (bankr[i] == CACHE_M + PAGE)
		strcpy(name, "CACH1");
}

unsigned int selbank = 0, showbank = false;
void showbanks()
{
	auto& cpu = t_cpu_mgr::get_cpu();
	for (unsigned i = 0; i < 4; i++)
	{
		char ln[64]; sprintf(ln, "%d:", i);
		char attr = ((selbank == i) && (showbank) ? (w_otheroff & 0xF) | w_curs : w_otheroff | (activedbg == wndbanks ? 0x10 : 0));
		tprint(banks_x, banks_y + i + 1, ln, attr);
		strcpy(ln, "?????");
		cpu.bank_names(i, ln);
		attr = ((selbank == i) && (showbank) ? w_curs : ((bankr[i] != bankw[i] ? w_bankro : w_bank) | (activedbg == wndbanks ? 0x10 : 0)));
		tprint(banks_x + 2, banks_y + i + 1, ln, attr);
	}
	frame(banks_x, banks_y + 1, 7, 4, FRAME);
	tprint(banks_x, banks_y, "pages", w_title);
}

void bdown() {
	selbank++; selbank &= 3;
}
void bup() {
	selbank--; selbank &= 3;
}
// ������ ��������
void benter() {
	auto& cpu = t_cpu_mgr::get_cpu();
	debugscr();
	debugflip();

	char bankstr[64] = { 0 }; cpu.bank_names(selbank, bankstr);
	unsigned val;
	sscanf(&bankstr[3], "%x", &val);

	if ((input.lastkey >= '0' && input.lastkey <= '9') || (input.lastkey >= 'A' && input.lastkey <= 'F'))
		PostThreadMessage(GetCurrentThreadId(), WM_KEYDOWN, input.lastkey, 1);

	val = input2(banks_x + 5, banks_y + selbank + 1, val);
	if (val != UINT_MAX) {
		// set new bank
		comp.ts.page[selbank] = val;
		set_banks();
	}
}

int dispatch_banks() {
	if ((conf.mem_model == MM_TSL) && (selbank != UINT_MAX) &&
		((input.lastkey >= '0' && input.lastkey <= '9') || (input.lastkey >= 'A' && input.lastkey <= 'F')))
	{
		benter();
		return 1;
	}
	return 0;
}

void show_pc_history()
{
	Z80 &cpu = t_cpu_mgr::get_cpu();
  
  char xx[pc_history_wx + 1];
  for (u32 i = 0; i < pc_history_wy; i++)
	{
    int p = main_z80_dbg_cntx.pc_hist_ptr - i;
    if (p < 0) p += z80_pc_history_size;
    
    sprintf(xx, "%02X:%04X", main_z80_dbg_cntx.pc_hist[p].page, main_z80_dbg_cntx.pc_hist[p].addr);
    tprint(pc_history_x, pc_history_y + i + 1, xx, w_other);
  }
  
	tprint(pc_history_x, pc_history_y, "PC hist", w_title);
 	frame(pc_history_x, pc_history_y + 1, pc_history_wx, pc_history_wy, FRAME);
}

void showports()
{
	char ln[64];
	sprintf(ln, "  FE:%02X", comp.pFE);
	tprint(ports_x, ports_y, ln, w_other);
	sprintf(ln, "7FFD:%02X", comp.p7FFD);
	tprint(ports_x, ports_y + 1, ln, (comp.p7FFD & 0x20) && !((conf.mem_model == MM_PENTAGON && conf.ramsize == 1024)) ? w_48_k : w_other);

	dbg_extport = -1;

	if (dbg_extport != UINT_MAX)
		sprintf(ln, "%04X:%02X", dbg_extport, dgb_extval);
	else
		sprintf(ln, "cmos:%02X", comp.cmos_addr);
	tprint(ports_x, ports_y + 2, ln, w_other);

	sprintf(ln, "EFF7:%02X", comp.pEFF7);
	tprint(ports_x, ports_y + 3, ln, w_other);
	frame(ports_x, ports_y, 7, 4, FRAME);
	tprint(ports_x, ports_y - 1, "ports", w_title);
}

void showdos()
{
	//    CD:802E
	//    STAT:24
	//    SECT:00
	//    T:00/01
	//    S:00/00
	//[vv]   if (conf.trdos_present) comp.wd.process();
	char ln[64];
	const u8 atr = conf.trdos_present ? w_other : w_otheroff;
	sprintf(ln, "CD:%02X%02X", comp.wd.cmd, comp.wd.data);
	tprint(dos_x, dos_y, ln, atr);
	sprintf(ln, "STAT:%02X", comp.wd.status);
	tprint(dos_x, dos_y + 1, ln, atr);
	sprintf(ln, "SECT:%02X", comp.wd.sector);
	tprint(dos_x, dos_y + 2, ln, atr);
	sprintf(ln, "T:%02X/%02X", comp.wd.seldrive->track, comp.wd.track);
	tprint(dos_x, dos_y + 3, ln, atr);
	sprintf(ln, "S:%02X/%02X", comp.wd.system, comp.wd.rqs);
	tprint(dos_x, dos_y + 4, ln, atr);
	frame(dos_x, dos_y, 7, 5, FRAME);
#if 1
	tprint(dos_x, dos_y - 1, "beta128", w_title);
#else
	sprintf(ln, "%X-%X %d", comp.wd.state, comp.wd.state2, comp.wd.seldrive->track);
	tprint(dos_x, dos_y - 1, ln, atr);
#endif
}

#ifdef MOD_GSBASS
INT_PTR CALLBACK gsdlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	char tmp[0x200];
	if (msg == WM_INITDIALOG) {
	repaint:
		while (SendDlgItemMessage(dlg, IDC_GSLIST, LB_GETCOUNT, 0, 0))
			SendDlgItemMessage(dlg, IDC_GSLIST, LB_DELETESTRING, 0, 0);
		if (gs.modsize) {
			sprintf(tmp, "%-.20s (%s)", gs.mod, gs.mod_playing ? "playing" : "stopped");
			SendDlgItemMessage(dlg, IDC_GSLIST, LB_ADDSTRING, 0, LPARAM(tmp));
		}
		for (unsigned i = 1; i < gs.total_fx; i++) {
			sprintf(tmp, "%csmp %d: v=%d, n=%d, %d%s",
				gs.cur_fx == i ? '*' : ' ', i,
				gs.sample[i].volume, gs.sample[i].note, gs.sample[i].end,
				gs.sample[i].loop < gs.sample[i].end ? " (L)" : nil);
			SendDlgItemMessage(dlg, IDC_GSLIST, LB_ADDSTRING, 0, LPARAM(tmp));
		}
		*tmp = 0;
		for (auto i = 0; i < 0x100; i++) {
			if (gs.badgs[i]) sprintf(tmp + strlen(tmp), "%02X ", i);
		}
		SendDlgItemMessage(dlg, IDE_GS, WM_SETTEXT, 0, LPARAM(tmp));
		return 1;
	}
	if (msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE) EndDialog(dlg, 0);
	if (msg != WM_COMMAND) return 0;
	const unsigned id = LOWORD(wp);
	if (id == IDCANCEL || id == IDOK) EndDialog(dlg, 0);
	if (id == IDB_GS_CLEAR) { memset(gs.badgs, 0, sizeof gs.badgs); SendDlgItemMessage(dlg, IDE_GS, WM_SETTEXT, 0, 0); }
	if (id == IDB_GS_RESET) { gs.reset(); goto repaint; }
	if (id == IDB_GS_PLAY || (id == IDC_GSLIST && HIWORD(wp) == LBN_DBLCLK)) {
		unsigned i = SendDlgItemMessage(dlg, IDC_GSLIST, LB_GETCURSEL, 0, 0);
		if (i > 0x100) return 1;
		if (!i && gs.modsize) {
			gs.mod_playing ^= 1;
			if (gs.mod_playing) gs.restart_mod(0, 0); else gs.stop_mod();
			goto repaint;
		}
		if (!gs.modsize) i++;
		gs.debug_note(i);
	}
	return 0;
}

void mon_gsdialog()
{
	if (conf.gs_type == 2)
		DialogBox(hIn, MAKEINTRESOURCE(IDD_GS), wnd, gsdlg);
	else MessageBox(wnd, "high-level GS emulation\nis not initialized", nullptr, MB_OK | MB_ICONERROR);
}
#else
void mon_gsdialog() {}
#endif
