.NOLIST
.INCLUDE "M128DEF.INC"
.INCLUDE "_MACROS.ASM"
.LIST
.LISTMAC

.EQU    WHISTLES        =1

.DEF    POLY_LO =R06    ;всегда = $21
.DEF    POLY_HI =R07    ;всегда = $10
.DEF    FF_FL   =R08
.DEF    CRC_LO  =R09
.DEF    CRC_HI  =R10
.DEF    ADR1    =R11
.DEF    ADR2    =R12
.DEF    FF      =R13    ;всегда = $FF
.DEF    ONE     =R14    ;всегда = $01
.DEF    NULL    =R15    ;всегда = $00
.DEF    DATA    =R16
.DEF    TEMP    =R17
.DEF    COUNT   =R18
.DEF    BITS    =R19
.DEF    GUARD   =R22
;локально используются: R0,R1,R20,R21,R24,R25

.EQU    DBSIZE_HI       =HIGH(4096)
.EQU    DBMASK_HI       =HIGH(4095)
.EQU    nCONFIG         =PORTF0
.EQU    nSTATUS         =PORTF1
.EQU    CONF_DONE       =PORTF2

.EQU    SOH     =$01
.EQU    EOT     =$04
.EQU    ACK     =$06
.EQU    NAK     =$15
.EQU    CAN     =$18

.EQU    CMD_17  =$51    ;read_single_block
.EQU    ACMD_41 =$69    ;sd_send_op_cond

.DSEG
        .ORG    $0100
BUFFER:                 ;главный буфер
        .ORG    $0200
BUFSECT:                ;буфер сектора
        .ORG    $0400
BUF4FAT:                ;временный буфер (FAT и т.п.)
        .ORG    $0600
HEADER:                 ;заголовок файла
        .ORG    $0680
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
KOL_CLS:.BYTE   4       ;кол-во полных кластеров файла
STEP:   .BYTE   1


.CSEG
        .ORG    LARGEBOOTSTART
BOOTLOADER_BEGIN:
;
;FLASHSIZE - размер обновляемой области в блоках по 256 байт
.EQU    FLASHSIZE       =BOOTLOADER_BEGIN/128
;
RESET:  CLI
;Причина реcета? Если не PowerOn и не ExtReset то на основную программу
        IN      DATA,MCUCSR
        ANDI    DATA,0B00000011
        BRNE    START1
        JMP     0
;
BAD_BOOTLDR_CRC:
START1: CLR     NULL
        LDI     GUARD,$5A
        LDI     TEMP,$01
        MOV     ONE,TEMP
        LDI     TEMP,$FF
        MOV     FF,TEMP
        LDI     TEMP,$21
        MOV     POLY_LO,TEMP
        LDI     TEMP,$10
        MOV     POLY_HI,TEMP
;WatchDog OFF, если вдруг включен
        LDI     TEMP,0B00011111
        OUT     WDTCR,TEMP
        OUT     WDTCR,NULL
;LED on
        CBI     PORTB,7
        SBI     DDRB,7
;будем юзать стек
        LDI     TEMP,LOW(RAMEND)
        OUT     SPL,TEMP
        LDI     TEMP,HIGH(RAMEND)
        OUT     SPH,TEMP
;проверка CRC загрузчика
        LDIZ    BOOTLOADER_BEGIN*2              ;адрес в байтах
        OUT     RAMPZ,ONE
        LDIY    BOOTLOADER_END-BOOTLOADER_BEGIN ;длина в словах
        RCALL   CALK_CRC_FLASH
        BRNE    BAD_BOOTLDR_CRC ;ЧТО ДЕЛАТЬ, ЕСЛИ НЕ ПРАВИЛЬНАЯ CRC У BOOTLOADERа ???
;хочет ли пользователь обновиться ?
        SBIS    PINC,7
        RJMP    UPDATE_ME        ;если нажато "SoftReset"
;проверка CRC программы
START9: LDIZ    $0000                           ;адрес в байтах
        OUT     RAMPZ,NULL
        LDIY    FLASHSIZE<<7                    ;длина в словах
        RCALL   CALK_CRC_FLASH
        BRNE    UPDATE_ME        ;если некорректная CRC
;
        IN      TEMP,MCUCSR
        ANDI    TEMP,0B11111100
        OUT     MCUCSR,TEMP
        LDI     TEMP,0B00011000
        OUT     WDTCR,TEMP
