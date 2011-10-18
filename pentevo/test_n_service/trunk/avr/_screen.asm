.EQU    WIN_SHADOW_ATTR=$01
;
;--------------------------------------
;
SCR_FADE:
        LDI     XL,0
        LDI     XH,1
        RCALL   SCR_SET_CURSOR
        LDI     TEMP,$77
        LDIW    53*23
        RJMP    SCR_FILLLONG_ATTR
;
;--------------------------------------
;
SCR_BACKGND:
        LDI     XL,0
        LDI     XH,0
        RCALL   SCR_SET_CURSOR
        LDI     DATA,$20        ;" "
        LDI     TEMP,$F0
        LDI     COUNT,53
        RCALL   SCR_FILL_CHAR_ATTR
        LDI     DATA,$B0        ;"░"
        LDI     TEMP,$77
        LDIW    53*23
        RCALL   SCR_FILLLONG_CHAR_ATTR
        LDI     DATA,$20        ;" "
        LDI     TEMP,$F0
        LDI     COUNT,53
        RCALL   SCR_FILL_CHAR_ATTR
        CBR     FLAGS1,0B00000011
        SBR     FLAGS1,0B00000100
        LDI     XL,0
        LDI     XH,0
        RCALL   SCR_SET_CURSOR
        LDIZ    MSG_TITLE1*2
        RCALL   SCR_PRINTSTRZ
        CALL    PRINT_SHORT_VERS
        LDIZ    MSG_TITLE2*2
        RJMP    SCR_PRINTSTRZ
;
;--------------------------------------
;                        ┌──┬────────────────────── коорд.лев.верхн угола окна
;                        │  │   ┌────────────────── ширина (без учёта тени)
;                        │  │   │   ┌────────────── высота (без учёта тени)
;                        │  │   │   │    ┌───────── атрибут окна
;WINDOW_DESCRIPTOR:      │  │   │   │    │    ┌──── флаги: .0 - "с тенью/без тени"
;               .DB     15, 3, 25, 13, $1F, $01
;
;in:    Z == указатель на описатель окна (в младших 64K)
;const: WIN_SHADOW_ATTR == атрибут тени
.EQU    WIN_X   =0
.EQU    WIN_Y   =1
.EQU    WIN_W   =2
.EQU    WIN_W2  =3
.EQU    WIN_H   =4
.EQU    WIN_H2  =5
.EQU    WIN_ATTR=6
.EQU    WIN_FLGS=7
;
WINDOW: GETMEM  8

        LPM     XL,Z+
        STH     WIN_X,XL
        LPM     XH,Z+
        STH     WIN_Y,XH
        LPM     TEMP,Z+
        STH     WIN_W,TEMP
        SUBI    TEMP,2
        STH     WIN_W2,TEMP
        LPM     TEMP,Z+
        STH     WIN_H,TEMP
        SUBI    TEMP,2
        STH     WIN_H2,TEMP
        LPM     TEMP,Z+
        STH     WIN_ATTR,TEMP
        LPM     TEMP,Z+
        STH     WIN_FLGS,TEMP

        RCALL   SCR_SET_CURSOR
        LDH     TEMP,WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$DA ;"┌"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$C4 ;"─"
        LDH     COUNT,WIN_W2
        RCALL   SCR_FILL_CHAR
        LDI     DATA,$BF ;"┐"
        RCALL   SCR_PUTCHAR
WIND_1:
        LDH     XL,WIN_X
        LDH     XH,WIN_Y
        INC     XH
        STH     WIN_Y,XH
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$B3 ;"│"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$20 ;" "
        LDH     COUNT,WIN_W2
        RCALL   SCR_FILL_CHAR
        LDI     DATA,$B3 ;"│"
        RCALL   SCR_PUTCHAR
        LDH     COUNT,WIN_FLGS
        SBRS    COUNT,0
        RJMP    WIND_2
        LDI     TEMP,WIN_SHADOW_ATTR
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
WIND_2: LDH     COUNT,WIN_H2
        DEC     COUNT
        STH     WIN_H2,COUNT
        BRNE    WIND_1

        LDH     XL,WIN_X
        LDH     XH,WIN_Y
        INC     XH
        STH     WIN_Y,XH
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$C0 ;"└"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$C4 ;"─"
        LDH     COUNT,WIN_W2
        RCALL   SCR_FILL_CHAR
        LDI     DATA,$D9 ;"┘"
        RCALL   SCR_PUTCHAR
        LDH     COUNT,WIN_FLGS
        SBRS    COUNT,0
        RJMP    WIND_3
        LDI     TEMP,WIN_SHADOW_ATTR
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
WIND_3:
        LDH     COUNT,WIN_FLGS
        SBRS    COUNT,0
        RJMP    WIND_4
        LDH     XL,WIN_X
        INC     XL
        LDH     XH,WIN_Y
        INC     XH
        RCALL   SCR_SET_CURSOR
        LDI     TEMP,WIN_SHADOW_ATTR
        LDH     COUNT,WIN_W
        RCALL   SCR_FILL_ATTR
