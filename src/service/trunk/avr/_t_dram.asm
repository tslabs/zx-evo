;3         4         5
;01234567890123456789012
;ÚÄÄÄÄ ’¥áâ DRAM ÄÄÄÄ¿  18
;³ à®¢¥¤¥­® æ¨ª«®¢  ³  19
;³ ¡¥§ ®è¨¡®ª   1234 ³  20
;³ á ®è¨¡ª ¬¨      0 ³  21
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  22
;
;
.EQU    MTST_PASS_LO    =0
.EQU    MTST_PASS_HI    =1
.EQU    MTST_FAIL_LO    =2
.EQU    MTST_FAIL_HI    =3
.EQU    MTST_CALLMODE   =4
;
;--------------------------------------
;
MTST_SHOW_REPORT:
        GETMEM  5
        STH     MTST_CALLMODE,DATA
        PUSH    FLAGS1
        ANDI    FLAGS1,0B11111100

        LDI     TEMP,MTST_PASS_CNT0
        CALL    FPGA_REG
        STH     MTST_PASS_LO,DATA
        LDI     TEMP,MTST_PASS_CNT1
        CALL    FPGA_REG
        STH     MTST_PASS_HI,DATA
        LDI     TEMP,MTST_FAIL_CNT0
        CALL    FPGA_REG
        STH     MTST_FAIL_LO,DATA
        LDI     TEMP,MTST_FAIL_CNT1
        CALL    FPGA_REG
        STH     MTST_FAIL_HI,DATA

        LDH     TEMP,MTST_FAIL_LO
        OR      DATA,TEMP
        BRNE    MTST_RPT1

        LDH     TEMP,MTST_CALLMODE
        TST     TEMP
        BREQ    MTST_RPT3
        LDI     TEMP,$77
        CALL    SCR_SET_ATTR
        RJMP    MTST_RPT4
MTST_RPT3:
        LDIZ    WIND_T_DRAM_1*2
        RJMP    MTST_RPT2
MTST_RPT1:
        LDIZ    WIND_T_DRAM_2*2
MTST_RPT2:
        CALL    WINDOW

        LDIZ    MLMSG_MTST*2
        CALL    SCR_PRINTMLSTR
MTST_RPT4:
        LDI     XL,43
        LDI     XH,20
        CALL    SCR_SET_CURSOR
        LDH     XL,MTST_PASS_LO
        LDH     XH,MTST_PASS_HI
        RCALL   MTST_DECWORD

        LDI     XL,43
        LDI     XH,21
        CALL    SCR_SET_CURSOR
        LDH     XL,MTST_FAIL_LO
        LDH     XH,MTST_FAIL_HI
        RCALL   MTST_DECWORD

        POP     FLAGS1
        FREEMEM 5
        RET
;
MTST_DECWORD:
        PUSHX
        ADIW    XL,1
        LDI     DATA,$20 ;" "
        BRNE    MTST_DECWRD1
        LDI     DATA,$3E ;">"
MTST_DECWRD1:
        CALL    SCR_PUTCHAR
        POPX
        JMP     DECWORD
;
;--------------------------------------
;
WIND_T_DRAM_1:
        .DB     30,18,21,5,$77,$00
WIND_T_DRAM_2:
        .DB     30,18,21,5,$AE,$00
;
;--------------------------------------
;
