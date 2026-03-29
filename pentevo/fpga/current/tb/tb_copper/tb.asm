
    output "tb.bin"
    include "../files/asm/tsconfig.asm"

    org 0

    ld bc, FMADDR
    ld a, 0x01 | FM_EN
    out (c), a

    ld hl, cp_start
    ld de, 0x1000 | FM_CLIST
    ld bc, cp_end - cp_start
    ldir

    ld bc, COPPER
    ld a, 0
    out (c), a

    di
    halt

cp_start
    include "copper.asm"
cp_end