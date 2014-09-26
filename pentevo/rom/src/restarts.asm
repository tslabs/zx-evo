
; -- RST 08-30

; wait for DMA end
        org DWT         ; 8
        xtbc dstatus
DWT1    inf
        ret p
        jr DWT1

; TS-FAT driver entry
        org TSF         ; 16
        jp TSFAT

; TS-BIOS API entry
        org BAPI        ; 24
        jp BIOSAPI

        org h'20
        ret

        org h'28
        ret

        org h'30
        ret

; -- IM1 INT
        org h'38
IM1
        push bc
        push de
        push hl
        push af
        call KBD_PROC
        ld a, (evt)
        or a            ; was last event processed?
        jr nz, IM11     ; not yet - return
        call KBD_EVT
        ld a, b
        ld (evt), a
IM11
        pop af
        pop hl
        pop de
        pop bc
        ei
        ret


; -- NMI
        org h'66
NMI
        retn
