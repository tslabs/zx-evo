
; ------- MACRO definitions

; write imm8 to XTport
  macro wrxt port, val
        ld bc, port
        ld a, val
        out (c), a
        endm

; write A to XTport
  macro wrxta port
        ld bc, port
        out (c), a
        endm

; write imm8 to F7
  macro outf7 reg, val
        ld bc, reg << 8 | PF7
        ld a, val
        out (c), a
        endm

; read imm8 from XTport
  macro rdxt port
        ld bc, port
        in a, (c)
        endm

; print message
  macro pmsgc addr, yc, attr
        ld de, addr
        ld h, yc
        ld b, attr
        call PRINT_MSG_C
        endm

; draw box
  macro box xy, xys, attr
        ld hl, xy
        ld bc, xys
        ld a, attr
        call DRAW_BOX
        endm

; convert byte to 2-digit ASCII 10-base number
  macro dec8 dc
        defb (dc / 10) + '0'
        defb (dc % 10) + '0'
        endm

; convert byte to 2-digit ASCII 16-base number
  macro hex8 dc
        defb (dc >> 4) + (((dc >> 4) > 9) & 1) * 7 + '0' 
        defb (dc & 15) + (((dc & 15) > 9) & 1) * 7 + '0' 
        endm

; ORG with pad
  macro orgp addr
        defs addr - $, 0x55
        endm
