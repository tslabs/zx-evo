
	NAME CSTARTUP
	EXTERN main
	EXTERN tsbios
	EXTERN tsfat
	EXTERN biosapi
	EXTERN IM1_ISR
    
	PUBLIC DWT

#include "conf.asm"
#include "macro.asm"

; -----------------------------------
; -- Reset entry

    RSEG START

    di
    ld sp, stackp
    jp tsbios
    ; jp entry

; -----------------------------------
; -- Restarts

; RST 8: wait for DMA end
    RSEG RST08
DWT:
    xtbc dstatus
DWT1:
    inf
    ret p
    jr DWT1
    
; RST 10: TS-FAT driver entry
    RSEG RST10
    jp tsfat

; RST 18: TS-BIOS API entry
    RSEG RST18
    jp biosapi

    RSEG RST20
    ret

    RSEG RST28
    ret

    RSEG RST30
    ret

; RST 38: INT IM1 Handler

    RSEG RST38
    jp IM1_ISR
    
; -----------------------------------
; -- NMI Handler
    
    RSEG NMISR
    
    retn

; -----------------------------------
; -- Init entry

    RSEG UDATA0
	RSEG IDATA0
	RSEG ECSTR

	RSEG CDATA0
	RSEG CCSTR

	RSEG RCODE

seg_init:
; Zero out uninitialized static vars
	ld	bc, SFE(UDATA0) - SFB(UDATA0)
    ld a, b
    or c
    jr z, l1
    ld	hl, SFB(UDATA0)
    ld (hl), 0
    dec bc
    ld a, b
    or c
    jr z, l1
    ld e, l
    ld d, h
    inc de
    ldir
l1:

; Copy initialized static vars
	ld	bc, SFE(CDATA0) - SFB(CDATA0)
    ld a, b
    or c
    jr z, l2
	ld	hl, SFB(CDATA0)
	ld	de, SFB(IDATA0)
	ldir
l2:

; Copy writable string literals
	ld	bc, SFE(CCSTR) - SFB(CCSTR)
    ld a, b
    or c
    ret z
	ld	hl, SFB(CCSTR)
	ld	de, SFB(ECSTR)
	ldir
    ret

	end
