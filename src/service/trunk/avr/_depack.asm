.EQU    DMLZ_DATA       =0
.EQU    DMLZ_TEMP       =1
.EQU    DMLZ_TMP2       =2
.EQU    DMLZ_TMP3       =3
.EQU    DMLZ_XL         =4
.EQU    DMLZ_XH         =5
.EQU    DMLZ_JUMP       =6
;
;--------------------------------------
;
.DSEG
DMLZ_Z:         .BYTE   2
DMLZ_REGS:      .BYTE   7
.CSEG
;
;--------------------------------------
;in:    Z,RAMPZ == указатель на упакованные данные
DMLZ_INIT:
        STSZ    DMLZ_Z
        LDIZ    DMLZ_REGS
        LDI     TEMP,$80
        STD     Z+DMLZ_TEMP,TEMP
        LDI     TEMP,LOW(MEGABUFFER)
        STD     Z+DMLZ_TMP2,TEMP
        LDI     TEMP,HIGH(MEGABUFFER)
        STD     Z+DMLZ_TMP3,TEMP
        STD     Z+DMLZ_JUMP,FF
        RET
;(не менять RAMPZ между вызовами DMLZ_GETBYTE)
;--------------------------------------
;out:   sreg.Z == CLR - успешно, SET - конец данных
;       DATA == очередной байт
DMLZ_GETBYTE:
        LDIZ    DMLZ_REGS
        LDD     DATA,Z+DMLZ_DATA
        LDD     TEMP,Z+DMLZ_TEMP
        LDD     TMP2,Z+DMLZ_TMP2
        LDD     TMP3,Z+DMLZ_TMP3
        LDD     XL,Z+DMLZ_XL
        LDD     XH,Z+DMLZ_XH
        LDD     R0,Z+DMLZ_JUMP
        LDSZ    DMLZ_Z
        TST     R0
        BRMI    DMLZ_MS
        BREQ    DMLZ_METKA0
        RJMP    DMLZ_METKA1
;
DMLZ_MS:ELPM    R0,Z+
        STSZ    DMLZ_Z
        LDIZ    DMLZ_REGS
        STD     Z+DMLZ_JUMP,NULL
DMLZ_OUT:
        STD     Z+DMLZ_DATA,DATA
        STD     Z+DMLZ_TEMP,TEMP
        STD     Z+DMLZ_XL,XL
        STD     Z+DMLZ_XH,XH
        MOV     XL,TMP2
        MOV     XH,TMP3
        ST      X+,R0
        SUBI    XH,HIGH(MEGABUFFER) ;
        ANDI    XH,DBMASK_HI        ;address warp
        ADDI    XH,HIGH(MEGABUFFER) ;
        STD     Z+DMLZ_TMP2,XL
        STD     Z+DMLZ_TMP3,XH
        MOV     DATA,R0
        CLZ
        RET

DMLZ_METKA0:
DMLZ_M0:LDI     WH,$02
        LDI     WL,$FF
DMLZ_M1:ADD     TEMP,TEMP
        BRNE    DMLZ_M2
        ELPM    TEMP,Z+
        ROL     TEMP
DMLZ_M2:ROL     WL
        BRCC    DMLZ_M1
        DEC     WH
        BRNE    DMLZ_X2
        LDI     DATA,2
        ASR     WL
        BRCS    DMLZ_N1
        INC     DATA
        INC     WL
        BREQ    DMLZ_N2
        LDI     WH,$03
        LDI     WL,$3F
        RJMP    DMLZ_M1

DMLZ_X2:DEC     WH
        BRNE    DMLZ_X3
        LSR     WL
        BRCS    DMLZ_MS
        INC     WH
        RJMP    DMLZ_M1

DMLZ_X6:ADD     DATA,WL
DMLZ_N2:LDI     WH,$04
        LDI     WL,$FF
        RJMP    DMLZ_M1

DMLZ_N1:INC     WL
        BRNE    DMLZ_M4
        INC     WH
DMLZ_N5:ROR     WL
        BRCS    DMLZ_END
        ROL     WH
        ADD     TEMP,TEMP
        BRNE    DMLZ_N6
        ELPM    TEMP,Z+
        ROL     TEMP
DMLZ_N6:BRCC    DMLZ_N5
        ADD     DATA,WH
        LDI     WH,6
        RJMP    DMLZ_M1
DMLZ_X3:DEC     WH
        BRNE    DMLZ_X4
        LDI     DATA,1
        RJMP    DMLZ_M3
DMLZ_X4:DEC     WH
        BRNE    DMLZ_X5
        INC     WL
        BRNE    DMLZ_M4
        LDI     WH,$05
        LDI     WL,$1F
        RJMP    DMLZ_M1
DMLZ_X5:DEC     WH
        BRNE    DMLZ_X6
        MOV     WH,WL
DMLZ_M4:ELPM    WL,Z+
DMLZ_M3:DEC     WH
        MOV     XL,WL
        MOV     XH,WH
        ADD     XL,TMP2
        ADC     XH,TMP3
DMLZ_LDIR:
        SUBI    XH,HIGH(MEGABUFFER) ;
        ANDI    XH,DBMASK_HI        ;address warp
        ADDI    XH,HIGH(MEGABUFFER) ;
        LD      R0,X+
        STSZ    DMLZ_Z
        LDIZ    DMLZ_REGS
        STD     Z+DMLZ_JUMP,ONE
        RJMP    DMLZ_OUT

DMLZ_METKA1:
        DEC     DATA
        BRNE    DMLZ_LDIR

        RJMP    DMLZ_M0
;
DMLZ_END:
        SEZ
        RET
;
;--------------------------------------
;
