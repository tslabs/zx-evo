
; ------- external modules
extern    font8
#include "conf.asm"
#include "macro.asm"
#include "vars.asm"


; ------- main code
        rseg CODE
        
        di
        jp MAIN
    
    
; -- reset procedures
        org h'80
MAIN
        di
        
        xtr
        xt page1, 5
        ld sp, stck
        im 1
        xor a
        ld i, a
        
        call READ_NVRAM
        call CALC_CRC
        push af
        call nz, LOAD_DEFAULTS
        pop af
        jr nz, SETUP        ; CRC error
        
        ld bc, h'FEFE
        in a, (c)
        rrca
        jr nc, SETUP        ; CS pressed
        

RESET
        ld hl, RESET2
        ld de, res_buf
        ld bc, RESET2_END - RESET2
        ldir
        jp res_buf
        
        
RESET2:        
        push af
        xtr
        xt page0, 0
        xt page2, 2
        xt page3, 0
        pop af
        
        ld bc, h'7FFE
        in a, (c)
        rrca
        rrca
        jr nc, RES_TRD      ; SS pressed
        
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
        ld a, (fld0_pos)
        call OPT_HG
        
; Main cycle
        ei
; Event handler        
S_MAIN        
        ld a, (evt)
        or a
        jr z, S_MAIN
        
        ld c, a             ; debug!!!
        ld a, 2
        out (254), a
        ld a, c
        call EVT_PROC
        xor a
        out (254), a
        ld (evt), a
        jr S_MAIN
        

; ------- subroutines

; Setup init
S_INIT
        xor a
        ld (evt), a
        ld (fld_curr), a
        ld (fld0_pos), a
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
        jr z, EV0_E
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
        
        
EV0_E
        ld a, (opt_nvr)
        ld e, a
        ld d, high(nv_buf)
        ld a, (de)
        inc a
        ld (de), a
        ld a, c
        jr OPT_HG
        
OPT_DH
        ld b, opt_norm      ; change to var!
        jr OPT_1
        
OPT_HG       
        ld b, opt_hgl       ; change to var!
OPT_1
        push af
        call OPT_DEC
        ld hl, (fld_top)
        add a, h
        ld h, a             ; Y coord
        call PRINT_MSG
        
        ld a, (sel_max)
        ld c, a
        ld a, (opt_nvr)
        ld e, a
        ld d, high(nv_buf)
        ld a, (de)
        cp c                ; is the value larger than max?
        jr c, OPP2          ; no - ok
        xor a               ; yes - zero it
        ld (de), a
OPP2
        push af
        ld de, (fld_sel)
        call MSG_LEN
        ld c, a
        neg
        add a, 36
        ld l, a
        pop af
        
        ex de, hl
        ld b, 0
        inc c
OPP3
        sub 1
        jr c, OPP4
        add hl, bc
        jr OPP3
OPP4
        ex de, hl
        ld b, sel_norm       ; change to var!
        call PRINT_MSG
        pop af
        ret

        
OPT_DEC
; in:
;       
        push af
        ld hl, (fld_tab)
        add a, a
        add a, a
        add a, l
        ld l, a
        adc a, h
        sub l
        ld h, a             ; HL = address of option desc pointer
        
        ld e, (hl)
        inc hl
        ld d, (hl)
        inc hl
        ld (fld_opt), de
        ld a, (de)          ; number of choises
        inc de
        ld (sel_max), a
        ld a, (de)          ; address in NVRAM
        inc de
        ld (opt_nvr), a
        push de
        ld e, (hl)
        inc hl
        ld d, (hl)
        ld (fld_sel), de
        pop de
        pop af
        ret
        

BOX0        
        ld hl, OPTTAB0
BOX
        ld e, (hl)      ; X coord of box
        inc hl
        ld d, (hl)      ; Y coord of box
        inc hl
        ld c, (hl)      ; X size of box
        inc hl
        ld b, (hl)      ; Y size of box
        inc hl
        push hl
        ex de, hl
        ld a, box_norm  ; change to var!
        call DRAW_BOX
        pop hl
        
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
        push bc
        call OPT_DH
        pop bc
        inc a
        cp c
        jr c, BX01
        ret
        
                
