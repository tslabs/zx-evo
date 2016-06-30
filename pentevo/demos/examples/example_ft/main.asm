
  device zxspectrum48
  include "tsconfig.asm"
  include "ft81x.asm"

; 640x480@57Hz (24Mhz)
; F_MUL     equ 3
; H_FPORCH  equ 16
; H_SYNC    equ 96
; H_BPORCH  equ 48
; H_VISIBLE equ 640
; V_FPORCH  equ 11
; V_SYNC    equ 2
; V_BPORCH  equ 31
; V_VISIBLE equ 480

; 640x480@74Hz (32Mhz)
; F_MUL     equ 4 
; H_FPORCH  equ 24
; H_SYNC    equ 40
; H_BPORCH  equ 128
; H_VISIBLE equ 640
; V_FPORCH  equ 9
; V_SYNC    equ 3
; V_BPORCH  equ 28
; V_VISIBLE equ 480

; 640x480@76Hz (32Mhz)
; F_MUL     equ 4
; H_FPORCH  equ 16
; H_SYNC    equ 96
; H_BPORCH  equ 48
; H_VISIBLE equ 640
; V_FPORCH  equ 11
; V_SYNC    equ 2
; V_BPORCH  equ 31
; V_VISIBLE equ 480

; 800x600@60Hz (40Mhz)
F_MUL     equ 5
H_FPORCH  equ 40
H_SYNC    equ 128
H_BPORCH  equ 88
H_VISIBLE equ 800
V_FPORCH  equ 1
V_SYNC    equ 4
V_BPORCH  equ 23
V_VISIBLE equ 600

; 800x600@69Hz (48Mhz)
; F_MUL     equ 6
; H_FPORCH  equ 56
; H_SYNC    equ 120
; H_BPORCH  equ 64
; H_VISIBLE equ 800
; V_FPORCH  equ 37
; V_SYNC    equ 6
; V_BPORCH  equ 23
; V_VISIBLE equ 600

; 800x600@85Hz (56Mhz)
; F_MUL     equ 7
; H_FPORCH  equ 32
; H_SYNC    equ 64
; H_BPORCH  equ 152
; H_VISIBLE equ 800
; V_FPORCH  equ 1
; V_SYNC    equ 3
; V_BPORCH  equ 27
; V_VISIBLE equ 600

; 1024x768@59Hz (64Mhz)
; F_MUL     equ 8
; H_FPORCH  equ 24
; H_SYNC    equ 136
; H_BPORCH  equ 160
; H_VISIBLE equ 1024
; V_FPORCH  equ 3
; V_SYNC    equ 6
; V_BPORCH  equ 29
; V_VISIBLE equ 768

; 1024x768@67Hz (72Mhz)
; F_MUL     equ 9
; H_FPORCH  equ 24
; H_SYNC    equ 136
; H_BPORCH  equ 144
; H_VISIBLE equ 1024
; V_FPORCH  equ 3
; V_SYNC    equ 6
; V_BPORCH  equ 29
; V_VISIBLE equ 768

; 1024x768@76Hz (80Mhz)
; F_MUL     equ 10
; H_FPORCH  equ 16
; H_SYNC    equ 96
; H_BPORCH  equ 176
; H_VISIBLE equ 1024
; V_FPORCH  equ 1
; V_SYNC    equ 3
; V_BPORCH  equ 28
; V_VISIBLE equ 768

  org $8000
codestart:

    di

    FT_CMD FT_CMD_RST_PULSE
    FT_DELAY 65535
    FT_DELAY 65535
    FT_DELAY 65535

    FT_CMD FT_CMD_CLKEXT
    FT_DELAY 65535
    FT_CMD FT_CMD_SLEEP
    FT_DELAY 65535
    FT_CMDP FT_CMD_CLKSEL, F_MUL | 192
    FT_ACTIVE
    FT_ACTIVE
    FT_DELAY 65535

    FT_WR8 FT_REG_PCLK, 1
    FT_WR8 FT_REG_PCLK_POL, 0    FT_WR8 FT_REG_CSPREAD, 0      ; This is critical for correct colors display
    FT_WR16 FT_REG_HSYNC0,  H_FPORCH
    FT_WR16 FT_REG_HSYNC1,  H_FPORCH + H_SYNC    FT_WR16 FT_REG_HOFFSET, H_FPORCH + H_SYNC + H_BPORCH    FT_WR16 FT_REG_HSIZE,   H_VISIBLE    FT_WR16 FT_REG_HCYCLE,  H_FPORCH + H_SYNC + H_BPORCH + H_VISIBLE
    FT_WR16 FT_REG_VSYNC0,  V_FPORCH - 1
    FT_WR16 FT_REG_VSYNC1,  V_FPORCH + V_SYNC - 1
    FT_WR16 FT_REG_VOFFSET, V_FPORCH + V_SYNC + V_BPORCH - 1    FT_WR16 FT_REG_VSIZE,   V_VISIBLE
    FT_WR16 FT_REG_VCYCLE,  V_FPORCH + V_SYNC + V_BPORCH + V_VISIBLE
    FL_LOAD_DL DL1, DL1_

    FT_WR8 FT_REG_DLSWAP, FT_DLSWAP_FRAME

    ei
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
