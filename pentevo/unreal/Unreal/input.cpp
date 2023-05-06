#include "std.h"
#include "emul.h"
#include "vars.h"
#include "dx/dx.h"
#include "tape.h"
#include "input.h"
#include "inputpc.h"
#include "emulator/debugger/debug.h"
#include "util.h"

u8 pastekeys[0x80 - 0x20] =
{
	// s     !     "     #     $     %     &     '     (     )     *     +     ,     -   .       /
	0x71, 0xB1, 0xD1, 0xB3, 0xB4, 0xB5, 0xC5, 0xC4, 0xC3, 0xC2, 0xF5, 0xE3, 0xF4, 0xE4, 0xF3, 0x85,
	// 0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
	0x41, 0x31, 0x32, 0x33, 0x34, 0x35, 0x45, 0x44, 0x43, 0x42, 0x82, 0xD2, 0xA4, 0xE2, 0xA5, 0x84,
	// @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
	0xB2, 0x19, 0x7D, 0x0C, 0x1B, 0x2B, 0x1C, 0x1D, 0x6D, 0x5B, 0x6C, 0x6B, 0x6A, 0x7B, 0x7C, 0x5A,
	// P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
	0x59, 0x29, 0x2C, 0x1A, 0x2D, 0x5C, 0x0D, 0x2A, 0x0B, 0x5D, 0x0A, 0xD5, 0x93, 0xD4, 0xE5, 0xC1,
	// `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
	0x83, 0x11, 0x75, 0x04, 0x13, 0x23, 0x14, 0x15, 0x65, 0x53, 0x64, 0x63, 0x62, 0x73, 0x74, 0x52,
	// p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~
	0x51, 0x21, 0x24, 0x12, 0x25, 0x54, 0x05, 0x22, 0x03, 0x55, 0x02, 0x94, 0x92, 0x95, 0x91, 0xC4
}; //`=0x83, 127=' - Alone Coder

u8 ruspastekeys[64] =
{
	'A','B','W','G','D','E','V','Z','I','J','K','L','M','N','O','P',
	'R','S','T','U','F','H','C','^','[',']',127,'Y','X','\\',64,'Q',
	'a','b','w','g','d','e','v','z','i','j','k','l','m','n','o','p',
	'r','s','t','u','f','h','c','~','{','}','_','y','x','|','`','q'
}; //Alone Coder

void k_input::clear_zx()
{
	for (int i = 0; i < _countof(kbd_x4); i++)
		kbd_x4[i] = -1;
}

inline void k_input::press_zx(u8 key)
{
	if (key & 0x08)
		kbd[0] &= ~1; // caps
	if (key & 0x80)
		kbd[7] &= ~2; // sym
	if (key & 7)
		kbd[(key >> 4) & 7] &= ~(1 << ((key & 7) - 1));
}

bool k_input::process_pc_layout()
{
	for (unsigned i = 0; i < pc_layout_count; i++)
	{
		if (kbdpc[pc_layout[i].vkey] & 0x80)
		{
			press_zx(((kbdpc[DIK_LSHIFT] | kbdpc[DIK_RSHIFT]) & 0x80) ? pc_layout[i].shifted : pc_layout[i].normal);
			return true;
		}
	}
	return false;
}

