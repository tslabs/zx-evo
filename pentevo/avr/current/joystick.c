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
#include "config_interface.h"
#include "kbmap.h"

//if want Log than comment next string
#undef LOGENABLE

static volatile u8 joystick_state[4]      = { 0 };
static volatile u8 joystick_last_state[4] = { 0 };

const u8 default_joymap[] PROGMEM =
{
    KEY_CS,KEY_8,              // right
    KEY_CS,KEY_5,              // left
    KEY_CS,KEY_6,              // down
    KEY_CS,KEY_7,              // up
    NO_KEY,KEY_SP,             // B
    NO_KEY,KEY_M,              // C
    NO_KEY,KEY_SP,             // A
    NO_KEY,KEY_EN,             // Start
    NO_KEY,NO_KEY,             // other 8 keys....
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
    NO_KEY,NO_KEY,
};

static u8       joystick_flags = 0;
joystick_mode_t joystick_mode;

static u8 joystick_sega_select;

void joystick_set_gpio() {
    switch(joystick_mode.b.pad0) {
        case CFGIF_PAD_MODE_KEMPSTON:
            PORTA = 0b11111111;
            DDRA  = 0b00000000; // pulled up
            break;
        case CFGIF_PAD_MODE_SEGA:
        case CFGIF_PAD_MODE_SEGA6BUTTON:
            PORTA = 0b11111011;
            DDRA  = 0b00000100; // pulled up, PA2 used as output for SMD gamepad SEL
            joystick_sega_select = 0;
            break;
        default:
            break;
    }

    switch(joystick_mode.b.pad1) {
        case CFGIF_PAD_MODE_SEGA:
        case CFGIF_PAD_MODE_SEGA6BUTTON:
            PORTA = 0b11111011;
            DDRA  = 0b00000100; // pulled up, PA2 used as output for SMD gamepad SEL
            joystick_sega_select = 0;
            break;

        default:
            break;
    }
}

void joystick_init() {
    joystick_flags = 0;
    joystick_mode.raw = 0;

    // read joy configuration from eeprom
    // wait for eeprom
	eeprom_busy_wait();

    if ((eeprom_read_byte((u8*)JOYSTICK_EEPROM_POS)   == 'J') && 
	    (eeprom_read_byte((u8*)JOYSTICK_EEPROM_POS+1) == 'O') &&
        (eeprom_read_byte((u8*)JOYSTICK_EEPROM_POS+2) == 'Y'))
	{
        // use keymap from eeprom
        joystick_flags |= JOYSTICK_FLAG_EEPROM_MAP;

        // read config from eeprom
        joystick_mode.raw = eeprom_read_byte((u8*)JOYSTICK_EEPROM_MODE);
    };

    printf("joyflags = 0x%02X\n", joystick_flags);
    
    // set gpio
    joystick_set_gpio();

    // enable joystick
    joystick_flags |= JOYSTICK_FLAG_ACTIVE;
}

void joystick_init_eeprom(u8 mode) {
    eeprom_busy_wait();

    // write info
    eeprom_write_byte((u8*)JOYSTICK_EEPROM_POS,   'J');
    eeprom_write_byte((u8*)JOYSTICK_EEPROM_POS+1, 'O');
    eeprom_write_byte((u8*)JOYSTICK_EEPROM_POS+2, 'Y');
    eeprom_write_byte((u8*)JOYSTICK_EEPROM_MODE, mode);

    // write empty mapping
    for (u32 ptr = JOYSTICK_EEPROM_PAD0_MAP; ptr < JOYSTICK_EEPROM_PAD1_MAP + 16*2; ptr++)
        eeprom_write_byte((u8*)ptr, NO_KEY);
}

void joystick_set_mode(u8 mode) {
    // disable joystick
    joystick_flags &= ~JOYSTICK_FLAG_ACTIVE;

    // change mode
    joystick_mode.raw = mode;

    // filter out unsupported modes
    if (joystick_mode.b.pad0 > CFGIF_PAD_MODE_LAST) joystick_mode.b.pad0 = CFGIF_PAD_MODE_KEMPSTON;
    if (joystick_mode.b.pad1 > CFGIF_PAD_MODE_LAST) joystick_mode.b.pad1 = CFGIF_PAD_MODE_NONE;

    // write mode to eeprom
    eeprom_busy_wait();

    // check if eeprom data is used
    if ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0)
        joystick_init_eeprom(joystick_mode.raw);
    else
        // write mode only
        eeprom_write_byte((u8*)JOYSTICK_EEPROM_MODE, joystick_mode.raw);
    
    // set gpio
    joystick_set_gpio();

    // enable joystick
    joystick_flags |= JOYSTICK_FLAG_ACTIVE;
}

