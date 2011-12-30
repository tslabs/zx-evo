#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "pins.h"
#include "mytypes.h"

#include "getfaraddress.h"
#include "main.h"
#include "zx.h"
#include "kbmap.h"
#include "rs232.h"

const UBYTE default_kbmap[] PROGMEM =
{
NO_KEY,NO_KEY, // 00
NO_KEY,NO_KEY, // 01 F9
NO_KEY,NO_KEY, // 02
NO_KEY,NO_KEY, // 03 F5
NO_KEY,NO_KEY, // 04 F3
NO_KEY,NO_KEY, // 05 F1
NO_KEY,NO_KEY, // 06 F2
NO_KEY,NO_KEY, // 07 F12
NO_KEY,NO_KEY, // 08
NO_KEY,NO_KEY, // 09 F10
NO_KEY,NO_KEY, // 0A F8
NO_KEY,NO_KEY, // 0B F6
NO_KEY,NO_KEY, // 0C F4
KEY_CS,KEY_SP, // 0D TAB
KEY_CS,KEY_1 , // 0E ~
NO_KEY,NO_KEY, // 0F

NO_KEY,NO_KEY, // 10
NO_KEY,NO_KEY, // 11 ALT
KEY_CS,NO_KEY, // 12 LSHIFT
NO_KEY,NO_KEY, // 13
NO_KEY,NO_KEY, // 14 LCTRL
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
NO_KEY,NO_KEY, // 77 Num Lock
NO_KEY,NO_KEY, // 78 F11
KEY_SS,KEY_K , // 79 keypad +
KEY_3 ,NO_KEY, // 7A keypad 3
KEY_SS,KEY_J , // 7B keypad -
KEY_SS,KEY_B , // 7C keypad *
KEY_9 ,NO_KEY, // 7D keypad 9
NO_KEY,NO_KEY, // 7E Scroll Lock
NO_KEY,NO_KEY  // 7F F7 !!!Warning real code is 0x83 is (converted to 0x7F)
};

