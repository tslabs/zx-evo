.NOLIST
.INCLUDE "M128DEF.INC"
.INCLUDE "_MACROS.ASM"

.MACRO  SPICS_SET
        SBI     PORTB,0
.ENDMACRO

.MACRO  SPICS_CLR
        CBI     PORTB,0
.ENDMACRO

.MACRO  LED_ON
        CBI     PORTB,7
.ENDMACRO

.MACRO  LED_OFF
        SBI     PORTB,7
.ENDMACRO

.LIST
.LISTMAC

.DEF    FF_FL   =R08
.DEF    FF      =R13    ;всегда = $FF
.DEF    ONE     =R14    ;всегда = $01
.DEF    NULL    =R15    ;всегда = $00
.DEF    DATA    =R16
.DEF    TEMP    =R17
.DEF    COUNT   =R18
.DEF    BITS    =R19
;локально используются: R0,R1,R20,R21,R24,R25

.EQU    DBSIZE_HI       =HIGH(4096)
.EQU    DBMASK_HI       =HIGH(4095)
.EQU    nCONFIG         =PORTF0
.EQU    nSTATUS         =PORTF1
.EQU    CONF_DONE       =PORTF2

.EQU    CMD_17          =$51    ;read_single_block
.EQU    ACMD_41         =$69    ;sd_send_op_cond

.EQU    SD_CS0          =$57
.EQU    SD_CS1          =$5F
.EQU    FLASH_LOADDR    =$F0
.EQU    FLASH_MIDADDR   =$F1
.EQU    FLASH_HIADDR    =$F2
.EQU    FLASH_DATA      =$F3
.EQU    FLASH_CTRL      =$F4
.EQU    SCR_LOADDR      =$40
.EQU    SCR_HIADDR      =$41
.EQU    SCR_CHAR        =$44
.EQU    SCR_MODE        =$4E
;
;--------------------------------------
;
.DSEG
        .ORG    $0100
BUFFER:
        .ORG    $0300
BUF4FAT:
        .ORG    $0500
CAL_FAT:.BYTE   1       ;тип обнаруженной FAT
MANYFAT:.BYTE   1       ;количество FAT-таблиц
BYTSSEC:.BYTE   1       ;количество секторов в кластере
ROOTCLS:.BYTE   4       ;сектор начала root директории
SEC_FAT:.BYTE   4       ;количество секторов одной FAT
RSVDSEC:.BYTE   2       ;размер резервной области
STARTRZ:.BYTE   4       ;начало диска/раздела
FRSTDAT:.BYTE   4       ;адрес первого сектора данных от BPB
SEC_DSC:.BYTE   4       ;количество секторов на диске/разделе
CLS_DSC:.BYTE   4       ;количество кластеров на диске/разделе
FATSTR0:.BYTE   4       ;начало первой FAT таблицы
FATSTR1:.BYTE   4       ;начало второй FAT таблицы
TEK_DIR:.BYTE   4       ;кластер текущей директории
KCLSDIR:.BYTE   1       ;кол-во кластеров директории
NUMSECK:.BYTE   1       ;счетчик секторов в кластере
TFILCLS:.BYTE   4       ;текущий кластер
MPHWOST:.BYTE   1       ;кол-во секторов в последнем кластере
KOL_CLS:.BYTE   4       ;кол-во кластеров файла минус 1
ZTFILCLS:.BYTE  4
ZMPHWOST:.BYTE  1
ZKOL_CLS:.BYTE  4
SDERROR:.BYTE   1
LASTSECFLAG:
        .BYTE   1
F_ADDR0:.BYTE   1
F_ADDR1:.BYTE   1
F_ADDR2:.BYTE   1
ERRFLG1:.BYTE   1
ERRFLG2:.BYTE   1
;
;--------------------------------------
;
.CSEG
        .ORG    0
        JMP     START
        JMP     START   ;EXT_INT0       ; IRQ0 Handler
        JMP     START   ;EXT_INT1       ; IRQ1 Handler
        JMP     START   ;EXT_INT2       ; IRQ2 Handler
        JMP     START   ;EXT_INT3       ; IRQ3 Handler
        JMP     START   ;EXT_INT4       ; IRQ4 Handler
        JMP     START   ;EXT_INT5       ; IRQ5 Handler
        JMP     START   ;EXT_INT6       ; IRQ6 Handler
        JMP     START   ;EXT_INT7       ; IRQ7 Handler
        JMP     START   ;TIM2_COMP      ; Timer2 Compare Handler
        JMP     START   ;TIM2_OVF       ; Timer2 Overflow Handler
        JMP     START   ;TIM1_CAPT      ; Timer1 Capture Handler
        JMP     START   ;TIM1_COMPA     ; Timer1 CompareA Handler
        JMP     START   ;TIM1_COMPB     ; Timer1 CompareB Handler
        JMP     START   ;TIM1_OVF       ; Timer1 Overflow Handler
        JMP     START   ;TIM0_COMP      ; Timer0 Compare Handler
        JMP     START   ;TIM0_OVF       ; Timer0 Overflow Handler
        JMP     START   ;SPI_STC        ; SPI Transfer Complete Handler
        JMP     START   ;USART0_RXC     ; USART0 RX Complete Handler
        JMP     START   ;USART0_DRE     ; USART0,UDR Empty Handler
        JMP     START   ;USART0_TXC     ; USART0 TX Complete Handler
        JMP     START   ;ADC            ; ADC Conversion Complete Handler
        JMP     START   ;EE_RDY         ; EEPROM Ready Handler
        JMP     START   ;ANA_COMP       ; Analog Comparator Handler
        JMP     START   ;TIM1_COMPC     ; Timer1 CompareC Handler
        JMP     START   ;TIM3_CAPT      ; Timer3 Capture Handler
        JMP     START   ;TIM3_COMPA     ; Timer3 CompareA Handler
        JMP     START   ;TIM3_COMPB     ; Timer3 CompareB Handler
        JMP     START   ;TIM3_COMPC     ; Timer3 CompareC Handler
        JMP     START   ;TIM3_OVF       ; Timer3 Overflow Handler
        JMP     START   ;USART1_RXC     ; USART1 RX Complete Handler
        JMP     START   ;USART1_DRE     ; USART1,UDR Empty Handler
        JMP     START   ;USART1_TXC     ; USART1 TX Complete Handler
        JMP     START   ;TWI_INT        ; Two-wire Serial Interface Interrupt Handler
        JMP     START   ;SPM_RDY        ; SPM Ready Handler
;
;--------------------------------------
;
MSG_CFGFPGA:
        .DB     $0D,$0A,$0A,$0A,"Load FPGA configuration... ",0
MSG_OK:
        .DB     "Ok!",$0A
MSG_NEWLINE:
        .DB     $0D,$0A,0,0
;
MSG_TITLE:
        .DB     "  ZX Evolution Flasher ",0
