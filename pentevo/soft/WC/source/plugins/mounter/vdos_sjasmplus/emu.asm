
		DEVICE	ZXSPECTRUM48

;YO		EQU	0				; tasm assumes that TRUE == 0
;NO		EQU	1				; and that FALSE == 1 !!!!!
YO		EQU	1
NO		EQU	0

STSPAGE		EQU	#57

DEBUG		EQU	NO
DEBUGG		EQU	NO
REAL		EQU	NO
SVSP		EQU	YO

;----------------------------------------------------------

MOD2PAGE	EQU	#FE
MOD2ADR		EQU	#8000

VDOS		EQU	#0000;8000			; building directly, to avoid using buggy DISP/PHASE

VSTACK		EQU	#0880
VCODE		EQU	#0890

VORG = #3408						; EQU is not allowed, because VORG is supposed to change

REGHL		EQU	#0880

PAGE0		EQU	#10AF
PAGE2		EQU	#12AF
PAGE3		EQU	#13AF
MEMCFG		EQU	#21AF
SYSCFG		EQU	#20AF

	;DEFMAC  _ORGD
	;ORG     \0+VDOS
	;.PHASE  \0
	;ENDMAC 
	MACRO _ORGD addr
		ORG	VDOS+addr
		;DISP	addr
	ENDM

		ORG	VDOS

		;INCBIN	"rst8.bin"
		DS	16384, #CF			; "rst8.bin" is simply 16K of #CF
;		.INCBIN 504T				; this was commented out by JTN

		_ORGD	#00
		JP	START
		_ORGD	#08
		JP	RST8
		_ORGD	#10
		JP	RST10
		_ORGD	#20
		JP	RST20
		_ORGD	#30
		JP	RST30
		_ORGD	#38
		RET
		_ORGD	#66
		RETN

;----------------------------------------------------------

		_ORGD	VCODE

START		CALL	INI93
		CALL	RSINIT
		RET

;----------------------------------------------------------

RST30
        IF     NO

        _ORGD   #30
CALLRAM
        LD      (CRAMHL),HL
        LD      HL,#3D2F
        EX      (SP),HL
        PUSH    DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        INC     HL
        EX      (SP),HL
        EX      DE,HL
        LD      (CRAMJP),HL

CRAMHL  EQU     $+1
        LD      HL,#2121
CRAMJP  EQU     $+1
        JP      #C3C3

        ENDIF

;----------------------------------------------------------

RST10
        IF     NO

        EX      (SP),HL
        PUSH    AF
        PUSH    DE
        LD      E,(HL)
        INC     HL
        LD      D,(HL)
        LD      BC,0-8
        ADD     HL,BC
        LD      (EX_JP),HL
        EX      DE,HL
        JP      (HL)

        ENDIF

;----------------------------------------------------------

RST20
		INCLUDE	"rsemu.asm"

;----------------------------------------------------------

	IF     REAL
		;.INCLUDE EMUR		TO BE ADDED
        ELSE
		INCLUDE	"emu93.asm"
        ENDIF

