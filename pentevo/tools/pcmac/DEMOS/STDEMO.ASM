var loop,st
limit	equ	100
macro("FILL")
#stack st
loop	:= limit
#while loop
#push st,loop
loop := loop -1
#wend

endm

macro("DEFINE")
#while tos(st) != limit
	dw	pop(st)
#wend
#clostack	st
endm

	FILL
	DEFINE
