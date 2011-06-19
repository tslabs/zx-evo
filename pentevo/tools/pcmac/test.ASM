#include	"apu.mac"

		org     0

		in r0, (sram_data)
		in r0, (vcnt)
		in 0 r0, (sram_data)
		in 3 r0, (sram_data)
		
		out 0 (border), r0
		out 1 (covox), r0
		out 3 (dram_data), r0
		out 0 (dram_addr), r0
		out 0 (sram_addr), r15
l2:

		wait for none
		wait not none

		jr l2
		jr equal l1
		jr z l1
		jr not_equal l1
		jr nz l1
		
		wait for vsync
l1:
		wait not hsync_start
		
		halt
		
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
		
		wait cbeg
		wait post_cbeg
		wait pre_cend
		wait cend
		
		wait hblank
		wait hblank, cbeg
		wait for hblank, cbeg
		wait not hblank, cbeg
		
		inc r0
		inc r15
		inc 8 r0
		inc 32 r0

		dec r0
		dec r15
		dec 8 r0
		dec 32 r0

		cpl r0
		cpl 8 r0
		cpl 32 r0
		cpl r15
		
		neg r0
		neg 8 r0
		neg 32 r0
		neg r15
		
		cmp r0, 0
		cmp r0, 255

		tst r0, 0
		tst r0, 255
		
		halt
		
		wait (hcnt) r0
		wait (vcnt) r0
		wait (vcnt) r15
		wait 0 (vcnt) r15
		wait 1 (vcnt) r15

		wait (vcnt) is r0
		wait (vcnt) less r0
		wait (vcnt) greater r0
		wait (vcnt) not r0
		
		wait 0 (vcnt) is r0
		wait 0 (vcnt) less r0
		wait 0 (vcnt) greater r0
		wait 0 (vcnt) not r0
		wait 1 (vcnt) not r0
		wait 2 (vcnt) not r0
		wait 3 (vcnt) not r0
		
		
		
		