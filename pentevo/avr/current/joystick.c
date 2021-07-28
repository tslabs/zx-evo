#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "getfaraddress.h"
#include "pins.h"
#include "mytypes.h"

#include "rs232.h"
#include "zx.h"
#include "joystick.h"
#include "kbmap.h"

//if want Log than comment next string
#undef LOGENABLE

static volatile u8 joystate[4] = { 0 };     // both gamepads start

const u8 default_joymap[] PROGMEM =
{
KEY_CS,KEY_8,              // right
KEY_CS,KEY_5,              // left
KEY_CS,KEY_6,              // down
KEY_CS,KEY_7,              // up
NO_KEY,KEY_SP,             // B
NO_KEY,KEY_J,              // C
KEY_SS,KEY_P,              // A
NO_KEY,KEY_EN,             // Start
};

KBMAP_VALUE joymap_get(u8 button) {
    KBMAP_VALUE ret = {NO_KEY, NO_KEY};

    //default map
    u32 tblptr = button*2 + GET_FAR_ADDRESS(default_joymap);
    ret.tb.b1 = pgm_read_byte_far(tblptr++);
    ret.tb.b2 = pgm_read_byte_far(tblptr);

    return ret;
}

void joystick_task(void)
{
    // pass-through joy1
	static u8 joy_state = 0;
	u8 temp = joystate[2];

	if (joy_state ^ temp)
	{
		//change state of joystick pins
		joy_state = temp;

		//send to port
        zx_spi_send(SPI_KEMPSTON_JOYSTICK, joy_state);

#ifdef LOGENABLE
	char log_joystick[] = "JS..\r\n";
	log_joystick[2] = ((temp >> 4) <= 9)?'0'+(temp >> 4):'A'+(temp >> 4)-10;
	log_joystick[3] = ((temp & 0x0F) <= 9)?'0'+(temp & 0x0F):'A'+(temp & 0x0F)-10;
	to_log(log_joystick);
#endif
	}
    KBMAP_VALUE map;

    // 2nd joy keyboard mapping
    static u8 joy2_state = 0;
    temp = joy2_state ^ joystate[2];
    if (temp != 0) {
        joy2_state = joystate[2];

        for (u8 p = 0; p < 8; p++, temp>>=1) {
            if (temp & 1) {
                // state changed
                map = joymap_get(p);
                bool isReleased = !(joy2_state & (1<<p));
                update_keys(map.tb.b1, isReleased);       // NO_KEY already filtered out inside
                update_keys(map.tb.b2, isReleased);
            }
        }

    };
}

#if 0
void joystick_poll(void) {
    // try to support 6pin joys
    static volatile u8 select = 0;
    u8 joy6btn;             // d0 - 1st joy is 6btn, d1 - 2nd joy

    while (select < 7) { 
        u8 joyport = JOYSTICK_PIN, keyport = JOYSTICK_EXT2_PIN, extport = JOYSTICK_EXT_PIN;
        switch (select) {
            case 0:
            case 2:
                // select = low,  update A and Start
                joystate[0] = (joystate[0] & (0b00111111))  | ((~joyport << (6 - JOYSTICK_FIRE))      & 0x40)
			                                                | ((~extport << (7 - JOYSTICK_EXT_START_C))  & 0x80); 
                joystate[2] = (joystate[2] & (0b00111111))  | ((~keyport << (6 - JOYSTICK_EXT2_FIRE)) & 0x40)
			                                                | ((~extport << (7 - JOYSTICK_EXT2_START_C)) & 0x80); 
                JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT | (1 << JOYSTICK_EXT_SEL);
                select++;
                break;
            case 1:
            case 3:
                // select = high, D-Pad, B and C
                joystate[0] = (joystate[0] & (0b11000000))  | ( ~joyport & JOYSTICK_MASK) 
                                                            | ((~extport << (5 - JOYSTICK_EXT_START_C))  & 0x20);
                joystate[2] = (joystate[2] & (0b11000000))  | ( ~keyport & JOYSTICK_EXT2_MASK) 
                                                            | ((~extport << (5 - JOYSTICK_EXT2_START_C)) & 0x20);
                JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT & ~(1 << JOYSTICK_EXT_SEL);
                select++;
                break;
            case 4:
                // select = low, verify if D0-D3 = 0b0000 for 6bt controller
                joy6btn = 0;
                if ((joyport & 0b00001111) == 0) joy6btn |= 1;
                if ((keyport & 0b00001111) == 0) joy6btn |= 2;

                JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT | (1 << JOYSTICK_EXT_SEL);
                select = (joy6btn == 0) ? 6 : select + 1;
                break;
            case 5:
                // select - high, read XYZ/Mode
                if (joy6btn & 1) joystate[1] = (joystate[1] & ~0b00001111) | (~joyport & 0b00001111);
                if (joy6btn & 2) joystate[3] = (joystate[3] & ~0b00001111) | (~keyport & 0b00001111);

                JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT & ~(1 << JOYSTICK_EXT_SEL);
                select = 7;
                break;
            case 6:
                // select - high, reset to low and bail out
                JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT & ~(1 << JOYSTICK_EXT_SEL);
                select = 7;
                break;
            default:
                select = 7;
                break;
        }
    };
    select = 0;
}
#endif