GAVGAV: RJMP    GAVGAV
;
;--------------------------------------
;
UPDATE_ME:
        LDI     TEMP,      0B01111001 ;
        OUTPORT PORTB,TEMP
        LDI     TEMP,      0B10000111 ; LED on, spi outs
        OUTPORT DDRB,TEMP
;SPI init
        LDI     TEMP,(1<<SPI2X)
        OUT     SPSR,TEMP
        LDI     TEMP,(1<<SPE)|(1<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP
;UART1 Set baud rate
        OUTPORT UBRR1H,NULL
        LDI     TEMP,5     ;115200 baud, 11059.2 kHz, Normal speed
        OUTPORT UBRR1L,TEMP
;UART1 Normal Speed
        OUTPORT UCSR1A,NULL
;UART1 data8bit, 2stopbits
        LDI     TEMP,(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS)
        OUTPORT UCSR1C,TEMP
;UART1 Разрешаем передачу
        LDI     TEMP,(1<<TXEN)
        OUTPORT UCSR1B,TEMP

        INPORT  TEMP,DDRF
        SBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

        LDI     TEMP,147 ;40 us @ 11.0592 MHz
LDFPGA1:DEC     TEMP    ;1
        BRNE    LDFPGA1 ;2

        INPORT  TEMP,DDRF
        CBR     TEMP,(1<<nCONFIG)
        OUTPORT DDRF,TEMP

LDFPGA2:INPORT  DATA,PINF
        ANDI    DATA,(1<<nSTATUS)
        BREQ    LDFPGA2

        LDIZ    PACKED_FPGA*2
        OUT     RAMPZ,ONE
        LDIY    BUFFER
                                        ;A=DATA; A'=TEMP; B=R21; C=R20;
;deMLZ                                  ;DEC40
;(не трогаем стек! всё ОЗУ под буфер)
        LDI     TEMP,$80                ;        LD      A,#80
                                        ;        EX      AF,AF'--------
MS:     ELPM    R0,Z+                   ;MS      LDI
        ST      Y+,R0
;-begin-PUT_BYTE_1---
;UART тут для отладки ;)
;WRUX1:  INPORT  R1,UCSR1A
;        SBRS    R1,UDRE
;        RJMP    WRUX1
;        OUTPORT UDR1,R0
        OUT     SPDR,R0
PUTB1:  IN      R1,SPSR
        SBRS    R1,SPIF
        RJMP    PUTB1
;-end---PUT_BYTE_1---
        SUBI    YH,HIGH(BUFFER) ;
        ANDI    YH,DBMASK_HI    ;Y warp
        ADDI    YH,HIGH(BUFFER) ;
M0:     LDI     R21,$02                 ;M0      LD      BC,#2FF
        LDI     R20,$FF
M1:                                     ;M1      EX      AF,AF'--------
M1X:    ADD     TEMP,TEMP               ;M1X     ADD     A,A
        BRNE    M2                      ;        JR      NZ,M2
        ELPM    TEMP,Z+                 ;        LD      A,(HL)
                                        ;        INC     HL
        ROL     TEMP                    ;        RLA
M2:     ROL     R20                     ;M2      RL      C
        BRCC    M1X                     ;        JR      NC,M1X
                                        ;        EX      AF,AF'--------
        DEC     R21                     ;        DJNZ    X2
        BRNE    X2
        LDI     DATA,2                  ;        LD      A,2
        ASR     R20                     ;        SRA     C
        BRCS    N1                      ;        JR      C,N1
        INC     DATA                    ;        INC     A
        INC     R20                     ;        INC     C
        BREQ    N2                      ;        JR      Z,N2
        LDI     R21,$03                 ;        LD      BC,#33F
        LDI     R20,$3F
        RJMP    M1                      ;        JR      M1
                                        ;
X2:     DEC     R21                     ;X2      DJNZ    X3
        BRNE    X3
        LSR     R20                     ;        SRL     C
        BRCS    MS                      ;        JR      C,MS
        INC     R21                     ;        INC     B
        RJMP    M1                      ;        JR      M1
X6:                                     ;X6
        ADD     DATA,R20                ;        ADD     A,C
N2:     LDI     R21,$04                 ;N2
        LDI     R20,$FF                 ;        LD      BC,#4FF
        RJMP    M1                      ;        JR      M1
N1:                                     ;N1
        INC     R20                     ;        INC     C
        BRNE    M4                      ;        JR      NZ,M4
                                        ;        EX      AF,AF'--------
        INC     R21                     ;        INC     B
