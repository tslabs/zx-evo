#include "std.h"
#include "emul.h"
#include "vars.h"
#include "tsconf.h"
#include "memory.h"
#include "util.h"

// input: ports 7FFD,1FFD,DFFD,FFF7,FF77,EFF7, flags CF_TRDOS,CF_CACHEON
void set_banks()
{
	// set default values for memory windows
	bankw[1] = bankr[1] = page_ram(5);
	bankw[2] = bankr[2] = page_ram(2);
	bankm[0] = BANKM::BANKM_ROM;
	bankm[1] = bankm[2] = bankm[3] = BANKM::BANKM_RAM;

	// screen begining
	temp.base = memory + comp.ts.vpage * PAGE;
	temp.base_2 = temp.base;		// FIX THIS SHIT ANYWHERE YOU MEEEET IT!!!!

	// these flags will be re-calculated
	comp.flags &= ~(CF_DOSPORTS | CF_Z80FBUS | CF_LEAVEDOSRAM | CF_LEAVEDOSADR | CF_SETDOSROM);

	u8* bank0, * bank3;

	if (comp.flags & CF_TRDOS)
		bank0 = (comp.p7FFD & 0x10) ? base_dos_rom : base_sys_rom;
	else
		bank0 = (comp.p7FFD & 0x10) ? base_sos_rom : base_128_rom;

	unsigned bank = (comp.p7FFD & 7);

	switch (conf.mem_model)
	{
	case MM_PENTAGON:
		if (!(comp.pEFF7 & EFF7_LOCKMEM))
			bank |= (comp.p7FFD & 0x20) | (comp.p7FFD & 0xC0) >> 3; // 7FFD bits 657..210

		bank3 = page_ram(bank & temp.ram_mask);

		if (comp.pEFF7 & EFF7_ROCACHE)
			bank0 = page_ram(0); //Alone Coder 0.36.4
		break;

	case MM_TSL:
	{
		u8 tmp;

		if (comp.ts.w0_map_n)
			/* linear */
			tmp = comp.ts.page[0];

		else
			/* mapped */
		{
			if (comp.flags & CF_TRDOS)
				tmp = (comp.p7FFD & 0x10) ? 1 : 0;
			else
				tmp = (comp.p7FFD & 0x10) ? 3 : 2;

			tmp |= comp.ts.page[0] & 0xFC;
		}

		if (comp.ts.w0_ram || comp.ts.vdos)
			// RAM at #0000
		{
			bankm[0] = comp.ts.w0_we ? BANKM::BANKM_RAM : BANKM::BANKM_ROM;
			bank0 = page_ram(comp.ts.vdos ? 0xFF : tmp);
		}

		else
		{
			// ROM at #0000
			bankm[0] = BANKM::BANKM_ROM;
			bank0 = page_rom(tmp & 0x1F);
		}

		bankr[1] = bankw[1] = page_ram(comp.ts.page[1]);
		bankr[2] = bankw[2] = page_ram(comp.ts.page[2]);
		bank3 = page_ram(comp.ts.page[3]);
	}
	break;

	default: bank3 = page_ram(0);
	}

	bankw[0] = bankr[0] = bank0;
	bankw[3] = bankr[3] = bank3;

	if (bankr[0] >= ROM_BASE_M ||
		(conf.mem_model == MM_TSL && !comp.ts.w0_we && !comp.ts.vdos)) bankw[0] = TRASH_M;

	if (bankr[1] >= ROM_BASE_M) bankw[1] = TRASH_M;
	if (bankr[2] >= ROM_BASE_M) bankw[2] = TRASH_M;
	if (bankr[3] >= ROM_BASE_M) bankw[3] = TRASH_M;

	u8 dosflags = CF_LEAVEDOSRAM;
	if (conf.mem_model == MM_TSL && comp.ts.vdos)
		dosflags = 0;
	if (conf.mem_model == MM_PENTAGON)
		dosflags = CF_LEAVEDOSADR;

	if (comp.flags & CF_TRDOS)
	{
		comp.flags |= dosflags | CF_DOSPORTS;
	}
	else if ((comp.p7FFD & 0x10) && conf.trdos_present)
	{ // B-48, inactive DOS, DOS present
	   // for Scorp, ATM-1/2 and KAY, TR-DOS not started on executing RAM 3Dxx
		if (!((dosflags & CF_LEAVEDOSRAM) && bankr[0] < page_ram(MAX_RAM_PAGES)))
			comp.flags |= CF_SETDOSROM;
	}

	if (comp.flags & CF_CACHEON)
	{
		u8* cpage = CACHE_M;
		if (conf.cache == 32 && !(comp.p7FFD & 0x10)) cpage += PAGE;
		bankr[0] = bankw[0] = cpage;
		// if (comp.pEFF7 & EFF7_ROCACHE) bankw[0] = TRASH_M; //Alone Coder 0.36.4
	}

	if ((comp.flags & CF_DOSPORTS) ? conf.floatdos : conf.floatbus)
		comp.flags |= CF_Z80FBUS;

	if (temp.led.osw && (trace_rom | trace_ram))
	{
		for (unsigned i = 0; i < 4; i++) {
			unsigned bank = (bankr[i] - RAM_BASE_M) / PAGE;
			if (bank < MAX_PAGES) used_banks[bank] = 1;
		}
	}
}

