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

static volatile u8 joystick_state[4]         = { 0 };
static volatile u8 joystick_last_state[4]    = { 0 };
static volatile u8 joystick_autofire_mask[4] = { 0 };

const u8 default_joymap[] PROGMEM =
{
    CFGIF_PAD_MAPPING_MOD_CS | KEY_8,   // right
    CFGIF_PAD_MAPPING_MOD_CS | KEY_5,   // left
    CFGIF_PAD_MAPPING_MOD_CS | KEY_6,   // down
    CFGIF_PAD_MAPPING_MOD_CS | KEY_7,   // up
    KEY_SP,                             // B
    KEY_M,                              // C
    KEY_SP,                             // A
    KEY_EN,                             // Start
    CFGIF_PAD_MAPPING_NO_KEY,           // other 8 keys....
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
    CFGIF_PAD_MAPPING_NO_KEY,
};

static u8       joystick_autofire_delay = 0;
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
    joystick_autofire_delay = JOYSTICK_DEFAULT_AUTOFIRE_DELAY;

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

        // read autoire map
        for (u8 pos = 0; pos < sizeof(joystick_autofire_mask); pos++) {
            joystick_autofire_mask[pos] = eeprom_read_byte((u8*)(JOYSTICK_EEPROM_PAD0_AUTOFIRE + pos));
        }
    };
    
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
    for (u32 ptr = JOYSTICK_EEPROM_PAD0_MAP; ptr < JOYSTICK_EEPROM_PAD1_MAP + 16; ptr++)
        eeprom_write_byte((u8*)ptr, CFGIF_PAD_MAPPING_NO_KEY);

    // write empty autofire map
    for (u32 ptr = JOYSTICK_EEPROM_PAD0_AUTOFIRE; ptr < JOYSTICK_EEPROM_PAD1_AUTOFIRE + 2; ptr++) 
        eeprom_write_byte((u8*)ptr, 0);
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

u8 joystick_keymap_get(u8 joystick, u8 index) {
    u8 ret = CFGIF_PAD_MAPPING_NO_KEY;

    //default map
    if (joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) {
        // read from eeprom
        u32 tblptr = index + (joystick == 1 ? JOYSTICK_EEPROM_PAD1_MAP : JOYSTICK_EEPROM_PAD0_MAP);
		ret = eeprom_read_byte((u8*)tblptr);
    } else {
        // read default keymap
        u32 tblptr = index + GET_FAR_ADDRESS(default_joymap);
        ret = pgm_read_byte_far(tblptr);
    }

    return ret;
}

u8 joystick_keymap_read(u8 joystick, u8 index) {
    // sanity check
    if ((index > 15) || (joystick > 1) || ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0))
        return CFGIF_PAD_MAPPING_NO_KEY;

    return joystick_keymap_get(joystick, index);
}

u8 joystick_autofire_read(u8 joystick, u8 index) {
    // sanity check
    if ((index > 1) || (joystick > 1) || ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0))
        return 0;

    return joystick_autofire_mask[joystick*2 + index];
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
    u32 tblptr = index + (joystick == 1 ? JOYSTICK_EEPROM_PAD1_MAP : JOYSTICK_EEPROM_PAD0_MAP);

    // wait for eeprom
    eeprom_busy_wait();
    eeprom_write_byte((u8*)tblptr, data);

    // enable joystick
    joystick_flags |= JOYSTICK_FLAG_ACTIVE;
}

void joystick_autofire_write(u8 joystick, u8 index, u8 data) {
    // sanity check
    if ((index > 1) || (joystick > 1)) return;

    // disable joystick
    joystick_flags &= ~JOYSTICK_FLAG_ACTIVE;

    // init eeprom map if it's not used
    if ((joystick_flags & JOYSTICK_FLAG_EEPROM_MAP) == 0) {
        joystick_init_eeprom(joystick_mode.raw);
    }

    // write
    u32 tblptr = index + (joystick == 1 ? JOYSTICK_EEPROM_PAD1_AUTOFIRE : JOYSTICK_EEPROM_PAD0_AUTOFIRE);

    // wait for eeprom
    eeprom_busy_wait();
    eeprom_write_byte((u8*)tblptr, data);

    joystick_autofire_mask[joystick*2 + index] = data;

    // enable joystick
    joystick_flags |= JOYSTICK_FLAG_ACTIVE;
}

void joystick_state_to_key(u8 joystick, u8 offset, u8 state, u8 mask) {
    u8 map = CFGIF_PAD_MAPPING_NO_KEY;
    for (u8 p = 0; p < 8; p++, mask>>=1) {
        if (mask & 1) {
            // state changed
            map = joystick_keymap_get(joystick, p + offset);
            u8 isReleased = !(state & (1<<p)) ? 1 : 0;
            if (map != CFGIF_PAD_MAPPING_NO_KEY) {
                if (map & CFGIF_PAD_MAPPING_MOD_CS) update_keys(KEY_CS, isReleased);
                if (map & CFGIF_PAD_MAPPING_MOD_SS) update_keys(KEY_SS, isReleased);
                update_keys(map & CFGIF_PAD_MAPPING_KEY_MASK, isReleased);
            }
        }
    }
}

void joystick_check_state(u8 joystick, u8 toKeymap) {
    u8 joyptr = joystick*2;
    u8 joydata = joystick_state[joyptr] & (joystick_flags & JOYSTICK_FLAG_AUTOFIRE_MASK ? ~joystick_autofire_mask[joyptr] : 0xFF);
    u8 mask = joystick_last_state[joyptr] ^ joydata;
    if (mask != 0) {
        joystick_last_state[joyptr] = joydata;
        if (toKeymap == 1) 
            joystick_state_to_key(joystick, 0, joydata , mask);
        else 
            // only first 8 buttons available through kempston
            zx_spi_send(SPI_KEMPSTON_JOYSTICK, joydata);
    }
}

void joystick_task(void)
{
    if ((joystick_flags & JOYSTICK_FLAG_ACTIVE) == 0) return;

    if (joystick_autofire_delay == 0) {
        joystick_flags ^= JOYSTICK_FLAG_AUTOFIRE_MASK;
        joystick_autofire_delay = JOYSTICK_DEFAULT_AUTOFIRE_DELAY;
    }

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
