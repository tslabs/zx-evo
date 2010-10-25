.NOLIST
.INCLUDE "M128DEF.INC"
.INCLUDE "_MACROS.ASM"
.LIST
.LISTMAC

.DEF    MODE1   =R10    ;глобальный регистр, (.0 - 1=TV mode; .1 - 0=сетка на "бордюре")
.DEF    LANG    =R11    ;глобальный регистр, язык интерфейса (умнож.на 2)
.DEF    INT6VECT=R12    ;глобальный регистр, (обработчики INT6)
.DEF    FF      =R13    ;глобальный регистр, всегда = $FF
.DEF    ONE     =R14    ;глобальный регистр, всегда = $01
.DEF    NULL    =R15    ;глобальный регистр, всегда = $00
.DEF    DATA    =R16
.DEF    TEMP    =R17
.DEF    COUNT   =R18
.DEF    BITS    =R19
.DEF    FLAGS1  =R20    ;глобальный регистр, флаги:
                        ;.0 - PUTCHAR вызывает UARTDIRECT_PUTCHAR
                        ;.1 - PUTCHAR вызывает UART_PUTCHAR
                        ;.2 - PUTCHAR вызывает SCR_PUTCHAR
                        ;.7 - видимый/скрытый курсор (на экране)
.DEF    TMP2    =R22
.DEF    TMP3    =R23
.DEF    WL      =R24
.DEF    WH      =R25
; DATA,TEMP,COUNT,WL,WH,XL,XH,ZL,ZH - могут передавать параметры в функции и возвращать результаты
; Y - указатель на стек данных (растёт вниз)
; R0,R1 и остальные - используются локально
;
;--------------------------------------
;
.EQU    DBSIZE_HI       =HIGH(2048)
.EQU    DBMASK_HI       =HIGH(2047)
.EQU    nCONFIG         =PORTF0
.EQU    nSTATUS         =PORTF1
.EQU    CONF_DONE       =PORTF2
;
;--------------------------------------
;регистры fpga
.EQU    TEMP_REG        =$A0
.EQU    SD_CS0          =$A1
.EQU    SD_CS1          =$A2
.EQU    FLASH_LOADDR    =$A3
.EQU    FLASH_MIDADDR   =$A4
.EQU    FLASH_HIADDR    =$A5
.EQU    FLASH_DATA      =$A6
.EQU    FLASH_CTRL      =$A7
.EQU    SCR_LOADDR      =$A8    ; текущая позиция печати
.EQU    SCR_HIADDR      =$A9    ;
.EQU    SCR_ATTR        =$AA    ; запись атрибута в ATTR
.EQU    SCR_FILL        =$AB    ; прединкремент адреса и запись атрибута в ATTR и в память
                                ; (если только дергать spics_n, будет писаться предыдущий ATTR)
.EQU    SCR_CHAR        =$AC    ; прединкремент адреса и запись символа в память и ATTR в память
                                ; (если только дергать spics_n, будет писаться предыдущий символ)
.EQU    SCR_MOUSE_TEMP  =TEMP_REG
.EQU    SCR_MOUSE_X     =$AD
.EQU    SCR_MOUSE_Y     =$AE
.EQU    SCR_MODE        =$AF    ; .0 - TV-mode (default==1)

.EQU    MTST_CONTROL    =$50
.EQU    MTST_PASS_CNT0  =$51
.EQU    MTST_PASS_CNT1  =TEMP_REG
.EQU    MTST_FAIL_CNT0  =$52
.EQU    MTST_FAIL_CNT1  =TEMP_REG

.EQU    COVOX           =$53

.EQU    INT_CONTROL     =$54
;
;--------------------------------------
;
.MACRO  SPICS_SET
        SBI     PORTB,0
.ENDMACRO

.MACRO  SPICS_CLR
        CBI     PORTB,0
.ENDMACRO
;
;--------------------------------------
;
.DSEG
        .ORG    $0300
