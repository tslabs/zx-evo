;SCREEN SAVER PLUGIN
;--------------------------------------
WLD     EQU	#6006
SCRBUF  EQU	#0100
CONST   EQU	31
PALETTE EQU	#0000
;---------------------------------------
        ORG	#7E00
;---------------------------------------
;HEADER:
        DS	16
        DB	"WildCommanderMDL";    Header
        DB	#01;                  Version
        DB	0;                       Type
        DB	1; Pages
        DB	0; Page to #8000
;-------
        DB	0,2; CODE

       DUP	5
        DB	0,0
       EDUP	
;-------
        DS	16
;-------
        DS	32*3
;-------
        DB	0
;-------
        DW	#6000,#0000; MAX SIZE
;-------
        DB	"SCREEEN SAVER   "
        DB	"                "
;-------
        DB	2;0-BY EXTENSION ONLY
;            1-IF LOADED
;            2-BY TIMER
;---------------------------------------------------------------
        ORG	#8000

        LD	A,15
        CALL	WLD
        ;
        LD	A,#FF
        EXA	
        XOR	A
        CALL	WLD
        LD	HL,#C000
        LD	DE,0
        LD	BC,2048
        LDIR	
        LD	A,1
        EXA	
        LD	A,65
        CALL	WLD
        LD	HL,0
        LD	DE,#C000
        LD	BC,2048
        LDIR	
        LD	A,0
        EXA	
        LD	A,65
        CALL	WLD
        LD	A,1
        EXA	
        LD	A,64
        CALL	WLD
        LD	A,%11000011
        EXA	
        LD	A,66
        CALL	WLD
        ;
        LD	HL,#C000+128
        LD	DE,#C001+128
        LD	B,36
CLLA    PUSH	BC,DE,HL
        LD	BC,127
        LD	(HL),0
        LDIR	
        POP	HL,DE,BC
        INC	H
        INC	D
        DJNZ	CLLA

        LD	HL,#C000
        LD	DE,#C001
        LD	B,36
CLLS    PUSH	BC,DE,HL
        LD	BC,127
        LD	(HL),#B1
        LDIR	
        POP	HL,DE,BC
        INC	H
        INC	D
        DJNZ	CLLS
        ;
        LD	HL,SCRBUF
        LD	DE,SCRBUF+1
        LD	BC,2047
        LD	(HL),0
        LDIR	
        ;
        LD	HL,PAL
        LD	DE,#0000
        LD	BC,32
        LDIR	
        ;
MAIN_LOOP
        HALT	
        CALL	FLAME
        CALL	PR_BUF
        HALT	
        LD	A,45
        CALL	WLD
        JP	Z,MAIN_LOOP
        ;
EXIT
        LD	A,15
        CALL	WLD
        RET	
;---------------------------------------------------------------
PR_BUF
        LD	HL,SCRBUF
        LD	DE,#C000+5120+128
        LD	HX,'PALETTE

        LD	C,16
PBL2
        PUSH	DE
        LD	B,90
PBL1
        LD	A,(HL)
        AND	CONST
        LD	LX,A
        LD	A,(IX)
        LD	(DE),A
        INC	HL
        INC	DE
        DJNZ	PBL1
        POP	DE
        INC	D
        DEC	C
        JR	NZ,PBL2
        RET	
;---------------------------------------------------------------
FLAME
        LD	IX,SCRBUF+90
        LD	DE,SCRBUF
        LD	BC,90*16
BEGIN
        LD	A,(DE)
        CP	(IX+0)
        JR	NZ,LOOP1
        XOR	A
LOOP1   CP	(IX-1)
        JR	NZ,LOOP2
        XOR	A
LOOP2   ADD	A,(IX+0)
        ADD	A,(IX+1)
        ADD	A,(IX+90)
        SRA	A
        SRA	A
        LD	(DE),A
        INC	IX
        INC	DE
        DEC	BC
        LD	A,B
        OR	C
        JR	NZ,BEGIN
        ;
        LD	DE,SCRBUF+1440
        LD	B,180
BB_LOOP
        LD	HL,(SEED)
        LD	A,H
        ADD	HL,HL
        XOR	H
        ADD	HL,HL
        ADD	HL,HL
        ADD	HL,HL
        XOR	H
        ADD	HL,HL
        ADD	HL,HL
        XOR	H
        ADD	HL,HL
        ADD	HL,HL
        LD	L,A
        LD	(SEED),HL
        LD	A,H
        XOR	L

        AND	CONST
        LD	(DE),A
        INC	DE
        DJNZ	BB_LOOP
        RET	
;---------------------------------------------------------------
SEED    DW	#FFFF
        ;
PAL
        DB	0,0,0,1
        DB	2,3,2,2
        DB	2,2,10,11
        DB	6,14,14,15
        ;
        DB	14,14,14,15
        DB	2,3,14,15
        DB	3,7,10,11
        DB	6,14,14,15

;---------------------------------------------------------------
END
        DISPLAY	"LENGHT: ",END-#7E00



