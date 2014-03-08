        INCLUDE "ILBM_H.asm"

@startcode:
        ORG     #7E00
	
        INCLUDE "WCHDR.asm"
		
        LD      (mysp),sp        

        LD      A,%00100000
        call    SetVideoMode
        
        call    ClearScreen

		LD      A,1
        call    SetPage3
		
        LD      HL,0
        LD      (NEXT_LN),HL
        
        LD      HL,0
        LD      (GOX),HL
        call    Xoff
        LD      HL,0
        LD      (GOY),HL
        call    Yoff
        
        LD      A,SCREENPAGE
        LD      (NEXT_PG),A		
		xor     a
		ld	    (wcABT),a
		ld	    (wcENT),a
		ld      a,1
        call    SetVideoPage
        
        ld	    a,SCREENMODE
        call    SetVideoMode

		ld      hl,FILE_BUF
        PUSH    HL
        LD      B,FILEBUFSZ/512
        call    LoadDataSectors
        POP     HL

        LD      DE,sFORM   ; проверка на сигнатуру "FORM"
        CALL    fchunk
        JP      NZ,EXIT

        LD      DE,formlen ; сохраняем длину формы.
        LD      BC,4
        LDIR 

        LD      DE,sILBM   ; проверка на сигнатуру "ILBM"
        CALL    fchunk
        JP      NZ,EXIT

        LD      DE,sBMHD   ; проверка на сигнатуру "BMHD"
        CALL    fchunk
        JP      NZ,EXIT

        DUP     4          ; пропускаем длину чанка
        INC     HL
        EDUP 

        LD      DE,MyBMHD
        LD      BC,BMHD
        LDIR 

        LD      DE,sCMAP
        CALL    fchunk
        JP      NZ,EXIT

        DUP     4          ; пропускаем длину чанка
        INC     HL
        EDUP

        LD      IX,MyBMHD
        LD      A,(IX+BMHD.nPlanes)
        LD      B,A
        LD      A,1
        ADD     A,A
        DJNZ    $-1
        LD      (colors),A ; 0 = 256
        LD      B,A
        
        EXX 
        LD      HL,PAL_BUF
        EXX
        EX      DE,HL
        CALL    convert_pal0
        EX      DE,HL

;-------
		push	hl,bc,af		; записываем палитру
		ld		bc,tsFMAddr
		ld		a,0x10
		out		(c),a
		ld		hl,PAL_BUF
		ld		de,0
		ld		bc,0x200
		ldir
		xor		a
		ld		bc,tsFMAddr
		out		(c),a
		pop		af,bc,hl
;------
		
        LD      D,(IX+BMHD._H)
        LD      E,(IX+BMHD._H+1)
        
		PUSH	HL
		LD		HL,512
		OR		A
		SBC		HL,DE
		JR		NC,sl3
		LD		DE,512
sl3:    LD      (HTODO),DE
		EX		DE,HL
		LD      DE,288:CALL GWH:LD (GMY),HL
		POP		HL

        LD      DE,sBODY
        CALL    fchunk
        JP      NZ,EXIT
        LD      DE,4
        ADD     HL,DE

        PUSH    HL
        DEC     HL
        LD      E,(HL):DEC HL
        LD      D,(HL):DEC HL
        LD      C,(HL):DEC HL
        LD      B,(HL) ; BCDE =  длина данных BODY.
        POP     HL
        
        PUSH    HL
        PUSH    DE
        LD      DE,FILE_BUF+FILEBUFSZ
        EX      DE,HL
        OR      A
        SBC     HL,DE
        POP     DE ; мл. часть длины BODY
        
        EX      DE,HL
        OR      A
        SBC     HL,DE ; вычли ко-во необраб-х данных в буфере
        JR      NC,NAX
        LD      A,B
        OR      C
        JR      NZ,MAX
        ADD     HL,DE
        JR      $+3
MAX     DEC     BC
NAX     EX      DE,HL
        LD      HL,511
        ADD     HL,DE
        JR      NC,$+3
        INC     BC
        EX      DE,HL
        LD      DE,FILE_BUF
        EX      DE,HL

        SRL     B
        RR      C
        RR      D

        LD      E,D
        LD      D,C
