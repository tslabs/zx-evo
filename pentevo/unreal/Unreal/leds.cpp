#include "std.h"
#include "emul.h"
#include "vars.h"
#include "font.h"
#include "hard/gs/gs.h"
#include "tape.h"
#include "draw.h"
#include "emulator/debugger/debug.h"
#include "emulator/debugger/dbgbpx.h"
#include "hard/memory.h"
#include "util.h"

extern VCTR vid;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

extern u8 pause;

static unsigned pitch;

static const u32 out_pal[16] =
{
	0x000000, 0x0000C0, 0xC00000, 0xC000C0, 0x00C000, 0x00C0C0, 0xC0C000, 0xC0C0C0,
	0x000000, 0x0000FF, 0xFF0000, 0xFF00FF, 0x00FF00, 0x00FFFF, 0xFFFF00, 0xFFFFFF
};

#define DRAW_PIX(p,c) *(u32*)(p) = out_pal[(c)]

static void draw_line(u32* ptr, u8 ink, int len, int x = 0, int y = 0)
{
	ptr += x + y * pitch;
	ink &= 0xF;
	while (len--)
		*(ptr++) = out_pal[ink];
}

static void draw_bitmap(u32* ptr, u8 ink, u8* bmp, int width = 8, int height = 1, int x = 0, int y = 0)
{
	ptr += x + y * pitch;

	for (int y = 0; y < height; y++)
	{
		u32* line = ptr;
		for (int x = 0; x < width / 8; x++)
		{
			u8 byte = *(bmp++);

			for (int b = 0; (b < 8) && ((x * 8 + b) < width); b++)
			{
				*(line++) = (byte & (0x80 >> b)) ? out_pal[ink & 0xF] : out_pal[(ink >> 4) & 0xF];
			}
		}
		ptr += pitch;
	}
}

static void draw_bitmap_tr(u32* ptr, u8 ink, u8* bmp, int width = 8, int height = 1, int x = 0, int y = 0)
{
	ptr += x * 2 + y * pitch;
	ink &= 0xF;

	for (int y = 0; y < height; y++)
	{
		u32* line = ptr;
		for (int x = 0; x < width / 8; x++)
		{
			u8 byte = *(bmp++);

			for (int b = 0; (b < 8) && ((x * 8 + b) < width); b++)
			{
				if (byte & (0x80 >> b))
					*line = out_pal[ink];
				line++;
			}
		}
		ptr += pitch;
	}
}

void text_i(u32* dst, const char* text, u8 ink, unsigned off = 0)
{
	u8 mask = 0xF0; ink &= 0x0F;
	for (u8* x = (u8*)text; *x; x++) {
		u32* d0 = dst + off;
		for (unsigned y = 0; y < 8; y++) {
			u8 byte = font[(*x) * 8 + y];

			for (int b = 0; b < 6; b++)
			{
				if (byte & (0x80 >> b))
					d0[b] = out_pal[ink];
			}

			d0 += pitch;
		}
		dst += 6;
	}
}

u32* aypos;
void paint_led(unsigned level, u8 at)
{
	u32* ptr = aypos;
	if (level > 15) level = 15, at = 0x0E;
	draw_line(aypos, at, level);
	aypos += pitch;
}

