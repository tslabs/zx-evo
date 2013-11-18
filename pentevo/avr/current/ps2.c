#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/delay.h>

#include "mytypes.h"
#include "main.h"
#include "ps2.h"
#include "zx.h"
#include "pins.h"
#include "spi.h"
#include "rs232.h"
#include "rtc.h"

//if want Log than comment next string
#undef LOGENABLE

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
volatile UBYTE ps2keyboard_cmd_count;
volatile UBYTE ps2keyboard_cmd;

volatile UBYTE  ps2keyboard_log[16];
volatile UBYTE  ps2keyboard_log_start;
volatile UBYTE  ps2keyboard_log_end;

void ps2keyboard_to_log(UBYTE data)
{
	if( ps2keyboard_log_end != 0xFF )
	{
		if( ps2keyboard_log_end == 0xFE )
		{
			//first byte after reset
			ps2keyboard_log_end = ps2keyboard_log_start;
		}

#ifdef LOGENABLE
		{
			char log_ps2kb_parse[] = "LG<..:..\r\n";
			UBYTE b = ps2keyboard_log_end;
			log_ps2kb_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
			log_ps2kb_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
			b = data;
			log_ps2kb_parse[6] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
			log_ps2kb_parse[7] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
			to_log(log_ps2kb_parse);
		}
#endif
		ps2keyboard_log[ps2keyboard_log_end] = data;
		ps2keyboard_log_end++;
		if( ps2keyboard_log_end >= sizeof(ps2keyboard_log) )
		{
			ps2keyboard_log_end = 0;
		}

		if( ps2keyboard_log_end == ps2keyboard_log_start )
		{
			//overload
			ps2keyboard_log_end = 0xFF;
		}
	}
}

UBYTE ps2keyboard_from_log(void)
{
	UBYTE ret;
	if( ps2keyboard_log_start == 0xFF )
	{
		//reset state
		return 0;
	}
	if( ps2keyboard_log_end<sizeof(ps2keyboard_log) )
	{
		if( ps2keyboard_log_end != ps2keyboard_log_start )
		{
			ret = ps2keyboard_log[ps2keyboard_log_start];
#ifdef LOGENABLE
		{
			char log_ps2kb_parse[] = "LG>..:..\r\n";
			UBYTE b = ret;
			log_ps2kb_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
			log_ps2kb_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
			b = ps2keyboard_log_start;
			log_ps2kb_parse[6] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
			log_ps2kb_parse[7] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
			to_log(log_ps2kb_parse);
		}
#endif
			ps2keyboard_log_start++;
			if( ps2keyboard_log_start >= sizeof(ps2keyboard_log) )
			{
				ps2keyboard_log_start = 0;
			}
		}
		else
		{
			ret=0;
		}
	}
	else
	{
		ret = ps2keyboard_log_end;
		if ( ret==0xFE )
		{
			ret=0;
		}
		else
		{
			//after reading overload 'FF' - reset log
			ps2keyboard_reset_log();
		}
	}
	//0 - no data, 0xFF - overload
	return ret;
}

void ps2keyboard_reset_log(void)
{
#ifdef LOGENABLE
	if( ps2keyboard_log_start!=0xFF )
	to_log("LGRESET\r\n");
#endif
	ps2keyboard_log_start = 0xFF;
}

static void ps2keyboard_release_clk(void)
{
	ps2keyboard_count = 12; //counter reinit
	if( flags_register & FLAG_PS2KEYBOARD_DIRECTION )
	{
		PS2KBDAT_DDR &= ~(1<<PS2KBDAT); //ps2 keyboard data pin to input mode
		flags_register &= ~(FLAG_PS2KEYBOARD_DIRECTION); //set to receive mode
	}

	//release ps2 receiver (disabled by now)
	EIFR = (1<<INTF4); // clr any spurious int which can happen when we pulldown clock pin
	PS2KBCLK_DDR  &= ~(1<<PS2KBCLK); //ps2 keyboard clk pin to input mode
	PS2KBCLK_PORT |= (1<<PS2KBCLK);  //release clk pin
}

void ps2keyboard_send(UBYTE data)
{
#ifdef LOGENABLE
{
	char log_ps2kb_parse[] = "KB>..\r\n";
	UBYTE b = data;
	log_ps2kb_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2kb_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_ps2kb_parse);
}
#endif
	ps2keyboard_shifter = ps2_encode(data); //prepare data
	flags_register |= FLAG_PS2KEYBOARD_DIRECTION; //set send mode
	PS2KBCLK_PORT &= ~(1<<PS2KBCLK); //bring ps2 keyboard clk pin -
    PS2KBCLK_DDR  |= (1<<PS2KBCLK);  //generate interruption
}

