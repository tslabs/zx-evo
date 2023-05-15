#include "std.h"
#include "emulator/z80/z80.h"
#include "emul.h"
#include "vars.h"
#include "gs.h"
#include "gsz80.h"
#include "gsz80_fn.h"
#include "vs1001.h"
#include "hard/sdcard.h"

namespace z80gs
{

	__int64 gs_t_states; // inc'ed with GSCPUINT every gs int
	__int64 gscpu_t_at_frame_start; // gs_t_states+gscpu.t when spectrum frame begins

	static constexpr t_mem_if fast_mem_if = { gsz80_rm<false>, gsz80_wm<false> };
	static constexpr t_mem_if dbg_mem_if = { gsz80_rm<true>, gsz80_wm<true> };

	TGsZ80 gscpu(1, BankNames, step, delta, SetLastT, membits, &fast_mem_if, &dbg_mem_if);

	SNDRENDER sound;
	u8 membits[0x10000];

	forceinline u8 m1_cycle(Z80& cpu);

	u8 in(unsigned port);
	void out(unsigned port, u8 val);

	void flush_gs_z80()
	{
		if (gscpu.dbgchk)
		{
			gscpu.set_dbg_mem_if();
			gsz80_z80loop<true>();
		}
		else
		{
			gscpu.set_fast_mem_if();
			gsz80_z80loop<false>();
		}
	}

	u8* TGsZ80::direct_mem(unsigned addr) const
	{
		return z80gs::am_r(addr);
	}

	u8 TGsZ80::m1_cycle()
	{
		return z80gs::m1_cycle(*this);
	}

	u8 TGsZ80::in(unsigned port)
	{
		return z80gs::in(port);
	}

	void TGsZ80::out(const unsigned port, const u8 val)
	{
		z80gs::out(port, val);
	}

	void TGsZ80::retn()
	{
		nmi_in_progress = false;
	}

	constexpr u8 MPAG = 0x00;
	constexpr u8 MPAGEX = 0x10;

	constexpr u8 DMA_MOD = 0x1b;
	constexpr u8 DMA_HAD = 0x1c;
	constexpr u8 DMA_MAD = 0x1d;
	constexpr u8 DMA_LAD = 0x1e;
	constexpr u8 DMA_CST = 0x1f;

	constexpr u8 GSCFG0 = 0x0F;

	constexpr u8 M_NOROM = 1;
	constexpr u8 M_RAMRO = 2;
	constexpr u8 M_EXPAG = 8;

	u8* gsbankr[4] = { ROM_GS_M, GSRAM_M + 3 * PAGE, ROM_GS_M, ROM_GS_M + PAGE }; // bank pointers for read
	u8* gsbankw[4] = { TRASH_M, GSRAM_M + 3 * PAGE, TRASH_M, TRASH_M }; // bank pointers for write

	unsigned gs_v[4];
	u8 gsvol[4], gsbyte[4];
	unsigned led_gssum[4], led_gscnt[4];
	u8 gsdata_in, gsdata_out, gspage = 0;
	u8 gscmd, gsstat;

	u64 mult_gs, mult_gs2;

	// ngs
	u8 ngs_mode_pg1; // page ex number
	u8 ngs_cfg0;
	u8 ngs_s_ctrl;
	u8 ngs_s_stat;
	u8 SdRdVal, SdRdValNew;
	u8 ngs_dmamod;


	bool SdDataAvail = false;

	constexpr int GSINTFQ = 37500; // hz
	int GSCPUFQI;
	const unsigned GSCPUINT = GSCPUFQ / GSINTFQ;
	constexpr int MULT_GS_SHIFT = 12; // cpu tick -> gscpu tick precision
	void nmi();



	void apply_gs()
	{
		GSCPUFQI = GSCPUFQ / conf.intfq;
		mult_gs = (temp.snd_frame_ticks << MULT_C) / GSCPUFQI;
		mult_gs2 = (GSCPUFQI << MULT_GS_SHIFT) / conf.frame;
	}

