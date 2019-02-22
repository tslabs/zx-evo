//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include "defs.h"

static u16 mouse_x;
static u16 mouse_y;
static u8 mouse_wheel;

void gcMouseInit(u8 gfxpage) __naked
{
    gfxpage;                // to avoid SDCC warning

    __asm
    ld hl,#2
    add hl,sp
    ld a,(hl)
    ld bc,#0x13AF   ;PAGE3
    out (c),a
    ld bc,#0x19AF   ;SGPAGE
    out (c),a

;; prepare sprite
    ld hl,#mouse_img
    ld de,#0xC000
    ld b,#16
0$: push bc
    push de
    ldi
    ldi
    ldi
    ldi
    pop de
    pop bc
    inc d
    djnz 0$

    ld bc,#0xFBDF
    in a,(c)
    ld (mouse_dx),a
    ld bc,#0xFFDF
    in a,(c)
    ld (mouse_dy),a
    ret
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    .even
mouse_desc:
    .ds 6
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
mouse_dx:
    .db 0
mouse_dy:
    .db 0
_mouse_buttons::
    .db 0
_mouse_lmb::
    .db 0
_mouse_rmb::
    .db 0
_mouse_mmb::
    .db 0
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
mouse_img:
    .db 0x80,0x00,0x00,0x00
    .db 0x88,0x00,0x00,0x00
    .db 0x8F,0x80,0x00,0x00
    .db 0x8F,0xF8,0x00,0x00
    .db 0x8F,0xFF,0x80,0x00
    .db 0x8F,0xFF,0xF8,0x00
    .db 0x8F,0xFF,0xFF,0x80
    .db 0x8F,0xFF,0x88,0x80
    .db 0x8F,0x88,0x00,0x00
    .db 0x88,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    .db 0x00,0x00,0x00,0x00
    __endasm;
}

u16 gcGetMouseX(void)
{
    return mouse_x;
}

u16 gcGetMouseY(void)
{
    return mouse_y;
}

u8 gcGetMouseWheel(void)
{
    return mouse_wheel;
}

u8 gcGetMouseXS(void)
{
    return (mouse_x>>2);
}

u8 gcGetMouseYS(void)
{
    return (mouse_y>>3);
}

void gcMouseUpdate(void) __naked
{
    __asm
    ld bc,#0xFADF
    in e,(c)
    ld a,e
    rrca
    rrca
    rrca
    rrca
    and #0x0F
    ld (_mouse_wheel),a
    ld a,e
    and #0x0F
    ld (_mouse_buttons),a
    and #1
    ld (_mouse_lmb),a
    ld a,e
    and #2
    ld (_mouse_rmb),a
    ld a,e
    and #4
    ld (_mouse_mmb),a

    ld bc,#0xFBDF
    ld a,(mouse_dx)
    ld e,a
    in a,(c)
    ld (mouse_dx),a
    sub e
    ld e,a
    rlca
    sbc a,a
    ld d,a
    ld hl,(_mouse_x)
    add hl,de
    ld de,#0xFF00
    sbc hl,de
    add hl,de
    jr c,.+2+3
    ld hl,#0
    ld de,#-320+1
    add hl,de
    sbc hl,de
    jr nc,.+2+3
    ld hl,#320-1
    ld (_mouse_x),hl

    ld bc,#0xFFDF
    ld a,(mouse_dy)
    ld e,a
    in a,(c)
    ld (mouse_dy),a
    sub e
    ld e,a
    xor a
    sub e
    ld e,a
    rlca
    sbc a,a
    ld d,a
    ld hl,(_mouse_y)
    add hl,de
    ld de,#0xFF00
    sbc hl,de
    add hl,de
    jr c,.+2+3
    ld hl,#0
    ld de,#-240+1
    add hl,de
    sbc hl,de
    jr nc,.+2+3
    ld hl,#240-1
    ld (_mouse_y),hl

; build sprite descriptor
    ld de,#mouse_desc
    ld hl,(_mouse_y)
    ld a,l
    ld (de),a
    inc de
    ld a,h
    and #0x01
    or #0x22
    ld (de),a
    inc de
    ld hl,(_mouse_x)
    ld a,l
    ld (de),a
    inc de
    ld a,h
    and #0x01
    ld (de),a
    inc de
    xor a
    ld (de),a
    inc de
    ld a,#0xF0
    ld (de),a

; copy sprite descriptor to SFILE
    ld a,#0x10
    ld bc,#0x15AF       ;FMADDR
    out (c),a
    push bc
    ld hl,#mouse_desc
    ld de,#0x0200
    ld bc,#6
    ldir
    pop bc
    xor a
    out (c),a
    ret
    __endasm;
}
