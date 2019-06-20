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

typedef enum KEY_t
{
    KEY_TRU     = (u8)0x04,    // (CS+3)
    KEY_PGUP    = (u8)0x04,
    KEY_INV     = (u8)0x05,    // (CS+4)
    KEY_PGDN    = (u8)0x05,
    KEY_CAPS    = (u8)0x06,    // (CS+2)
    KEY_EDIT    = (u8)0x07,    // (CS+1)
    KEY_ESC     = (u8)0x07,
    KEY_LEFT    = (u8)0x08,    // (CS+5)
    KEY_RIGHT   = (u8)0x09,    // (CS+8)
    KEY_DOWN    = (u8)0x0A,    // (CS+6)
    KEY_UP      = (u8)0x0B,    // (CS+7)
    KEY_BACK    = (u8)0x0C,    // (CS+0)
    KEY_ENTER   = (u8)0x0D,
    KEY_EXT     = (u8)0x0E,    // (SS+CS)
    KEY_TAB     = (u8)0x0E,
    KEY_GRAPH   = (u8)0x0F,    // (CS+9)
    KEY_DEL     = (u8)0x0F,
    KEY_INS     = (u8)0x10,    // (SS+W)
    KEY_HOME    = (u8)0x11,    // (SS+Q)
    KEY_END     = (u8)0x12,    // (SS+E)
    KEY_SSENT   = (u8)0x13,    // (SS+ENTER)
    KEY_RALT    = (u8)0x13,
    KEY_SSSP    = (u8)0x14,    // (SS+SPACE)
    KEY_LALT    = (u8)0x14,
    KEY_CSENT   = (u8)0x15,    // (CS+ENTER)
    KEY_BREAK   = (u8)0x16,    // (CS+SPACE)
    KEY_SPACE   = (u8)0x20,
    KEY_F1      = (u8)0xC0,
    KEY_F2      = (u8)0xC1,
    KEY_F3      = (u8)0xC2,
    KEY_F4      = (u8)0xC3,
    KEY_F5      = (u8)0xC4,
    KEY_F6      = (u8)0xC5,
    KEY_F7      = (u8)0xC6,
    KEY_F8      = (u8)0xC7,
    KEY_F9      = (u8)0xC8,
    KEY_F10     = (u8)0xC9,
    KEY_F11     = (u8)0xCA,
    KEY_POWER   = (u8)0xCB,
    KEY_SLEEP   = (u8)0xCD,
    KEY_WAKE    = (u8)0xCE
} KEY_t;

void ps2_init();
KEY_t gcGetKey();
void gcWaitKey(KEY_t key);
void gcWaitNoKey();
