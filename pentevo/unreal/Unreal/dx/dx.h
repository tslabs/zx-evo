#pragma once
#include "sysdefs.h"
#include "ddraw.h"
#include "dinput.h"

constexpr auto MAXWQSIZE = 32;

extern const DRIVER drivers[3];
extern size_t renders_count;

extern u8 active;
extern u8 pause;

extern LPDIRECTDRAW2 dd;
extern LPDIRECTDRAWSURFACE sprim;
extern LPDIRECTDRAWSURFACE surf0;
extern LPDIRECTDRAWSURFACE surf1;

extern LPDIRECTINPUTDEVICE2 dijoyst;

void sound_play();
void sound_stop();
void do_sound_none();
void do_sound_wave();
void do_sound_ds();
void do_sound();
void setpal(char system);
void set_priority();

void flip();
void set_vidmode();
void updatebitmap();
void adjust_mouse_cursor();
void start_dx();
void done_dx();
void scale_normal();

void readdevice(VOID *md, DWORD sz, LPDIRECTINPUTDEVICE dev);
void readmouse(DIMOUSESTATE *md);
void read_keyboard(PVOID kbd_data);
