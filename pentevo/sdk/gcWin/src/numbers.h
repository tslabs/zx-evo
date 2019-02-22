//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
//::                     Window System                       ::
//::               by dr_max^gc (c)2018-2019                 ::
//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

unsigned char* dec2asc8(u8 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

    __asm
    ld h,#0
    ld d,h
    ld e,h
    ld c,h
    push ix
    ld ix,#ascbuff
    call decasc8
    pop ix
    ld hl,#ascbuff
    ret
    __endasm;
}

unsigned char* dec2asc16(u16 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

    __asm
    push hl
    exx
    pop hl
    exx
    ld hl,#0
    ld c,#0
    push ix
    ld ix,#ascbuff
    call decasc16
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

void gcPrintDec32(u32 num) __naked __z88dk_fastcall
{
    num;        // to avoid SDCC warning

  __asm
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; print 32bit decimal number
; i:
;   HLDE = 32bit NUMBER
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
    call DEC32_0
    call decasc32_dig

    call DEC32_1
    call decasc32_dig

    call DEC32_2
    call decasc32_dig

    call DEC32_3
    call decasc32_dig

    call DEC32_4
    call decasc32_dig
;;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
;HL - 16bit NUMBER
;IX - ASCII BUFFER
decasc16:
    call DEC32_5
    call decasc32_dig

    push bc
    exx
    pop bc
    call DEC32_6
    call decasc32_dig

decasc8:
    call DEC32_7
    call decasc32_dig

    call DEC32_8
    call decasc32_dig
    inc c

    ld a,l
    add a,#0x30
    call decasc32_dig
    ld (ix),#0
    ret

decasc32_dig:
    ld b,a
    cp #0x30
    jr z,2$
    inc c
1$: ld a,b
    ld 0 (ix),a
    inc ix
    ret
2$: ld a,c
    or a
    jr nz,1$
    ret

DEC32_0:
    ld de,#0x3B9A       ;Hi
    exx
    ld de,#0xCA00       ;Lo
    exx
    jr divdc32
DEC32_1:
    ld de,#0x05F5       ;Hi
    exx
    ld de,#0xE100       ;Lo
    exx
    jr divdc32
DEC32_2:
    ld de,#0x0098       ;Hi
    exx
    ld de,#0x9680       ;Lo
    exx
    jr divdc32
DEC32_3:
    ld de,#15           ;Hi
    exx
    ld de,#0x4240       ;Lo
    exx
    jr divdc32
DEC32_4:
    ld de,#1            ;Hi
    exx
    ld de,#0x86A0       ;Lo
    exx
    jr divdc32
DEC32_5:
    ld de,#0            ;Hi
    exx
    ld de,#10000        ;Lo
    exx
    jr divdc32
DEC32_6:
    ld de,#1000         ;Lo
    jr divdc16
DEC32_7:
    ld de,#100          ;Lo
    jr divdc16
DEC32_8:
    ld de,#10           ;Lo

divdc16:
    ld a,#0x2F
    inc a
    or a
    sbc hl,de
    jr nc,divdc16+2
    add hl,de
    ret

divdc32:
    ld a,#0x2F
    inc a
    exx
    or a
    sbc hl,de
    exx
    sbc hl,de
    jr nc,divdc32+2
    exx
    add hl,de
    exx
    adc hl,de
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
;   HLDE = 32bit NUMBER
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
    ld 0 (ix),a
    inc ix
    pop af
    or #0xF0
    daa
    add #0xA0
    adc #0x40
    ld 0 (ix),a
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
    ld l,0 (ix)
    ld h,1 (ix)
    inc ix
    inc ix

gc_printf_loop$:
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    inc hl
    cp #0x0A
    jr z,gc_printf_linefeed$
    cp #'%'
    jp nz,gc_printf_char$
    ld a,(hl)
    or a
    jp z,gc_printf_exit$
    inc hl
    cp #'h'
    jp z,gc_printf_short$
    cp #'u'
    jp z,gc_printf_udec16$
    cp #'x'
    jp z,gc_printf_hex16$
    cp #'s'
    jp z,gc_printf_string$
;;
gc_printf_char$:
    ld bc,(sym_attr)
    push hl
    call sym_prn
    ld a,e
    ld (cur_x),a
    pop hl
    jr gc_printf_loop$
;;
gc_printf_linefeed$:
    ld a,(cur_y)
    inc a
    ld (cur_y),a
    ld d,a
    ld a,(win_x)
    ld (cur_x),a
    ld e,a
    jr gc_printf_loop$

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
    ld l,0 (ix)
    ld h,1 (ix)
    inc ix
    inc ix
    call strprnz
    pop hl
    jr gc_printf_loop$

;; %hx
gc_printf_hex8$:
    push hl
    ld l,0 (ix)
    inc ix
    inc ix
    call _gcPrintHex8
    pop hl
    jp gc_printf_loop$

;; %x (hex)
gc_printf_hex16$:
    push hl
    ld l,0 (ix)
    ld h,1 (ix)
    inc ix
    inc ix
    call _gcPrintHex16
    pop hl
    jp gc_printf_loop$

;; %hu
gc_printf_udec8$:
    push hl
    ld l,0 (ix)
    inc ix
    inc ix
    call _gcPrintDec8
    pop hl
    jp gc_printf_loop$

;; %u (insigned decimal)
gc_printf_udec16$:
    push hl
    ld l,0 (ix)
    ld h,1 (ix)
    inc ix
    inc ix
    call _gcPrintDec16
    pop hl
    jp gc_printf_loop$
;;
gc_printf_exit$:
    pop ix
    ret
    __endasm;
}
