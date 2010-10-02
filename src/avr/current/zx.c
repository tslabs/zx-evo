#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "mytypes.h"
#include "zx.h"
#include "pins.h"
#include "getfaraddress.h"
#include "main.h"
#include "spi.h"
#include "rs232.h"
#include "ps2.h"
#include "rtc.h"

//if want Log than comment next string
#undef LOGENABLE

//zx mouse registers
volatile UBYTE zx_mouse_button;
volatile UBYTE zx_mouse_x;
volatile UBYTE zx_mouse_y;

#define ZX_FIFO_SIZE 256 /* do not change this since it must be exactly byte-wise */

UBYTE zx_fifo[ZX_FIFO_SIZE];

UBYTE zx_fifo_in_ptr;
UBYTE zx_fifo_out_ptr;

UBYTE zx_counters[40]; // filter ZX keystrokes here to assure every is pressed and released only once
UBYTE zx_map[5]; // keys bitmap. send order: LSbit first, from [4] to [0]


volatile UBYTE shift_pause;

UBYTE zx_realkbd[11];




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
NO_KEY,NO_KEY, // 7E Scroll Lock
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
NO_KEY,NO_KEY, // 7C Print Screen
KEY_CS,KEY_3 , // 7D PGUP
NO_KEY,NO_KEY, // 7E
NO_KEY,NO_KEY  // 7F
};


void zx_init(void)
{
	zx_fifo_in_ptr=zx_fifo_out_ptr=0;

	zx_task(ZX_TASK_INIT);

//	nSPICS_DDR	|= (1<<nSPICS);
//	nSPICS_PORT &= ~(1<<nSPICS);
//	_delay_us(10);
//	nSPICS_PORT |= (1<<nSPICS);
//	_delay_us(10);
//	spi_send(0xE2); // send specific reset
//	_delay_us(10);
//	nSPICS_PORT &= ~(1<<nSPICS);
//	_delay_us(10);
//	nSPICS_PORT |= (1<<nSPICS);

	//на всякий случай сбрасываем комп
	zx_spi_send(SPI_RST_REG, 0, 0);
}

UBYTE zx_spi_send(UBYTE addr, UBYTE data, UBYTE mask)
{
	UBYTE status;
	UBYTE ret;
	nSPICS_PORT &= ~(1<<nSPICS); // fix for status locking
	nSPICS_PORT |= (1<<nSPICS);  // set address of SPI register
	status = spi_send(addr);
	nSPICS_PORT &= ~(1<<nSPICS); // send data for that register
	ret = spi_send(data);
	nSPICS_PORT |= (1<<nSPICS);

	//if CPU waited
	if ( status&mask ) zx_wait_task(status);

	return ret;
}