N5:     ROR     R20                     ;N5      RR      C
        BRCS    DEMLZEND                ;        RET     C
        ROL     R21                     ;        RL      B
        ADD     TEMP,TEMP               ;        ADD     A,A
        BRNE    N6                      ;        JR      NZ,N6
        ELPM    TEMP,Z+                 ;        LD      A,(HL)
                                        ;        INC     HL
        ROL     TEMP                    ;        RLA
N6:     BRCC    N5                      ;N6      JR      NC,N5
                                        ;        EX      AF,AF'--------
        ADD     DATA,R21                ;        ADD     A,B
        LDI     R21,6                   ;        LD      B,6
        RJMP    M1                      ;        JR      M1
X3:     DEC     R21                     ;X3
        BRNE    X4                      ;        DJNZ    X4
        LDI     DATA,1                  ;        LD      A,1
        RJMP    M3                      ;        JR      M3
X4:     DEC     R21                     ;X4      DJNZ    X5
        BRNE    X5
        INC     R20                     ;        INC     C
        BRNE    M4                      ;        JR      NZ,M4
        LDI     R21,$05                 ;        LD      BC,#51F
        LDI     R20,$1F
        RJMP    M1                      ;        JR      M1
X5:     DEC     R21                     ;X5
        BRNE    X6                      ;        DJNZ    X6
        MOV     R21,R20                 ;        LD      B,C
M4:     ELPM    R20,Z+                  ;M4      LD      C,(HL)
                                        ;        INC     HL
M3:     DEC     R21                     ;M3      DEC     B
                                        ;        PUSH    HL
        MOV     XL,R20                  ;        LD      L,C
        MOV     XH,R21                  ;        LD      H,B
        ADD     XL,YL                   ;        ADD     HL,DE
        ADC     XH,YH
                                        ;        LD      C,A
                                        ;        LD      B,0
LDIRLOOP:
        SUBI    XH,HIGH(BUFFER) ;
        ANDI    XH,DBMASK_HI    ;X warp
        ADDI    XH,HIGH(BUFFER) ;
        LD      R0,X+                   ;        LDIR
        ST      Y+,R0
;-begin-PUT_BYTE_2---
;UART тут для отладки ;)
;WRUX2:  INPORT  R1,UCSR1A
;        SBRS    R1,UDRE
;        RJMP    WRUX2
;        OUTPORT UDR1,R0
        OUT     SPDR,R0
PUTB2:  IN      R1,SPSR
        SBRS    R1,SPIF
        RJMP    PUTB2
;-end---PUT_BYTE_2---
        SUBI    YH,HIGH(BUFFER) ;
        ANDI    YH,DBMASK_HI    ;Y warp
        ADDI    YH,HIGH(BUFFER) ;
        DEC     DATA
        BRNE    LDIRLOOP
                                        ;        POP     HL
        RJMP    M0                      ;        JR      M0
                                        ;END_DEC40
