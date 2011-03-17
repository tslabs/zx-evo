#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "mytypes.h"
#include "ps2.h"
#include "zx.h"
#include "pins.h"
#include "spi.h"
#include "rs232.h"

//ps2 flags register
volatile UBYTE ps2_flags;

UBYTE ps2_decode(UBYTE count, UWORD shifter)
{
	UBYTE t,byte;

	if( count!=0 ) return 0x00; // have nothing received

	// check packet:
	//shifter.hi - stp.par.7.6.5.4.3.2
	//shifter.lo - 1.0.strt.x.x.x.x.x

	if( !( shifter&0x8000 ) ) return 0x00; // stopbit must be 1
	if( shifter&0x0020 ) return 0x00; // startbit must be 0


	byte = (UBYTE) ( 0x00FF & (shifter>>6) );

	t = byte ^ (byte>>4);
	t = t ^ (t>>2);
	t = t ^ (t>>1); // parity

	t = t ^ (UBYTE) ( shifter>>14 ); // compare parities

	if( !(t&1) ) return 0x00; // must be different

	return byte;
}

UWORD ps2_encode(UBYTE byte)
{
	UWORD t;
	t = byte ^ (byte>>4);
	t = t ^ (t>>2);
	t = ~(1 & (t ^ (t>>1))); // parity

	t = (((t<<8) + byte)<<1) + 0x0400;

	// prepare to shifter:
	//shifter.hi - x.x.x.x.x.stp.par.7
	//shifter.lo - 6.5.4.3.2.1.0.strt
	return t;
}

volatile UWORD ps2keyboard_shifter;
volatile UBYTE ps2keyboard_count;
volatile UBYTE ps2keyboard_timeout;

void ps2keyboard_task(void)
{
	UBYTE byte;

	if( ps2keyboard_count!=0 ) return; // not received anything

	byte = ps2_decode(ps2keyboard_count, ps2keyboard_shifter);
	if( byte ) // there is no zero byte in scancode tables so we can ignore and use it as 'nothing received'
	{
		ps2keyboard_parse(byte);
	}

	ps2keyboard_count = 11; // re-init shift counter

	//release ps2 receiver (disabled by now)
	EIFR = (1<<INTF4); // clr any spurious int which can happen when we pulldown clock pin
	PS2KBCLK_DDR  &= ~(1<<PS2KBCLK);
	PS2KBCLK_PORT |= (1<<PS2KBCLK);  //release clk pin
}

void ps2keyboard_parse(UBYTE recbyte)
{
	static UBYTE was_release = 0;
	static UBYTE was_E0 = 0;

	static UBYTE last_scancode = 0;
	static UBYTE last_scancode_E0 = 1;

	static UBYTE skipshit = 0;

#ifdef LOGENABLE
	char log_ps2keyboard_parse[] = "KB..\r\n";
	log_ps2keyboard_parse[2] = ((recbyte >> 4) <= 9 )?'0'+(recbyte >> 4):'A'+(recbyte >> 4)-10;
	log_ps2keyboard_parse[3] = ((recbyte & 0x0F) <= 9 )?'0'+(recbyte & 0x0F):'A'+(recbyte & 0x0F)-10;
	to_log(log_ps2keyboard_parse);
#endif


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


	to_zx( recbyte, was_E0, was_release ); // send valid scancode to zx decoding stage

	was_E0 = 0;
	was_release = 0;

	return;
}

volatile UWORD ps2mouse_shifter;
volatile UBYTE ps2mouse_count;
volatile UBYTE ps2mouse_timeout;
volatile UBYTE ps2mouse_initstep;
volatile UBYTE ps2mouse_resp_count;

UBYTE ps2mouse_init_sequence[] =
 	"\xFF"      //
	"\xFF"      // reset
	"\xFF"      //
	"\xF3\xC8"  // set sample rate 200  | switch to
	"\xF3\x64"  // set sample rate 100  |     scroll
	"\xF3\x50"  // set sample rate 80   |         mode
	"\xF2"      // get device type
	"\xF3\x0A"  // set sample rate 10
	"\xF2"      // get device type
	"\xE8\x02"  // set resolution
	"\xE6"      // set scaling 1:1
	"\xF3\x64"  // set sample rate 100
	"\xF4"      // enable
	;

static void ps2mouse_release_clk(void)
{
	ps2mouse_count = 12; //counter reinit
	if( ps2_flags & PS2MOUSE_DIRECTION_FLAG )
	{
		PS2MSDAT_DDR &= ~(1<<PS2MSDAT); //ps2 mouse data pin to input mode
		ps2_flags &= ~(PS2MOUSE_DIRECTION_FLAG); //clear direction
	}

	//release ps2 receiver (disabled by now)
	EIFR = (1<<INTF5); // clr any spurious int which can happen when we pulldown clock pin
	PS2MSCLK_DDR  &= ~(1<<PS2MSCLK); //ps2 mouse clk pin to input mode
	PS2MSCLK_PORT |= (1<<PS2MSCLK);  //release clk pin
}

void ps2mouse_send(UBYTE data)
{
	ps2mouse_shifter = ps2_encode(data); //prepare data
	ps2_flags |= PS2MOUSE_DIRECTION_FLAG; //set send mode
	PS2MSCLK_PORT &= ~(1<<PS2MSCLK); //bring ps2 mouse clk pin -
    PS2MSCLK_DDR  |= (1<<PS2MSCLK);  //generate interruption
}

