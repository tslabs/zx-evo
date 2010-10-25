;
;--------------------------------------
;
PRINT_SHORT_VERS:
        LDIZ    $DFFC
        OUT     RAMPZ,ONE
        ELPM    XL,Z+
        ELPM    XH,Z
        MOV     DATA,XL
        ANDI    DATA,$1F
        BREQ    PRVERS9
        MOV     TEMP,XH
        LSL     XL
        ROL     TEMP
        LSL     XL
        ROL     TEMP
        LSL     XL
        ROL     TEMP
        ANDI    TEMP,$0F
        BREQ    PRVERS9
        CPI     TEMP,13
        BRCC    PRVERS9
        MOV     COUNT,XH
        LSR     COUNT
        ANDI    COUNT,$3F
        CPI     COUNT,9
        BRCS    PRVERS9
        PUSH    DATA
        LDI     DATA,$28 ;"("
        RCALL   PUTCHAR
        MOV     DATA,COUNT
        RCALL   DECBYTE
        MOV     DATA,TEMP
        RCALL   DECBYTE
        POP     DATA
        RCALL   DECBYTE
        LDI     DATA,$29 ;")"
        RCALL   PUTCHAR
PRVERS9:RET
;
;--------------------------------------
;in:    Z == указатель на структуру строк (в младших 64K)
PRINTMLSTR:
        ADD     ZL,LANG
        ADC     ZH,NULL
        LPM     WL,Z+
        LPM     WH,Z+
        MOVW    ZL,WL
;
; - - - - - - - - - - - - - - - - - - -
;in:    Z == указатель на строку (в младших 64K)
PRINTSTRZ:
PRSTRZ1:LPM     DATA,Z+
        TST     DATA
        BREQ    PRSTRZ2
        RCALL   PUTCHAR
        RJMP    PRSTRZ1
PRSTRZ2:RET
;
;--------------------------------------
;out byte in dec
;in:    DATA == byte (00..99)
DECBYTE:SUBI    DATA,208
        SBRS    DATA,7
        SUBI    DATA,48
        SUBI    DATA,232
        SBRS    DATA,6
        SUBI    DATA,24
        SUBI    DATA,244
        SBRS    DATA,5
        SUBI    DATA,12
        SUBI    DATA,250
        SBRS    DATA,4
        SUBI    DATA,6
;
; - - - - - - - - - - - - - - - - - - -
;out byte in hex
;in:    DATA == byte
HEXBYTE:PUSH    DATA
        SWAP    DATA
        RCALL   HEXHALF
        POP     DATA
HEXHALF:ANDI    DATA,$0F
        CPI     DATA,$0A
        BRCS    HEXBYT1
        ADDI    DATA,$07
HEXBYT1:ADDI    DATA,$30
;
; - - - - - - - - - - - - - - - - - - -
;
PUTCHAR:SBRC    FLAGS1,0
        CALL    UARTDIRECT_PUTCHAR
        SBRC    FLAGS1,1
        CALL    UART_PUTCHAR
        SBRC    FLAGS1,2
        CALL    SCR_PUTCHAR
        RET
;
;--------------------------------------
;out word in dec (right justify)
;in:    X == word
DECWORD:LDIZ    DECWTAB*2
        LDI     COUNT,4
        CLR     DATA
DECW5:  LPM     WL,Z+
        LPM     WH,Z+
DECW2:  SUB     XL,WL
        SBC     XH,WH
        BRCS    DECW1
        INC     DATA
        RJMP    DECW2
DECW1:  ADD     XL,WL
        ADC     XH,WH
        TST     DATA
        BRNE    DECW3
        LDI     DATA,$20
        RCALL   DECWPC
        CLR     DATA
        RJMP    DECW4
DECW3:  ORI     DATA,$30
        RCALL   DECWPC
        LDI     DATA,$30
DECW4:  DEC     COUNT
        BRNE    DECW5
        MOV     DATA,XL
        ORI     DATA,$30
        RJMP    PUTCHAR
DECWPC: PUSHZ
        PUSHX
        PUSH    COUNT
        RCALL   PUTCHAR
        POP     COUNT
        POPX
        POPZ
        RET
DECWTAB:.DW     10000,1000,100,10
;
;--------------------------------------
;in:    DATA
HEXBYTE_FOR_DUMP:
        PUSH    DATA
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
        POP     DATA
        RET
;
;--------------------------------------
;
