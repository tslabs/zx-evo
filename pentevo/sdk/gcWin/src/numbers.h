
void gcPrintDec8(u8 num) __naked __z88dk_fastcall
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
    push ix
    ld ix,#ascbuff
    call decasc32
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
    ld	  de,#0         ;Hi
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
    jr divdc16

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

divdc16:
    ld a,#0x2F
    inc a
    or a
    sbc hl,de
    jr nc,divdc16+2
    add hl,de
    ret
  __endasm;
}
