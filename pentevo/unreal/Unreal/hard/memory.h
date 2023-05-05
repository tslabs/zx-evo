#pragma once
#include "sysdefs.h"
#include "emul.h"
#include "vars.h"

void set_mode(rom_mode mode);
void set_banks();
void set_scorp_profrom(unsigned read_address);
void cmos_write(u8 val);
u8 cmos_read();
void rand_ram();

forceinline u8 *am_r(const unsigned addr)
{
   return bankr[(addr >> 14) & 3] + (addr & (PAGE-1));
}
