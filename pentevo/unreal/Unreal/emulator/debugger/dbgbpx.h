#pragma once
#include "emulator/z80/z80.h"

void init_bpx(const char* file);
void done_bpx();

void mon_bpdialog();
unsigned calc(const Z80& cpu, unsigned *script);
