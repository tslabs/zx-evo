
  device ZXSPECTRUM48

  include "tsconfig.asm"
  include "macro.asm"

; ----- 3 entry

    di
    ld sp, stackp
    jp START

; ----- Restarts

; RST 8: wait for DMA end
    orgp 0x0008

DWT:
    ld bc, DSTATUS
DWT1:
    inf
    ret p
    jr DWT1

; RST 10: TS-FAT driver entry
    orgp 0x0010
    jp tsfat

; RST 18: TS-BIOS API entry
    orgp 0x0018
    jp biosapi

    orgp 0x0020
    ret

    orgp 0x0028
    ret

    orgp 0x0030
    ret

; RST 38: INT IM1 Handler
    orgp 0x0038

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

; NMI Handler
    orgp 0x0066
    retn

; ----- Reset procedures

START:
    di
    wrxt PAGE1, 5

    im 1
    ld a, 63
    ld i, a
    xor a
    out (254), a

    call READ_NVRAM
    call CALC_CRC
    jr z, MN2

    call LOAD_DEFAULTS  ; If NVRAM CRC16 error - load defaults and run SETUP
    jp SETUP

MN2:
    rdxt STATUS
    bit 6, a
    push af
    call nz, FDV_RES        ; clear virtual FDDs at cold restart
    pop af
    call nz, sd_reset_init  ; dual SD spi init (only cmd0)

    ld a, (nres)
    or a
    call nz, NGS_RES    ; Reset NGS if enabled

    ld a, (fres)
    or a
    call nz, FT_RES     ; Reset FTxxx if enableds

    ld bc, 0x7FFE
    in a, (c)
    rrca
    rrca
    jp nc, SETUP        ; Go to setup if CS pressed

  ; --- Regular system start
RESET
    di
    call CLS_ZX

    wrxt PAGE1, 5
    wrxt PAGE2, 2
    wrxt PAGE3, 0
    wrxt VPAGE, 5
    wrxt VCONFIG, MODE_ZX | RRES_256X192

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
    wrxta FDDVIRT

;INT offset
    ld a, (into)
    wrxta HSINT

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
    wrxta SYSCONFIG

    ld bc, 0xFEFE
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
    wrxt PAGE0, 0
    jr RES_3

RES_ROM04
    wrxt PAGE0, 4
    jr RES_3

RES_VROM
    wrxt PAGE0, vrompage
    ld a, 8   ; b1000
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
    wrxta MEMCONFIG
    ld sp, 0x3D2E
    jp 0x3D2F       ; ret to #0000

RES_48
    ld a, d
    or 1    ; ROM 48
    wrxta MEMCONFIG
    jp 0

RES_128
    ld a, d
    wrxta MEMCONFIG
    jp 0

RES_SYS
    ld a, d
    or 4    ; b0100
    wrxta MEMCONFIG     ; no mapping
    jp 0

RES_BT_SR       ; sys.rom
    jr $

RES_BT_BD       ; boot.$c
    wrxt PAGE3, 0

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
ide_takoy_ide
    call start  ; a = Return code: 1 - no device, 2 - FAT32 not found, 3 - file not found
    call c, BT_ERROR
    jr c, ide_takoy_ide
    pop af
    push de

    or 1
    wrxta MEMCONFIG      ; mapping, Basic 48 ROM
    wrxt PAGE0, 0

    ld iy, 0x5C3A
    ld hl, 0x2758
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
    wrxta MEMCONFIG      ; mapping, Basic 48 ROM
    wrxt PAGE0, 0
    jp (hl)

RES_4
    halt

RESET2_END

; ------- ERROR Messages

BT_ERROR
    push af
    call TX_MODE

    ld hl, 0x0308
    ld bc, 0x0940
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
    wrxt VCONFIG, MODE_NOGFX

    call S_INIT
    call CLS_ZX
    call TX_MODE

    box 0, 0x1E50, 0x8F
    pmsgc M_HEAD1, 1, 0x8E
    pmsgc M_HEAD2, 2, 0x8E
    pmsgc M_HLP, 28, 0x87
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

; ------- Periphery

  ; --- Reset NGS
NGS_RES
    ld a, 0x80
    out (0x33), a
    ret

  ; --- First boot after conf loading, one-time initialization
