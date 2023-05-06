#pragma once

#include "debug.h"

struct menuitem final
{
   const char *text;
   enum flags_t { disabled = 1, left = 2, right = 4, center = 8 } flags;
};

struct menudef final
{
   menuitem *items;
   unsigned n_items;
   const char *title;
   unsigned pos;
};

extern u8 txtscr[debug_text_width * debug_text_height * 2];
extern char str[0x80];
extern unsigned nfr;

void filledframe(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 color = fframe_inside);
void fillattr(unsigned x, unsigned y, unsigned dx, u8 color = fframe_inside);
void fillrect(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 color = fframe_inside);
void tprint(unsigned x, unsigned y, const char *str, u8 attr);
void tprint_fg(unsigned x, unsigned y, const char *str, u8 attr);
unsigned inputhex(unsigned x, unsigned y, unsigned sz, bool hex);
unsigned input2(unsigned x, unsigned y, unsigned val);
unsigned input4(unsigned x, unsigned y, unsigned val);
void debugflip();
char handle_menu(menudef *menu);
void frame(unsigned x, unsigned y, unsigned dx, unsigned dy, u8 attr);
