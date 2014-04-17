
; This is a simple example of Frame and Line maskable interrupt sources
; It initializes custom IM2 procedures and uses them in BASIC
;   RANDOMIZE USR 15616
;   LOAD "ex02b" CODE
;   RETURN
;   RANDOMIZE USR 32768

im2vec  equ $BE

    device zxspectrum48
	include "tsconfig.asm"
	; include "tsfuncs.asm"

	org $8000
	; this address is arbitrary, you are free to use any
	; note that example uses window $C000 for banks access, so org addr and stack should be set to lower values
codestart:

    di
    ld hl, line_isr
    ld ((im2vec << 8) | INT_VEC_LINE), hl
    ld hl, frame_isr
    ld ((im2vec << 8) | INT_VEC_FRAME), hl

    ld bc, INTMASK
    ld a, INT_MSK_FRAME | INT_MSK_LINE
    out (c), a

    ld a, im2vec
    ld i, a
    im 2
    ld bc, BORDER
    ei
    ret

line_isr
    push af
li1
    ld a, 0
    out (254), a
    inc a
    and 7
    ld (li1 + 1), a
    pop af
    ei
    ret

frame_isr
    jp 56

codeend:

    SAVETRD "example02.trd", "ex02b.C", codestart, codeend - codestart