WIND_4:
        LDH     TEMP,WIN_ATTR
        RCALL   SCR_SET_ATTR

        FREEMEM 8
        RET
;
;--------------------------------------
;                        ┌──┬────────────────────── коорд.лев.верхн угола окна
;                        │  │   ┌────────────────── длина_строки + 2 =
;                        │  │   │                   = ширина без учёта рамки и тени
;                        │  │   │    ┌───────────── количество пунктов меню
;                        │  │   │    │    ┌──────── атрибут для окна
;MENU_DESCRIPTOR:        │  │   │    │    │    ┌─── атрибут для курсора
;               .DB     15, 3, 18+2, 2, $1F, $F0
;               .DW     BKGND_TASK ──────────────── ссылка на фоновую задачу
;               .DW     PERIOD ──────────────────── период вызова фоновой задачи, мс (1..16383)
;
;               .DW     HANDLER1 ────────────────── указатель на обработчик для 1-го пункта меню
;               .DW     HANDLER2 ────────────────── указатель на обработчик для 2-го пункта меню
;
;               .DB     "─ Заголовок окна ─"        \
;               .DB     "Первый пункт меню "         > язык 0
;               .DB     "Второй пункт меню "        /
;
;               .DB     " Header of window "        \
;               .DB     "It's first item   "         > язык 1
;               .DB     "It's second item  "        /
;                       ;123456789012345678 (длина_строки=18)
;
; ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
; ▒▒▒▒┌── Заголовок окна ──┐▒▒▒▒▒   ▒▒▒▒┌─ Header of window ─┐▒▒▒▒▒
; ▒▒▒▒│ Первый пункт меню  │░▒▒▒▒   ▒▒▒▒│ It's first item    │░▒▒▒▒
; ▒▒▒▒│ Второй пункт меню  │░▒▒▒▒   ▒▒▒▒│ It's second item   │░▒▒▒▒
; ▒▒▒▒└────────────────────┘░▒▒▒▒   ▒▒▒▒└────────────────────┘░▒▒▒▒
; ▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒   ▒▒▒▒▒░░░░░░░░░░░░░░░░░░░░░░▒▒▒▒
; ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒   ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒
;--------------------------------------
;Меню (выбор функции - выполнение - возврат в меню)
;in:    Z == указатель на описатель меню (в младших 64K)
;const: WIN_SHADOW_ATTR == атрибут тени
.EQU    MENU_TIMEOUT_0  =0
.EQU    MENU_TIMEOUT_1  =1
.EQU    MENU_DESC_L     =2
.EQU    MENU_DESC_H     =3
.EQU    MENU_SELECT     =4
.EQU    MENU_X          =5
.EQU    MENU_Y          =6
.EQU    MENU_Y_TEMP     =7
.EQU    MENU_WIDTH2     =8
.EQU    MENU_HEIGHT2    =9
.EQU    MENU_H2_TEMP    =10
.EQU    MENU_WIN_ATTR   =11
.EQU    MENU_CUR_ATTR   =12
.EQU    MENU_BTASK_L    =13
.EQU    MENU_BTASK_H    =14
.EQU    MENU_BPERIOD_L  =15
.EQU    MENU_BPERIOD_H  =16
;
MENU:   GETMEM  17
        STH     MENU_DESC_L,ZL
        STH     MENU_DESC_H,ZH
        STH     MENU_SELECT,NULL
