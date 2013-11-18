; 3                                             9
;ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;³      ÚÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿       ³
;³      ³ pc/win32 ³  TESTCOM            ³       ³
;³      ÃÄÄÄÄÄÄÄÄÄÄÙ                     ³       ³
;³      ³ Bit rate 115200   No parity    ³       ³
;³      ³ Data bits 8       Flow control ³       ³
;³      ³ Stop bits 2        û RTS/CTS   ³       ³
;³      ³                   DSR - Ignored³       ³
;³      ³           Start BERT           ³       ³
;³      ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ       ³
;³                      ³COM port                ³
;³                      ³                        ³
;³                RS-232³                        ³
;³ÚÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿³
;³³ ZX-Evo ³   Last sec        65535 sec        ³³16
;³ÃÄÄÄÄÄÄÄÄÙ     10472            10472         ³³17
;³³ RxBuff °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° RTS ³³18
;³³ TxBuff °°°°°°°°°°°°°°°°°°°°°°°°°°°°°°°° CTS ³³19
;³ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ³20
;ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
; 3                                             9
;
;--------------------------------------
;
.EQU    TRS_TIMEOUT     =0
.EQU    TRS_SECONDS     =2
.EQU    TRS_COUNT_LAST  =4
.EQU    TRS_COUNT_TOTAL =6
;                        10
;--------------------------------------
;
MSG_TRS_1:
        .DB     $16,20, 3,$C2
        .DB     $16,11, 4,"pc/win32 "
        .DB     $16,23, 4,"TESTCOM"
        .DB     $16, 9, 5,$C3
        .DB     $16,11, 6,"Bit rate 115200   No parity"
        .DB     $16,11, 7,"Data bits 8"
        .DB     $16,29, 7,"Flow control "
        .DB     $16,11, 8,"Stop bits 2"
        .DB     $16,30, 8,$FB," RTS/CTS"
        .DB     $16,29, 9,"DSR - Ignored"
        .DB     $16,21,10,"Start BERT "
        .DB     $16,25,11,$C2
        .DB     $16,25,12,$B3,"COM port"
        .DB     $16,25,13,$B3
        .DB     $16,19,14,"RS-232",$B3
        .DB     $16,12,15,$C2
        .DB     $16,25,15,$C1
        .DB     $16, 5,16,"ZX Evo",$16,16,16,"Last sec"
        .DB     $16,38,16,"sec"
        .DB     $16, 3,17,$C3
        .DB     $16, 5,18,"RxBuff",$16,45,18,"RTS",$16, 5,19,"TxBuff",$16,45,19,"CTS",0,0
;
;--------------------------------------
;
WIND_T_RS_1:
        .DB     2,2,49,20,$DF,$01
WIND_T_RS_2:
        .DB     9,3,34,9,$DF,$00
WIND_T_RS_3:
        .DB     9,3,12,3,$DF,$00
WIND_T_RS_4:
        .DB     3,15,47,6,$DF,$00
WIND_T_RS_5:
        .DB     3,15,10,3,$DF,$00
;
;--------------------------------------
;
TESTRS232:
        ANDI    FLAGS1,0B11111100
        ORI     FLAGS1,0B00000100
        GETMEM  10

        LDIZ    WIND_T_RS_1*2
        CALL    WINDOW
        LDIZ    WIND_T_RS_2*2
        CALL    WINDOW
        LDIZ    WIND_T_RS_3*2
        CALL    WINDOW
        LDIZ    WIND_T_RS_4*2
        CALL    WINDOW
        LDIZ    WIND_T_RS_5*2
        CALL    WINDOW
        LDIZ    MSG_TRS_1*2
        CALL    SCR_PRINTSTRZ

T_RS_0: STH     TRS_SECONDS+0,FF
        STH     TRS_SECONDS+1,FF
        ORI     FLAGS1,0B00010000
        RCALL   T_RS_CLRBUFFS
        MOVW    ZL,YL
        RCALL   T_RS_STATUS

T_RS_1: CALL    INKEY
        BREQ    T_RS_2
        CPI     DATA,KEY_SPACE
        BREQ    T_RS_0
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    T_RS_2
        CPI     DATA,KEY_ESC
        BRNE    T_RS_2
        RCALL   T_RS_CLRBUFFS
        CBR     FLAGS1,0B00010000

        FREEMEM 10
        RET

T_RS_2: MOVW    ZL,YL
        CALL    CHECK_TIMEOUT_MS
        BRCC    T_RS_3
        RCALL   T_RS_STATUS
T_RS_3: RCALL   T_RS_BUFFBAR
T_RS_5: CALL    UART_CHK_CTS
        LDS     TEMP,UART_TX_LN
        CPI     TEMP,UART_TXBSIZE
        BRCC    T_RS_1
        CALL    UART_GETCHAR
        BREQ    T_RS_1
        CALL    UART_PUTCHAR
        LDH     XL,TRS_COUNT_TOTAL+0
        LDH     XH,TRS_COUNT_TOTAL+1
        LDH     ZL,TRS_COUNT_TOTAL+2
        LDH     ZH,TRS_COUNT_TOTAL+3
        ADIW    XL,1
        ADC     ZL,NULL
        ADC     ZH,NULL
        STH     TRS_COUNT_TOTAL+0,XL
        STH     TRS_COUNT_TOTAL+1,XH
        STH     TRS_COUNT_TOTAL+2,ZL
        STH     TRS_COUNT_TOTAL+3,ZH
        LDH     XL,TRS_COUNT_LAST+0
        LDH     XH,TRS_COUNT_LAST+1
        ADIW    XL,1
        STH     TRS_COUNT_LAST+0,XL
        STH     TRS_COUNT_LAST+1,XH
        RJMP    T_RS_5
