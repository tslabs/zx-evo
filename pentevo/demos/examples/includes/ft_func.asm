
  include "ft81x.asm"

  ifndef MF_SIZE
  define MF_SIZE 256
  endif

  ifndef MF_ADDR
  define MF_ADDR 0
  endif

; --------------------
    macro ft_cmd cmd
    ld b, cmd
    call ft_commd
    endm

; --------------------
    macro ft_wr8 addr, val
    ld de, addr & 0xFFFF
    ld a, val
    call ft_wreg8
    endm

; --------------------
    macro ft_wr16 addr, val
    ld de, addr & 0xFFFF
    ld bc, val
    call ft_wreg16
    endm

; --------------------
    macro ft_rd8 addr
    ld de, addr & 0xFFFF
    call ft_rreg8
    endm

; --------------------
    macro ft_rd16 addr
    ld de, addr & 0xFFFF
    call ft_rreg16
    endm

; --------------------
    macro ft_delay del
    ld b, del
.c1
    halt
    djnz .c1
    endm

; --------------------
; check FT812
ft_detect
    ; check config version
    ld bc, STATUS
    in a, (c)
    and 7
    cp 7
    ; ret nz    ; !!! fix Unreal

    ; check chip presence
    ft_rd8 FT_REG_ID
    cp 0x7C
    ret

; --------------------
ft_copr_reset
    ft_wr8 FT_REG_CPURESET, 1
    ft_wr16 FT_REG_CMD_READ, 0
    ft_wr16 FT_REG_CMD_WRITE, 0
    ft_wr8 FT_REG_CPURESET, 0
    ret

; --------------------
; in:
;    HL - address of video mode structure
ft_init
    ft_cmd FT_CMD_RST_PULSE
    ft_delay 18
    ft_cmd FT_CMD_CLKEXT
    ft_delay 6
    ft_cmd FT_CMD_SLEEP
    ft_delay 6

    ld b, FT_CMD_CLKSEL
    ld a, (hl)
    inc hl
    or 192
    ld c, a
    call ft_cmd_p

    ft_cmd FT_CMD_ACTIVE
    ft_cmd FT_CMD_ACTIVE
    ft_delay 6

    ft_wr8 FT_REG_PCLK, 1
    ft_wr8 FT_REG_PCLK_POL, 0
    ft_wr8 FT_REG_CSPREAD, 0            ; This is critical for correct colors display

    macro ft_tab_load reg
    ld de, reg & 0xFFFF
    ld c, (hl)
    inc hl
    ld b, (hl)
    inc hl
    call ft_wreg16
    endm

    ft_tab_load FT_REG_HSYNC0
    ft_tab_load FT_REG_HSYNC1
    ft_tab_load FT_REG_HOFFSET
    ft_tab_load FT_REG_HSIZE
    ft_tab_load FT_REG_HCYCLE
    ft_tab_load FT_REG_VSYNC0
    ft_tab_load FT_REG_VSYNC1
    ft_tab_load FT_REG_VOFFSET
    ft_tab_load FT_REG_VSIZE
    ft_tab_load FT_REG_VCYCLE
    ret

; --------------------
; in:
;    DE - reg lower 16 bits
;    A - value
ft_wreg8
    push af
    FT_ON

    ld a, (FT_RAM_REG >> 16) | 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    pop af
    out (SPI_DATA), a

    FT_OFF
    ret

; --------------------
; in:
;    DE - reg lower 16 bits
;    BC - value
ft_wreg16
    FT_ON

    ld a, (FT_RAM_REG >> 16) | 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a

    ld a, c
    out (SPI_DATA), a
    ld a, b
    out (SPI_DATA), a

    FT_OFF
    ret

; --------------------
; in:
;    DE - reg lower 16 bits
; out:
;    A - value
ft_rreg8
    FT_ON

    ld a, FT_RAM_REG >> 16
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a    ; dummy
    in a, (SPI_DATA)     ; dummy

    in a, (SPI_DATA)

    push af
    FT_OFF
    pop af
    ret

; --------------------
; in:
;    DE - reg lower 16 bits
; out:
;    A - value
ft_rreg16
    FT_ON

    ld a, FT_RAM_REG >> 16
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a    ; dummy
    in a, (SPI_DATA)     ; dummy

    in a, (SPI_DATA)
    ld c, a
    in a, (SPI_DATA)
    ld b, a

    FT_OFF
    ret

; --------------------
; in:
;    HL - memory address
;    ADE - FT812 address
;    BC - number of bytes
ft_read
    push af
    FT_ON
    pop af

    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    out (SPI_DATA), a    ; dummy
    in a, (SPI_DATA)     ; dummy

    ld a, b
    ld b, c
    ld c, SPI_DATA

.c1
    inir
    or a
    jr z, .exit
    dec a
    jr .c1

.exit
    FT_OFF
    ret

; --------------------
; in:
;    HL - memory address
;    ADE - FT812 address
;    BC - number of bytes
; out:
;    HL - address + number of bytes
;    ADE - FT812 address + number of bytes
ft_write
    push af
    FT_ON
    pop af

    push af
    or 0x80
    out (SPI_DATA), a
    ld a, d
    out (SPI_DATA), a
    ld a, e
    out (SPI_DATA), a
    pop af

    ex de, hl
    add hl, bc
    ex de, hl
    adc a, 0
    push af

    ld a, c
    or a
    ld a, b
    ld b, c
    ld c, SPI_DATA
    jr z, .c1
    otir
    or a
    jr z, .exit