MENU_AGAIN:
        STH     MENU_TIMEOUT_1,NULL
        RCALL   SCR_BACKGND
        LDH     ZL,MENU_DESC_L
        LDH     ZH,MENU_DESC_H
        LPM     XL,Z+
        STH     MENU_X,XL
        LPM     XH,Z+
        STH     MENU_Y,XH
        STH     MENU_Y_TEMP,XH
        LPM     TEMP,Z+
        STH     MENU_WIDTH2,TEMP
        LPM     TEMP,Z+
        STH     MENU_HEIGHT2,TEMP
        STH     MENU_H2_TEMP,TEMP
        LPM     TEMP,Z+
        STH     MENU_WIN_ATTR,TEMP
        LPM     TEMP,Z+
        STH     MENU_CUR_ATTR,TEMP

        LPM     TEMP,Z+
        STH     MENU_BTASK_L,TEMP
        LPM     TEMP,Z+
        STH     MENU_BTASK_H,TEMP
        LPM     TEMP,Z+
        STH     MENU_BPERIOD_L,TEMP
        LPM     TEMP,Z+
        STH     MENU_BPERIOD_H,TEMP

        LDH     TEMP,MENU_HEIGHT2
        LSL     TEMP
        ADD     ZL,TEMP
        ADC     ZH,NULL
        LDH     DATA,MENU_WIDTH2
        SUBI    DATA,2
        LDH     TEMP,MENU_HEIGHT2
        INC     TEMP
        MUL     DATA,TEMP
        MOV     TEMP,LANG
        LSR     TEMP
        BREQ    MENU_5
MENU_6: ADD     ZL,R0
        ADC     ZH,R1
        DEC     TEMP
        BRNE    MENU_6
MENU_5:
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,MENU_WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$DA ;"┌"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$C4 ;"─"
        RCALL   SCR_PUTCHAR
        LDH     COUNT,MENU_WIDTH2
        SUBI    COUNT,2
        RCALL   SCR_PRINTSTRN
        LDI     DATA,$C4 ;"─"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$BF ;"┐"
        RCALL   SCR_PUTCHAR
MENU_1:
        LDH     XL,MENU_X
        LDH     XH,MENU_Y_TEMP
        INC     XH
        STH     MENU_Y_TEMP,XH
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,MENU_WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$B3 ;"│"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$20 ;" "
        RCALL   SCR_PUTCHAR
        LDH     COUNT,MENU_WIDTH2
        SUBI    COUNT,2
        RCALL   SCR_PRINTSTRN
        LDI     DATA,$20 ;" "
        RCALL   SCR_PUTCHAR
        LDI     DATA,$B3 ;"│"
        RCALL   SCR_PUTCHAR
        LDI     TEMP,WIN_SHADOW_ATTR
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
MENU_2: LDH     COUNT,MENU_H2_TEMP
        DEC     COUNT
        STH     MENU_H2_TEMP,COUNT
        BRNE    MENU_1

        LDH     XL,MENU_X
        LDH     XH,MENU_Y_TEMP
        INC     XH
        STH     MENU_Y_TEMP,XH
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,MENU_WIN_ATTR
        RCALL   SCR_SET_ATTR
        LDI     DATA,$C0 ;"└"
        RCALL   SCR_PUTCHAR
        LDI     DATA,$C4 ;"─"
        LDH     COUNT,MENU_WIDTH2
        RCALL   SCR_FILL_CHAR
        LDI     DATA,$D9 ;"┘"
        RCALL   SCR_PUTCHAR
        LDI     TEMP,WIN_SHADOW_ATTR
        LDI     COUNT,1
        RCALL   SCR_FILL_ATTR
MENU_3:
        LDH     XL,MENU_X
        INC     XL
        LDH     XH,MENU_Y_TEMP
        INC     XH
        RCALL   SCR_SET_CURSOR
        LDI     TEMP,WIN_SHADOW_ATTR
        LDH     COUNT,MENU_WIDTH2
        ADDI    COUNT,2
        RCALL   SCR_FILL_ATTR

        LDH     ZL,MENU_BTASK_L
        LDH     ZH,MENU_BTASK_H
        LDI     DATA,0
        ICALL

MENU_DRAWCURSOR:
        LDH     XL,MENU_X
        INC     XL
        LDH     XH,MENU_Y
        INC     XH
        LDH     TEMP,MENU_SELECT
        ADD     XH,TEMP
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,MENU_CUR_ATTR
        LDH     COUNT,MENU_WIDTH2
        RCALL   SCR_FILL_ATTR

