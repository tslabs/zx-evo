
;    IN A,(XX)						; this was commented out by JTN
; TT JP YY
; YY LD (ZZ),HL
;    LD HL,TT
;    JP _INXX

;----------------------------------------------------------

TEST		EQU	NO

	IF	TEST

VDOS		EQU	#8000
VORG		EQU	#3B00
REGHL		EQU	#087E

	ENDIF

;----------------------------------------------------------

        ;DEFMAC  IC
        ;ORG     \0+VDOS
        ;.PHASE  \0
;       ;IN      A,(C)					; this was commented out by JTN
        ;OUT     (#1F),A
        ;JP      VORG
        ;ORG     VORG+VDOS
        ;.PHASE  VORG
        ;LD      (REGHL),HL
        ;LD      HL,\0
        ;JP      _INC
        ;.ENDIF
;VORG    =       $
        ;ENDMAC 

        ;DEFMAC  IP
        ;ORG     \0+VDOS
        ;.PHASE  \0
;       ;IN      A,(#\1)				; this was commented out by JTN
        ;OUT     (#1F),A
        ;.IF     "\2"-""
        ;JP      VORG
        ;.ELSE
        ;JR      $+\2+2
        ;ORG     $+VDOS+\2
        ;.PHASE  $-VDOS
        ;JP      VORG
        ;.ENDIF
        ;ORG     VORG+VDOS
        ;.PHASE  VORG
        ;LD      (REGHL),HL
        ;LD      HL,\0
        ;JP      _IN\1
        ;.ENDIF
;VORG    =       $
        ;ENDMAC 

	MACRO IP addr, port
		ORG	addr+VDOS
		;DISP	addr
		;IN	A,(port)			; this was commented out by JTN
		OUT	(#1F),A
		JP	VORG

		ORG	VORG+VDOS
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr

		;JP	_IN\1
		IF	port==#1F
		JP	_IN1F
		ENDIF
		IF	port==#3F
		JP	_IN3F
		ENDIF
		IF	port==#5F
		JP	_IN5F
		ENDIF
		IF	port==#7F
		JP	_IN7F
		ENDIF
		IF	port==#FF
		JP	_INFF
		ENDIF
VORG = $
	ENDM

	MACRO IP3 addr, port, smth
		ORG	addr+VDOS
		;DISP	addr
		;IN	A,(port)			; this was commented out by JTN
		OUT	(#1F),A
		JR	$+(smth)+2
		ORG     $+VDOS+smth
		;DISP	$-VDOS
		JP	VORG

		ORG	VORG+VDOS
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr

		;JP	_IN\1
		IF	port==#1F
		JP	_IN1F
		ENDIF
		IF	port==#3F
		JP	_IN3F
		ENDIF
		IF	port==#5F
		JP	_IN5F
		ENDIF
		IF	port==#7F
		JP	_IN7F
		ENDIF
		IF	port==#FF
		JP	_INFF
		ENDIF
		IF	port==#E1
		JP	_INFF1
		ENDIF
		IF	port==#E2
		JP	_INFF2
		ENDIF
		IF	port==#EC
		JP	_INHC
		ENDIF
VORG = $
	ENDM

        ;DEFMAC  OP
        ;ORG     \0+VDOS
        ;.PHASE  \0
;       ;OUT     (#\1),A				; this was commented out by JTN
        ;OUT     (#1F),A
        ;.IF     "\2"-""
        ;JP      VORG
        ;.ELSE
        ;JR      $+\2+2
        ;ORG     $+VDOS+\2
        ;.PHASE  $-VDOS
        ;JP      VORG
        ;.ENDIF
        ;ORG     VORG+VDOS
        ;.PHASE  VORG
        ;LD      (REGHL),HL
        ;LD      HL,\0
        ;JP      _OT\1
        ;.ENDIF
;VORG    =       $
        ;ENDMAC 

	MACRO	TrapOUT addr, port

		ORG	VDOS+addr
		;DISP	addr
        	;OUT	(port),A			; this was commented out by JTN
		OUT	(#1F),A
		JP	VORG

		ORG	VDOS+VORG
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr

		IF port==#1F : JP _OT1F : ENDIF
		IF port==#3F : JP _OT3F : ENDIF
		IF port==#5F : JP _OT5F : ENDIF
		IF port==#7F : JP _OT7F : ENDIF
		IF port==#FF : JP _OTFF : ENDIF
		IF port==#C : JP _OTCD : ENDIF
		IF port != #1F && port != #3F && port != #5F && port != #7F && port != #FF && port != #C
		DISPLAY "error: TrapOUT does not work with port ", port
		ENDIF
VORG = $
	ENDM

	MACRO	TrapOUTJR addr, port

		ORG	VDOS+addr-3
		;DISP	addr
1		JP	VORG
        	;OUT	(port),A			; this was commented out by JTN
		OUT	(#1F),A
		JR	1b

		ORG	VDOS+VORG
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr

		IF port==#1F : JP _OT1F : ENDIF
		IF port==#3F : JP _OT3F : ENDIF
		IF port==#5F : JP _OT5F : ENDIF
		IF port==#7F : JP _OT7F : ENDIF
		IF port==#FF : JP _OTFF : ENDIF
		IF port != #1F && port != #3F && port != #5F && port != #7F && port != #FF
		DISPLAY "error: TrapOUTJR does not work with port ", port
		ENDIF
VORG = $
	ENDM


        MACRO OP3 addr, port, smth
        	ORG	addr+VDOS
        	;DISP	addr
        	;OUT	(port),A			; this was commented out by JTN
        	OUT	(#1F),A

		JR	$+(smth)+2
		ORG	$+VDOS+(smth)
		;DISP	$-VDOS
		JP	VORG

		ORG	VORG+VDOS
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr

		;JP      _OT\1				; this one is actually a bit annoying to replicate
		IF	port==#1F
		JP	_OT1F
		ENDIF
		IF	port==#3F
		JP	_OT3F
		ENDIF
		IF	port==#5F
		JP	_OT5F
		ENDIF
		IF	port==#7F
		JP	_OT7F
		ENDIF
		IF	port==#FF
		JP	_OTFF
		ENDIF
VORG = $
	ENDM

        ;DEFMAC  OC
        ;ORG     \0+VDOS
        ;.PHASE  \0
;       ;OUT     (C),A					; this was commented out by JTN
        ;OUT     (#1F),A
        ;.IF     "\2"-""
        ;JP      VORG
        ;.ELSE
        ;JR      $+\2+2
        ;ORG     $+VDOS+\2
        ;.PHASE  $-VDOS
        ;JP      VORG
        ;.ENDIF
        ;ORG     VORG+VDOS
        ;.PHASE  VORG
        ;LD      (REGHL),HL
        ;LD      HL,\0
        ;JP      _OTC
        ;.ENDIF
;VORG    =       $
        ;ENDMAC 

	MACRO OC addr
		ORG	addr+VDOS
		;DISP	addr
		;OUT	(C),A				; this was commented out by JTN
		OUT	(#1F),A
		JP	VORG

		ORG	VORG+VDOS
		;DISP	VORG
		LD	(REGHL),HL
		LD	HL,addr
		JP	_OTC
VORG = $
	ENDM

;----------------------------------------------------------

		OC	#2A53

		TrapOUT	#20B8,#C

		TrapOUT	#3EC9,#1F
		TrapOUT	#3EDF,#1F
		TrapOUT	#3D9A,#1F
		TrapOUT	#2FC3,#1F
		TrapOUT	#3F25,#1F
		TrapOUT	#3F4D,#1F
		TrapOUT	#2F28,#1F
		TrapOUT	#2000,#1F
		TrapOUT	#2093,#1F
		TrapOUT	#2D80,#1F

		TrapOUT	#1E3A,#3F
		TrapOUT	#3E7E,#3F
		TrapOUT	#3E95,#3F
		TrapOUT	#2085,#3F

		TrapOUT	#3F1B,#5F
		TrapOUT	#2F1D,#5F
		TrapOUT	#208D,#5F
		TrapOUT	#2D75,#5F

		TrapOUT	#2748,#7F
		TrapOUT	#3E44,#7F
		TrapOUTJR	#3E4C,#7F

		TrapOUT	#3DD5,#FF
		TrapOUT	#1FF3,#FF
		TrapOUT	#3D4D,#FF
		TrapOUT	#2FB1,#FF
		TrapOUT	#02BE,#FF
		TrapOUT	#2F0C,#FF
		TrapOUT	#2F3C,#FF

		IP	#2740,#1F
		IP	#3E30,#1F
		IP	#3E3A,#1F
		IP	#3EB5,#1F
		IP	#3DB5,#1F
		IP	#3DBA,#1F
		IP	#3F33,#1F
		IP	#1FDD,#1F
		IP	#2F2F,#1F
		IP	#2076,#1F
		IP	#2099,#1F
		IP	#2D87,#1F

		IP	#3E78,#3F
		IP	#3E87,#3F
		IP	#3EBC,#3F
		IP	#3F55,#3F
		IP	#3F69,#3F
		IP	#3E50,#3F

		IP	#3F5A,#5F
		IP	#3F72,#5F

		IP	#3EFE,#7F

		IP	#3DA6,#FF
		IP	#3ECE,#FF
		IP	#3FBC,#FF
		IP3	#3FCA,#FF,0-7
		IP	#3FD7,#FF
		IP3	#3FE5,#FF,0-7
		IP	#20B1,#FF

	;IP	#3EF5,FF1,2
		IP3	#3EF5,#E1,2

		ORG	#3EF5+VDOS+4
		OUT	(#1F),A
                          ;IN HC
	;IP      #3EF3,HC,0-7
		IP3	#3EF3,#EC,0-7

	;IP      #2F59,FF2,2
		IP3	#2F59,#E2,2

		ORG	#2F59+VDOS+4
		OUT	(#1F),A
                          ;OUT 1F
		OP3	#2F57,#1F,7

		OP3	#2F4D,#FF,0-45
		TrapOUT	#2F50,#7F

;----------------------------------------------------------

	IF	TEST

		ORG	#0800

_OT1F		NOP
_OT3F		NOP
_OT5F		NOP
_OT7F		NOP
_OTFF		NOP
_OTC

_IN1F		NOP
_IN3F		NOP
_IN5F		NOP
_IN7F		NOP
_INFF		NOP
_INFF1		NOP
_INC		NOP
_INHC

	ENDIF


