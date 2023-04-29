#pragma once
#include "sysdefs.h"

namespace z80gs
{
const int GSCPUFQ = 24000000; // hz //12
extern const unsigned GSCPUINT;
extern unsigned __int64 gs_t_states; // inc'ed with GSCPUINT every gs int

void apply_gs();
void init_gs_frame();
void flush_gs_frame();

u8 in_gs(unsigned port);
void out_gs(unsigned port, u8 val);
void reset();
void __cdecl BankNames(int i, char *Name);
__int64 __cdecl delta();
void __cdecl SetLastT();

u8 Rm(u32 addr);
void Wm(u32 addr, u8 val);
u8 DbgRm(u32 addr);
void DbgWm(u32 addr, u8 val);
}

class TGsZ80 : public Z80
{
public:
   TGsZ80(u32 Idx, 
       TBankNames BankNames, TStep Step, TDelta Delta,
       TSetLastT SetLastT, u8 *membits, const TMemIf *FastMemIf, const TMemIf *DbgMemIf) :
       Z80(Idx, BankNames, Step, Delta, SetLastT, membits, FastMemIf, DbgMemIf) { }

   virtual u8 *DirectMem(unsigned addr) const override; // get direct memory pointer in debuger
/*
   virtual u8 rm(unsigned addr) override;
   virtual u8 dbgrm(unsigned addr) override;
   virtual void wm(unsigned addr, u8 val) override;
   virtual void dbgwm(unsigned addr, u8 val) override;
*/
   virtual u8 m1_cycle() override;
   virtual u8 in(unsigned port) override;
   virtual void out(unsigned port, u8 val) override;
   virtual u8 IntVec() override;
   virtual void CheckNextFrame() override;
   virtual void retn() override;
};
