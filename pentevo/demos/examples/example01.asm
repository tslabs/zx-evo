	
	include "tsconfig.asm"
	
	org 32768
	
	ld h, high VPAGE
	
	output "example01.bin"
