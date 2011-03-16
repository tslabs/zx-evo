#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>


#include "mytypes.h"
#include "zx.h"
#include "pins.h"
#include "getfaraddress.h"
#include "spi.h"




/*

69 - keypad 1
6B - keypad 4
6C - keypad 7
70 - keypad 0
71 - keypad .
72 - keypad 2
73 - keypad 5
74 - keypad 6
75 - keypad 8
79 - keypad +
7A - keypad 3
7B - keypad -
7C - keypad *
7D - keypad 9
E0 5A - keypad enter
E0 4A - keypad /

*/

const UBYTE kmap[] PROGMEM =
{
NO_KEY,NO_KEY, // 00
RST_48,NO_KEY, // 01  F9
NO_KEY,NO_KEY, // 02
NO_KEY,NO_KEY, // 03
NO_KEY,NO_KEY, // 04
NO_KEY,NO_KEY, // 05
NO_KEY,NO_KEY, // 06
RSTSYS,NO_KEY, // 07 F12
NO_KEY,NO_KEY, // 08
RST128,NO_KEY, // 09 F10
NO_KEY,NO_KEY, // 0A
NO_KEY,NO_KEY, // 0B
NO_KEY,NO_KEY, // 0C
KEY_CS,KEY_SP, // 0D TAB
KEY_CS,KEY_1 , // 0E ~
NO_KEY,NO_KEY, // 0F

NO_KEY,NO_KEY, // 10
NO_KEY,NO_KEY, // 11
KEY_CS,NO_KEY, // 12 LSHIFT
NO_KEY,NO_KEY, // 13
NO_KEY,NO_KEY, // 14
KEY_Q ,NO_KEY, // 15 Q
KEY_1 ,NO_KEY, // 16 1
NO_KEY,NO_KEY, // 17
NO_KEY,NO_KEY, // 18
NO_KEY,NO_KEY, // 19
KEY_Z ,NO_KEY, // 1A Z
KEY_S ,NO_KEY, // 1B S
KEY_A ,NO_KEY, // 1C A
KEY_W ,NO_KEY, // 1D W
KEY_2 ,NO_KEY, // 1E 2
NO_KEY,NO_KEY, // 1F

NO_KEY,NO_KEY, // 20
KEY_C ,NO_KEY, // 21 C
KEY_X ,NO_KEY, // 22 X
KEY_D ,NO_KEY, // 23 D
KEY_E ,NO_KEY, // 24 E
KEY_4 ,NO_KEY, // 25 4
KEY_3 ,NO_KEY, // 26 3
NO_KEY,NO_KEY, // 27
NO_KEY,NO_KEY, // 28
KEY_SP,NO_KEY, // 29 SPACE
KEY_V ,NO_KEY, // 2A V
KEY_F ,NO_KEY, // 2B F
KEY_T ,NO_KEY, // 2C T
KEY_R ,NO_KEY, // 2D R
KEY_5 ,NO_KEY, // 2E 5
NO_KEY,NO_KEY, // 2F

NO_KEY,NO_KEY, // 30
KEY_N ,NO_KEY, // 31 N
KEY_B ,NO_KEY, // 32 B
KEY_H ,NO_KEY, // 33 H
KEY_G ,NO_KEY, // 34 G
KEY_Y ,NO_KEY, // 35 Y
KEY_6 ,NO_KEY, // 36 6
NO_KEY,NO_KEY, // 37
NO_KEY,NO_KEY, // 38
NO_KEY,NO_KEY, // 39
KEY_M ,NO_KEY, // 3A M
KEY_J ,NO_KEY, // 3B J
KEY_U ,NO_KEY, // 3C U
KEY_7 ,NO_KEY, // 3D 7
KEY_8 ,NO_KEY, // 3E 8
NO_KEY,NO_KEY, // 3F

NO_KEY,NO_KEY, // 40
KEY_SS,KEY_N , // 41 ,
KEY_K ,NO_KEY, // 42 K
KEY_I ,NO_KEY, // 43 I
KEY_O ,NO_KEY, // 44 O
KEY_0 ,NO_KEY, // 45 0
KEY_9 ,NO_KEY, // 46 9
NO_KEY,NO_KEY, // 47
NO_KEY,NO_KEY, // 48
KEY_SS,KEY_M , // 49 .
KEY_SS,KEY_C , // 4A /
KEY_L ,NO_KEY, // 4B L
KEY_SS,KEY_Z , // 4C :
KEY_P ,NO_KEY, // 4D P
KEY_SS,KEY_J , // 4E -
NO_KEY,NO_KEY, // 4F

NO_KEY,NO_KEY, // 50
NO_KEY,NO_KEY, // 51
KEY_SS,KEY_P , // 52 "
NO_KEY,NO_KEY, // 53
KEY_SS,KEY_8 , // 54 [
KEY_SS,KEY_K , // 55 +
NO_KEY,NO_KEY, // 56
NO_KEY,NO_KEY, // 57
KEY_CS,KEY_2 , // 58 CAPSLOCK
KEY_SS,NO_KEY, // 59 RSHIFT
KEY_EN,NO_KEY, // 5A ENTER
KEY_SS,KEY_9 , // 5B ]
NO_KEY,NO_KEY, // 5C
KEY_SS,KEY_CS, // 5D backslash
NO_KEY,NO_KEY, // 5E
NO_KEY,NO_KEY, // 5F

NO_KEY,NO_KEY, // 60
KEY_SS,KEY_CS, // 61 backslash
NO_KEY,NO_KEY, // 62
NO_KEY,NO_KEY, // 63
NO_KEY,NO_KEY, // 64
NO_KEY,NO_KEY, // 65
KEY_CS,KEY_0 , // 66 BACKSPACE
NO_KEY,NO_KEY, // 67
NO_KEY,NO_KEY, // 68
KEY_1 ,NO_KEY, // 69 keypad 1
NO_KEY,NO_KEY, // 6A
KEY_4 ,NO_KEY, // 6B keypad 4
KEY_7 ,NO_KEY, // 6C keypad 7
NO_KEY,NO_KEY, // 6D
NO_KEY,NO_KEY, // 6E
NO_KEY,NO_KEY, // 6F

KEY_0 ,NO_KEY, // 70 keypad 0
KEY_SS,KEY_M , // 71 keypad .
KEY_2 ,NO_KEY, // 72 keypad 2
KEY_5 ,NO_KEY, // 73 keypad 5
KEY_6 ,NO_KEY, // 74 keypad 6
KEY_8 ,NO_KEY, // 75 keypad 8
CLRKYS,NO_KEY, // 76 ESC
NO_KEY,NO_KEY, // 77
RSTRDS,NO_KEY, // 78 F11
KEY_SS,KEY_K , // 79 keypad +
KEY_3 ,NO_KEY, // 7A keypad 3
KEY_SS,KEY_J , // 7B keypad -
KEY_SS,KEY_B , // 7C keypad *
KEY_9 ,NO_KEY, // 7D keypad 9
NO_KEY,NO_KEY, // 7E
NO_KEY,NO_KEY  // 7F
};



