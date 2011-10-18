.EQU    TBEEP_N =0
;
;--------------------------------------
;
.DSEG
T_BEEP_PTR:     .BYTE   2
T_BEEP_DELTA:   .BYTE   2
.CSEG
;
;┌──────────────────────────────┐
;│                              │
;│           12345 Гц           │
;│                              │
;│ <>, <> - изменение частоты │
;└──────────────────────────────┘
;
;--------------------------------------
;
TESTBEEP:
        GETMEM  1

        LDIZ    WIND_T_BEEP*2
        CALL    WINDOW
        LDIZ    MLMSG_TBEEP*2
        CALL    SCR_PRINTMLSTR

        LDI     DATA,0B00000001
        LDI     TEMP,INT_CONTROL
        CALL    FPGA_REG

        LDI     DATA,7
T_BEEP_NEWFREQ:
        STH     TBEEP_N,DATA
        LDI     XL,20
        LDI     XH,10
        CALL    SCR_SET_CURSOR
        LDH     DATA,TBEEP_N
        LSL     DATA
        LSL     DATA
        LDIZ    T_BEEP_FREQTAB*2
        ADD     ZL,DATA
        ADC     ZH,NULL
        LPM     XL,Z+
        LPM     XH,Z+
        PUSHZ
        CALL    DECWORD
        POPZ
        LPM     DATA,Z+
        STS     T_BEEP_DELTA+0,DATA
        LPM     DATA,Z+
        STS     T_BEEP_DELTA+1,DATA
        LDI     TEMP,COVOX
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_SET_FPGA_REG
.ENDIF
        SPICS_SET
        OUT     SPDR,TEMP
        CALL    FPGA_RDY_RD
        ;теперь ничего не выводить в FPGA (не менять текущий регистр) !
T_BEEP_WAITKEY:
        LDI     DATA,0B00000001
        MOV     INT6VECT,DATA
        CALL    WAITKEY
        CLR     INT6VECT

        CPI     DATA,KEY_ESC
        BREQ    T_BEEP_ESCAPE
        CPI     DATA,KEY_UP
        BREQ    T_BEEP_UP
        CPI     DATA,KEY_DOWN
        BREQ    T_BEEP_DOWN
        RJMP    T_BEEP_WAITKEY

T_BEEP_UP:
        LDH     DATA,TBEEP_N
        CPI     DATA,14
        BRCC    T_BEEP_WAITKEY
        INC     DATA
        RJMP    T_BEEP_NEWFREQ

T_BEEP_DOWN:
        LDH     DATA,TBEEP_N
        TST     DATA
        BREQ    T_BEEP_WAITKEY
        DEC     DATA
        RJMP    T_BEEP_NEWFREQ

T_BEEP_ESCAPE:
        LDI     DATA,0B00000000
        LDI     TEMP,INT_CONTROL
        CALL    FPGA_REG

        FREEMEM 1
        RET
;
;--------------------------------------
;
T_BEEP_INT:
        PUSH    DATA
        PUSH    XH
        PUSHW
        PUSHZ
        LDI     ZH,HIGH(TABL_SINUS*2)
        LDS     XH,T_BEEP_PTR+0
        LDS     ZL,T_BEEP_PTR+1
        LDSW    T_BEEP_DELTA

T_BEEP_IN1:
        ADD     XH,WL
        ADC     ZL,WH
        LPM     DATA,Z
        SPICS_CLR
        OUT     SPDR,DATA
T_BEEP_IN2:
        SBIS    SPSR,SPIF
        RJMP    T_BEEP_IN2
        IN      DATA,SPDR
        SPICS_SET
        CPI     DATA,12
        BRCS    T_BEEP_IN1

        STS     T_BEEP_PTR+0,XH
        STS     T_BEEP_PTR+1,ZL
        POPZ
        POPW
        POP     XH
        LDI     DATA,(1<<INT6)
        OUTPORT EIFR,DATA
        POP     DATA
        RET
;
;--------------------------------------
;
WIND_T_BEEP:
        .DB     8,8,32,6,$DF,$01
;
T_BEEP_FREQTAB:
        .DW        75,     45
        .DW       107,     64
        .DW       152,     91
        .DW       214,    128
        .DW       302,    181
        .DW       427,    256
        .DW       604,    362
        .DW       854,    512
        .DW      1208,    724
        .DW      1709,   1024
        .DW      2417,   1448
        .DW      3418,   2048
        .DW      4833,   2896
        .DW      6836,   4096
        .DW      9668,   5793
;
;--------------------------------------
;