void set_mode(rom_mode mode)
{
	if (mode == rom_mode::nochange)
		return;

	if (mode == rom_mode::cache)
	{
		comp.flags |= CF_CACHEON;
		set_banks();
		return;
	}

	// no RAM/cache/SERVICE
	comp.flags &= ~CF_CACHEON;

	switch (mode)
	{
	case rom_mode::s128:
		comp.flags &= ~CF_TRDOS;
		comp.p7FFD &= ~0x10;
		break;
	case rom_mode::sos:
		comp.flags &= ~CF_TRDOS;
		comp.p7FFD |= 0x10;
		break;
	case rom_mode::sys:
		comp.flags |= CF_TRDOS;
		comp.p7FFD &= ~0x10;
		break;
	case rom_mode::dos:
		comp.flags |= CF_TRDOS;
		comp.p7FFD |= 0x10;
		break;
	}
	set_banks();
}

u8 cmosBCD(u8 binary)
{
	if (!(cmos[11] & 4)) binary = (binary % 10) + 0x10 * ((binary / 10) % 10);
	return binary;
}

u8 cmos_read()
{
	static SYSTEMTIME st;
	static bool UF = false;
	static unsigned Seconds = 0;
	static u64 last_tsc = 0ULL;
	u8 reg = comp.cmos_addr;
	u8 rv;
	if (conf.cmos == 2)
		reg &= 0x3F;

	if ((1 << reg) & ((1 << 0) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 12)))
	{
		u64 tsc = rdtsc();
		// [vv] Часы читаются не чаще двух раз в секунду
		if ((tsc - last_tsc) >= 25 * temp.ticks_frame)
		{
			GetLocalTime(&st);
			if (st.wSecond != Seconds)
			{
				UF = true;
				Seconds = st.wSecond;
			}
		}
	}

	if (input.buffer.Enabled() && reg >= 0xF0)
	{
		return input.buffer.Pop();
	}

	switch (reg)
	{
	case 0:     return cmosBCD((BYTE)st.wSecond);
	case 2:     return cmosBCD((BYTE)st.wMinute);
	case 4:     return cmosBCD((BYTE)st.wHour);
	case 6:     return 1 + (((BYTE)st.wDayOfWeek + 8 - conf.cmos) % 7);
	case 7:     return cmosBCD((BYTE)st.wDay);
	case 8:     return cmosBCD((BYTE)st.wMonth);
	case 9:     return cmosBCD(st.wYear % 100);
	case 10:    return 0x20 | (cmos[10] & 0xF); // molodcov_alex
	case 11:    return (cmos[11] & 4) | 2;
	case 12:  // [vv] UF
		rv = UF ? 0x10 : 0;
		UF = false;
		return rv;
	case 13:    return 0x80;
	}

	return cmos[reg];
}

void cmos_write(u8 val)
{
	if (conf.cmos == 2) comp.cmos_addr &= 0x3F;

	if (conf.mem_model == MM_TSL && comp.cmos_addr == 0x0C)
	{
		BYTE keys[256];
		if (GetKeyboardState(keys) && !keys[VK_CAPITAL] != !(val & 0x02))
		{
			keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY, 0);
			keybd_event(VK_CAPITAL, 0x45, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
		}
		if (val & 0x01)
			input.buffer.Empty();
	}

	cmos[comp.cmos_addr] = val;
}

void NVRAM::write(u8 val)
{
	const int SCL = 0x40, SDA = 0x10, WP = 0x20,
		SDA_1 = 0xFF, SDA_0 = 0xBF,
		SDA_SHIFT_IN = 4;

	if ((val ^ prev) & SCL) // clock edge, data in/out
	{
		if (val & SCL) // nvram reads SDA
		{
			if (state == RD_ACK)
			{
				if (val & SDA) goto idle; // no ACK, stop
				// move next byte to host
				state = SEND_DATA;
				dataout = nvram[address];
				address = (address + 1) & 0x7FF;
				bitsout = 0; goto terminate; // out_z==1;
			}

			if ((1 << state) & ((1 << RCV_ADDR) | (1 << RCV_CMD) | (1 << RCV_DATA))) {
				if (out_z) // skip nvram ACK before reading
					datain = 2 * datain + ((val >> SDA_SHIFT_IN) & 1), bitsin++;
			}

		}
		else { // nvram sets SDA

			if (bitsin == 8) // byte received
			{
				bitsin = 0;
				if (state == RCV_CMD) {
					if ((datain & 0xF0) != 0xA0) goto idle;
					address = (address & 0xFF) + ((datain << 7) & 0x700);
					if (datain & 1) { // read from current address
						dataout = nvram[address];
						address = (address + 1) & 0x7FF;
						bitsout = 0;
						state = SEND_DATA;
					}
					else
						state = RCV_ADDR;
				}
				else if (state == RCV_ADDR) {
					address = (address & 0x700) + datain;
					state = RCV_DATA; bitsin = 0;
				}
				else if (state == RCV_DATA) {
					nvram[address] = datain;
					address = (address & 0x7F0) + ((address + 1) & 0x0F);
					// state unchanged
				}

				// EEPROM always acknowledges
				out = SDA_0; out_z = 0; goto terminate;
			}

			if (state == SEND_DATA) {
				if (bitsout == 8) { state = RD_ACK; out_z = 1; goto terminate; }
				out = (dataout & 0x80) ? SDA_1 : SDA_0; dataout *= 2;
				bitsout++; out_z = 0; goto terminate;
			}

			out_z = 1; // no ACK, reading
		}
		goto terminate;
	}

	if ((val & SCL) && ((val ^ prev) & SDA)) // start/stop
	{
		if (val & SDA) { idle: state = IDLE; } // stop
		else state = RCV_CMD, bitsin = 0; // start
		out_z = 1;
	}

	// else SDA changed on low SCL


terminate:
	if (out_z) out = (val & SDA) ? SDA_1 : SDA_0;
	prev = val;
}

void rand_ram()
{
	u8* mem = page_ram(0);
	for (u32 i = 0; i < (4096 * 1024); i++)
		mem[i] = rand();
}