	static void flush_gs_sound()
	{
		if (temp.sndblock)
			return;

		//!psb
		const unsigned l = gs_v[0] + gs_v[1];    //!psb
		const unsigned r = gs_v[2] + gs_v[3];    //!psb

		const unsigned lv = (l + r / 2) / 2;
		const unsigned rv = (r + l / 2) / 2;

		sound.update(unsigned((gs_t_states + gscpu.t) - gscpu_t_at_frame_start), lv, rv);     //!psb

		for (int ch = 0; ch < 4; ch++)
		{
			gsleds[ch].level = led_gssum[ch] * gsvol[ch] / (led_gscnt[ch] * (0x100 * 0x40 / 16) + 1);
			led_gssum[ch] = led_gscnt[ch] = 0;
			gsleds[ch].attrib = 0x0F;
		}
	}

	void init_gs_frame()
	{
		assert(gscpu.t < LONG_MAX);
		gscpu_t_at_frame_start = gs_t_states + gscpu.t;
		sound.start_frame();
	}

	void flush_gs_frame()
	{
		flush_gs_z80();
		sound.end_frame(unsigned((gs_t_states + gscpu.t) - gscpu_t_at_frame_start));
	}

	void out_gs(unsigned port, const u8 val)
	{
		port &= 0xFF;

		switch (port)
		{
		case 0x33: // GSCTR
			if (val & 0x80) // reset
			{
				reset();
				flush_gs_z80();
				return;
			}
			if (val & 0x40) // nmi
			{
				nmi();
				flush_gs_z80();
				return;
			}
			return;
			break;
		default:;
		}

		flush_gs_z80();
		switch (port)
		{
		case 0xB3: // GSDAT
			gsdata_out = val;
			gsstat |= 0x80;
			break;
		case 0xBB: // GSCOM
			gscmd = val;
			gsstat |= 0x01;
			break;
		default:;
		}
	}

	u8 in_gs(unsigned port)
	{
		flush_gs_z80();
		port &= 0xFF;
		switch (port)
		{
		case 0xB3: gsstat &= 0x7F; return gsdata_in;
		case 0xBB: return gsstat | 0x7E;
		}
		return 0xFF;
	}

	void gs_byte_to_dac(unsigned addr, u8 byte)
	{
		flush_gs_sound();
		const unsigned chan = (addr >> 8) & 3;
		gsbyte[chan] = byte;
		//   gs_v[chan] = (gsbyte[chan] * gs_vfx[gsvol[chan]]) >> 8;
		gs_v[chan] = ((char)(gsbyte[chan] - 0x80) * (signed)gs_vfx[gsvol[chan]]) / 256 + gs_vfx[33]; //!psb
		led_gssum[chan] += byte;
		led_gscnt[chan]++;
	}

	forceinline u8* am_r(u32 addr)
	{
		return &gsbankr[(addr >> 14U) & 3][addr & (PAGE - 1)];
	}

	void __cdecl BankNames(int i, char* Name)
	{
		if (gsbankr[i] < GSRAM_M + MAX_GSRAM_PAGES * PAGE)
			sprintf(Name, "RAM%2X", ULONG((gsbankr[i] - GSRAM_M) / PAGE));
		if ((gsbankr[i] - ROM_GS_M) < PAGE * MAX_GSROM_PAGES)
			sprintf(Name, "ROM%2X", ULONG((gsbankr[i] - ROM_GS_M) / PAGE));
	}


	forceinline u8 m1_cycle(Z80& cpu)
	{
		cpu.r_low++;
		CPUTACT(1);
		return cpu.rd(cpu.pc++);
	}

	static void UpdateMemMapping()
	{
		const bool ram_ro = (ngs_cfg0 & M_RAMRO) != 0;
		const bool no_rom = (ngs_cfg0 & M_NOROM) != 0;
		if (no_rom)
		{
			gsbankr[0] = gsbankw[0] = GSRAM_M;
			gsbankr[1] = gsbankw[1] = GSRAM_M + 3 * PAGE;
			gsbankr[2] = gsbankw[2] = GSRAM_M + gspage * PAGE;
			gsbankr[3] = gsbankw[3] = GSRAM_M + ngs_mode_pg1 * PAGE;

			if (ram_ro)
			{
				if (gspage == 0 || gspage == 1) // RAM0 or RAM1 in PG2
					gsbankw[2] = TRASH_M;
				if (ngs_mode_pg1 == 0 || ngs_mode_pg1 == 1) // RAM0 or RAM1 in PG3
					gsbankw[3] = TRASH_M;
			}
		}
		else
		{
			gsbankw[0] = gsbankw[2] = gsbankw[3] = TRASH_M;
			gsbankr[0] = ROM_GS_M;                                  // ROM0
			gsbankr[1] = gsbankw[1] = GSRAM_M + 3 * PAGE;           // RAM3
			gsbankr[2] = ROM_GS_M + (gspage & 0x1F) * PAGE;        // ROMn
			gsbankr[3] = ROM_GS_M + (ngs_mode_pg1 & 0x1F) * PAGE;  // ROMm
		}
	}