#if 1
void joystick_poll(void) {
    // 3pin only
    // TODO: add 6pin polling
    static u8 select = 0;

    u8 joyport = JOYSTICK_PIN, keyport = JOYSTICK_EXT2_PIN, extport = JOYSTICK_EXT_PIN;
    if (select == 0) {
        // Sel  D0 D1 D2 D3 D4 D5
        // 0    00 00 vv ^^ A  ST
        
        // update A and Start
        joystate[0] = (joystate[0] & (0b00111111))  | ((~joyport << (6 - JOYSTICK_FIRE))      & 0x40)
			                                        | ((~extport << (7 - JOYSTICK_EXT_START_C))  & 0x80); 
        joystate[2] = (joystate[2] & (0b00111111))  | ((~keyport << (6 - JOYSTICK_EXT2_FIRE)) & 0x40)
			                                        | ((~extport << (7 - JOYSTICK_EXT2_START_C)) & 0x80); 

        JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT | (1 << JOYSTICK_EXT_SEL);
        select = 1;
    } else {
        // Sel  D0 D1 D2 D3 D4 D5
        // 1    >> << vv ^^ B  C

        // update D-Pad, B and C
        joystate[0] = (joystate[0] & (0b11000000)) | ( ~joyport & JOYSTICK_MASK) 
                                                   | ((~extport << (5 - JOYSTICK_EXT_START_C))  & 0x20);
        joystate[2] = (joystate[2] & (0b11000000)) | ( ~keyport & JOYSTICK_EXT2_MASK) 
                                                   | ((~extport << (5 - JOYSTICK_EXT2_START_C)) & 0x20);

        JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT & ~(1 << JOYSTICK_EXT_SEL);
        select = 0;
    }
}
#endif

/*
void joystick_poll(void)
{
	u8 ext_port = JOYSTICK_EXT_PORT;
	cur_state = (ext_port & (1 << JOYSTICK_EXT_SEL))
		? (cur_state & ~(JOYSTICK_MASK | 0x20))
			| ((~JOYSTICK_PIN) & JOYSTICK_MASK)
			| ((~JOYSTICK_EXT_PIN << (5 - JOYSTICK_EXT_START_C)) & 0x20)
		: (cur_state & (JOYSTICK_MASK | 0x20))
			| ((~JOYSTICK_PIN << (6 - JOYSTICK_FIRE)) & 0x40)
			| ((~JOYSTICK_EXT_PIN << (7 - JOYSTICK_EXT_START_C)) & 0x80);
	JOYSTICK_EXT_PORT = ext_port ^ (1 << JOYSTICK_EXT_SEL);
}
*/
