#pragma once

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

u8 __fastcall Rm(u32 addr);
void __fastcall Wm(u32 addr, u8 val);
u8 __fastcall DbgRm(u32 addr);
void __fastcall DbgWm(u32 addr, u8 val);
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
   virtual unsigned char rm(unsigned addr) override;
   virtual unsigned char dbgrm(unsigned addr) override;
   virtual void wm(unsigned addr, unsigned char val) override;
   virtual void dbgwm(unsigned addr, unsigned char val) override;
*/
   virtual unsigned char m1_cycle() override;
   virtual unsigned char in(unsigned port) override;
   virtual void out(unsigned port, unsigned char val) override;
   virtual u8 IntVec() override;
   virtual void CheckNextFrame() override;
   virtual void retn() override;
};
