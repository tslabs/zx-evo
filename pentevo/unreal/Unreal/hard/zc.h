#pragma once
#include "sysdefs.h"
#include "sdcard.h"

// Z-Controller by KOE
// Only SD-card

class zc_t
{
	TSdCard sd_card_;
	u8 cfg_ = 0;
	u8 status_ = 0;
	u8 rd_buff_ = 0;
public:
	void reset();
	void open(const char* name) { sd_card_.Open(name); }
	void close() { sd_card_.Close(); }
	void wr(u32 port, u8 val);
	u8 rd(u32 port);
};

extern zc_t zc;
