
	.module crt0
	.globl	_main

	.area	_HEADER (ABS)
	.org 	0
  di
  
  ; turn page RAM at #0000
  ld bc, #0x21AF  ; MEMCONFIG
  ld a, #0x0E     ; W0RAM | W0MAP_N | W0WE
  out (c), a
  
  ; set RAM pages sequentially
  ld bc, #0x10AF
  ld a, #8
  out (c), a      ; PAGE0 = 8
  inc b        
  inc a
  out (c), a      ; PAGE1 = 9
  inc b           
  inc a
  out (c), a      ; PAGE2 = 10
  
  ; run main()
  ld sp, #0xC000
  ei
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

