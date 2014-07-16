
; This is a  example of Frame and Line maskable interrupt sources
; Run it from TR-DOS:
;   RUN "ex02a" CODE

    device zxspectrum48
	include "tsconfig.asm"

	org $8000

    ld bc, FMADDR
    ld a, FM_EN
    out (c), a      ; open FPGA arrays at #0000
    
    ; clean SFILE
    ld hl, FM_SFILE
    xor a
l1  ld (hl), a
    inc l
    jr nz, l1
l2  ld (hl), a
    inc l
    jr nz, l2
    
    out (c), a      ; close FPGA arrays at #0000
    
	; zeroing scrollers to avoid surprises from previous usage
	ld b, high GXOFFSL : out (c), a
	ld b, high GXOFFSH : out (c), a
	ld b, high GYOFFSL : out (c), a
	ld b, high GYOFFSH : out (c), a
	ld b, high T0XOFFSL : out (c), a
	ld b, high T0XOFFSH : out (c), a
	ld b, high T0YOFFSL : out (c), a
	ld b, high T0YOFFSH : out (c), a
	ld b, high T1XOFFSL : out (c), a
	ld b, high T1XOFFSH : out (c), a
	ld b, high T1YOFFSL : out (c), a
	ld b, high T1YOFFSH : out (c), a
    
	output "example03.bin"
