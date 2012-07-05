#pragma once

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

u8 __fastcall Rm(u32 addr);
void __fastcall Wm(u32 addr, u8 val);
u8 __fastcall DbgRm(u32 addr);
void __fastcall DbgWm(u32 addr, u8 val);

void reset(ROM_MODE mode);