;        LD      A,B				; ILBMов больше 32Мб 
;        LD      (BODY_REMAIN+2),A	; не бывает.
        LD      (BODY_REMAIN),DE ; в блоках по 512 байт.


        LD      L,(IX+BMHD._W+1)
        LD      H,(IX+BMHD._W)
        
        PUSH    HL
        CALL    Pix2Bytes
        LD      A,L
        LD      (rows),A
        POP     HL
        
        PUSH    HL
		LD		DE,512
		EX		DE,HL
		OR		A
		SBC		HL,DE
		JR		NC,sl4
		LD		DE,512
sl4:	EX		DE,HL
        LD      DE,360:CALL GWH:LD (GMX),HL
		POP		HL
        
        SRL     H
		RR      L	
		LD		A,H			; если ширина больше 512 пикс, то выводить будем только 512
		OR		A
		JR		Z,sl1
		LD		A,0xff
		JR		sl2
sl1:	LD      A,L
        DEC     A
sl2:	LD      (dma_len),A
        POP     HL

        LD      (nxtrow),HL

        LD      A,SCREENPAGE
        LD      (NEXT_PG),A

@XXL:   LD      HL,(HTODO)
        LD      A,H
        OR      L
        JP      Z,EXIT
        LD      DE,32
        OR      A
        SBC     HL,DE
        JR      NC,NAD
        ADD     HL,DE
        LD      A,L
        LD      L,0
        JR      $+3
NAD     LD      A,E

        LD      (HTODO),HL
			
        LD      B,A ; кроме  последнего прохода по 32 линии

        CALL    DOLINE

        LD      A,(NEXT_PG)
        INC     A
        LD      (NEXT_PG),A
        LD      DE,0
        LD      (NEXT_LN),DE
        JR      XXL
        
ClearScreen:
        XOR     A
        CALL    SetPage3Video

        LD      A,#FF
        CALL    DMATrans        
        LD      B,159
        LD      C,255
        LD      A,7
        CALL    DMATrans
        
        LD      HL,0,(0xC000),HL
        LD      DE,2
        LD      B,0,C,0
        LD      A,0
        CALL    DMATrans
        
        LD      A,#FE
        CALL    DMATrans
        LD      A,#FE
        CALL    DMATrans
        RET

;*****************************************************************
;*****************************************************************
@DOLINE:

		PUSH    BC
        LD      B,(IX+BMHD.nPlanes)
        LD      A,(rows)
        LD      C,A

        LD      HL,(nxtrow)
        LD      DE,BP_BUF
        CALL    UnRLERow

        LD      (nxtrow),HL

        LD      DE,BP_BUF
        LD      HL,READY_BUF

        LD      A,(rows)
        LD      B,A
        LD      A,(IX+BMHD.nPlanes)
        LD      C,A

		CALL	SUCK
		
	
		ld	a,255
        call    DMATrans
		
        LD      A,(NEXT_PG)
        LD      C,A
        LD      DE,(NEXT_LN)
        
        PUSH    DE
        LD      HL,READY_BUF
        LD      A,H:AND %00111111:LD H,A
        LD      B,#80

        XOR     A
        call    DMATrans

        LD      A,(dma_len)
        LD      B,A

        LD      A,6
        call    DMATrans

        XOR     A
        LD      B,A
        LD      A,5
        call    DMATrans

        LD      A,#FD
        call    DMATrans
        POP     DE
        
        INC     D
        INC     D
        LD      (NEXT_LN),DE
        POP     BC
        DJNZ    DOLINE
		RET 

EXIT:   EI:HALT
        CALL UPPP,NZ,GUP
        CALL DWWW,NZ,GDN
        CALL RGGG,NZ,GRG
        CALL LFFF,NZ,GLF

        call    ESC:JR NZ,EXIT_ESC
        call    ENKE:JR NZ,EXIT_ENT
        JR      EXIT
        
EXIT_ESC:
        XOR     A
        RET 
EXIT_ENT:
        LD      A,2
        RET