MENU_WAITKEY:
        CALL    INKEY
        BREQ    MENU_NOKEY
        CPI     DATA,KEY_ENTER
        BREQ    MENU_ENTER
        CPI     DATA,KEY_UP
        BREQ    MENU_UP
        CPI     DATA,KEY_DOWN
        BREQ    MENU_DOWN
        CPI     DATA,KEY_PAGEUP
        BREQ    MENU_TOP
        CPI     DATA,KEY_HOME
        BREQ    MENU_TOP
        CPI     DATA,KEY_PAGEDOWN
        BREQ    MENU_BOTTOM
        CPI     DATA,KEY_END
        BREQ    MENU_BOTTOM
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    MENU_NOKEY
        CPI     DATA,KEY_ESC
        BREQ    MENU_ESCAPE
        CPI     DATA,KEY_CAPSLOCK
        BREQ    MENU_SWITCH_LANG
        CPI     DATA,KEY_SCROLLLOCK
        BREQ    MENU_SWITCH_VGA
        CPI     DATA,KEY_F1
        BREQ    MENU_HELP
MENU_NOKEY:
        MOVW    ZL,YL
        CALL    CHECK_TIMEOUT_MS
        BRCC    MENU_WAITKEY
        LDH     ZL,MENU_BTASK_L
        LDH     ZH,MENU_BTASK_H
        LDI     DATA,1
        ICALL
        MOVW    ZL,YL
        LDH     WL,MENU_BPERIOD_L
        LDH     WH,MENU_BPERIOD_H
        CALL    SET_TIMEOUT_MS
        RJMP    MENU_WAITKEY
;
MENU_TOP:
        RJMP    MENU_TOP0
MENU_BOTTOM:
        RJMP    MENU_BOTTOM0
MENU_SWITCH_LANG:
        RJMP    MENU_SWLNG0
MENU_SWITCH_VGA:
        RJMP    MENU_SWVGA0
MENU_HELP:
        RJMP    MENU_HELP0
;
MENU_ESCAPE:
        FREEMEM 17
        RET
;
MENU_ENTER:
        RCALL   SCR_FADE
        LDH     ZL,MENU_DESC_L
        LDH     ZH,MENU_DESC_H
        ADIW    ZL,10
        LDH     TEMP,MENU_SELECT
        LSL     TEMP
        ADD     ZL,TEMP
        ADC     ZH,NULL
        LPM     XL,Z+
        LPM     XH,Z+
        MOVW    ZL,XL
        ICALL
        RJMP    MENU_AGAIN
;
MENU_UP:
        LDH     TEMP,MENU_SELECT
        TST     TEMP
        BRNE    MENU_UP_1
        RJMP    MENU_WAITKEY
MENU_UP_1:
        RCALL   MENU_CLR_CURSOR
        LDH     TEMP,MENU_SELECT
        DEC     TEMP
        STH     MENU_SELECT,TEMP
        RJMP    MENU_DRAWCURSOR
;
MENU_DOWN:
        LDH     TEMP,MENU_SELECT
        LDH     DATA,MENU_HEIGHT2
        DEC     DATA
        CP      TEMP,DATA
        BRCS    MENU_DOWN_1
        RJMP    MENU_WAITKEY
MENU_DOWN_1:
        RCALL   MENU_CLR_CURSOR
        LDH     TEMP,MENU_SELECT
        INC     TEMP
        STH     MENU_SELECT,TEMP
        RJMP    MENU_DRAWCURSOR
;
MENU_TOP0:
        LDH     TEMP,MENU_SELECT
        RCALL   MENU_CLR_CURSOR
        CLR     TEMP
        STH     MENU_SELECT,TEMP
        RJMP    MENU_DRAWCURSOR
;
MENU_BOTTOM0:
        LDH     TEMP,MENU_SELECT
        RCALL   MENU_CLR_CURSOR
        LDH     TEMP,MENU_HEIGHT2
        DEC     TEMP
        STH     MENU_SELECT,TEMP
        RJMP    MENU_DRAWCURSOR
;
MENU_HELP0:
        RCALL   SCR_FADE
        LDIZ    WIND_MENU_HELP*2
        RCALL   WINDOW
        LDIZ    MLMSG_MENU_HELP*2
        RCALL   SCR_PRINTMLSTR
        CALL    WAITKEY
        RJMP    MENU_AGAIN
WIND_MENU_HELP:
        .DB     3,13,37,9,$CF,$01
