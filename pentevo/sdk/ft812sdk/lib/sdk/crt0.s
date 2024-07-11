
  .module crt0
  .globl  _main
  .globl      s__DATA
  .globl      l__DATA
  .globl      s__INITIALIZED
  .globl      s__INITIALIZER
  .globl      l__INITIALIZER

  .area  _HEADER (ABS)
  .org   0
  di
  
  ; enable cache and 14MHz
  ld bc, #0x20AF  ; SYSCONFIG
  ld a, #0x06     ; ZCLK14 | CACHEEN
  out (c), a
  
  ; turn page RAM at #0000
  ld b, #0x21     ; MEMCONFIG
  ld a, #0x0E     ; W0RAM | W0MAP_N | W0WE
  out (c), a
  
  ; set RAM pages sequentially
  ld b, #0x10
  ld a, #2
  out (c), a      ; PAGE0 = 2
  jp l0
l0:
  inc b        
  xor a
  out (c), a      ; PAGE1 = 0
  inc b           
  inc a
  out (c), a      ; PAGE2 = 1
  
  ld sp, #0xC000
  call init
  im 1
  jp _main

  .org  0x38
  ei
  ret

  .org  0x3B
init:
  ; init vars
  ld hl, #s__DATA
  ld bc, #l__DATA
l1:
  ld a, b
  or c
  jr z, l2
  ld (hl), #0
  dec bc
  ld a, b
  or c
  jr z, l2
  ld e, l
  ld d, h
  inc de
  ldir
  
l2:
  ld bc, #l__INITIALIZER
  ld a, b
  or c
  ret z
  ld  hl, #s__INITIALIZER
  ld  de, #s__INITIALIZED
  ldir
  ret
  
  ; Ordering of segments for the linker.
  .area _HOME
  .area _CODE
  .area _INITIALIZER
  .area _GSINIT
  .area _GSFINAL

  .area _DATA
  .area _INITIALIZED
  .area _BSEG
  .area _BSS
  .area _HEAP

  .area _GSINIT
  .area _GSFINAL