void ps2keyboard_task(void)
{
	UBYTE b;

	if ( ( ps2keyboard_count == 12 ) &&
		 ( ps2keyboard_cmd != 0) &&
		 ( ps2keyboard_cmd_count != 0 ) )
	{
		//delay need for pause between release and hold clk pin
		_delay_us(100);

		//if need send command on current stage
		if ( ((ps2keyboard_cmd_count == 4)&&(ps2keyboard_cmd == PS2KEYBOARD_CMD_SETLED)) ||
		     ((ps2keyboard_cmd_count == 3)&&(ps2keyboard_cmd == PS2KEYBOARD_CMD_RESET)) )
		{
			ps2keyboard_send(ps2keyboard_cmd);
			ps2keyboard_cmd_count--;
		}
		else
		//if need send led data on current stage
		if ( ((ps2keyboard_cmd_count == 2)&&(ps2keyboard_cmd == PS2KEYBOARD_CMD_SETLED)) )
		{
			b = (PS2KEYBOARD_LED_SCROLLOCK|PS2KEYBOARD_LED_NUMLOCK|PS2KEYBOARD_LED_CAPSLOCK)&modes_register;
			ps2keyboard_send(b);
			ps2keyboard_cmd_count--;
		}
	}

	if ( ( ps2keyboard_count<12 ) &&
		 ( ps2keyboard_timeout==0 ) )
	{
		//error due send/receive
		ps2keyboard_release_clk();
#ifdef LOGENABLE
		to_log("KBerr\r\n");
#endif
		//TODO: чета делать

		//reset command
		ps2keyboard_cmd_count = 0;
		ps2keyboard_cmd = 0;

		//reset buffer
		zx_clr_kb();
	}

	if ( ps2keyboard_count!=0 ) return; // not received anything

	if ( !(flags_register&FLAG_PS2KEYBOARD_DIRECTION) )
	{
		//receive complete
		b = ps2_decode(ps2keyboard_count, ps2keyboard_shifter);
#ifdef LOGENABLE
{
	char log_ps2kb_parse[] = "KB<..\r\n";
	log_ps2kb_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2kb_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_ps2kb_parse);
}
#endif
		if ( ps2keyboard_cmd )
		{
			//wait for 0xFA on current stage
			if ( ((ps2keyboard_cmd == PS2KEYBOARD_CMD_SETLED)&&(ps2keyboard_cmd_count == 3 || ps2keyboard_cmd_count == 1)) ||
			     ((ps2keyboard_cmd == PS2KEYBOARD_CMD_RESET)&&(ps2keyboard_cmd_count == 2)) )
			{
				if( b != 0xFA )
				{
				 	ps2keyboard_cmd_count = 0;
					//if non FA - may be scan code received
					if ( b ) ps2keyboard_parse(b);
				}
				else ps2keyboard_cmd_count--;

				if ( ps2keyboard_cmd_count == 0 ) ps2keyboard_cmd = 0;
			}
			else
			//wait for 0xAA on current stage
			if ( ((ps2keyboard_cmd == PS2KEYBOARD_CMD_RESET)&&(ps2keyboard_cmd_count == 1)) )
			{
				if ( b != 0xAA )
				{
					//if non AA - may be scan code received
					if ( b ) ps2keyboard_parse(b);
				}
				ps2keyboard_cmd_count = 0;
				ps2keyboard_cmd = 0;
			}
		}
		else
		if ( b ) // there is no zero byte in scancode tables so we can ignore and use it as 'nothing received'
		{
			ps2keyboard_parse(b);
		}
	}

	ps2keyboard_release_clk();
}

