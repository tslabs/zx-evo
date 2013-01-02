
#pragma language=extended

#include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include <intrz80.h>
#include "defs.h"
#include "funcs.c"

void dma_dummy(u8 pdma)
{
	dma_set(0, pdma, 0, pdma, 255, 255); dma_mem()
}

C_task int main(void)
{
	u8 f;
	
	disable_interrupt();
	// sfile_null(0x4000);
	border(0); cls();
	color(C_HEAD); xy(8,1); msg("Select when to turn TS on:");
	color(C_NORM); xy(16,3); msg("1. On fill");
	color(C_NORM); xy(16,4); msg("2. On check");
	color(C_NORM); xy(16,5); msg("3. Never");
	color(C_NORM);
	xy(172,5); msg("0. Z80_LP: ");
	
	for(f=0;!f;)
	{
		xy(236,5);
		if (z80_lp) msg("ON "); else msg("OFF");
		f = key_disp();
	};
	
	color(C_ACHT); xy(0,7); msg("LoAddr");
	color(C_ACHT); xy(0,16); msg("PgNum");
    
	while(1)
	{
		u8 p;
		u16 tmp;
		u8 ts = 128 + (z80_lp ? 16 : 0);
		
		for (p=6; p>0; p++)
		{
			u8 pdma = (p < 160) ? 192 : 128;
				
// --- Increments test ---
			page3(p);
			if (mode==1) tscfg(ts);
			if(!dma_busy) dma_dummy(pdma);
			ram_fill_inc();
			tscfg(0);
			if (mode==2) tscfg(ts);
			if(!dma_busy) dma_dummy(pdma);
			color((tmp = ram_check_inc()) ? C_WARN : C_INFO);
			cx = (p&31)<<3; cy = ((p&0xE0)>>5) + 7; drawc('#');
			tscfg(0);
			if (tmp)
			{
				u8 *adr = (u8 *)tmp;
				xy(0,0); color(C_NORM);
				msg("Page:"); hex8(p);
				msg("  Addr:"); hex16(tmp);
				msg("  Written:"); hex8(tmp & 0xFF);
				msg("  Read:"); hex8(*adr);
			}

// --- PgNum test ---
			page3(p);
			if (mode==1) tscfg(ts);
			if(!dma_busy) dma_dummy(pdma);
			ram_fill_p(p);
			tscfg(0);
			if (mode==2) tscfg(ts);
			if(!dma_busy) dma_dummy(pdma);
			color((tmp = ram_check_p(p)) ? C_WARN : C_INFO);
			tscfg(0);
			cx = (p&31)<<3; cy = ((p&0xE0)>>5) + 16; drawc('#');
			if (tmp)
			{
				u8 *adr = (u8 *)tmp;
				xy(0,0); color(C_NORM);
				msg("Page:"); hex8(p);
				msg("  Addr:"); hex16(tmp);
				msg("  Written:"); hex8(p);
				msg("  Read:"); hex8(*adr);
			}
			
			ram_fill_p(0);
		}
	}
}
