;		INCLUDE "tsconfig.asm"
		
SCREENPAGE EQU  0 ; по правилам WC
SCREENMODE EQU  %11000010; 360x288c256

wcABT   EQU     #6004
wcENT   EQU     #6005
wcTNM   EQU     #6009
wcIN    EQU     #6006


VideoMode		equ %11010001
Vid_page		equ 0 // 1й видео буфер (16 страниц)
data_page       equ 1
;Tile_page		equ #10 // 2й видео буфер
;Tile_spr_page	equ #18 // 2й видео буфер
;Tile0_spr_page	equ #20 // 3й видео буфер


;SFileAddr       EQU 512

twister_page	equ data_page ;equ 0x81 // 138

start		di
		call USPO		; wait keyoff
		call SetDefaultScreen

	        xor a			; Vid_page
		call MNGCVPL
		ld (clr_screen+1),a	; clr_pages
		ld (clr_screen+7),a
		ld hl,0
		ld (#c000),hl
		ld hl,scr_clr
		call set_ports

	        ld a,twister_page	; receive gfx page
		call MNGC_PL
	        ld (datapg+1),a		; page with gfx
		ld de,sinsin+#100
		ld bc,#8030
		call SINMAKE		; sin create

		ld hl,#c000		; fill line data
		ld xl,0
		ld b,255
datapg		ld a,0
twdb_fill	
		ld de,160
		add hl,de
		jr nc,twdb_fill1
		inc a
		ld h,#c0
twdb_fill1
		ld xh,high twmem_db
		ld (ix+0),l		; fill line adr
		inc xh
		ld (ix+0),h
		inc xh
		ld (ix+0),a		; fill line page
		inc ix
		djnz twdb_fill
		ld xh,high twmem_db
		ld (ix+0),l
		inc xh
		ld (ix+0),h
		inc xh
		ld (ix+0),a

		ld bc, 0x15AF		; set palitra
		ld a, 0x10
		out (c), a

		;load CRAM
		ld hl,twpal  ;some palette array address
		ld de, 0x0000 ;CRAM access window address
		ld bc, 32 ;any size here up to 512
		ldir

		;close FMAddr
		ld bc, 0x15AF
		xor a
		out (c),a

		ld b,#07		; palsel - 0
		out (c),a
		ld b,#20		; SysConfig - WOWTURBO ;) + cache
		ld a,6
		out (c),a
		ld b,#2b		; Cache ON for 8000 & 4000
		ld a,#0c
		out (c),a
        
        ei:halt
	    ld a,VideoMode	
        call SetVideoMode	; swith gfx on
        ld a,1
        call SetVideoPage	; video page

cycle	call DMAinit		; set init pos for output gfx
		call DMAdance		; plasssma
		halt
		call ENKE		; wait keypressed
		jr nz,waaa
	    jp cycle
	        
waaa		call SetDefaultScreen	; exit
	        xor a
	        ret

scr_clr
		defb #1a,0	;
		defb #1b,0	;
clr_screen	defb #1c,#20	;
		defb #1d,0	;
		defb #1e,0	;
		defb #1f,#20	;

		defb #28,216	;
		defb #26,360/2+1	;
		defb #27,%00000100
		db #ff
DMAinit
        ;surce(bhl)'n'dest(cde)
        ld b,%10000000+data_page,hl,0	; source
        ld c,Vid_page,de,10		; dest
        ld a,#00
        call DMATrans			; outs data for dma
        ;size&burstz
        ld c,(320/4)-1
	ld b,0
	ld a,#07
	call DMATrans
        
        ;Alignz
        ld e,0;16c
        ld h,0;source off
        ld l,1;dest on
        ld a,#0B
        call DMATrans
        ret


set_ports	ld c,#af
		ld a,(hl)
		cp #ff
		jr z,dma_stats
		ld b,a
		inc hl
		ld a,(hl)
		inc hl
		out (c),a
		jr set_ports+2

dma_stats	ld bc,#27af
		in a,(c)
		AND #80
		jr nz,$-4
		ret
DMAdance
sinc		ld hl,sinsin	; inc sin data's
		inc (hl)
		inc h
		inc (hl)
		inc l
		dec h
		ld (sinc+1),hl


lc2		ld a,0
		inc a
		ld (lc2+1),a
		ld (r1+1),a
		ld l,a
		add #6f
		add l
		ld (r2+1),a


		ld bc,288		; all lines on screen
lc1		push bc
r1		ld a,#01
		inc a
		ld ($-2),a
		ld h,high sinsin	
		ld l,a

		ld c,(hl)
		inc h
r2		ld a,#01
		dec a
		ld ($-2),a
		ld l,a
		ld a,(hl)
		add c			; WHAT IS IT! Shit! Fuck! 8)
		ld l,a

		ld h,high twmem_db
		ld e,(hl)		; line adr in mem
		inc h
		ld d,(hl)
		inc h
		ld a,(hl)		; line page 
		ld bc,#1aaf
		out (c),e
		inc b
		out (c),d
		inc b
		out (c),a
		ld b,#27		;send gfx
		ld a,%00010001
		out (c),a
		in a,(c)		; and wait
		AND #80
		jr nz,$-4
		pop bc
		dec bc
		ld a,b
		or c
		jr nz,lc1
		ret

