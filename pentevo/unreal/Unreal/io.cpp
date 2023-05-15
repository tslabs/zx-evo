#include "std.h"
#include "emul.h"
#include "funcs.h"
#include "vars.h"
#include "draw.h"
#include "hard/memory.h"
#include "sound/sound.h"
#include "hard/gs/gs.h"
#include "sound/saa1099.h"
#include "hard/zc.h"
#include "hard/cpu/z80main.h"
#include "tape.h"
#include "sound/ayx32.h"
#include "util.h"
#include "sound/dev_moonsound.h"

#ifdef LOG_FE_OUT
extern FILE* f_log_FE_in;
#endif
#ifdef LOG_FE_IN
extern FILE* f_log_FE_out;
#endif

u8 ay_last_reg; // temp!

bool assert_dma_active()
{
	bool rc = true;

	if (comp.ts.dma.state != DMA_ST_NOP)
	{
		color(CONSCLR_WARNING);
		printf("Cannot write to DMA when it is active! PC = %02X:%04X\r\n", comp.ts.page[cpu.pc >> 14], cpu.pc);
		color(CONSCLR_DEFAULT);
		rc = false;
	}

	return rc;
}

void out(unsigned port, u8 val)
{

	port &= 0xFFFF;
	u8 p1 = (port & 0xFF);          // lower 8 bits of port address
	u8 p2 = ((port >> 8) & 0xFF);   // higher 8 bits of port address
	brk_port_out = port;
	brk_port_val = val;

	// if (p1 == 0xFD)
	// printf("out (%04X), %02X\n", port, val);

	if (conf.ulaplus != ulaplus::none)
	{
		/* ULA+ register select */
		if (port == 0xBF3B)
		{
			comp.ulaplus_reg = val;
		}

		/* ULA+ data */
		if (port == 0xFF3B)
		{
			switch (comp.ulaplus_reg & 0xC0)
			{
			case 0:  // CRAM
				comp.ulaplus_cram[comp.ulaplus_reg] = val;
				break;

			case 64:  // MODE
				comp.ulaplus_mode = val & 1;
				break;

			default:
				break;
			}
		}
	}

#ifdef MOD_GS
	// 10111011 | BB
	// 10110011 | B3
	// 00110011 | 33
	if (p1 == 0x33 && conf.gs_type) // 33
	{
		out_gs(port, val);
		return;
	}

	if ((port & 0xF7) == 0xB3 && conf.gs_type) // BB, B3
	{
		out_gs(port, val);
		return;
	}
#endif

	// ZXM-MoonSound
	if (conf.sound.moonsound && !(comp.flags & CF_DOSPORTS) && (((p1 & 0xFC) == 0xC4) || (p1 & 0xFE) == 0x7E))
	{
		if (zxmmoonsound.write(port, val))
			return;
	}

	// z-controller
	if (conf.zc && ((p1 == 0x57) || (p1 == 0x77)))
	{
		zc.wr(port, val);
		return;
	}

	if ((conf.mem_model == MM_TSL) && (p1 == 0xAF))
		ts_ext_port_wr(p2, val);

	if (comp.flags & CF_DOSPORTS)
	{

		if ((p1 & 0x1F) == 0x1F) // 1F, 3F, 5F, 7F, FF
		{
			if (conf.mem_model == MM_TSL)
			{
				if (comp.ts.vdos)
				{ // vdos_on
					if (p1 == 0xFF)
					{ // out vgsys
						comp.wd.out(p1, val);
						return;
					}

					// out vg
					comp.ts.vdos = 0;
					set_banks();
					return;
				}

				if ((1 << comp.wd.drive) & comp.ts.fddvirt)
				{ // vdos_off
					comp.ts.vdos = 1;
					set_banks();
					return;
				}
			}
			// physical drive
			comp.wd.out(p1, val);
			return;
		}
		// don't return - out to port #FE works in trdos!
	}
	else // не dos
	{
		// VG93 free access in TS-Conf (FDDVirt.7 = 1)
		if ((conf.mem_model == MM_TSL) && (comp.ts.fddvirt & 0x80) && ((p1 & 0x1F) == 0x1F)) // 1F, 3F, 5F, 7F, FF
			comp.wd.out(p1, val);

		if ((u8)port == 0x1F && conf.sound.ay_scheme == ay_scheme::pos)
		{
			comp.active_ay = val & 1;
			return;
		}

		if (!(port & 6) && (conf.ide_scheme == ide_scheme::nemo || conf.ide_scheme == ide_scheme::nemo_a8))
		{
			const unsigned hi_byte = (conf.ide_scheme == ide_scheme::nemo) ? (port & 1) : (port & 0x100);
			if (hi_byte)
			{
				comp.ide_write = val;
				return;
			}
			if ((port & 0x18) == 0x08)
			{
				if ((port & 0xE0) == 0xC0)
					hdd.write(8, val);
				return;
			} // CS1=0,CS0=1,reg=6
			if ((port & 0x18) != 0x10)
				return; // invalid CS0,CS1

		write_hdd_5:
			port >>= 5;
		write_hdd:
			port &= 7;
			if (port)
				hdd.write(port, val);
			else
				hdd.write_data(val | (comp.ide_write << 8));
			return;
		}
	}


	// port #FE
	bool pFE;

	// others xxxxxxx0
	pFE = !(port & 1);

	if (pFE)
	{
		//[vv]      assert(!(val & 0x08));

#ifdef LOG_FE_OUT
		fprintf(f_log_FE_out, "%d\t%02X\r\n", (u32)(comp.t_states + cpu.t), val);
#endif

		spkr_dig = (val & 0x10) ? conf.sound.beeper_vol : 0;
		mic_dig = (val & 0x08) ? conf.sound.micout_vol : 0;

		// speaker & mic
		if ((comp.pFE ^ val) & 0x18)
		{
			//          __debugbreak();
			flush_dig_snd();
		}

		u8 new_border = (val & 7);

		new_border |= 0xF0;
		if (comp.ts.border != new_border)
			update_screen();

		comp.ts.border = new_border;

		comp.pFE = val;  // looks obsolete
		// do not return! intro to navy seals (by rst7) uses out #FC for to both FE and 7FFD
	}

	// #xD
	if (!(port & 2))
	{
		// port DD - covox
		if (conf.sound.covoxDD && (u8)port == 0xDD)
		{
			//         __debugbreak();
			flush_dig_snd();
			covDD_vol = val * conf.sound.covoxDD_vol / 0x100;
			return;
		}

		// zx128 port and related - !!! rewrite it using switch
		if (!(port & 0x8000))
		{
			// 7FFD blocker, if 48 lock bit is set
			if (comp.p7FFD & 0x20)
			{
				// #EFF7.2 = 0 disables lock
				if ((conf.mem_model == MM_PENTAGON && conf.ramsize == 1024) && !(comp.pEFF7 & EFF7_LOCKMEM))
					goto set_7ffd;
			}

			// In TS Memory Model the actual value of #7FFD ignored, and page3 is used instead
			if (conf.mem_model == MM_TSL)
			{
				const u8 lock128_auto = !(!(cpu.opcode & 0x80) ^ !(cpu.opcode & 0x40));    // out(c), R = no lock or out(#FD), a = lock128
				const u8 page128 = val & 0x07;
				const u8 page512 = ((val & 0xC0) >> 3) | (val & 0x07);
				const u8 page1024 = (val & 0x20) | ((val & 0xC0) >> 3) | (val & 0x07);

				switch (comp.ts.lck128)
				{
					// 512kB
				case 0:
					comp.ts.page[3] = page512;
					break;

					// 128kB
				case 1:
					comp.ts.page[3] = page128;
					break;

					// 512/128kB auto
				case 2:
					comp.ts.page[3] = lock128_auto ? page128 : page512;
					break;

					// 1024kB
				case 3:
					comp.ts.page[3] = page1024;
					val &= ~0x20;
					break;
				}
			}

		set_7ffd:
			comp.p7FFD = val;      // all models apart from TSL will deal with this variable
			comp.ts.vpage = comp.ts.vpage_d = (val & 8) ? 7 : 5;

			set_banks();
			return;
		}

		// FFFD - PSG address
		if ((port & 0xC0FF) == 0xC0FD)
		{
			// printf("Reg: %d\n", val);

			// switch active PSG via NedoPC scheme
			switch (conf.sound.ay_scheme)
			{
			case ay_scheme::chrv:
				if ((val & 0xF8) == 0xF8)
				{
					if (conf.sound.ay_chip == (SNDCHIP::CHIP_YM2203))
					{
						fmsoundon0 = val & 4;
						tfmstatuson0 = val & 2;
					}
					comp.active_ay = val & 1;
				};
				break;

			case ay_scheme::ayx32:
				if ((val & 0xF8) == 0xF8)
					comp.active_ay = ~val & 1;
				break;

			case ay_scheme::quadro:
				comp.active_ay = (port >> 12) & 1;
				break;
			}

			ay_last_reg = val;
			if ((conf.sound.ay_scheme == ay_scheme::ayx32) && (val >= 16))
				ayx32.write_addr((SNDAYX32::REG)val);
			else
				ay[comp.active_ay].select(val);
			return;
		}

		// BFFD - PSG data
		if (((port & 0xC000) == 0x8000) && conf.sound.ay_scheme != ay_scheme::none)
		{
			// printf("Write: %d\n", val);

			u8 n_ay;
			switch (conf.sound.ay_scheme)
			{
			case ay_scheme::quadro:
				n_ay = (port >> 12) & 1;
				break;

			default:
				n_ay = comp.active_ay;
			}

			if ((conf.sound.ay_scheme == ay_scheme::ayx32) && (ay_last_reg >= 16))
				ayx32.write(temp.sndblock ? 0 : cpu.t, val);
			else
				ay[n_ay].write(temp.sndblock ? 0 : cpu.t, val);

			if (conf.input.mouse == 2 && ay[n_ay].get_activereg() == 14)
				input.aymouse_wr(val);
			return;
		}
		return;
	}

	if (conf.sound.sd && (port & 0xAF) == 0x0F)
	{ // soundrive
 //      __debugbreak();
		if ((u8)port == 0x0F) comp.p0F = val;
		if ((u8)port == 0x1F) comp.p1F = val;
		if ((u8)port == 0x4F) comp.p4F = val;
		if ((u8)port == 0x5F) comp.p5F = val;
		flush_dig_snd();
		sd_l = (conf.sound.sd_vol * (comp.p0F + comp.p1F)) >> 8;
		sd_r = (conf.sound.sd_vol * (comp.p4F + comp.p5F)) >> 8;
		return;
	}
	if (conf.sound.covoxFB && !(port & 4))
	{ // port FB - covox
 //      __debugbreak();
		flush_dig_snd();
		covFB_vol = val * conf.sound.covoxFB_vol / 0x100;
		return;
	}

	if (conf.sound.saa1099 && (p1 == 0xFF)) // saa1099
	{
		if (port & 0x100)
			Saa1099.WrCtl(val);
		else
			Saa1099.WrData(temp.sndblock ? 0 : cpu.t, val);
		return;
	}

	if ((port == 0xEFF7) && (conf.mem_model == MM_PENTAGON || conf.mem_model == MM_TSL))
	{
		const u8 oldp_eff7 = comp.pEFF7;
		comp.pEFF7 = (comp.pEFF7 & conf.EFF7_mask) | (val & ~conf.EFF7_mask);
		// comp.pEFF7 |= EFF7_GIGASCREEN; // [vv] disable turbo

		if (conf.mem_model == MM_PENTAGON)
			TURBO((comp.pEFF7 & EFF7_GIGASCREEN) ? 1 : 2);

		init_raster();

		if ((comp.pEFF7 ^ oldp_eff7) & (EFF7_ROCACHE | EFF7_LOCKMEM))
			set_banks();
		return;
	}
	if (conf.cmos && (((comp.pEFF7 & EFF7_CMOS) && conf.mem_model == MM_PENTAGON) ||            // check bit 7 port EFF7 for Pentagon
		(conf.mem_model == MM_TSL && ((comp.pEFF7 & EFF7_CMOS) || (comp.flags & CF_DOSPORTS)))  // check bit 7 port EFF7 or active DOS for TSConf
		))
	{
		unsigned mask = 0xFFFF;

		if (port == (0xDFF7 & mask))
		{
			comp.cmos_addr = val;
			return;
		}
		if (port == (0xBFF7 & mask))
		{
			if (conf.mem_model == MM_TSL && comp.cmos_addr >= 0xF0 && val <= 2)
			{
				if (val < 2)
				{
					input.buffer.Enable(false);

					static unsigned version = 0;
					if (!version)
					{
						unsigned day, year;
						char month[8];
						static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
						sscanf(__DATE__, "%s %d %d", month, &day, &year);
						version = day | ((strstr(months, month) - months) / 3 + 1) << 5 | (year - 2000) << 9;
					}

					strcpy((char*)cmos + 0xF0, "UnrealSpeccy");
					*(unsigned*)(cmos + 0xFC) = version;
				}
				else input.buffer.Enable(true);
			}
			else cmos_write(val);
			return;
		}
	}
	if ((p1 == 0xEF) && (zf232.open_port))
		zf232.write(p2, val);
}

