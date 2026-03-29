
    output "tb.bin"
    ; include "tsconfig.asm"

    org 0

    ld a, 1
    out 0x77, a ; spi_ctrl

    ld a, 0x55
    out 0x57, a ; spi_data

    ld a, 0xAA
    out 0x57, a ; spi_data

    ld a, 3
    out 0x77, a ; spi_ctrl

    di
    halt