DEMLZEND:;теперь можно юзать стек
;SPI reinit
        LDI     TEMP,(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
        OUT     SPCR,TEMP
;св.диод погасить
        SBI     PORTB,7
;
;--------------------------------------
;инициализация SD карточки
        CBI     PORTB,0 ;SDCS=0
        LDI     TEMP,32
        RCALL   SD_RD_DUMMY
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
        LDI     DATA,ACMD_41
        RCALL   SD_EXCHANGE
        MOV     DATA,R24
        RCALL   SD_EXCHANGE

        LDIZ    CMD55*2+1
        LDI     TEMP,5
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

        SBI     PORTB,0 ;SDCS=1
;
;--------------------------------------
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
        BREQ    LASTCLS
        INC     TEMP
        RJMP    NEXTCLS
LASTCLS:STS     KCLSDIR,TEMP
        LDIY    0
        RCALL   RDDIRSC
;
;--------------------------------------
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
;        OUT     RAMPZ,ONE
DALSHE: ELPM    DATA,Z+
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
;--------------------------------------
;инициализация переменных
;для последующего чтения файла
;Z указывает на описатель файла
        LDD     XL,Z+$1A
        LDD     XH,Z+$1B
        LDD     YL,Z+$14
        LDD     YH,Z+$15        ;считали номер первого кластера файла
        STSX    TFILCLS+0
        STSY    TFILCLS+2
        LDD     XL,Z+$1C
        LDD     XH,Z+$1D
        LDD     YL,Z+$1E
        LDD     YH,Z+$1F        ;считали длину файла
        LDI     R24,LOW(511)
        LDI     R25,HIGH(511)
        RCALL   HLDEPBC
        RCALL   BCDE200         ;получили кол-во секторов
        LDS     DATA,BYTSSEC
        DEC     DATA
        AND     DATA,XL
        INC     DATA
        STS     MPHWOST,DATA    ;кол-во секторов в последнем кластере
        LDS     DATA,BYTSSEC
        RCALL   BCDE_A          ;получили кол-во (полных) кластеров
        STSX    KOL_CLS+0
        STSY    KOL_CLS+2
        STS     NUMSECK,NULL
;
;--------------------------------------
;загружаем данные из файла, шьём во флеш
        RCALL   NEXTSEC
        STS     STEP,NULL

        LDIY    BUFSECT
        LDIX    HEADER
        CLR     CRC_LO
        CLR     CRC_HI
        LDI     R20,128
SDUPD01:LD      DATA,Y+
        ST      X+,DATA
        RCALL   CRC_UPDATE
        DEC     R20
        BRNE    SDUPD01
        OR      CRC_LO,CRC_HI
        BREQ    SDUPD02
SDUPD03:
       LDI     DATA,5  ;ошибка в файле (CRC/signature)
        RJMP    SD_ERROR
SDUPD02:RCALL   CHECK_SIGNATURE
        BRNE    SDUPD03

        LDI     XL,LOW(HEADER+$40)
;        LDI     XH,HIGH(HEADER+$40)
        CLR     ADR1
        CLR     ADR2
SDUPD13:LDI     COUNT,8
        LD      BITS,X+
SDUPD12:LSR     BITS
        BRCS    SDUPD20
;"пустой" блок
        MOV     FF_FL,FF
        RCALL   BLOCK_FLASH
        BREQ    SDUPD90
SDUPD11:
;св.диод - мигать при обновлении
        SBI     PORTB,7
        SBRS    ZH,5
        CBI     PORTB,7

        DEC     COUNT
        BRNE    SDUPD12
        RJMP    SDUPD13
;подготавливаем данные
;если необходимо загружаем с SD
SDUPD20:PUSHX
        PUSH    BITS
        PUSH    COUNT
        LDS     DATA,STEP
        TST     DATA
        BRNE    SDUPD30
;
        LDIY    BUFSECT+128
        LDIX    BUFFER
        CLR     R20
SDUPD21:LD      DATA,Y+
        ST      X+,DATA
        DEC     R20
        BRNE    SDUPD21
        STS     STEP,ONE
        RJMP    SDUPD80
;
SDUPD30:LDIY    BUFSECT+384
        LDIX    BUFFER
        LDI     R20,128
SDUPD31:LD      DATA,Y+
        ST      X+,DATA
        DEC     R20
        BRNE    SDUPD31

        RCALL   NEXTSEC
        STS     STEP,NULL

        LDIY    BUFSECT
        LDIX    BUFFER+128
        LDI     R20,128
SDUPD32:LD      DATA,Y+
        ST      X+,DATA
        DEC     R20
        BRNE    SDUPD32
;шьём блок
SDUPD80:POP     COUNT
        POP     BITS
        POPX
        CLR     FF_FL
        RCALL   BLOCK_FLASH
        BRNE    SDUPD11
;
SDUPD90:RJMP    START9  ;проверка CRC основной программы и если всё Ok её запуск.
;
;--------------------------------------
;out:   DATA
SD_RECEIVE:
        SER     DATA
; - - - - - - - - - - - - - - - - - - -
;in:    DATA
;out:   DATA
SD_EXCHANGE:
        OUT     SPDR,DATA
SDEXCH: IN      R0,SPSR
        SBRS    R0,SPIF
        RJMP    SDEXCH
        IN      DATA,SPDR
        RET
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
        LDI     TEMP,6
SD_WR_PGX:
        OUT     RAMPZ,ONE
SDWRP61:ELPM    DATA,Z+
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
        CBI     PORTB,0 ;SDCS=0

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
        LDI     TEMP,3
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
SDRDSE4:RCALL   SD_WAIT_NOTFF
        CPI     DATA,$FF
        BRNE    SDRDSE4

        SBI     PORTB,0 ;SDCS=1
        RET

SDRDSE8:
       LDI     DATA,2  ;ошибка при чтении сектора
        RJMP    SD_ERROR
;
;--------------------------------------
;
LOAD_DATA:
        LDIZ    BUFSECT
        RCALL   SD_READ_SECTOR  ;читать один сектор
;;;;;;;;
;       LDIZ    BUFSECT
;RAMOU1:LD      DATA,Z+
;       RCALL   WRUART
;       CPI     ZH,HIGH(BUFSECT+512)
;       BRNE    RAMOU1
;;;;;;;;
        RET
;
;--------------------------------------
;
LOADLST:LDIZ    BUF4FAT
        RCALL   SD_READ_SECTOR  ;читать один сектор
;;;;;;;;
;       LDIZ    BUF4FAT
;RAMOU2:LD      DATA,Z+
;       RCALL   WRUART
;       CPI     ZH,HIGH(BUF4FAT+512)
;       BRNE    RAMOU2
;;;;;;;;
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
;
LST_CLS:LDS     DATA,CAL_FAT
        TST     DATA
        BRNE    LST_CL1
        CPI     XH,$0F
        BRNE    LST_NO
        CPI     XL,$FF
        RET
LST_CL1:DEC     DATA
        BRNE    LST_CL2
        CPI     XH,$FF
        BRNE    LST_NO
        CPI     XL,$FF
        RET
LST_CL2:CPI     YH,$0F
        BRNE    LST_NO
        CPI     YL,$FF
        BRNE    LST_NO
        CPI     XH,$FF
        BRNE    LST_NO
        CPI     XL,$FF
LST_NO: RET
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
;out:   sreg.Z == SET - считан последний сектор файла
NEXTSEC:LDIZ    KOL_CLS
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
NEXT_OK:TST     ONE
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
;        TST     TEMP
;        BREQ    NEXT_OK
        CP      TEMP,DATA
        RET
;
;--------------------------------------
;
SD_ERROR:
        SBI     PORTB,0 ;SDCS=1
        LDI     TEMP,LOW(RAMEND)
        OUT     SPL,TEMP
        LDI     TEMP,HIGH(RAMEND)
        OUT     SPH,TEMP

        MOV     ZL,DATA
        RCALL   WRUART

SD_ERR1:CBI     PORTB,7
        LDI     DATA,5
        RCALL   BEEP
        SBI     PORTB,7
        LDI     DATA,5
        RCALL   DELAY
        DEC     ZL
        BRNE    SD_ERR1

;HALT:   RJMP    HALT


;;UPDATE_FROM_UART:
;;UART1 Set baud rate
;        OUTPORT UBRR1H,NULL
;        LDI     TEMP,5     ;115200 baud @ 11.0592 MHz, Normal speed
;        OUTPORT UBRR1L,TEMP
;;UART1 Normal Speed
;        OUTPORT UCSR1A,NULL
;;UART1 data8bit, 2stopbits
;        LDI     TEMP,(1<<UCSZ1)|(1<<UCSZ0)|(1<<USBS)
;        OUTPORT UCSR1C,TEMP

;UART1 Разрешаем приём/передачу
        LDI     TEMP,(1<<RXEN)|(1<<TXEN)
        OUTPORT UCSR1B,TEMP
;
.IFDEF  WHISTLES
        LDIZ    MSG_TITLE*2
        RCALL   STRING2UART
.ENDIF
;
;инициируем обмен по протоколу XModem-CRC
        LDI     TEMP,20 ;если в течении ~60 секунд не начнётся обмен - будет перезагрузка бутлоадера (20*timeout=60)
UUPD00: PUSH    TEMP
        LDI     DATA,$43
        LDIY    HEADER
        RCALL   XMODEM_PACKET_RECEIVER
        POP     TEMP
        BRNE    UUPD01
        DEC     TEMP
        BRNE    UUPD00
        RJMP    START1

UUPD01: OR      CRC_LO,CRC_HI
        BREQ    UUPD03
.IFDEF  WHISTLES
UUPD04:
        LDI     DATA,CAN
        RCALL   WRUART
        RCALL   WRUART
        RCALL   WRUART
        RCALL   WRUART
        RCALL   WRUART
        RCALL   DELAY_3SEC
        LDIZ    MSG_WRONGDATA*2
        RCALL   STRING2UART
        RCALL   DELAY_3SEC
        RJMP    START9  ;проверка CRC основной программы и если всё Ok её запуск.
.ELSE
        RJMP    UUPD_F2
.ENDIF
UUPD03:
        RCALL   CHECK_SIGNATURE
.IFDEF  WHISTLES
        BRNE    UUPD04
.ELSE
        BRNE    UUPD_F2
.ENDIF
;-------
        LDI     XL,LOW(HEADER+$40)
;        LDI     XH,HIGH(HEADER+$40)
        CLR     ADR1
        CLR     ADR2
UUPD13: LDI     COUNT,8
        LD      BITS,X+
UUPD12: LSR     BITS
        BRCS    UUPD14
;пропускаем "пустой" блок
        RCALL   INCADR
        BREQ    UUPD20
UUPD11: DEC     COUNT
        BRNE    UUPD12
        RJMP    UUPD13
;загружаем блок
UUPD14: LDIY    BUFFER
        LDI     DATA,ACK
UUPD15: RCALL   XMODEM_PACKET_RECEIVER
        BRNE    UUPD16
        LDI     DATA,NAK
        RJMP    UUPD15
UUPD16: LDI     DATA,ACK
UUPD17: RCALL   XMODEM_PACKET_RECEIVER
        BRNE    UUPD18
        LDI     DATA,NAK
        RJMP    UUPD17
;шьём принятый блок (два XModem-ых пакета по 128 байт)
UUPD18: CLR     FF_FL
        RCALL   BLOCK_FLASH
        BRNE    UUPD11
;-------
UUPD20:
;        LDIY    BUFFER
;UUPD20A:ST      Y+,FF
;        TST     YL
;        BRNE    UUPD20A
        LDI     XL,LOW(HEADER+$40)
;        LDI     XH,HIGH(HEADER+$40)
        CLR     ADR1
        CLR     ADR2
UUPD23: LDI     COUNT,8
        LD      BITS,X+
UUPD22: LSR     BITS
        BRCC    UUPD24
;пропускаем блок
        RCALL   INCADR
        BREQ    UUPD_FINISH
UUPD21: DEC     COUNT
        BRNE    UUPD22
        RJMP    UUPD23
;стираем "пустой" блок
UUPD24: MOV     FF_FL,FF
        RCALL   BLOCK_FLASH
        BRNE    UUPD21
;-------
UUPD_FINISH:
        LDI     DATA,ACK
        RCALL   WRUART
        RCALL   RDUART
        CPI     DATA,EOT ; обязательно должно придти EOT
        LDI     DATA,ACK
        BREQ    UUPD_F1
UUPD_F2:LDI     DATA,CAN
        RCALL   WRUART
        RCALL   WRUART
        RCALL   WRUART
        RCALL   WRUART
UUPD_F1:RCALL   WRUART
.IFDEF  WHISTLES
        RCALL   DELAY_3SEC
        LDIZ    MSG_SUCCESSFULLY*2
        RCALL   STRING2UART
        LDIX    HEADER+6
UUPD_F8:LD      DATA,X+
        TST     DATA
        BREQ    UUPD_F9
        RCALL   WRUART
        RJMP    UUPD_F8
UUPD_F9:
.ENDIF
        RJMP    START9  ;проверка CRC основной программы и если всё Ok её запуск.
;
;--------------------------------------
;
CHECK_SIGNATURE:
        LDIZ    SIGNATURE*2
        OUT     RAMPZ,ONE
        LDIX    HEADER
CHKSIG1:ELPM    DATA,Z+
        LD      TEMP,X+
        CP      DATA,TEMP
        BRNE    CHKSIG9
        CPI     DATA,$1A
        BRNE    CHKSIG1
CHKSIG9:RET
;
;--------------------------------------
;XMODEM_PACKET_RECEIVER
;in:    DATA == <C>, <NAK> или <ACK>
;       Y == указатель на буфер
;out:   sreg.Z == SET - timeout (Y без изменений)
;                 CLR - Ok! (Y=+128)
XMRXERR:SUBI    YL,128
        SBC     YH,NULL
        LDI     DATA,NAK
;
XMODEM_PACKET_RECEIVER:
        RCALL   WRUART
        LDI     TEMP,6 ;таймаут 3 сек.
XMRX3:
;св.диод - мигать при ожидании
        SBI     PORTB,7
        SBRC    TEMP,0
        CBI     PORTB,7

        LDI     R20,$00 ;\
        LDI     R21,$70 ; > ~0,5 сек
        LDI     ZL,$05  ;/
XMRX1:  SUBI    R20,1
        SBCI    R21,0
        SBCI    ZL,0
        BRNE    XMRX2
        DEC     TEMP
        BRNE    XMRX3
        RET

XMRX2:  RCALL   INUART
        BREQ    XMRX1
        CPI     DATA,SOH
        BRNE    XMRX1
;св.диод - мигать
        SBI     PORTB,7
        SBRS    ADR1,1
        CBI     PORTB,7

        RCALL   RDUART  ;block num
        RCALL   RDUART  ;block num (inverted)
        CLR     CRC_LO
        CLR     CRC_HI
        LDI     R20,128
XMRX4:  RCALL   RDUART
        ST      Y+,DATA
        RCALL   CRC_UPDATE
        DEC     R20
        BRNE    XMRX4

        RCALL   RDUART
        MOV     TEMP,DATA
        RCALL   RDUART
        CP      DATA,CRC_LO
        BRNE    XMRXERR
        CP      TEMP,CRC_HI
        BRNE    XMRXERR
        CLZ
        RET
;
;
;--------------------------------------
;STRING2UART
;in:    Z == указательна строку (в буте)
STRING2UART:
        OUT     RAMPZ,ONE
SRT2UA1:ELPM    DATA,Z+
        TST     DATA
        BREQ    SRT2UA2
        RCALL   WRUART
        RJMP    SRT2UA1
SRT2UA2:RET
;
;--------------------------------------
;WRUART
;in:    DATA == передаваемый байт
WRUART: PUSH    TEMP
WRU_1:  INPORT  TEMP,UCSR1A
        SBRS    TEMP,UDRE
        RJMP    WRU_1
        OUTPORT UDR1,DATA
        POP     TEMP
        RET
;
;--------------------------------------
;RDUART
;out:   DATA == принятый байт
RDUART: INPORT  DATA,UCSR1A
        SBRS    DATA,RXC
        RJMP    RDUART
        INPORT  DATA,UDR1
        RET
;
;--------------------------------------
;INUART
;out:   sreg.Z == CLR - есть данные (DATA == принятый байт)
;                 SET - нет данных
INUART: INPORT  DATA,UCSR1A
        SBRS    DATA,RXC
        RJMP    INU9
        INPORT  DATA,UDR1
        CLZ
INU9:   RET
;
;--------------------------------------
;in:    DATA - длительность *0.1ms
BEEP:   ;SBI     PORTB,0 ;SDCS=1
        OUT     SPCR,NULL       ;SPI off

BEE2:   LDI     TEMP,100;100 периодов 1кГц
BEE1:   SBI     PORTB,1
        RCALL   BEEPDLY
        CBI     PORTB,1
        RCALL   BEEPDLY
        DEC     TEMP
        BRNE    BEE1
        DEC     DATA
        BRNE    BEE2
;;SPI reinit
;        LDI     TEMP,(1<<SPE)|(0<<DORD)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)
;        OUT     SPCR,TEMP
        RET

