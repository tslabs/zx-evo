
  device zxspectrum48
  include "tsconfig.asm"
  include "ft81x.asm"

  org $8000
codestart:

    di

    FT_CMD FT_CMD_CLKEXT
    FT_DELAY 65535
    FT_CMD FT_CMD_SLEEP
    FT_DELAY 65535
    FT_CMDP FT_CMD_CLKSEL, 4 | 64
    FT_ACTIVE
    FT_ACTIVE
    FT_DELAY 65535

    FT_WR8 FT_REG_PCLK, 1
    FT_WR8 FT_REG_PCLK_POL, 0    FT_WR8 FT_REG_CSPREAD, 0    FT_WR16 FT_REG_HCYCLE, 1040    FT_WR16 FT_REG_HSYNC0, 0    FT_WR16 FT_REG_HSYNC1, 120    FT_WR16 FT_REG_HOFFSET, 184    FT_WR16 FT_REG_HSIZE, 800    FT_WR16 FT_REG_VCYCLE, 666    FT_WR16 FT_REG_VSYNC0, 0
    FT_WR16 FT_REG_VSYNC1, 6
    FT_WR16 FT_REG_VOFFSET, 29    FT_WR16 FT_REG_VSIZE, 600
    FL_LOAD_DL DL1, DL1_

    FT_WR8 FT_REG_DLSWAP, FT_DLSWAP_FRAME

    ei
    ret

; --------------------------

DL1:
    FT_CLEAR_COLOR_RGB 0, 0, 0
    FT_CLEAR 1, 1, 1                ; clear screen

    FT_BEGIN FT_BITMAPS             ; start drawing bitmaps
    FT_VERTEX2II 420, 410, 31, 'F'  ; ascii F in font 31
    FT_VERTEX2II 444, 410, 31, 'T'  ; ascii T
    FT_VERTEX2II 470, 410, 31, 'D'  ; ascii D
    FT_VERTEX2II 499, 410, 31, 'I'  ; ascii I
    FT_END

    FT_COLOR_RGB 160, 22, 22        ; change colour to red
    FT_POINT_SIZE 320               ; set point size to 20 pixels in radius
    FT_BEGIN FT_POINTS              ; start drawing points
    FT_VERTEX2II 392, 433, 0, 0     ; red point
    FT_END

    FT_BEGIN FT_BITMAPS
    FT_COLOR_RGB 255, 255, 255
    FT_COLOR_A 255
    FT_VERTEX2II 250, 130, 31, 'V'
    FT_COLOR_A 192
    FT_VERTEX2II 258, 138, 31, 'D'
    FT_COLOR_A 128
    FT_VERTEX2II 266, 146, 31, 'A'
    FT_COLOR_A 64
    FT_VERTEX2II 274, 154, 31, 'C'
    FT_END

    FT_COLOR_RGB 0, 255, 32
    FT_COLOR_A 255
    FT_BEGIN FT_LINES
    FT_VERTEX2F 16 * 10, 16 * 30
    FT_VERTEX2F 16 * 150, 16 * 40
    FT_LINE_WIDTH 80
    FT_VERTEX2F 16 * 10, 16 * 80
    FT_VERTEX2F 16 * 150, 16 * 90
    FT_END

    FT_DISPLAY                      ; display the image
DL1_ equ ($ - DL1) >> 2

codeend:
  output "example_ft.bin"
  savetrd "example_ft.trd", "example.C", codestart, codeend - codestart