;--------------------------------------
GUP     LD HL,(GOY),DE,GSTP
        OR A:SBC HL,DE:RET C
GGY     LD (GOY),HL
        CALL Yoff
        RET
;-------
GDN     LD HL,(GOY),DE,GSTP
        ADD HL,DE
        LD DE,(GMY)
        OR A:SBC HL,DE:RET NC
        ADD HL,DE
        JR GGY
;-------
GRG     LD HL,(GOX),DE,GSTP
        ADD HL,DE
        LD DE,(GMX)
        OR A:SBC HL,DE:RET NC
        ADD HL,DE
        JR GGX
GLF     LD HL,(GOX),DE,GSTP
        OR A:SBC HL,DE:RET C
GGX     LD (GOX),HL
        CALL Xoff
        RET        

;---------------------------------------------------------------------	
FBUF_UPD:
        PUSH    AF,BC,DE
        LD      HL,(BODY_REMAIN)
        LD		A,H
        OR      L
        JR      Z,E_UPD ; надо закончить декодирование и перейти на опрос клавиатуры.
        LD      DE,FILEBUFSZ/512
        OR      A
        SBC     HL,DE
		LD		B,E		; = длина буфера / 512
        JR      NC,FCC		
        ADD     HL,DE			
        LD      B,L				
        LD      HL,0				
FCC     LD      (BODY_REMAIN),HL
        LD      HL,FILE_BUF
        PUSH    HL
        call    LoadDataSectors
        POP     HL
        POP     DE,BC,AF
        RET 
E_UPD:				; выход был аварийный, - снимаем лишние данные для возврата в основной цикл.
        LD      HL,FILE_BUF
        DI
        LD      SP,(mysp)
        EI
		jp		EXIT
;=====================================================================
;=====================================================================
SetPage3:
        EXA
		xor     a
		jp      wcIN
        
SetPage3Video:
        EXA
        ld      a,65
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
        ld      a,22
        jp      wcIN

SetVideoPage:
        exa
        ld      a,MNGV_PL
        jp      wcIN

SetVideoMode:
        exa
        ld      a,GVmod
        jp      wcIN

LoadDataSectors:
		ld		a,48
        jp      wcIN

DMATrans:
        exa
        ld      a,DMA_PL
        jp      wcIN

Xoff    ld      a,68
        jp      wcIN
Yoff    ld      a,67
        jp      wcIN

GWH:    OR A:SBC HL,DE:RET NC
        LD HL,0
        RET
        
;----------------------------
	MODULE UnRLERow
;----------------------------
@UnRLERow:
        LD C,A
UnRLEalg
        PUSH BC
rle_decode
        LD B,(HL)
        BIT 7,B:JR NZ,replicate
        INC B
        LD A,C
        SUB B
        
        INC HL
        BIT 6,H:CALL NZ,FBUF_UPD
        LD C,0-1
copy    LDI
        BIT 6,H:CALL NZ,FBUF_UPD
        DJNZ copy

        LD C,A
        OR A
        JR Z,endrow
        JP next_bpl
        
replicate
        LD A,B
        NEG 
        INC A
        LD B,A
        LD A,C
        SUB B
        LD C,A
        INC HL
        EXA 
        BIT 6,H:CALL NZ,FBUF_UPD
        LD A,(HL)
        
do_replicate
        LD (DE),A:INC DE
        DJNZ do_replicate
        
        INC HL
		BIT	6,H:CALL NZ,FBUF_UPD
        LD A,C
        OR A:JR Z,endrow
next_bpl
        LD A,C
        OR A
        jp NZ,rle_decode
endrow
        POP BC
        djnz UnRLEalg
        RET 
	ENDMODULE
;----------------------------

;--------------------
@Pix2Bytes:
        PUSH    DE
        LD      DE,15
        ADD     HL,DE
        DUP     4  ; = /16 = num of words
;       DUP     3  ; = /8  = num of bytes
        SRL     H
        RR      L
        EDUP 
        SLA     L
        RL      H
        POP     DE
        RET 
;--------------------


;---------------------------------------------------------------

@convert_pal0:

        LD      HL,0
        LD      C,L
