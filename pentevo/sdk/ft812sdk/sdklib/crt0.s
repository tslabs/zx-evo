
	.module crt0
	.globl	_main

	.area	_HEADER (ABS)
	.org 	0
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
  xor a
  out (c), a      ; PAGE0 = 0
  inc b        
  inc a
  out (c), a      ; PAGE1 = 1
  inc b           
  inc a
  out (c), a      ; PAGE2 = 2
  
  ; run main()
  ld sp, #0xC000
  im 1
  jp _main

	.org	0x38
  ei
  ret

	;; Ordering of segments for the linker.
	.area	_HOME
	.area	_CODE
	.area	_INITIALIZER
	.area   _GSINIT
	.area   _GSFINAL

	.area	_DATA
	.area	_INITIALIZED
	.area	_BSEG
	.area   _BSS
	.area   _HEAP

	.area   _GSINIT
	.area   _GSFINAL