.c1
    otir
    dec a
    jr nz, .c1

.exit
    FT_OFF
    pop af
    ret

; --------------------
; in:
;    HL - memory address
;    DE - FT812 DL offset
;    BC - number of bytes
; out:
;    DE - FT812 DL offset + number of bytes
ft_load_dl
    ld a, FT_RAM_DL >> 16
    jr ft_write

; --------------------
; in:
;    HL - memory address
;    BC - number of bytes
ft_load_copr
    push hl
    push bc

.w1
    ; read available CMDB buffer space
    ft_rd16 FT_REG_CMDB_SPACE
    ld a, b
    or c
    jr z, .w1

    ; size = min(remainder, size)
    pop hl ; number of bytes
    or a
    sbc hl, bc
    jr nc, .c2
    add hl, bc
    ld b, h
    ld c, l
    ld hl, 0
.c2
    ex (sp), hl ; remainder <-> memory address
    ld a, FT_REG_CMDB_WRITE >> 16
    ld de, FT_REG_CMDB_WRITE & 0xFFFF
    call ft_write

    pop bc    ; remainder
    ld a, b
    or c
    jr nz, ft_load_copr
    ret

; --------------------
; attention! you should only request integral parts of FIFO size, e.g. half
; in:
;    HL - memory address
;    BC - number of bytes
load_mediafifo
    push hl
    ld (.s1 + 1), bc

    ; check free space
    ft_rd16 FT_REG_MEDIAFIFO_WRITE
    ld h, b
    ld l, c

.w1
    push hl
    ; check if error happened
    ft_rd8 FT_REG_CMD_READ
    inc a   ; 0x0FFF
    jr z, .err

    ft_rd16 FT_REG_MEDIAFIFO_READ
    ; used = (wptr - rptr) mod fsize
    or a
    sbc hl, bc
    ld a, h
    and (MF_SIZE - 1) >> 8
    ld h, a
    ex de, hl
    ; free = (fsize - 4) - used
    ld hl, MF_SIZE - 4
    sbc hl, de
.s1
    ld bc, 0
    sbc hl, bc
    pop hl
    jr c, .w1

    ; load data
    ld a, MF_ADDR >> 16
    ld de, MF_ADDR & 0xFFFF
    or a
    adc hl, de
    ex de, hl
    adc a, 0
    pop hl
    call ft_write

    ; update FIFO write pointer
    ld c, e
    ld a, d
    and (MF_SIZE - 1) >> 8
    ld b, a
    ld de, FT_REG_MEDIAFIFO_WRITE & 0xFFFF
    call ft_wreg16
    or a
    ret

.err
    pop hl
    pop hl
    call ft_copr_reset
    scf
    ret

; --------------------
; in:
;    B - command code
;    C - command parameter
ft_commd
    ld c, 0
ft_cmd_p
    FT_ON
    ld a, b
    out (SPI_DATA), a
    ld a, c
    out (SPI_DATA), a
    xor a
    out (SPI_DATA), a
    FT_OFF
    ret

; --------------------
; .rdata

; Vmodes
vm_640_480_57Hz   ft_mode_tab F0_MUL, H0_FPORCH, H0_SYNC, H0_BPORCH, H0_VISIBLE, V0_FPORCH, V0_SYNC, V0_BPORCH, V0_VISIBLE
vm_640_480_74Hz   ft_mode_tab F1_MUL, H1_FPORCH, H1_SYNC, H1_BPORCH, H1_VISIBLE, V1_FPORCH, V1_SYNC, V1_BPORCH, V1_VISIBLE
vm_640_480_76Hz   ft_mode_tab F2_MUL, H2_FPORCH, H2_SYNC, H2_BPORCH, H2_VISIBLE, V2_FPORCH, V2_SYNC, V2_BPORCH, V2_VISIBLE
vm_800_600_60Hz   ft_mode_tab F3_MUL, H3_FPORCH, H3_SYNC, H3_BPORCH, H3_VISIBLE, V3_FPORCH, V3_SYNC, V3_BPORCH, V3_VISIBLE
vm_800_600_69Hz   ft_mode_tab F4_MUL, H4_FPORCH, H4_SYNC, H4_BPORCH, H4_VISIBLE, V4_FPORCH, V4_SYNC, V4_BPORCH, V4_VISIBLE
vm_800_600_85Hz   ft_mode_tab F5_MUL, H5_FPORCH, H5_SYNC, H5_BPORCH, H5_VISIBLE, V5_FPORCH, V5_SYNC, V5_BPORCH, V5_VISIBLE
vm_1024_768_59Hz  ft_mode_tab F6_MUL, H6_FPORCH, H6_SYNC, H6_BPORCH, H6_VISIBLE, V6_FPORCH, V6_SYNC, V6_BPORCH, V6_VISIBLE
vm_1024_768_67Hz  ft_mode_tab F7_MUL, H7_FPORCH, H7_SYNC, H7_BPORCH, H7_VISIBLE, V7_FPORCH, V7_SYNC, V7_BPORCH, V7_VISIBLE
vm_1024_768_76Hz  ft_mode_tab F8_MUL, H8_FPORCH, H8_SYNC, H8_BPORCH, H8_VISIBLE, V8_FPORCH, V8_SYNC, V8_BPORCH, V8_VISIBLE