__inline u8 in1(unsigned port)
{
	port &= 0xFFFF;
	brk_port_in = port;

	u8 p1 = (port & 0xFF);
	u8 p2 = ((port >> 8) & 0xFF);

	u8 tmp;

	if (conf.ulaplus != ulaplus::none)
	{
		if (port == 0xFF3B)
		{
			// ULA+ DATA
			const u8 c = (!(comp.ulaplus_reg & 0xC0) && (comp.ulaplus_mode & 1)) ? comp.ulaplus_cram[comp.ulaplus_reg] : 0xFF;
			return c;
		}
	}

	// В начале дешифрация портов по полным 8бит
	// ngs
#ifdef MOD_GS
	if ((port & 0xF7) == 0xB3 && conf.gs_type)
		return in_gs(port);
#endif

	// ZXM-MoonSound
	if (conf.sound.moonsound && !(comp.flags & CF_DOSPORTS) && (((p1 & 0xFC) == 0xC4) || ((p1 & 0xFE) == 0x7E)))
	{
		u8 val = 0xFF;

		if (zxmmoonsound.read(port, val))
			return val;
	}

	// z-controller
	if (conf.zc && (p1 == 0x57) || (p1 == 0x77))
	{
		// no shadow-mode ZXEVO patch here since 0x57 port in read mode is the same
		// as in noshadow-mode, i.e. no A15 is used to decode port.
		return zc.rd(port);
	}

	if ((conf.mem_model == MM_TSL) && (p1 == 0xAF))
	{
		// TS-Config extensions ports
		switch (p2) {

		case TSR_STATUS:
		{
			tmp = comp.ts.pwr_up | (comp.ts.vdac2 ? TS_VDAC2 : comp.ts.vdac);
			comp.ts.pwr_up = TS_PWRUP_OFF;
			return tmp;
		}

		case TSR_PAGE2:
			return comp.ts.page[2];

		case TSR_PAGE3:
			return comp.ts.page[3];

		case TSR_DMASTATUS:
			return (comp.ts.dma.state == DMA_ST_NOP) ? 0x00 : 0x80;
		}
	}

	if (comp.flags & CF_DOSPORTS)
	{
		if ((port & 0x18A3) == (0xFFFE & 0x18A3))
		{ // SMUC
			if (conf.smuc)
			{
				if ((port & 0xA044) == (0xDFBA & 0xA044)) return cmos_read(); // clock
				if ((port & 0xA044) == (0xFFBA & 0xA044)) return comp.nvram.out; // SMUC system port
				if ((port & 0xA044) == (0x7FBA & 0xA044)) return comp.p7FBA | 0x3F;
				if ((port & 0xA044) == (0x5FBA & 0xA044)) return 0x3F;
				if ((port & 0xA044) == (0x5FBE & 0xA044)) return 0x57;
				if ((port & 0xA044) == (0x7FBE & 0xA044)) return 0x57;
			}
		}

		u8 p1 = (u8)port;
		// 1F = 0001|1111b
		// 3F = 0011|1111b
		// 5F = 0101|1111b
		// 7F = 0111|1111b
		// DF = 1101|1111b порт мыши
		// FF = 1111|1111b
		if ((p1 & 0x9F) == 0x1F || p1 == 0xFF) // 1F, 3F, 5F, 7F, FF
		{
			if (conf.mem_model == MM_TSL)
			{
				if (comp.ts.vdos)
				{ // vdos_on
					comp.ts.vdos = 0;
					set_banks();
					return 0xFF;
				}

				if ((1 << comp.wd.drive) & comp.ts.fddvirt)
				{ // vdos_off
					comp.ts.vdos_m1 = 1;
					return 0xFF;
				}
			}
			return comp.wd.in(p1); // physical drive
		}
	}
	else // не dos
	{
		// VG93 free access in TS-Conf (FDDVirt.7 = 1)
		if ((conf.mem_model == MM_TSL) && (comp.ts.fddvirt & 0x80) && ((p1 & 0x1F) == 0x1F)) // 1F, 3F, 5F, 7F, FF
			return comp.wd.in(p1);

		if (!(port & 6) && (conf.ide_scheme == ide_scheme::nemo || conf.ide_scheme == ide_scheme::nemo_a8))
		{
			const unsigned hi_byte = (conf.ide_scheme == ide_scheme::nemo) ? (port & 1) : (port & 0x100);
			if (hi_byte)
				return comp.ide_read;
			comp.ide_read = 0xFF;
			if ((port & 0x18) == 0x08)
				return ((port & 0xE0) == 0xC0) ? hdd.read(8) : 0xFF; // CS1=0,CS0=1,reg=6
			if ((port & 0x18) != 0x10)
				return 0xFF; // invalid CS0,CS1

			port >>= 5;
			port &= 7;
			if (port)
				return hdd.read(port);
			const unsigned v = hdd.read_data();
			comp.ide_read = (u8)(v >> 8);
			return (u8)v;
		}
	}


	if (!(port & 0x20))
	{ // kempstons
		port = (port & 0xFFFF) | 0xFA00; // A13,A15 not used in decoding
		if ((port == 0xFADF || port == 0xFBDF || port == 0xFFDF) && conf.input.mouse == 1)
		{ // mouse
			input.mouse_joy_led |= 1;
			if (port == 0xFBDF)
				return input.kempston_mx();
			if (port == 0xFFDF)
				return input.kempston_my();
			return input.mbuttons;
		}
		input.mouse_joy_led |= 2;
		u8 res = (conf.input.kjoy) ? input.kjoy : 0xFF;
		return res;
	}

	// port #FE
	bool pFE;
	// others xxxxxxx0
	pFE = !(port & 1);

	if (pFE)
	{
		if ((cpu.pc & 0xFFFF) == 0x0564 && bankr[0][0x0564] == 0x1F && conf.tape_autostart && !comp.tape.play_pointer)
			start_tape();

		u8 val = input.read(port >> 8);
#ifdef LOG_FE_IN
		fprintf(f_log_FE_in, "%d\t%02X\t%02X\r\n", (u32)(comp.t_states + cpu.t), val, cpu.a);
#endif
		return val;
	}

	// xxFD - AY
	if ((p1 == 0xFD) && conf.sound.ay_scheme != ay_scheme::none)
	{
		if ((conf.sound.ay_scheme == ay_scheme::chrv) && (conf.sound.ay_chip == (SNDCHIP::CHIP_YM2203)) && (tfmstatuson0 == 0))
			return 0x7F;  // always ready

		if ((p2 & 0xC0) != 0xC0)
			return 0xFF;

		u8 n_ay;
		if (conf.sound.ay_scheme == ay_scheme::quadro)
			n_ay = (port >> 12) & 1;
		else
			n_ay = comp.active_ay;

		// read selected AY register
		if (conf.input.mouse == 2 && (ay[n_ay].get_activereg() == 14))
		{
			input.mouse_joy_led |= 1;
			return input.aymouse_rd();
		}

		u8 rc;
		if ((conf.sound.ay_scheme == ay_scheme::ayx32) && (ay_last_reg >= 16))
			rc = ayx32.read();
		else
			rc = ay[n_ay].read();

		// printf("Read: %d\n", rc);
		return rc;
	}

	//   if ((port & 0x7F) == 0x7B) { // FB/7B
	if ((port & 0x04) == 0x00)
	{ // FB/7B (for MODPLAYi)
		if (conf.cache)
		{
			comp.flags &= ~CF_CACHEON;
			if (port & 0x80) comp.flags |= CF_CACHEON;
			set_banks();
		}
		return 0xFF;
	}

	if (conf.cmos && ((comp.pEFF7 & EFF7_CMOS) || // check 7 bit port EFF7
		(((comp.pEFF7 & EFF7_CMOS) || (comp.flags & CF_DOSPORTS)) && conf.mem_model == MM_TSL))) // check bit 7 port EFF7 or active DOS for TSConf
	{
		constexpr unsigned mask = 0xFFFF;

		if (port == (0xBFF7 & mask))
			return cmos_read();
	}

	if ((p1 == 0xEF) && (zf232.open_port))
		return zf232.read(p2);

	if (conf.portff && (p1 == 0xFF))
	{
		if (vmode != 2) return 0xFF; // ray is not in paper
		const unsigned ula_t = (cpu.t + temp.border_add) & temp.border_and;
		return temp.base[vcurr->atr_offs + (ula_t - vcurr[-1].next_t) / 4];
	}
	return 0xFF;
}