;
;--------------------------------------
;
T_RS_BUFFBAR:
        LDI     XL,12
        LDI     XH,18
        CALL    SCR_SET_CURSOR
        LDS     COUNT,UART_RX_LN
.IF (UART_RXBSIZE>128)
        LSR     COUNT
.ENDIF
.IF (UART_RXBSIZE>64)
        LSR     COUNT
.ENDIF
.IF (UART_RXBSIZE>32)
        INC     COUNT
        LSR     COUNT
.ENDIF
        RCALL   T_RSBAR0
        LDI     TEMP,$AE
        SBIS    PORTD,5
        LDI     TEMP,$C0
        LDI     COUNT,3
        CALL    SCR_FILL_ATTR
        LDI     TEMP,$DF
        CALL    SCR_SET_ATTR

        LDI     XL,12
        LDI     XH,19
        CALL    SCR_SET_CURSOR
        LDS     COUNT,UART_TX_LN
.IF (UART_TXBSIZE>128)
        LSR     COUNT
.ENDIF
.IF (UART_TXBSIZE>64)
        LSR     COUNT
.ENDIF
.IF (UART_TXBSIZE>32)
        INC     COUNT
        LSR     COUNT
.ENDIF
        RCALL   T_RSBAR0
        LDI     TEMP,$AE
        SBIS    PINB,6
        LDI     TEMP,$C0
        LDI     COUNT,3
        CALL    SCR_FILL_ATTR
        LDI     TEMP,$DF
        JMP     SCR_SET_ATTR
;
;--------------------------------------
;
T_RSBAR0:
        LDI     DATA,32
        SUB     DATA,COUNT
        TST     COUNT
        BREQ    T_RSBAR1
        PUSH    DATA
        LDI     DATA,$DB ;"Û"
        CALL    SCR_FILL_CHAR
        POP     DATA
T_RSBAR1:
        TST     DATA
        BREQ    T_RSBAR2
        MOV     COUNT,DATA
        LDI     DATA,$B0 ;"°"
        CALL    SCR_FILL_CHAR
T_RSBAR2:
        LDI     DATA,$20 ;" "
        JMP     SCR_PUTCHAR
;
;--------------------------------------
;
T_RS_STATUS:
        LDIW    1000
        CALL    SET_TIMEOUT_MS

        LDI     XL,32
        LDI     XH,16
        CALL    SCR_SET_CURSOR
        LDH     XL,TRS_SECONDS+0
        LDH     XH,TRS_SECONDS+1
        ADIW    XL,1
        BRNE    T_RSSTAT1
        STH     TRS_COUNT_LAST+0,NULL
        STH     TRS_COUNT_LAST+1,NULL
        STH     TRS_COUNT_TOTAL+0,NULL
        STH     TRS_COUNT_TOTAL+1,NULL
        STH     TRS_COUNT_TOTAL+2,NULL
        STH     TRS_COUNT_TOTAL+3,NULL
T_RSSTAT1:
        STH     TRS_SECONDS+0,XL
        STH     TRS_SECONDS+1,XH
        CALL    DECWORD

        LDI     XL,18
        LDI     XH,17
        CALL    SCR_SET_CURSOR
        LDH     XL,TRS_COUNT_LAST+0
        LDH     XH,TRS_COUNT_LAST+1
        CALL    DECWORD
        STH     TRS_COUNT_LAST+0,NULL
        STH     TRS_COUNT_LAST+1,NULL

        LDI     XL,35
        LDI     XH,17
        CALL    SCR_SET_CURSOR
        LDH     WL,TRS_SECONDS+0
        LDH     WH,TRS_SECONDS+1
        TST     WL
        BRNE    T_RSSTAT2
        TST     WH
        BRNE    T_RSSTAT2
        MOVW    XL,WL
        RJMP    T_RSSTAT3
T_RSSTAT2:
        LDH     XL,TRS_COUNT_TOTAL+0
        LDH     XH,TRS_COUNT_TOTAL+1
        LDH     ZL,TRS_COUNT_TOTAL+2
        LDH     ZH,TRS_COUNT_TOTAL+3
        CALL    DIV3216U
T_RSSTAT3:
        JMP     DECWORD
;
;--------------------------------------
;
T_RS_CLRBUFFS:
.IFNDEF DEBUG_FPGA_OUT
        CLI
        LDS     TEMP,UART_TX_HD
        STS     UART_TX_TL,TEMP
        STS     UART_TX_LN,NULL
        LDS     TEMP,UART_RX_HD
        STS     UART_RX_TL,TEMP
        STS     UART_RX_LN,NULL
        RTS_CLR
        SEI
.ENDIF
        RET
;
;--------------------------------------
;
