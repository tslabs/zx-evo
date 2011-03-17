#include <avr/io.h>
#include <avr/interrupt.h>
#include "mytypes.h"
#include "ps2.h"
#include "zx.h"
#include "pins.h"
#include "spi.h"


volatile UWORD ps2_shifter;
volatile UBYTE ps2_count;
volatile UBYTE ps2_timeout;




void ps2_task(void)
{
	UBYTE byte;

	if( ps2_count!=0 ) return; // not received anything

	byte = ps2_decode();
	if( byte ) // there is no zero byte in scancode tables so we can ignore and use it as 'nothing received'
	{
		ps2_parse(byte);
	}


	ps2_count = 11; // re-init shift counter

	//release ps2 receiver (disabled by now)
	EIFR = (1<<INTF4); // clr any spurious int which can happen when we pulldown clock pin
	PS2KBCLK_DDR  &= ~(1<<PS2KBCLK);
	PS2KBCLK_PORT |= (1<<PS2KBCLK);  //release clk pin
}



UBYTE ps2_decode(void)
{
	UBYTE t,byte;

	if( ps2_count!=0 ) return 0x00; // have nothing received

	// check packet:
	//ps2_shifter.hi - stp.par.7.6.5.4.3.2
	//ps2_shifter.lo - 1.0.strt.x.x.x.x.x

	if( !( ps2_shifter&0x8000 ) ) return 0x00; // stopbit must be 1
	if( ps2_shifter&0x0020 ) return 0x00; // startbit must be 0


	byte = (UBYTE) ( 0x00FF & (ps2_shifter>>6) );

	t = byte ^ (byte>>4);
	t = t ^ (t>>2);
	t = t ^ (t>>1); // parity

	t = t ^ (UBYTE) ( ps2_shifter>>14 ); // compare parities

	if( !(t&1) ) return 0x00; // must be different

	return byte;
}


void ps2_parse(UBYTE recbyte)
{
	static UBYTE was_release = 0;
	static UBYTE was_E0 = 0;

	static UBYTE last_scancode = 0;
	static UBYTE last_scancode_E0 = 1;

	static UBYTE skipshit = 0;



	if( skipshit )
	{
		skipshit--;
		return;
	}


	if( recbyte==0xFA ) return;
	if( recbyte==0xFE ) return;
	if( recbyte==0xEE ) return;
	if( recbyte==0xAA ) return;


	if( recbyte==0xE0 )
	{
		was_E0 = 1;
		return;
	}


	if( recbyte==0xF0 )
	{
		was_release = 1;
		return;
	}

	if( recbyte==0xE1 ) // pause pressed
	{
		skipshit=7;
		return; // skip next 7 bytes
	}


/*	// check and kill autorepeat
	if( (!was_release) && (recbyte==last_scancode) && (was_E0==last_scancode_E0) )
	{
		was_E0 = 0;
		was_release = 0;
		return;
	}


	if( was_release )
	{
		last_scancode = 0x00;
		last_scancode_E0 = 1; // set last to impossible scancode
	}
	else
	{
		last_scancode = recbyte;
		last_scancode_E0 = was_E0;
	}
*/

	if( (recbyte==last_scancode) && (was_E0==last_scancode_E0) )
	{
		if( was_release )
		{
			last_scancode = 0x00;
			last_scancode_E0 = 1; // impossible scancode: E0 00
		}
		else // was depress
		{
			return;
		}
	}

	if( !was_release )
	{
                last_scancode = recbyte;
                last_scancode_E0 = was_E0;
	}






	if( (recbyte==0x12) && was_E0 ) // skip E0 12
	{
		was_E0 = 0;
		was_release = 0;
		return;
	}


	to_zx( recbyte, was_E0, was_release );

	was_E0 = 0;
	was_release = 0;

	return;
}