BEEPDLY:LDI     R24,$64
        LDI     R25,$05
BEEPDL1:SBIW    R24,1
        BRNE    BEEPDL1
        RET
;
;--------------------------------------
;
DELAY_3SEC:
        LDI     DATA,30
; - - - - - - - - - - - - - - - - - - -
;DELAY
;in:    DATA/10 == количество секунд
DELAY:
        LDI     R20,$1E ;\
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
;CALK_CRC_FLASH
;in:    Z == адрес в байтах
;       Y == кол-во слов
;out:   sreg.Z == SET - crc ok!
CALK_CRC_FLASH:
        MOV     CRC_LO,FF
        MOV     CRC_HI,FF
CCRCFL1:ELPM    DATA,Z+
        RCALL   CRC_UPDATE
        ELPM    DATA,Z+
        RCALL   CRC_UPDATE
;св.диод - мигать при подсчёте CRC
        SBI     PORTB,7
        SBRS    ZH,5
        CBI     PORTB,7

        SBIW    YL,1
        BRNE    CCRCFL1
        OR      CRC_LO,CRC_HI
        RET
;
;--------------------------------------
;
;in:    DATA - byte
;       CRC_LO,CRC_HI
;out:   CRC_LO,CRC_HI
;cng:   TEMP
CRC_UPDATE:
;        PUSH    TEMP
        EOR     CRC_HI,DATA
        LDI     TEMP,8
