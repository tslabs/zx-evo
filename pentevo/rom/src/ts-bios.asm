
; ------- external modules
extern	font8
#include "conf.asm"
#include "macro.asm"


; ------- main code
        rseg CODE
        xtr
        
		jp MAIN
    
		org h'38
IM1:
		ei
		ret
		
		org h'66
NMI:
		retn
		
		org h'100
MAIN:
        di
        
l1:     inc a
        out (254),a        
        jr l1
        
        xt page1, h'05
        ld sp, stck
        
        call LOAD_NVRAM
        call CALC_CRC
        jr z, RESET
        
        call LOAD_DEFAULTS
        call SAVE_NVRAM
        jr SETUP
        
RESET:  
        
SETUP:       
        call CLS
        call LD_FONT
        call LD_S_PAL

        halt

        
; ------- subroutines
CALC_CRC:

        ld hl, nv_buf
        ld de, 0
        ld b, 62
CC0:
        ld a, d
        add a, (hl)
        ld d, a
        ld a, e
        xor (hl)
        ld e, a
        inc hl
        djnz CC0
        
        ld a, d
        cp (hl)
        ret nz
        inc hl
        ld a, e
        cp (hl)
        ret

LOAD_DEFAULTS:
        ld hl, nv_buf
        ld de, nv_buf + 1
        ld bc, h'003D
        ld (hl), b
        ldir
        ret
        
LOAD_NVRAM:
        ret
        
SAVE_NVRAM:
        call CALC_CRC
        ld (nv_buf + 62), de
        ret
        
LD_FONT:
        xt page3, h'FE
        ld hl, font8
        ld de, win3
        ld bc, h'4000
        ldir
        ret

CLS:        
        xt page3, h'FF
        ld hl, win3
        ld de, win3 + 1
        ld bc, h'3FFF
        ld (hl), 0
        ldir
        ret

LD_S_PAL:        
        
	
#include "arrays.asm"

        end
		