;
MENU_SWLNG3:
        FREEMEM 2
MENU_SWLNG0:
        MOV     DATA,LANG
        ADDI    DATA,2
        CPI     DATA,MAX_LANG*2
        BRCS    MENU_SWLNG1
        CLR     DATA
MENU_SWLNG1:
        MOV     LANG,DATA
        LSR     DATA
        LDIW    EE_LANG
        CALL    EEPROM_WRITE
        RCALL   SCR_FADE
        LDIZ    WIND_MENU_SWLNG*2
        RCALL   WINDOW
        LDI     TEMP,$9E
        RCALL   SCR_SET_ATTR
        LDIZ    MLMSG_MENU_SWLNG*2
        RCALL   SCR_PRINTMLSTR
        GETMEM  2
        MOVW    ZL,YL
        LDIW    2000
        CALL    SET_TIMEOUT_MS
MENU_SWLNG2:
        MOVW    ZL,YL
        CALL    CHECK_TIMEOUT_MS
        BRCS    MENU_SWLNG9
        CALL    INKEY
        BREQ    MENU_SWLNG2
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    MENU_SWLNG9
        CPI     DATA,KEY_CAPSLOCK
        BREQ    MENU_SWLNG3
MENU_SWLNG9:
        FREEMEM 2
        RJMP    MENU_AGAIN
WIND_MENU_SWLNG:
        .DB     13,11,27,3,$9F,$01
;
MENU_SWVGA0:
        LDI     TEMP,0B10000000
        EOR     MODE1,TEMP
        MOV     DATA,MODE1
        ANDI    DATA,0B10000000
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG
        MOV     DATA,MODE1
        LDIW    EE_MODE1
        CALL    EEPROM_WRITE
        RCALL   SCR_KBDSETLED
        RJMP    MENU_AGAIN
;
MENU_CLR_CURSOR:
        LDH     XL,MENU_X
        INC     XL
        LDH     XH,MENU_Y
        INC     XH
        ADD     XH,TEMP
        RCALL   SCR_SET_CURSOR
        LDH     TEMP,MENU_WIN_ATTR
        LDH     COUNT,MENU_WIDTH2
        RJMP    SCR_FILL_ATTR
;
SCR_KBDSETLED:
        LDI     DATA,$ED
        RCALL   PS2K_SEND_BYTE
        BREQ    SCR_SETLED_FAIL
        RCALL   PS2K_RECEIVE_BYTE
        BREQ    SCR_SETLED_FAIL
        CPI     DATA,$FA
        BRNE    SCR_SETLED_FAIL
        LDI     DATA,0B00000000
        SBRS    MODE1,7
        LDI     DATA,0B00000001
        RCALL   PS2K_SEND_BYTE
SCR_SETLED_FAIL:
        RET
;
;--------------------------------------
;Установка текущего атрибута
;in:    TEMP - attr
SCR_SET_ATTR:
        MOV     DATA,TEMP
        LDI     TEMP,SCR_ATTR
        RJMP    FPGA_REG
;
;--------------------------------------
;Установка позиции печати на экране
;       XL - x (0..52)
;       XH - y (0..24)
SCR_SET_CURSOR:
        LDI     TEMP,53
        MUL     XH,TEMP
        CLR     XH
        ADD     XL,R0
        ADC     XH,R1
        SBIW    XL,1
        ANDI    XH,$07
        LDI     TEMP,SCR_LOADDR
        MOV     DATA,XL
        RCALL   FPGA_REG
        LDI     TEMP,SCR_HIADDR
        MOV     DATA,XH
        RJMP    FPGA_REG
;
;--------------------------------------
;in:    Z == указатель на структуру строк (в младших 64K)
SCR_PRINTMLSTR:
        ADD     ZL,LANG
        ADC     ZH,NULL
        LPM     WL,Z+
        LPM     WH,Z+
        MOVW    ZL,WL
;
; - - - - - - - - - - - - - - - - - - -
;in:    Z == указатель на строку (в младших 64K)
SCR_PRINTSTRZ:
        SPICS_SET
        LDI     TEMP,SCR_CHAR
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_SET_FPGA_REG
.ENDIF
        OUT     SPDR,TEMP
        RCALL   FPGA_RDY_RD
