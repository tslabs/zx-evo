#include	"apu.mac"

		org     0

		out 0 (apu_border), r0
		out 1 (apu_covox), r0
		out 3 (apu_data_w), r0
		out 0 (apu_addr), r0
		out 0 (sram_addr), r15
l2:

		wait for cend
		wait not cbeg

		jr l2
		jr equal l1
		jr z l1
		jr not_equal l1
		jr nz l1
		
		wait for vsync
l1:
		wait not hsync_start
		
		halt
		
		dec r0
		dec r15
		dec 8 r0
		dec 32 r0

		inc r0
		inc r15
		inc 8 r0
		inc 32 r0

		xor r0, r0
		xor 32 r0, r0
		tst r0, r0
		tst r0, 10h
		tst 32 r0, r0

		add r0, r0
		add 8 r0, r0
		add 16 r0, r0
		add 32 r0, r0
		cmp r0, r0
		
		and r0,r0
		and r15, r15
		and 16 r0, r0
		
		ld r1, r2
		ld 8 r0, r0
		ld 16 r1, r15
		ld 24 r14, r15
		ld 32 r15, r0
		
		ld r0, 0
		ld r1, 16
		ld r10, 240
		
		and r1, 15
		xor r9, 7
		sub r10, 100
		or r0, 255
		