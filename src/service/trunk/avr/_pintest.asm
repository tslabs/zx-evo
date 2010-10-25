;
;--------------------------------------
;
PINTEST:
        CLR     TEMP
        SBI     PORTD,3
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        SBIS    PIND,3
        SBR     TEMP,$01
        CBI     PORTD,3
        SBI     DDRD,3
        NOP
        NOP
        SBIC    PIND,3
        SBR     TEMP,$02
        CBI     DDRD,3
        TST     TEMP
        BREQ    PINTEST_UART_OK
CHAOS00:
        SBI     DDRB,7
CHAOS2: CALL    RANDOM
        SBI     PORTB,7
        SBRC    DATA,0
        CBI     PORTB,7
        LDIX    $6C00
CHAOS1: SUBI    XL,1
        SBCI    XH,0
        BRNE    CHAOS1
        RJMP    CHAOS2

PINTEST_UART_OK:
        CALL    UARTDIRECT_INIT

        RCALL   UART_CRLF
        RCALL   UART_CRLF
        RCALL   UART_CRLF
        LDIZ    MLMSG_TITLE1*2
        CALL    PRINTMLSTR
        CALL    PRINT_SHORT_VERS
        LDIZ    MLMSG_PINTEST*2
        CALL    PRINTMLSTR

        LDI     TEMP,0B01010101
        OUT     PORTA,TEMP
        LDI     TEMP,0B10101010
        OUT     DDRA,TEMP
        LDI     TEMP,0B10000010
        OUT     PORTB,TEMP
        LDI     TEMP,0B00000101
        OUT     DDRB,TEMP
        LDI     TEMP,0B00010101
        OUT     PORTC,TEMP
        LDI     TEMP,0B00001010
        OUT     DDRC,TEMP
        SBI     PORTD,5
        CBI     DDRD,5
        SBI     PORTE,0
        CBI     DDRE,0
        CBI     PORTE,1
        SBI     DDRE,1
        LDI     TEMP,0B00010101
        OUTPORT PORTG,TEMP
        LDI     TEMP,0B00001010
        OUTPORT DDRG,TEMP
        DELAY_US 100

        CLR     DATA
        IN      TEMP,PINA
        CPI     TEMP,0B01010101
        BREQ    PINTEST_OK11
        ORI     DATA,$01
PINTEST_OK11:
        IN      TEMP,PINB
        ANDI    TEMP,0B10000111
        CPI     TEMP,0B10000000
        BREQ    PINTEST_OK12
        ORI     DATA,$02
PINTEST_OK12:
        IN      TEMP,PINC
        ANDI    TEMP,0B00011111
        CPI     TEMP,0B00010101
        BREQ    PINTEST_OK13
        ORI     DATA,$04
PINTEST_OK13:
        SBIS    PIND,5
        ORI     DATA,$08
        IN      TEMP,PINE
        ANDI    TEMP,0B00000011
        CPI     TEMP,0B00000001
        BREQ    PINTEST_OK14
        ORI     DATA,$10
PINTEST_OK14:
        INPORT  TEMP,PING
        ANDI    TEMP,0B00011111
        CPI     TEMP,0B00010101
        BREQ    PINTEST_OK15
        ORI     DATA,$20
PINTEST_OK15:

        LDI     TEMP,0B10101010
        OUT     PORTA,TEMP
        LDI     TEMP,0B01010101
        OUT     DDRA,TEMP
        LDI     TEMP,0B00000101
        OUT     PORTB,TEMP
        LDI     TEMP,0B10000010
        OUT     DDRB,TEMP
        LDI     TEMP,0B00001010
        OUT     PORTC,TEMP
        LDI     TEMP,0B00010101
        OUT     DDRC,TEMP
        CBI     PORTD,5
        SBI     DDRD,5
        CBI     PORTE,0
        SBI     DDRE,0
        SBI     PORTE,1
        CBI     DDRE,1
        LDI     TEMP,0B00001010
        OUTPORT PORTG,TEMP
        LDI     TEMP,0B00010101
        OUTPORT DDRG,TEMP
        DELAY_US 100

        IN      TEMP,PINA
        CPI     TEMP,0B10101010
        BREQ    PINTEST_OK21
        ORI     DATA,$01
PINTEST_OK21:
        IN      TEMP,PINB
        ANDI    TEMP,0B10000111
        CPI     TEMP,0B00000101
        BREQ    PINTEST_OK22
        ORI     DATA,$02
PINTEST_OK22:
        IN      TEMP,PINC
        ANDI    TEMP,0B00011111
        CPI     TEMP,0B00001010
        BREQ    PINTEST_OK23
        ORI     DATA,$04
PINTEST_OK23:
        SBIC    PIND,5
        ORI     DATA,$08
        IN      TEMP,PINE
        ANDI    TEMP,0B00000011
        CPI     TEMP,0B00000010
        BREQ    PINTEST_OK24
        ORI     DATA,$10
PINTEST_OK24:
        INPORT  TEMP,PING
        ANDI    TEMP,0B00011111
        CPI     TEMP,0B00001010
        BREQ    PINTEST_OK25
        ORI     DATA,$20
PINTEST_OK25:
        TST     DATA
        BRNE    PINTEST_ERROR
        LDIZ    MLMSG_PINTEST_OK*2
        CALL    PRINTMLSTR
        RET
;
PINTEST_ERROR:
        PUSH    DATA
        CALL    CLRPINS
        LDIZ    MLMSG_PINTEST_ERROR*2
        CALL    PRINTMLSTR
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR1
        LDIZ    MSG_PINTEST_PA*2
        CALL    PRINTSTRZ
PINTEST_ERR1:
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR2
        LDIZ    MSG_PINTEST_PB*2
        CALL    PRINTSTRZ
PINTEST_ERR2:
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR3
        LDIZ    MSG_PINTEST_PC*2
        CALL    PRINTSTRZ
PINTEST_ERR3:
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR4
        LDIZ    MSG_PINTEST_PD*2
        CALL    PRINTSTRZ
PINTEST_ERR4:
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR5
        LDIZ    MSG_PINTEST_PE*2
        CALL    PRINTSTRZ
PINTEST_ERR5:
        POP     DATA
        LSR     DATA
        PUSH    DATA
        BRCC    PINTEST_ERR6
        LDIZ    MSG_PINTEST_PF*2
        CALL    PRINTSTRZ
PINTEST_ERR6:
        POP     DATA
        LSR     DATA
        BRCC    PINTEST_ERR7
        LDIZ    MSG_PINTEST_PG*2
        CALL    PRINTSTRZ
PINTEST_ERR7:
        LDIZ    MLMSG_HALT*2
        CALL    PRINTMLSTR
        CALL    CLRPINS
PINTEST_HALT:
        RJMP    PINTEST_HALT
;
;--------------------------------------
;
