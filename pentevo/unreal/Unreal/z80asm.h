#pragma once

extern char asmbuf[0x40];
extern unsigned char asmresult[24];

unsigned char *disasm(unsigned char *cmd, unsigned current, char labels);
int assemble_cmd(unsigned char *cmd, unsigned addr);
