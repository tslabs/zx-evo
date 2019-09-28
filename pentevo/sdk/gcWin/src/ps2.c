//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#include "defs.h"
#include "ps2.h"

__sfr __banked __at 0xDFF7 CMOS_ADDR;
__sfr __banked __at 0xBFF7 CMOS_DATA;
__sfr __banked __at 0xEFF7 CMOS_CONF;

#define EIHALT __asm__("ei\n halt\n");

KEY_t lastkey;
KEY_STAT_t keyboard_stat;

const KEY_t keymap[] =
{
    0, KEY_F9, 0, KEY_F5, KEY_F3, KEY_F1, KEY_F2, 0,            // 0x00
    0, KEY_F10, KEY_F8, KEY_F6, KEY_F4, KEY_TAB, 0x60, 0,       // 0x08
    0, KEY_LALT, 0, 0, 0, 'q', '1', 0,                          // 0x10
    0, 0, 'z', 's', 'a', 'w', '2', 0,                           // 0x18
    0, 'c', 'x', 'd', 'e', '4', '3', 0,                         // 0x20
    0, ' ', 'v', 'f', 't', 'r', '5', 0,                         // 0x28
    0, 'n', 'b', 'h', 'g', 'y', '6', 0,                         // 0x30
    0, 0, 'm', 'j', 'u', '7', '8', 0,                           // 0x38
    0, ',', 'k', 'i', 'o', '0', '9', 0,                         // 0x40
    0, '.', '/', 'l', ';', 'p', '-', 0,                         // 0x48
    0, 0, '\'', 0, '[', '=', 0, 0,                              // 0x50
    KEY_CAPS, 0, KEY_ENTER, ']', 0, '\\', 0, 0,                 // 0x58
    0, 0, 0, 0, 0, 0, KEY_BACK, 0,                              // 0x60
    0, '1', 0, '4', '7', 0, 0, 0,                               // 0x68
    '0', '.', '2', '5', '6', '8', KEY_ESC, 0,                   // 0x70
    KEY_F11, '+', '3', '-', '*', '9', 0, KEY_F7                 // 0x78
};

const KEY_t keymap_shift[] =
{
    0, KEY_F9, 0, KEY_F5, KEY_F3, KEY_F1, KEY_F2, 0,            // 0x00
    0, KEY_F10, KEY_F8, KEY_F6, KEY_F4, KEY_TAB, 0x60, 0,       // 0x08
    0, KEY_LALT, 0, 0, 0, 'Q', '!', 0,                          // 0x10
    0, 0, 'Z', 'S', 'A', 'W', '@', 0,                           // 0x18
    0, 'C', 'X', 'D', 'E', '$', '#', 0,                         // 0x20
    0, ' ', 'V', 'F', 'T', 'R', '%', 0,                         // 0x28
    0, 'N', 'B', 'H', 'G', 'Y', '^', 0,                         // 0x30
    0, 0, 'M', 'J', 'U', '&', '*', 0,                           // 0x38
    0, '<', 'K', 'I', 'O', ')', '(', 0,                         // 0x40
    0, '>', '?', 'L', ':', 'P', '_', 0,                         // 0x48
    0, 0, '"', 0, '{', '+', 0, 0,                               // 0x50
    KEY_CAPS, 0, KEY_ENTER, '}', 0, '|', 0, 0,                  // 0x58
    0, 0, 0, 0, 0, 0, KEY_BACK, 0,                              // 0x60
    0, '1', 0, '4', '7', 0, 0, 0,                               // 0x68
    '0', '.', '2', '5', '6', '8', KEY_ESC, 0,                   // 0x70
    KEY_F11, '+', '3', '-', '*', '9', 0, KEY_F7                 // 0x78
};

