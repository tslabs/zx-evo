
    PUBLIC tsbios
    PUBLIC tsfat
    PUBLIC IM1_ISR

    EXTERN DWT

; -----------------------------------
; -- external headers

#include "conf.asm"
#include "macro.asm"

; -----------------------------------
; -- external sources

    RSEG TSFAT
tsfat:
#include "tsfat.inc"

    RSEG CODE

#include "booter.asm"
#include "dehst.asm"
#include "arrays.asm"

rslsys:
#include "rslsys.inc"

; -----------------------------------
; -- INT IM1 Handler

IM1_ISR:
    push bc
    push de
    push hl
    push af
    call KBD_PROC
    ld a, (evt)
    or a            ; was last event processed?
    jr nz, IM11     ; not yet - return
    call KBD_EVT
    ld a, b
    ld (evt), a
IM11:
    pop af
    pop hl
    pop de
    pop bc
    ei
    ret

; -----------------------------------
; -- reset procedures

tsbios:
    di
    xtr
    xt page1, 5

    im 1
    ld a, 63
    ld i, a
    xor a
    out (254), a

    ;call sd_reset_init

    call READ_NVRAM
    call CALC_CRC
    jr z, MN2
    call LOAD_DEFAULTS    ; if NVRAM CRC16 error - load defaults and run SETUP
    jp SETUP
MN2:
    inxt xstatus
    bit 6, a
    jr z, MN1     ; check if first boot after conf loading

    xor a     ; nulling vdos mountings
    ld (fddv), a
    call WRITE_NVRAM

