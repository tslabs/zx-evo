;;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
	.module     crt0
;;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    .globl      _main
    .globl      l__INITIALIZER
    .globl      s__INITIALIZED
    .globl      s__INITIALIZER
;;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
    TS_VCONFIG      .equ    #0x00
    TS_STATUS       .equ    #0x00
    TS_VPAGE        .equ    #0x01
    TS_GXOFFSL      .equ    #0x02
    TS_GXOFFSH      .equ    #0x03
    TS_GYOFFSL      .equ    #0x04
    TS_GYOFFSH      .equ    #0x05
    TS_TSCONFIG     .equ    #0x06
    TS_PALSEL       .equ    #0x07
    TS_BORDER       .equ    #0x0F
    TS_PAGE0        .equ    #0x10
    TS_PAGE1        .equ    #0x11
    TS_PAGE2        .equ    #0x12
    TS_PAGE3        .equ    #0x13

    TS_VID_256X192  .equ    #0x00
    TS_VID_320X200  .equ    #0x40
    TS_VID_320X240  .equ    #0x80
    TS_VID_360X288  .equ    #0xC0

    TS_VID_ZX       .equ    #0x00
    TS_VID_16C      .equ    #0x01
    TS_VID_256C     .equ    #0x02
    TS_VID_TEXT     .equ    #0x03
    TS_VID_FT812    .equ    #0x04
    TS_VID_NOGFX    .equ    #0x20
;;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
   .area    _HEADER (ABS)
   .org     0
    ld      bc,#0x21AF      ; MEMCONFIG
    ld      a,#0x0E         ; W0RAM | W0MAP_N | W0WE
    out     (c), a
; TS_PAGE0
    ld      b,#TS_PAGE0
    xor     a
    out     (c),a
    jp      init

    .org    0x18
    ei
    ret
    .org    0x20
    ei
    ret
    .org    0x28
    ei
    ret
    .org    0x30
    ei
    ret
    .org    0x38
    ei
    ret

    .org    0x100
init:
    ld      sp,#0xC000
    ld      bc,#0x20AF  ; SYSCONFIG
    ld      a,#0x06     ; ZCLK14 | CACHEEN
    out     (c), a

; TS_GXOFFSL
    xor     a
    ld      b,#TS_GXOFFSL
    out     (c),a
; TS_GXOFFSH
    inc     b
    out     (c),a
; TS_GYOFFSL
    inc     b
    out     (c),a
    xor     a
; TS_GYOFFSH
    inc     b
    out     (c),a

; VPage
    ld      b,#TS_VPAGE
    ld      a,#0x80
    out     (c),a

; PAGE3
    ld      b,#TS_PAGE3
    out     (c),a

; Vconfig
;    ld      b,#TS_VCONFIG
;    ld      a,#TS_VID_256X192 | #TS_VID_TEXT
;    ld      a,#TS_VID_320X240 | #TS_VID_TEXT
;    out     (c),a

; clear memory
    ld      hl,#0x8000
    ld      d,h
    ld      e,l
    ld      c,l
    ld      b,h
    inc     e
    ld      (hl),l
    dec     bc
    ldir

    call    gsinit
    jp      _main

gsinit::
    ld      bc,#l__INITIALIZER
    ld      a,b
    or      a,c
    ret     z
    ld      de,#s__INITIALIZED
    ld      hl,#s__INITIALIZER
    ldir
    ret

;; Ordering of segments for the linker.
    .area   _HOME
    .area   _CODE
    .area   _INITIALIZER
    .area   _GSINIT
    .area   _GSFINAL
    .area   _DATA
    .area   _INITIALIZED
    .area   _BSEG
    .area   _BSS
    .area   _HEAP

    .area   _CODE

    .area   _GSINIT

    .area   _GSFINAL