const UBYTE kmap_E0[] PROGMEM =
{
NO_KEY,NO_KEY, // 60
NO_KEY,NO_KEY, // 61
NO_KEY,NO_KEY, // 62
NO_KEY,NO_KEY, // 63
NO_KEY,NO_KEY, // 64
NO_KEY,NO_KEY, // 65
NO_KEY,NO_KEY, // 66
NO_KEY,NO_KEY, // 67
NO_KEY,NO_KEY, // 68
KEY_SS,KEY_E , // 69 END
NO_KEY,NO_KEY, // 6A
KEY_CS,KEY_5 , // 6B LEFT
KEY_SS,KEY_Q , // 6C HOME
NO_KEY,NO_KEY, // 6D
NO_KEY,NO_KEY, // 6E
NO_KEY,NO_KEY, // 6F

KEY_SS,KEY_W , // 70 INS
KEY_CS,KEY_9 , // 71 DEL
KEY_CS,KEY_6 , // 72 DOWN
NO_KEY,NO_KEY, // 73
KEY_CS,KEY_8 , // 74 RIGHT
KEY_CS,KEY_7 , // 75 UP
CLRKYS,NO_KEY, // 76 ESC
NO_KEY,NO_KEY, // 77
NO_KEY,NO_KEY, // 78
NO_KEY,NO_KEY, // 79
KEY_CS,KEY_4 , // 7A PGDN
NO_KEY,NO_KEY, // 7B
NO_KEY,NO_KEY, // 7C
KEY_CS,KEY_3 , // 7D PGUP
NO_KEY,NO_KEY, // 7E
NO_KEY,NO_KEY  // 7F
};