	void out(unsigned port, u8 val)
	{
		//   printf(__FUNCTION__" port=0x%X, val=0x%X\n", (port & 0xFF), val);
		switch (port & 0xFF)
		{
		case MPAG:
		{
			const bool ext_mem = (ngs_cfg0 & M_EXPAG) != 0;

			gspage = rol8(val, 1) & temp.gs_ram_mask & (ext_mem ? 0xFF : 0xFE);

			if (!ext_mem)
				ngs_mode_pg1 = (rol8(val, 1) & temp.gs_ram_mask) | 1;
			//         printf(__FUNCTION__"->GSPG, %X, Ro=%d, NoRom=%d, Ext=%d\n", gspage, RamRo, NoRom, ExtMem);
			UpdateMemMapping();
			return;
		}
		case 0x02: gsstat &= 0x7F; return;
		case 0x03: gsstat |= 0x80; gsdata_in = val; return;
		case 0x05: gsstat &= 0xFE; return;
		case 0x06: case 0x07: case 0x08: case 0x09:
		{
			flush_gs_sound();
			const unsigned chan = (port & 0x0F) - 6; val &= 0x3F;
			gsvol[chan] = val;
			//         gs_v[chan] = (gsbyte[chan] * gs_vfx[gsvol[chan]]) >> 8;
			gs_v[chan] = ((char)(gsbyte[chan] - 0x80) * (signed)gs_vfx[gsvol[chan]]) / 256 + gs_vfx[33]; //!psb
			return;
		}
		case 0x0A: gsstat = (gsstat & 0x7F) | (gspage << 7); return;
		case 0x0B: gsstat = (gsstat & 0xFE) | ((gsvol[0] >> 5) & 1); return;

		}

		//   printf(__FUNCTION__" port=0x%X, val=0x%X\n", (port & 0xFF), val);
		   // ngs
		switch (port & 0xFF)
		{
		case GSCFG0:
		{
			ngs_cfg0 = val & 0x3F;
			//          printf(__FUNCTION__"->GSCFG0, %X, Ro=%d, NoRom=%d, Ext=%d\n", ngs_cfg0, RamRo, NoRom, ExtMem);
			UpdateMemMapping();
		}
		break;

		case MPAGEX:
		{
			//          assert((ngs_cfg0 & M_EXPAG) != 0);
			ngs_mode_pg1 = rol8(val, 1) & temp.gs_ram_mask;
			UpdateMemMapping();
		}
		break;

		case S_CTRL:
			//          printf(__FUNCTION__"->S_CTRL\n");
			if (val & 0x80)
				ngs_s_ctrl |= (val & 0xF);
			else
				ngs_s_ctrl &= ~(val & 0xF);

			if (!(ngs_s_ctrl & _MPXRS))
				Vs1001.Reset();

			Vs1001.SetNcs((ngs_s_ctrl & _MPNCS) != false);
			break;

		case MC_SEND:
			Vs1001.WrCmd(val);
			break;

		case MD_SEND:
			Vs1001.Wr(val);
			break;

		case SD_SEND:
			SdCard.Wr(val);
			SdRdValNew = SdCard.Rd();
			SdDataAvail = true;
			break;

		case DMA_MOD:
			ngs_dmamod = val;
			break;

		case DMA_HAD:
			if (ngs_dmamod == 1)
				temp.gsdmaaddr = (temp.gsdmaaddr & 0x0000ffff) | (unsigned(val & 0x1F) << 16); // 5bit only
			break;

		case DMA_MAD:
			if (ngs_dmamod == 1)
				temp.gsdmaaddr = (temp.gsdmaaddr & 0x001f00ff) | (unsigned(val) << 8);
			break;

		case DMA_LAD:
			if (ngs_dmamod == 1)
				temp.gsdmaaddr = (temp.gsdmaaddr & 0x001fff00) | val;
			break;

		case DMA_CST:
			if (ngs_dmamod == 1)
				temp.gsdmaon = val;
			break;
		}
	}

