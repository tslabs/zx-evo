
; ------- external modules
extern    font8
#include "conf.asm"
#include "macro.asm"
#include "vars.asm"


; ------- main code
        rseg CODE
        jp MAIN
    
    
; -- IM1 INT
        org h'38
IM1     
        push bc
        push de
        push hl
        push af
        call KBD_PROC
        ld a, (evt)
        or a            ; was last event processed?
        jr nz, IM11     ; not yet - return
        call KBD_EVT
IM11
        ld a, ev_kb_down
        ld (evt), a
        pop af
        pop hl
        pop de
        pop bc
        ei
        ret
    
    
; -- NMI        
        org h'66
NMI
        retn
    
    
; -- reset procedures
        org h'100
MAIN
        di
        
        xtr
        xt page1, 5
        ld sp, stck
        im 1
        xor a
        ld i, a
        
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
RESET
        ld hl, RESET2
        ld de, res_buf
        ld bc, RESET2_END - RESET2      ; No check for stack violation at this point!!!
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

        
RES_TRD
        xtr
        xt memconf, 1
        ld sp, h'3D2E
        
        jp h'3D2F           ; ret to #0000
        
        
RES_48
        xtr
        xt memconf, 1
        jp 0

        
RES_128
        xtr
        xt memconf, 0
        jp 0

        
RESET2_END


; ------- BIOS Setup

SETUP:       
        call S_INIT
        call CLS
        call LD_FONT
        call LD_S_PAL
        
        xtr
        xt vconf, rres_320x240 | mode_text
        xt vpage, txpage
        xt page3, txpage

        box 0, h'1E50, h'8F
        pmsgc M_HEAD1, 1, h'8E
        
        ld de, M_HEAD2
        ld h, 2
        ld b, h'8E
        ld a, MH2_SZ
        call PMC2
        chr ' '
        num date 4      ; day (1-31)
        chr '.'
        num date 5      ; month (1-12)
        chr '.'
        chr '2'
        chr '0'
        num date 6      ; year (0-99)
        chr ' '
        num date 3      ; hour (0-23)
        chr ':'
        num date 2      ; minute (0-59)
        chr ':'
        num date 1      ; second (0-59)
        
        pmsgc M_HLP, 28, h'87
        call BOX0
        
        
; Main cycle
        ei
; Event handler        
S_MAIN        
        ld a, (evt)
        or a
        jr z, S_MAIN
        call EVT_PROC
        xor a
        ld (evt), a
        jr S_MAIN
        

; ------- subroutines

; Setup init
S_INIT
        xor a
        ld (evt), a
        ld (fld_curr), a
        ld (fld0_pos), a
        dec a
        ld (last_key), a
        ret
        

; Event processing
EVT_PROC
        ld a, (fld_curr)
        or a
        jr z, EVT_P0
        ret

        
EVT_P0        
        ld a, (fld_max)
        dec a
        ld b, a
        ld a, (fld0_pos)
        ld c, a
        ld a, (evt)
        cp ev_kb_up
        jr z, EV0_U
        cp ev_kb_down
        jr z, EV0_D
        cp ev_kb_enter
        ; jr z, EV0_E
        ret

EV0_U
        ld a, c
        or a
        ret z
        call OPT_DH
        dec a
EVO0_1
        ld (fld0_pos), a
        jr OPT_HG

        
EV0_D
        ld a, c
        cp b
        ret nc
        call OPT_DH
        inc a
        jr EVO0_1
        
OPT_DH
        ld b, opt_norm      ; change to var!
        jr OPT_1
        
OPT_HG       
        ld b, opt_hgl       ; change to var!
OPT_1
        push af
        ld c, a
        ld de, (fld_top)
        add a, d
        ld d, a             ; Y coord
        ld hl, (fld_tab)
        ld a, c
        add a, a
        add a, a
        add a, l
        ld l, a
        adc a, h
        sub l
        ld h, a
        ld c, (hl)
        inc hl
        ld h, (hl)
        ld l, c
        inc hl
        inc hl
        ex de, hl
        call PRINT_MSG
        pop af
        ret

        
BOX0
        box h'0707, h'0E20, h'8F
        ld hl, OPTTAB0
        ld a, (hl)      ; X coord of list top
        inc hl
        ld (fld_top), a
        ld a, (hl)      ; Y coord of list top
        inc hl
        ld (fld_top + 1), a
        ld e, (hl)      ; X coord of header text
        inc hl
        ld d, (hl)      ; Y coord of header text
        inc hl
        ld b, (hl)      ; attrs
        inc hl
        ld a, (hl)      ; number of options
        ld (fld_max), a
        ld c, a
        inc hl
        ex de, hl
        call PRINT_MSG
        ld (fld_tab), de
        
        ld a, (fld0_pos)
        ld b, a
        xor a
