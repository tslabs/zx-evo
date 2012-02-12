
; ------- MACRO definitions


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

        
; write [addr]8 to XTport 
xta     macro port, addr
        xtp port
__val 	defl -1
		ld a, (addr)
        out (c), a
        endm

        
; write [addr]16 to XTport 
xthl    macro port, addr
        xtp port
		
		ld hl, (addr)
        out (c), l
__prt   defl port + 1
        inc b
        out (c), h
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
      
