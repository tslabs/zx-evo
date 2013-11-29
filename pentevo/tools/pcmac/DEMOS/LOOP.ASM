;LOOP.ASM
;Demo program for looping directives

;This file contains two macro definitions
;for the same purpose. The first version (DEFS)
;works when the argument is 0. The second one
;works only for argument greater then zero.
;This program can not be compiled because
;the last macro tries to generate 100000000 bytes.
;I don't think you can wait until it finishes.
;If you start the compilation of this program
;do use the option -l and when you are tired 
;looking the zeroes press CTRL+BREAK!

macro("DEFS *",NUMERIC)
#ifndef defs
	var defs
#endif
defs := #0
#while defs>0
	DB 0
	defs := defs - 1
#wend
endm

macro("SKIP *",NUMERIC)
#ifndef skip
	var skip
#endif
skip := #0
#repeat
	DB 0
	skip := skip - 1
#until skip=0
endm
	DEFS	10
	SKIP	10
	DEFS	0
	SKIP	0
;End of file LOOP.ASM
