#pragma once
#include "sysdefs.h"

struct TGsLed
{
    unsigned level;
    unsigned attrib;
};

extern TGsLed gsleds[8];
extern unsigned gs_vfx[0x41];

void make_gs_volume(unsigned level = 0x3F);
void reset_gs();
void init_gs();
void done_gs();
void out_gs(u8 port, u8 byte);
u8 in_gs(u8 port);

void init_gs_frame();
void flush_gs_frame();
void reset_gs_sound();
void apply_gs();