void ay_led()
{
	aypos = temp.led.ay;
	u8 sum = 0;

	const int max_ay = (conf.sound.ay_scheme > ay_scheme::pseudo) ? 2 : 1;
	for (int n_ay = 0; n_ay < max_ay; n_ay++) {
		for (int i = 0; i < 3; i++) {
			const u8 r_mix = ay[n_ay].get_reg(7);
			const u8 tone = (r_mix >> i) & 1;
			const u8 noise = (r_mix >> (i + 3)) & 1;
			u8 c1 = 0, c2 = 0;
			u8 v = ay[n_ay].get_reg(i + 8);
			if (!tone) c1 = c2 = 0x0F;
			if (!noise) c2 = 0x0E;
			if (v & 0x10) {
				const unsigned r_envT = ay[n_ay].get_reg(11) + 0x100 * ay[n_ay].get_reg(12);
				if (r_envT < 0x400) {
					v = (3 - (r_envT >> 3)) & 0x0F;
					if (!v) v = 6;
				}
				else v = ay[n_ay].get_env() / 2;
				c1 = 0x0C;
			}
			else v &= 0x0F;
			if (!c1) c1 = c2;
			if (!c2) c2 = c1;
			if (!c1) v = 0;
			sum |= v;
			paint_led(v, c1);
			paint_led(v, c2);
			paint_led(0, 0);
		}
	}

	const unsigned FMvols[] = { 4,9,15,23,35,48,70,105,140,195,243,335,452,608,761,1023 };
#define _cBlue 0x09
#define _cRed 0x0a
#define _cPurp 0x0b
#define _cGreen 0x0c
#define _cCyan 0x0d
#define _cYell 0x0e
#define _cWhite 0x0f

	const int FMalg1[] = {
	_cBlue , _cPurp , _cGreen, _cWhite,
	_cPurp , _cPurp , _cGreen, _cWhite,
	_cGreen, _cPurp , _cGreen, _cWhite,
	_cPurp , _cGreen, _cGreen, _cWhite,

	_cGreen, _cWhite, _cGreen, _cWhite,
	_cPurp , _cWhite, _cWhite, _cWhite,
	_cGreen, _cWhite, _cWhite, _cWhite,
	_cWhite, _cWhite, _cWhite, _cWhite
	};

	const int FMalg2[] = {
	_cBlue , _cPurp , _cGreen, _cYell ,
	_cPurp , _cPurp , _cGreen, _cYell ,
	_cGreen, _cPurp , _cGreen, _cYell ,
	_cPurp , _cGreen, _cGreen, _cYell ,

	_cGreen, _cYell , _cGreen, _cYell ,
	_cPurp , _cYell , _cYell , _cYell ,
	_cGreen, _cYell , _cYell , _cYell ,
	_cYell , _cYell , _cYell , _cYell
	};

	constexpr int FMslots[] = { 0,2,1,3 };
	
	if (conf.sound.ay_chip == SNDCHIP::CHIP_YM2203) {
		for (int ayN = 0; ayN < max_ay; ayN++) {
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 4; j++) {
					unsigned v = ay[ayN].Chip2203->CH[i].SLOT[j].vol_out;
					if (v > 1023) v = 1023;
					int c; //Alone Coder 0.36.7
					for (/*int*/ c = 0; c < 16; c++)
						if (FMvols[c] >= v) break;
					if ((i == 2) && (((ay[ayN].Chip2203->OPN.ST.mode) & 0xc0) == 0x40))
						paint_led(15 - c, FMalg2[ay[ayN].Chip2203->CH[i].ALGO * 4 + FMslots[j]]);
					else
						paint_led(15 - c, FMalg1[ay[ayN].Chip2203->CH[i].ALGO * 4 + FMslots[j]]);
				}
				paint_led(0, 0);
			}
		}
	} //Dexus

#ifdef MOD_GS
	if (sum || !conf.gs_type) return; // else show GS indicators
	aypos = temp.led.ay; // reset y-pos, if nothing above
	for (unsigned ch = 0; ch < 8; ch++) {
		unsigned v = gsleds[ch].level, a = gsleds[ch].attrib;
		paint_led(v, a);
		paint_led(v, a);
		paint_led(0, 0);
	}
#endif
}

