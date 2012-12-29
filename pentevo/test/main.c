
#pragma language=extended

#include <stdio.h>
#include <string.h>
// #include <stdlib.h>
#include <intrz80.h>
#include "defs.h"
#include "funcs.c"
#include "msg.c"
#include "menu.c"

int main(void)
{
	disable_interrupt();
	
	while(1)
	{
		cls();
		color(C_HEAD); xy(16,1);
		msg("Memory test");
		color(C_NORM); xy(0,8); msg("Fill pgnum");
		color(C_NORM); xy(0,16); msg("Fill inc");

		// while(1);
//		msg_disp();
//		key_disp();
		
		while(1)
		{
			u8 p;
			for (p=8; p<255; p++)
			{
				border(p&7); 
				
				page3(p);
				ram_fill_p(p);
				dma_copy(p);
				page3(p+1);
				color(ram_check_p(p) ? C_WARN : C_INFO);
				cx = (p&31)<<3;
				cy = ((p&0xE0)>>5) + 8;
				drawc('#');
	
				page3(p);
				ram_fill_inc();
				dma_copy(p);
				page3(p+1);
				color(ram_check_inc() ? C_WARN : C_INFO);
				cx = (p&31)<<3;
				cy = ((p&0xE0)>>5) + 16;
				drawc('#');
			}
		}
	}
}