SINMAKE INC     C
        LD      HL,SIN_DAT
        PUSH    BC
        LD      B,E
LP_SMK1 PUSH    HL
        LD      H,(HL)
        LD      L,B
        LD      A,#08
LP_SMK2 ADD     HL,HL
        JR      NC,$+3
        ADD     HL,BC
        DEC     A
        JR      NZ,LP_SMK2
        LD      A,H
        LD      (DE),A
        POP     HL
        INC     HL
        INC     E
        BIT     6,E
        JR      Z,LP_SMK1
        LD      H,D
        LD      L,E
        DEC     L
        LD      A,(HL)
        LD      (DE),A
        INC     E
LP_SMK3 LD      A,(HL)
        LD      (DE),A
        INC     E
        DEC     L
        JR      NZ,LP_SMK3
LP_SMK4 LD      A,(HL)
        NEG
        LD      (DE),A
        INC     L
        INC     E
        JR      NZ,LP_SMK4
        POP     BC
LP_SMK5 LD      A,(DE)
        ADD     A,B
        LD      (DE),A
        INC     E
        JR      NZ,LP_SMK5
	di
        RET

		
		
;=====================================================================
;=====================================================================
SetPage3:
        EXA
		xor a
		jp wcIN
        
SetPage3Video:
        EXA
        ld a,65
        jp wcIN

SetDefaultScreen:
        ld a,_GEDPL
        jp wcIN

Xoff    ld      a,68
        jp      wcIN
Yoff    ld      a,67
        jp      wcIN
    
UPPP    ld      a,17
        jp      wcIN
DWWW    ld      a,18
        jp      wcIN
LFFF    ld      a,19
        jp      wcIN
RGGG    ld      a,20
        jp      wcIN
    
ESC:
        ld      a,23
        jp      wcIN
ENKE:
        ld      a,45
        jp      wcIN

SetVideoPage:
        exa
        ld      a,_MNGV_PL
        jp      wcIN

SetVideoMode:
        exa
        ld      a,_GVmod
        jp      wcIN
SetTileMap:
        ld      a,_GVtm
        jp      wcIN
SetTlGP:		
        ld      a,_GVtl
        jp      wcIN

SetSpGP:		
        ld      a,_GVsgp
        jp      wcIN	

MNGCVPL:
        exa
        ld      a,_MNGCVPL
        jp      wcIN		

MNGC_PL:
        exa
        ld      a,_MNGC_PL
        jp      wcIN		
USPO:
        exa
        ld      a,_USPO
        jp      wcIN
DMATrans:
        exa
        ld      a,_DMAPL
        jp      wcIN

GWH:    OR A:SBC HL,DE:RET NC
        LD HL,0
        RET
;=====================================================================
;=====================================================================       		

SIN_DAT
	  DB  #00,#06,#0D,#13,#19,#1F,#25,#2C
	  DB  #32,#38,#3E,#44,#4A,#50,#56,#5C
	  DB  #62,#67,#6D,#73,#78,#7E,#83,#88
	  DB  #8E,#93,#98,#9D,#A2,#A7,#AB,#B0
	  DB  #B4,#B9,#BD,#C1,#C5,#C9,#CD,#D0
	  DB  #D4,#D7,#DB,#DE,#E1,#E4,#E7,#E9
	  DB  #EC,#EE,#F0,#F2,#F4,#F6,#F7,#F9
	  DB  #FA,#FB,#FC,#FD,#FE,#FE,#FF,#FF

	  	align 256,0x55
twpal	incbin "rorat3d wmf.tga.pal"

		align 256,0x66
sinsin	incbin "tw_sin.bin"
		db 0
		align 512,0x77
twmem_db	ds #300
