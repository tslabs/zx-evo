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

        IN      TEMP,EICRB
        ORI     TEMP,(1<<ISC61)|(0<<ISC60)
        OUT     EICRB,TEMP
        IN      TEMP,EIMSK
        ORI     TEMP,(1<<INT6)
        OUT     EIMSK,TEMP
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
        LDI     DATA,$7F
        LDI     TEMP,COVOX
        CALL    FPGA_REG
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
        IN      TEMP,EIMSK
        CBR     TEMP,(1<<INT6)
        OUT     EIMSK,TEMP

        FREEMEM 1
        RET
;
;--------------------------------------
;
T_BEEP_INT:
        PUSH    DATA
        PUSHZ
        LDS     ZH,T_BEEP_PTR+0
        LDS     ZL,T_BEEP_PTR+1
        LDS     DATA,T_BEEP_DELTA+0
        ADD     ZH,DATA
        LDS     DATA,T_BEEP_DELTA+1
        ADC     ZL,DATA
        STS     T_BEEP_PTR+0,ZH
        STS     T_BEEP_PTR+1,ZL
        LDI     ZH,HIGH(TABL_SINUS*2)
        LPM     DATA,Z
        CALL    FPGA_SAME_REG
        POPZ
        POP     DATA
        RET
;
;--------------------------------------
;
WIND_T_BEEP:
        .DB     8,8,32,6,$DF,$01
;
T_BEEP_FREQTAB:
        .DW        76,    181
        .DW       107,    256
        .DW       151,    362
        .DW       214,    512
        .DW       302,    724
        .DW       427,   1024
        .DW       604,   1448
        .DW       854,   2048
        .DW      1208,   2896
        .DW      1709,   4096
        .DW      2417,   5793
        .DW      3418,   8192
        .DW      4834,  11585
        .DW      6836,  16384
        .DW      9667,  23170
;
;--------------------------------------
;