FDV_RES
    xor a     ; Nulling VDOS mountings
    ld (fddv), a
    call WRITE_NVRAM
    ret

  ; --- Reset FT8xx
SPI_CTRL  equ 0x77
SPI_DATA  equ 0x57
SPI_FT_CS_ON  equ 7
SPI_FT_CS_OFF equ 3
FT_CMD_RST_PULSE   equ 0x68   ; cc 00 00

FT_RES
    ld a, SPI_FT_CS_ON
    out (SPI_CTRL), a
    ld a, FT_CMD_RST_PULSE
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    ld a, SPI_FT_CS_OFF
    out (SPI_CTRL), a
    ret

; ------- Subroutines

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
    ld bc, 0xFEFE
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
    ld b, 0x7F      ; additional check for SS - it could be not polled yet
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
	ld	hl, 0xFFFF
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
    xor	0x10
    ld	h, a
    ld	a, l
    xor	0x21
    ld	l, a
CRC3
    djnz CRC2
    dec	c
    jr	nz, CRC1
    ret

READ_NVRAM
    outf7 SHADOW, SHADOW_ON

    ld ix, nv_buf
    ld l, nv_1st
    ld a, nv_size
    ld c, PF7
RNV1
    ld b, NVADDR
    out (c), l
    ld b, NVDATA
    in d, (c)
    ld (ix + 0), d
    inc ix
    inc l
    dec a
    jr nz, RNV1

    outf7 SHADOW, SHADOW_OFF
    ret

LOAD_DEFAULTS
    ld hl, nv_def
    ld de, nv_buf
    ld bc, nv_size - 2
    ldir

WRITE_NVRAM
    call CALC_CRC
    ld (nvcs), hl

    outf7 SHADOW, SHADOW_ON

    ld ix, nv_buf
    ld l, nv_1st
    ld a, nv_size
    ld c, PF7
WNV1
    ld b, NVADDR
    out (c), l
    ld b, NVDATA
    ld d, (ix + 0)
    inc ix
    out (c), d
    inc l
    dec a
    jr nz, WNV1

    outf7 SHADOW, SHADOW_OFF
    ret

LD_FONT
    wrxt PAGE3, txpage ^ 1
    ld ix, font8
    ld de, WIN3
    jp DEHRUST

TX_MODE
    call CLS_TXT
    call LD_FONT
    call LD_S_PAL

    wrxt VCONFIG, RRES_320X240 | MODE_TEXT
    wrxt VPAGE, txpage
    wrxt PAGE3, txpage
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
    ld bc, PAGE3
    out (c), e
    ld b, DMASADDRX >> 8
    out (c), e
    ld b, DMADADDRX >> 8
    out (c), e
    ld b, DMASADDRH >> 8
    out (c), h
    ld b, DMADADDRH >> 8
    out (c), h
    ld b, DMASADDRL >> 8
    out (c), l
    set 7, h
    set 6, h
    ld (hl), a
    inc l
    ld (hl), a
    inc l
    ld b, DMADADDRL >> 8
    out (c), l
    ld b, DMANUM >> 8
    dec d
    dec d
    out (c), d
    wrxt DMALEN, 127
    wrxt DMACTR, DMA_DEV_MEM    ; Run DMA
    call DWT
    wrxt DMANUM, 0
    wrxt DMALEN, 126
    wrxt DMACTR, DMA_DEV_MEM    ; Run last burst
    jp DWT

LD_S_PAL
    ld hl, pal_bb
LD_PAL
    wrxt PALSEL, pal_sel
    wrxt FMADDR,  FM_EN | (pal_seg >> 4)
    ld de, pal_seg << 8 | (pal_sel * 32)
    ld bc, 32
LDP1
    ldir
    wrxt FMADDR, 0
    ret

LD_64_PAL
    ld hl, pal_64c
    wrxt PALSEL, pal_sel
    wrxt FMADDR,  FM_EN | (pal_seg >> 4)
    ld de, pal_seg << 8
    ld bc, 128
    jr LDP1

; Calculate message length
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
    ld de, 'É' << 8 | 'Í'
    ld b, '»'
    call DB2
    pop hl
    pop bc
    inc h
