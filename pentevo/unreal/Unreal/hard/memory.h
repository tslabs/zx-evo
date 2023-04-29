#pragma once
#include "sysdefs.h"
#include "emul.h"
#include "vars.h"

void set_mode(ROM_MODE mode);
void set_banks();
void set_scorp_profrom(unsigned read_address);
void cmos_write(u8 val);
u8 cmos_read();
void rand_ram();

Z80INLINE u8 *am_r(unsigned addr)
{
   return bankr[(addr >> 14) & 3] + (addr & (PAGE-1));
}
//u8 rmdbg(u32 addr);
//void wmdbg(u32 addr, u8 val);
//u8 *MemDbg(u32 addr);

enum BANKM
{
    BANKM_ROM = 0,
    BANKM_RAM
};