void ps2keyboard_send_cmd(UBYTE cmd)
{
	if ( ps2keyboard_cmd == 0 )
	{
		ps2keyboard_cmd = cmd;
		switch ( cmd )
		{
		case PS2KEYBOARD_CMD_RESET:
			ps2keyboard_cmd_count = 3;
			break;
		case PS2KEYBOARD_CMD_SETLED:
			ps2keyboard_cmd_count = 4;
			break;
		default:
			ps2keyboard_cmd = 0;
		}
	}
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
	if ( skipshit ) log_ps2keyboard_parse[1] = skipshit + '0';
	log_ps2keyboard_parse[2] = ((recbyte >> 4) <= 9 )?'0'+(recbyte >> 4):'A'+(recbyte >> 4)-10;
	log_ps2keyboard_parse[3] = ((recbyte & 0x0F) <= 9 )?'0'+(recbyte & 0x0F):'A'+(recbyte & 0x0F)-10;
	to_log(log_ps2keyboard_parse);
#endif

	if( recbyte==0xFA ) return;
	if( recbyte==0xFE ) return;
	if( recbyte==0xEE ) return;
	if( recbyte==0xAA ) return;

	//start write to log only for full key data
	if( (recbyte!=0xE1) && (skipshit==0) ) //PAUSE not logged
	{
		if( ps2keyboard_log_start == 0xFF )
		{
			//reseting log
			ps2keyboard_log_end = 0xFE;
			ps2keyboard_log_start = 0;
		}
		if( (ps2keyboard_log_end!=0xFE) || ((was_release==0) && (was_E0==0)/* && (skipshit==0)*/) )
		{
		   	ps2keyboard_to_log(recbyte);
		}
	}

	if( skipshit )
	{
		skipshit--;
		return;
	}

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
			was_E0 = 0;
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
#ifdef LOGENABLE
	char log_ps2keyboard_parse2[] = "KB(..,.,.)\r\n";
	log_ps2keyboard_parse2[3] = ((recbyte >> 4) <= 9 )?'0'+(recbyte >> 4):'A'+(recbyte >> 4)-10;
	log_ps2keyboard_parse2[4] = ((recbyte & 0x0F) <= 9 )?'0'+(recbyte & 0x0F):'A'+(recbyte & 0x0F)-10;
	log_ps2keyboard_parse2[6] = (was_E0)?'1':'0';
	log_ps2keyboard_parse2[8] = (was_release)?'1':'0';
	to_log(log_ps2keyboard_parse2);
#endif

	was_E0 = 0;
	was_release = 0;

	return;
}

volatile UWORD ps2mouse_shifter;
volatile UBYTE ps2mouse_count;
volatile UBYTE ps2mouse_timeout;
volatile UBYTE ps2mouse_initstep;
volatile UBYTE ps2mouse_resp_count;
volatile UBYTE ps2mouse_cmd;

const UBYTE ps2mouse_init_sequence[] =
 	"\xFF"      //
	"\xFF"      // reset
	"\xFF"      //
	"\xF3\xC8"  // set sample rate 200  | switch to
	"\xF3\x64"  // set sample rate 100  |     scroll
	"\xF3\x50"  // set sample rate 80   |         mode
	"\xF2"      // get device type
	"\xE6"      // set scaling 1:1
	"\xF3\x64"  // set sample rate 100
	"\xF4"      // enable
	;

static void ps2mouse_release_clk(void)
{
	ps2mouse_count = 12; //counter reinit
	if( flags_register & FLAG_PS2MOUSE_DIRECTION )
	{
		PS2MSDAT_DDR &= ~(1<<PS2MSDAT); //ps2 mouse data pin to input mode
		flags_register &= ~(FLAG_PS2MOUSE_DIRECTION); //set to receive mode
	}

	//release ps2 receiver (disabled by now)
	EIFR = (1<<INTF5); // clr any spurious int which can happen when we pulldown clock pin
	PS2MSCLK_DDR  &= ~(1<<PS2MSCLK); //ps2 mouse clk pin to input mode
	PS2MSCLK_PORT |= (1<<PS2MSCLK);  //release clk pin
}

void ps2mouse_send(UBYTE data)
{
#ifdef LOGENABLE
{
	UBYTE b=data;
	char log_ps2mouse_parse[] = "MS>..\r\n";
	log_ps2mouse_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2mouse_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_ps2mouse_parse);
}
#endif
	ps2mouse_shifter = ps2_encode(data); //prepare data
	flags_register |= FLAG_PS2MOUSE_DIRECTION; //set send mode
	PS2MSCLK_PORT &= ~(1<<PS2MSCLK); //bring ps2 mouse clk pin -
    PS2MSCLK_DDR  |= (1<<PS2MSCLK);  //generate interruption
}