const UBYTE default_kbmap_E0[] PROGMEM =
{
NO_KEY,NO_KEY, // 00
NO_KEY,NO_KEY, // 01
NO_KEY,NO_KEY, // 02
NO_KEY,NO_KEY, // 03
NO_KEY,NO_KEY, // 04
NO_KEY,NO_KEY, // 05
NO_KEY,NO_KEY, // 06
NO_KEY,NO_KEY, // 07
NO_KEY,NO_KEY, // 08
NO_KEY,NO_KEY, // 09
NO_KEY,NO_KEY, // 0A
NO_KEY,NO_KEY, // 0B
NO_KEY,NO_KEY, // 0C
NO_KEY,NO_KEY, // 0D
NO_KEY,NO_KEY, // 0E
NO_KEY,NO_KEY, // 0F

NO_KEY,NO_KEY, // 10
NO_KEY,NO_KEY, // 11 ALT GR
NO_KEY,NO_KEY, // 12
NO_KEY,NO_KEY, // 13
NO_KEY,NO_KEY, // 14 RCTRL
NO_KEY,NO_KEY, // 15
NO_KEY,NO_KEY, // 16
NO_KEY,NO_KEY, // 17
NO_KEY,NO_KEY, // 18
NO_KEY,NO_KEY, // 19
NO_KEY,NO_KEY, // 1A
NO_KEY,NO_KEY, // 1B
NO_KEY,NO_KEY, // 1C
NO_KEY,NO_KEY, // 1D
NO_KEY,NO_KEY, // 1E
NO_KEY,NO_KEY, // 1F LEFT WINDOWS

NO_KEY,NO_KEY, // 20
NO_KEY,NO_KEY, // 21 multimedia Volume -
NO_KEY,NO_KEY, // 22
NO_KEY,NO_KEY, // 23
NO_KEY,NO_KEY, // 24
NO_KEY,NO_KEY, // 25
NO_KEY,NO_KEY, // 26
NO_KEY,NO_KEY, // 27 RIGHT WINDOWS
NO_KEY,NO_KEY, // 28
NO_KEY,NO_KEY, // 29
NO_KEY,NO_KEY, // 2A
NO_KEY,NO_KEY, // 2B
NO_KEY,NO_KEY, // 2C
NO_KEY,NO_KEY, // 2D
NO_KEY,NO_KEY, // 2E
NO_KEY,NO_KEY, // 2F APPLICATION

NO_KEY,NO_KEY, // 30
NO_KEY,NO_KEY, // 31
NO_KEY,NO_KEY, // 32 multimedia Volume +
NO_KEY,NO_KEY, // 33
NO_KEY,NO_KEY, // 34 multimedia Play/Pause
NO_KEY,NO_KEY, // 35
NO_KEY,NO_KEY, // 36
NO_KEY,NO_KEY, // 37 POWER
NO_KEY,NO_KEY, // 38
NO_KEY,NO_KEY, // 39
NO_KEY,NO_KEY, // 3A
NO_KEY,NO_KEY, // 3B multimedia Stop
NO_KEY,NO_KEY, // 3C
NO_KEY,NO_KEY, // 3D
NO_KEY,NO_KEY, // 3E
NO_KEY,NO_KEY, // 3F SLEEP

NO_KEY,NO_KEY, // 40
NO_KEY,NO_KEY, // 41
NO_KEY,NO_KEY, // 42
NO_KEY,NO_KEY, // 43
NO_KEY,NO_KEY, // 44
NO_KEY,NO_KEY, // 45
NO_KEY,NO_KEY, // 46
NO_KEY,NO_KEY, // 47
NO_KEY,NO_KEY, // 48
NO_KEY,NO_KEY, // 49
KEY_SS,KEY_V , // 4A keypad /
NO_KEY,NO_KEY, // 4B
NO_KEY,NO_KEY, // 4C
NO_KEY,NO_KEY, // 4D
NO_KEY,NO_KEY, // 4E
NO_KEY,NO_KEY, // 4F

NO_KEY,NO_KEY, // 50 multimedia Active
NO_KEY,NO_KEY, // 51
NO_KEY,NO_KEY, // 52
NO_KEY,NO_KEY, // 53
NO_KEY,NO_KEY, // 54
NO_KEY,NO_KEY, // 55
NO_KEY,NO_KEY, // 56
NO_KEY,NO_KEY, // 57
NO_KEY,NO_KEY, // 58
NO_KEY,NO_KEY, // 59
KEY_EN,NO_KEY, // 5A keypad ENTER
NO_KEY,NO_KEY, // 5B
NO_KEY,NO_KEY, // 5C
NO_KEY,NO_KEY, // 5D
NO_KEY,NO_KEY, // 5E WAKE
NO_KEY,NO_KEY, // 5F

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

/** User map offset in EEPROM */
#define user_kbmap 0
/** User map (extent E0) offset in EEPROM */
#define user_kbmap_E0 512

//for loading user map (pointer to start eeprom)
//const void* saved_kbmap = (void*)0;

//pointers to map
//UBYTE* kbmap;
//UBYTE* kbmap_E0;

//if want Log than comment next string
#undef LOGENABLE

void kbmap_init(void)
{
	//set pointers
//	kbmap = dbuf;
//	kbmap_E0 = dbuf + sizeof(default_kbmap);

#ifdef LOGENABLE
	to_log("kbmap_init start\r\n");
#endif
	//wait for eeprom
	eeprom_busy_wait();

#ifdef LOGENABLE
	to_log("eeprom OK\r\n");
#endif

	//read signature from eeprom
//	eeprom_read_block(dbuf, saved_kbmap, 2);

	//check signature
	if ( (eeprom_read_byte((UBYTE*)user_kbmap)=='K') &&
	     (eeprom_read_byte((UBYTE*)user_kbmap+1)=='B') )
	{
		//read from eeprom
//		eeprom_read_block(kbmap, saved_kbmap, sizeof(default_kbmap)+sizeof(default_kbmap_E0));
//		kbmap[0] = NO_KEY ;
//		kbmap[1] = NO_KEY ;
		flags_ex_register |= FLAG_EX_PS2KEYBOARD_MAP;
#ifdef LOGENABLE
		to_log("KBMAP:EEPROM\r\n");
#endif
	}
	else
	{
		//set default
//		memcpy_P(kbmap, default_kbmap, sizeof(default_kbmap));
//		memcpy_P(kbmap_E0, default_kbmap_E0, sizeof(default_kbmap_E0));
#ifdef LOGENABLE
		to_log("KBMAP:DEFAULT\r\n");
#endif
	}
}

KBMAP_VALUE kbmap_get(UBYTE scancode, UBYTE was_E0)
{
	KBMAP_VALUE ret = {{NO_KEY,NO_KEY}};

	if( scancode < 0x7F )
	{
		if( flags_ex_register&FLAG_EX_PS2KEYBOARD_MAP )
		{
			//user map
			if ( scancode )
			{
				UWORD tblptr = scancode*2 + (was_E0)?user_kbmap_E0:user_kbmap;
				ret.tb.b1 = eeprom_read_byte((UBYTE*)tblptr++ );
				ret.tb.b2 = eeprom_read_byte((UBYTE*)tblptr );
			}
		}
		else
		{
			//default map
			ULONG tblptr = scancode*2;
			if( was_E0 )
			{
				tblptr += GET_FAR_ADDRESS(default_kbmap_E0);
			}
			else
			{
				tblptr += GET_FAR_ADDRESS(default_kbmap);
			}
			ret.tb.b1 = pgm_read_byte_far( tblptr++ );
			ret.tb.b2 = pgm_read_byte_far( tblptr );
		}
	}
#ifdef LOGENABLE
{
	char log_map[] = "MP..:..,..\r\n";
	UBYTE b = scancode;
	log_map[2] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_map[3] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b = ret.tb.b1;
	log_map[5] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_map[6] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	b = ret.tb.b2;
	log_map[8] = ((b >> 4) <= 9 )?'0'+(b >> 4):'A'+(b >> 4)-10;
	log_map[9] = ((b & 0x0F) <= 9 )?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
	to_log(log_map);
}
#endif
	return ret;
}
