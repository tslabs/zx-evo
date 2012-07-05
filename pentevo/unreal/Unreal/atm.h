#pragma once

void set_atm_FF77(unsigned port, unsigned char val);
void set_atm_aFE(unsigned char addr);
void atm_writepal(unsigned char val);
u8 atm_readpal();
unsigned char atm450_z(unsigned t);
