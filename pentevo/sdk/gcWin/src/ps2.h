//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

#pragma once

#include "defs.h"

typedef struct KEY_STAT_t
{
    unsigned    KEYS_LATRUS :1;
    unsigned    bit1        :1;
    unsigned    bit2        :1;
    unsigned    bit3        :1;
    unsigned    bit4        :1;
    unsigned    KEYS_ALT    :1;
    unsigned    KEYS_CTRL   :1;
    unsigned    KEYS_SHIFT  :1;
} KEY_STAT_t;

#define PS2_BREAK_CODE      0xF0
#define PS2_EXTEND_CODE     0xE0

#define PS2_F9      0x01
#define PS2_F5      0x03
#define PS2_F3      0x04
#define PS2_F1      0x05
#define PS2_F2      0x06
#define PS2_F12     0x07
#define PS2_F10     0x09
#define PS2_F8      0x0A
#define PS2_F6      0x0B
#define PS2_F4      0x0C
#define PS2_TAB     0x0D
#define PS2_GRAVE   0x0E
#define PS2_ALT     0x11
#define PS2_LSHIFT  0x12
#define PS2_CTRL    0x14
#define PS2_CAPS    0x58
#define PS2_RSHIFT  0x59

#define KEY_TRU     0x04    // (CS+3)
#define KEY_PGUP    0x04
#define KEY_INV     0x05    // (CS+4)
#define KEY_PGDN    0x05
#define KEY_CAPS    0x06    // (CS+2)
#define KEY_EDIT    0x07    // (CS+1)
#define KEY_ESC     0x07
#define KEY_LEFT    0x08    // (CS+5)
#define KEY_RIGHT   0x09    // (CS+8)
#define KEY_DOWN    0x0A    // (CS+6)
#define KEY_UP      0x0B    // (CS+7)
#define KEY_BACK    0x0C    // (CS+0)
#define KEY_ENTER   0x0D
#define KEY_EXT     0x0E    // (SS+CS)
#define KEY_TAB     0x0E
#define KEY_GRAPH   0x0F    // (CS+9)
#define KEY_DEL     0x0F
#define KEY_INS     0x10    // (SS+W)
#define KEY_HOME    0x11    // (SS+Q)
#define KEY_END     0x12    // (SS+E)
#define KEY_SSENT   0x13    // (SS+ENTER)
#define KEY_RALT    0x13
#define KEY_SSSP    0x14    // (SS+SPACE)
#define KEY_LALT    0x14
#define KEY_CSENT   0x15    // (CS+ENTER)
#define KEY_BREAK   0x16    // (CS+SPACE)
#define KEY_SPACE   0x20
#define KEY_F1      0xC0
#define KEY_F2      0xC1
#define KEY_F3      0xC2
#define KEY_F4      0xC3
#define KEY_F5      0xC4
#define KEY_F6      0xC5
#define KEY_F7      0xC6
#define KEY_F8      0xC7
#define KEY_F9      0xC8
#define KEY_F10     0xC9
#define KEY_F11     0xCA
#define KEY_POWER   0xCB
#define KEY_SLEEP   0xCD
#define KEY_WAKE    0xCE

void ps2_init();
u8 gcGetKey();
void gcWaitKey(u8 key);
void gcWaitNoKey();