BX01
        cp b
        push bc
        call z, OPT_HG
        call nz, OPT_DH
        pop bc
        inc a
        cp c
        jr c, BX01
        ret
        
                
; KBD event processing
; check pressed key and set event to process
KBD_EVT
        ld a, (key)
        cp 13       ; enter
        ld a, ev_kb_enter
        ret z
        cp 11       ; up
        ld a, ev_kb_up
        ret z
        cp 10       ; down
        ld a, ev_kb_down
        ret z
        xor a
        ret
        
        
; KBD procedure
; makes keyboard mapping and autorepeat
KBD_PROC
        call KBD_POLL
        jr nc, KPC1     ; no key pressed
        ld c, a
        ld a, (last_key)
        cp c            ; is key the same as last time?
        jr z, KPC2      ; yes - autorepeat

        ld a, 15        ; initial autorepeat value
        ld (kbd_del), a
        ld a, c 
        ld (last_key), a
        ld b, 0
        ld hl, keys_caps
        bit 0, l
        jr nz, KPC4     ; CS map
        ld hl, keys_symb
        bit 1, l
        jr nz, KPC4     ; SS map
        ld hl, keys_norm
KPC4
        add hl, bc      ; map to layout
        ld a, (hl)
        ld (map_key), a
        jr KPC3
        
KPC2
        ld a, (kbd_del)
        dec a
        ld (kbd_del), a
        jr nz, KPC5     ; autorepeat in progress
        ld a, 2         ; autorepeat value
        ld (kbd_del), a
        ld a, (map_key)
        jr KPC3
KPC1
        ld a, 255
        ld (last_key), a
KPC5
        xor a
KPC3
        ld (key), a
        ret


; KBD poll
; out: 
;   A - key pressed (0-39), CS+SS = 36
;   L - bit0 = CS, bit1 = SS,
;   fc - c = pressed / nc = not pressed (A = 40)
KBD_POLL
        ld bc, h'FEFE
        xor a
        ld l, a
KBP1
        in d, (c)
        ld e, 5
KBP2
        rr d
        jr nc, KBP3     ; some key pressed
KBP4
        inc a
        dec e
        jr nz, KBP2
        rlc b
        jr c, KBP1
        ret             ; no key pressed
KBP3
        or a
        jr z, KBP5      ; CS pressed
        cp 36
        jr z, KBP6      ; SS pressed
        scf             ; some other key pressed
        ret
KBP5
        set 0, l
        jr KBP4
KBP6
        set 1, l
        bit 0, l        ; was CS pressed before?
        jr z, KBP4      ; no - go back to cycle
        scf             ; yes - CS+SS pressed
        ret
        

CALC_CRC
        ld hl, nv_buf
        ld de, 0
        ld b, 62
CC0
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

        
LOAD_DEFAULTS
        ld hl, nv_buf
        ld de, nv_buf + 1
        ld bc, h'003D
        ld (hl), b
        ldir
        ret
        
        
LOAD_NVRAM
        ret
        
        
SAVE_NVRAM
        call CALC_CRC
        ld (nv_buf + 62), de
        ret
        
        
LD_FONT
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

        
LD_S_PAL
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
PRINT_MSG_C
        push de
        ld l, -1
PMC1
        ld a, (de)
        inc de
        inc l
        or a
        jr nz, PMC1
        pop de
        
        ld a, l
PMC2
        rrca
        and 127
        neg
        add a, 40
        ld l, a


; DE - addr
; HL - coords        
; B - attr
PRINT_MSG
        set 6, h
        set 7, h
PM1
        ld a, (de)
        inc de
        or a
        ret z
        call SYM
        jr PM1
        
        
; HL - coords
; BC - sizes
; A - attr
DRAW_BOX
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
DB1
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


NUM_10
        ld e, 0
N102
        sub 10
        jr c, N101
        inc e
        jr N102
N101
        ld d, a
        ld a, e
        add a, '0'
        call SYM
        
        ld a, d
        add a, '0' + 10
SYM
        ld (hl), a
        set 7, l
        ld (hl), b
        res 7, l
        inc l
        ret
        

; #include "booter.asm"
#include "arrays.asm"

        end
        
