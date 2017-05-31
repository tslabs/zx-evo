
  output "example00.bin"
  include "tsconfig.asm"

VPG equ 32

    org $8000

    ; load font
    ld bc, PAGE3
    ld a, VPG ^ 1
    out (c), a

    ld hl, font
    ld de, #c000
    ld bc, 2048
    ldir

    ; turn on textmode 80x30
    ld bc, VCONFIG
    ld a, VID_320X240 | VID_TEXT
    out (c), a

    ; turn on videopage
    ld bc, VPAGE
    ld a, VPG
    out (c), a

    ; turn on page at #c000
    ld bc, PAGE3
    out (c), a

    ; clear screen
    ld hl, #c000
    ld de, #c001
    ld bc, #3fff
    ld (hl), l
    ldir

    ; print text
    ld de, txt1
    ld hl, #c000
    ld c, #07   ; set attribute ink 7, paper 0
    call print

    ld de, txt2
    ld hl, #c100
    ld c, #0c   ; set attribute ink 4, bright 1, paper 0
    call print

    ld de, txt3
    ld hl, #dd00
    ld c, #70   ; set attribute ink 0, paper 7
    call print

  jr $

print:
    ld a, (de)
    or a
    ret z
    ld (hl), a
    set 7, l
    ld (hl), c
    res 7, l
    inc l
    inc de
    jr print

txt1:
    defb "Hello in TS-Conf text mode", 0

txt2:
    defb "Next line, green", 0

txt3:
    defb "Bottom line, inverse colors", 0

font:
    incbin "866_code.fnt"