void ps2mouse_task(void)
{
	UBYTE byte;

	if ( ( ps2mouse_count == 12 ) &&
		 ( ps2mouse_resp_count == 0) &&
		 ( ps2mouse_init_sequence[ps2mouse_initstep] != 0 ) )
	{
		//delay need for pause between release and hold clk pin
		_delay_us(100);

		//initialization not complete
		//send next command to mouse
		ps2mouse_send(ps2mouse_init_sequence[ps2mouse_initstep]);
		ps2mouse_resp_count++;
	}

	if ( ( ps2mouse_count<12 ) &&
		 ( ps2mouse_timeout==0 ) )
	{
		//error due send/receive
		ps2mouse_release_clk();
#ifdef LOGENABLE
		to_log("MSerr\r\n");
#endif
		//disable mouse
		zx_mouse_reset(0);

		//TODO: чета делать чтобы плуг анд плай был
		//типа если маус уже проинициализирован то инитить заново
	}

	if ( ps2mouse_count!=0 ) return; // not received anything

	if ( !(ps2_flags&PS2MOUSE_DIRECTION_FLAG) )
	{
		//receive complete
		byte = ps2_decode(ps2mouse_count, ps2mouse_shifter);

#ifdef LOGENABLE
{
	char log_ps2mouse_parse[] = "MS<..\r\n";
	log_ps2mouse_parse[3] = ((byte >> 4) <= 9 )?'0'+(byte >> 4):'A'+(byte >> 4)-10;
	log_ps2mouse_parse[4] = ((byte & 0x0F) <= 9 )?'0'+(byte & 0x0F):'A'+(byte & 0x0F)-10;
	to_log(log_ps2mouse_parse);
}
#endif

		switch( ps2mouse_init_sequence[ps2mouse_initstep] )
		{
			//initialization complete - working mode
			case 0:
				//TODO: send to ZX here
				ps2mouse_resp_count++;
				switch( ps2mouse_resp_count )
				{
				case 1:
					//byte 1: Y overflow | X overflow | Y sign bit | X sign bit | 1 | Middle Btn | Right Btn | Left Btn
					zx_mouse_button = (zx_mouse_button&0xF0) + ((byte^0x07)&0x0F);
					break;
				case 2:
					//byte 2: X movement
					zx_mouse_x += byte;
					break;
				case 3:
					//byte 3: Y movement
					zx_mouse_y += byte;
					if ( !(ps2_flags&PS2MOUSE_TYPE_FLAG) )
					{
						//classical mouse
						ps2mouse_resp_count = 0;
						ps2_flags |= PS2MOUSE_ZX_READY_FLAG;
					}
					break;
				case 4:
					//byte 4: wheel movement
					zx_mouse_button += ((byte<<4)&0xF0);
					ps2_flags |= PS2MOUSE_ZX_READY_FLAG;
					ps2mouse_resp_count = 0;
				}
				break;

			//reset command
			case 0xFF:
				if ( ps2mouse_resp_count==1 )
				{
					//must be acknowledge
					if ( byte != 0xFA )
					{
						//reset initialization
						ps2mouse_initstep = 0;
						ps2mouse_resp_count = 0;
						break;
					}
				}
				ps2mouse_resp_count++;
				if ( ps2mouse_resp_count >= 4 )
				{
					ps2mouse_resp_count = 0;
					ps2mouse_initstep++;
				}
				break;

			//get device type
			case 0xF2:
				if ( ps2mouse_resp_count==1 )
				{
					ps2mouse_resp_count++;
					//must be acknowledge
					if ( byte != 0xFA )
					{
						//reset initialization
						ps2mouse_initstep = 0;
						ps2mouse_resp_count = 0;
					}
					break;
				}
				else
				{
					ps2mouse_resp_count = 0;
					ps2mouse_initstep++;

					if ( byte > 0 )
					{
						ps2_flags |= PS2MOUSE_TYPE_FLAG;
					}
					else
					{
						ps2_flags &= ~(PS2MOUSE_TYPE_FLAG);
					}
				}
				break;

			//other commands
			default:
				if ( ps2mouse_resp_count==1 )
				{
					//must be acknowledge
					if ( byte != 0xFA )
					{
						//reset initialization
						ps2mouse_initstep = 0;
						ps2mouse_resp_count = 0;
						break;
					}
				}
				ps2mouse_resp_count = 0;
				ps2mouse_initstep++;
			  	break;
		}
	}
#ifdef LOGENABLE
	else
	{
		//send complete
		char log_ps2mouse_parse[] = "MS>..\r\n";
		byte = ps2mouse_init_sequence[ps2mouse_initstep];
		log_ps2mouse_parse[3] = ((byte >> 4) <= 9 )?'0'+(byte >> 4):'A'+(byte >> 4)-10;
		log_ps2mouse_parse[4] = ((byte & 0x0F) <= 9 )?'0'+(byte & 0x0F):'A'+(byte & 0x0F)-10;
		to_log(log_ps2mouse_parse);
	}
#endif

	ps2mouse_release_clk();
}

