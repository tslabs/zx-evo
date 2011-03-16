.EQU    TVID_LCOUNT     =0
.EQU    TVID_PTR_L      =1
.EQU    TVID_PTR_H      =2
;
;--------------------------------------
;
TESTVIDEO:
        GETMEM  3
;
        LDIZ    T_VID_VBAND*2
        RCALL   T_VID_DRAWBAND
        CALL    WAITKEY
        CPI     DATA,KEY_ESC
        BRNE    T_VID01
        RJMP    T_VID99
T_VID01:
;
        LDIZ    T_VID_HBAND*2
        RCALL   T_VID_DRAWBAND
        CALL    WAITKEY
        CPI     DATA,KEY_ESC
        BREQ    T_VID99
;
        MOV     DATA,MODE1
        ANDI    DATA,0B00000001
        ORI     DATA,0B11111100
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG
        CALL    WAITKEY
        PUSH    DATA
        MOV     DATA,MODE1
        ORI     DATA,0B11111110
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG
        POP     DATA
        CPI     DATA,KEY_ESC
        BREQ    T_VID99
;
        LDI     XL,0
        LDI     XH,0
        CALL    SCR_SET_CURSOR
        LDIZ    442
T_VID80:LDI     DATA,$C5        ;"Å"
        LDI     TEMP,$09
        LDI     COUNT,1
        CALL    SCR_FILL_CHAR_ATTR
        LDI     DATA,$C5        ;"Å"
        LDI     TEMP,$0A
        LDI     COUNT,1
        CALL    SCR_FILL_CHAR_ATTR
        LDI     DATA,$C5        ;"Å"
        LDI     TEMP,$0C
        LDI     COUNT,1
        CALL    SCR_FILL_CHAR_ATTR
        SBIW    ZL,1
        BRNE    T_VID80
        CALL    WAITKEY
;        CPI     DATA,KEY_ESC
;        BREQ    T_VID99
;
T_VID99:FREEMEM  3
        RET
;
;--------------------------------------
;
T_VID_DRAWBAND:
        LDI     XL,0
        LDI     XH,0
        CALL    SCR_SET_CURSOR
T_VID15:LPM     COUNT,Z+
        STH     TVID_PTR_L,ZL
        STH     TVID_PTR_H,ZH
        TST     COUNT
        BREQ    T_VID19
T_VID14:STH     TVID_LCOUNT,COUNT
        LDH     ZL,TVID_PTR_L
        LDH     ZH,TVID_PTR_H
T_VID13:LPM     COUNT,Z+
        TST     COUNT
        BREQ    T_VID11
        LPM     TEMP,Z+
        LDI     DATA,$20        ;" "
        CPI     TEMP,$10
        BRCC    T_VID12
        LDI     DATA,$DB        ;"Û"
T_VID12:CALL    SCR_FILL_CHAR_ATTR
        RJMP    T_VID13
T_VID11:LDH     COUNT,TVID_LCOUNT
        DEC     COUNT
        BRNE    T_VID14
        RJMP    T_VID15
T_VID19:RET
;
;--------------------------------------
;
T_VID_VBAND:
        .DB     3,  7,$70,7,$60,7,$50,7,$40,7,$30,7,$20,7,$10,4,$00 ,0
        .DB     3,  7,$F0,7,$E0,7,$D0,7,$C0,7,$B0,7,$A0,7,$90,4,$00 ,0
        .DB     13, 7,$0F,7,$0E,7,$0D,7,$0C,7,$0B,7,$0A,7,$09,4,$00 ,0
        .DB     3,  7,$F0,7,$E0,7,$D0,7,$C0,7,$B0,7,$A0,7,$90,4,$00 ,0
        .DB     3,  7,$70,7,$60,7,$50,7,$40,7,$30,7,$20,7,$10,4,$00 ,0
        .DB     0,0
T_VID_HBAND:
        .DB     3,  53,$00                         ,0
        .DB     3,  7,$10,7,$90,25,$09,7,$90,7,$10 ,0
        .DB     3,  7,$20,7,$A0,25,$0A,7,$A0,7,$20 ,0
        .DB     3,  7,$30,7,$B0,25,$0B,7,$B0,7,$30 ,0
        .DB     3,  7,$40,7,$C0,25,$0C,7,$C0,7,$40 ,0
        .DB     3,  7,$50,7,$D0,25,$0D,7,$D0,7,$50 ,0
        .DB     3,  7,$60,7,$E0,25,$0E,7,$E0,7,$60 ,0
        .DB     3,  7,$70,7,$F0,25,$0F,7,$F0,7,$70 ,0
        .DB     1,  53,$00                         ,0
        .DB     0,0
;
;--------------------------------------
;