MSG_ID_FLASH:
        .DB     "ID flash memory chip: ",0,0
MSG_OPENFILE:
        .DB     "Open file from SD-card...",0
MSG_SDERROR:
        .DB     "SD error: ",0,0
MSG_CARD:
        .DB     "Card",0,0
MSG_READERROR:
        .DB     "Read error",0,0
MSG_FAT:
        .DB     "FAT",0
MSG_FILE:
        .DB     "File",0,0
MSG_NOTFOUND:
        .DB     " not found",0,0
MSG_EMPTY:
        .DB     " empty",0,0
MSG_TOOBIG:
        .DB     " too big",0,0
MSG_F_ERASE:
        .DB     "Erase...",0,0
MSG_F_WRITE:
        .DB     "Write...",0,0
MSG_F_CHECK:
        .DB     "Check...",0,0
MSG_F_COMPLETE:
        .DB     "Successfully complete.",0,0
MSG_F_ERROR:
        .DB     "ERROR!",0,0
MSG_HALT:
        .DB     "HALT!",0
;
CMD00:  .DB     $40,$00,$00,$00,$00,$95
CMD08:  .DB     $48,$00,$00,$01,$AA,$87
CMD16:  .DB     $50,$00,$00,$02,$00,$FF
CMD55:  .DB     $77,$00,$00,$00,$00,$FF ;app_cmd
CMD58:  .DB     $7A,$00,$00,$00,$00,$FF ;read_ocr
CMD59:  .DB     $7B,$00,$00,$00,$00,$FF ;crc_on_off
FILENAME:
        .DB     "ZXEVO   ROM",0
;
PACKED_FPGA:
.NOLIST
.INCLUDE "FPGA.INC"
.LIST
.INCLUDE "_NVRAM.ASM"
;
;--------------------------------------
;
START:  CLI
        CLR     NULL
        LDI     TEMP,$01
        MOV     ONE,TEMP
        LDI     TEMP,$FF
        MOV     FF,TEMP
;WatchDog OFF
        LDI     TEMP,0B00011111
        OUT     WDTCR,TEMP
        OUT     WDTCR,NULL

        OUT     MCUCSR,NULL
;
        LDI     TEMP,LOW(RAMEND)
        OUT     SPL,TEMP
        LDI     TEMP,HIGH(RAMEND)
        OUT     SPH,TEMP
;
        OUT     RAMPZ,ONE
;
        LDI     TEMP,      0B11111111
        OUTPORT PORTG,TEMP
        LDI     TEMP,      0B00000000
        OUTPORT DDRG,TEMP

        LDI     TEMP,      0B00001000
        OUTPORT PORTF,TEMP
        OUTPORT DDRF,TEMP

        LDI     TEMP,      0B11110011
        OUT     PORTE,TEMP
        LDI     TEMP,      0B00000000
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
;UART1 Set baud rate
        OUTPORT UBRR1H,NULL
        LDI     TEMP,5     ;115200 baud @ 11059.2 kHz, Normal speed
        OUTPORT UBRR1L,TEMP
;UART1 Normal Speed
        OUTPORT UCSR1A,NULL
;UART1 data8bit, 2stopbits
        LDI     TEMP,(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS)
        OUTPORT UCSR1C,TEMP
;UART1 Разрешаем передачу
        LDI     TEMP,(1<<TXEN)
        OUTPORT UCSR1B,TEMP
