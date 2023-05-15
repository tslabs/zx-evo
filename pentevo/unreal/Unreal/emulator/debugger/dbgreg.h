#pragma once
#include "sysdefs.h"

struct t_reg_layout final
{
   size_t offs;
   u8 width;
   u8 x,y;
   u8 lf,rt,up,dn;
};

extern const t_reg_layout regs_layout[];
extern const size_t regs_layout_count;

void ra();
void rf();
void rbc();
void rde();
void rhl();
void rsp();
void rpc();
void rix();
void riy();
void ri();
void rr();
void rm();
void r_1();
void r_2();
void rSF();
void rZF();
void rF5();
void rHF();
void rF3();
void rPF();
void rNF();
void rCF();

void rcodejump();
void rdatajump();

void rleft();
void rright();
void rup();
void rdown();
void renter();

void showregs();
char dispatch_regs();
