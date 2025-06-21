INI93
        LD      A,#FF
        LD      (REGFF),A
        XOR     A
        LD      (CH_STAT),A
        LD      (REG1F),A
        LD      (REG3F),A
        LD      (REG5F),A
        LD      (REG7F),A
        LD      (SIDE),A
        LD      (DRIVE),A
        RET

REGCOM  DEFB    0
REG1F   DEFB    0
REG3F   DEFB    0
REG5F   DEFB    0
REG7F   DEFB    0
REGFF   DEFB    #FF
SIDE    DEFB    0
DRIVE   DEFB    0


REGSP   DEFW    0

	;DEFMAC  SAVESP
	;.IF     SVSP
	;LD      (REGSP),SP
	;LD      SP,VSTACK
	;.ENDIF
	;ENDMAC 
	MACRO	SAVESP
	IF	SVSP
		LD	(REGSP),SP
		LD	SP,VSTACK
	ENDIF
	ENDM

_OT1F
        SAVESP
        PUSH    AF
        PUSH    BC
        CALL    _C1F
        POP     BC
        POP     AF
        JP      EXI

_OT3F
        SAVESP
        LD      (REG3F),A
        JP      EXI
_OT5F
        SAVESP
        LD      (REG5F),A
        JP      EXI
_OT7F
        SAVESP
        LD      (REG7F),A
        JP      EXI
_OTFF
        SAVESP
        PUSH    AF
        PUSH    BC
        LD      B,A
        CALL    _OCFF
        POP     BC
        POP     AF
        JP      EXI

_OTCD
        SAVESP
        PUSH    AF
        PUSH    BC
        LD      A,D
        CALL    _OCX
        POP     BC
        POP     AF
        JP      EXI

_OTC
        SAVESP
        PUSH    AF
        PUSH    BC
        CALL    _OCX
        POP     BC
        POP     AF
        JP      EXI

_OCX
        LD      B,A
        LD      A,#FF
        CP      C
        JR      NZ,_CNFF

_OCFF
        LD      A,B
        AND     3
        LD      (DRIVE),A
        LD      A,B
        RRA
        RRA
        RRA
        RRA
        CPL
        AND     1
        LD      (SIDE),A
        RET
_CNFF
        LD      A,#7F
        CP      C
        JR      NZ,_CN7F
        LD      A,B
        LD      (REG7F),A
        RET
_CN7F
        LD      A,#5F
        CP      C
        JR      NZ,_CN5F
        LD      A,B
        LD      (REG5F),A
        RET
_CN5F
        LD      A,#3F
        CP      C
        JR      NZ,_CN3F
        LD      A,B
        LD      (REG3F),A
        RET
_CN3F
        LD      A,#1F
        CP      C
        CALL    NZ,8
        LD      A,B
_C1F
;--------------------------------------
;COMMANDS
        LD      (REGCOM),A
        LD      C,A

        LD      A,#FF
        LD      (REGFF),A

        LD      A,2
        LD      (CH_STAT),A     ;INDEX FOR NON RW COMMANDS

        RL      C
        JR      C,_NMOVEH

;STEP AND MOVE COMMANDS

        RL      C
        JR      C,_MOVEH

        RL      C
        JR      C,_STEPH

;       LD      A,#7F
;       LD      (REGFF),A

        RL      C
        JR      C,_SEARCH
;RECOVERY  MOVE HEAD TO 0 TACK
        XOR     A
        LD      (REG3F),A
        RET
_SEARCH
;SEARCH TRACK
        LD      A,(REG7F)
        LD      (REG3F),A
        RET
_STEPH
;STEP THE SAME WAY
        RL      C
        RET     NC      ;TRK REG NO CHANGE

;       CALL    UNDCONSTR
        CCF
        RR      C               ;!!!!!!
_MOVEH
;STEP FORWARD/BACKWARD
        RL      C
        LD      A,(REG3F)
        JR      C,_MOVEB
        INC     A
        RET     Z
        LD      (REG3F),A
        RET
_MOVEB
        OR      A
        RET     Z
        DEC     A
        LD      (REG3F),A
        RET
;--------------------------------------
_NMOVEH

        RL      C
        JR      NC,_LDSV

        RL      C
        JR      C,_RWTRK
        RL      C
        JR      NC,_RDADR
;INTERUPT

        RET
_RDADR
        JR      DATA_READY

_RWTRK
        JR      DATA_READY

_LDSV


DATA_READY
        LD      A,#7F
        LD      (REGFF),A
        XOR     A
        LD      (CH_STAT),A
        LD      A,2
        LD      (REG1F),A
        RET

;       CALL    UNDCONSTR

;-----------------------------
_IN1F
        SAVESP
        CALL    CHANGE1F
        LD      A,(REG1F)