;SPI init
        LDI     TEMP,(1<<SPI2X)
        OUT     SPSR,TEMP
        LDI     TEMP,(1<<SPE)|(1<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP
;ждём включения ATX, а потом ещё чуть-чуть.
UP11:   SBIS    PINF,0 ;PINC,5 ; а если powergood нет вообще ?
        RJMP    UP11
        LDI     DATA,5
        RCALL   DELAY

        LDIZ    MSG_CFGFPGA*2
        RCALL   UART_PRINTSTRZ
;загрузка FPGA
        INPORT  TEMP,DDRF
        SBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

        LDI     TEMP,147 ;40 us @ 11.0592 MHz
LDFPGA1:DEC     TEMP    ;1
        BRNE    LDFPGA1 ;2

        INPORT  TEMP,DDRF
        CBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

LDFPGA2:SBIS    PINF,nSTATUS
        RJMP    LDFPGA2

        LDIZ    PACKED_FPGA*2
        LDIY    BUFFER
;(не трогаем стек! всё ОЗУ под буфер)
        LDI     TEMP,$80
MS:     LPM     R0,Z+
        ST      Y+,R0
;-begin-PUT_BYTE_1---
        OUT     SPDR,R0
PUTB1:  SBIS    SPSR,SPIF
        RJMP    PUTB1
;-end---PUT_BYTE_1---
        SUBI    YH,HIGH(BUFFER) ;
        ANDI    YH,DBMASK_HI    ;Y warp
        ADDI    YH,HIGH(BUFFER) ;
M0:     LDI     R21,$02
        LDI     R20,$FF
M1:
M1X:    ADD     TEMP,TEMP
        BRNE    M2
        LPM     TEMP,Z+
        ROL     TEMP
M2:     ROL     R20
        BRCC    M1X
        DEC     R21
        BRNE    X2
        LDI     DATA,2
        ASR     R20
        BRCS    N1
        INC     DATA
        INC     R20
        BREQ    N2
        LDI     R21,$03
        LDI     R20,$3F
        RJMP    M1

X2:     DEC     R21
        BRNE    X3
        LSR     R20
        BRCS    MS
        INC     R21
        RJMP    M1

X6:     ADD     DATA,R20
N2:     LDI     R21,$04
        LDI     R20,$FF
        RJMP    M1

N1:     INC     R20
        BRNE    M4
        INC     R21
N5:     ROR     R20
        BRCS    DEMLZEND
        ROL     R21
        ADD     TEMP,TEMP
        BRNE    N6
        LPM     TEMP,Z+
        ROL     TEMP
N6:     BRCC    N5
        ADD     DATA,R21
        LDI     R21,6
        RJMP    M1
X3:     DEC     R21
        BRNE    X4
        LDI     DATA,1
        RJMP    M3
X4:     DEC     R21
        BRNE    X5
        INC     R20
        BRNE    M4
        LDI     R21,$05
        LDI     R20,$1F
        RJMP    M1
X5:     DEC     R21
        BRNE    X6
        MOV     R21,R20
M4:     LPM     R20,Z+
M3:     DEC     R21
        MOV     XL,R20
        MOV     XH,R21
        ADD     XL,YL
        ADC     XH,YH
LDIRLOOP:
        SUBI    XH,HIGH(BUFFER) ;
        ANDI    XH,DBMASK_HI    ;X warp
        ADDI    XH,HIGH(BUFFER) ;
        LD      R0,X+
        ST      Y+,R0
;-begin-PUT_BYTE_2---
        OUT     SPDR,R0
PUTB2:  SBIS    SPSR,SPIF
        RJMP    PUTB2
;-end---PUT_BYTE_2---
        SUBI    YH,HIGH(BUFFER) ;
        ANDI    YH,DBMASK_HI    ;Y warp
        ADDI    YH,HIGH(BUFFER) ;
        DEC     DATA
        BRNE    LDIRLOOP

        RJMP    M0
;теперь можно юзать стек
DEMLZEND:
        SBIS    PINF,CONF_DONE
        RJMP    DEMLZEND
;SPI reinit
        LDI     TEMP,(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP

        SBI     DDRE,6
        LED_OFF
        LDIZ    MSG_OK*2
        RCALL   UART_PRINTSTRZ
; - - - - - - - - - - - - - - - - - - -
        RCALL   NVRAM_READ_MODE
        LDI     TEMP,SCR_MODE
        RCALL   FPGA_REG
; - - - - - - - - - - - - - - - - - - -
        LDI     XL,0
        LDI     XH,0
        RCALL   SET_CURSOR
        LDIZ    MSG_TITLE*2
        RCALL   PRINTSTRZ

        LDIZ    LARGEBOOTSTART*2-4
;        OUT     RAMPZ,ONE
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
PRVERS9:
; - - - - - - - - - - - - - - - - - - -
;информация о Flash-ROM чипе
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,2
        RCALL   SET_CURSOR
        LDIZ    MSG_ID_FLASH*2
        RCALL   PRINTSTRZ

        RCALL   F_ID
        MOV     DATA,ZL
        RCALL   HEXBYTE
        LDI     DATA,$20
        RCALL   PUTCHAR
        MOV     DATA,ZH
        RCALL   HEXBYTE
; - - - - - - - - - - - - - - - - - - -
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,3
        RCALL   SET_CURSOR
        LDIZ    MSG_OPENFILE*2
        RCALL   PRINTSTRZ
;
;инициализация SD карточки
        LDI     TEMP,SD_CS1
        SER     DATA
        RCALL   FPGA_REG
        LDI     TEMP,32
        RCALL   SD_RD_DUMMY

        LDI     TEMP,SD_CS0
        SER     DATA
        RCALL   FPGA_REG
        SER     R24
SDINIT1:LDIZ    CMD00*2
        RCALL   SD_WR_PGM_6
        DEC     R24
        BRNE    SDINIT2
        LDI     DATA,1  ;нет SD
        RJMP    SD_ERROR
SDINIT2:CPI     DATA,$01
        BRNE    SDINIT1

        LDIZ    CMD08*2
        RCALL   SD_WR_PGM_6
        LDI     R24,$00
        SBRS    DATA,2
        LDI     R24,$40
        LDI     TEMP,4
        RCALL   SD_RD_DUMMY

SDINIT3:LDIZ    CMD55*2
        RCALL   SD_WR_PGM_6
        LDI     TEMP,2
        RCALL   SD_RD_DUMMY
        LDI     DATA,ACMD_41
        RCALL   SD_EXCHANGE
        MOV     DATA,R24
        RCALL   SD_EXCHANGE

        LDIZ    CMD55*2+2
        LDI     TEMP,4
        RCALL   SD_WR_PGX
        TST     DATA
        BRNE    SDINIT3

SDINIT4:LDIZ    CMD59*2
        RCALL   SD_WR_PGM_6
        TST     DATA
        BRNE    SDINIT4

SDINIT5:LDIZ    CMD16*2
        RCALL   SD_WR_PGM_6
        TST     DATA
        BRNE    SDINIT5
;
; - - - - - - - - - - - - - - - - - - -
;поиск FAT, инициализация переменных
WC_FAT: LDIX    0
        LDIY    0
        RCALL   LOADLST
        LDIZ    BUF4FAT+$01BE
        LD      DATA,Z
        TST     DATA
        BRNE    RDFAT05
        LDI     ZL,$C2
        LD      DATA,Z
        LDI     TEMP,0
        CPI     DATA,$01
        BREQ    RDFAT06
        LDI     TEMP,2
        CPI     DATA,$0B
        BREQ    RDFAT06
        CPI     DATA,$0C
        BREQ    RDFAT06
        LDI     TEMP,1
        CPI     DATA,$06
        BREQ    RDFAT06
        CPI     DATA,$0E
        BRNE    RDFAT05
RDFAT06:STS     CAL_FAT,TEMP
        LDI     ZL,$C6
        LD      XL,Z+
        LD      XH,Z+
        LD      YL,Z+
        LD      YH,Z
        RJMP    RDFAT00
RDFAT05:LDIZ    BUF4FAT
        LDD     BITS,Z+$0D
        LDI     DATA,0
        LDI     TEMP,0
        LDI     COUNT,8
RDF051: ROR     BITS
        ADC     DATA,NULL
        DEC     COUNT
        BRNE    RDF051
        DEC     DATA
        BRNE    RDF052
        INC     TEMP
RDF052: LDD     DATA,Z+$0E
        LDD     R0,Z+$0F
        OR      DATA,R0
        BREQ    RDF053
        INC     TEMP
RDF053: LDD     DATA,Z+$13
        LDD     R0,Z+$14
        OR      DATA,R0
        BRNE    RDF054
        INC     TEMP
RDF054: LDD     DATA,Z+$20
        LDD     R0,Z+$21
        OR      DATA,R0
        LDD     R0,Z+$22
        OR      DATA,R0
        LDD     R0,Z+$23
        OR      DATA,R0
        BRNE    RDF055
        INC     TEMP
RDF055: LDD     DATA,Z+$15
        ANDI    DATA,$F0
        CPI     DATA,$F0
        BRNE    RDF056
        INC     TEMP
RDF056: CPI     TEMP,4
        BREQ    RDF057
        LDI     DATA,3  ;не найдена FAT
        RJMP    SD_ERROR
RDF057: STS     CAL_FAT,FF
        LDIY    0
        LDIX    0
RDFAT00:STSX    STARTRZ+0
        STSY    STARTRZ+2
        RCALL   LOADLST
        LDIY    0
        LDD     XL,Z+22
        LDD     XH,Z+23         ;bpb_fatsz16
        MOV     DATA,XH
        OR      DATA,XL
        BRNE    RDFAT01         ;если не fat12/16 (bpb_fatsz16=0)
        LDD     XL,Z+36         ;то берем bpb_fatsz32 из смещения +36
        LDD     XH,Z+37
        LDD     YL,Z+38
        LDD     YH,Z+39
RDFAT01:STSX    SEC_FAT+0
        STSY    SEC_FAT+2       ;число секторов на fat-таблицу
        LDIY    0
        LDD     XL,Z+19
        LDD     XH,Z+20         ;bpb_totsec16
        MOV     DATA,XH
        OR      DATA,XL
        BRNE    RDFAT02         ;если не fat12/16 (bpb_totsec16=0)
        LDD     XL,Z+32         ;то берем из bpb_totsec32 смещения +32
        LDD     XH,Z+33
        LDD     YL,Z+34
        LDD     YH,Z+35
RDFAT02:STSX    SEC_DSC+0
        STSY    SEC_DSC+2       ;к-во секторов на диске/разделе
;вычисляем rootdirsectors
        LDD     XL,Z+17
        LDD     XH,Z+18         ;bpb_rootentcnt
        LDIY    0
        MOV     DATA,XH
        OR      DATA,XL
        BREQ    RDFAT03
        LDI     DATA,$10
        RCALL   BCDE_A
        MOVW    YL,XL           ;это реализована формула
                                ;rootdirsectors = ( (bpb_rootentcnt*32)+(bpb_bytspersec-1) )/bpb_bytspersec
                                ;в Y rootdirsectors
                                ;если fat32, то Y=0 всегда
RDFAT03:PUSH    YH
        PUSH    YL
        LDD     DATA,Z+16       ;bpb_numfats
        STS     MANYFAT,DATA
        LDSX    SEC_FAT+0
        LDSY    SEC_FAT+2
        DEC     DATA
RDF031: LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
        DEC     DATA
        BRNE    RDF031
        POP     R24
        POP     R25
                                ;полный размер fat-области в секторах
        RCALL   HLDEPBC         ;прибавили rootdirsectors
        LDD     R24,Z+14
        LDD     R25,Z+15        ;bpb_rsvdseccnt
        STS     RSVDSEC+0,R24
        STS     RSVDSEC+1,R25
        RCALL   HLDEPBC         ;прибавили bpb_resvdseccnt
        STSX    FRSTDAT+0
        STSY    FRSTDAT+2       ;положили номер первого сектора данных
        LDIZ    SEC_DSC
        RCALL   BCDEHLM         ;вычли из полного к-ва секторов раздела
        LDIZ    BUF4FAT
        LDD     DATA,Z+13
        STS     BYTSSEC,DATA
        RCALL   BCDE_A          ;разделили на к-во секторов в кластере
        STSX    CLS_DSC+0
        STSY    CLS_DSC+2       ;положили кол-во кластеров на разделе

        LDS     DATA,CAL_FAT
        CPI     DATA,$FF
        BRNE    RDFAT04
        LDSX    CLS_DSC+0
        LDSY    CLS_DSC+2
        PUSHY
        PUSHX
        LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
        RCALL   RASCHET
        LDI     DATA,1
        POPX
        POPY
        BREQ    RDFAT04
        LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
        LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
        RCALL   RASCHET
        LDI     DATA,2
        BREQ    RDFAT04
        CLR     DATA
RDFAT04:STS     CAL_FAT,DATA
;для fat12/16 вычисляем адрес первого сектора директории
;для fat32 берем по смещемию +44
;на выходе YX == сектор rootdir
        LDIX    0
        LDIY    0
        TST     DATA
        BREQ    FSRROO2
        DEC     DATA
        BREQ    FSRROO2
        LDD     XL,Z+44
        LDD     XH,Z+45
        LDD     YL,Z+46
        LDD     YH,Z+47
FSRROO2:STSX    ROOTCLS+0
        STSY    ROOTCLS+2       ;сектор root директории
        STSX    TEK_DIR+0
        STSY    TEK_DIR+2

FSRR121:PUSHX
        PUSHY
        LDSX    RSVDSEC
        LDIY    0
        LDIZ    STARTRZ
        RCALL   BCDEHLP
        STSX    FATSTR0+0
        STSY    FATSTR0+2
        LDIZ    SEC_FAT
        RCALL   BCDEHLP
        STSX    FATSTR1+0
        STSY    FATSTR1+2
        POPY
        POPX

        LDI     TEMP,1
        MOV     R0,XL
        OR      R0,XH
        OR      R0,YL
        OR      R0,YH
        BREQ    LASTCLS
NEXTCLS:PUSH    TEMP
        RCALL   RDFATZP
        RCALL   LST_CLS
        POP     TEMP
        BRCC    LASTCLS
        INC     TEMP
        RJMP    NEXTCLS
LASTCLS:STS     KCLSDIR,TEMP
        LDIY    0
        RCALL   RDDIRSC
;
; - - - - - - - - - - - - - - - - - - -
;поиск файла в директории
        LDIY    0               ;номер описателя файла
        RJMP    FNDMP32

FNDMP31:ADIW    YL,1            ;номер++               ─────────┐
        ADIW    ZL,$20          ;следующий описатель             │
        CPI     ZH,HIGH(BUF4FAT+512);                            │
                                ;вылезли за сектор?              │
        BRNE    FNDMP32         ;нет ещё                         │
        RCALL   RDDIRSC         ;считываем следующий             │
        BRNE    FNDMP37         ;кончились сектора в директории ═│═╗
FNDMP32:LDD     DATA,Z+$0B      ;атрибуты                        │ ║
        SBRC    DATA,3          ;длиное имя/имя диска?           │ ║
        RJMP    FNDMP31         ;да ────────────────────────────┤ ║
        SBRC    DATA,4          ;директория?                     │ ║
        RJMP    FNDMP31         ;да ────────────────────────────┤ ║
        LD      DATA,Z          ;первый символ                   │ ║
        CPI     DATA,$E5        ;удалённый файл?                 │ ║
        BREQ    FNDMP31         ;да ────────────────────────────┘ ║
        TST     DATA            ;пустой описатель? (конец списка)  ╚═ в этой директории
        BREQ    FNDMP37         ;да ═════════════════════════════════ нет нашёго файла
        PUSH    ZL
        MOVW    XL,ZL
        LDIZ    FILENAME*2
DALSHE: LPM     DATA,Z+
        TST     DATA
        BREQ    NASHEL
        LD      TEMP,X+
        CP      DATA,TEMP
        BREQ    DALSHE
;не совпало
        MOV     ZH,XH
        POP     ZL
        RJMP    FNDMP31
;нет такого файла
FNDMP37:
        LDI     DATA,4  ;нет файла
        RJMP    SD_ERROR
;найден описатель
NASHEL: MOV     ZH,XH
        POP     ZL
;
; - - - - - - - - - - - - - - - - - - -
;инициализация переменных
;для последующего чтения файла
;Z указывает на описатель файла
        LDD     XL,Z+$1A
        LDD     XH,Z+$1B
        LDD     YL,Z+$14
        LDD     YH,Z+$15        ;считали номер первого кластера файла
        STSX    TFILCLS+0
        STSY    TFILCLS+2
        STSX    ZTFILCLS+0
        STSY    ZTFILCLS+2
        LDD     XL,Z+$1C
        LDD     XH,Z+$1D
        LDD     YL,Z+$1E
        LDD     YH,Z+$1F        ;считали длину файла

        MOV     DATA,XL
        OR      DATA,XH
        OR      DATA,YL
        OR      DATA,YH
        BRNE    F01
        LDI     DATA,5  ;пустой файл
        RJMP    SD_ERROR
F01:
        LDI     DATA,$08
        CP      XL,ONE
        CPC     XH,NULL
        CPC     YL,DATA
        CPC     YH,NULL
        BRCS    F02
        LDI     DATA,5  ;большой файл
        RJMP    SD_ERROR
F02:
        LDI     R24,LOW(511)
        LDI     R25,HIGH(511)
        RCALL   HLDEPBC
        RCALL   BCDE200         ;получили кол-во секторов
        SBIW    XL,1
        SBC     YL,NULL
        SBC     YH,NULL
        LDS     DATA,BYTSSEC
        DEC     DATA
        AND     DATA,XL
        INC     DATA
        STS     MPHWOST,DATA    ;кол-во секторов в последнем кластере
        STS     ZMPHWOST,DATA
        LDS     DATA,BYTSSEC
        RCALL   BCDE_A
        STSX    KOL_CLS+0
        STSY    KOL_CLS+2
        STSX    ZKOL_CLS+0
        STSY    ZKOL_CLS+2
        STS     NUMSECK,NULL
; - - - - - - - - - - - - - - - - - - -
;всё нормально, начинаем
;стирание
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,4
        RCALL   SET_CURSOR
        LDIZ    MSG_F_ERASE*2
        RCALL   PRINTSTRZ
        RCALL   F_ERASE
;запись
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,5
        RCALL   SET_CURSOR
        LDIZ    MSG_F_WRITE*2
        RCALL   PRINTSTRZ
        LDI     XL,0
        LDI     XH,16
        RCALL   SET_CURSOR
        LDI     DATA,$01 ;"░"
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
        LDI     TEMP,$FF
FCHXY1: SPICS_CLR
        SPICS_SET
        DEC     TEMP
        BRNE    FCHXY1
        LDI     XL,0
        LDI     XH,16
        RCALL   SET_CURSOR

        STS     F_ADDR0,NULL
        STS     F_ADDR1,NULL
        STS     F_ADDR2,NULL

F13:    RCALL   NEXTSEC
        STS     LASTSECFLAG,DATA

        LDIZ    BUFFER
        LDS     XL,F_ADDR0
        LDS     XH,F_ADDR1
        LDS     YL,F_ADDR2

F11:    RCALL   F_WRITE
        ADIW    XL,1
        ADC     YL,NULL
        ADIW    ZL,1
        CPI     ZH,HIGH(BUFFER+512)
        BRNE    F11

        LED_OFF
        SBRC    XH,1
        LED_ON  ;мигать при программировании

        STS     F_ADDR0,XL
        STS     F_ADDR1,XH
        STS     F_ADDR2,YL

        CPI     XL,$00
        BRNE    F12
        MOV     DATA,XH
        ANDI    DATA,$07
        BRNE    F12
        LDI     DATA,$02 ;"▒"
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
F12:
        LDS     DATA,LASTSECFLAG
        TST     DATA
        BRNE    F13
;проверка
        RCALL   F_RST
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        RCALL   FPGA_REG
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,6
        RCALL   SET_CURSOR
        LDIZ    MSG_F_CHECK*2
        RCALL   PRINTSTRZ
        LDI     XL,0
        LDI     XH,16
        RCALL   SET_CURSOR

        LDSX    ZTFILCLS+0
        LDSY    ZTFILCLS+2
        STSX    TFILCLS+0
        STSY    TFILCLS+2
        LDS     DATA,ZMPHWOST
        STS     MPHWOST,DATA
        LDSX    ZKOL_CLS+0
        LDSY    ZKOL_CLS+2
        STSX    KOL_CLS+0
        STSY    KOL_CLS+2
        STS     NUMSECK,NULL
        STS     ERRFLG1,NULL
        STS     ERRFLG2,NULL
;
        STS     F_ADDR0,NULL
        STS     F_ADDR1,NULL
        STS     F_ADDR2,NULL

F25:    RCALL   NEXTSEC
        STS     LASTSECFLAG,DATA
        LDIZ    BUFFER
        LDS     XL,F_ADDR0
        LDS     XH,F_ADDR1
        LDS     YL,F_ADDR2

F21:    RCALL   F_IN
        LD      TEMP,Z+
        CP      DATA,TEMP
        BREQ    F26
        STS     ERRFLG1,ONE
F26:    ADIW    XL,1
        ADC     YL,NULL
        CPI     ZH,HIGH(BUFFER+512)
        BRNE    F21

        LED_OFF
        SBRC    XH,3
        LED_ON  ;мигать при проверке

        STS     F_ADDR0,XL
        STS     F_ADDR1,XH
        STS     F_ADDR2,YL

        CPI     XL,$00
        BRNE    F22
        MOV     DATA,XH
        ANDI    DATA,$07
        BRNE    F22
        LDS     TEMP,ERRFLG1
        TST     TEMP
        BREQ    F23
        STS     ERRFLG2,ONE
        STS     ERRFLG1,NULL
        LDI     DATA,$58 ;"X"
        RJMP    F24
F23:    LDI     DATA,$03 ;"█"
F24:    LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
F22:
        LDS     DATA,LASTSECFLAG
        TST     DATA
        BRNE    F25
;стоп
        LED_OFF
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,7
        RCALL   SET_CURSOR
        LDIZ    MSG_F_ERROR*2
        LDS     TEMP,ERRFLG2
        TST     TEMP
        BRNE    F91
        LDIZ    MSG_F_COMPLETE*2
F91:    RCALL   PRINTSTRZ
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,9
        RCALL   SET_CURSOR
        LDIZ    MSG_HALT*2
        RCALL   PRINTSTRZ
        CBI     DDRE,6
STOP1:  RJMP    STOP1

;
;--------------------------------------
;out:   DATA
SD_RECEIVE:
        SER     DATA
; - - - - - - - - - - - - - - - - - - -
;in:    DATA
;out:   DATA
SD_EXCHANGE:
        RJMP    FPGA_SAME_REG
;
;--------------------------------------
;in;    TEMP - n
SD_RD_DUMMY:
        SER     DATA
        RCALL   SD_EXCHANGE
        DEC     TEMP
        BRNE    SD_RD_DUMMY
        RET
;
;--------------------------------------
;in:    Z
SD_WR_PGM_6:
        LDI     TEMP,2
        RCALL   SD_RD_DUMMY
        LDI     TEMP,6
SD_WR_PGX:
SDWRP61:LPM     DATA,Z+
        RCALL   SD_EXCHANGE
        DEC     TEMP
        BRNE    SDWRP61
; - - - - - - - - - - - - - - - - - - -
;out:   DATA
SD_WAIT_NOTFF:
        LDI     TEMP,32
SDWNFF2:SER     DATA
        RCALL   SD_EXCHANGE
        CPI     DATA,$FF
        BRNE    SDWNFF1
        DEC     TEMP
        BRNE    SDWNFF2
SDWNFF1:RET
;
;--------------------------------------
;in:    Z - куда
;       Y,X - №сектора
SD_READ_SECTOR:
        PUSHZ
        LDIZ    CMD58*2
        RCALL   SD_WR_PGM_6
        RCALL   SD_RECEIVE
        SBRC    DATA,6
        RJMP    SDRDSE1
        LSL     XL
        ROL     XH
        ROL     YL
        MOV     YH,YL
        MOV     YL,XH
        MOV     XH,XL
        CLR     XL
SDRDSE1:
        LDI     TEMP,3+2
        RCALL   SD_RD_DUMMY

        LDI     DATA,CMD_17
        RCALL   SD_EXCHANGE
        MOV     DATA,YH
        RCALL   SD_EXCHANGE
        MOV     DATA,YL
        RCALL   SD_EXCHANGE
        MOV     DATA,XH
        RCALL   SD_EXCHANGE
        MOV     DATA,XL
        RCALL   SD_EXCHANGE
        SER     DATA
        RCALL   SD_EXCHANGE

        SER     R24
SDRDSE2:RCALL   SD_WAIT_NOTFF
        DEC     R24
        BREQ    SDRDSE8
        CPI     DATA,$FE
        BRNE    SDRDSE2

        POPZ
        LDI     R24,$00
        LDI     R25,$02
SDRDSE3:RCALL   SD_RECEIVE
        ST      Z+,DATA
        SBIW    R24,1
        BRNE    SDRDSE3

        LDI     TEMP,2
        RCALL   SD_RD_DUMMY
;SDRDSE4:RCALL   SD_WAIT_NOTFF
;        CPI     DATA,$FF
;        BRNE    SDRDSE4
        RET

SDRDSE8:
       LDI     DATA,2  ;ошибка при чтении сектора
        RJMP    SD_ERROR
;
;--------------------------------------
;чтение сектора данных
LOAD_DATA:
        LDIZ    BUFFER
        RCALL   SD_READ_SECTOR  ;читать один сектор
        RET
;
;--------------------------------------
;чтение сектора служ.инф. (FAT/DIR/...)
LOADLST:LDIZ    BUF4FAT
        RCALL   SD_READ_SECTOR  ;читать один сектор
        LDIZ    BUF4FAT
        RET
;
;--------------------------------------
;чтение сектора dir по номеру описателя (Y)
;на выходе: DATA=#ff (sreg.Z=0) выход за пределы dir
RDDIRSC:PUSHY
        MOVW    XL,YL
        LDIY    0
        LDI     DATA,$10
        RCALL   BCDE_A
        PUSH    XL
        LDS     DATA,BYTSSEC
        PUSH    DATA
        RCALL   BCDE_A
        LDS     DATA,KCLSDIR
        DEC     DATA
        CP      DATA,XL
        BRCC    RDDIRS3
        POP     YL
        POP     YL
        POPY
        SER     DATA
        TST     DATA
        RET
RDDIRS3:LDSY    TEK_DIR+2
        MOV     DATA,XL
        TST     DATA
        LDSX    TEK_DIR+0
        BREQ    RDDIRS1
RDDIRS2:PUSH    DATA
        RCALL   RDFATZP
        POP     DATA
        DEC     DATA
        BRNE    RDDIRS2
RDDIRS1:RCALL   REALSEC
        POP     R0
        DEC     R0
        POP     DATA
        AND     DATA,R0
        ADD     XL,DATA
        ADC     XH,NULL
        ADC     YL,NULL
        ADC     YH,NULL
        RCALL   LOADLST
        POPY
        CLR     DATA
        RET
;
;--------------------------------------
;out:   sreg.C == CLR - EOCmark
;(chng: TEMP)
LST_CLS:LDI     TEMP,$0F
        LDS     DATA,CAL_FAT
        TST     DATA
        BRNE    LST_CL1
        CPI     XL,$F7
        CPC     XH,TEMP
        RET
LST_CL1:DEC     DATA
        BRNE    LST_CL2
        CPI     XL,$F7
        CPC     XH,FF
        RET
LST_CL2:CPI     XL,$F7
        CPC     XH,FF
        CPC     YL,FF
        CPC     YH,TEMP
        RET
;
;--------------------------------------
;
RDFATZP:LDS     DATA,CAL_FAT
        TST     DATA
        BREQ    RDFATS0         ;FAT12
        DEC     DATA
        BREQ    RDFATS1         ;FAT16
;FAT32
        LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
        MOV     DATA,XL
        MOV     XL,XH
        MOV     XH,YL
        MOV     YL,YH
        CLR     YH
        RCALL   RDFATS2
        ADIW    ZL,1
        LD      YL,Z+
        LD      YH,Z
        RET
;FAT16
RDFATS1:LDIY    0
        MOV     DATA,XL
        MOV     XL,XH
        CLR     XH
RDFATS2:PUSH    DATA
        PUSHY
        LDIZ    FATSTR0
        RCALL   BCDEHLP
        RCALL   LOADLST
        POPY
        POP     DATA
        ADD     ZL,DATA
        ADC     ZH,NULL
        ADD     ZL,DATA
        ADC     ZH,NULL
        LD      XL,Z+
        LD      XH,Z
        RET
;FAT12
RDFATS0:MOVW    ZL,XL
        LSL     ZL
        ROL     ZH
        ADD     ZL,XL
        ADC     ZH,XH
        LSR     ZH
        ROR     ZL
        MOV     DATA,XL
        MOV     XL,ZH
        CLR     XH
        CLR     YL
        CLR     YH
        LSR     XL
        PUSH    DATA
        PUSHZ
        LDIZ    FATSTR0
        RCALL   BCDEHLP
        RCALL   LOADLST
        POPY
        ANDI    YH,$01
        ADD     ZL,YL
        ADC     ZH,YH
        LD      YL,Z+
        CPI     ZH,HIGH(BUF4FAT+512)
        BRNE    RDFATS4
        PUSH    YL
        LDIY    0
        ADIW    XL,1
        RCALL   LOADLST
        POP     YL
RDFATS4:POP     DATA
        LD      XH,Z
        MOV     XL,YL
        LDIY    0
        LSR     DATA
        BRCC    RDFATS3
        LSR     XH
        ROR     XL
        LSR     XH
        ROR     XL
        LSR     XH
        ROR     XL
        LSR     XH
        ROR     XL
RDFATS3:ANDI    XH,$0F
        RET
;
;--------------------------------------
;вычисление реального сектора
;на входе YX==номер FAT
;на выходе YX==адрес сектора
REALSEC:MOV     DATA,YH
        OR      DATA,YL
        OR      DATA,XH
        OR      DATA,XL
        BRNE    REALSE1
        LDIZ    FATSTR1
        LDSX    SEC_FAT+0
        LDSY    SEC_FAT+2
        RJMP    BCDEHLP
REALSE1:SBIW    XL,2            ;номер кластера-2
        SBC     YL,NULL
        SBC     YH,NULL
        LDS     DATA,BYTSSEC
        RJMP    REALSE2
REALSE3:LSL     XL
        ROL     XH
        ROL     YL
        ROL     YH
REALSE2:LSR     DATA
        BRCC    REALSE3
                                ;умножили на размер кластера
        LDIZ    STARTRZ
        RCALL   BCDEHLP         ;прибавили смещение от начала диска
        LDIZ    FRSTDAT
        RJMP    BCDEHLP         ;прибавили смещение от начала раздела
;
;--------------------------------------
;YX>>9 (деление на 512)
BCDE200:MOV     XL,XH
        MOV     XH,YL
        MOV     YL,YH
        LDI     YH,0
        LDI     DATA,1
; - - - - - - - - - - - - - - - - - - -
;YXDATA>>до"переноса"
;если в DATA вкл.только один бит, то получается
;YX=YX/DATA
BCDE_A1:LSR     YH
        ROR     YL
        ROR     XH
        ROR     XL
BCDE_A: ROR     DATA
        BRCC    BCDE_A1
        RET
;
;--------------------------------------
;YX=[Z]-YX
BCDEHLM:LD      DATA,Z+
        SUB     DATA,XL
        MOV     XL,DATA
        LD      DATA,Z+
        SBC     DATA,XH
        MOV     XH,DATA
        LD      DATA,Z+
        SBC     DATA,YL
        MOV     YL,DATA
        LD      DATA,Z
        SBC     DATA,YH
        MOV     YH,DATA
        RET
;
;--------------------------------------
;YX=YX+[Z]
BCDEHLP:LD      DATA,Z+
        ADD     XL,DATA
        LD      DATA,Z+
        ADC     XH,DATA
        LD      DATA,Z+
        ADC     YL,DATA
        LD      DATA,Z
        ADC     YH,DATA
        RET
;
;--------------------------------------
;YX=YX+R25R24
HLDEPBC:ADD     XL,R24
        ADC     XH,R25
        ADC     YL,NULL
        ADC     YH,NULL
        RET
;
;--------------------------------------
;
RASCHET:RCALL   BCDE200
        LDIZ    SEC_FAT
        RCALL   BCDEHLM
        MOV     DATA,XL
        ANDI    DATA,$F0
        OR      DATA,XH
        OR      DATA,YL
        OR      DATA,YH
        RET
;
;--------------------------------------
;чтение очередного сектора файла в BUFFER
;out:   DATA == 0 - считан последний сектор файла
NEXTSEC:
        LDI     TEMP,SD_CS0
        SER     DATA
        RCALL   FPGA_REG

        LDIZ    KOL_CLS
        LD      DATA,Z+
        LD      TEMP,Z+
        OR      DATA,TEMP
        LD      TEMP,Z+
        OR      DATA,TEMP
        LD      TEMP,Z+
        OR      DATA,TEMP
        BREQ    LSTCLSF
        LDSX    TFILCLS+0
        LDSY    TFILCLS+2
        RCALL   REALSEC
        LDS     DATA,NUMSECK
        ADD     XL,DATA
        ADC     XH,NULL
        ADC     YL,NULL
        ADC     YH,NULL
        RCALL   LOAD_DATA
        LDSX    TFILCLS+0
        LDSY    TFILCLS+2
        LDS     DATA,NUMSECK
        INC     DATA
        STS     NUMSECK,DATA
        LDS     TEMP,BYTSSEC
        CP      TEMP,DATA
        BRNE    NEXT_OK

        STS     NUMSECK,NULL
        RCALL   RDFATZP
        STSX    TFILCLS+0
        STSY    TFILCLS+2
        LDIZ    KOL_CLS
        LD      DATA,Z
        SUBI    DATA,1
        ST      Z+,DATA
        LD      DATA,Z
        SBC     DATA,NULL
        ST      Z+,DATA
        LD      DATA,Z
        SBC     DATA,NULL
        ST      Z+,DATA
        LD      DATA,Z
        SBC     DATA,NULL
        ST      Z+,DATA
NEXT_OK:SER     DATA
        RET

LSTCLSF:LDSX    TFILCLS+0
        LDSY    TFILCLS+2
        RCALL   REALSEC
        LDS     DATA,NUMSECK
        ADD     XL,DATA
        ADC     XH,NULL
        ADC     YL,NULL
        ADC     YH,NULL
        RCALL   LOAD_DATA
        LDS     DATA,NUMSECK
        INC     DATA
        STS     NUMSECK,DATA
        LDS     TEMP,MPHWOST
        SUB     DATA,TEMP
        RET
;
;--------------------------------------
;ошибки
SD_ERROR:
        STS     SDERROR,DATA
        LDI     TEMP,LOW(RAMEND)
        OUT     SPL,TEMP
        LDI     TEMP,HIGH(RAMEND)
        OUT     SPH,TEMP

        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,4
        RCALL   SET_CURSOR
        LDIZ    MSG_SDERROR*2
        RCALL   PRINTSTRZ
        LDS     DATA,SDERROR
        CPI     DATA,1
        BRNE    SD_ERR2
        LDIZ    MSG_CARD*2
        RCALL   PRINTSTRZ
        RJMP    SD_NOTFOUND
SD_ERR2:
        CPI     DATA,2
        BRNE    SD_ERR3
        LDIZ    MSG_READERROR*2
        RCALL   PRINTSTRZ
        RJMP    SD_ERR9
SD_ERR3:
        CPI     DATA,3
        BRNE    SD_ERR4
        LDIZ    MSG_FAT*2
        RCALL   PRINTSTRZ
        RJMP    SD_NOTFOUND
SD_ERR4:
        CPI     DATA,4
        BRNE    SD_ERR5
        LDIZ    MSG_FILE*2
        RCALL   PRINTSTRZ
SD_NOTFOUND:
        LDIZ    MSG_NOTFOUND*2
        RCALL   PRINTSTRZ
        RJMP    SD_ERR9
SD_ERR5:
        CPI     DATA,5
        BRNE    SD_ERR6
        LDIZ    MSG_FILE*2
        RCALL   PRINTSTRZ
        LDIZ    MSG_EMPTY*2
        RCALL   PRINTSTRZ
        RJMP    SD_ERR9
SD_ERR6:
        LDIZ    MSG_FILE*2
        RCALL   PRINTSTRZ
        LDIZ    MSG_TOOBIG*2
        RCALL   PRINTSTRZ
SD_ERR9:
;
        LDS     ZL,SDERROR
SD_ERR1:LED_ON
        LDI     DATA,5
        RCALL   BEEP
        LED_OFF
        LDI     DATA,5
        RCALL   DELAY
        DEC     ZL
        BRNE    SD_ERR1
;
        RCALL   UART_NEWLINE
        LDI     XL,0
        LDI     XH,6
        RCALL   SET_CURSOR
        LDIZ    MSG_HALT*2
        RCALL   PRINTSTRZ
        CBI     DDRE,6
STOP2:  RJMP    STOP2
;
;======================================
;чтение ID Flash-ROM чипа
;out:   ZL,ZH
F_ID:   RCALL   F_RST
        LDI     DATA,$90
        RCALL   F_CMD
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        RCALL   FPGA_REG
        LDI     XL,$00
        LDI     XH,$00
        LDI     YL,$00
        RCALL   F_IN
        MOV     ZL,DATA
        LDI     XL,$01
        RCALL   F_IN
        MOV     ZH,DATA
        RJMP    F_RST
;
;--------------------------------------
;запись одного байта во Flash-ROM
;in:    RAM[Z] == data
;       XL,XH,YL == address
F_WRITE:LDI     DATA,$A0
        RCALL   F_CMD
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000001
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_LOADDR
        MOV     DATA,XL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        MOV     DATA,XH
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_HIADDR
        MOV     DATA,YL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        LD      DATA,Z
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000101
        RCALL   FPGA_REG
        LDI     DATA,0B00000001
        RCALL   FPGA_SAME_REG
        LDI     DATA,0B00000011
        RCALL   FPGA_SAME_REG
        LDI     TEMP,FLASH_DATA
        RCALL   FPGA_REG
F_WRIT1:RCALL   FPGA_SAME_REG
        LD      TEMP,Z
        EOR     DATA,TEMP
        SBRC    DATA,7
        RJMP    F_WRIT1
        RET
;
;--------------------------------------
;стирание Flash-ROM
F_ERASE:LDI     DATA,$80
        RCALL   F_CMD
        LDI     DATA,$10
        RCALL   F_CMD
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        RCALL   FPGA_REG
F_ERAS1:LED_OFF
        RCALL   FPGA_SAME_REG
        LED_ON
        SBRS    DATA,7
        RJMP    F_ERAS1
;
; - - - - - - - - - - - - - - - - - - -
;сброс Flash-ROM чипа
F_RST:  LDI     DATA,$F0
        RCALL   F_CMD
        LDI     TEMP,19 ;~5 us @ 11.0592 MHz
F_RST1: DEC     TEMP    ;1
        BRNE    F_RST1  ;2
        RET
;
;--------------------------------------
;комманда в Flash-ROM чип
;in:    DATA == instructions
F_CMD:  PUSH    DATA
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000001
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_LOADDR
        LDI     DATA,$55
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        LDI     DATA,$55
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        LDI     DATA,$AA
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000101
        RCALL   FPGA_REG
        LDI     DATA,0B00000001
        RCALL   FPGA_SAME_REG
        LDI     TEMP,FLASH_LOADDR
        LDI     DATA,$AA
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        LDI     DATA,$2A
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        LDI     DATA,$55
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000101
        RCALL   FPGA_REG
        LDI     DATA,0B00000001
        RCALL   FPGA_SAME_REG
        LDI     TEMP,FLASH_LOADDR
        LDI     DATA,$55
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        LDI     DATA,$55
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        POP     DATA
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000101
        RCALL   FPGA_REG
        LDI     DATA,0B00000001
        RJMP    FPGA_SAME_REG
;
;--------------------------------------
;чтение одного байта Flash-ROM
;in:    XL,XH,YL == address
;out:   DATA == data
F_IN:   LDI     TEMP,FLASH_LOADDR
        MOV     DATA,XL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        MOV     DATA,XH
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_HIADDR
        MOV     DATA,YL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        LDI     DATA,$FF
        RJMP    FPGA_REG
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
        RCALL   RD_WHEN_RDY
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
RD_WHEN_RDY:
        SBIS    SPSR,SPIF
        RJMP    RD_WHEN_RDY
        IN      DATA,SPDR
        SPICS_SET
        RET
;
;--------------------------------------
;
UART_NEWLINE:
        LDIZ    MSG_NEWLINE*2
;
; - - - - - - - - - - - - - - - - - - -
;вывод строки на UART
;in:    Z == указательна строку (в младших 64K)
UART_PRINTSTRZ:
UPSTRZ1:LPM     DATA,Z+
        TST     DATA
        BREQ    UPSTRZ2
        RCALL   UART_PUTCHAR
        RJMP    UPSTRZ1
UPSTRZ2:RET
;
;--------------------------------------
;установка позиции печати на экране
;in:    XL == x (0..31)
;       XH == y (0..23)
SET_CURSOR:
        LDI     TEMP,32
        MUL     XH,TEMP
        CLR     XH
        ADD     XL,R0
        ADC     XH,R1
        SBIW    XL,1
        LDI     TEMP,SCR_LOADDR
        MOV     DATA,XL
        RCALL   FPGA_REG
        LDI     TEMP,SCR_HIADDR
        MOV     DATA,XH
        RJMP    FPGA_REG
;
;--------------------------------------
;вывод строки на экран и на UART
;in:    Z == указательна строку (в младших 64K)
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
;in:    DATA == byte (0..99)
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
;вывод символа на экран и на UART
;in:    DATA == char
PUTCHAR:PUSH    DATA
        PUSH    TEMP
        LDI     TEMP,SCR_CHAR
        RCALL   FPGA_REG
        POP     TEMP
        POP     DATA
UART_PUTCHAR:
        PUSH    TEMP
UPCHR1: INPORT  TEMP,UCSR1A
        SBRS    TEMP,UDRE
        RJMP    UPCHR1
        OUTPORT UDR1,DATA
        POP     TEMP
        RET
;
;--------------------------------------
;in:    DATA == продолжительность *0.1 сек
BEEP:
BEE2:   LDI     TEMP,100;100 периодов 1кГц
BEE1:   CBI     PORTE,6
        RCALL   BEEPDLY
        SBI     PORTE,6
        RCALL   BEEPDLY
        DEC     TEMP
        BRNE    BEE1
        DEC     DATA
        BRNE    BEE2
        RET

BEEPDLY:LDI     R24,$64
        LDI     R25,$05
BEEPDL1:SBIW    R24,1
        BRNE    BEEPDL1
        RET
;
;--------------------------------------
;in:    DATA == продолжительность *0.1 сек
DELAY:  LDI     R20,$1E ;\
        LDI     R21,$FE ;/ 0,1 сек @ 11.0592MHz
DELAY1: LPM             ;3
        LPM             ;3
        LPM             ;3
        LPM             ;3
        SUBI    R20,1   ;1
        SBCI    R21,0   ;1
        SBCI    DATA,0  ;1
        BRNE    DELAY1  ;2(1)
        RET
;
;--------------------------------------
;