void ps2mouse_task(void)
{
	UBYTE b;

	if (  ps2mouse_count == 12  )
	{
		if (  ps2mouse_init_sequence[ps2mouse_initstep] != 0  )
		{
		 	if ( ps2mouse_resp_count == 0 )
			{
				//delay need for pause between release and hold clk pin
				_delay_us(200);

				//initialization not complete
				//send next command to mouse
				ps2mouse_send(ps2mouse_init_sequence[ps2mouse_initstep]);
				ps2mouse_resp_count++;
			}
		}
		else if ( ps2mouse_cmd != 0 )
		{
		 	if ( ps2mouse_resp_count == 0 )
			{
				//delay need for pause between release and hold clk pin
				_delay_us(200);

				//start command
				flags_ex_register |= FLAG_EX_PS2MOUSE_CMD;
				ps2mouse_send(ps2mouse_cmd);
				ps2mouse_resp_count++;
			}
			else if( flags_ex_register & FLAG_EX_PS2MOUSE_CMD )
			{
				switch( ps2mouse_cmd )
				{
					case PS2MOUSE_CMD_SET_RESOLUTION:
						if ( ps2mouse_resp_count == 2 )
						{
							//delay need for pause between release and hold clk pin
							_delay_us(200);

							//send resolution
							ps2mouse_send(rtc_read(RTC_PS2MOUSE_RES_REG)&0x03);
							ps2mouse_resp_count++;
						}
						break;
				}
			}
		}
	}

	if ( ( ps2mouse_count<12 ) &&
		 ( ps2mouse_timeout==0 ) )
	{
#ifdef LOGENABLE
		char log_ps2mouse_err[] = "MS.err.\r\n";
		if( flags_register&FLAG_PS2MOUSE_DIRECTION ) log_ps2mouse_err[2]='S';	else log_ps2mouse_err[2]='R';
		if( ps2mouse_count<10 ) log_ps2mouse_err[6]='0'+ps2mouse_count;  else log_ps2mouse_err[6]='A'+ps2mouse_count-10;
		to_log(log_ps2mouse_err);
#endif
		//error due exchange data with PS/2 mouse

		//get direction
		b = flags_register&FLAG_PS2MOUSE_DIRECTION;

		//reset pins and states
		ps2mouse_release_clk();

		//analizing error
		if( b && (ps2mouse_initstep==0) )
		{
			//error due send first init byte - mouse not connected to PS/2

			//disable mouse
			zx_mouse_reset(0);
		}
		else
		{
			//error due receive or send non first byte - mouse connected to PS/2

			//re-init mouse
			ps2mouse_initstep = 0;
		}
	}

	if ( ps2mouse_count!=0 ) return; // not received anything

	if ( !(flags_register&FLAG_PS2MOUSE_DIRECTION) )
	{
		//receive complete
		b = ps2_decode(ps2mouse_count, ps2mouse_shifter);

#ifdef LOGENABLE
{
	char log_ps2mouse_parse[] = "MS<..\r\n";
	log_ps2mouse_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2mouse_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_ps2mouse_parse);
}
#endif

		//if command proceed than command code
		//if initialization proceed than current init code
		//else 0
		switch( (flags_ex_register&FLAG_EX_PS2MOUSE_CMD)?ps2mouse_cmd:ps2mouse_init_sequence[ps2mouse_initstep] )
		{
			//initialization complete - working mode
			case 0:
				ps2mouse_resp_count++;
				switch( ps2mouse_resp_count )
				{
				case 1:
					//byte 1: Y overflow | X overflow | Y sign bit | X sign bit | 1 | Middle Btn | Right Btn | Left Btn
					zx_mouse_button = (zx_mouse_button&0xF0) + ((b^0x07)&0x0F);
					break;
				case 2:
					//byte 2: X movement
					zx_mouse_x += b;
					break;
				case 3:
					//byte 3: Y movement
					zx_mouse_y += b;
					if ( !(flags_register&FLAG_PS2MOUSE_TYPE) )
					{
						//classical mouse
						ps2mouse_resp_count = 0;
						flags_register |= FLAG_PS2MOUSE_ZX_READY;
					}
					break;
				case 4:
					//byte 4: wheel movement
					zx_mouse_button += ((b<<4)&0xF0);
					flags_register |= FLAG_PS2MOUSE_ZX_READY;
					ps2mouse_resp_count = 0;
				}
				break;

			//reset command
			case PS2MOUSE_CMD_RESET:
				if ( ps2mouse_resp_count==1 )
				{
					//must be acknowledge
					if ( b != 0xFA )
					{
						if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
						{
							//reset command
							ps2mouse_cmd = 0;
							flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
						}
						else
						{
							//reset initialization
							ps2mouse_initstep = 0;
						}
						ps2mouse_resp_count = 0;
						break;
					}
				}
				ps2mouse_resp_count++;
				if ( ps2mouse_resp_count >= 4 )
				{
					ps2mouse_resp_count = 0;
					if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
					{
						//reset command
						ps2mouse_cmd = 0;
						flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
					}
					else
					{
						//next initialization stage
						ps2mouse_initstep++;
					}
				}
				break;

			//get device type
			case PS2MOUSE_CMD_GET_TYPE:
				if ( ps2mouse_resp_count==1 )
				{
					ps2mouse_resp_count++;
					//must be acknowledge
					if ( b != 0xFA )
					{
						if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
						{
							//reset command
							ps2mouse_cmd = 0;
							flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
						}
						else
						{
							//reset initialization
							ps2mouse_initstep = 0;
						}
						ps2mouse_resp_count = 0;
					}
					break;
				}
				else
				{
					ps2mouse_resp_count = 0;
					if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
					{
						//reset command
						ps2mouse_cmd = 0;
						flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
					}
					else
					{
						//next initialization stage
						ps2mouse_initstep++;
					}

					if ( b > 0 )
					{
						flags_register |= FLAG_PS2MOUSE_TYPE;
					}
					else
					{
						flags_register &= ~(FLAG_PS2MOUSE_TYPE);
					}
				}
				break;

			//set resolution
			case PS2MOUSE_CMD_SET_RESOLUTION:
				//must be acknowledge
				if ( b != 0xFA )
				{
					if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
					{
						//reset command
						ps2mouse_cmd = 0;
						flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
					}
					else
					{
						//reset initialization
						ps2mouse_initstep = 0;
					}
					ps2mouse_resp_count = 0;
				}
				else
				{
					if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
					{
						if( ps2mouse_resp_count >= 3 )
						{
							ps2mouse_resp_count = 0;
							//reset command
							ps2mouse_cmd = 0;
							flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
						}
						else
						{
							ps2mouse_resp_count ++;
						}
					}
					else
					{
						//next initialization stage
						ps2mouse_resp_count = 0;
						ps2mouse_initstep++;
					}
				}
				break;

			//other commands
			default:
				if( flags_ex_register&FLAG_EX_PS2MOUSE_CMD )
				{
					//reset command
					ps2mouse_cmd = 0;
					flags_ex_register &= ~FLAG_EX_PS2MOUSE_CMD;
				}
				else
				{
					//next initialization stage
					ps2mouse_initstep++;
					if ( ps2mouse_resp_count==1 )
					{
						//must be acknowledge
						if ( b != 0xFA )
						{
							//reset initialization
							ps2mouse_initstep = 0;
						}
					}
				}
				ps2mouse_resp_count = 0;
			  	break;
		}
	}