SCR_PRSTRZ1:
        LPM     DATA,Z+
        TST     DATA
        BREQ    SCR_PRSTRZ9
        CPI     DATA,$15
        BREQ    SCR_PRSTRZ2
        CPI     DATA,$16
        BREQ    SCR_PRSTRZ3
        RCALL   FPGA_SAME_REG
        RJMP    SCR_PRSTRZ1
SCR_PRSTRZ2:
        LPM     DATA,Z+
        LDI     TEMP,SCR_ATTR
        RCALL   FPGA_REG
        RJMP    SCR_PRINTSTRZ ;SCR_PRSTRZ1
SCR_PRSTRZ3:
        LPM     XL,Z+
        LPM     XH,Z+
        RCALL   SCR_SET_CURSOR
        RJMP    SCR_PRINTSTRZ ;SCR_PRSTRZ1
SCR_PRSTRZ9:
        RET
;
;--------------------------------------
;in:    Z == указатель на строку (в RAM)
;       COUNT == длина строки
SCR_PRNRAMSTRN:
        SPICS_SET
        LDI     TEMP,SCR_CHAR
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_SET_FPGA_REG
.ENDIF
        OUT     SPDR,TEMP
        RCALL   FPGA_RDY_RD
SCR_PRSN1:
        LD      DATA,Z+
        RCALL   FPGA_SAME_REG
        DEC     COUNT
        BRNE    SCR_PRSN1
        RET
;
;--------------------------------------
;in:    Z == указатель на строку (в младших 64K)
;       COUNT == длина строки
SCR_PRINTSTRN:
        SPICS_SET
        LDI     TEMP,SCR_CHAR
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_SET_FPGA_REG
.ENDIF
        OUT     SPDR,TEMP
        RCALL   FPGA_RDY_RD
SCR_PRSTRN1:
        LPM     DATA,Z+
        RCALL   FPGA_SAME_REG
        DEC     COUNT
        BRNE    SCR_PRSTRN1
        RET
;
;--------------------------------------
;in:    DATA
SCR_PUTCHAR:
        PUSH    TEMP
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
        POP     TEMP
        RET
;
;--------------------------------------
;Заполнение символом и атрибутом
;in:    DATA == символ
;       TEMP == атрибут
;       COUNT == количество
SCR_FILL_CHAR_ATTR:
        PUSH    DATA
        MOV     DATA,TEMP
        LDI     TEMP,SCR_ATTR
        RCALL   FPGA_REG
        POP     DATA
;Заполнение символом и текущим атрибутом
;in:    DATA == символ
;       COUNT == количество
SCR_FILL_CHAR:
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
        DEC     COUNT
        BRNE    SCR_FA1
        RET
;
;--------------------------------------
;Заполнение атрибутом
;in:    TEMP == атрибут
;       COUNT == количество
SCR_FILL_ATTR:
        MOV     DATA,TEMP
        LDI     TEMP,SCR_FILL
        RCALL   FPGA_REG
        DEC     COUNT
        BREQ    SCR_FA9
SCR_FA1:SPICS_CLR
        SPICS_SET
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_REPEAT_SEQ
.ENDIF
        DEC     COUNT
        BRNE    SCR_FA1
SCR_FA9:RET
;
;--------------------------------------
;Заполнение символом и атрибутом (LONG)
;in:    DATA == символ
;       TEMP == атрибут
;       W == количество
SCR_FILLLONG_CHAR_ATTR:
        PUSH    DATA
        MOV     DATA,TEMP
        LDI     TEMP,SCR_ATTR
        RCALL   FPGA_REG
        POP     DATA
;Заполнение символом и текущим атрибутом (LONG)
;in:    DATA == символ
;       W == количество
SCR_FILLLONG_CHAR:
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
        SBIW    WL,1
        BRNE    SCR_FL1
        RET
;
;--------------------------------------
;Заполнение атрибутом (LONG)
;in:    TEMP == атрибут
;       W == количество
SCR_FILLLONG_ATTR:
        MOV     DATA,TEMP
        LDI     TEMP,SCR_FILL
        RCALL   FPGA_REG
        SBIW    WL,1
        BREQ    SCR_FL9
SCR_FL1:SPICS_CLR
        SPICS_SET
.IFDEF DEBUG_FPGA_OUT
        CALL    DBG_REPEAT_SEQ
.ENDIF
        SBIW    WL,1
        BRNE    SCR_FL1
SCR_FL9:RET
;
;
;--------------------------------------
;
