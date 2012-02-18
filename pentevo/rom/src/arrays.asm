
; ------- messages

M_HEAD1
        defb 'TS-BIOS Setup Utility', 0
M_HEAD2
        defb 'Build date:', 0
MH2_SZ  equ $ - M_HEAD2 + 18

M_HLP   defb 'Arrows - move,  Enter - change option,  F12 - exit', 0


; ------- tabs
; -- options tab
OPTTAB0
; byte - X, Y coord of box
; byte - X, Y size of box
; byte - X, Y coord of list top
; byte - X, Y coord of header text
; byte - number of options
; string - message
        defw h'0707
        defw h'0B20
        defb h'0A, h'0A
        defb h'0C, h'08
        defb h'8C
        defb 6
        defb 'Select NVRAM options:', 0        
; word - address of option desc
; word - address of option choises
		defw OPT0, SEL0
		defw OPT1, SEL1
		defw OPT5, SEL1
		defw OPT2, SEL2
		defw OPT3, SEL3
		defw OPT4, SEL4
		; defw OPT5, SEL5
		; defw OPT6, SEL6
		; defw OPT7, SEL7
		
; -- option text
; byte - number of choises
; byte - address in NVRAM
; string - option
; byte - 0
OPT0    defb 3, low(cfrq), 'CPU speed, MHz:', 0
OPT1    defb 8, low(btto), 'Boot to:', 0
OPT5    defb 8, low(b2to), 'CS Boot to:', 0
OPT2    defb 4, low(l128), '128k Lock:', 0
OPT3    defb 4, low(ayfr), 'AY clock, MHz:', 0
OPT4    defb 6, low(zpal), 'ZX Palette:', 0
; OPT5    defb 0, h'B0, 0, 0, '', 0
; OPT6    defb 0, h'B0, 0, 0, '', 0
; OPT7    defb 0, h'B0, 0, 0, '', 0

; -- choises
; *repeat n times
; string - choise
; byte - 0
SEL0
		defb ' 3.5', 0
        defb ' 7.0', 0
		defb '14.0', 0
		
SEL1    
		defb '  Basic 48', 0
		defb ' Basic 128', 0
        defb '    TR-DOS', 0
		defb 'SD:boot.c$', 0
		defb 'SD:sys.rom', 0
		defb '    RS-232', 0
		defb 'ROM:#04-07', 0
		defb 'RAM:#F8-FB', 0
		
SEL2    
        defb '  OFF', 0
		defb '   ON', 0
		defb 'Auto1', 0
		defb 'Auto2', 0
		
SEL3    
        defb '1.750', 0
		defb '1.773', 0
		defb '3.500', 0
		defb '3.546', 0
		
SEL4    
        defb ' Pulsar', 0   ; 0
		defb 'B.black', 0   ; 1
		defb '   Pale', 0   ; 2
		defb '   Dark', 0   ; 3
		defb 'Grayscl', 0   ; 4
		defb ' Custom', 0   ; 5
		
; SEL5    defb '', 0
		; defb '', 0
		
; SEL6    defb '', 0
		; defb '', 0
		
; SEL7    defb '', 0
		; defb '', 0
        
        
; -- palette
pal_bb               ; bright black
        defw h'0000
        defw h'0010
        defw h'4000
        defw h'4010
        defw h'0200
        defw h'0210
        defw h'4200
        defw h'4210
        defw h'2108
        defw h'0018
        defw h'6000
        defw h'6018
        defw h'0300
        defw h'0318
        defw h'6300
        defw h'6318
        
pal_puls             ; pulsar
        defw h'0000
        defw h'0010
        defw h'4000
        defw h'4010
        defw h'0200
        defw h'0210
        defw h'4200
        defw h'4210
        defw h'0000
        defw h'0018
        defw h'6000
        defw h'6018
        defw h'0300
        defw h'0318
        defw h'6300
        defw h'6318

pal_dark              ; dark
        defw b'000000000000000
        defw b'000000000001000
        defw b'010000000000000
        defw b'010000000001000
        defw b'000000100000000
        defw b'000000100001000
        defw b'010000100000000
        defw b'010000100001000
        defw b'000000000000000
        defw b'000000000010000
        defw b'100000000000000
        defw b'100000000010000
        defw b'000001000000000
        defw b'000001000010000
        defw b'100001000000000
        defw b'100001000010000


pal_pale              ; pale
        defw b'010000100001000
        defw b'010000100010000
        defw b'100000100001000
        defw b'100000100010000
        defw b'010001000001000
        defw b'010001000010000
        defw b'100001000001000
        defw b'100001000010000
        defw b'010000100001000
        defw b'010000100011000
        defw b'110000100001000
        defw b'110000100011000
        defw b'010001100001000
        defw b'010001100011000
        defw b'110001100001000
        defw b'110001100011000

pal_gsc               ; grayscale
        defw 0  << 10 + 0  << 5 + 0
        defw 3  << 10 + 3  << 5 + 3
        defw 6  << 10 + 6  << 5 + 6
        defw 9  << 10 + 9  << 5 + 9
        defw 12 << 10 + 12 << 5 + 12
        defw 16 << 10 + 16 << 5 + 16
        defw 19 << 10 + 19 << 5 + 19
        defw 22 << 10 + 22 << 5 + 22
        defw 0  << 10 + 0  << 5 + 0
        defw 4  << 10 + 4  << 5 + 4
        defw 8  << 10 + 8  << 5 + 8
        defw 11 << 10 + 11 << 5 + 11
        defw 14 << 10 + 14 << 5 + 14
        defw 17 << 10 + 17 << 5 + 17
        defw 20 << 10 + 20 << 5 + 20
        defw 24 << 10 + 24 << 5 + 24
        
        
keys_norm
        defb 0, 'zxcvasdfgqwert1234509876poiuy', 13, 'lkjh ', 14, 'mnb'
keys_caps
        defb 0, 'ZXCVASDFGQWERT', 7, 6, 4, 5, 8, 12, 15, 9, 11, 10, 'POIUY', 13, 'LKJH ', 14, 'MNB'
keys_symb
        defb 0, ':`?/~|\\{}   <>!@#$%_)(', 39, '&', 34, '; ][', 13, '=+-^ ', 14, '.,*'