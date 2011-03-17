.MACRO  RTS_SET
        SBI     PORTD,5
.ENDMACRO

.MACRO  RTS_CLR
        CBI     PORTD,5
.ENDMACRO
;
;--------------------------------------
;
.DSEG
UART_TX_HD:     .BYTE   1
UART_TX_TL:     .BYTE   1
UART_TX_LN:     .BYTE   1
UART_RX_HD:     .BYTE   1
UART_RX_TL:     .BYTE   1
UART_RX_LN:     .BYTE   1
;        .ORG    $0300                   ;см. SERVICE.ASM
;.EQU    UART_TXBSIZE    =128            ;размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт (32,64,128 или 256)
;UART_TX_BUFF:   .BYTE   UART_TXBSIZE    ;адрес д.б. кратен UART_TXBSIZE
;.EQU    UART_RXBSIZE    =128            ;размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт (32,64,128 или 256)
;UART_RX_BUFF:   .BYTE   UART_RXBSIZE    ;адрес д.б. кратен UART_RXBSIZE
.CSEG
;
;--------------------------------------
;
UARTDIRECT_INIT:
        RCALL   UART_SET_BAUDRATE
        LDI     TEMP,(1<<TXEN)  ;Разрешаем передачу
        OUTPORT UCSR1B,TEMP     ;
        CBR     FLAGS1,0B00000010
        SBR     FLAGS1,0B00000001
        RET
;
;--------------------------------------
;
UARTDIRECT_CRLF:
        LDI     DATA,$0D
        RCALL   UARTDIRECT_PUTCHAR
        LDI     DATA,$0A
;
;--------------------------------------
;in:    DATA == передаваемый байт
UARTDIRECT_PUTCHAR:
UD_PCHR:INPORT  R0,UCSR1A
        SBRS    R0,UDRE
        RJMP    UD_PCHR
        OUTPORT UDR1,DATA
        RET
;
;--------------------------------------
;
UART_SET_BAUDRATE:
        OUTPORT UBRR1H,NULL
        LDI     TEMP,5 ;115200 baud (CPU @ 11.0592 MHz), Normal speed
        OUTPORT UBRR1L,TEMP
        OUTPORT UCSR1A,NULL ;Normal Speed
        LDI     TEMP,(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS) ;data8bit, 2stopbits
        OUTPORT UCSR1C,TEMP
        RET
;
;--------------------------------------
;
UART_INIT:
        RCALL   UART_SET_BAUDRATE
        LDI     TEMP,(1<<RXCIE)|(1<<RXEN)|(1<<TXEN)
        OUTPORT UCSR1B,TEMP
        CBR     FLAGS1,0B00000001
        SBR     FLAGS1,0B00000010

        LDI     TEMP,LOW(UART_TX_BUFF)
        STS     UART_TX_HD,TEMP
        STS     UART_TX_TL,TEMP
        STS     UART_TX_LN,NULL
        LDI     TEMP,LOW(UART_RX_BUFF)
        STS     UART_RX_HD,TEMP
        STS     UART_RX_TL,TEMP
        STS     UART_RX_LN,NULL

        RTS_CLR
        SBI     DDRD,5
        CBI     DDRB,6

        RET
;
;--------------------------------------
;USART1 RX Interrupt handler
USART1_RXC:
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP
        PUSH    DATA

        INPORT  DATA,UDR1
        LDS     TEMP,UART_RX_LN
        CPI     TEMP,UART_RXBSIZE ;буфер полный?
        BRCC    U1RX9
        PUSHX
        LDI     XH,HIGH(UART_RX_BUFF)
        LDS     XL,UART_RX_HD
        ST      X+,DATA
        ANDI    XL,UART_RXBSIZE-1
        ORI     XL,LOW(UART_RX_BUFF)
        STS     UART_RX_HD,XL
        POPX
        INC     TEMP
        STS     UART_RX_LN,TEMP
        CPI     TEMP,UART_RXBSIZE-20
        BRCS    U1RX9
        SBRC    FLAGS1,4
        RTS_SET
U1RX9:
        POP     DATA
        POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        RETI
;
;--------------------------------------
;USART1 UDR Empty Interrupt handler
USART1_DRE:
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP

        LDS     TEMP,UART_TX_LN
        TST     TEMP ;есть что в буфере?
        BREQ    U1TX1
        SBRS    FLAGS1,4
        RJMP    U1TX2
        SBIC    PINB,6
        RJMP    U1TX1

U1TX2:  PUSHX
        PUSH    DATA
        LDI     XH,HIGH(UART_TX_BUFF)
        LDS     XL,UART_TX_TL
        LD      DATA,X+
        OUTPORT UDR1,DATA
        ANDI    XL,UART_TXBSIZE-1
        ORI     XL,LOW(UART_TX_BUFF)
        STS     UART_TX_TL,XL
        DEC     TEMP
        STS     UART_TX_LN,TEMP
        POP     DATA
        POPX
        RJMP    U1TX9