void k_input::make_matrix()
{
	u8 altlock = conf.input.altlock ? (kbdpc[DIK_LMENU] | kbdpc[DIK_RMENU]) & 0x80 : 0;
	int i;

	kjoy = 0xFF;
	switch (keymode)
	{
	case km_default:
		clear_zx();
		if (!altlock)
		{
			if (!conf.input.keybpcmode || !process_pc_layout())
			{
				for (i = 0; i < VK_MAX; i++)
				{
					if (kbdpc[i] & 0x80)
					{
						*(inports[i].port1) &= inports[i].mask1;
						*(inports[i].port2) &= inports[i].mask2;
					}
				}
			}
		}

		if (conf.input.fire)
		{
			if (!--firedelay)
				firedelay = conf.input.firedelay, firestate ^= 1;
			const zxkeymap* active_zxk = conf.input.active_zxk;
			if (firestate) *(active_zxk->zxk[conf.input.firenum].port) &= active_zxk->zxk[conf.input.firenum].mask;
		}
		break;

	case km_keystick:
		for (i = 0; i < _countof(kbd_x4); i++)
			kbd_x4[i] = rkbd_x4[i];
		if (stick_delay) stick_delay--, altlock = 1;
		if (!altlock)
			for (i = 0; i < VK_MAX; i++)
				if (kbdpc[i] & 0x80)
					*(inports[i].port1) ^= ~inports[i].mask1,
					* (inports[i].port2) ^= ~inports[i].mask2;
		if ((kbd_x4[0] ^ rkbd_x4[0]) | (kbd_x4[1] ^ rkbd_x4[1])) stick_delay = 10;
		break;

	case km_paste_hold:
	{
		clear_zx();
		if (tdata & 0x08) kbd[0] &= ~1; // caps
		if (tdata & 0x80) kbd[7] &= ~2; // sym
		if (tdata & 7) kbd[(tdata >> 4) & 7] &= ~(1 << ((tdata & 7) - 1));
		if (tdelay) { tdelay--; break; }
		tdelay = conf.input.paste_release;
		if (tdata == 0x61) tdelay += conf.input.paste_newline;
		keymode = km_paste_release;
		break;
	}

	case km_paste_release:
	{
		clear_zx();
		if (tdelay) { tdelay--; break; }
		if (textsize == textoffset)
		{
			keymode = km_default;
			free(textbuffer);
			textbuffer = 0;
			break;
		}
		tdelay = conf.input.paste_hold;
		u8 kdata = textbuffer[textoffset++];
		if (kdata == 0x0D)
		{
			if (textoffset < textsize && textbuffer[textoffset] == 0x0A) textoffset++;
			tdata = 0x61;
		}
		else
		{
			if (kdata == 0xA8) kdata = 'E'; //Alone Coder (big YO)
			if ((kdata >= 0xC0) || (kdata == 0xB8)) //RUS
			{
				//pressedit=
				//0 = press edit, pressedit++, textoffset--
				//1 = press letter, pressedit++, textoffset--
				//2 = press edit, pressedit=0
				switch (pressedit)
				{
				case 0:
				{
					tdata = 0x39;
					pressedit++;
					textoffset--;
					break;
				}
				case 1:
				{
					if (kdata == 0xB8) kdata = '&'; else kdata = ruspastekeys[kdata - 0xC0];
					tdata = pastekeys[kdata - 0x20];
					pressedit++;
					textoffset--;
					break;
				}
				case 2:
				{
					tdata = 0x39;
					pressedit = 0;
				}
				default: ;
				}
				if (!tdata)
					break; // empty key
			} //Alone Coder
			else
			{
				if (kdata < 0x20 || kdata >= 0x80) break; // keep release state
				tdata = pastekeys[kdata - 0x20];
				if (!tdata) break; // empty key
			}
		}
		keymode = km_paste_hold;
		break;
	}
	}
	kjoy ^= 0xFF;
	if (conf.input.joymouse)
		kjoy |= mousejoy;

	for (i = 0; i < _countof(kbd_x4); i++)
		rkbd_x4[i] = kbd_x4[i];
	if (!conf.input.keymatrix)
		return;
	for (;;)
	{
		char done = 1;
		for (int k = 0; k < _countof(kbd) - 1; k++)
		{
			for (int j = k + 1; j < _countof(kbd); j++)
			{
				if (((kbd[k] | kbd[j]) != 0xFF) && (kbd[k] != kbd[j]))
				{
					kbd[k] = kbd[j] = (kbd[k] & kbd[j]);
					done = 0;
				}
			}
		}
		if (done)
			return;
	}
}

