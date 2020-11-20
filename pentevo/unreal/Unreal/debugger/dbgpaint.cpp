#include "std.h"
#include "emul.h"
#include "vars.h"
#include "debug.h"
#include "dbgpaint.h"
#include "dx.h"
#include "font16.h"
#include "util.h"

u8 txtscr[debug_text_width * debug_text_height * 2];

static struct
{
	u8 x, y, dx, dy, c;
} frames[50];

unsigned nfr;

void debugflip()
{
    // draw screen memory
    if (show_scrshot) {
        for (auto y = 0; y < wat_sz_y * 16; y++) {
            auto *pixelBuf = debug_gdibuf + ((y + wat_y * 16) * DEBUG_WND_WIDTH) + (wat_x * 8);

            // top/bottom border
            if ((y < (((wat_sz_y * 16) - 192) / 2)) || (y >= (192 + (((wat_sz_y * 16) - 192) / 2)))) {
                for (auto x = 0; x < 37 * 8; x++) *pixelBuf++ = comp.ts.border & 0x7;
            } else {
                // left border
                for (auto x = 0; x < (((wat_sz_x * 8) - 256) / 2); x++) *pixelBuf++ = comp.ts.border & 0x7;
                
                // screen memory
                auto ys = y - (((wat_sz_y * 16) - 192) / 2);
                auto g = ((ys & 0x07) << 8) + ((ys & 0x38) << 2) + ((ys & 0xC0) << 5);
                auto a = ((ys & 0xF8) << 2) + 0x1800;
                auto page = ((comp.p7FFD ^ scrshot_page_mask) & 0x08) ? 7 : 5;
                auto *scr = page_ram(page) + g;
                auto *atr = page_ram(page) + a;

                for (auto x = 0; x < 32; x++) {
                    auto pixel = *scr++;
                    for (auto c = 0; c < 8; c++) {
                        *pixelBuf++ = (pixel & 0x80) ? (*atr & 7) | ((*atr & 0x40) >> 3) : ((*atr & 0x38) >> 3) | ((*atr & 0x40) >> 3);
                        pixel <<= 1;
                    }
                    atr++;
                }
                
                
                // right border
                for (auto x = (256 + ((wat_sz_x * 8) - 256) / 2); x < wat_sz_x * 8; x++) *pixelBuf++ = comp.ts.border & 0x7;

            }
        }
    }

	// print text
	for (auto y = 0; y < debug_text_height; y++)
		for (auto x = 0; x < debug_text_width; x++)
		{
			const auto atr = txtscr[(y * debug_text_width) + x + debug_text_size];
			if (atr == 0xFF) continue;   // transparent color
			const auto chr = txtscr[(y * debug_text_width) + x];
			auto bp = (y * DEBUG_WND_WIDTH * 16) + (x * 8);

			for (auto by = 0; by < 16; by++)
			{
				auto f = font16[(chr * 16) + by];

				for (auto bx = 0; bx < 8; bx++)
				{
					debug_gdibuf[bp + bx] = (f & 0x80) ? (atr & 0xF) : (atr >> 4);
					f <<= 1;
				}

				bp += DEBUG_WND_WIDTH;
			}
		}

	// show frames
	for (unsigned i = 0; i < nfr; i++)
	{
		const u8 a1 = (frames[i].c | 0x08) * 0x11;
		auto y = frames[i].y * 16 - 1;
		for (auto x = 8 * frames[i].x - 1; x < (frames[i].x + frames[i].dx) * 8; x++) debug_gdibuf[y * DEBUG_WND_WIDTH + x] = a1;
		y = (frames[i].y + frames[i].dy) * 16;
		for (auto x = 8 * frames[i].x - 1; x < (frames[i].x + frames[i].dx) * 8; x++) debug_gdibuf[y * DEBUG_WND_WIDTH + x] = a1;
		auto x = frames[i].x * 8 - 1;
		for (y = 16 * frames[i].y; y < (frames[i].y + frames[i].dy) * 16; y++) debug_gdibuf[y * DEBUG_WND_WIDTH + x] = a1;
		x = (frames[i].x + frames[i].dx) * 8;
		for (y = 16 * frames[i].y; y < (frames[i].y + frames[i].dy) * 16; y++) debug_gdibuf[y * DEBUG_WND_WIDTH + x] = a1;
	}

	/*gdibmp.header.bmiHeader.biBitCount = 8;
	if (needclr)
		gdi_frame();
	SetDIBitsToDevice(temp.debug_gdidc, temp.gx, temp.gy, 640, 480, 0, 0, 0, 480, bptr, &gdibmp.header, DIB_RGB_COLORS);
	gdibmp.header.bmiHeader.biBitCount = temp.obpp;*/
	InvalidateRect(debug_wnd, nullptr, FALSE);
}

