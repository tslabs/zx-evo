
MF_ADDR     equ 0
MF_SIZE     equ 4096

    device zxspectrum48
    include "tsconfig.asm"

    org $8000
codestart:
    jp main
    include "ft_func.asm"
    
main
    ei
    ld hl, vm_1024_768_59Hz
    call ft_init

    FL_LOAD_DL DL1, DL1_

    FT_WR8 FT_REG_DLSWAP, FT_DLSWAP_FRAME
    ret


; --------------------------

DL1:
    FT_CLEAR_COLOR_RGB 0, 0, 0
    FT_CLEAR 1, 1, 1

    FT_COLOR_A 255
    FT_BEGIN FT_POINTS

    FT_COLOR_RGB 50, 15, 11
    FT_POINT_SIZE 511 << 4
    FT_VERTEX2F 512 << 4, 384 << 4

    FT_COLOR_RGB 255, 255, 100
    FT_POINT_SIZE 400 << 4
    FT_VERTEX2F 400 << 4, 300 << 4

    FT_COLOR_RGB 34, 240, 67
    FT_POINT_SIZE 320 << 4
    FT_VERTEX2F 320 << 4, 240 << 4
    FT_END

    FT_COLOR_A 255
    FT_BEGIN FT_LINES

    FT_COLOR_RGB 0, 255, 255
    FT_LINE_WIDTH 16
    FT_VERTEX2F 500 << 4, 40 << 4
    FT_VERTEX2F 799 << 4, 70 << 4

    FT_COLOR_RGB 255, 80, 0
    FT_LINE_WIDTH 80
    FT_VERTEX2F 10 << 4, 130 << 4
    FT_VERTEX2F 200 << 4, 190 << 4
    FT_END

    FT_BEGIN FT_BITMAPS
    FT_COLOR_RGB 255, 32, 100
    FT_VERTEX2II 420, 410, 31, 'X'
    FT_VERTEX2II 444, 410, 31, 'X'
    FT_VERTEX2II 470, 410, 31, 'X'
    FT_END

    FT_BEGIN FT_BITMAPS
    FT_COLOR_RGB 255, 255, 255
    FT_COLOR_A 255
    FT_VERTEX2II 250, 130, 31, 'O'
    FT_COLOR_A 192
    FT_VERTEX2II 258, 138, 31, 'O'
    FT_COLOR_A 128
    FT_VERTEX2II 266, 146, 31, 'O'
    FT_COLOR_A 64
    FT_VERTEX2II 274, 154, 31, 'O'
    FT_END

    FT_BEGIN FT_RECTS
    FT_LINE_WIDTH 15
    FT_COLOR_RGB 0, 0, 0
    FT_COLOR_A 128
    FT_VERTEX2F 100 << 4, 80 << 4
    FT_VERTEX2F 923 << 4, 687 << 4
    ; FT_VERTEX2F 0 << 4, 0 << 4
    ; FT_VERTEX2F 1023 << 4, 767 << 4
    FT_END

    FT_DISPLAY
DL1_ equ ($ - DL1) >> 2

codeend:
    output "example_ft.bin"
    savetrd "example_ft.trd", "example.C", codestart, codeend - codestart