CRC_UP2:LSL     CRC_LO
        ROL     CRC_HI
        BRCC    CRC_UP1
        EOR     CRC_LO,POLY_LO
        EOR     CRC_HI,POLY_HI
CRC_UP1:DEC     TEMP
        BRNE    CRC_UP2
;        POP     TEMP
        RET
;
;--------------------------------------
;BLOCK_FLASH
;in:    [BUFFER] == data
;       ADR1 == mid address
;       ADR2 == high address
;       FF_FL == $FF - erase only
;out:   sreg.Z == SET - это был последний блок (выше по адресам обновлять запрещено!)
;       ADR1 == mid address
;       ADR2 == high address
BLOCK_FLASH:
        CLR     ZL
        MOV     ZH,ADR1
        OUT     RAMPZ,ADR2
        LDI     GUARD,$A5
        LDI     DATA,(1<<PGERS)|(1<<SPMEN) ;page erase
        RCALL   DO_SPM

        INC     FF_FL
        BREQ    BLKFL1

        CLR     ZL
        MOV     ZH,ADR1
        OUT     RAMPZ,ADR2
        LDIY    BUFFER
        LDI     TEMP,$80
BLKFL2: LD      R0,Y+
        LD      R1,Y+
        LDI     DATA,(1<<SPMEN) ;transfer data from RAM to Flash page buffer
        RCALL   DO_SPM
        ADIW    ZL,2
        DEC     TEMP
        BRNE    BLKFL2

        CLR     ZL
        MOV     ZH,ADR1
        OUT     RAMPZ,ADR2
        LDI     DATA,(1<<PGWRT)|(1<<SPMEN)  ;execute page write
        RCALL   DO_SPM