void load_led()
{
	char ln[20]; u8 diskcolor = 0;

#ifdef GS_BASS
	if (gs.loadmod) {
		text_i(temp.led.load, "", 0x0D);
		gs.loadmod = 0;
	}
	else if (gs.loadfx) {
		sprintf(ln, "\x0D%d", gs.loadfx);
		text_i(temp.led.load, ln, 0x0D);
		gs.loadfx = 0;
	}
	else
#endif
		if (trdos_format) {
			diskcolor = (trdos_format < romled_time * 3 / 4) ? 0x06 : 0x0E;
			trdos_format--;
		}
		else if (trdos_save) {
			diskcolor = (trdos_save < romled_time * 3 / 4) ? 0x02 : 0x0A;
			trdos_save--;
		}
		else if (trdos_load) {
			diskcolor = (trdos_load < romled_time * 3 / 4) ? 0x01 : 0x09;
			trdos_load--;
		}
		else if (trdos_seek) {
			trdos_seek--;
		}
		else if (comp.tape.play_pointer) {
			static u8 tapeled[11 * 2] = {
			   0x7F, 0xFE, 0x80, 0x01, 0x80, 0x01, 0x93, 0xC9, 0xAA, 0x55, 0x93, 0xC9,
			   0x80, 0x01, 0x8F, 0xF1, 0x80, 0x01, 0xB5, 0xA9, 0xFF, 0xFF };
			const int tapecolor = 0x51;
			draw_bitmap(temp.led.load, tapecolor, tapeled, 16, 11);
			int time = (int)(temp.led.tape_started + tapeinfo[comp.tape.index].t_size - comp.t_states);
			if (time < 0) {
				find_tape_index(); time = 0;
				temp.led.tape_started = comp.t_states;
				u8* ptr = tape_image + tapeinfo[comp.tape.index].pos;
				if (ptr == comp.tape.play_pointer && comp.tape.index)
					comp.tape.index--, ptr = tape_image + tapeinfo[comp.tape.index].pos;
				for (; ptr < comp.tape.play_pointer; ptr++)
					temp.led.tape_started -= tape_pulse[*ptr];
			}
			time /= (conf.frame * conf.intfq);
			sprintf(ln, "%X:%02d", time / 60, time % 60);
			text_i(temp.led.load + pitch * 12 - 8, ln, 0x0D);
		}
	if (diskcolor | trdos_seek) {
		if (diskcolor) {
			for (int i = 0; i < 7; i++)
				draw_line(temp.led.load, diskcolor, 12, 2, i);
			static u8 disk[] = { 0x38, 0x1C, 0x3B, 0x9C, 0x3B, 0x9C, 0x3B, 0x9C, 0x38,0x1C };
			draw_bitmap_tr(temp.led.load, diskcolor, disk, 16, 5, 0, 7);
		}
		if (comp.wd.seldrive->track != 0xFF) {
			sprintf(ln, "%02X", comp.wd.seldrive->track * 2 + comp.wd.side);
			text_i(temp.led.load + pitch - 4 * 4, ln, 0x05 + (diskcolor & 8));
		}
	}
}

static unsigned p_frames = 1;
static u64 led_updtime, p_time;
double p_fps;
__inline void update_perf_led()
{
	u64 now = led_updtime - p_time;
	if (now >= temp.cpufq) // усреднение за секунду
	{
		p_fps = (p_frames * temp.cpufq) / double(now) + 0.005;
		p_frames = 0;
		p_time = led_updtime;
	}
	p_frames++;
}

void perf_led()
{
	char bf[0x20]; unsigned PSZ;
	if (conf.led.perf_t)
		sprintf(bf, "%6d*%2.2f", cpu.haltpos ? cpu.haltpos : cpu.t, p_fps), PSZ = 7;
	else
		sprintf(bf, "%2.2f fps", p_fps), PSZ = 5;
	text_i(temp.led.perf, bf, 0x0E);
	if (cpu.haltpos) {
		draw_line(temp.led.perf, 0x9, PSZ * 8, 0, 8);
		draw_line(temp.led.perf, 0xA, cpu.haltpos * PSZ * 8 / conf.frame, 1, 8);
	}
}

