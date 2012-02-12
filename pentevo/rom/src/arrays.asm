
; ------- definitions

; -- addresses

nv_buf:       equ h'5B80
stck:         equ h'5BFF


; -- NVRAM cells
cfrq:	equ h'B0
btto:	equ h'B1
l128:	equ h'B2
ayfr:	equ h'B3		
zpal:	equ h'B4		
		

; ------- tabs
; -- options tab
; word - address of option text
; word - address of option choises
OPTTAB:
		defw OPT0, SEL0
		defw OPT1, SEL1
		defw OPT2, SEL2
		defw OPT3, SEL3
		defw OPT4, SEL4
		; defw OPT5, SEL5
		; defw OPT6, SEL6
		; defw OPT7, SEL7
		
		
; ------- messages
; -- option text
; byte - number of choises
; byte - address in NVRAM
; byte - X coordinate
; byte - Y coordinate
; text - option
; byte - 0
OPT0:	defb 3, cfrq, 0, 0, 'CPU frequency, MHz:', 0
OPT1:	defb 8, btto, 0, 0, 'Boot to:', 0
OPT2:	defb 4, l128, 0, 0, '128k Lock:', 0
OPT3:	defb 4, ayfr, 0, 0, 'AY frequency, MHz:', 0
OPT4:	defb 7, zpal, 0, 0, 'ZX Palette:', 0
; OPT5:	defb 0, h'B0, 0, 0, '', 0
; OPT6:	defb 0, h'B0, 0, 0, '', 0
; OPT7:	defb 0, h'B0, 0, 0, '', 0


; -- choises
; *repeat n times
; text - choise
; byte - 0
SEL0:	defb '7.0', 0
		defb '14.0', 0
		defb '3.5', 0
		
SEL1:	defb 'TR-DOS', 0
		defb 'Basic 48k', 0
		defb 'Basic 128k', 0
		defb 'SD: boot.c$', 0
		defb 'SD: system.rom', 0
		defb 'RS-232', 0
		defb 'Custom ROM (page #04-07)', 0
		defb 'Custom RAM (page #F8-FB)', 0
		
SEL2:	defb 'ON', 0
		defb 'OFF', 0
		defb 'Auto (boleq)', 0
		defb 'Auto (lvd)', 0
		
SEL3:	defb '1.75', 0
		defb '1.7733', 0
		defb '3.5', 0
		defb '3.546', 0
		
SEL4:	defb 'Pulsar', 0
		defb 'Bright black', 0
		defb 'Dim', 0
		defb 'Pale', 0
		defb 'Dark', 0
		defb 'Black and white', 0
		defb 'Custom', 0
		
; SEL5:	defb '', 0
		; defb '', 0
		
; SEL6:	defb '', 0
		; defb '', 0
		
; SEL7:	defb '', 0
		; defb '', 0