const KEY_t keymapE0[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x00
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x08
    0, KEY_LALT,  0, 0, 0, 0, 0, 0,                             // 0x10
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x18
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x20
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x28
    0, 0, 0, 0, 0, 0, 0, KEY_POWER,                             // 0x30
    0, 0, 0, 0, 0, 0, 0, KEY_SLEEP,                             // 0x38
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x40
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x48
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x50
    0, 0, 0, 0, 0, 0, KEY_WAKE, 0,                              // 0x58
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x60
    0, KEY_END, 0, KEY_LEFT, KEY_HOME, 0, 0, 0,                 // 0x68
    KEY_INS, KEY_DEL, KEY_DOWN, 0, KEY_RIGHT, KEY_UP, 0, 0,     // 0x70
    0, 0, KEY_PGDN, 0, 0, KEY_PGUP, 0, KEY_F7                   // 0x78
};

u8 cmos_read()
{
    CMOS_CONF = 0x80;
    CMOS_ADDR = 0x0C;
    CMOS_DATA = 0x00;
    CMOS_ADDR = 0xF0;
    return CMOS_DATA;
}

void cmos_clear()
{
    CMOS_CONF = 0x80;
    CMOS_ADDR = 0x0C;
    CMOS_DATA = 0x01;
    CMOS_ADDR = 0xF0;
    CMOS_DATA = 0x02;
}

void ps2_init()
{
    CMOS_CONF = 0x80;
    CMOS_ADDR = 0x0C;
    CMOS_DATA = 0x00;
    CMOS_ADDR = 0xF0;
    CMOS_DATA = 0x02;
    *(u8*)keyboard_stat = 0;
}

KEY_t ps2_read()
{
    KEY_t scancode = cmos_read();
    if(scancode == 0x83)
        scancode = 0x7F;

    switch (scancode)
    {
    case 0x00:
        return 0x00;
        break;

    case 0xFF:
        cmos_clear();
        return 0x00;
        break;

    case PS2_LSHIFT:
        keyboard_stat.KEYS_SHIFT = 1;
        return 0x00;
        break;

    case PS2_RSHIFT:
        keyboard_stat.KEYS_SHIFT = 1;
        return 0x00;
        break;

    case PS2_CTRL:
        keyboard_stat.KEYS_CTRL = 1;
        return 0x00;
        break;

    case PS2_ALT:
        keyboard_stat.KEYS_ALT = 1;
        return 0x00;
        break;

    // key unpressed
    case PS2_BREAK_CODE:
        EIHALT
        scancode = cmos_read();

        switch (scancode)
        {
        case PS2_LSHIFT:
            keyboard_stat.KEYS_SHIFT = 0;
            break;

        case PS2_RSHIFT:
            keyboard_stat.KEYS_SHIFT = 0;
            break;

        case PS2_CTRL:
            keyboard_stat.KEYS_CTRL = 0;
            break;

        case PS2_ALT:
            keyboard_stat.KEYS_ALT = 0;
            break;
        }
        return 0x00;
        break;

    // extented key pressed
    case PS2_EXTEND_CODE:
        EIHALT
        scancode = cmos_read();

        switch (scancode)
        {
        case PS2_CTRL:
            keyboard_stat.KEYS_CTRL = 1;
            return 0x00;
            break;

        case PS2_ALT:
            keyboard_stat.KEYS_ALT = 1;
            return 0x00;
            break;

        // extended key unpressed
        case PS2_BREAK_CODE:
            EIHALT
            scancode = cmos_read();
            switch (scancode)
            {
            case PS2_CTRL:
                keyboard_stat.KEYS_CTRL = 0;
                break;

            case PS2_ALT:
                keyboard_stat.KEYS_ALT = 0;
                break;
            }
            return 0x00;
            break;
        }
        return keymapE0[scancode];
        break;

    default:
        if(!keyboard_stat.KEYS_SHIFT)
        {
            return keymap[scancode];
        }
        else
        {
            return keymap_shift[scancode];
        }
        break;
    }
}

KEY_t gcGetKey()
{
    KEY_t scancode = ps2_read();
    lastkey = scancode;
    return scancode;
}

void gcWaitKey(KEY_t key)
{
    do
    {
        EIHALT
    }
    while (gcGetKey() != key);
}

void gcWaitNoKey(void)
{
    do
    {
        EIHALT
    }
    while (gcGetKey());
}
