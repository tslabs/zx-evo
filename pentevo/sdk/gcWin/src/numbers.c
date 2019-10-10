//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
#include "defs.h"
#include "gcWin.h"

char *dec2asc8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    ld h,#0
    push ix
    ld ix,#ascbuff
    call decasc8
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc8s(s8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    ld h,#0
    push ix
    ld ix,#ascbuff
    bit 7,l
    jr z,0$
    ld a,l
    neg
    ld l,a
    ld (ix),#'-'
    inc ix
0$: call decasc8
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    call decasc16
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc16s(s16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    bit 7,h
    jr z,0$
    call _neg_hl
    ld (ix),#'-'
    inc ix
0$: call decasc16
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *dec2asc32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    call decasc32
    pop ix
    ld hl,#ascbuff
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
;;
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
char *dec2asc32s(s32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    bit 7,d
    jr z,0$
    call _neg_dehl
    ld (ix),#'-'
    inc ix
0$: call decasc32
    pop ix
    ld hl,#ascbuff
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
    ld ix,#ascbuff
    call hexasc8
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *hex2asc16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    call hexasc16
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
char *hex2asc32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    push ix
    ld ix,#ascbuff
    call hexasc32
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ret

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;DEHL - 32bit NUMBER
;IX - ASCII BUFFER
hexasc32::
    ld c,#0x00
    ld a,d
    call hexasc8_h
    ld a,d
    call hexasc8_l
    ld a,e
    call hexasc8_h
    ld a,e
    call hexasc8_l
    ld a,h
    call hexasc8_h
    ld a,h
    call hexasc8_l
    ld a,l
    jr hexasc8_

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;HL - 16bit NUMBER
;IX - ASCII BUFFER
hexasc16::
    ld c,#0x00
    ld a,h
    call hexasc8_h
    ld a,h
    call hexasc8_l
    ld a,l
    jr hexasc8_

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;A - 8bit NUMBER
;IX - ASCII BUFFER
hexasc8::
    ld c,#0x00
hexasc8_:
    push af
    call hexasc8_h
    pop af
    or #0xF0
    daa
    add #0xA0
    adc #0x40
    ld 0(ix),a
    inc ix
    ret
;;
hexasc8_h:
    rra
    rra
    rra
    rra
    or #0xF0
    daa
    add #0xA0
    adc #0x40
    jr hexasc_put
;;
hexasc8_l:
    or #0xF0
    daa
    add #0xA0
    adc #0x40
;;
hexasc_put:
    cp #0x30
    jr z,0$
    inc c
0$: inc c
    dec c
    ret z
    ld 0(ix),a
    inc ix
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec8(u8 num) __z88dk_fastcall
{
    gcPrintString(dec2asc8(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec8s(s8 num) __z88dk_fastcall
{
    gcPrintString(dec2asc8s(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec16(u16 num) __z88dk_fastcall
{
    gcPrintString(dec2asc16(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec16s(s16 num) __z88dk_fastcall
{
    gcPrintString(dec2asc16s(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec32s(s32 num) __z88dk_fastcall
{
    gcPrintString(dec2asc32s(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintDec32(u32 num) __z88dk_fastcall
{
    gcPrintString(dec2asc32(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintHex8(u8 num) __z88dk_fastcall
{
    gcPrintString(hex2asc8(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintHex16(u16 num) __z88dk_fastcall
{
    gcPrintString(hex2asc16(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintHex32(u32 num) __z88dk_fastcall
{
    gcPrintString(hex2asc32(num));
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
s32 a2d32s(char *string) __naked __z88dk_fastcall
{
    string;     // to avoid SDCC warning

  __asm
    ld a,(hl)
    cp #'-'
    jr nz,0$
    inc hl
0$: ex af,af
    call _a2d32
    ex af,af
    ret nz
    jp _neg_dehl
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u32 a2d32(char *string) __naked __z88dk_fastcall
{
    string;     // to avoid SDCC warning

  __asm
    push hl
    pop ix
;
    ld l,#0
    ld h,l
    ld e,l
    ld d,l
;
0$: ld a,(ix)
    sub #0x30
    cp #10
    ret nc
    inc ix
    call _mul10
    ld c,#0
    add a,l
    ld l,a
    ld a,h
    adc a,c
    ld h,a
    ld a,e
    adc a,c
    ld e,a
    ld a,d
    adc a,c
    ld d,a
    jr 0$
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
u32 mul10(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    add hl,hl
    rl e
    rl d
    push de
    push hl
    add hl,hl
    rl e
    rl d
    add hl,hl
    rl e
    rl d
    pop bc
    add hl,bc
    ex de,hl
    pop bc
    adc hl,bc
    ex de,hl
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
s16 neg_hl(s16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    ld a,l
    neg
    ld l,a
    ld a,#0
    sbc a,h
    ld h,a
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
s32 neg_dehl(s32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    ld a,l
    neg
    ld l,a
    ld a,#0
    sbc a,h
    ld h,a
    ld a,#0
    sbc a,e
    ld e,a
    ld a,#0
    sbc a,d
    ld d,a
    ret
  __endasm;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintf(char *string, ...) __naked
{
    string;         // to avoid SDCC warning

  __asm
    push ix
    ld ix,#4
    add ix,sp
;;
    ld l,0(ix)
    ld h,1(ix)

gc_printf_loop$:
    xor a
    ld (printf_cnt),a
    ld a,#0x20
    ld (printf_fil),a
    inc ix
    inc ix

gc_printf_loop1$:
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    inc hl
    cp #'%'
    jr nz,gc_printf_char$
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    cp #0x3A
    jr nc,0$
    cp #0x30
    jr c,gc_printf_char$
;;
    jr nz,1$
    ld a,#0x30
    ld (printf_fil),a

1$: push ix
    call _a2d32
    ld a,l
    ld (printf_cnt),a
; !!! IX points to not number symbol in ascii buffer
    push ix
    pop hl
    pop ix
    ld a,(hl)
;;
0$: inc hl
    cp #'l'
    jp z,gc_printf_long$
    cp #'u'
    jp z,gc_printf_udec16$
    cp #'d'
    jp z,gc_printf_sdec16$
    cp #'x'
    jp z,gc_printf_hex16$
    cp #'X'
    jp z,gc_printf_hex16$
    cp #'s'
    jp z,gc_printf_string$
    cp #'m'
    jr z,gc_printf_array$
    cp #'c'
    jp nz,gc_printf_char$
    ld a,0(ix)
;;
gc_printf_char$:
    push hl
    ld l,a
    call _putsym
    pop hl
    jr gc_printf_loop1$

;; %l modificator (long)
gc_printf_long$:
    ld a,(hl)
    inc hl
    cp #'u'
    jp z,gc_printf_udec32$
    cp #'d'
    jp z,gc_printf_sdec32$
    cp #'x'
    jp z,gc_printf_hex32$
    cp #'X'
    jp z,gc_printf_hex32$
    jr gc_printf_char$

;; %s (string)
gc_printf_string$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _gcPrintString
    pop hl
    jp gc_printf_loop$

;; %m (array)
gc_printf_array$:
    ld a,(hl)
    inc hl
    cp #'x'
    jr z,gc_printf_array_x$
    jr gc_printf_char$

gc_printf_array_x$:
    ld a,(hl)
    inc hl
    ld (printf_xmod),a
    cp #'b'
    jr z,gc_printf_array_xb$
    cp #'w'
    jr z,gc_printf_array_xb$
    cp #'d'
    jr z,gc_printf_array_xb$
    jr gc_printf_char$

printf_xmod:
    .db 0

gc_printf_array_xb$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    inc ix
    inc ix
    ld c,0(ix)
    ld b,1(ix)
    push ix

    push hl
    pop ix

1$: ld a,b
    or c
    jr z,0$
    push bc
    ld de,#0x0000
    ld h,e
    ld l,0(ix)
    inc ix
    ld a,(printf_xmod)
    cp #'b'
    jr z,2$
    ld h,0(ix)
    inc ix
    cp #'w'
    jr z,2$
    ld e,0(ix)
    inc ix
    ld d,0(ix)
    inc ix
2$: push ix
    call _hex2asc32
    call gc_printf_buff$
    ld l,#0x20
    call _putsym
    pop ix
    pop bc
    dec bc
    jr 1$
0$: pop ix
    pop hl
    jp gc_printf_loop$

;; %x (hex)
gc_printf_hex16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _hex2asc16
    call gc_printf_buff$
    pop hl
    jp gc_printf_loop$

;; %lx (hex)
gc_printf_hex32$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    inc ix
    inc ix
    ld e,0(ix)
    ld d,1(ix)
    call _hex2asc32
    call gc_printf_buff$
    pop hl
    jp gc_printf_loop$

;; %u (unsigned decimal)
gc_printf_udec16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _dec2asc16
    call gc_printf_buff$
    pop hl
    jp gc_printf_loop$

;; %d (signed decimal)
gc_printf_sdec16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    bit 7,h
    call nz,_neg_hl
    call _dec2asc16
    jr gc_printf_buff_s$

;; %lu
gc_printf_udec32$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    inc ix
    inc ix
    ld e,0(ix)
    ld d,1(ix)
    call _dec2asc32
    call gc_printf_buff$
    pop hl
    jp gc_printf_loop$

;; %ld
gc_printf_sdec32$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    inc ix
    inc ix
    ld e,0(ix)
    ld d,1(ix)
    bit 7,d
    call nz,_neg_dehl
    call _dec2asc32
;;
gc_printf_buff_s$:
    push hl
    call strlen
    ld a,(printf_fil)
    cp #0x20
    jr z,0$
    bit 7,1(ix)
    call nz,gc_printf_minus$
    ld a,(printf_cnt)
    sub b
    call nc,gc_printf_spacer$
    jr 1$
0$: ld a,(printf_cnt)
    bit 7,1(ix)
    jr z,.+2+1
    inc b
    sub b
    call nc,gc_printf_spacer$
    bit 7,1(ix)
    call nz,gc_printf_minus$
1$: pop hl
    call _gcPrintString
    pop hl
    jp gc_printf_loop$
;;
gc_printf_minus$:
    inc b
    push hl
    push bc
    ld a,#'-'
    ld l,a
    call _putsym
    pop bc
    pop hl
    ret
;;
gc_printf_buff$:
    push hl
    call strlen
    ld a,(printf_cnt)
    sub b
    call nc,gc_printf_spacer$
    pop hl
    jp _gcPrintString
;;
gc_printf_spacer$:
    ret z
    ld b,a
0$: push bc
    ld a,(printf_fil)
    ld l,a
    call _putsym
    pop bc
    djnz 0$
    ret

;; exit
gc_printf_exit$:
    pop ix
    ret
;;
printf_cnt:
    .db 0
printf_fil:
    .db 0
  __endasm;
}
