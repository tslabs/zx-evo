#include "std.h"
#include "resource.h"
#include "emul.h"
#include "sysdefs.h"
#include "vars.h"
#include "config.h"
#include "draw.h"
#include "dx/dx.h"
#include "emulator/debugger/debug.h"
#include "hard/memory.h"
#include "sound/sound.h"
#include "savesnd.h"
#include "tape.h"
#include "emulator/ui/gui.h"
#include "snapshot.h"
#include "hard/fdd/wd93dat.h"
#include "hard/cpu/z80main.h"
#include "emulkeys.h"
#include "util.h"
#include "visuals.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

void main_pause()
{
	pause = 1;
	flip();
	sound_stop();
	updatebitmap();
	active = 0;
	adjust_mouse_cursor();

	while (!process_msgs())
		Sleep(100);
	//eat();

	active = 1; adjust_mouse_cursor();
	sound_play();
	pause = 0;
}

void main_debug()
{
	Z80& curr_cpu = t_cpu_mgr::get_cpu();

	curr_cpu.dbgchk = 1;
	curr_cpu.dbgbreak = 1;
	dbgbreak = 1;
}

enum { FIX_FRAME = 0, FIX_LINE, FIX_PAPER, FIX_NOPAPER, FIX_HWNC, FIX_LAST };

const char* fix_titles[FIX_LAST] = {
   "%d t-states / int",
   "%d t-states / line",
   "paper starts at %d",
   "border only: %d",
   "hardware mc: %d"
};


u8 whatfix = 0, whatsnd = 0;
u8 fixmode = -1;
int mul0 = 100, mul1 = 1000;

void chfix(int dx)
{
	if (!fixmode) {
		int value;
		switch (whatfix) {
		case FIX_FRAME: value = (conf.frame += dx); break;
		case FIX_LINE: value = (conf.t_line += dx); break;
			//case FIX_PAPER: value = (conf.paper += dx); break;
		case FIX_NOPAPER: value = (conf.nopaper ^= dx ? 1 : 0); break;
		case FIX_HWNC: value = (comp.pEFF7 ^= dx ? EFF7_HWMC : 0) ? 1 : 0; break;
		}
		//      video_timing_tables();
		apply_sound(); // t/frame affects AY engine!
		sprintf(statusline, fix_titles[whatfix], value); statcnt = 50;
		if (dx) conf.ula_preset = -1;
		return;
	}
	if (fixmode != 1) return;

	dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);

	*statusline = 0; statcnt = 50;
	switch (whatsnd) {
	case 0:
		conf.sound.ay_stereo = (conf.sound.ay_stereo + dx + num_aystereo) % num_aystereo;
		sprintf(statusline, "Stereo preset: %s", aystereo[conf.sound.ay_stereo]);
		break;
	case 1:
		if (dx) conf.sound.ay_samples ^= 1;
		sprintf(statusline, "Digital Soundchip: %s", conf.sound.ay_samples ? "yes" : "no");
		break;
	case 2:
		conf.sound.ay_vols = (conf.sound.ay_vols + num_ayvols + dx) % num_ayvols;
		sprintf(statusline, "Chip Table: %s", ayvols[conf.sound.ay_vols]);
		break;
	case 3:
		conf.pal = (conf.pal + dx);
		if (conf.pal == conf.num_pals) conf.pal = 0;
		if (conf.pal == -1) conf.pal = conf.num_pals - 1;
		sprintf(statusline, "Palette: %s", pals[conf.pal].name);
		//video_color_tables();
		return;
	}
	apply_sound();
}

void main_selectfix()
{
	if (!fixmode) whatfix = (whatfix + 1) % FIX_LAST;
	fixmode = 0; mul0 = 1, mul1 = 10;
	if (whatfix == FIX_FRAME) mul0 = 100, mul1 = 1000;
	chfix(0);
}

void main_selectsnd()
{
	if (fixmode == 1) whatsnd = (whatsnd + 1) & 3;
	fixmode = 1;
	chfix(0);
}

void main_incfix() { chfix(mul0); }
void main_decfix() { chfix(-mul0); }
void main_incfix10() { chfix(mul1); }
void main_decfix10() { chfix(-mul1); }

void main_leds()
{
	conf.led.enabled ^= 1;
	sprintf(statusline, "leds %s", conf.led.enabled ? "on" : "off"); statcnt = 50;
}

void main_status()
{
	conf.led.status ^= 1;
	sprintf(statusline, "status line %s", conf.led.status ? "on" : "off"); statcnt = 50;
}

void main_maxspeed()
{
	conf.sound.enabled ^= 1;
	temp.frameskip = conf.sound.enabled ? conf.frameskip : conf.frameskipmax;
	if (conf.sound.enabled) sound_play(); else sound_stop();
	sprintf(statusline, "Max speed: %s", conf.sound.enabled ? "NO" : "YES"); statcnt = 50;
	set_priority();
}

// select filter / driver through gui dialog ----------------------------