u8 joystick_get_mode() {
    return joystick_mode.raw;
}

KBMAP_VALUE joystick_keymap_get(u8 joystick, u8 index) {
    KBMAP_VALUE ret = {NO_KEY, NO_KEY};

    //default map
    if (joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) {
        // read from eeprom
        u32 tblptr = index*2 + (joystick == 1 ? JOYSTICK_EEPROM_PAD1_MAP : JOYSTICK_EEPROM_PAD0_MAP);
        ret.tb.b1 = eeprom_read_byte((u8*)tblptr++);
		ret.tb.b2 = eeprom_read_byte((u8*)tblptr);
    } else {
        // read default keymap
        u32 tblptr = index*2 + GET_FAR_ADDRESS(default_joymap);
        ret.tb.b1 = pgm_read_byte_far(tblptr++);
        ret.tb.b2 = pgm_read_byte_far(tblptr);
    }

    return ret;
}

u8 joystick_keymap_read(u8 joystick, u8 index) {
    // sanity check
    if ((index > 15) || (joystick > 1) || ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0))
        return CFGIF_PAD_MAPPING_NO_KEY;

    KBMAP_VALUE val = joystick_keymap_get(joystick, index);

    if ((val.tb.b1 == NO_KEY) && (val.tb.b2 == NO_KEY)) return CFGIF_PAD_MAPPING_NO_KEY;
    
    u8 rtn = (val.tb.b1 == KEY_CS ? CFGIF_PAD_MAPPING_MOD_CS : 
              val.tb.b1 == KEY_SS ? CFGIF_PAD_MAPPING_MOD_SS : 0);
    rtn |= val.tb.b2;

    return rtn;
}

void joystick_keymap_write(u8 joystick, u8 index, u8 data) {
    // sanity check
    if ((index > 15) || (joystick > 1)) return;

    // disable joystick
    joystick_flags &= ~JOYSTICK_FLAG_ACTIVE;

    // init eeprom map if it's not used
    
    if ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0) {
        joystick_init_eeprom(joystick_mode.raw);
    }

    // write
    u32 tblptr = index*2 + (joystick == 1 ? JOYSTICK_EEPROM_PAD1_MAP : JOYSTICK_EEPROM_PAD0_MAP);

    // wait for eeprom
    eeprom_busy_wait();

    if (data == CFGIF_PAD_MAPPING_NO_KEY) {
        eeprom_write_byte((u8*)tblptr++, NO_KEY);
        eeprom_write_byte((u8*)tblptr, NO_KEY);
    } else {
        eeprom_write_byte((u8*)tblptr++,
            ((data & CFGIF_PAD_MAPPING_MOD_CS) == CFGIF_PAD_MAPPING_MOD_CS) ? KEY_CS : 
            ((data & CFGIF_PAD_MAPPING_MOD_SS) == CFGIF_PAD_MAPPING_MOD_SS) ? KEY_SS : NO_KEY);
        eeprom_write_byte((u8*)tblptr,
            (data & CFGIF_PAD_MAPPING_KEY_MASK));
    }

    // enable joystick
    joystick_flags |= JOYSTICK_FLAG_ACTIVE;
}

#if 1

void joystick_state_to_key(u8 joystick, u8 offset, u8 state, u8 mask) {
    KBMAP_VALUE map = {NO_KEY, NO_KEY};
    for (u8 p = 0; p < 8; p++, mask>>=1) {
        if (mask & 1) {
            // state changed
            map = joystick_keymap_get(joystick, p + offset);
            u8 isReleased = !(state & (1<<p)) ? 1 : 0;
            update_keys(map.tb.b1, isReleased);       // NO_KEY already filtered out inside
            update_keys(map.tb.b2, isReleased);
        }
    }
}

