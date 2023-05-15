#pragma once
#include "sysdefs.h"

u8 *editam(unsigned addr);

__inline u8 editrm(unsigned addr)
{
	const auto ptr = editam(addr);
	return ptr ? *ptr : 0;
}

unsigned memadr(unsigned addr);

void mleft();
void mright();
void mup();
void mdown();
void mpgdn();
void mpgup();
void mswitch();
void mstl();
void mendl();
void mtext();
void mcode();
void mgoto();
void mmodemem();
void mmodephys();
void mmodelog();
void mdiskgo();

void mpc();
void msp();
void mbc();
void mde();
void mhl();
void mix();
void miy();
void showmem();
char dispatch_mem();
