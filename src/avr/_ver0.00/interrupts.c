#include <avr/io.h>
#include <avr/interrupt.h>

#include "mytypes.h"
#include "pins.h"
#include "ps2.h"
#include "spi.h"

ISR(TIMER2_OVF_vect)
{
	static UBYTE counter=0x00;
	static BYTE dir=0x01;
	static BYTE ocr=0x00;

	counter++;
	if( counter&128 )
	{
		counter=0;

		ocr += dir;
		if( (ocr==(-1)) && (dir==(-1)) )
		{
			dir = -dir;
			ocr = 1;
		} else if( (ocr==0) && (dir==1) )
		{
			dir = -dir;
			ocr = 0xFF;
		}

		OCR2 = ocr;
	}


	if( (ps2_count<11) && (ps2_count!=0) )
	{
		if( ps2_timeout ) ps2_timeout--;

		if( !ps2_timeout )
		{
			ps2_count = 11;
		}
	}
}




ISR(INT4_vect)
{
	ps2_shifter >>= 1;
	if( (PS2KBDAT_PIN&(1<<PS2KBDAT)) ) ps2_shifter |= 0x8000;

	if( !(--ps2_count) )
	{
		PS2KBCLK_PORT &= ~(1<<PS2KBCLK);
                PS2KBCLK_DDR  |= (1<<PS2KBCLK);

                EIFR = (1<<INTF4); // clr any additional int which can happen when we pulldown clock pin
	}

	ps2_timeout = PS2_TIMEOUT;

	EIFR = (1<<INTF4);
}