EXI		LD	(EX_JP),HL

	IF	DEBUG

        EX      (SP),HL
        LD      (STCK),HL
        EX      (SP),HL
        LD      (RGAC),A
        PUSH    AF
        LD      A,(REGFF)
        LD      (RGAC+1),A
        POP     AF

        PUSH    AF
        PUSH    BC
        LD      BC,PAGE3
        IN      A,(C)
        LD      (PGG),A
        LD      A,#08
        OUT     (C),A

        LD      BC,(#FFFE)
        SET     6,B
        SET     7,B
        LD      A,L
        LD      (BC),A
        INC     BC
        LD      A,H
        LD      (BC),A
        INC     BC

STCK    EQU     $+1
        LD      HL,0

        LD      A,L
        LD      (BC),A
        INC     BC
        LD      A,H
        LD      (BC),A
        INC     BC

        LD      HL,(REGHL)

        LD      A,L
        LD      (BC),A
        INC     BC
        LD      A,H
        LD      (BC),A
        INC     BC

RGAC    EQU     $+1
        LD      HL,0

        LD      A,L
        LD      (BC),A
        INC     BC
        LD      A,H
        LD      (BC),A
        INC     BC

        LD      A,B
        OR      A
        JR      NZ,$+2+2
        LD      B,#FF

        LD      (#FFFE),BC
        LD      BC,PAGE3
PGG     EQU     $+1
        LD      A,0
        OUT     (C),A

        POP     BC
        POP     AF

        ENDIF

		LD	HL,(REGHL)

	IF	REAL
		CALL	VGOFF
	ENDIF

	IF	SVSP
		LD	SP,(REGSP)
	ENDIF

EX_JP		EQU	$+1
		JP      #C3C3

;----------------------------------------------------------

RST8
JSTS
	IF	DEBUG

		IF	SVSP
		LD	SP,(REGSP)
		ENDIF

        PUSH    AF
        PUSH    BC
        LD      BC,MEMCFG
        LD      A,#00
        OUT     (C),A
        LD      BC,#7FFD
        LD      A,STSPAGE
        OUT     (C),A
        POP     BC
        POP     AF
        PUSH    HL
        LD      HL,#DB00
        EX      (SP),HL
        JP      #2A53

	ELSE

		;.LOCAL
		POP	HL
		LD	BC,0
1
		LD	D,8
		LD	A,7
		OUT	(#FE),A
		CALL	2f
3
		LD	A,6
		OUT	(#FE),A
		CALL	2f

		RL	L
		RL	H
		JR	NC,$+3
		INC	HL
		RLA
		RL	L
		RL	H
		JR	NC,$+3
		INC	HL
		RLA
		AND	3
		OUT	(#FE),A
		CALL	2f
		DEC	D
		JR	NZ,3b
		JR	1b
2
		EX	(SP),HL
		EX	(SP),HL
		DEC	BC
		LD	A,B
		OR	C
		JR	NZ,2b
		RET

	ENDIF

;----------------------------------------------------------

		_ORGD	#1D73
		LD	A,(HL)		;FOR STS
		RET

		_ORGD	#3D2F
		NOP
		RET

;----------------------------------------------------------
;LOAD SEC

		_ORGD	#3FEC
		INI
		JP	LD_SEC

		_ORGD	#3FE9
		OUT	(#1F),A		;EXIT LD SEC

;SAVE SEC
		_ORGD	#3FD1
		OUTI
		JP	SV_SEC

		_ORGD	#3FCE
		OUT	(#1F),A		;EXIT SV SEC

;----------------------------------------------------------

		INCLUDE	"emutrap.asm"

;----------------------------------------------------------

        IF	DEBUGG

        ORG     #7F00
        DI

        LD      BC,SYSCFG
        LD      A,#01
        LD      A,0
        OUT     (C),A
        LD      BC,MEMCFG
        LD      A,#0E
        OUT     (C),A

        LD      BC,#10AF
        LD      A,#FF
        OUT     (C),A

        LD      HL,#8000
        LD      DE,#0000
        LD      BC,#4000
        LDIR

        RST     0

        LD      BC,PAGE0
        LD      A,#00
        OUT     (C),A

        LD      BC,MEMCFG
        LD      A,#40
        OUT     (C),A

        LD      BC,#7FFD
        LD      A,#10
        OUT     (C),A

        LD      BC,#29AF
        LD      A,#0F
        OUT     (C),A

        LD      BC,#EFF7
        LD      A,#80
        OUT     (C),A
        LD      BC,#DFF7
        LD      A,#EF
        OUT     (C),A
        LD      BC,#BFF7
        LD      A,#05
        OUT     (C),A

        LD      HL,0
        PUSH    HL
        JP      #3D2F
SSTS
        LD      BC,#7FFD
        LD      A,STSPAGE
        OUT     (C),A
        JP      #DB00

        DEFS    #7FD0-$

        CALL    MMMMM
        LD      A,6
        OUT     (#FE),A
        JR      $

        LD      C,1
        LD      A,0
        CALL    #3D13
        RET

        LD      BC,#0105
        LD      DE,#0000
        LD      HL,#7000
        CALL    #3D13
        RET

MMMMM
        LD      C,#7F
        LD      HL,#2A53
        PUSH    HL
        JP      #3D2F

        ORG     #C000
SENDEMU
        .INCBIN SENDEMU
        .RUN    $
        CALL    SENDEMU
        JP      RSSHARE

        ORG     #6000
RSSHARE
        .INCBIN RSSHARE

	ENDIF

	IF	NO

        ORG     #7F00
        LD      BC,#7FFD
        LD      A,#17
        OUT     (C),A
        JP      #DB00

        ORG     #8000
        CALL    RSINIT
        LD      C,5
        LD      HL,#C000
        LD      DE,0
        CALL    RSEMU
        RET

        .INCLUDE RSEMU		TO BE ADDED

	ENDIF

		SAVEBIN	"emu.bin", VDOS, 16384