DSTACK:
.EQU    UART_TXBSIZE    =128            ;размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт (...32,64,128,256)
UART_TX_BUFF:   .BYTE   UART_TXBSIZE    ;адрес д.б. кратен UART_TXBSIZE
.EQU    UART_RXBSIZE    =128            ;размер буфера д.б. равен СТЕПЕНЬ_ДВОЙКИ байт
UART_RX_BUFF:   .BYTE   UART_RXBSIZE    ;адрес д.б. кратен UART_RXBSIZE

        .ORG    $0400
BUFSECT:                ;буфер сектора
        .ORG    $0600
BUF4FAT:                ;временный буфер (FAT и т.п.)
        .ORG    $0800
MEGABUFFER:
        .ORG    RAMEND
HSTACK:
        .ORG    $0100
RND:    .BYTE   3
;
;--------------------------------------
;
.ESEG
                .ORG    $0000
EE_DUMMY:       .DB     $FF
EE_MODE1:       .DB     $FF
EE_LANG:        .DB     $00
;
;--------------------------------------
;
.CSEG
        .ORG    0
        JMP     START
        JMP     START   ;EXT_INT0 ; IRQ0 Handler
        JMP     START   ;EXT_INT1 ; IRQ1 Handler
        JMP     START   ;EXT_INT2 ; IRQ2 Handler
        JMP     START   ;EXT_INT3 ; IRQ3 Handler
        JMP     EXT_INT4 ; IRQ4 Handler
        JMP     EXT_INT5 ; IRQ5 Handler
        JMP     EXT_INT6 ; IRQ6 Handler
        JMP     START   ;EXT_INT7 ; IRQ7 Handler
        JMP     START   ;TIM2_COMP ; Timer2 Compare Handler
        JMP     START   ;TIM2_OVF ; Timer2 Overflow Handler
        JMP     START   ;TIM1_CAPT ; Timer1 Capture Handler
        JMP     START   ;TIM1_COMPA ; Timer1 CompareA Handler
        JMP     START   ;TIM1_COMPB ; Timer1 CompareB Handler
        JMP     START   ;TIM1_OVF ; Timer1 Overflow Handler
        JMP     START   ;TIM0_COMP ; Timer0 Compare Handler
        JMP     TIM0_OVF ; Timer0 Overflow Handler
        JMP     START   ;SPI_STC ; SPI Transfer Complete Handler
        JMP     START   ;USART0_RXC ; USART0 RX Complete Handler
        JMP     START   ;USART0_DRE ; USART0,UDR Empty Handler
        JMP     START   ;USART0_TXC ; USART0 TX Complete Handler
        JMP     START   ;ADC ; ADC Conversion Complete Handler
        JMP     START   ;EE_RDY ; EEPROM Ready Handler
        JMP     START   ;ANA_COMP ; Analog Comparator Handler
        JMP     START   ;TIM1_COMPC ; Timer1 CompareC Handler
        JMP     START   ;TIM3_CAPT ; Timer3 Capture Handler
        JMP     TIM3_COMPA ; Timer3 CompareA Handler
        JMP     START   ;TIM3_COMPB ; Timer3 CompareB Handler
        JMP     START   ;TIM3_COMPC ; Timer3 CompareC Handler
        JMP     START   ;TIM3_OVF ; Timer3 Overflow Handler
        JMP     USART1_RXC ; USART1 RX Complete Handler
        JMP     USART1_DRE ; USART1,UDR Empty Handler
        JMP     START   ;USART1_TXC ; USART1 TX Complete Handler
        JMP     START   ;TWI_INT ; Two-wire Serial Interface Interrupt Handler
        JMP     START   ;SPM_RDY ; SPM Ready Handler

        .DW     0,0
        .DB     "================"
        .DB     " ZX Evo Service "
        .DB     "================"
