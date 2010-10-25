;
;--------------------------------------
;
.DSEG
MSCOUNTER:      .BYTE   2
.CSEG
;
;--------------------------------------
;
TIMERS_INIT:
;TIMER 3
        LDI     TEMP,(0<<WGM31)|(0<<WGM30)
        OUTPORT TCCR3A,TEMP
        LDI     TEMP,(0<<WGM33)|(1<<WGM32)|(0<<CS32)|(0<<CS31)|(1<<CS30)
        OUTPORT TCCR3B,TEMP

        LDI     TEMP,HIGH(11058)
        OUTPORT OCR3AH,TEMP
        LDI     TEMP,LOW(11058)
        OUTPORT OCR3AL,TEMP
;
        INPORT  TEMP,ETIMSK
        ORI     TEMP,(1<<OCIE3A)
        OUTPORT ETIMSK,TEMP

        RET
;
;--------------------------------------
;
TIM3_COMPA:
        PUSH    TEMP
        IN      TEMP,SREG
        PUSH    TEMP

        LDS     TEMP,MSCOUNTER+0
        INC     TEMP
        STS     MSCOUNTER+0,TEMP
        BRNE    TIM3_CMPA_1
        LDS     TEMP,MSCOUNTER+1
        INC     TEMP
        STS     MSCOUNTER+1,TEMP
TIM3_CMPA_1:

        POP     TEMP
        OUT     SREG,TEMP
        POP     TEMP
        RETI
;
;--------------------------------------
;in:    W == таймайт, мс (1..16383)
;       Z == указатель в RAM (используются два байта)
SET_TIMEOUT_MS:
        PUSHX
        CLI
        LDS     XL,MSCOUNTER+0
        LDS     XH,MSCOUNTER+1
        SEI
        ADD     XL,WL
        ADC     XH,WH
        ORI     XH,$80
        ST      Z,XL
        STD     Z+1,XH
        POPX
        RET
;
;--------------------------------------
;in:    Z == указатель в RAM (используются два байта)
;out:   sreg.C - SET == время вышло
CHECK_TIMEOUT_MS:
        PUSH    WH
        LDD     WH,Z+1
        SBRS    WH,7
        RJMP    CHKTMS_9
        PUSH    WL
        LD      WL,Z
        PUSHX
        CLI
        LDS     XL,MSCOUNTER+0
        LDS     XH,MSCOUNTER+1
        SEI
        SUB     XL,WL
        SBC     XH,WH
        SBRC    XH,6
        RJMP    CHKTMS_8
        STD     Z+1,NULL
        POPX
        POP     WL
CHKTMS_9:
        POP     WH
        SEC
        RET
CHKTMS_8:
        POPX
        POP     WL
        POP     WH
        CLC
        RET
;
;--------------------------------------
;
