
; APU project for Pentevo
; This should draw a color bars at the first 64 pixels of border
; starting from line 100, 40 lines in total
; (c) TS-Labs, 2011

#include	"apu.mac"
		
		org 0
		
		ld r2,100
l1:		ld r0, 0
		ld r1, 40
		wait (vcnt) r2

l3:		wait line_start
								;		tacts count
l2:		out (border), r0		;	0/4/8 - 7MHz pixels
		inc r0					;	1
		tst r0, 64				;	2
		jr z l2					;	3

		dec r1
		jr nz l3
		
		jr l1
		
		