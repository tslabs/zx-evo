#pragma once
#include "sysdefs.h"

u8 Rm(u32 addr);
void Wm(u32 addr, u8 val);
u8 DbgRm(u32 addr);
void DbgWm(u32 addr, u8 val);

void reset(rom_mode mode);
void reset_sound(void);
void set_clk(void);