;r
        LD      A,(DE)
        INC     DE

        CALL    div10
        SLA     A
        SLA     A
        AND     %01111100
        LD      H,A

;g
        LD      A,(DE)
        INC     DE

        CALL    div10
        DUP     5
        SLA     A
        RL      C
        EDUP 
        AND     %11100000
        LD      L,A
        LD      A,C
        AND     %11
        OR      H
        LD      H,A

;b
        LD      A,(DE)
        INC     DE

        CALL    div10
        AND     %00011111
        OR      L
        LD      L,A

        PUSH    HL
        EXX 
        POP     DE
        LD      (HL),E:INC HL
        LD      (HL),D:INC HL
        EXX 

        DJNZ    convert_pal0
        RET	

;++++++++++++++++++++++++++++

	MODULE	div10
@div10:
		PUSH    HL,DE,BC
        LD      DE,10
        LD      H,D
        LD      L,A
        CALL    DIV16
        LD      A,L
        CP      5
        JR      C,.R1
        INC     E
.R1     LD      A,E
        POP     BC,DE,HL
        RET 
	ENDMODULE

	MODULE	DIV16
; HL/DE -> DE, HL - остаток.
; CF = 1 - ERROR
@DIV16:
        LD      A,D
        OR      E
        JR      Z,.ZERO  ; деление на 0 не допустимо.
        EXX 
        PUSH    HL
        PUSH    DE
        PUSH    BC
        LD      HL,1
        LD      D,H
        LD      E,H
        LD      B,H
        LD      C,L
        EXX 
.L2:     PUSH    DE
.L1:     EX      DE,HL
        ADD     HL,HL   ; умножение делителя на 2.
        EX      DE,HL
        JR      C,.OVER0
        SBC     HL,DE
        JR      C,.OVERFL
        EXX 
        ADD     HL,HL  ; подсчет величины множителя, кратного 2.
        EXX 
        ADD     HL,DE   ; восстановление делимого.
        JR      .L1
.OVER0:  RR      D
        JR      .OVER1
.OVERFL: ADD     HL,DE   ; восст-е делимого.
        SRL     D       ; деление делителя на 2 - восстановление
.OVER1:  RR      E       ; до состояния до переполнения.
        OR      A
        SBC     HL,DE
        JR      C,.E_DIV16 ; делитель и без умножения на 2 был
                          ; больше делимого.
        EXX 
        ADD     HL,DE   ; подсчет частного.
        EX      DE,HL   ; копим частное в DE.
        LD      H,B     ; обнуляем множитель для делителя.
        LD      L,C     ; инициализация счетчика для множителя.
        EXX 
        POP     DE      ; восстановление делителя.
        JR      .L2      ; делим оставшуюся часть делимого.
.E_DIV16: ADD     HL,DE   ; HL - остаток от деления.
        EXX 
        PUSH    DE
        EXX 
        POP     DE      ; DE - частное.
        POP     AF      ; снимаем лишние данные со стека
        EXX 
        POP     BC
        POP     DE
        POP     HL
        EXX 
        OR      A
        RET 
.ZERO:   SCF 
        RET
	ENDMODULE

;-------------------------
@cpchunk:
LP      LD      A,(DE)
        OR      A
        RET     Z
        CP      (HL)
        RET     NZ
        INC     HL
        INC     DE
        JR      LP

		MODULE fchunk
@fchunk:
        PUSH    HL,DE

        LD      DE,FILE_BUF+FILEBUFSZ
        EX      DE,HL
        OR      A
        SBC     HL,DE
        LD      B,H   ;оставшаяся длина буфера.
        LD      C,L
        POP     DE,HL
.fchunkL: LD      A,(DE) ; первый символ искомой строки
        CPIR 
        JP      PO,.efchunk
        PUSH    DE
        INC     DE
        CALL    Z,cpchunk
        JR      Z,.efchunkOK
        POP     DE
        JR      .fchunkL
.efchunk:
        LD      A,1
        OR      A
        RET 
.efchunkOK:
        POP     DE
        RET 
	ENDMODULE
	
	MODULE SUCK
