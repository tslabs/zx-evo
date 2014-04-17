
; This is a simple example of Frame and Line maskable interrupt sources
; Run it from TR-DOS:
;   RUN "ex02a" CODE

im2vec  equ $BE

    device zxspectrum48
	include "tsconfig.asm"

	org $8000
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
    jr $

line_isr
    out (c), a
    inc a
    ei
    ret

frame_isr
    ld a, 224
    ei
    ret

codeend:

	;output "example02.bin"
    ;savesna "example02.sna",#8000
    EMPTYTRD "example02.trd"
    SAVETRD "example02.trd", "ex02a.C", codestart, codeend - codestart