	u8 in(unsigned port)
	{
		switch (port & 0xFF)
		{
		case 0x01: return gscmd;
		case 0x02: gsstat &= 0x7F; return gsdata_out;
		case 0x03: gsstat |= 0x80; gsdata_in = 0xFF; return 0xFF;
		case 0x04: return gsstat;
		case 0x05: gsstat &= 0xFE; return 0xFF;
		case 0x0A: gsstat = (gsstat & 0x7F) | (gspage << 7); return 0xFF;
		case 0x0B: gsstat = (gsstat & 0xFE) | (gsvol[0] >> 5); return 0xFF;


			// ngs
		case GSCFG0:
			return ngs_cfg0;
		case S_CTRL:
			return ngs_s_ctrl;

		case S_STAT:
			if (Vs1001.GetDreq())
				ngs_s_stat |= _MPDRQ;
			else
				ngs_s_stat &= ~_MPDRQ;
			return ngs_s_stat;

		case MC_READ:
			return Vs1001.Rd();

		case SD_READ:
		{
			u8 Tmp = SdRdVal;
			SdRdVal = SdRdValNew;
			return Tmp;
		}
		case SD_RSTR:
			if (SdDataAvail)
			{
				SdDataAvail = false;
				return SdRdValNew;
			}
			return SdCard.Rd();

		case DMA_MOD:
			return ngs_dmamod;

		case DMA_HAD:
			if (ngs_dmamod == 1)
				return (temp.gsdmaaddr >> 16) & 0x1F; // 5bit only
			break;

		case DMA_MAD:
			if (ngs_dmamod == 1)
				return (temp.gsdmaaddr >> 8) & 0xFF;
			break;

		case DMA_LAD:
			if (ngs_dmamod == 1)
				return temp.gsdmaaddr & 0xFF;
			break;

		case DMA_CST:
			if (ngs_dmamod == 1)
				return temp.gsdmaon;
			break;
		}
		return 0xFF;
	}

	inline void stepi()
	{
		u8 opcode = m1_cycle(gscpu);
		(::normal_opcode[opcode])(gscpu);
	}

	void fastcall step()
	{
		stepi();
	}

	__int64 __cdecl delta()
	{
		return gs_t_states + gscpu.t - gscpu.debug_last_t;
	}

	void __cdecl SetLastT()
	{
		gscpu.debug_last_t = gs_t_states + gscpu.t;
	}

	void nmi()
	{
		gscpu.sp -= 2;

		gsz80_wm<false>(gscpu.sp, gscpu.pcl);
		gsz80_wm<false>(gscpu.sp + 1, gscpu.pch);
		gscpu.pc = 0x66;
		gscpu.iff1 = gscpu.halted = 0;
	}

	void reset()
	{
		gscpu.reset();
		gsbankr[0] = ROM_GS_M; gsbankr[1] = GSRAM_M + 3 * PAGE; gsbankr[2] = ROM_GS_M; gsbankr[3] = ROM_GS_M + PAGE;
		gsbankw[0] = TRASH_M; gsbankw[1] = GSRAM_M + 3 * PAGE; gsbankw[2] = TRASH_M, gsbankw[3] = TRASH_M;

		gscpu.t = 0;
		gs_t_states = 0;
		gscpu_t_at_frame_start = 0;
		ngs_cfg0 = 0;
		ngs_s_stat = u8(rdtsc() & ~7) | _SDDET | _MPDRQ;
		ngs_s_ctrl = u8(rdtsc() & ~0xF) | _SDNCS;
		SdRdVal = SdRdValNew = 0xFF;
		SdDataAvail = false;
		Vs1001.Reset();

		ngs_mode_pg1 = 1;
		ngs_dmamod = 0;
		temp.gsdmaaddr = 0;
		temp.gsdmaon = 0;
	}
} // end of z80gs namespace