__inline int sign_pm(int a) { return (a < 0) ? -1 : 1; }

char k_input::readdevices()
{
	if (nokb) nokb--;
	if (nomouse) nomouse--;

	kbdpc[VK_JLEFT] = kbdpc[VK_JRIGHT] = kbdpc[VK_JUP] = kbdpc[VK_JDOWN] = kbdpc[VK_JFIRE] = 0;
	int i;
	for (i = 0; i < 32; i++)
		kbdpc[VK_JB0 + i] = 0;
	if (active && dijoyst)
	{
		dijoyst->Poll();
		DIJOYSTATE js;
		readdevice(&js, sizeof js, dijoyst);
		if ((i16)js.lX < 0) kbdpc[VK_JLEFT] = 0x80;
		if ((i16)js.lX > 0) kbdpc[VK_JRIGHT] = 0x80;
		if ((i16)js.lY < 0) kbdpc[VK_JUP] = 0x80;
		if ((i16)js.lY > 0) kbdpc[VK_JDOWN] = 0x80;

		for (i = 0; i < 32; i++)
		{
			if (js.rgbButtons[i] & 0x80)
				kbdpc[VK_JB0 + i] = 0x80;
		}
	}

	mbuttons = 0xFF;
	msx_prev = msx, msy_prev = msy;
	kbdpc[VK_LMB] = kbdpc[VK_RMB] = kbdpc[VK_MMB] = kbdpc[VK_MWU] = kbdpc[VK_MWD] = 0;
	if ((conf.fullscr || conf.lockmouse) && !nomouse)
	{
		unsigned cl1, cl2;
		cl1 = abs(msx - msx_prev) * ay_reset_t / conf.frame;
		cl2 = abs(msx - msx_prev);
		ay_x0 += (cl2 - cl1) * sign_pm(msx - msx_prev);
		cl1 = abs(msy - msy_prev) * ay_reset_t / conf.frame;
		cl2 = abs(msy - msy_prev);
		ay_y0 += (cl2 - cl1) * sign_pm(msy - msy_prev);
		ay_reset_t = 0;

		//      printf("%s\n", __FUNCTION__);
		DIMOUSESTATE md;
		readmouse(&md);
		if (conf.input.mouseswap)
		{
			u8 t = md.rgbButtons[0];
			md.rgbButtons[0] = md.rgbButtons[1];
			md.rgbButtons[1] = t;
		}
		msx = md.lX; msy = -md.lY;
		if (conf.input.mousescale >= 0)
		{
			msx *= (1 << conf.input.mousescale);
			msy *= (1 << conf.input.mousescale);
		}
		else
		{
			msx /= (1 << -conf.input.mousescale);
			msy /= (1 << -conf.input.mousescale);
		}

		if (md.rgbButtons[0])
		{
			mbuttons &= ~1;
			kbdpc[VK_LMB] = 0x80;
		}
		if (md.rgbButtons[1])
		{
			mbuttons &= ~2;
			kbdpc[VK_RMB] = 0x80;
		}
		if (md.rgbButtons[2])
		{
			mbuttons &= ~4;
			kbdpc[VK_MMB] = 0x80;
		}

		int wheel_delta = md.lZ - prev_wheel;
		prev_wheel = md.lZ;
		//      if (wheel_delta < 0) kbdpc[VK_MWD] = 0x80;
		//      if (wheel_delta > 0) kbdpc[VK_MWU] = 0x80;
		//0.36.6 from 0.35b2
		if (conf.input.mousewheel == mouse_wheel_mode::keyboard)
		{
			if (wheel_delta < 0)
				kbdpc[VK_MWD] = 0x80;
			if (wheel_delta > 0)
				kbdpc[VK_MWU] = 0x80;
		}

		if (conf.input.mousewheel == mouse_wheel_mode::kempston)
		{
			if (wheel_delta < 0)
				wheel -= 0x10;
			if (wheel_delta > 0)
				wheel += 0x10;
			mbuttons = (mbuttons & 0x0F) + (wheel & 0xF0);
		}
		//~
	}

	if (nokb)
		memset(kbdpc, 0, sizeof(kbdpc));
	else
	{
		static u8 kbdpc_prev[VK_MAX];
		if (!dbgbreak && buffer.Enabled())
			memcpy(kbdpc_prev, kbdpc, sizeof(kbdpc));

		read_keyboard(kbdpc);

		if (!dbgbreak && input.buffer.Enabled()) // TODO: нажатие и отжатие ESC попадает в буфер
		{
			for (int i = 0; i < sizeof(kbdpc); i++)
			{
				if ((kbdpc[i] & 0x80) != (kbdpc_prev[i] & 0x80) && dik_scan[i])
				{
					if (dik_scan[i] & 0x0100) input.buffer.Push(0xE0);
					if (kbdpc_prev[i] & 0x80) input.buffer.Push(0xF0);
					input.buffer.Push(dik_scan[i] & 0x00FF);
				}
			}
		}
	}
	lastkey = process_msgs();

	return lastkey ? 1 : 0;
}

