#pragma once
#include "sysdefs.h"

void set_turbo(void);
void set_atm_FF77(unsigned port, u8 val);
void set_atm_aFE(u8 addr);
void atm_writepal(u8 val);
u8 atm_readpal();
u8 atm450_z(unsigned t);
