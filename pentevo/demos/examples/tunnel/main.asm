
    output "main.bin"
    include "tsconfig.asm"
    include "tsfuncs.asm"

BSIZE equ 264
BNUM  equ 16
PNUM  equ 10
XNUM  equ 9
YNUM  equ 9

    org $6000
    jp main
    include "ft_func.asm"

main
    ld a, 4
    ld (offs_x), a
    ld (offs_y), a
    
    ld bc, SYSCONFIG
    ld a, SYS_ZCLK14 | SYS_CACHEEN
    out (c), a

    ei
    ld hl, vm_1024_768_59Hz
    ; ld hl, vm_1024_768_67Hz
    ; ld hl, vm_1024_768_76Hz
    call ft_init

    ft_wr8  FT_REG_INT_MASK,FT_INT_SWAP
    ft_wr8  FT_REG_INT_EN,1

    ld  bc, VCONFIG
    ld  a, VID_FT812
    out (c),a

loop
    xor a
l2
    ld (offs_p), a
    call calc_offs
    ld a, c
    ld bc, DMASADDRX
    out (c), a
    ld bc, DMASADDRL
    out (c), e
    ld bc, DMASADDRH
    out (c), d
    dma_len BSIZE, BNUM

    ld a, FT_RAM_DL >> 16
    ld de, FT_RAM_DL & 0xFFFF
    call ft_w_addr
    ld a, 3: out (254), a ; !!!
    dma_ctr DMA_RAM_SPI
    dma_wait
    ld a, 0: out (254), a ; !!!
    FT_OFF

    ft_wr8 FT_REG_DLSWAP, FT_DLSWAP_FRAME

wait_int
    ft_rd8  FT_REG_INT_FLAGS
    and FT_INT_SWAP
    jr z, wait_int

    call keys

    ld a, (offs_p)
    inc a
    cp PNUM
    jr c, l2
    jr loop

offs_p db 0
offs_x db 0
offs_y db 0

calc_offs
    define Y_SET_SIZE (BSIZE * BNUM * PNUM * XNUM)
    define X_SET_SIZE (BSIZE * BNUM * PNUM)
    define P_SET_SIZE (BSIZE * BNUM)

    ld ix, 0
    ld c, 2   ; starting address for DL is 0x020000

    ld de, Y_SET_SIZE & 0xFFFF
    ld h, (Y_SET_SIZE >> 16) & 0xFF
    ld a, (offs_y)
    call c_add

    ld de, X_SET_SIZE & 0xFFFF
    ld h, (X_SET_SIZE >> 16) & 0xFF
    ld a, (offs_x)
    call c_add

    ld de, P_SET_SIZE & 0xFFFF
    ld h, (P_SET_SIZE >> 16) & 0xFF
    ld a, (offs_p)
    call c_add

    ld a, xh
    rla
    rl c
    rla
    rl c

    push ix
    pop de
    ret

; in/out: C:IX - address
; in: H:DE - adder
; in: A - number of addings
c_add
    or a
    ret z
    ld b, a
c1  add ix, de
    ld a, c
    adc h
    ld c, a
    djnz c1
    ret

keys
    ld a, 0xEF
    in a, (254)
    rra
    rra
    jr c, k1
    ld a, (offs_y)
    or a
    ret z
    dec a
    ld (offs_y), a
    ret
    
k1  rra
    jr c, k2
    ld a, (offs_y)
    inc a
    cp YNUM
    ret z
    ld (offs_y), a
    ret
    
k2  rra
    jr c, k3
    ld a, (offs_x)
    inc a
    cp XNUM
    ret z
    ld (offs_x), a
    ret
    
k3  rra
    ret c
    ld a, (offs_x)
    or a
    ret z
    dec a
    ld (offs_x), a
    ret