U1TX1:  INPORT  TEMP,UCSR1B
        CBR     TEMP,(1<<UDRIE) ;Запрещаем прерывания Empty Data Register
        OUTPORT UCSR1B,TEMP

U1TX9:  POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        RETI
;
;--------------------------------------
;Put byte to UART buffer
;in:    DATA
UART_PUTCHAR:
        PUSH    TEMP
        PUSHX

U_PCHR1:SBRC    FLAGS1,4
        RCALL   UART_CHK_CTS
        LDS     TEMP,UART_TX_LN
        CPI     TEMP,UART_TXBSIZE ;буфер полный?
        BRCC    U_PCHR1

        LDI     XH,HIGH(UART_TX_BUFF)
        LDS     XL,UART_TX_HD
        ST      X+,DATA
        CLI
        LDS     TEMP,UART_TX_LN
        INC     TEMP
        STS     UART_TX_LN,TEMP
        SEI
        ANDI    XL,UART_TXBSIZE-1
        ORI     XL,LOW(UART_TX_BUFF)
        STS     UART_TX_HD,XL

        SBRS    FLAGS1,4
        RJMP    U_PCHR8
        SBIC    PINB,6
        RJMP    U_PCHR9
U_PCHR8:INPORT  TEMP,UCSR1B
        SBR     TEMP,(1<<UDRIE) ;Разрешаем прерывания Empty Data Register
        OUTPORT UCSR1B,TEMP

U_PCHR9:POPX
        POP     TEMP
        RET
;
;--------------------------------------
;Get byte from UART buffer
;out:   sreg.Z == SET - нет данных
;       DATA == данные
UART_GETCHAR:
        PUSH    TEMP
        LDS     TEMP,UART_RX_LN
        TST     TEMP
        BREQ    U_GCHR9
        PUSHX
        LDI     XH,HIGH(UART_RX_BUFF)
        LDS     XL,UART_RX_TL
        LD      DATA,X+
        CLI
        LDS     TEMP,UART_RX_LN
        DEC     TEMP
        STS     UART_RX_LN,TEMP
        SEI
        CPI     TEMP,UART_RXBSIZE-21
        BRCC    U_GCHR1
        RTS_CLR
U_GCHR1:ANDI    XL,UART_RXBSIZE-1
        ORI     XL,LOW(UART_RX_BUFF)
        STS     UART_RX_TL,XL
        CLZ
        POPX
U_GCHR9:POP     TEMP
        RET
;
;--------------------------------------
;
UART_CRLF:
        LDI     DATA,$0D
        RCALL   UART_PUTCHAR
        LDI     DATA,$0A
        RJMP    UART_PUTCHAR
;
;--------------------------------------
;
UART_DUMP512:
        PUSH    FLAGS1
        CBR     FLAGS1,0B00001101
        SBR     FLAGS1,0B00000010
        PUSHZ
        LDIZ    MSG_DUMPHEAD*2
        RCALL   PRINTSTRZ
        POPZ
        LDIX    0
UDUMP3: RCALL   UART_CRLF
        MOV     DATA,XH
        RCALL   HEXBYTE
        MOV     DATA,XL
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   UART_PUTCHAR
        RCALL   UART_PUTCHAR
        PUSHZ
        LDI     WL,16
UDUMP1: LD      DATA,Z+
        RCALL   HEXBYTE_FOR_DUMP
        DEC     WL
        BRNE    UDUMP1
        LDI     DATA,$20
        RCALL   UART_PUTCHAR
        POPZ
        LDI     WL,16
UDUMP2: LD      DATA,Z+
        RCALL   PUTCHAR_FOR_DUMP
        DEC     WL
        BRNE    UDUMP2
        ADIW    XL,16
        CPI     XH,$02
        BRNE    UDUMP3
        POP     FLAGS1
        RET
MSG_DUMPHEAD:
        .DB     $0D,$0A,$3B,"     .0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .A .B .C .D .E .F",0
;
;--------------------------------------
;
UART_CHK_CTS:
        SBIC    PINB,6
        RET
        PUSH    TEMP
        LDS     TEMP,UART_TX_LN
        TST     TEMP
        BREQ    UART_CHKCTS9
        INPORT  TEMP,UCSR1B
        SBR     TEMP,(1<<UDRIE) ;Разрешаем прерывания Empty Data Register
        OUTPORT UCSR1B,TEMP
UART_CHKCTS9:
        POP     TEMP
        RET
;
;--------------------------------------
;
