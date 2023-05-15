// Z-Controller by KOE
// Only SD-card
#include "std.h"
#include "zc.h"
#include "emul.h"
#include "ft812.h"
#include "sdcard.h"
#include "vars.h"

void zc_t::reset()
{
	sd_card_.Reset();
	cfg_ = 3;
	status_ = 0;
	rd_buff_ = 0xff;
}

void zc_t::wr(const u32 port, u8 val)
{
	switch (port & 0xFF)
	{
	case 0x77: // config
		val &= 7;

		if ((comp.ts.vdac2) && ((cfg_ & 4) != (val & 4)))
			vdac2::set_ss((val & 4) != 0);

		cfg_ = val;
		break;

	case 0x57: // data
		if (!(cfg_ & 2))   // SD card selected
		{
			rd_buff_ = sd_card_.Rd();
			sd_card_.Wr(val);
		}
		else if ((cfg_ & 4) && (comp.ts.vdac2))   // FT812 selected
			rd_buff_ = vdac2::transfer(val);
		else
			rd_buff_ = 255;
		break;
	default:;
	}
}

u8 zc_t::rd(const u32 port)
{
	const u8 tmp = rd_buff_;

	switch (port & 0xFF)
	{
	case 0x77: // status
		return status_;      // always returns 0

	case 0x57: // data

		if (!(cfg_ & 2))   // SD card selected
		{
			rd_buff_ = sd_card_.Rd();
			sd_card_.Wr(0xff);
		}
		else if ((cfg_ & 4) && (comp.ts.vdac2))
			rd_buff_ = vdac2::transfer(0xFF);
		else
			rd_buff_ = 255;

		return tmp;
	default: ;
	}

	return 0xFF;
}

zc_t zc;