DB1
    push bc
    push hl
    ld de, 'º' << 8 | ' '
    ld b, 'º'
    call DB2
    pop hl
    pop bc
    inc h
    djnz DB1

    ld de, 'È' <<8 | 'Í'
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

biosapi:
    ret

; ----- Source includes

  include "booter.asm"
  include "dehst.asm"

; ------- Messages

M_HEAD1
        defb 'TS-BIOS Setup Utility', 0
M_HEAD2
        defb 'Build date: ', __DATE__, ' ', __TIME__, 0

M_HLP   defb 'Arrows - move,  Enter - change option,  F12 - exit', 0


; ------- Errors

ERR_ME
        defb 'System Meditation:', 0
ERR_MEZ
        defb 'Press SS + Reset to change start-up options', 0
ERR_ME0
        defb 'UNKNOWN ERROR!', 0
ERR_ME1
        defb 'Boot-Device NOT READY!', 0
ERR_ME2
        defb 'FAT32 NOT FOUND!', 0
ERR_ME3
        defb 'boot.$c NOT FOUND!', 0


; ------- Tabs
; -- Option table

OPT_X   equ 7   ; X coord of box
OPT_Y   equ 6   ; Y coord of box
OPT_SX  equ 32  ; X size of box
OPT_NUM equ 12  ; Number of options

OPTTAB0   ; Do not edit these values! (settings are above)
        defb OPT_X, OPT_Y, OPT_SX, OPT_NUM + 5      ; Options box X, Y and size
        defb OPT_X + 3, OPT_Y + 3                   ; X, Y coord of list top
        defb OPT_X + 5, OPT_Y + 1                   ; X, Y coord of header text
        defb 0x8C                                   ; Attrs
        defb OPT_NUM                                ; Number of options
        defb 'Select NVRAM options:', 0

        ; address of option desc, address of option choices
        defw OPT_CFQ, SEL_CFQ   ; CPU frequency
        defw OPT_CCH, SEL_ONF   ; CPU cache
        defw OPT_80L, SEL_LCK   ; #7FFD lock
        defw OPT_B1T, SEL_BOT   ; Boot 1 target
        defw OPT_B1B, SEL_BTB   ; Boot 1 bank
        defw OPT_B2T, SEL_BOT   ; Boot 2 target
        defw OPT_B2B, SEL_BTB   ; Boot 2 bank
        defw OPT_B1D, SEL_BDV   ; Boot device
        defw OPT_ZPL, SEL_ZPL   ; ZX palette
        defw OPT_NGR, SEL_ONF   ; NGS reset
        defw OPT_FTR, SEL_ONF   ; FT8xx reset
        defw OPT_INT, SEL_07    ; INT offset

; -- Option text
; byte - number of choices
; word - address in NVRAM vars
; string - option
OPT_CFQ    defb 3, low cfrq, high cfrq, 'CPU Speed, MHz:', 0
OPT_CCH    defb 2, low cach, high cach, 'CPU Cache:', 0
OPT_80L    defb 4, low l128, high l128, '#7FFD span:', 0
OPT_B1T    defb 5, low b1to, high b1to, 'Reset to:', 0
OPT_B1B    defb 4, low b1tb, high b1tb, ' ->bank:', 0
OPT_B2T    defb 5, low b2to, high b2to, 'CS Reset to:', 0
OPT_B2B    defb 4, low b2tb, high b2tb, ' ->bank:', 0
OPT_B1D    defb 7, low bdev, high bdev, 'Boot Device:', 0
OPT_ZPL    defb 7, low zpal, high zpal, 'ZX Palette:', 0
OPT_NGR    defb 2, low nres, high nres, 'NGS Reset:', 0
OPT_FTR    defb 2, low fres, high fres, 'FT8xx Reset:', 0
OPT_INT    defb 8, low into, high into, 'INT Offset:', 0

; -- Options
; string - choice
SEL_CFQ
        defb '       3.5', 0
        defb '         7', 0
        defb '        14', 0

SEL_BOT
        defb '   ROM #00', 0
        defb '   ROM #04', 0
        defb '   RAM #'
        hex8 vrompage
        defb 0
        defb 'BD boot.$c', 0
        defb 'BD sys.rom', 0

