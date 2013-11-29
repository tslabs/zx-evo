
; APU project for Pentevo
; This should draw a color bars at the first 64 pixels of border in each line
; (c) TS-Labs, 2011

#include	"apu.mac"
		
		org 0
		
l1:		ld r0, 0
		wait line_start
								;		tacts count
l2:		out (border), r0	;	0/4/8 - 7MHz pixels
		inc r0					;	1
		tst r0, 64				;	2
		jr z l2					;	3
		
		jr l1
		
		