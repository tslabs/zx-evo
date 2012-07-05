#pragma once

struct TRegLayout
{
   size_t offs;
   unsigned char width;
   unsigned char x,y;
   unsigned char lf,rt,up,dn;
};

extern const TRegLayout regs_layout[];
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