struct zx current;
struct zx towrite;

volatile UBYTE keys_changed;

volatile UBYTE send_state;


void zx_init(void)
{
	BYTE i;

	i=39;
	do current.counters[i] = towrite.counters[i] = 0x00; while( (--i)>=0 );

	current.map[0] = current.map[1] = current.map[2] = current.map[3] = current.map[4] = 0x00;

	towrite.map[0] = towrite.map[1] = towrite.map[2] = towrite.map[3] = towrite.map[4] = 0x00;


	nSPICS_DDR  |= (1<<nSPICS);
	nSPICS_PORT &= ~(1<<nSPICS);
	_delay_us(10);
	nSPICS_PORT |= (1<<nSPICS);
	_delay_us(10);
        	spi_send(0xE2);
	_delay_us(10);
	nSPICS_PORT &= ~(1<<nSPICS);
	_delay_us(10);
	nSPICS_PORT |= (1<<nSPICS);

	send_state = 0;
}



void zx_task(void) // zx task, tracks when there is need to send new keymap to the fpga
{
	UBYTE byte;

	if( !send_state )
	{
		nSPICS_PORT |= (1<<nSPICS); // set /SPICS

		if( keys_changed )
		{
			towrite = current; // copy current state
			keys_changed = 0;

			send_state = 6;
		}
	}
	else // we are sending data
	{
		send_state--;

		if( send_state==5 )
		{
			//send reset byte, /SPICS = 1
			byte=0x00;
			if( towrite.reset_type ) byte = ( ((towrite.reset_type+1)<<4)&0xF0 ) + 2;

			spi_send(byte);
		}
		else // send_state==4..0
		{
                        nSPICS_PORT &= ~(1<<nSPICS); // clr /SPICS

			spi_send( towrite.map[send_state] );
		}
	}
}





void to_zx(UBYTE scancode, UBYTE was_E0, UBYTE was_release)
{
	ULONG tbldisp,tblptr;
	UBYTE tbl1,tbl2;


	tbl1=tbl2=NO_KEY;

	if( was_E0 )
	{
		if( scancode==0x4A ) // keypad /
		{
			tbl1 = KEY_SS;
			tbl2 = KEY_V;
		}
		else if( scancode==0x5A ) // keypad enter
		{
			tbl1 = KEY_EN;
		}
		else if( (scancode>=0x60) && (scancode<=0x7F) )
		{
			tbldisp = (scancode-0x60)*2;
			tblptr = tbldisp + GET_FAR_ADDRESS(kmap_E0);

			tbl1 = pgm_read_byte_far( tblptr++ );
			tbl2 = pgm_read_byte_far( tblptr );
		}
	}
	else
	{
		if( scancode<=0x7F )
		{
			tbldisp = scancode*2;
			tblptr = tbldisp + GET_FAR_ADDRESS(kmap);

			tbl1 = pgm_read_byte_far( tblptr++ );
			tbl2 = pgm_read_byte_far( tblptr );
		}
	}

	if( tbl1!=NO_KEY )
	{
		update_keys(tbl1,was_release);

		if( tbl2!=NO_KEY ) update_keys(tbl2,was_release);
	}
}



void update_keys(UBYTE zxcode, UBYTE was_release)
{
	BYTE i;

	current.reset_type = 0;

	if( zxcode==NO_KEY )
	{ }
	else if( (zxcode==CLRKYS) && (!was_release) )
	{
		i=39;
		do
		{
			current.counters[i]=0;
		} while( (--i)>=0 );

		current.map[0]=current.map[1]=current.map[2]=current.map[3]=current.map[4] = 0;

                keys_changed = 1;
	}
	else if( zxcode>=RSTSYS )
	{
		keys_changed = 1;
		current.reset_type = was_release ? 0 : zxcode;
	}
	else if( zxcode < 40 );
	{
		if( was_release )
		{
			if( current.counters[zxcode] )
                        {
				if( !(--current.counters[zxcode]) )
				{
					current.map[zxcode>>3] &= ~(0x80>>(zxcode&7));
					keys_changed = 1;
				}
                        }
		}
		else // depress
		{
			if( !(current.counters[zxcode]++) )
			{
				keys_changed = 1;
				current.map[zxcode>>3] |= (0x80>>(zxcode&7));
			}
		}
	}
}



