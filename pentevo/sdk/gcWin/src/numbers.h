//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

unsigned char* dec2asc8(u8 num) __naked __z88dk_fastcall
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

unsigned char* dec2asc8s(s8 num) __naked __z88dk_fastcall
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

unsigned char* dec2asc16(u16 num) __naked __z88dk_fastcall
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

unsigned char* dec2asc16s(s16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

    __asm
    push ix
    ld ix,#ascbuff
    bit 7,h
    jr z,0$
; neg HL
    ld a,l
    neg
    ld l,a
    ld a,#0
    sbc a,h
    ld h,a
;
    ld (ix),#'-'
    inc ix
0$: call decasc16
    pop ix
    ld hl,#ascbuff
    ret
    __endasm;
}

unsigned char* dec2asc32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

    __asm
    push ix
    ld ix,#ascbuff
    call decasc32
    pop ix
    ld hl,#ascbuff
    ret
    __endasm;
}

unsigned char* dec2asc32s(s32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

    __asm
    push ix
    ld ix,#ascbuff
    bit 7,d
    jr z,0$
; neg DE:HL
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
;
    ld (ix),#'-'
    inc ix
0$: call decasc32
    pop ix
    ld hl,#ascbuff
    ret
    __endasm;
}

void gcPrintDec8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    call _dec2asc8
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintDec8s(s8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    call _dec2asc8s
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintDec16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    call _dec2asc16
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintDec16s(s16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    call _dec2asc16s
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintDec32s(s32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
    call _dec2asc32s
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintDec32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; print 32bit decimal number
; i:
;   DEHL = 32bit NUMBER
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    call _dec2asc32
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;DEHL - 32bit NUMBER
;IX - ASCII BUFFER
decasc32:
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
decasc8:
    ld c,#0
    jr decasc8_
decasc16:
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

void gcPrintHex8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; print 8bit hexadecimal number
; i:
;   L = 8bit NUMBER
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    ld a,l
    push ix
    ld ix,#ascbuff
    call hexasc8
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintHex16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; print 16bit hexadecimal number
; i:
;   HL = 16bit NUMBER
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    push ix
    ld ix,#ascbuff
    call hexasc16
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz
  __endasm;
}

void gcPrintHex32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; print 32bit hexadecimal number
; i:
;   DEHL = 32bit NUMBER
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    push ix
    ld ix,#ascbuff
    call hexasc32
    ld (ix),#0
    pop ix
    ld hl,#ascbuff
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
    jp strprnz

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;DEHL - 32bit NUMBER
;IX - ASCII BUFFER
hexasc32:
    ld a,d
    call hexasc8
    ld a,e
    call hexasc8

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;HL - 16bit NUMBER
;IX - ASCII BUFFER
hexasc16:
    ld a,h
    call hexasc8
    ld a,l

;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;A - 8bit NUMBER
;IX - ASCII BUFFER
hexasc8:
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

//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
void gcPrintf(char *string, ...) __naked
{
    string;         // to avoid SDCC warning

    __asm
    push ix
    ld ix,#4
    add ix,sp
;;
    ld a,(cur_x)
    ld e,a
    ld a,(cur_y)
    ld d,a
;;
    ld l,0(ix)
    ld h,1(ix)

gc_printf_loop$:
    inc ix
    inc ix

gc_printf_loop1$:
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    inc hl
    cp #0x0A
    jr z,gc_printf_linefeed$
    cp #'%'
    jr nz,gc_printf_char$
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    inc hl
    cp #'h'
    jr z,gc_printf_short$
    cp #'u'
    jr z,gc_printf_udec16$
    cp #'d'
    jp z,gc_printf_sdec16$
    cp #'x'
    jr z,gc_printf_hex16$
    cp #'s'
    jr z,gc_printf_string$
;;
gc_printf_char$:
    ld bc,(sym_attr)
    push hl
    call sym_prn
    ld a,e
    ld (cur_x),a
    pop hl
    jr gc_printf_loop1$
;;
gc_printf_linefeed$:
    ld a,(cur_y)
    inc a
    ld (cur_y),a
    ld d,a
    ld a,(win_x)
    ld (cur_x),a
    ld e,a
    jr gc_printf_loop1$

;; %h modificator (short)
gc_printf_short$:
    ld a,(hl)
    inc hl
    cp #'u'
    jr z,gc_printf_udec8$
    cp #'x'
    jr z,gc_printf_hex8$
    jr gc_printf_char$

;; %s (string)
gc_printf_string$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call strprnz
    pop hl
    jr gc_printf_loop$

;; %hx
gc_printf_hex8$:
    push hl
    ld l,0(ix)
    call _gcPrintHex8
    pop hl
    jr gc_printf_loop$

;; %x (hex)
gc_printf_hex16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _gcPrintHex16
    pop hl
    jr gc_printf_loop$

;; %hu
gc_printf_udec8$:
    push hl
    ld l,0(ix)
    call _gcPrintDec8
    pop hl
    jp gc_printf_loop$

;; %u (unsigned decimal)
gc_printf_udec16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _gcPrintDec16
    pop hl
    jp gc_printf_loop$

;; %d (signed decimal)
gc_printf_sdec16$:
    push hl
    ld l,0(ix)
    ld h,1(ix)
    call _gcPrintDec16s
    pop hl
    jp gc_printf_loop$

;; exit
gc_printf_exit$:
    pop ix
    ret
    __endasm;
}
