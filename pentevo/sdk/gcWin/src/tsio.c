#include "defs.h"

#define PUTCHAR_PROTOTYPE void putchar (char c)

void strprn_xy(u16 *str, u8 x, u8 y, u8 attr) __naked
{
str;    // to avoid SDCC warning
x;      // to avoid SDCC warning
y;      // to avoid SDCC warning
attr;   // to avoid SDCC warning
    __asm
    ld hl, #2
    add hl,sp
    ld e,(hl)
    inc hl
    ld d,(hl)   ; DE - string address
    inc hl
    ld c,(hl)   ; C - x
    inc hl
    ld a,(hl)   ; A - y
    inc hl
    ld b,(hl)   ; B - symbol attribute
    or #0xC0
    ld h,a
    ld l,c      ; HL - screen address

lp:
    ld a,(de)
    or a
    ret z
    ld (hl),a
    set 7,l
    ld (hl),b
    res 7,l
    inc l
    inc de
    jr lp
    __endasm;
}


PUTCHAR_PROTOTYPE
{
c;      //to avoid SDCC warning
    __asm
    ld hl,#2
    add hl,sp
    ld b,(hl)   ; B - charcode

    ld a,(#_pcst)
    or a
    jr z,$na
    xor a
    ld (#_pcst),a
    ld a,b
    sub #0x30
    ld (#_pca),a
    ret

$na:
    ld a,b
    cp #0x0A        ;\r
    jr nz,$nr
    ld a,(#_pcx0)
    ld (#_pcx),a
    ret
$nr:
    cp #0x0D        ;\n
    jr nz,$nn
    ld a,(#_pcy)
    inc a
    cp #30
    jr c,$nr0
    xor a
$nr0:
    ld (#_pcy),a
    ret
$nn:
    cp #0x07        ;\a
    jr nz,$n7
    ld a,#0x01
    ld (#_pcst),a
    ret

$n7:
    ld a,(#_pcy)
    or #0xC0
    ld h,a
    ld a,(#_pcx)
    ld l,a

    ld (hl),b
    set 7,l
    ld a,(#_pca)
    ld (hl),a

    ld hl,#_pcx
    inc (hl)
    ret
    __endasm;
}