void frame(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 attr)
{
	frames[nfr].x = x;
	frames[nfr].y = y;
	frames[nfr].dx = dx;
	frames[nfr].dy = dy;
	frames[nfr].c = attr;
	nfr++;
}

void tprint(unsigned x, unsigned y, const char *str, u8 attr)
{
	for (unsigned ptr = y * debug_text_width + x; *str; str++, ptr++) {
		txtscr[ptr] = *str; txtscr[ptr + debug_text_width * debug_text_height] = attr;
	}
}

void tprint_fg(unsigned x, unsigned y, const char *str, u8 attr)
{
	for (auto ptr = y * debug_text_width + x; *str; str++, ptr++) {
		txtscr[ptr] = *str; txtscr[ptr + debug_text_width * debug_text_height] = (txtscr[ptr + debug_text_width * debug_text_height] & 0xF0) + attr;
	}
}

void filledframe(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 color)
{
	for (auto yy = y; yy < (y + dy); yy++)
		for (auto xx = x; xx < (x + dx); xx++)
			txtscr[yy * debug_text_width + xx] = ' ',
			txtscr[yy * debug_text_width + xx + debug_text_height * debug_text_width] = color;
	nfr = 0; // delete other frames while dialog
	frame(x, y, dx, dy, fframe_frame);
}

void fillattr(unsigned x, unsigned y, unsigned dx, u8 color)
{
	for (auto xx = x; xx < (x + dx); xx++)
		txtscr[y* debug_text_width + xx + debug_text_height * debug_text_width] = color;
}

void fillrect(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 color)
{
	for (auto yy = y; yy < (y + dy); yy++)
		for (auto xx = x; xx < (x + dx); xx++)
			txtscr[yy * debug_text_width + xx] = ' ',
			txtscr[yy * debug_text_width + xx + debug_text_height * debug_text_width] = color;
}

char str[0x80];
unsigned inputhex(unsigned x, unsigned y, unsigned sz, bool hex)
{
	unsigned cr = 0;
	mousepos = 0;

	for (;;)
	{
		str[sz] = 0;

		unsigned i;
		for (i = strlen(str); i < sz; i++)
			str[i] = ' ';
		for (i = 0; i < sz; i++)
		{
			unsigned vl = u8(str[i]);
			tprint(x + i, y, reinterpret_cast<char*>(&vl), (i == cr) ? w_inputcur : w_inputbg);
		}

		debugflip();

		unsigned key;
		for (;; Sleep(20))
		{
			key = process_msgs();
			needclr = 0;
			debugflip();

			if (mousepos)
				return 0;
			if (key)
				break;
		}

		switch (key)
		{
		case VK_ESCAPE: return 0;
		case VK_RETURN:
			for (auto ptr = str + sz - 1; *ptr == ' ' && ptr >= str; *ptr-- = 0) {}
			return 1;
		case VK_LEFT:
			if (cr)
				cr--;
			continue;
		case VK_BACK:
			if (cr)
			{
				for (i = cr; i < sz; i++)
					str[i - 1] = str[i];
				str[sz - 1] = ' ';
				--cr;
			}
			continue;
		case VK_RIGHT:
			if (cr != sz - 1)
				cr++;
			continue;
		case VK_HOME:
			cr = 0;
			continue;
		case VK_END:
			for (cr = sz - 1; cr && str[cr] == ' ' && str[cr - 1] == ' '; cr--) {}
			continue;
		case VK_DELETE:
			for (i = cr; i < sz - 1; i++)
				str[i] = str[i + 1];
			str[sz - 1] = ' ';
			continue;
		case VK_INSERT:
			for (i = sz - 1; i > cr; i--)
				str[i] = str[i - 1];
			str[cr] = ' ';
			continue;
		default: ;
		}

		if (hex)
		{
			if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'F'))
				str[cr++] = u8(key);
		}
		else
		{
			u8 Kbd[256];
			GetKeyboardState(Kbd);
			u16 k;
			if (ToAscii(key, 0, Kbd, &k, 0) == 1)
			{
				char m;
				if (CharToOemBuff(reinterpret_cast<char *>(&k), &m, 1))
					str[cr++] = m;
			}
		}
		if (cr == sz)
			cr--;
	}
}