BLKFL1:
        LDI     DATA,(1<<RWWSRE)|(1<<SPMEN) ;re-enable the RWW section
        RCALL   DO_SPM
        LDI     GUARD,$5A

        TST     FF_FL
        BREQ    INCADR

        LDIY    BUFFER
        CLR     ZL
        MOV     ZH,ADR1
        OUT     RAMPZ,ADR2
        CLR     R20
BLKFL3: ELPM    DATA,Z+
        LD      TEMP,Y+
        CP      DATA,TEMP
        BRNE    FLASH_ERROR
        DEC     R20
        BRNE    BLKFL3
;
;--------------------------------------
;INCADR
;out:   sreg.Z == SET - это был последний блок (выше по адресам обновлять запрещено!)
;chng:  TEMP
INCADR:
        ADD     ADR1,ONE
        ADC     ADR2,NULL
        MOV     TEMP,ADR2
        CPI     TEMP,HIGH(FLASHSIZE)
        BRNE    INCADR9
        MOV     TEMP,ADR1
        CPI     TEMP,LOW(FLASHSIZE)
INCADR9:RET
;
;--------------------------------------
;
FLASH_ERROR:
CRITICAL_ERROR:
        RJMP    START1
;
;--------------------------------------
;DO_SPM
;in:    DATA == значение для SPMCSR
DO_SPM: PUSH    TEMP
WAIT1SPM:                       ; check for previous SPM complete
        INPORT  TEMP,SPMCSR
        SBRC    TEMP,SPMEN
        RJMP    WAIT1SPM
        CLI