;
;--------------------------------------
;
.INCLUDE "_MESSAGE.INC"
.INCLUDE "_UART.ASM"
.INCLUDE "_TIMERS.ASM"
.INCLUDE "_PINTEST.ASM"
.INCLUDE "_PS2K.ASM"
.INCLUDE "_T_PS2K.ASM"
.INCLUDE "_T_PS2M.ASM"
.INCLUDE "_OUTPUT.ASM"
.INCLUDE "_SCREEN.ASM"
;
;--------------------------------------
;обмен с регистрами в FPGA
;in:    TEMP == номер регистра
;       DATA == данные
;out:   DATA == данные
FPGA_REG:
        PUSH    DATA
        SPICS_SET
        OUT     SPDR,TEMP
        RCALL   FPGA_RDY_RD
        POP     DATA
;обмен без установки регистра
;in:    DATA == данные
;out:   DATA == данные
FPGA_SAME_REG:
        SPICS_CLR
        OUT     SPDR,DATA
;ожидание окончания обмена с FPGA по SPI
;и чтение пришедших данных
;out:   DATA == данные
FPGA_RDY_RD:
;        SBIC    SPSR,WCOL
;        JMP     CHAOS00
        SBIS    SPSR,SPIF
        RJMP    FPGA_RDY_RD
        IN      DATA,SPDR
        SPICS_SET
        RET
;
;--------------------------------------
;
EXT_INT6:
        PUSH    DATA
        IN      DATA,SREG
        SBRC    INT6VECT,0
        CALL    T_BEEP_INT
        OUT     SREG,DATA
        POP     DATA
        RETI
;
;--------------------------------------
;
.INCLUDE "_SD_LOWL.ASM"
.INCLUDE "_T_ZXKBD.ASM"
.INCLUDE "_T_BEEP.ASM"
.INCLUDE "_SD_FAT.ASM"
.INCLUDE "_DEPACK.ASM"
.INCLUDE "_FLASHER.ASM"
.INCLUDE "_T_VIDEO.ASM"
.INCLUDE "_T_DRAM.ASM"
.INCLUDE "_MISC.ASM"
;
;--------------------------------------
;
START:  CLI
        CLR     R0
        LDIZ    $0001
CLRALL1:ST      Z+,R0
        CPI     ZL,$1E
        BRNE    CLRALL1
        LDI     ZL,$20
CLRALL2:ST      Z+,NULL
        CPI     ZH,$11
        BRNE    CLRALL2
        INC     ONE
        DEC     FF
;
        LDI     TEMP,LOW(HSTACK)
        OUT     SPL,TEMP
        LDI     TEMP,HIGH(HSTACK)
        OUT     SPH,TEMP
        LDIX    RND
        ST      X+,TEMP
        ST      X+,FF
        ST      X+,ONE
;
        LDIW    EE_MODE1
        CALL    EEPROM_READ
        MOV     MODE1,DATA
        LDI     WL,LOW(EE_LANG)
        CALL    EEPROM_READ
        CPI     DATA,MAX_LANG
        BRCS    RDE1
        CLR     DATA
RDE1:   LSL     DATA
        MOV     LANG,DATA
;
        CALL    PINTEST
; - - - - - - - - - - - - - - -
        LDI     TEMP,      0B11111111
        OUTPORT PORTG,TEMP
        LDI     TEMP,      0B00000000
        OUTPORT DDRG,TEMP

        LDI     TEMP,      0B00001000
        OUTPORT PORTF,TEMP
        OUTPORT DDRF,TEMP

        LDI     TEMP,      0B11101111
        OUT     PORTE,TEMP
        LDI     TEMP,      0B00010000
        OUT     DDRE,TEMP

        LDI     TEMP,      0B11111111
        OUT     PORTD,TEMP
        LDI     TEMP,      0B00000000
        OUT     DDRD,TEMP

        LDI     TEMP,      0B11011111
        OUT     PORTC,TEMP
        LDI     TEMP,      0B00000000
        OUT     DDRC,TEMP

        LDI     TEMP,      0B11111001
        OUT     PORTB,TEMP
        LDI     TEMP,      0B10000111
        OUT     DDRB,TEMP

        LDI     TEMP,      0B11111111
        OUT     PORTA,TEMP
        LDI     TEMP,      0B00000000
        OUT     DDRA,TEMP
