;  4                              5678901234567
; ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
; ³Detecting mouse...                           ³03
; ³FF FA AA 00                                  ³
; ³Customization...                             ³
; ³F3 FA C8 FA F3 FA 64 FA F3 FA 50 FA          ³
; ³F2 FA 03                                     ³
; ³E8 FA 02 FA E6 FA F3 FA 64 FA F4 FA          ³
; ³Let's go!                                    ³
; ³08 00 00 00                    ÚÄÄÄÄÄÄÄÄÄÄÄ¿ ³10
; ³                               ³ÛÛÛ        ³ ³11
; ³                               ³ÛLÛ  M   R ³ ³12
; ³                               ³ÛÛÛ        ³ ³13
; ³                               ³           ³ ³14
; ³                               ³ Wheel = 1 ³ ³15
; ³                               ³           ³ ³16
; ³                               ³ X  =  123 ³ ³17
; ³                               ³ Y  =   58 ³ ³18
; ³                               ÀÄÄÄÄÄÄÄÄÄÄÄÙ ³
; ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;  4                              5678901234567
;
;--------------------------------------
;
.EQU    TPSM_BYTE1      =0
.EQU    TPSM_BYTE2      =1
.EQU    TPSM_BYTE3      =2
.EQU    TPSM_BYTE4      =3
.EQU    TPSM_X0         =4
.EQU    TPSM_X1         =5
.EQU    TPSM_Y          =6
.EQU    TPSM_Z          =7
.EQU    TPSM_BTN        =8
.EQU    TPSM_ID         =9
;
;--------------------------------------
;
.EQU    PS2M_BIT_PARITY =0
.EQU    PS2M_BIT_ACKBIT =1      ; ¯à¨­ïâ ACK-¡¨â
.EQU    PS2M_BIT_TX     =7      ; ¯¥à¥¤ ç 
.EQU    PS2M_BIT_READY  =7

.MACRO  PS2M_DATALINE_UP
        CBI     DDRD,7
        SBI     PORTD,7
.ENDMACRO

.MACRO  PS2M_DATALINE_DOWN
        CBI     PORTD,7
        SBI     DDRD,7
.ENDMACRO

.MACRO  PS2M_CLOCKLINE_UP
        CBI     DDRE,5
        SBI     PORTE,5
.ENDMACRO

.MACRO  PS2M_CLOCKLINE_DOWN
        CBI     PORTE,5
        SBI     DDRE,5
.ENDMACRO
;
;--------------------------------------
;
.DSEG
PS2M_BIT_COUNT: .BYTE   1
PS2M_DATA:      .BYTE   1
PS2M_RAW_READY: .BYTE   1
PS2M_RAW_CODE:  .BYTE   1
PS2M_FLAGS:     .BYTE   1
PS2M_TIMEOUT:   .BYTE   2
.CSEG
;
;--------------------------------------
;
MSG_TPSM_1:
        .DB     $16,37,15,"Wheel ="
MSG_TPSM_2:
        .DB     $16,37,12,"L   M   R"
        .DB     $16,37,17,"X  =",$16,37,18,"Y  =",0,0
MSG_TPSM_3:
        .DB     $16, 4,10,$15,$DF,0
;
;--------------------------------------
;
WIND_T_PS2M_1:
        .DB     3,2,47,19,$DF,$01
WIND_T_PS2M_2:
        .DB     9,10,34,4,$AF,$01
WIND_T_PS2M_3:
        .DB     35,10,13,10,$DF,$00
;
;--------------------------------------
;
EXT_INT5:
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP
        PUSH    COUNT
        PUSH    DATA
        LDS     DATA,PS2M_DATA
        LDS     COUNT,PS2M_BIT_COUNT
        LDS     TEMP,PS2M_FLAGS
        SBRC    TEMP,PS2M_BIT_TX
        RJMP    INT5TX_0

        CPI     COUNT,9
        BREQ    INT5RX9
        CPI     COUNT,10
        BRCC    INT5RXS
        TST     COUNT
        BRNE    INT5RX1
;start bit
        SBIC    PIND,7   ; data line
        RJMP    INT5RX_CLR_D
        CBR     TEMP,(1<<PS2M_BIT_PARITY)
        INC     COUNT
        RJMP    INT5_EXIT
;data bits
INT5RX1:LSR     DATA
        SBIS    PIND,7   ; data line
        RJMP    INT5RX2
        ORI     DATA,0B10000000
        EOR     TEMP,ONE
INT5RX2:INC     COUNT
        RJMP    INT5_EXIT
;parity bit
INT5RX9:SBIC    PIND,7   ; data line
        EOR     TEMP,ONE
        SBRS    TEMP,PS2M_BIT_PARITY
        RJMP    INT5RX_CLR_D
        INC     COUNT
        RJMP    INT5_EXIT
;stop bit
INT5RXS:SBIS    PIND,7   ; data line
        RJMP    INT5RX_CLR_D

        STS     PS2M_RAW_READY,COUNT
        STS     PS2M_RAW_CODE,DATA

INT5RX_CLR_F:
        CLR     TEMP
INT5RX_CLR_D:
        CLR     DATA
        CLR     COUNT
        RJMP    INT5_EXIT
;-------
INT5TX_0:
        CPI     COUNT,9
        BREQ    INT5TX9
        CPI     COUNT,10
        BREQ    INT5TXS
        CPI     COUNT,11
        BRCC    INT5TXA
        TST     COUNT
        BRNE    INT5TX1
;start bit
        SBR     TEMP,(1<<PS2M_BIT_PARITY)
        INC     COUNT
        RJMP    INT5_EXIT
;data bits
INT5TX1:ROR     DATA
        BRCC    INT5TX2
        PS2M_DATALINE_UP
        EOR     TEMP,ONE
        INC     COUNT
        RJMP    INT5_EXIT
INT5TX2:PS2M_DATALINE_DOWN
        INC     COUNT
        RJMP    INT5_EXIT
;parity bit
INT5TX9:SBRC    TEMP,PS2M_BIT_PARITY
        RJMP    INT5TXP
        PS2M_DATALINE_DOWN
        INC     COUNT
        RJMP    INT5_EXIT
INT5TXP:PS2M_DATALINE_UP
        INC     COUNT
        RJMP    INT5_EXIT
;stop bit
INT5TXS:PS2M_DATALINE_UP
        INC     COUNT
        RJMP    INT5_EXIT
;ack-bit
INT5TXA:CLR     TEMP
        SBIS    PIND,7   ; data line
        LDI     TEMP,(1<<PS2M_BIT_ACKBIT)
        CLR     DATA
        CLR     COUNT

INT5_EXIT:
        STS     PS2M_BIT_COUNT,COUNT
        STS     PS2M_DATA,DATA
        STS     PS2M_FLAGS,TEMP
        POP     DATA
        POP     COUNT
        POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        RETI
;
;--------------------------------------
;
TESTMOUSE:
        ANDI    FLAGS1,0B11111011
        ORI     FLAGS1,0B00000010
        LDIZ    MLMSG_MOUSE_TEST*2
        CALL    PRINTMLSTR
        ORI     FLAGS1,0B00000100

        GETMEM  10

T_PSM_RESTART:
        CLI
        PS2M_DATALINE_UP
        PS2M_CLOCKLINE_UP
        IN      TEMP,EICRB
        ORI     TEMP,(1<<ISC51)|(0<<ISC50)
        OUT     EICRB,TEMP
        IN      TEMP,EIMSK
        ORI     TEMP,(1<<INT5)
        OUT     EIMSK,TEMP
        STS     PS2M_FLAGS,NULL
        STS     PS2M_BIT_COUNT,NULL
        STS     PS2M_RAW_READY,NULL
        SEI

        LDIZ    WIND_T_PS2M_1*2
        CALL    WINDOW
        LDI     XL,4
        LDI     XH,3
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF
        LDIZ    MLMSG_MOUSE_DETECT*2
        CALL    PRINTMLSTR
        LDI     XL,4
        LDI     XH,4
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF
;
;
T_PSM_DETECT_2:
        STS     PS2M_RAW_READY,NULL
        LDIZ    PS2M_TIMEOUT
        LDIW    2
        CALL    SET_TIMEOUT_MS
T_PSM_DETECT_1:
        LDS     TEMP,PS2M_RAW_READY
        TST     TEMP
        BRNE    T_PSM_DETECT_2
        LDIZ    PS2M_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    T_PSM_DETECT_1

        LDI     DATA,$FF
        RCALL   HEXBYTE_FOR_DUMP
        RCALL   PS2M_SEND_BYTE
        BREQ    T_PSM_DETECT_FAIL0_A
        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM_DETECT_FAIL0_A
        RCALL   HEXBYTE_FOR_DUMP
        CPI     DATA,$FA
        BRNE    T_PSM_DETECT_FAIL0_A

        STS     PS2M_RAW_READY,NULL
        LDIZ    PS2M_TIMEOUT
        LDIW    1000
        CALL    SET_TIMEOUT_MS
T_PSM_DETECT_4:
        LDS     TEMP,PS2M_RAW_READY
        TST     TEMP
        BRNE    T_PSM_DETECT_5
        LDIZ    PS2M_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    T_PSM_DETECT_4
T_PSM_DETECT_FAIL2_A:
        RJMP    T_PSM_DETECT_FAIL2
;
T_PSM_DETECT_FAIL0_A:
        RJMP    T_PSM_DETECT_FAIL0
;
T_PSM_DETECT_5:
        LDS     DATA,PS2M_RAW_CODE
        RCALL   HEXBYTE_FOR_DUMP
        CPI     DATA,$AA
        BRNE    T_PSM_DETECT_FAIL2_A
        LDI     DATA,$00
        RCALL   T_PSM_DETECT_RECEIVE
; - - - - - - - - - - - - - - - - - - -
        LDI     XL,4
        LDI     XH,5
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF
        LDIZ    MLMSG_MOUSE_SETUP*2
        CALL    PRINTMLSTR
        LDI     XL,4
        LDI     XH,6
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF

        LDI     DATA,$F3
        RCALL   T_PSM_DETECT_SEND
        LDI     DATA,200
        RCALL   T_PSM_DETECT_SEND

        LDI     DATA,$F3
        RCALL   T_PSM_DETECT_SEND
        LDI     DATA,100
        RCALL   T_PSM_DETECT_SEND

        LDI     DATA,$F3
        RCALL   T_PSM_DETECT_SEND
        LDI     DATA,80
        RCALL   T_PSM_DETECT_SEND

        LDI     XL,4
        LDI     XH,7
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF

        LDI     DATA,$F2
        RCALL   T_PSM_DETECT_SEND

        STS     PS2M_RAW_READY,NULL
        LDIZ    PS2M_TIMEOUT
        LDIW    20
        CALL    SET_TIMEOUT_MS
T_PSM_DETECT_6:
        LDS     TEMP,PS2M_RAW_READY
        TST     TEMP
        BRNE    T_PSM_DETECT_7
        LDIZ    PS2M_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    T_PSM_DETECT_6
        RJMP    T_PSM_DETECT_FAIL2
T_PSM_DETECT_7:
        LDS     DATA,PS2M_RAW_CODE
        STH     TPSM_ID,DATA
        RCALL   HEXBYTE_FOR_DUMP
        TST     DATA
        BREQ    T_PSM_DETECT_8
        CPI     DATA,3
        BREQ    T_PSM_DETECT_8
        RJMP    T_PSM_DETECT_FAIL2

T_PSM_DETECT_8:
        LDI     XL,4
        LDI     XH,8
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF

        LDI     DATA,$E8
        RCALL   T_PSM_DETECT_SEND
        LDI     DATA,$02
        RCALL   T_PSM_DETECT_SEND

        LDI     DATA,$E6
        RCALL   T_PSM_DETECT_SEND

        LDI     DATA,$F3
        RCALL   T_PSM_DETECT_SEND
        LDI     DATA,100
        RCALL   T_PSM_DETECT_SEND

        LDI     DATA,$F4
        RCALL   T_PSM_DETECT_SEND

        LDI     XL,4
        LDI     XH,9
        RCALL   SCR_SET_CURSOR
        RCALL   UART_CRLF
        LDIZ    MLMSG_MOUSE_LETSGO*2
        CALL    PRINTMLSTR
        RCALL   UART_CRLF

        RJMP    T_PSM_MAIN
;
;--------------------------------------
;
T_PSM_DETECT_SEND:
        RCALL   HEXBYTE_FOR_DUMP
        RCALL   PS2M_SEND_BYTE
        BREQ    T_PSM_DETECT_FAIL1
        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM_DETECT_FAIL1
        RCALL   HEXBYTE_FOR_DUMP
        CPI     DATA,$FA
        BRNE    T_PSM_DETECT_FAIL1
        RET
;
;--------------------------------------
;
T_PSM_DETECT_RECEIVE:
        STH     TPSM_BYTE4,DATA         ; temporally
        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM_DETECT_FAIL1
        RCALL   HEXBYTE_FOR_DUMP
        LDH     TEMP,TPSM_BYTE4         ; temporally
        CP      DATA,TEMP
        BRNE    T_PSM_DETECT_FAIL1
        RET
;
;--------------------------------------
;
T_PSM_DETECT_FAIL1:
        POPZ
T_PSM_DETECT_FAIL2:
        LDIZ    MLMSG_MOUSE_FAIL1*2
        RJMP    T_PSM_DETECT_FAILZ

T_PSM_DETECT_FAIL0:
        LDIZ    MLMSG_MOUSE_FAIL0*2
T_PSM_DETECT_FAILZ:
        PUSHZ
        RCALL   UART_CRLF
        LDIZ    WIND_T_PS2M_2*2
        CALL    WINDOW
        LDI     XL,10
        LDI     XH,11
        RCALL   SCR_SET_CURSOR
        POPZ
        CALL    PRINTMLSTR
        LDI     XL,10
        LDI     XH,12
        RCALL   SCR_SET_CURSOR
        LDIZ    MLMSG_MOUSE_RESTART*2
        CALL    PRINTMLSTR

T_PSM_WAITKEY:
        CALL    WAITKEY
        CPI     DATA,KEY_ESC
        BREQ    T_PSM_ESCAPE
        CPI     DATA,KEY_ENTER
        BRNE    T_PSM_WAITKEY
        RJMP    T_PSM_RESTART

T_PSM_ESCAPE:
        CLR     DATA
        LDI     TEMP,SCR_MOUSE_TEMP
        RCALL   FPGA_REG
        CLR     DATA
        LDI     TEMP,SCR_MOUSE_X
        RCALL   FPGA_REG
        CLR     DATA
        LDI     TEMP,SCR_MOUSE_Y
        RCALL   FPGA_REG
        CLI
        PS2M_DATALINE_UP
        PS2M_CLOCKLINE_UP
        IN      TEMP,EIMSK
        CBR     TEMP,(1<<INT5)
        OUT     EIMSK,TEMP
        SEI
        ORI     FLAGS1,0B00000010
        FREEMEM 10
        RET
;
;--------------------------------------
;
T_PSM_MAIN:
        ANDI    FLAGS1,0B11111100

        LDIZ    WIND_T_PS2M_3*2
        CALL    WINDOW
        LDIZ    MSG_TPSM_2*2
        LDH     TEMP,TPSM_ID
        TST     TEMP
        BREQ    T_PSM01
        LDIZ    MSG_TPSM_1*2
T_PSM01:RCALL   SCR_PRINTSTRZ
        LDI     TEMP,150
        STH     TPSM_X0,TEMP
        STH     TPSM_X1,NULL
        LDI     TEMP,120
        STH     TPSM_Y,TEMP
        STH     TPSM_Z,NULL
        STH     TPSM_BTN,NULL

        STS     PS2M_RAW_READY,NULL

T_PSM10:
        LDH     TEMP,TPSM_ID
        TST     TEMP
        BREQ    T_PSM11
        LDI     XL,45
        LDI     XH,15
        RCALL   SCR_SET_CURSOR
        LDH     DATA,TPSM_Z
        CALL    HEXHALF
T_PSM11:
        LDI     XL,41
        LDI     XH,17
        RCALL   SCR_SET_CURSOR
        LDH     XL,TPSM_X0
        LDH     XH,TPSM_X1
        CALL    DECWORD

        LDI     XL,41
        LDI     XH,18
        RCALL   SCR_SET_CURSOR
        LDH     XL,TPSM_Y
        CLR     XH
        CALL    DECWORD

        LDI     XH,11
T_PSM12:PUSH    XH
        LDI     XL,36
        RCALL   SCR_SET_CURSOR
        LDI     TEMP,$DF
        LDH     DATA,TPSM_BTN
        SBRC    DATA,0
        LDI     TEMP,$AE
        LDI     COUNT,3
        RCALL   SCR_FILL_ATTR
        LDI     TEMP,$DF
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
        LDI     TEMP,$DF
        LDH     DATA,TPSM_BTN
        SBRC    DATA,2
        LDI     TEMP,$AE
        LDI     COUNT,3
        RCALL   SCR_FILL_ATTR
        LDI     TEMP,$DF
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
        LDI     TEMP,$DF
        LDH     DATA,TPSM_BTN
        SBRC    DATA,1
        LDI     TEMP,$AE
        LDI     COUNT,3
        RCALL   SCR_FILL_ATTR
        POP     XH
        INC     XH
        CPI     XH,14
        BRCS    T_PSM12

        LDH     XL,TPSM_X0
        LDH     XH,TPSM_X1
        ADIW    XL,49
        ADIW    XL,49 ; +98
        PUSH    XL
        MOV     DATA,XH
        LDI     TEMP,SCR_MOUSE_TEMP
        RCALL   FPGA_REG
        POP     DATA
        LDI     TEMP,SCR_MOUSE_X
        RCALL   FPGA_REG
        LDH     XL,TPSM_Y
        CLR     XH
        ADIW    XL,44
        PUSH    XL
        MOV     DATA,XH
        LDI     TEMP,SCR_MOUSE_TEMP
        RCALL   FPGA_REG
        POP     DATA
        LDI     TEMP,SCR_MOUSE_Y
        RCALL   FPGA_REG

T_PSM20:
        CALL    INKEY
        BREQ    T_PSM21
        CPI     DATA,KEY_ESC
        BRNE    T_PSM21
        RJMP    T_PSM_ESCAPE
T_PSM21:
        LDS     DATA,PS2M_RAW_READY
        TST     DATA
        BREQ    T_PSM20
        STS     PS2M_RAW_READY,NULL
        LDS     DATA,PS2M_RAW_CODE
        SBRS    DATA,3
        RJMP    T_PSM20
        STH     TPSM_BYTE1,DATA

        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM20
        STH     TPSM_BYTE2,DATA

        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM20
        STH     TPSM_BYTE3,DATA

        LDH     TEMP,TPSM_ID
        TST     TEMP
        BREQ    T_PSM30

        RCALL   PS2M_RECEIVE_BYTE
        BREQ    T_PSM20
        STH     TPSM_BYTE4,DATA
;
T_PSM30:
        LDH     DATA,TPSM_BYTE1
        ANDI    DATA,$07
        STH     TPSM_BTN,DATA

        LDH     XL,TPSM_X0
        LDH     XH,TPSM_X1
        LDH     WL,TPSM_BYTE2
        CLR     WH
        LDH     TEMP,TPSM_BYTE1
        SBRC    TEMP,4
        RJMP    T_PSM31
        ADD     XL,WL
        ADC     XH,NULL
        CPI     XL,LOW(318)
        CPC     XH,ONE ;HIGH(318)
        BRCS    T_PSM32
        LDI     XL,LOW(317)
        LDI     XH,HIGH(317)
        RJMP    T_PSM32
T_PSM31:COM     WL
        ADIW    WL,1
        SUB     XL,WL
        SBC     XH,WH
        BRCC    T_PSM32
        CLR     XL
        CLR     XH
T_PSM32:STH     TPSM_X0,XL
        STH     TPSM_X1,XH

        LDH     XL,TPSM_Y
        CLR     XH
        LDH     WL,TPSM_BYTE3
        CLR     WH
        LDH     TEMP,TPSM_BYTE1
        SBRS    TEMP,5
        RJMP    T_PSM33
        COM     WL
        ADIW    WL,1
        ADD     XL,WL
        ADC     XH,WH
        TST     XH
        BRNE    T_PSM34
        CPI     XL,250
        BRCS    T_PSM35
T_PSM34:LDI     XL,249
        RJMP    T_PSM35
T_PSM33:SUB     XL,WL
        SBC     XH,NULL
        BRCC    T_PSM35
        CLR     XL
T_PSM35:STH     TPSM_Y,XL

        LDH     TEMP,TPSM_ID
        TST     TEMP
        BREQ    T_PSM40

        LDH     DATA,TPSM_BYTE4
        LDH     XL,TPSM_Z
        ADD     XL,DATA
        ANDI    XL,$0F
        STH     TPSM_Z,XL

T_PSM40:
        LDIZ    MSG_TPSM_3*2
        RCALL   SCR_PRINTSTRZ

        ORI     FLAGS1,0B00000010
        LDH     DATA,TPSM_BYTE1
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
        LDH     DATA,TPSM_BYTE2
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
        LDH     DATA,TPSM_BYTE3
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
        LDH     TEMP,TPSM_ID
        TST     TEMP
        BREQ    T_PSM41
        LDH     DATA,TPSM_BYTE4
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
T_PSM41:LDI     DATA,$20
        RCALL   PUTCHAR
        ANDI    FLAGS1,0B11111100

        RJMP    T_PSM10
;
;--------------------------------------
;in:    DATA
;out:   sreg.Z - CLR == ok; SET == timeout
PS2M_SEND_BYTE:
        CLR     TEMP
PS2M_SEND0:
        SBIS    PINE,5   ; clock line
        RJMP    PS2M_SEND_BYTE
        DEC     TEMP
        BRNE    PS2M_SEND0

        CLI
        STS     PS2M_DATA,DATA
        LDI     TEMP,(1<<PS2M_BIT_TX)
        STS     PS2M_FLAGS,TEMP
        STS     PS2M_BIT_COUNT,NULL
        STS     PS2M_RAW_READY,NULL
        PS2M_CLOCKLINE_DOWN
        SEI
        DELAY_US 130
        PS2M_DATALINE_DOWN
        DELAY_US 20
        LDIZ    PS2M_TIMEOUT
        LDIW    15
        CALL    SET_TIMEOUT_MS
        PS2M_CLOCKLINE_UP
PS2M_SEND1:
        LDS     TEMP,PS2M_FLAGS
        ANDI    TEMP,(1<<PS2M_BIT_ACKBIT)
        BRNE    PS2M_SEND2
        LDIZ    PS2M_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2M_SEND1
        SEZ
PS2M_SEND2:
        RET
;
;--------------------------------------
;out:   sreg.Z - CLR == ok; SET == timeout
;       DATA == byte
PS2M_RECEIVE_BYTE:
        STS     PS2M_RAW_READY,NULL
        LDIZ    PS2M_TIMEOUT
        LDIW    7
        CALL    SET_TIMEOUT_MS

PS2M_RXBYTE1:
        LDS     TEMP,PS2M_RAW_READY
        TST     TEMP
        BRNE    PS2M_RXBYTE2
        LDIZ    PS2M_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2M_RXBYTE1
        SEZ
        RET

PS2M_RXBYTE2:
        LDS     DATA,PS2M_RAW_CODE
        STS     PS2M_RAW_READY,NULL
        RET
;
;--------------------------------------
;