void k_input::aymouse_wr(u8 val)
{
	// reset by edge bit6: 1->0
	if (ay_r14 & ~val & 0x40) ay_x0 = ay_y0 = 8, ay_reset_t = cpu.t;
	ay_r14 = val;
}

u8 k_input::aymouse_rd() const
{
	unsigned coord;
	if (ay_r14 & 0x40) {
		const unsigned cl1 = abs(msy - msy_prev) * ay_reset_t / conf.frame;
		const unsigned cl2 = abs(msy - msy_prev) * cpu.t / conf.frame;
		coord = ay_y0 + (cl2 - cl1) * sign_pm(msy - msy_prev);
	}
	else {
		const unsigned cl1 = abs(msx - msx_prev) * ay_reset_t / conf.frame;
		const unsigned cl2 = abs(msx - msx_prev) * cpu.t / conf.frame;
		coord = ay_x0 + (cl2 - cl1) * sign_pm(msx - msx_prev);
	}
	
	return 0xC0 | (coord & 0x0F) | (mbuttons << 4);
}

u8 k_input::kempston_mx() const
{
	const int x = (cpu.t * msx + (conf.frame - cpu.t) * msx_prev) / conf.frame;
	return (u8)x;
}

u8 k_input::kempston_my() const
{
	const int y = (cpu.t * msy + (conf.frame - cpu.t) * msy_prev) / conf.frame;
	return (u8)y;
}

u8 k_input::read(u8 scan)
{
	u8 res = 0xBF | (tape_bit() & 0x40);
	kbdled &= scan;

	for (int i = 0; i < 8; i++)
	{
		if (!(scan & (1 << i)))
			res &= kbd[i];
	}

	return res;
}

// read quorum additional keys (port 7E)
u8 k_input::read_quorum(u8 scan)
{
	u8 res = 0xFF;
	kbdled &= scan;

	for (int i = 0; i < 8; i++)
	{
		if (!(scan & (1 << i)))
			res &= kbd[8 + i];
	}

	return res;
}

void k_input::paste()
{
	free(textbuffer); textbuffer = 0;
	textsize = textoffset = 0;
	keymode = km_default;
	if (!OpenClipboard(wnd)) return;
	const HANDLE hClip = GetClipboardData(CF_TEXT);
	if (hClip) {
		void* ptr = GlobalLock(hClip);
		if (ptr) {
			keymode = km_paste_release; tdelay = 1;
			textsize = strlen((char*)ptr) + 1;
			memcpy(textbuffer = (u8*)malloc(textsize), ptr, textsize);
			GlobalUnlock(hClip);
		}
	}
	CloseClipboard();
}