; - - - - - - - - - - - - - - -
        LDIZ    MLMSG_STATUSOF_CRLF*2
        CALL    POWER_STATUS
        SBIS    PINF,0 ;VCC5
        RJMP    UP10
        SBIS    PINC,5 ;POWERGOOD
        RJMP    UP11
        RJMP    UP19
UP10:   LDIZ    MLMSG_POWER_ON*2
        CALL    PRINTMLSTR
;ждём включения ATX, а потом ещё чуть-чуть.
UP12:   SBIC    PINF,0 ;VCC5
        RJMP    UP11
        LDIZ    MLMSG_STATUSOF_CR*2
        CALL    POWER_STATUS
        RJMP    UP12
UP11:   LDI     COUNT,170 ;170 раз по 31 символу на скорости 115200 = ~500ms
UP13:   PUSH    COUNT
        LDIZ    MLMSG_STATUSOF_CR*2
        CALL    POWER_STATUS
        POP     COUNT
        DEC     COUNT
        BRNE    UP13
UP19:
; - - - - - - - - - - - - - - -
        LDIZ    MLMSG_CFGFPGA*2
        CALL    PRINTMLSTR
;SPI init
        LDI     TEMP,(1<<SPI2X)
        OUT     SPSR,TEMP
        LDI     TEMP,(1<<SPE)|(1<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP
;загрузка FPGA
        INPORT  TEMP,DDRF
        SBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

        DELAY_US 40

        INPORT  TEMP,DDRF
        CBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

LDFPGA1:SBIS    PINF,nSTATUS
        RJMP    LDFPGA1

        LDIZ    PACKED_FPGA*2
        OUT     RAMPZ,ONE
        CALL    DMLZ_INIT
LDFPGA3:CALL    DMLZ_GETBYTE
        BREQ    LDFPGA_DONE
        OUT     SPDR,DATA
LDFPGA2:SBIS    SPSR,SPIF
        RJMP    LDFPGA2
        RJMP    LDFPGA3
LDFPGA_DONE:
        SBIS    PINF,CONF_DONE
        RJMP    LDFPGA_DONE

        LDIZ    $0100
CLRRAM: ST      Z+,NULL
        CPI     ZH,$11
        BRNE    CLRRAM

;SPI reinit
        LDI     TEMP,(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP
; - - - - - - - - - - - - - - -
        LDIZ    MLMSG_DONE*2
        CALL    PRINTMLSTR
        DELAY_US 200
        LDIY    DSTACK
;
        CALL    UART_INIT
        CALL    PS2K_INIT
        CALL    TIMERS_INIT
        SEI

        MOV     DATA,MODE1
        LDI     TEMP,SCR_MODE
        CALL    FPGA_REG

        CALL    PS2K_DETECT_KBD

        LDI     DATA,$01
        LDI     TEMP,MTST_CONTROL
        CALL    FPGA_REG

        LDIZ    MSG_READY*2
        CALL    PRINTSTRZ
        CALL    SCR_KBDSETLED
;
NOEXIT:
        LDIZ    MENU_MAIN*2
        CALL    MENU
        RJMP    NOEXIT
;
MSG_READY:
        .DB     "---",$0D,$0A,0
;
;--------------------------------------
;
POWER_STATUS:
        CALL    PRINTMLSTR
        LDIZ    MSG_POWER_PG*2
        CALL    PRINTSTRZ
        LDI     DATA,$30 ;"0"
        SBIC    PINC,5 ;POWERGOOD
        LDI     DATA,$31 ;"1"
        CALL    HEXHALF
        LDIZ    MSG_POWER_VCC5*2
        CALL    PRINTSTRZ
        LDI     DATA,$30 ;"0"
        SBIC    PINF,0 ;VCC5
        LDI     DATA,$31 ;"1"
        JMP     HEXHALF
;
;--------------------------------------
;
;NOTHING:RET
;
;--------------------------------------
;
.NOLIST
        .ORG    $7F80
TABL_SINUS:
.INCLUDE "SIN256.INC"
        .ORG    $8000
PACKED_FPGA:
.INCLUDE "FPGA.INC"
;
;--------------------------------------
;
