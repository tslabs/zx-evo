#pragma once
#include "defs.h"

void init_bpx(char* file);
void done_bpx();

void mon_bpdialog();
unsigned calc(const Z80 *cpu, unsigned *script);