void input_led()
{
	if (input.kbdled != 0xFF) {
		u8 k0 = 0x9, k1 = 0xF, k2 = 0x0;
		if (input.keymode == k_input::km_paste_hold) k0 = 0xA;
		if (input.keymode == k_input::km_paste_release) k0 = 0x2;

		for (int i = 0; i < 4 * 2 + 1; i++)
		{
			draw_line(temp.led.input, k0, 16, 0, i);
		}

		for (int i = 0; i < 4; i++)
		{
			draw_line(temp.led.input, (input.kbdled & (0x08 >> i)) ? k2 : k1, 7, 1, i * 2 + 1);
			draw_line(temp.led.input, (input.kbdled & (0x10 << i)) ? k2 : k1, 7, 8, i * 2 + 1);
		}
	}
	static u8 joy[] = { 0x10, 0x38, 0x1C, 0x1C, 0x1C, 0x1C, 0x08, 0x00, 0x7E, 0xFF, 0x00, 0xE7 };
	static u8 mouse[] = { 0x0C, 0x12, 0x01, 0x79, 0xB5, 0xB5, 0xB5, 0xFC, 0xFC, 0xFC, 0xFC, 0x78 };
	if (input.mouse_joy_led & 2)
		draw_bitmap_tr(temp.led.input, 0xF, joy, 8, sizeof(joy), 4 * 2);
	if (input.mouse_joy_led & 1)
		draw_bitmap_tr(temp.led.input, 0xF, mouse, 8, sizeof(mouse), 6 * 2);
	input.mouse_joy_led = 0; input.kbdled = 0xFF;
}

#ifdef MOD_MONITOR
void debug_led()
{
	u32* ptr = temp.led.osw;
	if (trace_rom | trace_ram) {
		set_banks();
		if (trace_rom) {
			const u8 off = 0x01, on = 0x0C;
			text_i(ptr + 2 * 4, "B48", used_banks[(base_sos_rom - memory) / PAGE] ? on : off);
			text_i(ptr + 8 * 4, "DOS", used_banks[(base_dos_rom - memory) / PAGE] ? on : off);
			text_i(ptr + pitch * 8 + 2 * 4, "128", used_banks[(base_128_rom - memory) / PAGE] ? on : off);
			text_i(ptr + pitch * 8 + 8 * 4, "SYS", used_banks[(base_sys_rom - memory) / PAGE] ? on : off);
			ptr += pitch * 16;
		}
		if (trace_ram) {
			unsigned num_rows = conf.ramsize / 128;
			unsigned j; //Alone Coder 0.36.7
			for (unsigned i = 0; i < num_rows; i++) {
				char ln[9];
				for (/*unsigned*/ j = 0; j < 8; j++)
					ln[j] = used_banks[i * 8 + j] ? '*' : '-';
				ln[j] = 0;
				text_i(ptr, ln, 0x0D);
				ptr += pitch * 8;
			}
		}
		for (unsigned j = 0; j < MAX_PAGES; j++) used_banks[j] = 0;
	}
	for (unsigned w = 0; w < 4; w++) if (watch_enabled[w])
	{
		char bf[12]; sprintf(bf, "%8X", calc(cpu, watch_script[w]));
		text_i(ptr, bf, 0x0F); ptr += pitch * 8;
	}
}
#endif

#ifdef MOD_MEMBAND_LED
void show_mband(u32* dst, unsigned start)
{
	unsigned i;
	char xx[8]; sprintf(xx, "%02X", start >> 8);
	text_i(dst, xx, 0x0B); dst += 4 * 4;

	Z80& cpu = t_cpu_mgr::get_cpu();
	u8 band[128];
	for (i = 0; i < 128; i++) {
		u8 res = 0;
		for (unsigned q = 0; q < conf.led.bandBpp; q++)
			res |= cpu.membits[start++];
		band[i] = res;
	}

	for (i = 0; i < 8; i++)
		DRAW_PIX(dst + pitch * i, 0xB);
	dst++;

	for (unsigned p = 0; p < 16 * 8; p++, dst++) {
		u8 r = 0x1, w = 0x1, x = 0x1;
		u8 t = 0xB;

		if (band[p] & MEMBITS_R) r = 0xC;
		if (band[p] & MEMBITS_W) w = 0xA;
		if (band[p] & MEMBITS_X) x = 0xF;

		if (p && !(p & 0x1F)) t = 0x9;

		DRAW_PIX(dst + pitch * 0, t);
		DRAW_PIX(dst + pitch * 1, r);
		DRAW_PIX(dst + pitch * 2, r);
		DRAW_PIX(dst + pitch * 3, w);
		DRAW_PIX(dst + pitch * 4, w);
		DRAW_PIX(dst + pitch * 5, x);
		DRAW_PIX(dst + pitch * 6, x);
		DRAW_PIX(dst + pitch * 7, t);
	}

	for (i = 0; i < 8; i++)
		DRAW_PIX(dst + pitch * i, 0xB);
	dst++;

	sprintf(xx, "%02X", (start - 1) >> 8);
	text_i(dst, xx, 0x0B, 2);
}