unsigned input4(unsigned x, unsigned y, unsigned val)
{
	sprintf(str, "%04X", val);
	if (inputhex(x, y, 4, true))
	{
		sscanf(str, "%x", &val);
		return val;
	}
	return -1;
}

unsigned input2(unsigned x, unsigned y, unsigned val)
{
	sprintf(str, "%02X", val);
	if (inputhex(x, y, 2, true))
	{
		sscanf(str, "%x", &val);
		return val;
	}
	return -1;
}


void format_item(char *dst, unsigned width, const char *text, menuitem::flags_t flags)
{
	memset(dst, ' ', width + 2); dst[width + 2] = 0;
	unsigned sz = strlen(text), left = 0;
	if (sz > width) sz = width;
	if (flags & menuitem::right) left = width - sz;
	else if (flags & menuitem::center) left = (width - sz) / 2;
	memcpy(dst + left + 1, text, sz);
}

void paint_items(menudef *menu)
{
	char ln[debug_text_width]; unsigned item;

	unsigned maxlen = strlen(menu->title);
	for (item = 0; item < menu->n_items; item++) {
		unsigned sz = strlen(menu->items[item].text);
		maxlen = max(maxlen, sz);
	}
	const unsigned menu_dx = maxlen + 2;
	const unsigned menu_dy = menu->n_items + 3;
	const unsigned menu_x = (debug_text_width - menu_dx) / 2;
	const unsigned menu_y = (debug_text_height - menu_dy) / 2;
	filledframe(menu_x, menu_y, menu_dx, menu_dy, menu_inside);
	format_item(ln, maxlen, menu->title, menuitem::center);
	tprint(menu_x, menu_y, ln, menu_header);

	for (/*unsigned*/ item = 0; item < menu->n_items; item++) {
		u8 color = menu_item;
		if (menu->items[item].flags & menuitem::disabled) color = menu_item_dis;
		else if (item == menu->pos) color = menu_cursor;
		format_item(ln, maxlen, menu->items[item].text, menu->items[item].flags);
		tprint(menu_x, menu_y + item + 2, ln, color);
	}
}

void menu_move(menudef *menu, int dir)
{
	const unsigned start = menu->pos;
	for (;;) {
		menu->pos += dir;
		if (int(menu->pos) == -1) menu->pos = menu->n_items - 1;
		if (menu->pos >= menu->n_items) menu->pos = 0;
		if (!(menu->items[menu->pos].flags & menuitem::disabled)) return;
		if (menu->pos == start) return;
	}
}

char handle_menu(menudef *menu)
{
	if (menu->items[menu->pos].flags & menuitem::disabled)
		menu_move(menu, 1);
	for (;;)
	{
		paint_items(menu);
		debugflip();

		unsigned key;
		for (;; Sleep(20))
		{
			key = process_msgs();
			needclr = 0;
			debugflip();

			if (mousepos)
				return 0;
			if (key)
				break;
		}
		if (key == VK_ESCAPE)
			return 0;
		if (key == VK_RETURN || key == VK_SPACE)
			return 1;
		if (key == VK_UP || key == VK_LEFT)
			menu_move(menu, -1);
		if (key == VK_DOWN || key == VK_RIGHT)
			menu_move(menu, 1);
		if (key == VK_HOME || key == VK_PRIOR)
		{
			menu->pos = -1;
			menu_move(menu, 1);
		}
		if (key == VK_END || key == VK_NEXT)
		{
			menu->pos = menu->n_items;
			menu_move(menu, -1);
		}
	}
}