;       OUT     (#FE),A
        JP      EXI
_IN3F
        SAVESP
        LD      A,(REG3F)
        JR      CHANGE1F_
_IN5F
        SAVESP
        LD      A,(REG5F)
        JR      CHANGE1F_
_IN7F
        SAVESP
        LD      A,(REG7F)
CHANGE1F_
        CALL    CHANGE1F
        JP      EXI
_INFF
        SAVESP
        PUSH    HL
        LD      HL,REGFF
        LD      A,(HL)
        LD      (HL),#FF
        POP     HL
        JP      EXI
_INFF1
        SAVESP
        INC     HL
        INC     HL
        INC     HL
        INC     HL
        PUSH    HL
        LD      HL,REGFF

        LD      A,(HL)
        LD      (HL),#FF
        POP     HL
        AND     #C0
        JP      EXI

_INFF2
        SAVESP
        INC     HL
        INC     HL
        INC     HL
        INC     HL
        PUSH    HL
        LD      HL,REGFF

        LD      A,(HL)
        LD      (HL),#FF
        POP     HL
        AND     #80
        JP      EXI


_INC
        SAVESP
        PUSH    AF
        CALL    _INXC
        EX      (SP),HL
        LD      H,A
        EX      (SP),HL
        POP     AF
        JP      EXI

_INHC
        SAVESP
        PUSH    AF
        CALL    _INXC
        LD      (REGHL+1),A
        POP     AF
        JP      EXI
_INXC
        LD      A,#FF
        CP      C
        JR      NZ,_HCNFF
        PUSH    HL
        LD      HL,REGHL
        LD      A,(HL)
        LD      (HL),#FF
        POP     HL
        RET
_HCNFF
        CALL    CHANGE1F
        LD      A,#1F
        CP      C
        LD      A,(REG1F)
        RET     Z
        LD      A,#3F
        CP      C
        LD      A,(REG3F)
        RET     Z
        LD      A,#5F
        CP      C
        LD      A,(REG5F)
        RET     Z
        LD      A,#7F
        CP      C
        LD      A,(REG7F)
        RET     Z
        RST     8

CHANGE1F
        PUSH    AF
        LD      A,(REG1F)
CH_STAT EQU     $+1
        XOR     0
        LD      (REG1F),A
        POP     AF
        RET

LD_SEC
        SAVESP

;       PUSH    DE
;       PUSH    HL
;       LD      DE,#3FD8        ;STORM PATCH
;       OR      A
;       SBC     HL,DE
;       POP     HL
;       POP     DE
;       JR      NZ,$+2+1+2+1
;       DEC     HL
;       LD      (HL),#D3
;       INC     HL

        LD      A,(REGCOM)
        AND     #F0
        CP      #80
        JR      NZ,_NLDS

        INC     B
        DEC     HL
        LD      A,5
        CALL    DRIVER
        JR      NC,LDS_ERR

        INC     H
_ELDS
        LD      (REGHL),HL
        XOR     A
        LD      (REG1F),A
        LD      (CH_STAT),A
        DEC     A
        LD      (REGFF),A
        LD      HL,#3FE9
        LD      A,#80
        AND     #C0
        JP      EXI
LDS_ERR
        LD      A,5
        OUT     (#FE),A
        CALL    UNDCONSTR
_NLDS
        CP      #90
        CALL    Z,UNDCONSTR    ;MULTISECTOR
        CP      #C0
        JR      NZ,_ELDS
;LOAD ADRESS

        INC     B
        DEC     HL
        LD      A,(REG3F)
        LD      (HL),A          ;TRACK
        INC     HL
        LD      (HL),0          ;SIDE
        INC     HL
_LDAS   EQU     $+1
        LD      A,0
        AND     #0F
        INC     A
        LD      (_LDAS),A
        LD      (HL),A          ;SECTOR
        INC     HL
        LD      (HL),1          ;SIZE
        INC     HL
        LD      (HL),#AA        ;CRC
        INC     HL
        LD      (HL),#55        ;CRC
        INC     HL

        JR      _ELDS

SV_SEC
        SAVESP
        INC     B
        DEC     HL

        LD      A,6
        CALL    DRIVER
        JR      NC,LDS_ERR

        INC     H
        LD      (REGHL),HL
        XOR     A
        LD      (CH_STAT),A
        LD      (REG1F),A
        DEC     A
        LD      (REGFF),A
        LD      HL,#3FCE
        LD      A,#80
        AND     #C0
        JR      EXI


DRIVER
        PUSH    IX
        PUSH    IY

        PUSH    HL
        PUSH    DE
        PUSH    BC
        EXX
        EX      AF,AF'
        PUSH    HL
        PUSH    DE
        PUSH    BC
        PUSH    AF
        EX      AF,AF'
        EXX


        PUSH    AF
        LD      BC,PAGE3
        IN      A,(C)
        LD      (DRVPG3),A
        LD      BC,PAGE2
        IN      A,(C)
        LD      (DRVPG2),A
        .IF     DEBUG
        LD      A,2
        .ELSE
        LD      A,MOD2PAGE
        .ENDIF
        OUT     (C),A
        POP     AF

        LD      C,A
        LD      A,(REG5F)
        DEC     A
        LD      E,A
        LD      A,(REG3F)
        ADD     A,A
        LD      D,A
        LD      A,(SIDE)
        ADD     A,D
        LD      D,A

        LD      A,(DRIVE)
        LD      B,A
        .IF     DEBUG
        RST     #20
        .ELSE
        CALL    MOD2ADR
        .ENDIF

DRVPG2  EQU     $+1
        LD      A,#3E
        LD      BC,PAGE2
        OUT     (C),A
DRVPG3  EQU     $+1
        LD      A,#3E
        LD      BC,PAGE3
        OUT     (C),A

        EX      AF,AF'
        EXX

        POP     AF
        POP     BC
        POP     DE
        POP     HL
        EX      AF,AF'
        EXX

        POP     BC
        POP     DE
        POP     HL

        POP     IY
        POP     IX

        SCF
        RET


UNDCONSTR
        JP      JSTS

