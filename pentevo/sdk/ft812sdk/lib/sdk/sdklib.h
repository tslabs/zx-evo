
#pragma once

u8 getkey();
void anykey();
void anykey_press();
void anykey_release();
void di();
void ei();
void putc(u8);

#if ((__SDCC_VERSION_MAJOR == 3) && (__SDCC_VERSION_MINOR >= 7)) || (__SDCC_VERSION_MAJOR > 3)
  #define _PUTCHAR_PROTO int putchar(u8 c)
#else
  #define _PUTCHAR_PROTO void putchar(u8 c)
#endif

_PUTCHAR_PROTO;

#define asm(a) __asm a __endasm
