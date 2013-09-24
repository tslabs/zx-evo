	.globl	_main

	.area _HEADER (ABS)

	.org    0x0000	;--code-loc 0x0006

init:
	call gsinit
	jp   __pre_main

	.area	_CODE

__pre_main:
	push de
	ld de,#_HEAP_start
	ld (_heap_top),de
	pop de
	call _main
	di
	halt

	.area	_DATA

_heap_top::
	.dw 0

gsinit: .area   _GSINIT

	.area   _GSFINAL
	ret

	.area	_HEAP

_HEAP_start::
