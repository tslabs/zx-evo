;
;
;
;
;
;     ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
;     ³      Š« ¢¨ âãà  ZX        „¦®©áâ¨ª ³
;     ³ ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿            ³
;     ³ ³ 1 2 3 4 5 6 7 8 9 0 ³  ÚÄÄÄÄÄÄÄ¿ ³
;     ³ ³ Q W E R T Y U I O P ³  ³      ³ ³
;     ³ ³ A S D F G H J K L e ³  ³ < F > ³ ³
;     ³ ³ c Z X C V B N M s s ³  ³      ³ ³
;     ³ ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ  ÀÄÄÄÄÄÄÄÙ ³
;     ³    ÚÄÄÄÄÄÄÄÄÄÄÄ¿   ÚÄÄÄÄÄÄÄÄÄÄ¿    ³
;     ³    ³ SoftReset ³   ³ TurboKey ³    ³
;     ³    ÀÄÄÄÄÄÄÄÄÄÄÄÙ   ÀÄÄÄÄÄÄÄÄÄÄÙ    ³
;     ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ
;
;
;
;    03 11 19 27 35 36 28 20 12 04
;    02 10 18 26 34 37 29 21 13 05      43
;    01 09 17 25 33 38 30 22 14 06   41 44 40
;    00 08 16 24 32 39 31 23 15 07      42
;
;               46               45
;
;--------------------------------------
;
TESTZXKEYB:

        LDIZ    WIND_T_ZXKBD_1*2
        RCALL   WINDOW
        LDIZ    WIND_T_ZXKBD_2*2
        RCALL   WINDOW
        LDIZ    WIND_T_ZXKBD_3*2
        RCALL   WINDOW
        LDIZ    WIND_T_ZXKBD_4*2
        RCALL   WINDOW
        LDIZ    WIND_T_ZXKBD_5*2
        RCALL   WINDOW

        LDIZ    MLMSG_TZXK_1*2
        RCALL   SCR_PRINTMLSTR
        LDIZ    MSG_TZXK_2*2
        RCALL   SCR_PRINTSTRZ

        LDIZ    MEGABUFFER+48
T_ZXK03:ST      -Z,NULL
        TST     ZL
        BRNE    T_ZXK03

        OUT     DDRA,NULL
        OUT     PORTA,FF
        OUT     DDRC,NULL
        OUT     PORTC,NULL
;
T_ZXK10:LDIZ    MEGABUFFER
        SBI     DDRC,0
        RCALL   T_ZXK80
        CBI     DDRC,0
        SBI     DDRC,1
        RCALL   T_ZXK80
        CBI     DDRC,1
        SBI     DDRC,2
        RCALL   T_ZXK80
        CBI     DDRC,2
        SBI     DDRC,3
        RCALL   T_ZXK80
        CBI     DDRC,3
        SBI     DDRC,4
        RCALL   T_ZXK80
        CBI     DDRC,4

        INPORT  DATA,PING
        LDI     COUNT,5
        RCALL   T_ZXK82

        IN      DATA,PINC
        ROL     DATA
        ROL     DATA
        ROL     DATA
        LDI     COUNT,2
        RCALL   T_ZXK82