SEL_BTB
        defb '    TR-DOS', 0
        defb '  Basic 48', 0
        defb ' Basic 128', 0
        defb '       SYS', 0

SEL_BDV
        defb 'SD Z-contr', 0
        defb 'IDE Nemo M', 0
        defb 'IDE Nemo S', 0
        defb '    RS-232', 0
        defb 'IDE Smuc M', 0
        defb 'IDE Smuc S', 0
        defb 'SD2 Z-cont', 0

SEL_ONF
        defb ' OFF', 0
        defb '  ON', 0
        defb 'Auto', 0

SEL_LCK
        defb '     512k', 0
        defb '     128k', 0
        defb '128k Auto', 0
        defb '    1024k', 0

SEL_ZPL
        defb 'Default', 0
        defb 'B.black', 0
        defb '  Light', 0
        defb '   Pale', 0
        defb '   Dark', 0
        defb 'Grayscl', 0
        defb ' Custom', 0

SEL_07  defb '0', 0
        defb '1', 0
        defb '2', 0
        defb '3', 0
        defb '4', 0
        defb '5', 0
        defb '6', 0
        defb '7', 0

; -- palette
pal_bb               ; bright black
        defw 0x0000
        defw 0x0010
        defw 0x4000
        defw 0x4010
        defw 0x0200
        defw 0x0210
        defw 0x4200
        defw 0x4210
        defw 0x2108
        defw 0x0018
        defw 0x6000
        defw 0x6018
        defw 0x0300
        defw 0x0318
        defw 0x6300
        defw 0x6318

pal_lgt              ; light
        defw 0x0000
        defw 0x0014
        defw 0x5000
        defw 0x5014
        defw 0x0280
        defw 0x0294
        defw 0x5280
        defw 0x5294
        defw 0x0000
        defw 0x0018
        defw 0x6000
        defw 0x6018
        defw 0x0300
        defw 0x0318
        defw 0x6300
        defw 0x6318

pal_puls             ; pulsar
        defw 0x0000
        defw 0x0010
        defw 0x4000
        defw 0x4010
        defw 0x0200
        defw 0x0210
        defw 0x4200
        defw 0x4210
        defw 0x0000
        defw 0x0018
        defw 0x6000
        defw 0x6018
        defw 0x0300
        defw 0x0318
        defw 0x6300
        defw 0x6318

pal_dark              ; dark
        defw %000000000000000
        defw %000000000001000
        defw %010000000000000
        defw %010000000001000
        defw %000000100000000
        defw %000000100001000
        defw %010000100000000
        defw %010000100001000
        defw %000000000000000
        defw %000000000010000
        defw %100000000000000
        defw %100000000010000
        defw %000001000000000
        defw %000001000010000
        defw %100001000000000
        defw %100001000010000


pal_pale              ; pale
        defw %010000100001000
        defw %010000100010000
        defw %100000100001000
        defw %100000100010000
        defw %010001000001000
        defw %010001000010000
        defw %100001000001000
        defw %100001000010000
        defw %010000100001000
        defw %010000100011000
        defw %110000100001000
        defw %110000100011000
        defw %010001100001000
        defw %010001100011000
        defw %110001100001000
        defw %110001100011000

pal_gsc               ; grayscale
        defw 0  << 10 | 0  << 5 | 0
        defw 3  << 10 | 3  << 5 | 3
        defw 6  << 10 | 6  << 5 | 6
        defw 9  << 10 | 9  << 5 | 9
        defw 12 << 10 | 12 << 5 | 12
        defw 16 << 10 | 16 << 5 | 16
        defw 19 << 10 | 19 << 5 | 19
        defw 22 << 10 | 22 << 5 | 22
        defw 0  << 10 | 0  << 5 | 0
        defw 4  << 10 | 4  << 5 | 4
        defw 8  << 10 | 8  << 5 | 8
        defw 11 << 10 | 11 << 5 | 11
        defw 14 << 10 | 14 << 5 | 14
        defw 17 << 10 | 17 << 5 | 17
        defw 20 << 10 | 20 << 5 | 20
        defw 24 << 10 | 24 << 5 | 24