; KBD event processing
; check pressed key and set event to process
KBD_EVT
        ld a, (key)
        cp 11       ; up
        ld b, ev_kb_up
        ret z
        cp 10       ; down
        ld b, ev_kb_down
        ret z
        cp 13       ; enter
        ld b, ev_kb_enter
        ret z
        cp 14       ; help
        ld b, ev_kb_help
        ret z
        ld b, 0
        ret
        
        
; KBD procedure
; makes keyboard mapping and autorepeat
KBD_PROC
        call KBD_POLL
        jr nc, KPC1     ; no key pressed
        ld c, a
        ld b, 0
        ld hl, keys_caps
        bit 0, e
        jr nz, KPC4     ; CS map
        ld hl, keys_symb
        bit 1, e
        jr nz, KPC4     ; SS map
        ld hl, keys_norm
KPC4
        add hl, bc      ; map to layout
        ld c, (hl)
        ld a, (last_key)
        cp c            ; is key the same as last time?
        jr z, KPC2      ; yes - autorepeat

        ld a, 15        ; initial autorepeat value
        ld (kbd_del), a
        ld a, c 
        ld (last_key), a
        jr KPC3
        
KPC2
        ld a, (kbd_del)
        dec a
        ld (kbd_del), a
        jr nz, KPC5     ; autorepeat in progress
        ld a, 2         ; autorepeat value
        ld (kbd_del), a
        ld a, c
        jr KPC3
KPC1
        xor a
        ld (last_key), a
KPC5
        xor a
KPC3
        ld (key), a
        ret


; KBD poll
; out: 
;   A - key pressed (0-39), CS+SS = 36
;   E - bit0 = CS, bit1 = SS,
;   fc - c = pressed / nc = not pressed (A = 40)
KBD_POLL
        ld bc, h'FEFE
        xor a
        ld e, a
KBP1
        in h, (c)
        ld l, 5
KBP2
        rr h
        jr nc, KBP3     ; some key pressed
KBP4
        inc a
        dec l
        jr nz, KBP2
        rlc b
        jr c, KBP1
        ret             ; no key pressed
KBP3
        or a
        jr z, KBP5      ; CS pressed
        cp 36
        jr z, KBP6      ; SS pressed
                        ; some other key pressed
        ld b, h'7F      ; additional check for SS - it could be not polled yet
        in c, (c)
        bit 1, c
        jr nz, KBP7     ; no SS
        set 1, e        ; SS pressed
KBP7
        scf
        ret
KBP5
        set 0, e
        jr KBP4
KBP6
        set 1, e
        bit 0, e        ; was CS pressed before?
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

        
READ_NVRAM
        ret
        
        
LOAD_DEFAULTS
        ld hl, high(nv_buf) << 8 + nv_1st
        ld d, h
        ld e, l
        inc de
        ld bc, nv_size - 1
        ld (hl), b
        ldir
        
        
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
        xt palsel, pal_sel
        xt fmaddr, h'04 | fm_en
        ld hl, pal_tx
        ld de, pal_addr + pal_sel * 32
        ld bc, 32
        ldir
        xtr
        xt fmaddr, 0
        ret
        

; Calculate length of message
; in:
;   DE - addr
; out:
;   A - length
MSG_LEN
        push de
        ld l, -1
MLN1
        ld a, (de)
        inc de
        inc l
        or a
        jr nz, MLN1
        pop de
        ld a, l
        ret
        
        
; Print message terminated by 0, centered within 80 chars on screen
; in:
;   DE - addr
;   H - Y coord
;   B - attr
PRINT_MSG_C
        call MSG_LEN
PMC2
        rrca
        and 127
        neg
        add a, 40
        ld l, a


; Print message terminated by 0
; in:
;   DE - addr
;   HL - coords        
;   B - attr
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
        

; Drawing framed filled box
; in:
;   HL - coords
;   BC - sizes
;   A - attr
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

; ÉÍËÍ»
; º º º
; ÌÍÎÍ¹
; º º º
; ÈÍÊÍ¼


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
#include "restarts.asm"

        end
        