MN1:
; NGS Reset
    ld a, (nres)
    or a
    jr z, RES_5
    ld a, h'80
    out (h'33), a
RES_5

    ld bc, h'7FFE
    in a, (c)
    rrca
    rrca
    jp nc, SETUP    ; CS pressed

RESET
    di
    call CLS_ZX

    xtr
    xt page1, 5
    xt page2, 2
    xt page3, 0
    xt vpage, 5
    xt vconf, mode_zx | rres_256x192

; Palette
    ld a, (zpal)
    ld hl, pal_puls
    or a
    jr z, RES_2
    ld hl, pal_bb
    dec a
    jr z, RES_2
    ld hl, pal_lgt
    dec a
    jr z, RES_2
    ld hl, pal_pale
    dec a
    jr z, RES_2
    ld hl, pal_dark
    dec a
    jr z, RES_2
    ld hl, pal_gsc
    dec a
    jr z, RES_2
    ld hl, cpal
RES_2
    call LD_PAL
    call LD_64_PAL

; FDDVirt
    ld a, (fddv)
    xtr
    xta fddvirt

;INT offset
    ld a, (into)
    xta hsint

    ld hl, RESET2
    ld de, res_buf
    ld bc, RESET2_END - RESET2
    ldir
    jp res_buf

; ONLY relocatable code here!
RESET2:
; -- setting up h/w parameters

; 128 lock
    ld a, (l128)
    rrca
    rrca
    ld d, a     ; D keeps the Lock128 Mode

; AY & CPU freq
    ld a, (cach)
    rlca
    rlca
    ld e, a
    ld a, (cfrq)
    or e
    xtr
    xta sysconf

    ld bc, h'FEFE
    in a, (c)
    rrca
    ld a, (b2tb)
    ld e, a     ; E keeps the bank where boot to
    ld a, (b2to)
    jr nc, RES_1    ; CS pressed
    ld a, (b1tb)
    ld e, a     ; E keeps the bank where boot to
    ld a, (b1to)
RES_1
    or a    ; 0
    jr z, RES_ROM00
    dec a       ; 1
    jr z, RES_ROM04
    dec a       ; 2
    jr z, RES_VROM
    dec a       ; 3
    jr z, RES_BT_BD
    dec a       ; 4
    jr z, RES_BT_SR
    jr $

RES_ROM00
    xtr
    xt page0, 0
    jr RES_3

RES_ROM04
    xtr
    xt page0, 4
    jr RES_3

RES_VROM
    xtr
    xt page0, vrompage
    ld a, b'1000
    or d    ; Lock128 Mode
    ld d, a     ; RAM will be used instead of ROM

RES_3
    ld a, e
    or a    ; 0
    jr z, RES_TRD
    dec a       ; 1
    jr z, RES_48
    dec a       ; 2
    jr z, RES_128
    dec a       ; 3
    jr z, RES_SYS
    halt

RES_TRD
    ld a, d
    or 1    ; ROM 48
    xtr
    xta memconf
    ld sp, h'3D2E
    jp h'3D2F       ; ret to #0000

RES_48
    ld a, d
    or 1    ; ROM 48
    xtr
    xta memconf
    jp 0

RES_128
    ld a, d
    xtr
    xta memconf
    jp 0

RES_SYS
    ld a, d
    or b'0100
    xtr
    xta memconf     ; no mapping
    jp 0

RES_BT_SR       ; sys.rom
    jr $

RES_BT_BD       ; boot.$c
    xtr
    xt page3, 0

    ld a, (bdev)
    or a    ;0
    ld b, 0
    jr z, RES_BD_XX     ; SD1 Card
    inc b
    dec a       ;1
    jr z, RES_BD_XX     ; IDE Nemo Master
    inc b
    dec a       ;2
    jr z, RES_BD_XX     ; IDE Nemo Slave
    dec a       ;3
    jr z, RES_BD_RS
    inc b
    dec a       ;4
    jr z, RES_BD_XX     ; IDE Smuc Master
    inc b
    dec a       ;5
    jr z, RES_BD_XX     ; IDE Smuc Slave
    inc b
    dec a       ;6
    jr z, RES_BD_XX     ; SD2 Card
    jr $

RES_BD_XX       ; SD1 Card, Nemo Master, Nemo Slave, Smuc Master, Smuc Slave, SD2 Card (B = device)
    push de
ide_takoj_ide
    call start  ; a = Return code: 1 - no device, 2 - FAT32 not found, 3 - file not found
    call c, BT_ERROR
    jr c, ide_takoj_ide
    pop af
    push de

    or 1
    xtr
    xta memconf      ; mapping, Basic 48 ROM
    xt page0, 0

    ld iy, h'5C3A
    ld hl, h'2758
    exx
    ret

RES_BD_RS       ;RS-232
    push de
    ld ix, rslsys
    ld de, rslsys_addr
    call DEHRUST
    pop af
    jr nc, RES_4

    or 1
    xtr
    xta memconf      ; mapping, Basic 48 ROM
    xt page0, 0
    jp (hl)
RES_4
    halt

RESET2_END

; ------- ERROR Messages

BT_ERROR
    push af
    call TX_MODE

    ld hl, h'0308
    ld bc, h'0940
    ld a, err_norm
    call DRAW_BOX

    pmsgc ERR_ME, 5, err_norm
    pmsgc ERR_MEZ, 9, err_norm
    pop af

    ld b, a
    dec a
    ld de, ERR_ME1
    jr z, BTE1
    dec a
    ld de, ERR_ME2
    jr z, BTE1
    dec a
    ld de, ERR_ME3
    jr z, BTE1
    ld de, ERR_ME0

BTE1
    push bc
    ld h, 7
    ld b, err_norm
    call PRINT_MSG_C
    pop af
    dec a
    jr nz, $
    ld a, (device)
    ld b, a
    scf
    ret

; ------- BIOS Setup

SETUP:
    xtr
    xt vconf, mode_nogfx

    call S_INIT
    call CLS_ZX
    call TX_MODE

    box 0, h'1E50, h'8F
    pmsgc M_HEAD1, 1, h'8E
    pmsgc M_HEAD2, 2, h'8E
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
    ; cp ev_kb_help
    ; jr z, EV0_H
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
    ld de, (opt_nvr)
    ld a, (de)
    inc a
    ld (de), a
    ld a, c
    call OPT_HG
    jp WRITE_NVRAM

EV0_H
    jp RESET

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
    ld h, a     ; Y coord
    call PRINT_MSG

    ld a, (sel_max)
    ld c, a
    ld de, (opt_nvr)
    ld a, (de)
    cp c    ; is the value larger than max?
    jr c, OPP2      ; no - ok
    xor a       ; yes - zero it
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
    ld h, a     ; HL = address of option desc pointer

    ld e, (hl)
    inc hl
    ld d, (hl)
    inc hl
    ld (fld_opt), de
    ld a, (de)      ; number of choises
    inc de
    ld (sel_max), a
    ld a, (de)      ; address in NVRAM low
    inc de
    ld (opt_nvr), a
    ld a, (de)      ; address in NVRAM high
    inc de
    ld (opt_nvr+1), a

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
    cp c    ; is key the same as last time?
    jr z, KPC2      ; yes - autorepeat

    ld a, 15    ; initial autorepeat value
    ld (kbd_del), a
    ld a, c
    ld (last_key), a
    jr KPC3

KPC2
    ld a, (kbd_del)
    dec a
    ld (kbd_del), a
    jr nz, KPC5     ; autorepeat in progress
    ld a, 2     ; autorepeat value
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
    ret     ; no key pressed
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
    set 1, e    ; SS pressed
KBP7
    scf
    ret
KBP5
    set 0, e
    jr KBP4
KBP6
    set 1, e
    bit 0, e    ; was CS pressed before?
    jr z, KBP4      ; no - go back to cycle
    scf     ; yes - CS+SS pressed
    ret

CALC_CRC
    ld de, nv_buf + 1      ; 1st cell FDDVirt is ignored
    ld c, nv_size - 3
    call CRC16
    ld a, (de)
    cp l
    ret nz
    inc de
    ld a, (de)
    cp h
    ret

CRC16
	ld	hl, h'FFFF
CRC1
	ld	a, (de)
    inc	de
    xor	h
    ld	h, a
    ld	b, 8
CRC2
	add	hl, hl
    jr	nc, CRC3
    ld	a, h
    xor	h'10
    ld	h, a
    ld	a, l
    xor	h'21
    ld	l, a
CRC3
    djnz CRC2
    dec	c
    jr	nz, CRC1
    ret

READ_NVRAM
    outf7 shadow, shadow_on

    ld ix, nv_buf
    ld l, nv_1st
    ld a, nv_size
    ld c, pf7
RNV1
    ld b, nvaddr
    out (c), l
    ld b, nvdata
    in d, (c)
    ld (ix + 0), d
    inc ix
    inc l
    dec a
    jr nz, RNV1

    outf7 shadow, shadow_off
    ret

LOAD_DEFAULTS
    ld hl, nv_def
    ld de, nv_buf
    ld bc, nv_size - 2
    ldir

WRITE_NVRAM
    call CALC_CRC
    ld (nvcs), hl

    outf7 shadow, shadow_on

    ld ix, nv_buf
    ld l, nv_1st
    ld a, nv_size
    ld c, pf7
WNV1
    ld b, nvaddr
    out (c), l
    ld b, nvdata
    ld d, (ix + 0)
    inc ix
    out (c), d
    inc l
    dec a
    jr nz, WNV1

    outf7 shadow, shadow_off
    ret

LD_FONT
    xtr
    xt page3, txpage ^ 1
    ld ix, font8
    ld de, win3
    jp DEHRUST

TX_MODE
    call CLS_TXT
    call LD_FONT
    call LD_S_PAL

    xtr
    xt vconf, rres_320x240 | mode_text
    xt vpage, txpage
    xt page3, txpage
    ret

CLS_ZX
    ld e, 5
    ld d, 27
    jr CLT1

CLS_TXT
    ld e, txpage
    ld d, 64
CLT1
    ld hl, 0
    xor a

; Fill memory using DMA (only 256 byte aligned addresses!)
; EHL - address
; D - numbers of 256 byte blocks
; A - fill value
DMAFILL
    xtbc, page3
    out (c), e
    ld b, dmasax
    out (c), e
    ld b, dmadax
    out (c), e
    ld b, dmasah
    out (c), h
    ld b, dmadah
    out (c), h
    ld b, dmasal
    out (c), l
    set 7, h
    set 6, h
    ld (hl), a
    inc l
    ld (hl), a
    inc l
    ld b, dmadal
    out (c), l
    ld b, dmanum
    dec d
    dec d
    out (c), d
    xtrv
    xt dmalen, 127
    xt dmactr, dma_dev_mem    ; Run DMA
    call DWT
    xtrv
    xt dmanum, 0
    xt dmalen, 126
    xt dmactr, dma_dev_mem    ; Run last burst
    jp DWT

LD_S_PAL
    ld hl, pal_bb
LD_PAL
    xtr
    xt palsel, pal_sel
    xt fmaddr,  fm_en | (pal_seg >> 4)
    ld de, pal_seg << 8 | (pal_sel * 32)
    ld bc, 32
LDP1
    ldir
    xtr
    xt fmaddr, 0
    ret

LD_64_PAL
    ld hl, pal_64c
    xtr
    xt palsel, pal_sel
    xt fmaddr,  fm_en | (pal_seg >> 4)
    ld de, pal_seg << 8
    ld bc, 128
    jr LDP1

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

; NUM_10
    ; ld e, 0
; N102
    ; sub 10
    ; jr c, N101
    ; inc e
    ; jr N102
; N101
    ; ld d, a
    ; ld a, e
    ; add a, '0'
    ; call SYM

    ; ld a, d
    ; add a, '0' + 10

SYM
    ld (hl), a
    set 7, l
    ld (hl), b
    res 7, l
    inc l
    ret

; -----------------------------------
; Vars

    RSEG UDATA0

; -----------------------------------
; -- Setup

kbd_del         defs 1      ; current value of autorepeat counter
last_key        defs 1      ; last pressed key
map_key         defs 1      ; key mapped to layout
key             defs 1      ; actually pressed key mapped to layout (affected by autorepead)
evt             defs 1      ; event code
fld_curr        defs 1      ; current field of setup
fld_top         defs 2      ; coordinates of 1st option in the field
fld_max         defs 1      ; number of options in field
fld_tab         defs 2      ; addr of tab options addr tab
fld_opt         defs 2      ; addr of option
fld_sel         defs 2      ; addr of option choises
fld0_pos        defs 1      ; cursor position in field0
sel_max         defs 1      ; number of available choises in current option
opt_nvr         defs 2      ; NVRAM cell in vars for current option

; -----------------------------------
; -- NVRAM cells (must be exactly 56 bytes size!)
; ATTENTION! When changing NVRAM cells declaration, update defaults in 'nv_def' array!

nv_1st          equ h'B0        ; first NVRAM cell
nv_buf:

fddv            defs 1      ; FDDVirt (#29AF copy)  // non-removable #B0
cfrq            defs 1      ; CPU freq              // non-removable #B1
bdev            defs 1      ; boot device           // non-removable #B2

cach            defs 1      ; CPU Cache
b1to            defs 1      ; Boot option
b1tb            defs 1      ; Boot bank
b2to            defs 1      ; CS Boot option
b2tb            defs 1      ; CS Boot bank
l128            defs 1      ; #7FFD Span
zpal            defs 1      ; ZX palette
nres            defs 1      ; NGS Reset
into            defs 1      ; INT offset
                defs 10     ; dummy
cpal            defs 32     ; Custom palette (array)
nvcs            defs 2      ; checksum - must be last in the declaration

nv_size         equ $ - nv_buf

; -----------------------------------
; -- FAT driver

nsdc            defs 1      ; sec num in clus
eoc             defs 1      ; chain end flag
abt             defs 1
nr0             defs 2      ; number of sectors to be loaded
lstse           defs 4
rezde           defs 2
clhl            defs 2
clde            defs 2
llhl            defs 4
lthl            defs 2      ;last
ltde            defs 2

entry           defs 11     ; \ name
e_flag          defs 1      ; | flag
e_ntres         defs 1      ; | reserved for use by winNT
e_mills         defs 1      ; | Millisecond stap
e_cr_tm         defs 2      ; | CreateTime
e_cr_da         defs 2      ; | CreateDate
e_lt_ad         defs 2      ; | LstAccDate
clsde           defs 2      ; | FirstClusHI
e_wr_tm         defs 2      ; | WriteTime
e_wr_da         defs 2      ; | WriteDate
clshl           defs 2      ; | FirstClusLO
sizik           defs 4      ; / size

brezs           defs 2      ; fat parameters
bfats           defs 1
bftsz           defs 4
bsecpc          defs 2      ; use 1
brootc          defs 4
addtop          defs 4
sfat            defs 2
sdfat           defs 4
cuhl            defs 2
cude            defs 2
ldhl            defs 2      ; addr to read/write
count           defs 1
duhl            defs 2
dude            defs 2
lstcat          defs 4      ; active dir
blknum          defs 4; sector number to proceed
dahl            defs 2      ; see hdd proc.
dade            defs 2
drvre           defs 1
device          defs 1; 0 - SD, 1 - HDD Master, 2 - HDD Slave
blkt            defs 1; SD block scale (x1/x512)
zes             defs 1


; -----------------------------------
; -- FAT driver buffers

    RSEG FATBUF

secbu           defs 512
secbe
lobu            defs 512
lobe            defs 32

    end
