
; ------- external modules
extern	font8
#include "conf.asm"
#include "macro.asm"


; ------- main code
        rseg CODE
        
		jp MAIN
    
    
; -- IM1 INT
		org h'38
IM1:
		ei
		ret
	
	
; -- NMI        
		org h'66
NMI:
		retn
	
	
; -- reset procedures
		org h'100
MAIN:
        di
        
        xtr
        xt page1, 5
        ld sp, stck
        
        ; call LOAD_NVRAM
        ; call CALC_CRC
        ; jr z, RESET
        
        jr SETUP
        
        xor a
        jr RESET        ; debug!!!
        
        call LOAD_DEFAULTS
        call SAVE_NVRAM
        jr SETUP
        

; This proc makes reset to the specified destination
; input:    A - reset destination
RESET:
        ld hl, RESET2
        ld de, res_buf
        ld bc, RESET2_END - RESET2      ; No checking for stack at this point!!!
        ldir
        jp res_buf
        
        
RESET2:        
        push af
        xtr
        xt page0, 0
        xt page2, 2
        xt page3, 0
        pop af
        
        or a
        jr z, RES_TRD
        ; jr z, RES_48
        halt

        
RES_TRD:
        xtr
        xt memconf, 1
        ld sp, h'3D2E
        
        jp h'3D2F           ; ret to #0000
        
        
RES_48:
        xtr
        xt memconf, 1
        jp 0

        
RES_128:
        xtr
        xt memconf, 0
        jp 0

        
RESET2_END:


; ------- BIOS Setup

SETUP:       
        call CLS
        call LD_FONT
        call LD_S_PAL
        
        xtr
        xt vconf, rres_320x240 | mode_text
        xt vpage, txpage
        xt page3, txpage

        box 0, h'1E50, h'8F
        pmsgc M_HEAD1, 1, h'8E
        pmsgc M_HEAD2, 2, h'8E
        pmsgc M_HLP, 28, h'81
        
        box h'0708, h'0E20, h'8F
        pmsg M_OPT, h'080A, h'8C
        
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
        xtr
        xt page3, txpage ^ 1
        ld hl, font8
        ld de, win3
        ld bc, 2048
        ldir
        ret

        
CLS:    xtr
        xt page3, txpage
        ld hl, win3
        ld de, win3 + 1
        ld bc, h'3FFF
        ld (hl), 0
        ldir
        ret

        
LD_S_PAL:
        xtr
        xt palsel, 15
        xt fmaddr, h'04 | fm_en
        ld hl, pal_tx
        ld de, h'4000 + 15 * 32
        ld bc, 32
        ldir
        xtr
        xt fmaddr, 0
        ret
        

; DE - addr
; H - Y coord
; B - attr
PRINT_MSG_C:
        push de
        ld l, -1
PMC1:
        ld a, (de)
        inc de
        inc l
        or a
        jr nz, PMC1
        
        ld a, l
        and 254
        rrca
        neg
        add a, 40
        ld l, a
        pop de


; DE - addr
; HL - coords        
; B - attr
PRINT_MSG:
        set 6, h
        set 7, h
PM1:
        ld a, (de)
        or a
        ret z
        inc de
        ld (hl), a
        set 7, l
        ld (hl), b
        res 7, l
        inc l
        jr PM1
        
        
; HL - coords
; BC - sizes
; A - attr
DRAW_BOX:
        set 6, h
        set 7, h
        dec b
        dec b
        
        push bc
        push hl
        ld de, 'É' << 8 + 'Í'
        ld b, '»'
        call DB2
        pop hl
        pop bc
        inc h
DB1:
        push bc
        push hl
        ld de, 'º' << 8 + ' '
        ld b, 'º'
        call DB2
        pop hl
        pop bc
        inc h
        djnz DB1

        ld de, 'È' <<8 + 'Í'
        ld b, '¼'
        
DB2:        
        push hl
        push bc
        ld (hl), d
        inc hl
        ld (hl), e
        ld e, l
        ld d, h
        inc e
        dec c
        dec c
        ld b, 0
        ldir
        pop bc
        ld (hl), b
        pop hl
        
        set 7, l
        ld (hl), a
        ld e, l
        ld d, h
        inc e
        dec c
        ld b, 0
        ldir
        ret

;        ÉÍËÍ»
;        º º º
;        ÌÍÎÍ¹
;        º º º
;        ÈÍÊÍ¼

        
#include "arrays.asm"

        end
		
