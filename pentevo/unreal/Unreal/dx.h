#pragma once

enum
{
    DRIVER_DDRAW, 
    DRIVER_DDRAW16,
    DRIVER_DDRAW32,
    DRIVER_GDI, 
    DRIVER_OVR,
    DRIVER_BLT,
};

#define MAXWQSIZE 32

extern const RENDER drivers[];
extern size_t renders_count;

extern unsigned char active;
extern unsigned char pause;

extern LPDIRECTDRAW2 dd;
extern LPDIRECTDRAWSURFACE sprim;
extern LPDIRECTDRAWSURFACE surf0;
extern LPDIRECTDRAWSURFACE surf1;

extern LPDIRECTINPUTDEVICE2 dijoyst;

void sound_play();
void sound_stop();
void __fastcall do_sound_none();
void __fastcall do_sound_wave();
void __fastcall do_sound_ds();
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
void ReadKeyboard(PVOID KbdData);
