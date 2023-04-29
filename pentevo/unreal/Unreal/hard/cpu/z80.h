#pragma once
#include "sysdefs.h"

#ifdef MOD_FASTCORE
   namespace z80fast
   {
   #include "z80_main.h"
   }
#else
   #define z80fast z80dbg
#endif

#ifdef MOD_DEBUGCORE
   namespace z80dbg
   {
   #include "z80_main.h"
   }
#else
   #define z80dbg z80fast
#endif

u8 Rm(u32 addr);
void Wm(u32 addr, u8 val);
u8 DbgRm(u32 addr);
void DbgWm(u32 addr, u8 val);

void reset(ROM_MODE mode);
void reset_sound(void);
void set_clk(void);
