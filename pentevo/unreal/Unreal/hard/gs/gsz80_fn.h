#pragma once
#include "sysdefs.h"
#include "gsz80.h"

namespace z80gs
{
	// Адрес может превышать 0xFFFF
	// (чтобы в каждой команде работы с регистрами не делать &= 0xFFFF)
	template <bool use_debug>
	forceinline u8 gsz80_rm(u32 addr)
	{
		addr &= 0xFFFF;
		if constexpr (use_debug) {
			u8* membit = membits + (addr & 0xFFFF);
			*membit |= MEMBITS_R;
			dbgbreak |= (*membit & MEMBITS_BPR);
			gscpu.dbgbreak |= (*membit & MEMBITS_BPR);
		}

		const u8 byte = *am_r(addr);
		if ((addr & 0xE000) == 0x6000)
			gs_byte_to_dac(addr, byte);
		return byte;
	}

	// Адрес может превышать 0xFFFF
	// (чтобы в каждой команде работы с регистрами не делать &= 0xFFFF)
	template <bool use_debug>
	forceinline void gsz80_wm(u32 addr, u8 val)
	{
		addr &= 0xFFFF;
		if constexpr (use_debug) {
			u8* membit = z80gs::membits + (addr & 0xFFFF);
			*membit |= MEMBITS_W;
			dbgbreak |= (*membit & MEMBITS_BPW);
			gscpu.dbgbreak |= (*membit & MEMBITS_BPW);
		}

		u8* bank = gsbankw[(addr >> 14) & 3];
		bank[addr & (PAGE - 1)] = val;
	}

	template <bool use_debug>
	void gsz80_z80loop()
	{
		u64 end = u64((float(cpu.t) * float(GSCPUFQI)) / float(conf.frame) + 0.5f);//((cpu.t * mult_gs2) >> MULT_GS_SHIFT); //; // t*GSCPUFQI/conf.frame;
		end += gscpu_t_at_frame_start;
		for (;;)
		{ // while (gs_t_states+gscpu.t < end)

			while ((gs_t_states + gscpu.t < end) && (gscpu.t < GSCPUINT)) // (int)gscpu.t < max_gscpu_t
			{
				if constexpr (use_debug) {
					debug_events(gscpu);
				}
				stepi();
			}
			if (gscpu.t < GSCPUINT)
				break;

			gscpu.int_pend = true;
			if (gscpu.iff1 && gscpu.t != gscpu.eipos) // interrupt, but not after EI
				gscpu.handle_int(gscpu, gscpu.int_vec());

			gscpu.t -= GSCPUINT;
			gs_t_states += GSCPUINT;
		}
	}
}