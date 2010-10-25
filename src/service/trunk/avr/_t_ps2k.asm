;to do: перенумерация меток (работает - не трогай;)

;   0                                   4
;   5                                   1
; ┌─────────────────────────────────────────────┐
; │                                             │
; │ e   1 2 3 4 5 6 7 8 9 0 1 2  p s p  . . .   │07
; │                                             │
; │ ` 1 2 3 4 5 6 7 8 9 0 - = <  i h u  n / * - │
; │ t Q W E R T Y U I O P [ ] \  d e d  7 8 9   │
; │ c A S D F G H J K L ; '   e         4 5 6 + │
; │ s Z X C V B N M , . /     s        1 2 3   │
; │ c w a       s       a w m c  <  >  0   . e │
; │                                             │
; │─────────────────────────────────────────────│
; │ Raw data:                                   │16
; │  00 00 00 00 00 00 00 00 00 00 00 00 00 00  │17
; │                                             │
; │ Трёхкратное нажатие <ESC> - выход из теста  │19
; └─────────────────────────────────────────────┘
;
;
;--------------------------------------
;
.EQU    TPSK_PTR        =0
.EQU    TPSK_TEMP       =1
.EQU    TPSK_COUNT      =2
.EQU    TPSK_FLAGS      =3
.EQU    TPSK_LEDS       =4
;
;--------------------------------------
;
;TESTPS2KEYB_NOEXIT:
;        GETMEM  5
;        STH     TPSK_FLAGS,FF
;
;        LDIZ    WIND_T_PS2K*2
;        CALL    WINDOW
;        RJMP    T_PSK00
;
; - - - - - - - - - - - - - - - - - - -
;
TESTPS2KEYB:
        GETMEM  5
        STH     TPSK_FLAGS,NULL

        LDIZ    WIND_T_PS2K*2
        CALL    WINDOW
        LDIZ    MLMSG_TPS2K_0*2
        RCALL   SCR_PRINTMLSTR
T_PSK00:LDI     TEMP,$80
        STH     TPSK_LEDS,TEMP

        LDIZ    MSG_TPS2K_1*2
        RCALL   SCR_PRINTSTRZ
        LDI     DATA,$C4        ;"─"
        LDI     COUNT,45
        RCALL   SCR_FILL_CHAR

        LDIZ    MEGABUFFER+16
T_PSK01:ST      -Z,NULL
        TST     ZL
        BRNE    T_PSK01
        STS     PS2K_RAW_READY,NULL
        STH     TPSK_PTR,NULL

T_PSK10:
        LDS     DATA,PS2K_RAW_READY
        TST     DATA
        BREQ    T_PSK11
        LDS     DATA,PS2K_RAW_CODE
        RCALL   T_PSK80
T_PSK11:
        LDH     DATA,TPSK_LEDS
        SBRS    DATA,7
        RJMP    T_PSK1L
        ANDI    DATA,$07
        STH     TPSK_LEDS,DATA
        LDI     DATA,$ED
        RCALL   T_PSK80
        RCALL   PS2K_SEND_BYTE
        BREQ    T_PSK1_SETLED_FAIL
        RCALL   PS2K_RECEIVE_BYTE
        BREQ    T_PSK1_SETLED_FAIL
        RCALL   T_PSK80
        CPI     DATA,$FA
        BRNE    T_PSK1_SETLED_FAIL
        LDH     DATA,TPSK_LEDS
        RCALL   T_PSK80
        RCALL   PS2K_SEND_BYTE
        BREQ    T_PSK1_SETLED_FAIL
        RCALL   PS2K_RECEIVE_BYTE
        BREQ    T_PSK1_SETLED_FAIL
        RCALL   T_PSK80
        CPI     DATA,$FA
        BRNE    T_PSK1_SETLED_FAIL
        RCALL   T_PSK7_SHOW_LEDS
T_PSK1_SETLED_FAIL:

T_PSK1L:
        LDS     TEMP,PS2K_KEY_FLAGS
        SBRS    TEMP,PS2K_BIT_READY
        RJMP    T_PSK10
;
        STS     PS2K_KEY_FLAGS,NULL
        LDS     DATA,PS2K_KEY_CODE
        STH     TPSK_TEMP,TEMP

        LDH     COUNT,TPSK_FLAGS
;        TST     COUNT
;        BRMI    T_PSK21
        SBRS    TEMP,PS2K_BIT_RELEASE
        RJMP    T_PSK21
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    T_PSK22
        CPI     DATA,KEY_ESC
        BRNE    T_PSK22
        INC     COUNT
        STH     TPSK_FLAGS,COUNT
        CPI     COUNT,3
        BRCS    T_PSK21

        RCALL   SCR_KBDSETLED
        FREEMEM 5
        RET
;
T_PSK22:STH     TPSK_FLAGS,NULL
T_PSK21:SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    T_PSK12
        SBRC    TEMP,PS2K_BIT_RELEASE
        RJMP    T_PSK26

        LDH     XL,TPSK_LEDS
        LDI     XH,$01
        CPI     DATA,KEY_SCROLLLOCK
        BRNE    T_PSK23
        EOR     XL,XH
        ORI     XL,$80
T_PSK23:LSL     XH
        CPI     DATA,KEY_NUMLOCK
        BRNE    T_PSK24
        EOR     XL,XH
        ORI     XL,$80
T_PSK24:LSL     XH
        CPI     DATA,KEY_CAPSLOCK
        BRNE    T_PSK25
        EOR     XL,XH
        ORI     XL,$80
T_PSK25:STH     TPSK_LEDS,XL
T_PSK26:

        CPI     DATA,$83        ;F7
        BRNE    T_PSK12
        LDI     XL,(9<<3)|0
        RJMP    T_PSK14
T_PSK12:CPI     DATA,$84        ;SysReg
        BRNE    T_PSK13
        LDI     XL,(15<<3)|0
        RJMP    T_PSK14
T_PSK13:
        CPI     DATA,$80
        BRCC    T_PSK10_A
        LSL     DATA
        SBRC    TEMP,PS2K_BIT_EXTKEY
        ORI     DATA,$01
        LDIZ    TPSK_TAB*2
        ADD     ZL,DATA
        ADC     ZH,NULL
        LPM     XL,Z
        TST     XL
        BREQ    T_PSK10_A
T_PSK14:MOV     XH,XL
        ANDI    XH,0B00000111
        TST     XH
        BREQ    T_PSK15
        INC     XH
T_PSK15:LSR     XL
        LSR     XL
        ANDI    XL,0B00111110
        CPI     XL,36
        BRCS    T_PSK16
        INC     XL
T_PSK16:CPI     XL,30
        BRCS    T_PSK17
        INC     XL
T_PSK17:ADDI    XL,3
        ADDI    XH,7
        RCALL   SCR_SET_CURSOR
        LDH     DATA,TPSK_TEMP
        LDI     TEMP,$AE
        SBRC    DATA,PS2K_BIT_RELEASE
        LDI     TEMP,$D1
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
T_PSK10_A:
        RJMP    T_PSK10
;
;
T_PSK80:PUSH    DATA
        STS     PS2K_RAW_READY,NULL

        LDH     ZL,TPSK_PTR
        LDI     ZH,HIGH(MEGABUFFER)
        ST      Z+,DATA
        ANDI    ZL,$0F
        STH     TPSK_PTR,ZL

        ANDI    FLAGS1,0B11111100       ;!!!
        LDI     XL,5
        LDI     XH,17
        RCALL   SCR_SET_CURSOR
        LDH     ZL,TPSK_PTR
        INC     ZL
        LDI     COUNT,13

T_PSK81:STH     TPSK_COUNT,COUNT
        LDI     ZH,HIGH(MEGABUFFER)
        INC     ZL
        ANDI    ZL,$0F
        STH     TPSK_TEMP,ZL
        LD      DATA,Z
        PUSH    DATA
        LDI     TEMP,$0E
        CPI     DATA,$E0
        BREQ    T_PSK82
        CPI     DATA,$E1
        BREQ    T_PSK82
        LDI     TEMP,$0D
        CPI     DATA,$F0
        BREQ    T_PSK82
        LDI     TEMP,$0B
        CPI     DATA,$ED
        BREQ    T_PSK82
        LDI     TEMP,$0A
        CPI     DATA,$85
        BRCC    T_PSK82
        LDI     TEMP,$0F
T_PSK82:RCALL   SCR_SET_ATTR
        LDI     DATA,$20
        RCALL   SCR_PUTCHAR
        POP     DATA
        LDH     COUNT,TPSK_COUNT
        TST     COUNT
        BREQ    T_PSK83
        RCALL   HEXBYTE
        LDH     ZL,TPSK_TEMP
        LDH     COUNT,TPSK_COUNT
        DEC     COUNT
        RJMP    T_PSK81
T_PSK83:
        ORI     FLAGS1,0B00000010       ;!!!
        CALL    HEXBYTE
        LDI     DATA,$20
        CALL    PUTCHAR

        POP     DATA
        RET
;
;
T_PSK7_SHOW_LEDS:
        LDI     XL,41
        LDI     XH,7
        RCALL   SCR_SET_CURSOR
        LDH     DATA,TPSK_LEDS
        LDI     TEMP,$DC
        SBRS    DATA,1
        LDI     TEMP,$D0
        LDI     COUNT,2
        RCALL   SCR_FILL_ATTR
        LDH     DATA,TPSK_LEDS
        LDI     TEMP,$DC
        SBRS    DATA,2
        LDI     TEMP,$D0
        LDI     COUNT,2
        RCALL   SCR_FILL_ATTR
        LDH     DATA,TPSK_LEDS
        LDI     TEMP,$DC
        SBRS    DATA,0
        LDI     TEMP,$D0
        LDI     COUNT,2
        RJMP    SCR_FILL_ATTR
;
;--------------------------------------
;
WIND_T_PS2K:
        .DB     3,5,47,16,$DF,$01
;
;--------------------------------------
;
TPSK_TAB:
        .DB     0        , 0            ;00
        .DB     (11<<3)|0, 0            ;01
        .DB     0        , 0            ;02
        .DB     ( 7<<3)|0, 0            ;03
        .DB     ( 5<<3)|0, 0            ;04
        .DB     ( 3<<3)|0, 0            ;05
        .DB     ( 4<<3)|0, 0            ;06
        .DB     (14<<3)|0, 0            ;07
        .DB     0        , 0            ;08
        .DB     (12<<3)|0, 0            ;09
        .DB     (10<<3)|0, 0            ;0A
        .DB     ( 8<<3)|0, 0            ;0B
        .DB     ( 6<<3)|0, 0            ;0C
        .DB     ( 1<<3)|2, 0            ;0D
        .DB     ( 1<<3)|1, 0            ;0E
        .DB     0        , 0            ;0F
        .DB     0        , 0            ;10
        .DB     ( 3<<3)|5, (11<<3)|5    ;11
        .DB     ( 1<<3)|4, 0            ;12
        .DB     0        , 0            ;13
        .DB     ( 1<<3)|5, (14<<3)|5    ;14
        .DB     ( 2<<3)|2, 0            ;15
        .DB     ( 2<<3)|1, 0            ;16
        .DB     0        , 0            ;17
        .DB     0        , 0            ;18
        .DB     0        , 0            ;19
        .DB     ( 2<<3)|4, 0            ;1A
        .DB     ( 3<<3)|3, 0            ;1B
        .DB     ( 2<<3)|3, 0            ;1C
        .DB     ( 3<<3)|2, 0            ;1D
        .DB     ( 3<<3)|1, 0            ;1E
        .DB     0        , ( 2<<3)|5    ;1F
        .DB     0        , 0            ;20
        .DB     ( 4<<3)|4, 0            ;21
        .DB     ( 3<<3)|4, 0            ;22
        .DB     ( 4<<3)|3, 0            ;23
        .DB     ( 4<<3)|2, 0            ;24
        .DB     ( 5<<3)|1, 0            ;25
        .DB     ( 4<<3)|1, 0            ;26
        .DB     0        , (12<<3)|5    ;27
        .DB     0        , 0            ;28
        .DB     ( 7<<3)|5, 0            ;29
        .DB     ( 5<<3)|4, 0            ;2A
        .DB     ( 5<<3)|3, 0            ;2B
        .DB     ( 6<<3)|2, 0            ;2C
        .DB     ( 5<<3)|2, 0            ;2D
        .DB     ( 6<<3)|1, 0            ;2E
        .DB     0        , (13<<3)|5    ;2F
        .DB     0        , 0            ;30
        .DB     ( 7<<3)|4, 0            ;31
        .DB     ( 6<<3)|4, 0            ;32
        .DB     ( 7<<3)|3, 0            ;33
        .DB     ( 6<<3)|3, 0            ;34
        .DB     ( 7<<3)|2, 0            ;35
        .DB     ( 7<<3)|1, 0            ;36
        .DB     0        , 0            ;37
        .DB     0        , 0            ;38
        .DB     0        , 0            ;39
        .DB     ( 8<<3)|4, 0            ;3A
        .DB     ( 8<<3)|3, 0            ;3B
        .DB     ( 8<<3)|2, 0            ;3C
        .DB     ( 8<<3)|1, 0            ;3D
        .DB     ( 9<<3)|1, 0            ;3E
        .DB     0        , 0            ;3F
        .DB     0        , 0            ;40
        .DB     ( 9<<3)|4, 0            ;41
        .DB     ( 9<<3)|3, 0            ;42
        .DB     ( 9<<3)|2, 0            ;43
        .DB     (10<<3)|2, 0            ;44
        .DB     (11<<3)|1, 0            ;45
        .DB     (10<<3)|1, 0            ;46
        .DB     0        , 0            ;47
        .DB     0        , 0            ;48
        .DB     (10<<3)|4, 0            ;49
        .DB     (11<<3)|4, (19<<3)|1    ;4A
        .DB     (10<<3)|3, 0            ;4B
        .DB     (11<<3)|3, 0            ;4C
        .DB     (11<<3)|2, 0            ;4D
        .DB     (12<<3)|1, 0            ;4E
        .DB     0        , 0            ;4F
        .DB     0        , 0            ;50
        .DB     0        , 0            ;51
        .DB     (12<<3)|3, 0            ;52
        .DB     0        , 0            ;53
        .DB     (12<<3)|2, 0            ;54
        .DB     (13<<3)|1, 0            ;55
        .DB     0        , 0            ;56
        .DB     0        , 0            ;57
        .DB     ( 1<<3)|3, 0            ;58
        .DB     (14<<3)|4, 0            ;59
        .DB     (14<<3)|3, (21<<3)|5    ;5A
        .DB     (13<<3)|2, 0            ;5B
        .DB     0        , 0            ;5C
        .DB     (14<<3)|2, 0            ;5D
        .DB     0        , 0            ;5E
        .DB     0        , 0            ;5F
        .DB     0        , 0            ;60
        .DB     0        , 0            ;61
        .DB     0        , 0            ;62
        .DB     0        , 0            ;63
        .DB     0        , 0            ;64
        .DB     0        , 0            ;65
        .DB     (14<<3)|1, 0            ;66
        .DB     0        , 0            ;67
        .DB     0        , 0            ;68
        .DB     (18<<3)|4, (16<<3)|2    ;69
        .DB     0        , 0            ;6A
        .DB     (18<<3)|3, (15<<3)|5    ;6B
        .DB     (18<<3)|2, (16<<3)|1    ;6C
        .DB     0        , 0            ;6D
        .DB     0        , 0            ;6E
        .DB     0        , 0            ;6F
        .DB     (18<<3)|5, (15<<3)|1    ;70
        .DB     (20<<3)|5, (15<<3)|2    ;71
        .DB     (19<<3)|4, (16<<3)|5    ;72
        .DB     (19<<3)|3, 0            ;73
        .DB     (20<<3)|3, (17<<3)|5    ;74
        .DB     (19<<3)|2, (16<<3)|4    ;75
        .DB     ( 1<<3)|0, 0            ;76
        .DB     (18<<3)|1, 0            ;77
        .DB     (13<<3)|0, 0            ;78
        .DB     (21<<3)|3, 0            ;79
        .DB     (20<<3)|4, (17<<3)|2    ;7A
        .DB     (21<<3)|1, 0            ;7B
        .DB     (20<<3)|1, (15<<3)|0    ;7C
        .DB     (20<<3)|2, (17<<3)|1    ;7D
        .DB     (16<<3)|0, (17<<3)|0    ;7E
        .DB     0        , 0            ;7F
;
;--------------------------------------
;