void zx_task(UBYTE operation) // zx task, tracks when there is need to send new keymap to the fpga
{
	static UBYTE prev_code;
	static UBYTE task_state;
	static UBYTE reset_type;

	UBYTE was_data;
	UBYTE code,keynum,keybit;

	if(operation==ZX_TASK_INIT)
	{
		reset_type = 0;
		prev_code = KEY_V+1; // impossible scancode
		task_state = 0;
		shift_pause = 0;

		zx_clr_kb();
	}
	else /*if(operation==ZX_TASK_WORK)*/

	// из фифы приходит: нажатия и отжатия ресетов, нажатия и отжатия кнопков, CLRKYS (только нажание).
	// задача: упдейтить в соответствии с этим битмап кнопок, посылать его в фпгу, посылать ресеты.
	// кроме того, делать паузу в упдейте битмапа и посылке его в фпга между нажатием CS|SS и последующей не-CS|SS кнопки,
	// равно как и между отжатием не-CS|SS кнопки и последующим отжатием CS|SS.

	// сначала делаем тупо без никаких пауз - чтобы работало вообще с фифой

	{
		if( !task_state )
		{
			nSPICS_PORT |= (1<<nSPICS);

			was_data = 0;

			while( !zx_fifo_isempty() )
			{
				code=zx_fifo_copy(); // don't remove byte from fifo!

				if( code==CLRKYS )
				{
					was_data = 1; // we've got something!

					zx_fifo_get(); // remove byte from fifo

					reset_type = 0;
					prev_code  = KEY_V+1;

					zx_clr_kb();

					break; // flush changes immediately to the fpga
				}
				else if( (code&KEY_MASK) >= RSTSYS )
				{
					was_data = 1; // we've got something!

					zx_fifo_get(); // remove byte from fifo

					if( code&PRESS_MASK ) // reset key pressed
					{
						reset_type	= 0x30 & ((code+1)<<4);
						reset_type += 2;

						break; // flush immediately
					}
					else // reset key released
					{
						reset_type = 0;
					}
				}
				else /*if( (code&KEY_MASK) < 40 )*/
				{
					if( shift_pause ) // if we inside pause interval and need checking
					{
						if( (PRESS_MASK&prev_code) && (PRESS_MASK&code) )
						{
							if( /* prev key was CS|SS down */
								( (PRESS_MASK|KEY_CS)<=prev_code && prev_code<=(PRESS_MASK|KEY_SS) ) &&
								/* curr key is not-CS|SS down */
								( code<(PRESS_MASK|KEY_CS) || (PRESS_MASK|KEY_SS)<code )
							)
								break; // while loop
						}

						if( (!(PRESS_MASK&prev_code)) && (!(PRESS_MASK&code)) )
						{
							if( /* prev key was not-CS|SS up */
								( prev_code<KEY_CS || KEY_SS<prev_code ) &&
								/* curr key is CS|SS up */
								( KEY_CS<=prev_code && prev_code<=KEY_SS )
							)
								break;
						}
					}

					// just normal processing out of pause interval
					keynum = (code&KEY_MASK)>>3;

					keybit = 0x0080 >> (code&7); // KEY_MASK - надмножество битов 7

					if( code&PRESS_MASK )
						zx_map[keynum] |=	keybit;
					else
						zx_map[keynum] &= (~keybit);

					prev_code = code;
					zx_fifo_get();
					shift_pause = SHIFT_PAUSE; // init wait timer

					was_data = 1;
				}
			}

			if ( zx_realkbd[10] )
			{
				for (UBYTE i=0; i<5; i++)
				{
					 UBYTE tmp;
					 tmp = zx_realkbd[i+5];
					 was_data |= zx_realkbd[i] ^ tmp;
					 zx_realkbd[i] = tmp;
				}
				zx_realkbd[10] = 0;
			}

			if( was_data ) // initialize transfer
			{
				task_state = 7;
			}
		}
		else // sending bytes one by one in each state
		{
			task_state--;
#ifdef LOGENABLE
	char log_task_state[] = "TS..\r\n";
	log_task_state[2] = ((task_state >> 4) <= 9 )?'0'+(task_state >> 4):'A'+(task_state >> 4)-10;
	log_task_state[3] = ((task_state & 0x0F) <= 9 )?'0'+(task_state & 0x0F):'A'+(task_state & 0x0F)-10;
	to_log(log_task_state);
#endif

			if( task_state==6 ) // send (or not) reset
			{
				if( reset_type )
				{
					zx_spi_send(SPI_RST_REG, reset_type, 0x7F);
#ifdef LOGENABLE
	char log_reset_type[] = "TR..\r\n";
	log_reset_type[2] = ((reset_type >> 4) <= 9 )?'0'+(reset_type >> 4):'A'+(reset_type >> 4)-10;
	log_reset_type[3] = ((reset_type & 0x0F) <= 9 )?'0'+(reset_type & 0x0F):'A'+(reset_type & 0x0F)-10;
	to_log(log_reset_type);
#endif
				}
			}
			else if( task_state>0 )// task_state==5..1
			{
				UBYTE key_data;
				key_data = zx_map[task_state-1] | ~zx_realkbd[task_state-1];
				zx_spi_send(SPI_KBD_DAT, key_data, 0x7F);
#ifdef LOGENABLE
	char log_zxmap_task_state[] = "TK.. .. ..\r\n";
	log_zxmap_task_state[2] = ((key_data >> 4) <= 9 )?'0'+(key_data >> 4):'A'+(key_data >> 4)-10;
	log_zxmap_task_state[3] = ((key_data & 0x0F) <= 9 )?'0'+(key_data & 0x0F):'A'+(key_data & 0x0F)-10;
	log_zxmap_task_state[5] = ((zx_map[task_state-1] >> 4) <= 9 )?'0'+(zx_map[task_state-1] >> 4):'A'+(zx_map[task_state-1] >> 4)-10;
	log_zxmap_task_state[6] = ((zx_map[task_state-1] & 0x0F) <= 9 )?'0'+(zx_map[task_state-1] & 0x0F):'A'+(zx_map[task_state-1] & 0x0F)-10;
	log_zxmap_task_state[8] = ((zx_realkbd[task_state-1] >> 4) <= 9 )?'0'+(zx_realkbd[task_state-1] >> 4):'A'+(zx_realkbd[task_state-1] >> 4)-10;
	log_zxmap_task_state[9] = ((zx_realkbd[task_state-1] & 0x0F) <= 9 )?'0'+(zx_realkbd[task_state-1] & 0x0F):'A'+(zx_realkbd[task_state-1] & 0x0F)-10;
	to_log(log_zxmap_task_state);
#endif
			}
			else // task_state==0
			{
				UBYTE status;
				nSPICS_PORT |= (1<<nSPICS);
				status = spi_send(SPI_KBD_STB);    // strobe input kbd data to the Z80 port engine
				nSPICS_PORT &= ~(1<<nSPICS);
				nSPICS_PORT |= (1<<nSPICS);
				if ( status&0x7F ) zx_wait_task(status);
#ifdef LOGENABLE
	to_log("STB\r\n");
#endif
			}
		}
	}

}

