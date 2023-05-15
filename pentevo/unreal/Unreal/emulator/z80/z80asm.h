#pragma once
#include "sysdefs.h"

extern char asmbuf[0x40];
extern u8 asmresult[24];

u8 *disasm(u8 *cmd, unsigned current, char labels);
int assemble_cmd(u8 *cmd, unsigned addr);
