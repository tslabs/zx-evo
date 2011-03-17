#include <avr/io.h>
#include <util/delay.h>

#include "pins.h"
#include "mytypes.h"

#include "atx.h"
#include "rs232.h"

volatile UWORD atx_counter;

void wait_for_atx_power(void)
{
	UBYTE j = MCUCSR;

	//clear status register
	MCUCSR = 0;

#ifdef LOGENABLE
	char log_ps2keyboard_parse[] = "MC..\r\n";
	log_ps2keyboard_parse[2] = ((j >> 4) <= 9 )?'0'+(j >> 4):'A'+(j >> 4)-10;
	log_ps2keyboard_parse[3] = ((j & 0x0F) <= 9 )?'0'+(j & 0x0F):'A'+(j & 0x0F)-10;
	to_log(log_ps2keyboard_parse);
#endif

	//check power
	if ( (nCONFIG_PIN & (1<<nCONFIG)) == 0 )
	{
		//if not external reset
		//then wait for atx power on button (SOFTRESET)
		if ( !(j & ((1<<JTRF)|(1<<WDRF)|(1<<BORF)|(1<<EXTRF))) ||
			 (j & (1<<PORF)) )
		while( SOFTRES_PIN&(1<<SOFTRES) );

		//switch on ATX power
		ATXPWRON_PORT |= (1<<ATXPWRON);

		//1 sec delay
		j=50;
		do _delay_ms(20); while(--j);
	}

	//init port F
	PORTF = 0b11111000;
	//clear counter
	atx_counter = 0;
}

UBYTE atx_power_task(void)
{
	UBYTE j = 50;

	if ( atx_counter > 1700 )
	{
		//atx power off button pressed (~5 sec)

		//switch off atx power
		ATXPWRON_PORT &= ~(1<<ATXPWRON);
	}

	if ( ( nCONFIG_PIN & (1<<nCONFIG) ) == 0 )
	{
		//power down

		//wait for button released
		while (  ( SOFTRES_PIN & (1<<SOFTRES) ) == 0 );

		//1 sec delay
		do _delay_ms(20); while(--j);
	}
	return j;
}