void joystick_check_state(u8 joystick, u8 toKeymap) {
    u8 joyptr = joystick*2;
    u8 mask = joystick_last_state[joyptr] ^ joystick_state[joyptr];
    if (mask != 0) {
        joystick_last_state[joyptr] = joystick_state[joyptr];
        if (toKeymap == 1) 
            joystick_state_to_key(joystick, 0, joystick_state[joyptr], mask);
        else 
            // only first 8 buttons available through kempston
            zx_spi_send(SPI_KEMPSTON_JOYSTICK, joystick_state[joyptr]);
    }
}

void joystick_task(void)
{
    if ((joystick_flags & JOYSTICK_FLAG_ACTIVE) == 0) return;
    
    switch(joystick_mode.b.pad_mapping) {
        case CFGIF_PAD_MAPPING_KEMPSTON_KEYS:
            joystick_check_state(0, 0);
            joystick_check_state(1, 1);
            break;

        case CFGIF_PAD_MAPPING_KEYS_KEMPSTON:
            joystick_check_state(0, 1);
            joystick_check_state(1, 0);
            break;

        case CFGIF_PAD_MAPPING_KEYS_KEYS:
            joystick_check_state(0, 1);
            joystick_check_state(1, 1);
            break;

        default:
            break;
    }
}

#else
void joystick_task(void)
{
    // pass-through joy1
	static u8 joy_state = 0;
	u8 temp = joystate[0];

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
#endif

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
void joystick_kempston_read() {
    joystick_state[0] = (~JOYSTICK_PIN & JOYSTICK_MASK);
}

void joystick_sega_read(u8 joystick) {
    u8 joyport, start_c_bit;
    u8 extport = JOYSTICK_EXT_PIN;

    switch(joystick) {
        case 0:
            joyport = JOYSTICK_PIN;
            start_c_bit = (~extport >> JOYSTICK_EXT_START_C) & 1;
            break;
        case 1:
            joyport = JOYSTICK_EXT2_PIN;
            start_c_bit = (~extport >> JOYSTICK_EXT2_START_C) & 1;
            break;
        default:
            return;
    }
    u8 joyptr  = joystick*2;

    // 8-bit kempston mapping:
    // D7 D6 D5 D4 D3 D2 D1 D0
    // ST A  C  B  ^^ vv << >>

    if (joystick_sega_select == 0) {
        // Sel  D0 D1 D2 D3 D4 D5
        // 0    00 00 vv ^^ A  ST
        
        // update A and Start
        joystick_state[joyptr] = ((joystick_state[joyptr] & (0b00111111)) |
                                    ((~joyport     << (6 - JOYSTICK_FIRE)) & 0x40) |
			                         ( start_c_bit << (7))); 

    } else {
        // Sel  D0 D1 D2 D3 D4 D5
        // 1    >> << vv ^^ B  C

        // update D-Pad, B and C
        joystick_state[joyptr] = ((joystick_state[joyptr] & (0b11000000)) |
                                    (~joyport     & JOYSTICK_MASK) |
                                    ( start_c_bit << (5)));
    }

}

void joystick_sega_read_toggle_select() {
    if (joystick_sega_select == 0) {
        JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT | (1 << JOYSTICK_EXT_SEL);
        joystick_sega_select = 1;
    } else {
        JOYSTICK_EXT_PORT = JOYSTICK_EXT_PORT & ~(1 << JOYSTICK_EXT_SEL);
        joystick_sega_select = 0;
    }
}

void joystick_poll(void) {
    if ((joystick_flags & JOYSTICK_FLAG_ACTIVE) == 0) return;
    // 3pin only
    // TODO: add 6pin polling
    bool isSega = false;

    switch(joystick_mode.b.pad0) {
        case CFGIF_PAD_MODE_KEMPSTON:
            joystick_kempston_read();
            break;
        case CFGIF_PAD_MODE_SEGA:
            isSega = true;
            joystick_sega_read(0);
            break;
        default:
            break;
    }

    switch(joystick_mode.b.pad1) {
        case CFGIF_PAD_MODE_SEGA:
            isSega = true;
            joystick_sega_read(1);
            break;
        default:
            break;
    }

    // flip select
    if (isSega) joystick_sega_read_toggle_select();
}


#else
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

