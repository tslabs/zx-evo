
	.module crt0
	.globl	_main

	.area	_HEADER (ABS)
	.org 	0x8000
  jp _main

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