void memband_led()
{
	u32* dst = temp.led.memband;
	for (unsigned start = 0x0000; start < 0x10000;) {
		show_mband(dst, start);
		start += conf.led.bandBpp * 128;
		dst += 10 * pitch;
	}

	Z80& cpu = t_cpu_mgr::get_cpu();
	for (unsigned i = 0; i < 0x10000; i++)
		cpu.membits[i] &= ripper | ~(MEMBITS_R | MEMBITS_W | MEMBITS_X);
}
#endif


HANDLE hndKbdDev;

void init_leds()
{
	DefineDosDevice(DDD_RAW_TARGET_PATH, "Kbd_unreal_spec", "\\Device\\KeyboardClass0");
	hndKbdDev = CreateFile("\\\\.\\Kbd_unreal_spec", GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
	if (hndKbdDev == INVALID_HANDLE_VALUE) hndKbdDev = 0, conf.led.flash_ay_kbd = 0;
}

void done_leds()
{
	if (hndKbdDev) {
		DefineDosDevice(DDD_REMOVE_DEFINITION, "Kbd_unreal_spec", 0);
		CloseHandle(hndKbdDev); hndKbdDev = 0;
	}
}

void ay_kbd()
{
	static u8 pA, pB, pC;
	static unsigned prev_keyled = -1;

	KEYBOARD_INDICATOR_PARAMETERS InputBuffer;
	InputBuffer.LedFlags = InputBuffer.UnitId = 0;

	if (ay->get_reg(8) > pA) InputBuffer.LedFlags |= KEYBOARD_NUM_LOCK_ON;
	if (ay->get_reg(9) > pB) InputBuffer.LedFlags |= KEYBOARD_CAPS_LOCK_ON;
	if (ay->get_reg(10) > pC) InputBuffer.LedFlags |= KEYBOARD_SCROLL_LOCK_ON;

	pA = ay->get_reg(8), pB = ay->get_reg(9), pC = ay->get_reg(10);

	DWORD xx;
	if (prev_keyled != InputBuffer.LedFlags)
		prev_keyled = InputBuffer.LedFlags,
		DeviceIoControl(hndKbdDev, IOCTL_KEYBOARD_SET_INDICATORS,
			&InputBuffer, sizeof(KEYBOARD_INDICATOR_PARAMETERS), 0, 0, &xx, 0);
}

void key_led()
{
#if 0
#define key_x 1
#define key_y 1
	int i; //Alone Coder 0.36.7
	for (/*int*/ i = 0; i < 9; i++) text_16(rbuf + (key_y + i) * pitch * 16 + key_x * 2, "                                 ", 0x40);
	static char ks[] = "cZXCVASDFGQWERT1234509876POIUYeLKJHssMNB";
	for (i = 0; i < 8; i++) {
		for (int j = 0; j < 5; j++) {
			unsigned x, y, at;
			if (i < 4) y = 7 - 2 * i + key_y, x = 3 * j + 2 + key_x;
			else y = 2 * (i - 4) + 1 + key_y, x = 29 - 3 * j + key_x;
			unsigned a = ks[i * 5 + j] * 0x100 + ' ';
			at = (input.kbd[i] & (1 << j)) ? 0x07 : ((input.rkbd[i] & (1 << j)) ? 0xA0 : 0xD0);
			text_16(rbuf + 2 * x + y * pitch * 16, (char*)&a, at);
		}
	}
#endif
}

void time_led()
{
	static u64 prev_time;
	static char bf[8];
	if (led_updtime - prev_time > 5000) {
		prev_time = led_updtime;
		SYSTEMTIME st; GetLocalTime(&st);
		sprintf(bf, "%2d:%02d", st.wHour, st.wMinute);
	}
	text_i(temp.led.time, bf, 0x0D);
}

void init_memcycles(void)
{
	memset(vid.memcpucyc, 0, 320 * sizeof(vid.memcpucyc[0]));
	memset(vid.memvidcyc, 0, 320 * sizeof(vid.memvidcyc[0]));
	memset(vid.memtsscyc, 0, 320 * sizeof(vid.memtsscyc[0]));
	memset(vid.memtstcyc, 0, 320 * sizeof(vid.memtstcyc[0]));
	memset(vid.memdmacyc, 0, 320 * sizeof(vid.memdmacyc[0]));
}

void show_memcycle(u16 num, u32 col, u8 init, u32 vbpi)
{
	static int cnt;
	static u32* vidbuf;

	if (init)
	{
		cnt = 0;
		vidbuf = &vbuf[vid.buf][vbpi];
	}

	if (cnt > 112)
		return;

	num /= 4;
	u16 num1, num2;
	if ((cnt + num) < 112)
	{
		num1 = num;
		num2 = 0;
	}
	else
	{
		num1 = 112 - cnt;
		num2 = min(num - num1, 16);
	}

	while (num1--)
		*vidbuf++ = col;
	if (num2)
		vidbuf++;
	while (num2--)
		*vidbuf++ = 0xFF0000;
	cnt += num;
}

void show_memcycles(int startLine, int endLine)
{
	u32 vbp;
	u32 bar = 0x555555;

	for (int i = startLine; i < endLine; i++)
	{
		vbp = i * 896;
		vbuf[vid.buf][vbp + 15] = bar;
		vbuf[vid.buf][vbp + 128] = bar;
		show_memcycle(vid.memcpucyc[i], 0x40FF90, 1, vbp + 16);		// CPU
		show_memcycle(vid.memvidcyc[i], 0xFFFF90, 0, 0);			// Video
		show_memcycle(vid.memtstcyc[i], 0xFF9090, 0, 0);			// TSU Tiles
		show_memcycle(vid.memtsscyc[i], 0xC040FF, 0, 0);			// TSU Sprites
		show_memcycle(vid.memdmacyc[i], 0xFFFFFF, 0, 0);
	}

}

static void update_led_ptrs(u32* dst)
{
	for (int i = 0; i < NUM_LEDS; i++) {
		unsigned z = *(&conf.led.ay + i);
		int x = (signed short)(z & 0xFFFF);
		int y = (signed short)(((z >> 16) & 0x7FFF) + ((z >> 15) & 0x8000));
		if (x < 0) x += temp.ox;
		if (y < 0) y += temp.oy;
		*(&temp.led.ay + i) = (z & 0x80000000) ? dst + x + y * pitch : 0;
	}
}

// Вызывается раз в кадр
void showleds(u32* dst, unsigned bpitch)
{
	led_updtime = rdtsc();
	update_perf_led();

	if (temp.vidblock) return;

	pitch = bpitch / 4;

	if ((statcnt) && (conf.led.status))
	{
		int x, y;
		statcnt--;
		x = (temp.ox - strlen(statusline) * 6) / 2;
		y = temp.oy - 10; vbuf[vid.buf];
		text_i(dst + x + y * pitch, statusline, 0x09);
	}

	if (pause)
	{
		int x, y;
		x = temp.ox - 8 * 4;
		y = 0;
		text_i(dst + x + y * pitch, "pause", 0x0F);
	}

	if (!conf.led.enabled) return;

	update_led_ptrs(dst);

	if (temp.led.ay) ay_led();
	if (temp.led.perf) perf_led();
	if (temp.led.load) load_led();
	if (temp.led.input) input_led();
	if (temp.led.time) time_led();
#ifdef MOD_MONITOR
	if (temp.led.osw) debug_led();
#endif
#ifdef MOD_MEMBAND_LED
	if (temp.led.memband) memband_led();
#endif
	if (conf.led.flash_ay_kbd && hndKbdDev) ay_kbd();
	if (input.keymode == k_input::km_keystick) key_led();
}