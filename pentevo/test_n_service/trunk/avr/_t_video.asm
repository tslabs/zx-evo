.EQU    TVID_TST_CNT    =0
;
;--------------------------------------
;
TESTVIDEO:
        GETMEM  1

        LDI     XL,0
        LDI     XH,0
        CALL    SCR_SET_CURSOR
        LDI     DATA,$00
        LDI     TEMP,$00
        LDIW    53*25
        CALL    SCR_FILLLONG_CHAR_ATTR
        LDI     XL,15
        LDI     XH,4
        CALL    SCR_SET_CURSOR
        LDIZ    (MSG_TITLE2*2)+3
        CALL    SCR_PRINTSTRZ

T_VID22:MOV     DATA,MODE1
        ANDI    DATA,0B10000000
T_VID21:INC     DATA
        STH     TVID_TST_CNT,DATA
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG
        CALL    WAITKEY
        CPI     DATA,KEY_ESC
        BREQ    T_VID99
        LDH     DATA,TVID_TST_CNT
        MOV     TEMP,DATA
        ANDI    TEMP,$0F
        CPI     TEMP,6
        BRCS    T_VID21
        RJMP    T_VID22

T_VID99:MOV     DATA,MODE1
        ANDI    DATA,0B10000000
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG

        FREEMEM 1
        RET
;
;--------------------------------------
;
