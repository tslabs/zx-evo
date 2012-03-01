
; ------- MACRO definitions

; -- port macros

; write to F7
outf7   macro reg, val
        ld bc, reg << 8 + pf7
        ld a, val
        out (c), a
        endm


; read from F7
inf7    macro reg
        ld bc, reg << 8 + pf7
        in a, (c)
        endm


; read from AF
inxt    macro reg
        ld bc, reg << 8 + extp
        in a, (c)
        endm


; write imm8 to XTport
xt      macro port, val
        xtp port
        xtv val
        out (c), a
        endm


; write imm16 to adjacent XTports
xtw     macro port, val
        xtp port
__val1  defl low(val)
        xtv __val1
        out (c), a
__val1  defl high(val)
        xtv __val1
__prt   defl port + 1
        inc b
        out (c), a
        endm


; write A to XTport
xta     macro port
        xtp port
__val 	defl -1
        out (c), a
        endm


; load XTport to BC directly
xtbc    macro port
__xtp 	defl extp
__prt   defl port
        ld bc, port << 8 + extp
        endm


; reset flags
xtr     macro
__xtp 	defl -1
__prt 	defl -1
__val 	defl -1
        endm


; manage port
xtp     macro port
        .if __xtp <> extp
__xtp       defl extp
            .if __prt <> port
                ld bc, port << 8 + extp
__prt       defl port
            .else
                ld c, extp
            .endif

        .else
            .if __prt <> port
__prt           defl port
                ld b, port
            .endif
		.endif
        endm


; manage value
xtv     macro val
		.if val <> __val
__val 	defl val
            .if val = 0
                xor a
            .else
                ld a, val
            .endif
		.endif
        endm


; -- procedure macros

chr     macro symb
        ld a, symb
        call SYM
        endm


pmsg    macro addr, xy, attr
        ld de, addr
        ld hl, xy
        ld b, attr
        call PRINT_MSG
        endm


pmsgc   macro addr, yc, attr
        ld de, addr
        ld h, yc
        ld b, attr
        call PRINT_MSG_C
        endm


num     macro numb
        ld a, numb
        call NUM_10
        endm


box     macro xy, xys, attr
        ld hl, xy
        ld bc, xys
        ld a, attr
        call DRAW_BOX
        endm


hex8    macro hx
hxh     defl (hx >> 4) & h'F
        .if hxh > 9
        defb hxh + '0' + 7
        .else
        defb hxh + '0'
        .endif
hxl     defl hx & h'F
        .if hxl > 9
        defb hxl + '0' + 7
        .else
        defb hxl + '0'
        .endif
        endm
        
        
dec8    macro dc
        defb (dc / 10) + '0'
        defb (dc % 10) + '0'
        endm
