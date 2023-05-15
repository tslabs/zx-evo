#pragma once
#include "sysdefs.h"

namespace z80gs
{
	const int GSCPUFQ = 24000000; // hz //12
	extern int GSCPUFQI;
	extern const unsigned GSCPUINT;
 	extern __int64 gscpu_t_at_frame_start; // gs_t_states+gscpu.t when spectrum frame begins
	extern __int64 gs_t_states; // inc'ed with GSCPUINT every gs int

	extern u8* gsbankr[4]; // bank pointers for read
	extern u8* gsbankw[4]; // bank pointers for write
	

	void apply_gs();
	void init_gs_frame();
	void flush_gs_frame();

	u8 in_gs(unsigned port);
	void out_gs(unsigned port, u8 val);
	void reset();
	void __cdecl BankNames(int i, char* Name);
	__int64 __cdecl delta();
	void __cdecl SetLastT();
	forceinline u8* am_r(u32 addr);
	inline void stepi();

	void flush_gs_z80();

	class TGsZ80 : public Z80
	{
	public:
		TGsZ80(u32 Idx,
			t_bank_names BankNames, step_fn Step, delta_fn Delta,
			set_lastt_fn SetLastT, u8* membits, const t_mem_if* FastMemIf, const t_mem_if* DbgMemIf) :
			Z80(Idx, BankNames, Step, Delta, SetLastT, membits, FastMemIf, DbgMemIf) { }

		u8* direct_mem(unsigned addr) const override; // get direct memory pointer in debuger

		u8 m1_cycle() override;
		u8 in(unsigned port) override;
		void out(unsigned port, u8 val) override;
		void retn() override;

		u8 int_vec() override
		{
			return 0xFF;
		}

		void check_next_frame() override
		{
			if (t >= GSCPUINT)
			{
				t -= GSCPUINT;
				eipos -= GSCPUINT;
				gs_t_states += GSCPUINT;
				int_pend = true;
			}
		}

		
	};

	void fastcall step();

	void gs_byte_to_dac(unsigned addr, u8 byte);
	extern u8 membits[0x10000];
	extern SNDRENDER sound;

	extern TGsZ80 gscpu;
	
}

