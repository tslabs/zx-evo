#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "mytypes.h"
#include "pins.h"
#include "ps2.h"
#include "rs232.h"
#include "zx.h"
#include "spi.h"
#include "atx.h"
#include "rtc.h"

ISR(TIMER2_OVF_vect)
{
	static UBYTE counter=0x00;
	static BYTE dir=0x01;
	static BYTE ocr=0x00;

	counter++; // just fucking shit to fadein-fadeout LED :-)))
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

	// PS/2 keyboard timeout tracking
	if( (ps2keyboard_count<11) && (ps2keyboard_count!=0) ) // track timeout for PS/2 keyboard
	{
		if( ps2keyboard_timeout ) ps2keyboard_timeout--;

		if( !ps2keyboard_timeout )
		{
			ps2keyboard_count = 11;
		}
	}

	// pause for keyboard CS|SS
	if( shift_pause )
		shift_pause--;

	// PS/2 mouse timeout tracking
	if( (ps2mouse_count<12) && (ps2mouse_count!=0) )
	{
		if( ps2mouse_timeout ) ps2mouse_timeout--;
		else
		{
			ps2mouse_count = 12;
		}
	}

	//check soft reset
	if ( SOFTRES_PIN & (1<<SOFTRES) )
	{
		//not pressed
		atx_counter = 0;
	}
	else
	{
		//pressed
		atx_counter++;
	}
}


ISR(INT4_vect) // receive PS/2 keyboard data. TODO: sending mode...
{
	ps2keyboard_shifter >>= 1;
	if( (PS2KBDAT_PIN&(1<<PS2KBDAT)) ) ps2keyboard_shifter |= 0x8000;

	if( !(--ps2keyboard_count) )
	{
		PS2KBCLK_PORT &= ~(1<<PS2KBCLK);
                PS2KBCLK_DDR  |= (1<<PS2KBCLK);

                EIFR = (1<<INTF4); // clr any additional int which can happen when we pulldown clock pin
	}

	ps2keyboard_timeout = PS2KEYBOARD_TIMEOUT;

	//
	EIFR = (1<<INTF4);
}

 // receive/send PS/2 mouse data
ISR(INT5_vect)
{
	if( (ps2_flags&PS2MOUSE_DIRECTION_FLAG) != 0 )
	{
		//send mode
		if( --ps2mouse_count )
		{
	  		if ( ps2mouse_shifter&1 ) PS2MSDAT_PORT |= (1<<PS2MSDAT);
			else PS2MSDAT_PORT &= ~(1<<PS2MSDAT);

			ps2mouse_shifter >>= 1;

			if( ps2mouse_count == 11 )
			{
				//first interrupt is programmed
				PS2MSDAT_DDR |= (1<<PS2MSDAT);   //ps2mouse data pin to output mode
				_delay_us(200);  //hold ps2mouse clk pin ~200us
				PS2MSCLK_PORT |= (1<<PS2MSCLK);  //release ps2mouse clk pin
    		    PS2MSCLK_DDR  &= ~(1<<PS2MSCLK);
			}
			else if( ps2mouse_count == 1)
			{
				PS2MSDAT_DDR &= ~(1<<PS2MSDAT); //ps2mouse data pin to input mode
			}
		}
		else
		{
			//ack received
			PS2MSCLK_PORT &= ~(1<<PS2MSCLK);
    	    PS2MSCLK_DDR  |= (1<<PS2MSCLK);
		}
	}
	else
	{
		//receive mode
		ps2mouse_shifter >>= 1;
		if( (PS2MSDAT_PIN&(1<<PS2MSDAT)) ) ps2mouse_shifter |= 0x8000;

		if( (--ps2mouse_count) == 1 )
		{
			PS2MSCLK_PORT &= ~(1<<PS2MSCLK);
    		PS2MSCLK_DDR  |= (1<<PS2MSCLK);
			ps2mouse_count = 0;
		}
	}

	EIFR = (1<<INTF5);

	//set timeout
	ps2mouse_timeout = PS2MOUSE_TIMEOUT;
}

 // SPI_INT
ISR(INT6_vect)
{
	ps2_flags |= SPI_INT_FLAG;
	EIFR = (1<<INTF6);
}

 // RTC up data
ISR(INT7_vect)
{
	gluk_inc();

	EIFR = (1<<INTF7);
}