//#ifdef LOGENABLE
//	else
//	{
//		//send complete
//		char log_ps2mouse_parse[] = "MS>..\r\n";
//		b = ps2mouse_init_sequence[ps2mouse_initstep];
//		log_ps2mouse_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
//		log_ps2mouse_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
//		to_log(log_ps2mouse_parse);
//	}
//#endif

	ps2mouse_release_clk();
}

void ps2mouse_set_resolution(UBYTE code)
{
#ifdef LOGENABLE
{
	UBYTE b = zx_mouse_button;
	char log_ps2mouse_parse[] = "SS:..-..\r\n";
	log_ps2mouse_parse[3] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2mouse_parse[4] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b = code;
	log_ps2mouse_parse[6] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_ps2mouse_parse[7] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_ps2mouse_parse);
}
#endif

	//if pressed left and right buttons on mouse
	if ( (zx_mouse_button & 0x03) == 0 )
	{
		switch( code )
		{
			//keypad '*' - set default resolution
			case 0x7C:
				rtc_write(RTC_PS2MOUSE_RES_REG,0x00);
				ps2mouse_cmd = PS2MOUSE_CMD_SET_RESOLUTION;
			   	break;

			//keypad '+' - inc resolution
			case 0x79:
			{
				UBYTE data = rtc_read(RTC_PS2MOUSE_RES_REG)&0x03;
				if( data < 0x03 )
				{
					data++;
				}
				rtc_write(RTC_PS2MOUSE_RES_REG,data);
				ps2mouse_cmd = PS2MOUSE_CMD_SET_RESOLUTION;
			    break;
			}

			//keypad '-' - dec resolution
			case 0x7B:
			{
				UBYTE data = rtc_read(RTC_PS2MOUSE_RES_REG)&0x03;
				if (data)
				{
					data--;
				}
				rtc_write(RTC_PS2MOUSE_RES_REG,data);
				ps2mouse_cmd = PS2MOUSE_CMD_SET_RESOLUTION;
			    break;
			}
		}
	}
}
