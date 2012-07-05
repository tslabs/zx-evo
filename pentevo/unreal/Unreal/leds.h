#pragma once

extern HANDLE hndKbdDev;

void init_leds();
void done_leds();

void text_i(unsigned char *dst, const char *text, unsigned char ink, unsigned off = 0);
void showleds();