@SUCK:
;i:  DE - Source
;    HL - Destination
;     B - rows
;     C - bitplanes

        ld a,c:or a:ret z
        push bc,de,hl

        ld a,b
        push de
        exx
        pop hl
        ld e,a
        ld d,0
        exx
        ex de,hl
        ld a,c:cp 8:jr nz,ualg

suk		CALL EightBitplanes
		djnz suk
        
sexit   pop hl,de,bc
        RET
;---------------------------------------
ualg    push bc
        CALL sloup
        pop bc
        djnz ualg
        jr sexit

sloup   EXX
        PUSH HL
        EXX

        LD H,high TABL
;-------
        LD B,E
        
        EXX:LD A,(HL):ADD HL,DE
        EXX
        LD L,A
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A
        DEC C:JR Z,SLEND
        
BITAZ   LD E,B
        EXX:LD A,(HL):ADD HL,DE
        EXX
        LD L,A
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A
        DEC C:JR NZ,BITAZ      
;-------
SLEND   INC DE
;-------
        EXX
        POP HL:INC HL
        EXX
        RET
;---------------------------------------
EightBitplanes:
        EXX
        PUSH HL
        EXX

        LD H,high TABL
;-------
        LD C,E
        
        EXX:LD A,(HL):ADD HL,DE
        EXX
        LD L,A
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A:INC E
        LD A,(HL):INC H
        LD (DE),A,E,C
;-------
        DUP 6
        EXX:LD A,(HL):ADD HL,DE
        EXX
        LD L,A
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A,E,C
        EDUP
;-------
        EXX:LD A,(HL)
        EXX
        LD L,A
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL):INC H
        LD (DE),A:INC E
        LD A,(DE):OR (HL)
        LD (DE),A:INC DE
;-------------------------------
        EXX
        POP HL:INC HL
        EXX
        RET
		ENDMODULE
;---------------------------


@READY_BUF_Pg:      ; начальная страница буфера изображения.
        DB      0

@L_LEN:   DB      0 ; ширина линии левой части буфера изображения.
@R_LEN:   DB      0 ; ширина линии правой части буфера изображения

@PixLen:  DW      0 ; ширина изображения в точках.
@ByteLen: DB      0 ; ширина изображения в байтах.

@BODY_REMAIN:
        DW      0,0 ; длина необработанного остатка BODY.

@sILBM:   DB      "ILBM",0
@sBMHD:   DB      "BMHD",0
@sCAMG:   DB      "CAMG",0
@sCMAP:   DB      "CMAP",0
@sCTBL:   DB      "CTBL",0
@sSHAM:   DB      "SHAM",0
@sBODY:   DB      "BODY",0
@sFORM:   DB      "FORM",0

@formlen: DW      0,0

	STRUCT BMHD,0
_W	WORD	0
_H	WORD	0
_X	WORD	0
_Y	WORD	0
nPlanes BYTE	0
masking BYTE	0
compress BYTE	0
pad BYTE	0
transparency WORD	0
xAspectRatio BYTE	0
yAspectRatio BYTE	0
PageWidth WORD	0
PageHeight WORD	0
	ENDS	

//@MyILBM:  DS      ilbmSZ,0
@MyBMHD:  DS      BMHD,0
//@dmaset:  DS      dmasetupSZ,0

@GOX      DW      0
@GOY      DW      0
@GMX      DW      0
@GMY      DW      0
@colors:  DB      0
@TRDPOS:  DW      0
@BP_NEXT: DW      0
@BP_BACK: DW      0
@READYNEXT: DW    0
@HTODO:   DW      0
@PgTODO:  DB      0
@rows:    DB      0
@nxtrow:  DW      0
@lpp:     DB      0 ; линий в странице памяти.
@NEXT_PG: DB      0
@NEXT_LN: DW      0
@dma_len: DB      0
@IFFlenL:	dw		0
@IFFlenH:	dw		0
@mysp:	dw	0
		align 2
clrword	dw	0x0000
		align 512,0
@codend
		INCBIN "decode.bin"
@endcode:
		
		
	SAVEBIN "ilbmview.wmf", 0x7e00, endcode-0x7e00
