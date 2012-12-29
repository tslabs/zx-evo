
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

//		msg_disp();
//		key_disp();
		
		while(1)
		{
			u8 p;
			for (p=8; p<255; p++)
			{
				border(p&7); 
				
				color(C_NORM); xy(16,3);
				msg("Fill 0:   ");
				page3(p);
				ram_fill_0();
				dma_copy(p);
				page3(p+1);
				if (ram_check_0())
					{color(C_WARN); msg(" Error");}
				else
					{color(C_INFO); msg(" OK   ");}
	
				color(C_NORM); xy(16,4);
				msg("Fill inc: ");
				page3(p);
				ram_fill_inc();
				dma_copy(p);
				page3(p+1);
				if (ram_check_inc())
					{color(C_WARN); msg(" Error");}
				else
					{color(C_INFO); msg(" OK   ");}
			}
		}
	}
}