WAIT_EE:SBIC    EECR,EEWE       ; check that no EEPROM write access is present
        RJMP    WAIT_EE
        OUTPORT SPMCSR,DATA     ; SPM timed sequence
        CPI     GUARD,$A5       ;1
        BRNE    CRITICAL_ERROR  ;1
        SPM
        NOP
        NOP
        NOP
        NOP
WAIT2SPM:                       ; check for previous SPM complete
        INPORT  TEMP,SPMCSR
        SBRC    TEMP,SPMEN
        RJMP    WAIT2SPM
        POP     TEMP
        RET
;
;--------------------------------------
;
CMD00:  .DB     $40,$00,$00,$00,$00,$95
CMD08:  .DB     $48,$00,$00,$01,$AA,$87
CMD16:  .DB     $50,$00,$00,$02,$00,$FF
CMD55:  .DB     $77,$00,$00,$00,$00,$FF ;app_cmd
CMD58:  .DB     $7A,$00,$00,$00,$00,$FF ;read_ocr
CMD59:  .DB     $7B,$00,$00,$00,$00,$FF ;crc_on_off
;
FILENAME:       .DB     "ZXEVO_FWBIN",0
SIGNATURE:      .DB     "ZXEVO",$1A
;
.IFDEF  WHISTLES
MSG_TITLE:
.INCLUDE "EVOTITLE.INC"
MSG_WRONGDATA:
        .DB     $0D,$0A,$1B,"[1",$3B,"31mData is corrupt! Update is canceled.",0
MSG_SUCCESSFULLY:
        .DB     $0D,$0A,$1B,"[1",$3B,"37mUpdate is successfully.",$0D,$0A,$1B,"[33m",0
.ENDIF
;
PACKED_FPGA:
.NOLIST
.INCLUDE "FPGA.INC"
.LIST
;
;--------------------------------------
;
        .ORG    FLASHEND
        .DW     0       ;здесь должно быть CRC bootloader-а
BOOTLOADER_END:
