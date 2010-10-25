.EQU    KEY_ESC         =$76
.EQU    KEY_ENTER       =$5A
.EQU    KEY_UP          =$75
.EQU    KEY_DOWN        =$72
.EQU    KEY_LEFT        =$6B
.EQU    KEY_RIGHT       =$74
.EQU    KEY_PAGEUP      =$7D
.EQU    KEY_PAGEDOWN    =$7A
.EQU    KEY_HOME        =$6C
.EQU    KEY_END         =$69
.EQU    KEY_F1          =$05
.EQU    KEY_NUMLOCK     =$77
.EQU    KEY_CAPSLOCK    =$58
.EQU    KEY_SCROLLLOCK  =$7E
.EQU    KEY_Y           =$35

.EQU    PS2K_BIT_PARITY =0
.EQU    PS2K_BIT_EXTKEY =1      ; расш.код
.EQU    PS2K_BIT_RELEASE=2      ; отпускание
.EQU    PS2K_BIT_ACKBIT =3      ; принят ACK-бит
.EQU    PS2K_BIT_TX     =7      ; передача
.EQU    PS2K_BIT_READY  =7

.MACRO  PS2K_DATALINE_UP
        CBI     DDRD,6
        SBI     PORTD,6
.ENDMACRO

.MACRO  PS2K_DATALINE_DOWN
        CBI     PORTD,6
        SBI     DDRD,6
.ENDMACRO

.MACRO  PS2K_CLOCKLINE_UP
        CBI     DDRE,4
        SBI     PORTE,4
.ENDMACRO

.MACRO  PS2K_CLOCKLINE_DOWN
        CBI     PORTE,4
        SBI     DDRE,4
.ENDMACRO
;
;--------------------------------------
;
.DSEG
PS2K_BIT_COUNT: .BYTE   1
PS2K_DATA:      .BYTE   1
PS2K_RAW_READY: .BYTE   1
PS2K_RAW_CODE:  .BYTE   1
PS2K_SKIP:      .BYTE   1
PS2K_FLAGS:     .BYTE   1
PS2K_KEY_FLAGS: .BYTE   1
PS2K_KEY_CODE:  .BYTE   1
PS2K_TIMEOUT:   .BYTE   2
.CSEG
;
;--------------------------------------
;
PS2K_INIT:

        IN      TEMP,EICRB
        ORI     TEMP,(1<<ISC41)|(0<<ISC40)
        OUT     EICRB,TEMP
        IN      TEMP,EIMSK
        ORI     TEMP,(1<<INT4)
        OUT     EIMSK,TEMP

        LDI     TEMP,(0<<CS02)|(1<<CS01)|(0<<CS00) ; clk/8
        OUT     TCCR0,TEMP

        STS     PS2K_FLAGS,NULL
        STS     PS2K_BIT_COUNT,NULL
        STS     PS2K_SKIP,NULL
        STS     PS2K_KEY_FLAGS,NULL

        RET
;
;--------------------------------------
;
TIM0_OVF:
        PS2K_DATALINE_UP
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP
        IN      TEMP,TIMSK
        CBR     TEMP,(1<<TOIE0)
        OUT     TIMSK,TEMP
        POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        PS2K_CLOCKLINE_UP
        RETI
;
;--------------------------------------
;
EXT_INT4:
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP
        PUSH    COUNT
        PUSH    DATA
        LDS     DATA,PS2K_DATA
        LDS     COUNT,PS2K_BIT_COUNT
        LDS     TEMP,PS2K_FLAGS
        SBRC    TEMP,PS2K_BIT_TX
        RJMP    INT4TX_0

        CPI     COUNT,9
        BREQ    INT4RX9
        CPI     COUNT,10
        BRCC    INT4RXS
        TST     COUNT
        BRNE    INT4RX1
;start bit
        SBIC    PIND,6   ; data line
        RJMP    INT4RX_CLR_D
        CBR     TEMP,(1<<PS2K_BIT_PARITY)
        INC     COUNT
        RJMP    INT4_EXIT
;data bits
INT4RX1:LSR     DATA
        SBIS    PIND,6   ; data line
        RJMP    INT4RX2
        ORI     DATA,0B10000000
        EOR     TEMP,ONE
INT4RX2:INC     COUNT
        RJMP    INT4_EXIT
;parity bit
INT4RX9:SBIC    PIND,6   ; data line
        EOR     TEMP,ONE
        SBRS    TEMP,PS2K_BIT_PARITY
        RJMP    INT4RX_CLR_D
        INC     COUNT
        RJMP    INT4_EXIT
;stop bit
INT4RXS:SBIS    PIND,6   ; data line
        RJMP    INT4RX_CLR_D

        STS     PS2K_RAW_READY,COUNT
        STS     PS2K_RAW_CODE,DATA
        PS2K_CLOCKLINE_DOWN
        LDI     COUNT,$80
        OUT     TCNT0,COUNT     ;Tclk*8*128=~92us
        LDI     COUNT,(1<<TOV0)
        OUT     TIFR,COUNT
        IN      COUNT,TIMSK
        ORI     COUNT,(1<<TOIE0)
        OUT     TIMSK,COUNT

        LDS     COUNT,PS2K_SKIP
        TST     COUNT
        BRNE    INT4RX_SKIP

        CPI     DATA,$E1
        BREQ    INT4RX_E1
        CPI     DATA,$AA
        BREQ    INT4RX_CLR_F
        CPI     DATA,$AB
        BREQ    INT4RX_AB
        CPI     DATA,$EE
        BREQ    INT4RX_CLR_F
        CPI     DATA,$FA
        BREQ    INT4RX_CLR_F
        CPI     DATA,$FE
        BREQ    INT4RX_CLR_F
        CPI     DATA,$E0
        BREQ    INT4RX_E0
        CPI     DATA,$F0
        BREQ    INT4RX_F0

        SBR     TEMP,(1<<PS2K_BIT_READY)
        STS     PS2K_KEY_FLAGS,TEMP
        STS     PS2K_KEY_CODE,DATA
        RJMP    INT4RX_CLR_F

INT4RX_E0:
        SBR     TEMP,(1<<PS2K_BIT_EXTKEY)
        RJMP    INT4RX_CLR_D

INT4RX_F0:
        SBR     TEMP,(1<<PS2K_BIT_RELEASE)
        RJMP    INT4RX_CLR_D

INT4RX_SKIP:
        DEC     COUNT
        STS     PS2K_SKIP,COUNT
        BRNE    INT4RX_CLR_D
        LDS     DATA,PS2K_KEY_CODE
        CPI     DATA,$7E
        BRNE    INT4RX_CLR_D
        LDI     TEMP,(1<<PS2K_BIT_READY)|(1<<PS2K_BIT_RELEASE)|(1<<PS2K_BIT_EXTKEY)
        STS     PS2K_KEY_FLAGS,TEMP
        RJMP    INT4RX_CLR_F

INT4RX_AB:
        STS     PS2K_KEY_CODE,NULL
        LDI     COUNT,1
        STS     PS2K_SKIP,COUNT
        RJMP    INT4RX_CLR_F

INT4RX_E1:
        LDI     TEMP,(1<<PS2K_BIT_READY)|(1<<PS2K_BIT_EXTKEY)
        STS     PS2K_KEY_FLAGS,TEMP
        LDI     DATA,$7E
        STS     PS2K_KEY_CODE,DATA
        LDI     COUNT,7
        STS     PS2K_SKIP,COUNT

INT4RX_CLR_F:
        CLR     TEMP
INT4RX_CLR_D:
        CLR     DATA
        CLR     COUNT
        RJMP    INT4_EXIT
;-------
INT4TX_0:
        CPI     COUNT,9
        BREQ    INT4TX9
        CPI     COUNT,10
        BREQ    INT4TXS
        CPI     COUNT,11
        BRCC    INT4TXA
        TST     COUNT
        BRNE    INT4TX1
;start bit
        SBR     TEMP,(1<<PS2K_BIT_PARITY)
        INC     COUNT
        RJMP    INT4_EXIT
;data bits
INT4TX1:ROR     DATA
        BRCC    INT4TX2
        PS2K_DATALINE_UP
        EOR     TEMP,ONE
        INC     COUNT
        RJMP    INT4_EXIT
INT4TX2:PS2K_DATALINE_DOWN
        INC     COUNT
        RJMP    INT4_EXIT
;parity bit
INT4TX9:SBRC    TEMP,PS2K_BIT_PARITY
        RJMP    INT4TXP
        PS2K_DATALINE_DOWN
        INC     COUNT
        RJMP    INT4_EXIT
INT4TXP:PS2K_DATALINE_UP
        INC     COUNT
        RJMP    INT4_EXIT
;stop bit
INT4TXS:PS2K_DATALINE_UP
        INC     COUNT
        RJMP    INT4_EXIT
;ack-bit
INT4TXA:CLR     TEMP
        SBIS    PIND,6   ; data line
        LDI     TEMP,(1<<PS2K_BIT_ACKBIT)
        CLR     DATA
        CLR     COUNT

INT4_EXIT:
        STS     PS2K_BIT_COUNT,COUNT
        STS     PS2K_DATA,DATA
        STS     PS2K_FLAGS,TEMP
        POP     DATA
        POP     COUNT
        POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        RETI
;
;--------------------------------------
;out:   DATA == скан-код клавиши
;       TEMP.PS2K_BIT_EXTKEY == расш.код
WAITKEY:CALL    RANDOM
        LDS     TEMP,PS2K_KEY_FLAGS
        SBRS    TEMP,PS2K_BIT_READY
        RJMP    WAITKEY
        STS     PS2K_KEY_FLAGS,NULL
        SBRC    TEMP,PS2K_BIT_RELEASE
        RJMP    WAITKEY
        LDS     DATA,PS2K_KEY_CODE
        RET
;
;--------------------------------------
;out:   sreg.Z == clr - было нажатие (автоповтор)
;       DATA == скан-код клавиши
;       TEMP.PS2K_BIT_EXTKEY == расш.код
INKEY:  CALL    RANDOM
        LDS     TEMP,PS2K_KEY_FLAGS
        SBRS    TEMP,PS2K_BIT_READY
        RJMP    INKEY9
        STS     PS2K_KEY_FLAGS,NULL
        SBRC    TEMP,PS2K_BIT_RELEASE
        RJMP    INKEY9
        LDS     DATA,PS2K_KEY_CODE
        CLZ
        RET

INKEY9: SEZ
        RET
;
;--------------------------------------
;in:    DATA
;out:   sreg.Z - CLR == ok; SET == timeout
PS2K_SEND_BYTE:
        PS2K_DATALINE_UP
        IN      TEMP,TIMSK
        CBR     TEMP,(1<<TOIE0)
        OUT     TIMSK,TEMP
        PS2K_CLOCKLINE_UP
        CLR     TEMP
PS2K_SEND0:
        SBIS    PINE,4   ; clock line
        RJMP    PS2K_SEND_BYTE
        DEC     TEMP
        BRNE    PS2K_SEND0

        CLI
        PS2K_CLOCKLINE_DOWN
        STS     PS2K_DATA,DATA
        LDI     TEMP,(1<<PS2K_BIT_TX)
        STS     PS2K_FLAGS,TEMP
        STS     PS2K_BIT_COUNT,NULL
        STS     PS2K_SKIP,NULL
        STS     PS2K_KEY_FLAGS,NULL
        STS     PS2K_RAW_READY,NULL
        SEI
        DELAY_US 100
        PS2K_DATALINE_DOWN
        DELAY_US 50
        LDIZ    PS2K_TIMEOUT
        LDIW    15
        CALL    SET_TIMEOUT_MS
        PS2K_CLOCKLINE_UP
PS2K_SEND1:
        LDS     TEMP,PS2K_FLAGS
        ANDI    TEMP,(1<<PS2K_BIT_ACKBIT)
        BRNE    PS2K_SEND2
        LDIZ    PS2K_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2K_SEND1
        PS2K_DATALINE_UP
        SEZ
PS2K_SEND2:
        RET
;
;--------------------------------------
;out:   sreg.Z - CLR == ok; SET == timeout
;       DATA == byte
PS2K_RECEIVE_BYTE:
        STS     PS2K_RAW_READY,NULL
        LDIZ    PS2K_TIMEOUT
        LDIW    20
        CALL    SET_TIMEOUT_MS

PS2K_RXBYTE1:
        LDS     TEMP,PS2K_RAW_READY
        TST     TEMP
        BRNE    PS2K_RXBYTE2
        LDIZ    PS2K_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2K_RXBYTE1
        CLR     DATA
        SEZ
        RET

PS2K_RXBYTE2:
        LDS     DATA,PS2K_RAW_CODE
        RET
;
;--------------------------------------
;
.EQU    PS2K_DETECT_TEMP=0
;
PS2K_DETECT_KBD:
        GETMEM  1
        PS2K_CLOCKLINE_UP
        LDIZ    MLMSG_KBD_DETECT*2
        RCALL   PRINTMLSTR

        STS     PS2K_RAW_READY,NULL
        LDIZ    PS2K_TIMEOUT
        LDIW    500
        CALL    SET_TIMEOUT_MS
PS2K_DETECT_K1:
        LDS     TEMP,PS2K_RAW_READY
        TST     TEMP
        BRNE    PS2K_DETECT_K2
        LDIZ    PS2K_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2K_DETECT_K1
        RJMP    PS2K_DETECT_K3
PS2K_DETECT_K2:
        LDS     DATA,PS2K_RAW_CODE
        RCALL   HEXBYTE_FOR_DUMP
PS2K_DETECT_K3:

        LDI     DATA,$FF
        RCALL   PS2K_DETECT_SEND
;        BRNE    PS2K_DETECT_K6
        STS     PS2K_RAW_READY,NULL
        LDIZ    PS2K_TIMEOUT
        LDIW    1000
        CALL    SET_TIMEOUT_MS
PS2K_DETECT_K4:
        LDS     TEMP,PS2K_RAW_READY
        TST     TEMP
        BRNE    PS2K_DETECT_K5
        LDIZ    PS2K_TIMEOUT
        CALL    CHECK_TIMEOUT_MS
        BRCC    PS2K_DETECT_K4
        RCALL   PS2K_DETECT_NORESPONSE
        RJMP    PS2K_DETECT_K6
PS2K_DETECT_K5:
        LDS     DATA,PS2K_RAW_CODE
        RCALL   HEXBYTE_FOR_DUMP
        CPI     DATA,$AA
        BREQ    PS2K_DETECT_K6
        RCALL   PS2K_DETECT_UNWANTED

PS2K_DETECT_K6:
        LDI     DATA,$F2
        RCALL   PS2K_DETECT_SEND
        BRNE    PS2K_DETECT_K7
        LDI     DATA,$AB
        RCALL   PS2K_DETECT_RECEIVE
        BRNE    PS2K_DETECT_K7
        LDI     DATA,$83
        RCALL   PS2K_DETECT_RECEIVE
PS2K_DETECT_K7:
        LDI     DATA,$F0
        RCALL   PS2K_DETECT_SEND
        BRNE    PS2K_DETECT_K8
        LDI     DATA,$02
        RCALL   PS2K_DETECT_SEND
PS2K_DETECT_K8:
        LDI     DATA,$F3
        RCALL   PS2K_DETECT_SEND
        BRNE    PS2K_DETECT_K9
        LDI     DATA,$00
        RCALL   PS2K_DETECT_SEND
        LDIZ    MSG_READY*2+3
        CALL    PRINTSTRZ
PS2K_DETECT_K9:
        FREEMEM 1
        RET
;
PS2K_DETECT_SEND:
        RCALL   HEXBYTE_FOR_DUMP
        RCALL   PS2K_SEND_BYTE
        BREQ    PS2K_DETECT_TXFAIL
        RCALL   PS2K_RECEIVE_BYTE
        BREQ    PS2K_DETECT_NORESPONSE
        RCALL   HEXBYTE_FOR_DUMP
        CPI     DATA,$FA
        BRNE    PS2K_DETECT_UNWANTED
        RET
;
PS2K_DETECT_RECEIVE:
        STH     PS2K_DETECT_TEMP,DATA
        RCALL   PS2K_RECEIVE_BYTE
        BREQ    PS2K_DETECT_NORESPONSE
        RCALL   HEXBYTE_FOR_DUMP
        LDH     TEMP,PS2K_DETECT_TEMP
        CP      DATA,TEMP
        BRNE    PS2K_DETECT_UNWANTED
        RET
;
PS2K_DETECT_TXFAIL:
        LDIZ    MLMSG_TXFAIL*2
        RJMP    PS2K_DETECT_XXFAIL
PS2K_DETECT_UNWANTED:
        LDIZ    MLMSG_UNWANTED*2
        RJMP    PS2K_DETECT_XXFAIL
PS2K_DETECT_NORESPONSE:
        LDIZ    MLMSG_NORESPONSE*2
PS2K_DETECT_XXFAIL:
        CALL    PRINTMLSTR
        CLZ
        RET
;
;--------------------------------------
;