pal_64c
        defw 0x0000
        defw 0x0008
        defw 0x0010
        defw 0x0018
        defw 0x2000
        defw 0x2008
        defw 0x2010
        defw 0x2018
        defw 0x4000
        defw 0x4008
        defw 0x4010
        defw 0x4018
        defw 0x6000
        defw 0x6008
        defw 0x6010
        defw 0x6018
        defw 0x0100
        defw 0x0108
        defw 0x0110
        defw 0x0118
        defw 0x2100
        defw 0x2108
        defw 0x2110
        defw 0x2118
        defw 0x4100
        defw 0x4108
        defw 0x4110
        defw 0x4118
        defw 0x6100
        defw 0x6108
        defw 0x6110
        defw 0x6118
        defw 0x0200
        defw 0x0208
        defw 0x0210
        defw 0x0218
        defw 0x2200
        defw 0x2208
        defw 0x2210
        defw 0x2218
        defw 0x4200
        defw 0x4208
        defw 0x4210
        defw 0x4218
        defw 0x6200
        defw 0x6208
        defw 0x6210
        defw 0x6218
        defw 0x0300
        defw 0x0308
        defw 0x0310
        defw 0x0318
        defw 0x2300
        defw 0x2308
        defw 0x2310
        defw 0x2318
        defw 0x4300
        defw 0x4308
        defw 0x4310
        defw 0x4318
        defw 0x6300
        defw 0x6308
        defw 0x6310
        defw 0x6318

keys_norm
        defb 0, 'zxcvasdfgqwert1234509876poiuy', 13, 'lkjh ', 14, 'mnb'
keys_caps
        defb 0, 'ZXCVASDFGQWERT', 7, 6, 4, 5, 8, 12, 15, 9, 11, 10, 'POIUY', 13, 'LKJH ', 14, 'MNB'
keys_symb
        defb 0, ':`?/~|\\{}   <>!@#$%_)(', 39, '&', 34, '; ][', 13, '=+-^ ', 14, '.,*'

; ----- NVRAM default values

nv_def
        defb 0      ; FDDVirt [OFF]
        defb 0      ; CPU clock [3.5MHz]
        defb 0      ; boot device [SD Z-contr]
        defb 1      ; CPU Cache [ON]
        defb 0      ; Boot from [ROM #00]
        defb 0      ; Boot bank [TR-DOS]
        defb 3      ; CS Boot from [Boot Device boot.$c]
        defb 0      ; CS Boot bank [TR-DOS]
        defb 1      ; #7FFD Span [128kB]
        defb 0      ; ZX palette [Default]
        defb 0      ; NGS Reset [OFF]
        defb 0      ; FT8xx Reset [OFF]
        defb 1      ; INT offset [1]

        defs 9      ; padding (change it if adding options above!)

        ; Custom palette
        defw 0  << 10 | 0  << 5 | 0
        defw 2  << 10 | 2  << 5 | 2
        defw 4  << 10 | 4  << 5 | 4
        defw 6  << 10 | 6  << 5 | 6
        defw 8  << 10 | 8  << 5 | 8
        defw 10 << 10 | 10 << 5 | 10
        defw 12 << 10 | 12 << 5 | 12
        defw 14 << 10 | 14 << 5 | 14
        defw 1  << 10 | 1  << 5 | 1
        defw 3  << 10 | 3  << 5 | 3
        defw 5  << 10 | 5  << 5 | 5
        defw 7  << 10 | 7  << 5 | 7
        defw 9  << 10 | 9  << 5 | 9
        defw 11 << 10 | 11 << 5 | 11
        defw 13 << 10 | 13 << 5 | 13
        defw 15 << 10 | 15 << 5 | 15

; ----- Binary includes

font8
    incbin "866_code.fnt.hst"

sysvars
    incbin "sysvars.bin.hst"

rslsys:
  incbin "rslsys.bin.hst"

  orgp 0x2000
tsfat:
  incbin "tsfat.bin"

; ----- Export binary
  orgp 0x4000
bios_size equ $
  savebin "obj/ts-bios.bin", 0, bios_size

; -----------------------------------------

; ----- Vars

    org 0x5D00

; ----- Setup

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

nv_1st          equ 0xB0        ; first NVRAM cell
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
fres            defs 1      ; FT8xx Reset
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

    org 0x4200

secbu           defs 512
secbe
lobu            defs 512
lobe            defs 32