u8 in(const unsigned port)
{
	brk_port_val = in1(port);
	return brk_port_val;
}

// TS-Config extensions ports
void ts_ext_port_wr(u8 port, u8 val)
{
	switch (port)
	{
		// system
	case TSW_SYSCONF:
		comp.ts.sysconf = val;
		comp.ts.cacheconf = comp.ts.cache ? 0x0F : 0x00;
		set_clk();
		break;

	case TSW_CACHECONF:
		comp.ts.cacheconf = val & 0x0F;
		break;

	case TSW_FDDVIRT:
		comp.ts.fddvirt = val & 0x8F;
		break;

	case TSW_INTMASK:
		comp.ts.intmask = val & 0x07;
		comp.ts.intctrl.pend &= val & 0x07;
		break;

	case TSW_HSINT:
		comp.ts.hsint = val;
		comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line - 1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
		break;

	case TSW_VSINTL:
		comp.ts.vsintl = val;
		comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line - 1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
		break;

	case TSW_VSINTH:
		comp.ts.vsinth = val;
		comp.ts.intctrl.frame_t = ((comp.ts.hsint > (conf.t_line - 1)) || (comp.ts.vsint > 319)) ? -1 : (comp.ts.vsint * conf.t_line + comp.ts.hsint);
		break;

		// memory
	case TSW_MEMCONF:
		comp.ts.memconf = val;
		comp.p7FFD &= ~0x10;
		comp.p7FFD |= (val & 1) << 4;
		set_banks();
		break;

	case TSW_PAGE0:
		comp.ts.page[0] = val;
		set_banks();
		break;

	case TSW_PAGE1:
		comp.ts.page[1] = val;
		set_banks();
		break;

	case TSW_PAGE2:
		comp.ts.page[2] = val;
		set_banks();
		break;

	case TSW_PAGE3:
		comp.ts.page[3] = val;
		set_banks();
		break;

	case TSW_FMADDR:
		comp.ts.fmaddr = val;
		break;

		// video
	case TSW_VCONF:
		comp.ts.vconf_d = val;
		break;

	case TSW_VPAGE:
		comp.ts.vpage_d = val;
		set_banks();
		break;

	case TSW_TMPAGE:
		comp.ts.tmpage = val;
		break;

	case TSW_T0GPAGE:
		comp.ts.t0gpage[0] = val;
		break;

	case TSW_T1GPAGE:
		comp.ts.t1gpage[0] = val;
		break;

	case TSW_SGPAGE:
		comp.ts.sgpage = val;
		break;

	case TSW_BORDER:
		comp.ts.border = val;
		break;

	case TSW_TSCONF:
		comp.ts.tsconf = val;
		break;

	case TSW_PALSEL:
		comp.ts.palsel_d = val;
		break;

	case TSW_GXOFFSL:
		comp.ts.g_xoffsl_d = val;
		break;

	case TSW_GXOFFSH:
		comp.ts.g_xoffsh_d = val & 1;
		break;

	case TSW_GYOFFSL:
		comp.ts.g_yoffsl = val;
		comp.ts.g_yoffs_updated = 1;
		break;

	case TSW_GYOFFSH:
		comp.ts.g_yoffsh = val & 1;
		comp.ts.g_yoffs_updated = 1;
		break;

	case TSW_T0XOFFSL:
		comp.ts.t0_xoffsl = val;
		break;

	case TSW_T0XOFFSH:
		comp.ts.t0_xoffsh = val & 1;
		break;

	case TSW_T0YOFFSL:
		comp.ts.t0_yoffsl = val;
		break;

	case TSW_T0YOFFSH:
		comp.ts.t0_yoffsh = val & 1;
		break;

	case TSW_T1XOFFSL:
		comp.ts.t1_xoffsl = val;
		break;

	case TSW_T1XOFFSH:
		comp.ts.t1_xoffsh = val & 1;
		break;

	case TSW_T1YOFFSL:
		comp.ts.t1_yoffsl = val;
		break;

	case TSW_T1YOFFSH:
		comp.ts.t1_yoffsh = val & 1;
		break;

		// dma
	case TSW_DMASAL:
		if (assert_dma_active())
			comp.ts.saddrl = val & 0xFE;
		break;

	case TSW_DMASAH:
		if (assert_dma_active())
			comp.ts.saddrh = val & 0x3F;
		break;

	case TSW_DMASAX:
		if (assert_dma_active())
			comp.ts.saddrx = val;
		break;

	case TSW_DMADAL:
		if (assert_dma_active())
			comp.ts.daddrl = val & 0xFE;
		break;

	case TSW_DMADAH:
		if (assert_dma_active())
			comp.ts.daddrh = val & 0x3F;
		break;

	case TSW_DMADAX:
		if (assert_dma_active())
			comp.ts.daddrx = val;
		break;

	case TSW_DMALEN:
		if (assert_dma_active())
			comp.ts.dmalen = val;
		break;

	case TSW_DMANUM:
		if (assert_dma_active())
			comp.ts.dmanum = val;
		break;

	case TSW_DMACTR:
		if (assert_dma_active())
		{
			comp.ts.dma.ctrl = val;
			comp.ts.dma.state = DMA_ST_INIT;
		}
		break;

	default:
		color(CONSCLR_WARNING); printf("Illegal port! OUT (%02XAF), %02X, PC = %02X:%04X\r\n", port, val, comp.ts.page[cpu.pc >> 14], cpu.pc);
		break;
	}
}

#undef in_trdos
#undef out_trdos
