#pragma once

extern HANDLE hndKbdDev;
extern CACHE_ALIGNED u32 vbuf[2][sizeof_vbuf];

void text_i(unsigned char *dst, const char *text, unsigned char ink, unsigned off = 0);
void init_leds();
void done_leds();
void showleds();
void showleds();

void init_memcycles();
void show_memcycles();
