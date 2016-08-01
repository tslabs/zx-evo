
FS_BUF_SIZE equ 4096
MF_ADDR     equ 0
MF_SIZE     equ 4096
PIC_ADDR    equ MF_ADDR + MF_SIZE

    device zxspectrum128

    include "wc_kernel_h.asm"
    include "tsconfig.asm"

; --------------------
; .text
startCode
    org #0000
    include "wc_h.asm"

    align 512
    disp #8000

mainStart
    jp start

    include "ft_func.asm"

start
    ld (filesize), hl
    ld (filesize + 2), de

    ; check if this is a first run and FT812 init is needed, to avoid unnecessary display switchover between pictures
    ld a, (is_first_run)
    or a
    jr nz, .no_init

    ; init FT812
    ld hl, vm_1024_768_59Hz
    ; ld hl, vm_800_600_60Hz
    call ft_init
    ; detect FT812
    call ft_detect
    jp nz, error_no_ft

    ld a, 1
    ld (is_first_run), a

.no_init
    call show

    ; wait for user key
.waitkey
    halt
    wcapi _ENKE
    ld a, 2         ; next file request
    ret nz            ; show next file
    wcapi _ESC
    ld a, 0         ; exit signal
    ret nz            ; exit to commander
    jr .waitkey

; --------------------
; display error 'FT812 Not Found'
error_no_ft
    ld ix, err_no_ft
    wcapi _PRWOW

.waitkey
    halt
    wcapi _ENKE
    jr nz, .exit
    wcapi _ESC
    jr z, .waitkey

.exit
    ld ix, err_no_ft
    wcapi _RRESB

    xor a             ; exit signal
    ret

; --------------------
; check which file extension is opened
extension_check
    ld de, fs_entry
    wcapi _TENTRY

    ld hl, extlist
    ld de, fs_entry+8
    xor a

.c0
    ld b, 3
    call cmpstrn
    ret c
    inc hl
    inc hl
    inc hl
    inc a
    jr .c0

; --------------------
; compare fixed length string
; in:
;    HL - string1*
;    DE - string2*
;    B - length
; out:
;    C - match, NC - no match
cmpstrn
    push hl
    push de
    push af

.c0
    ld a, (de)
    cp (hl)
    jr nz, .exit
    inc hl
    inc de
    djnz .c0

    pop af
    pop de
    pop hl
    scf
    ret

.exit
    pop af
    pop de
    pop hl
    or a
    ret

; --------------------
; display file by extension
show
    call extension_check
    or a
    jr z, show_jpg
    dec a
    jr z, show_png
    dec a
    jp z, show_avi
    jp show_dls

; --------------------
; in:
;  HL - memory address
load_chunk
    ld bc, FS_BUF_SIZE / 2
    push bc
    call load_mediafifo
    pop bc
    ret c

    ; check for end of file
    ld hl, (filesize)
    or a
    sbc hl, bc
    jr nc, .c1
    ex de, hl
    ld hl, (filesize + 2)
    ld bc, 0
    sbc hl, bc
    ret c
    ld (filesize + 2), hl
    ex de, hl
.c1
    ld (filesize), hl
    ret

; --------------------
; JPEG/PNG viewer
show_jpg
show_png
    call ft_screen_on
    call ft_copr_reset

    ; start DL
    ld hl, pic_dl
    ld bc, pic_dl_ - pic_dl
    call ft_load_copr
    ft_delay 2  ; !!! emulator crutch

.c0
    ld hl, fs_buf
    ld b, FS_BUF_SIZE / 512
    wcapi _LOAD512

    ld hl, fs_buf
    call load_chunk
    ret c
    ld hl, fs_buf + FS_BUF_SIZE / 2
    call load_chunk
    ret c
    jr .c0

; --------------------
; DLS viewer
show_dls
    call ft_screen_on

    ld hl, fs_buf
    ld b, FS_BUF_SIZE / 512
    wcapi _LOAD512

    ld de, 0
    ld hl, fs_buf
    ld bc, (filesize)
    call ft_load_dl
    ft_wr8 FT_REG_DLSWAP, FT_DLSWAP_FRAME
    ret

; --------------------
; AVI player
show_avi
    call ft_screen_on

    ; +++
    ret

; --------------------
ft_screen_on
    ld a, %10000111    ; FT812 screen
    exa
    ld a, _GVmod
    jp _WCAPI

; --------------------
; .rdata
pic_dl
    dd FT_CMD_DLSTART
    FT_CLEAR_COLOR_RGB 0, 0, 0
    FT_CLEAR 1, 1, 1
    FT_DISPLAY
    dd FT_CMD_SWAP

    dd FT_CMD_DLSTART
    FT_CLEAR_COLOR_RGB 64, 64, 64
    FT_CLEAR 1, 1, 1
    ; FT_BITMAP_SOURCE 0
    ; FT_BITMAP_LAYOUT_H 2, 1
    ; FT_BITMAP_LAYOUT 7, 0, 0
    ; FT_BITMAP_SIZE_H 2, 1
    ; FT_BITMAP_SIZE 0, 0, 0, 0, 0

    dd FT_CMD_MEDIAFIFO
    dd MF_ADDR
    dd MF_SIZE

    dd FT_CMD_LOADIMAGE
    dd PIC_ADDR
    ; dd FT_OPT_MEDIAFIFO
    dd FT_OPT_MEDIAFIFO | FT_OPT_FULLSCREEN

    FT_BEGIN FT_BITMAPS
    FT_VERTEX2F 0, 0
    FT_END

    ; db 0x0c, 0xff, 0xff, 0xff, 0xf0, 0x00, 0x88, 0x00, 0x1a, 0x00, 0x00, 0x06
      ; db 0x44, 0x69, 0x73, 0x70
      ; db 0x6c, 0x61, 0x79, 0x20
      ; db 0x62, 0x69, 0x74, 0x6d
      ; db 0x61, 0x70, 0x20, 0x62
      ; db 0x79, 0x20, 0x6a, 0x70
      ; db 0x67, 0x20, 0x64, 0x65
      ; db 0x63, 0x6f, 0x64, 0x65
      ; db 0x00, 0x00, 0x00, 0x00

    FT_DISPLAY
    dd FT_CMD_SWAP
pic_dl_

extlist
    db "JPGPNGAVIDLS"

err_no_ft
    db 1                            ; window with header and text
    db 0                            ; cursor color mask
    dw 0x0A18                 ; YX (position)
    dw 0x0520                 ; HW (size)
    db 0x2F                     ; paper/ink (window color)
    db 0                            ; -reserved-
    dw 0x0000                 ; window restore buffer address
    dw 0x0000                 ; dividers
    dw err_no_ft_hdr    ; header text address
    dw err_no_ft_ftr    ; footer text address
    dw err_no_ft_txt    ; window text address

err_no_ft_hdr
    db "Error!",0
err_no_ft_txt
    db 13, "             FT812 not found!",0
err_no_ft_ftr
    db "Press ESC or Enter",0

; --------------------
; .data (bss)
filesize      dd 0
is_first_run  db 0
mainEnd

; ----------------
; .data (noinit)
fs_entry            equ $ : org $ + 32
    org (($ - 1) & 0xFE00) + 512    ; aka ALIGN 512
fs_buf                equ $ : org $ + FS_BUF_SIZE

    ent
endCode
    savebin "ftview.wmf", startCode, endCode - startCode
