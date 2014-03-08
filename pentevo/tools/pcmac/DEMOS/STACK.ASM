;STACK.ASM
;Demonstration program for the usage of stack directives

NONADDRESS EQU	-1 ;Any value that can not represent address

#list off

;A simple jump instruction
macro("JP *",NUMERIC)
	DB	0C3h
	DW	#0
endm

;LOOP macro definition nestable with break
macro("LOOP")
#ifndef loop_stack
var loop_stack,code_counter_store,address
#stack loop_stack
#endif
#push loop_stack , NONADDRESS
#push loop_stack , $
endm

;BREAK macro definition
macro("BREAK")
	JP	0 ; We do not know the address now
#push loop_stack , $-2  ;Store the address of
			;the second byte of the instruction
endm

;ENDLOOP macro definition that fills in the BREAK jumps
macro("ENDLOOP")
code_counter_store := $
address := pop(loop_stack)
#while tos(loop_stack) != NONADDRESS
$ := address
	DW code_counter_store+3 ; 3 is the length of
			  ; the jump-back instruction
address := pop(loop_stack)
#wend
$ := code_counter_store
	JP	address
#pop	loop_stack
endm

#if pass = 2
#list on
#endif

	LOOP      ;*  loop #1
	LOOP      ; * loop #2
	BREAK
	BREAK
	ENDLOOP   ; * endloop #2
	BREAK
	LOOP      ; * loop #3
	BREAK
	ENDLOOP   ; * endloop #3
	BREAK
	ENDLOOP   ;* endloop #1
;End of file STACK.ASM