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

u8 lastkey;

const u8 keymap[] =
{
    0, KEY_F9, 0, KEY_F5, KEY_F3, KEY_F1, KEY_F2, 0,
    0, KEY_F10, KEY_F8, KEY_F6, KEY_F4, KEY_TAB, 0x60, 0,
    0, 0, 0, 0, 0, 'q', '1', 0,
    0, 0, 'z', 's', 'a', 'w', '2', 0,
    0, 'c', 'x', 'd', 'e', '4', '3', 0,
    0, ' ', 'v', 'f', 't', 'r', '5', 0,
    0, 'n', 'b', 'h', 'g', 'y', '6', 0,
    0, 0, 'm', 'j', 'u', '7', '8', 0,
    0, ',', 'k', 'i', 'o', '0', '9', 0,
    0, '.', '/', 'l', ';', 'p', '-', 0,
    0, 0, '\'', 0, '[', '=', 0, 0,
    KEY_CAPS, 0, KEY_ENTER, ']', 0, '\\', 0, 0,
    0, 0, 0, 0, 0, 0, KEY_BACK, 0,
    0, '1', 0, '4', '7', 0, 0, 0,
    '0', '.', '2', '5', '6', '8', KEY_ESC, 0,
    KEY_F11, '+', '3', '-', '*', '9', 0, KEY_F7
};

const u8 keymapE0[] =
{
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x00
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x08
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x10
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x18
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x20
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x28
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x30
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x38
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x40
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x48
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x50
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x58
    0, 0, 0, 0, 0, 0, 0, 0,                                     // 0x60
    0, KEY_END, 0, KEY_LEFT, KEY_HOME, 0, 0, 0,                 // 0x68
    KEY_INS, KEY_DEL, KEY_DOWN, 0, KEY_RIGHT, KEY_UP, 0, 0,     // 0x70
    0, 0, KEY_PGDN, 0, 0, KEY_PGUP, 0, KEY_F7                   // 0x78
};

u8 cmos_read()
{
    CMOS_ADDR = 0xF0;
    return CMOS_DATA;
}

void cmos_clear()
{
    CMOS_ADDR = 0x0C;
    CMOS_DATA = 0x01;
}

void ps2_init()
{
    CMOS_CONF = 0x80;
    CMOS_ADDR = 0xF0;
    CMOS_DATA = 0x02;
}

u8 ps2_read()
{
    u8 scancode = cmos_read();
    if(scancode == 0x83) scancode = 0x7F;

    switch (scancode)
    {
    case 0x00:
        return 0x00;
    break;

    case 0xFF:
        cmos_clear();
        return 0x00;
    break;

    case 0xF0:
        EIHALT
        cmos_read();
        return 0x00;
    break;

    case 0xE0:
        EIHALT
        scancode = cmos_read();
        if(scancode == 0xF0)
        {
            EIHALT
            cmos_read();
            return 0x00;
        }
        return keymapE0[scancode];
    break;

    default:
        return keymap[scancode];
    break;
    }
}

u8 gcGetKey()
{
    u8 scancode;
    scancode = ps2_read();
    lastkey = scancode;
    return scancode;
}

void gcWaitKey(u8 key)
{
    do {
    __asm__("ei\n halt\n");
    } while (gcGetKey() != key);
}

void gcWaitNoKey(void)
{
    do {
    __asm__("ei\n halt\n");
    } while (gcGetKey());
}