INT_PTR CALLBACK filterdlg(HWND dlg, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_INITDIALOG) {
		HWND box = GetDlgItem(dlg, IDC_LISTBOX); int i;

		if (lp) {
			for (i = 0; i < countof(drivers); i++)
				SendMessage(box, LB_ADDSTRING, 0, (LPARAM)drivers[i].name);
			SendMessage(box, LB_SETCURSEL, conf.driver, 0);
			SetWindowText(dlg, "Select driver for rendering");
		}
		else {
			for (i = 0; renders[i].name; i++)
				SendMessage(box, LB_ADDSTRING, 0, (LPARAM)renders[i].name);
			SendMessage(box, LB_SETCURSEL, conf.render, 0);
		}

		RECT rcw, cli; GetWindowRect(box, &rcw); GetClientRect(box, &cli);
		RECT rcd; GetWindowRect(dlg, &rcd);

		int nc_width = (rcw.right - rcw.left) - (cli.right - cli.left);
		int nc_height = (rcw.bottom - rcw.top) - (cli.bottom - cli.top);
		int dlg_w = (rcd.right - rcd.left) - (rcw.right - rcw.left);
		int dlg_h = (rcd.bottom - rcd.top) - (rcw.bottom - rcw.top);
		nc_width += 300;
		nc_height += i * SendMessage(box, LB_GETITEMHEIGHT, 0, 0);
		dlg_w += nc_width; dlg_h += nc_height;
		SetWindowPos(box, 0, 0, 0, nc_width, nc_height, SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

		GetWindowRect(wnd, &rcw);
		SetWindowPos(dlg, 0,
			rcw.left + ((rcw.right - rcw.left) - dlg_w) / 2,
			rcw.top + ((rcw.bottom - rcw.top) - dlg_h) / 2,
			dlg_w, dlg_h, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

		SetFocus(box);
		return 0;
	}

	if ((msg == WM_COMMAND && wp == IDCANCEL) ||
		(msg == WM_SYSCOMMAND && (wp & 0xFFF0) == SC_CLOSE)) EndDialog(dlg, -1);

	if (msg == WM_COMMAND) {
		const int control = LOWORD(wp);
		if (control == IDOK || (control == IDC_LISTBOX && HIWORD(wp) == LBN_DBLCLK))
			EndDialog(dlg, SendDlgItemMessage(dlg, IDC_LISTBOX, LB_GETCURSEL, 0, 0));
	}
	return 0;
}

void main_selectfilter()
{
	sound_stop();
	const int index = DialogBoxParam(hIn, MAKEINTRESOURCE(IDD_FILTER_DIALOG), wnd, filterdlg, 0);
	eat(); sound_play(); if (index < 0) return;

	conf.render = index;
	sprintf(statusline, "Video: %s", renders[index].name); statcnt = 50;
	apply_video(); eat();
}

void main_selectdriver()
{
	if (!(temp.rflags & RF_DRIVER)) {
		strcpy(statusline, "Not available for this filter"); statcnt = 50;
		return;
	}

	sound_stop();
	const int index = DialogBoxParam(hIn, MAKEINTRESOURCE(IDD_FILTER_DIALOG), wnd, filterdlg, 1);
	eat();
	sound_play();

	if (index < 0)
		return;

	conf.driver = index;
	sprintf(statusline, "Render to: %s", drivers[index].name); statcnt = 50;
	apply_video();
	eat();
}

// ----------------------------------------------------------------------

void main_poke()
{
	sound_stop();
	DialogBox(hIn, MAKEINTRESOURCE(IDD_POKE), wnd, pokedlg);
	eat();
	sound_play();
}

void main_starttape()
{
	//if (comp.tape.play_pointer) stop_tape(); else start_tape();
	(comp.tape.play_pointer) ? stop_tape() : start_tape();
}

void main_tapebrowser()
{
#ifdef MOD_SETTINGS
	lastpage = "TAPE", setup_dlg();
#endif
}

#ifndef MOD_SETTINGS
void setup_dlg() {}
#endif

static const char* getrom(const rom_mode page)
{
	switch (page) {
	case rom_mode::s128: return "Basic 128";
	case rom_mode::sys: return "Service ROM";
	case rom_mode::dos: return "TR-DOS";
	case rom_mode::sos: return "Basic 48";
	case rom_mode::cache: return "Cache";
	}
	return "???";
}

void m_reset(rom_mode page)
{
	sprintf(statusline, "Reset to %s", getrom(page)); statcnt = 50;
	nmi_pending = 0;
	cpu.nmi_in_progress = false;
	reset(page);
}
void main_reset128() { m_reset(rom_mode::s128); }
void main_resetsys() { m_reset(rom_mode::sys); }
void main_reset48() { m_reset(rom_mode::sos); comp.p7FFD = 0x30; comp.pEFF7 |= EFF7_LOCKMEM; /*Alone Coder*/ }
void main_resetbas() { m_reset(rom_mode::sos); }
void main_resetdos() { if (conf.trdos_present) m_reset(rom_mode::dos); }
void main_resetcache() { if (conf.cache) m_reset(rom_mode::cache); }
void main_reset() { m_reset((rom_mode)conf.reset_rom); }

void m_nmi(rom_mode page)
{
	set_mode(page);
	sprintf(statusline, "NMI to %s", getrom(page)); statcnt = 50;
	cpu.sp -= 2;
		
	if (*cpu.direct_mem(cpu.pc) == 0x76) // nmi on halt command
		cpu.pc++;

	*cpu.direct_mem(cpu.sp) = cpu.pcl;
	*cpu.direct_mem(cpu.sp + 1) = cpu.pch;
	cpu.pc = 0x66;
	cpu.iff1 = cpu.halted = 0;
}

void main_nmi()
{
	nmi_pending = 1;
	m_nmi(rom_mode::nochange);
}

void main_nmidos()
{
	m_nmi(rom_mode::dos);
}

void main_nmicache() { m_nmi(rom_mode::cache); }

static void qsave(const char* fname) {
	char xx[0x200]; addpath(xx, fname);
	FILE* ff = fopen(xx, "wb");
	if (ff) {
		if (write_sna(ff)) sprintf(statusline, "Quick save to %s", fname), statcnt = 30;
		fclose(ff);
	}
}
void qsave1() { qsave("qsave1.sna"); }
void qsave2() { qsave("qsave2.sna"); }
void qsave3() { qsave("qsave3.sna"); }

static void qload(const char* fname) {
	char xx[0x200]; addpath(xx, fname);
	if (loadsnap(xx)) sprintf(statusline, "Quick load from %s", fname), statcnt = 30;
}
void qload1() { qload("qsave1.sna"); }
void qload2() { qload("qsave2.sna"); }
void qload3() { qload("qsave3.sna"); }

void main_keystick()
{
	input.keymode = (input.keymode == k_input::km_keystick) ? k_input::km_default : k_input::km_keystick;
}

void main_autofire()
{
	conf.input.fire ^= 1;
	input.firedelay = 1;
	sprintf(statusline, "autofire %s", conf.input.fire ? "on" : "off"), statcnt = 30;
}

void main_save_ram()
{
	save_ram();
	sprintf(statusline, "RAM dump saved"), statcnt = 30;
}

void main_save()
{
	sound_stop();
	if (conf.cmos)
		save_nv();
	u8 optype = 0;
	for (int i = 0; i < 4; i++)
	{
		if (!comp.wd.fdd[i].test())
			return;
		optype |= comp.wd.fdd[i].optype;
	}

	if (!optype)
		sprintf(statusline, "all saved"), statcnt = 30;
}

void main_fullscr()
{
	if (!(temp.rflags & (RF_GDI | RF_OVR | RF_CLIP)))
		sprintf(statusline, "only for overlay/gdi modes"), statcnt = 30;
	else
	{
		conf.fullscr ^= 1;
		apply_video();
	}
}

void main_flictoggle()
{
	conf.noflic ^= 1;
	apply_video();

	sprintf(statusline, "noflic %s", conf.noflic ? "on" : "off"); statcnt = 30;
}

void main_mouse()
{
	conf.lockmouse ^= 1;
	adjust_mouse_cursor();
	sprintf(statusline, "mouse %slocked", conf.lockmouse ? nil : "un"), statcnt = 30;
}

void main_help() { showhelp(); }
void mon_help() { showhelp("monitor_keys"); }

void main_pastetext() { input.paste(); }

void wnd_resize(int scale)
{
	if (conf.fullscr)
	{
		sprintf(statusline, "impossible in fullscreen mode");
		statcnt = 50;
		return;
	}

	if (!scale)
	{
		ShowWindow(wnd, SW_MAXIMIZE);
		return;
	}

	ShowWindow(wnd, SW_RESTORE);
	DWORD style = GetWindowLong(wnd, GWL_STYLE);
	RECT rc = { 0, 0, temp.ox * scale, temp.oy * scale };
	AdjustWindowRect(&rc, style, GetMenu(wnd) != 0);
	SetWindowPos(wnd, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	if (temp.rflags & RF_2X)
		scale *= 2;
	else if (temp.rflags & RF_3X)
		scale *= 3;
	else if (temp.rflags & RF_4X)
		scale *= 4;
	sprintf(statusline, "scale: %dx", scale);
	statcnt = 50;
}

void main_size1() { wnd_resize(1); }
void main_size2() { wnd_resize(2); }
void main_sizem() { wnd_resize(0); }

void correct_exit()
{
	sound_stop();
	if (!done_fdd(true))
		return;

	DeleteCriticalSection(&tsu_toggle_cr);

	nowait = 1;
	normal_exit = true;
	Exit = true;
}

void opensnap()
{
	sound_stop();
	opensnap(0);
	eat();
	sound_play();
}

void savesnap()
{
	sound_stop();
	savesnap(-1);
	eat();
	sound_play();
}

void main_visuals()
{
	visuals_on_off();
	SetForegroundWindow(wnd);
}
