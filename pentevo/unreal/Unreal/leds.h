#pragma once

extern HANDLE hndKbdDev;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

void text_i(u32 *dst, const char *text, u8 ink, unsigned off = 0);
void init_leds();
void done_leds();
void showleds( u32 *dst, unsigned pitch );

void init_memcycles();
void show_memcycles();