;
        LDIZ    MEGABUFFER
        LDI     XL,11
        LDI     XH,9
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+3
        RCALL   T_ZXK70
        LDD     DATA,Z+11
        RCALL   T_ZXK70
        LDD     DATA,Z+19
        RCALL   T_ZXK70
        LDD     DATA,Z+27
        RCALL   T_ZXK70
        LDD     DATA,Z+35
        RCALL   T_ZXK70
        LDD     DATA,Z+36
        RCALL   T_ZXK70
        LDD     DATA,Z+28
        RCALL   T_ZXK70
        LDD     DATA,Z+20
        RCALL   T_ZXK70
        LDD     DATA,Z+12
        RCALL   T_ZXK70
        LDD     DATA,Z+4
        RCALL   T_ZXK70

        LDI     XL,11
        LDI     XH,10
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+2
        RCALL   T_ZXK70
        LDD     DATA,Z+10
        RCALL   T_ZXK70
        LDD     DATA,Z+18
        RCALL   T_ZXK70
        LDD     DATA,Z+26
        RCALL   T_ZXK70
        LDD     DATA,Z+34
        RCALL   T_ZXK70
        LDD     DATA,Z+37
        RCALL   T_ZXK70
        LDD     DATA,Z+29
        RCALL   T_ZXK70
        LDD     DATA,Z+21
        RCALL   T_ZXK70
        LDD     DATA,Z+13
        RCALL   T_ZXK70
        LDD     DATA,Z+5
        RCALL   T_ZXK70

        LDI     XL,11
        LDI     XH,11
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+1
        RCALL   T_ZXK70
        LDD     DATA,Z+9
        RCALL   T_ZXK70
        LDD     DATA,Z+17
        RCALL   T_ZXK70
        LDD     DATA,Z+25
        RCALL   T_ZXK70
        LDD     DATA,Z+33
        RCALL   T_ZXK70
        LDD     DATA,Z+38
        RCALL   T_ZXK70
        LDD     DATA,Z+30
        RCALL   T_ZXK70
        LDD     DATA,Z+22
        RCALL   T_ZXK70
        LDD     DATA,Z+14
        RCALL   T_ZXK70
        LDD     DATA,Z+6
        RCALL   T_ZXK70

        LDI     XL,11
        LDI     XH,12
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+0
        RCALL   T_ZXK70
        LDD     DATA,Z+8
        RCALL   T_ZXK70
        LDD     DATA,Z+16
        RCALL   T_ZXK70
        LDD     DATA,Z+24
        RCALL   T_ZXK70
        LDD     DATA,Z+32
        RCALL   T_ZXK70
        LDD     DATA,Z+39
        RCALL   T_ZXK70
        LDD     DATA,Z+31
        RCALL   T_ZXK70
        LDD     DATA,Z+23
        RCALL   T_ZXK70
        LDD     DATA,Z+15
        RCALL   T_ZXK70
        LDD     DATA,Z+7
        RCALL   T_ZXK70

        LDI     XL,38
        LDI     XH,10
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+43
        RCALL   T_ZXK70
        LDI     XL,36
        LDI     XH,11
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+41
        RCALL   T_ZXK70
        LDD     DATA,Z+44
        RCALL   T_ZXK70
        LDD     DATA,Z+40
        RCALL   T_ZXK70
        LDI     XL,38
        LDI     XH,12
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+42
        RCALL   T_ZXK70

        LDI     XL,14
        LDI     XH,15
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+46
        LDI     COUNT,9
        RCALL   T_ZXK72

        LDI     XL,30
        LDI     XH,15
        RCALL   SCR_SET_CURSOR
        LDD     DATA,Z+45
        LDI     COUNT,8
        RCALL   T_ZXK72

        CALL    INKEY
        BREQ    T_ZXK11
        CPI     DATA,KEY_ESC
        BREQ    T_ZXK99
T_ZXK11:RJMP    T_ZXK10

T_ZXK99:OUT     DDRC,NULL
        OUT     PORTC,NULL
        RET
;
;
T_ZXK70:LDI     COUNT,1
T_ZXK72:LDI     TEMP,$DF
        ANDI    DATA,$03
        BREQ    T_ZXK71
        LDI     TEMP,$AE
        CPI     DATA,$03
        BREQ    T_ZXK71
        LDI     TEMP,$D1
T_ZXK71:RCALL   SCR_FILL_ATTR
        LDI     TEMP,$DF
        LDI     COUNT,1
        RJMP    SCR_FILL_ATTR
;
;
T_ZXK80:CLR     DATA
T_ZXK83:DEC     DATA
        BRNE    T_ZXK83
        IN      DATA,PINA
        LDI     COUNT,8
T_ZXK82:LD      TEMP,Z
        ANDI    TEMP,$FE
        ROR     DATA
        BRCS    T_ZXK81
        ORI     TEMP,$03
T_ZXK81:ST      Z+,TEMP
        DEC     COUNT
        BRNE    T_ZXK82
        CLZ
T_ZXK89:RET
;
;--------------------------------------
;
WIND_T_ZXKBD_1:
        .DB      7, 6,38,12,$DF,1
WIND_T_ZXKBD_2:
        .DB      9, 8,23, 6,$DF,0
WIND_T_ZXKBD_3:
        .DB     34, 9, 9, 5,$DF,0
WIND_T_ZXKBD_4:
        .DB     12,14,13, 3,$DF,0
WIND_T_ZXKBD_5:
        .DB     28,14,12, 3,$DF,0
;
;--------------------------------------
;
