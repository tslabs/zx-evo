#include "defs.h"

char ascbuff[32];

char *dec2asc8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    ld h,#0
    push ix
    ld ix,#_ascbuff
    call decasc8
    pop ix
    ld hl,#_ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#_ascbuff
    call decasc16
    pop ix
    ld hl,#_ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#_ascbuff
    call decasc32
    pop ix
    ld hl,#_ascbuff
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;DEHL - 32bit NUMBER
;IX - ASCII BUFFER
decasc32::
    push de
    exx
    pop hl
    exx

    ld c,#0

;; 10^9
    exx
    ld de,#0x3B9A       ;Hi
    exx
    ld de,#0xCA00       ;Lo
    call decasc32_dig

;; 10^8
    exx
    ld de,#0x05F5       ;Hi
    exx
    ld de,#0xE100       ;Lo
    call decasc32_dig

;; 10^7
    exx
    ld de,#0x0098       ;Hi
    exx
    ld de,#0x9680       ;Lo
    call decasc32_dig

;; 10^6
    exx
    ld de,#15           ;Hi
    exx
    ld de,#0x4240       ;Lo
    call decasc32_dig

;; 10^5
    exx
    ld de,#1            ;Hi
    exx
    ld de,#0x86A0       ;Lo
    call decasc32_dig

decasc16_:
;; 10^4
    exx
    ld de,#0            ;Hi
    exx
    ld de,#10000        ;Lo
    call decasc32_dig

;; 10^3
    ld de,#1000         ;Lo
    call decasc32_dig

decasc8_:
;; 10^2
    ld de,#100          ;Lo
    call decasc16_dig

;; 10^1
    ld de,#10           ;Lo
    call decasc16_dig

    ld a,l
    or #0x30
    ld 0(ix),a
    ld 1(ix),#0
    ret
;;
decasc8::
    ld c,#0
    jr decasc8_
decasc16::
    ld c,#0
    exx
    ld hl,#0
    exx
    jr decasc16_
;;
decasc16_dig:
    ld a,#0x30
    or a
0$: sbc hl,de
    jr c,1$
    inc a
    inc c
    jr 0$
1$: add hl,de
    inc c
    dec c
    ret z
    ld 0(ix),a
    inc ix
    ret

decasc32_dig:
    ld a,#0x30
    or a
0$: sbc hl,de
    exx
    sbc hl,de
    exx
    jr c,1$
    inc a
    inc c
    jr 0$
1$: add hl,de
    exx
    adc hl,de
    exx
    inc c
    dec c
    ret z
    ld 0(ix),a
    inc ix
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *hex2asc8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld a,l
    ld ix,#_ascbuff
    call hexasc8
    ld (ix),#0
    pop ix
    ld hl,#_ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *hex2asc16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#_ascbuff
    call hexasc16
    ld (ix),#0
    pop ix
    ld hl,#_ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *hex2asc32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#_ascbuff
    call hexasc32
    ld (ix),#0
    pop ix
    ld hl,#_ascbuff
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;DEHL - 32bit NUMBER
;IX - ASCII BUFFER
hexasc32::
    ld a,d
    call hexasc8
    ld a,e
    call hexasc8

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;HL - 16bit NUMBER
;IX - ASCII BUFFER
hexasc16::
    ld a,h
    call hexasc8
    ld a,l

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;A - 8bit NUMBER
;IX - ASCII BUFFER
hexasc8::
    push af
    rra
    rra
    rra
    rra
    or #0xF0
    daa
    add #0xA0
    adc #0x40
    ld 0(ix),a
    inc ix
    pop af
    or #0xF0
    daa
    add #0xA0
    adc #0x40
    ld 0(ix),a
    inc ix
    ret
  __endasm;
}