void zx_clr_kb(void)
{
	BYTE i;

	for( i=0; i<sizeof(zx_map)/sizeof(zx_map[0]); i++ )
	{
		zx_map[i] = 0;
	}

	for( i=0; i<sizeof(zx_realkbd)/sizeof(zx_realkbd[0]); i++ )
	{
		zx_realkbd[i] = 0xff;
	}

	for( i=0; i<sizeof(zx_counters)/sizeof(zx_counters[0]); i++ )
	{
		zx_counters[i] = 0;
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

		if ( scancode == 0x7C ) //Print Screen
		{
			//set/reset NMI
			zx_set_config( (was_release==0)? SPI_CONFIG_NMI_FLAG : 0 );
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

		//check key of vga mode switcher
		if ( ( scancode == 0x7E ) && !was_release) zx_vga_switcher();
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

	if( zxcode==NO_KEY )
	{
		/* NOTHING */
	}
	else if( (zxcode==CLRKYS) && (!was_release) ) // does not have release option
	{
		i=39;
		do zx_counters[i]=0; while( (--i)>=0 );

		if( !zx_fifo_isfull() )
			zx_fifo_put(CLRKYS);
	}
	else if( zxcode>=RSTSYS ) // resets - press and release
	{
		if( !zx_fifo_isfull() )
			zx_fifo_put( (was_release ? 0 : PRESS_MASK) | zxcode );
	}
	else if( zxcode < 40 ); // ordinary keys too
	{
		if( was_release )
		{
			if( zx_counters[zxcode] && !(--zx_counters[zxcode]) ) // left-to-right evaluation and shortcutting
			{
				if( !zx_fifo_isfull() )
					zx_fifo_put(zxcode);
			}
		}
		else // key pressed
		{
			if( !(zx_counters[zxcode]++) )
			{
				if( !zx_fifo_isfull() )
					zx_fifo_put( PRESS_MASK | zxcode );
			}
		}
	}
}


void zx_fifo_put(UBYTE input)
{
	zx_fifo[zx_fifo_in_ptr++] = input;
}

UBYTE zx_fifo_isfull(void)
{
	//always one byte unused, to distinguish between totally full fifo and empty fifo
	return( (zx_fifo_in_ptr+1)==zx_fifo_out_ptr );
}


UBYTE zx_fifo_isempty(void)
{
	return (zx_fifo_in_ptr==zx_fifo_out_ptr);
}

UBYTE zx_fifo_get(void)
{
	return zx_fifo[zx_fifo_out_ptr++]; // get byte permanently
}

UBYTE zx_fifo_copy(void)
{
	return zx_fifo[zx_fifo_out_ptr]; // get byte but leave it in fifo
}



void zx_mouse_reset(UBYTE enable)
{
	if ( enable )
	{
		//ZX autodetecting found mouse on this values
		zx_mouse_x = 0;
		zx_mouse_y = 1;
	}
	else
	{
		//ZX autodetecting not found mouse on this values
		zx_mouse_y = zx_mouse_x = 0xFF;
	}
	zx_mouse_button = 0xFF;
	flags_register|=(FLAG_PS2MOUSE_ZX_READY);
}

void zx_mouse_task(void)
{
	if ( flags_register&FLAG_PS2MOUSE_ZX_READY )
	{
#ifdef LOGENABLE
	char log_zxmouse[] = "ZXM.. .. ..\r\n";
	log_zxmouse[3] = ((zx_mouse_button >> 4) <= 9 )?'0'+(zx_mouse_button >> 4):'A'+(zx_mouse_button >> 4)-10;
	log_zxmouse[4] = ((zx_mouse_button & 0x0F) <= 9 )?'0'+(zx_mouse_button & 0x0F):'A'+(zx_mouse_button & 0x0F)-10;
	log_zxmouse[6] = ((zx_mouse_x >> 4) <= 9 )?'0'+(zx_mouse_x >> 4):'A'+(zx_mouse_x >> 4)-10;
	log_zxmouse[7] = ((zx_mouse_x & 0x0F) <= 9 )?'0'+(zx_mouse_x & 0x0F):'A'+(zx_mouse_x & 0x0F)-10;
	log_zxmouse[9] = ((zx_mouse_y >> 4) <= 9 )?'0'+(zx_mouse_y >> 4):'A'+(zx_mouse_y >> 4)-10;
	log_zxmouse[10] = ((zx_mouse_y & 0x0F) <= 9 )?'0'+(zx_mouse_y & 0x0F):'A'+(zx_mouse_y & 0x0F)-10;
	to_log(log_zxmouse);
#endif
		//TODO: пока сделал скопом, потом сделать по одному байту за заход
		zx_spi_send(SPI_MOUSE_BTN, zx_mouse_button, 0x7F);

		zx_spi_send(SPI_MOUSE_X, zx_mouse_x, 0x7F);

		zx_spi_send(SPI_MOUSE_Y, zx_mouse_y, 0x7F);

		//data sended - reset flag
		flags_register&=~(FLAG_PS2MOUSE_ZX_READY);
	}
}


void zx_wait_task(UBYTE status)
{
	UBYTE addr = 0;
	UBYTE data = 0xFF;

	//reset flag
	flags_register &= ~FLAG_SPI_INT;

	//prepare data
	switch( status&0x7F )
	{
	case ZXW_GLUK_CLOCK:
		{
			addr = zx_spi_send(SPI_GLUK_ADDR, data, 0);
			if ( status&0x80 ) data = gluk_get_reg(addr);
			break;
		}
	}

	if ( status&0x80 ) zx_spi_send(SPI_WAIT_DATA, data, 0);
	else data = zx_spi_send(SPI_WAIT_DATA, data, 0);

	if ( !(status&0x80) )
	{
		//save data
		switch( status&0x7F )
		{
		case ZXW_GLUK_CLOCK:
			{
				gluk_set_reg(addr, data);
				break;
			}
		}
	}
/*#ifdef LOGENABLE
	char log_wait[] = "W..A..D..\r\n";
	log_wait[1] = ((status >> 4) <= 9 )?'0'+(status >> 4):'A'+(status >> 4)-10;
	log_wait[2] = ((status & 0x0F) <= 9 )?'0'+(status & 0x0F):'A'+(status & 0x0F)-10;
	log_wait[4] = ((addr >> 4) <= 9 )?'0'+(addr >> 4):'A'+(addr >> 4)-10;
	log_wait[5] = ((addr & 0x0F) <= 9 )?'0'+(addr & 0x0F):'A'+(addr & 0x0F)-10;
	log_wait[7] = ((data >> 4) <= 9 )?'0'+(data >> 4):'A'+(data >> 4)-10;
	log_wait[8] = ((data & 0x0F) <= 9 )?'0'+(data & 0x0F):'A'+(data & 0x0F)-10;
	to_log(log_wait);
#endif	 */
}

void zx_vga_switcher(void)
{
	//invert VGA mode
	modes_register ^= MODE_VGA;

	//send configuration to FPGA
	zx_spi_send(SPI_CONFIG_REG, modes_register&MODE_VGA, 0x7F);

	//save mode register to RTC NVRAM
	rtc_write(RTC_COMMON_MODE_REG, modes_register);

	//set led on keyboard
	ps2keyboard_send_cmd(PS2KEYBOARD_CMD_SETLED);
}

void zx_set_config(UBYTE flags)
{
	//send configuration to FPGA
	zx_spi_send(SPI_CONFIG_REG, (modes_register&MODE_VGA) | (flags & ~MODE_VGA), 0x7F);
}
