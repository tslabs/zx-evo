;
.DSEG
SD_CARDTYPE:    .BYTE   1       ;.0-SDv1; .1-SDv2; .2-SDv2HC; .4-MMC
;       на данный момент имеет значение только это ^^^^^^^^^
;
FAT_CAL_FAT:    .BYTE   1       ;тип обнаруженной FAT
FAT_MANYFAT:    .BYTE   1       ;количество FAT-таблиц
FAT_BYTSSEC:    .BYTE   1       ;количество секторов в кластере
FAT_ROOTCLS:    .BYTE   4       ;КЛАСТЕР начала root директории
FAT_SEC_FAT:    .BYTE   4       ;количество секторов одной FAT
FAT_RSVDSEC:    .BYTE   2       ;размер резервной области
FAT_STARTRZ:    .BYTE   4       ;начало диска/раздела
FAT_FRSTDAT:    .BYTE   4       ;адрес первого сектора данных от BPB
FAT_SEC_DSC:    .BYTE   4       ;количество секторов на диске/разделе
FAT_CLS_DSC:    .BYTE   4       ;количество кластеров на диске/разделе
FAT_FATSTR0:    .BYTE   4       ;начало первой FAT таблицы
FAT_FATSTR1:    .BYTE   4       ;начало второй FAT таблицы
FAT_TEK_DIR:    .BYTE   4       ;кластер текущей директории
FAT_KCLSDIR:    .BYTE   1       ;кол-во кластеров директории
FAT_NUMSECK:    .BYTE   1       ;счетчик секторов в кластере
FAT_TFILCLS:    .BYTE   4       ;текущий кластер
FAT_MPHWOST:    .BYTE   1       ;кол-во секторов в последнем кластере
FAT_KOL_CLS:    .BYTE   4       ;кол-во кластеров файла минус 1
FAT_LASTSECFLAG:.BYTE   1
FAT_ERRHANDLER: .BYTE   2
FAT_LST0ZAP:    .BYTE   4
.CSEG
;
CMD00:  .DB     $40,$00,$00,$00,$00,$95
CMD08:  .DB     $48,$00,$00,$01,$AA,$87
CMD16:  .DB     $50,$00,$00,$02,$00,$FF
;
;инициализация SD карточки
;out:   XW == номер кластера root-директории
SD_FAT_INIT:
        STSZ    FAT_ERRHANDLER

        LDI     TEMP,SD_CS1
        SER     DATA
        CALL    FPGA_REG
        LDI     TEMP,32
        RCALL   SD_RD_DUMMY

        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG
        SER     COUNT
SDINIT1:LDIZ    CMD00*2     ;CMD0 (go_idle_state)
        RCALL   SD_WR_PGM_6
        DEC     COUNT
        BRNE    SDINIT2
        LDI     DATA,1  ;нет карты
        RJMP    SD_ERROR
SDINIT2:CPI     DATA,$01
        BRNE    SDINIT1

        LDIZ    CMD08*2     ;CMD8 (send_if_cond)
        RCALL   SD_WR_PGM_6
        LDI     WL,$00
        SBRS    DATA,2
        LDI     WL,$40
        LDI     TEMP,4
        RCALL   SD_RD_DUMMY

SDINIT3:LDI     DATA,$40+55 ;CMD55
        RCALL   SD_WR_CMD
        LDI     TEMP,2
        RCALL   SD_RD_DUMMY
        LDI     DATA,$40+41 ;ACMD41 (sd_send_op_cond)
        RCALL   SD_EXCHANGE
        MOV     DATA,WL
        RCALL   SD_EXCHANGE
        RCALL   SD_WR_CMX4
        TST     DATA
        BREQ    SDINIT5
        SBRS    DATA,2
        RJMP    SDINIT3

SDINIT4:LDI     DATA,$40+1  ;CMD1 (send_op_cond)
        RCALL   SD_WR_CMD
        TST     DATA
        BRNE    SDINIT4
        RCALL   SD_CRC_OFF
        RCALL   SD_SETBLKLEN
        LDI     TEMP,0B00010000
        RJMP    SDINIT9

SDINIT5:RCALL   SD_CRC_OFF
        RCALL   SD_SETBLKLEN

        LDI     TEMP,0B00000001
        TST     WL
        BREQ    SDINIT9
        LDI     DATA,$40+58 ;CMD58 (read_ocr)
        RCALL   SD_WR_CMD
        RCALL   SD_RECEIVE
        PUSH    DATA
        LDI     TEMP,3+2
        RCALL   SD_RD_DUMMY
        POP     DATA
        LDI     TEMP,0B00000010
        SBRC    DATA,6
        LDI     TEMP,0B00000110
SDINIT9:STS     SD_CARDTYPE,TEMP
;
; - - - - - - - - - - - - - - - - - - -
;поиск FAT, инициализация переменных
WC_FAT: LDIZ    FAT_LST0ZAP
        ST      Z+,FF
        ST      Z+,FF
        ST      Z+,FF
        ST      Z,FF
        LDIW    0
        LDIX    0
        RCALL   LOADLST
        RCALL   FAT_CHKSIGN
        BREQ    WC_FAT1
SDERR3: LDI     DATA,3  ;не найдена FAT
        RJMP    SD_ERROR
WC_FAT1:LDI     ZL,$BE
        LD      DATA,Z
        ANDI    DATA,$7F
        BRNE    RDFAT05
        LDI     ZL,$C2
        LD      DATA,Z
        CPI     DATA,$01
        BREQ    RDFAT06
        CPI     DATA,$04
        BREQ    RDFAT06
        CPI     DATA,$06
        BREQ    RDFAT06
        CPI     DATA,$0B
        BREQ    RDFAT06
        CPI     DATA,$0C
        BREQ    RDFAT06
        CPI     DATA,$0E
        BRNE    RDFAT05
RDFAT06:LDI     ZL,$C6
        LD      WL,Z+
        LD      WH,Z+
        LD      XL,Z+
        LD      XH,Z
        RJMP    RDFAT00
RDFAT05:LDIZ    BUF4FAT
        LDD     BITS,Z+13       ;BPB_SecPerClus
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
RDF052: LDD     DATA,Z+14       ;BPB_RsvdSecCnt
        LDD     R0,Z+15         ;BPB_RsvdSecCnt
        OR      DATA,R0
        BREQ    RDF053
        INC     TEMP
RDF053: LDD     DATA,Z+19       ;BPB_TotSec16
        LDD     R0,Z+20         ;BPB_TotSec16
        OR      DATA,R0
        BRNE    RDF054
        INC     TEMP
RDF054: LDD     DATA,Z+32       ;BPB_TotSec32
        LDD     R0,Z+33         ;BPB_TotSec32
        OR      DATA,R0
        LDD     R0,Z+34         ;BPB_TotSec32
        OR      DATA,R0
        LDD     R0,Z+35         ;BPB_TotSec32
        OR      DATA,R0
        BRNE    RDF055
        INC     TEMP
RDF055: LDD     DATA,Z+21       ;BPB_Media
        ANDI    DATA,$F0
        CPI     DATA,$F0
        BRNE    RDF056
        INC     TEMP
RDF056: CPI     TEMP,4
        BREQ    RDF057
        RJMP    SDERR3
;
RDF057: LDIW    0
        LDIX    0
RDFAT00:STSW    FAT_STARTRZ+0
        STSX    FAT_STARTRZ+2
        RCALL   LOADLST
        RCALL   FAT_CHKSIGN
        BREQ    RDF011
        RJMP    SDERR3
RDF011: LDIZ    BUF4FAT
        LDD     DATA,Z+11       ;BPB_BytsPerSec
        TST     DATA
        BRNE    SDERR4
        LDD     DATA,Z+12       ;BPB_BytsPerSec
        CPI     DATA,$02
        BREQ    RDF012
SDERR4: LDI     DATA,4  ;ошибка FAT (сектор не 512 байт)
        RJMP    SD_ERROR
RDF012: LDIX    0
        LDD     WL,Z+22         ;BPB_FATSz16
        LDD     WH,Z+23         ;BPB_FATSz16
        MOV     DATA,WH
        OR      DATA,WL
        BRNE    RDFAT01         ;если BPB_FATSz16==0 то берем BPB_FATSz32
        LDD     WL,Z+36         ;BPB_FATSz32
        LDD     WH,Z+37         ;BPB_FATSz32
        LDD     XL,Z+38         ;BPB_FATSz32
        LDD     XH,Z+39         ;BPB_FATSz32
RDFAT01:STSW    FAT_SEC_FAT+0
        STSX    FAT_SEC_FAT+2   ;число секторов на FAT-таблицу
        LDIX    0
        LDD     WL,Z+19         ;BPB_TotSec16
        LDD     WH,Z+20         ;BPB_TotSec16
        MOV     DATA,WH
        OR      DATA,WL
        BRNE    RDFAT02         ;если BPB_TotSec16==0 то берем из BPB_TotSec32
        LDD     WL,Z+32         ;BPB_TotSec32
        LDD     WH,Z+33         ;BPB_TotSec32
        LDD     XL,Z+34         ;BPB_TotSec32
        LDD     XH,Z+35         ;BPB_TotSec32
RDFAT02:STSW    FAT_SEC_DSC+0
        STSX    FAT_SEC_DSC+2   ;к-во секторов на диске/разделе
;вычисляем rootdirsectors
        LDD     WL,Z+17         ;BPB_RootEntCnt
        LDD     WH,Z+18         ;BPB_RootEntCnt
        LDIX    0
        MOV     DATA,WH
        OR      DATA,WL         ;в FAT32 поле BPB_RootEntCnt всегда 0
        BREQ    RDFAT03         ;поэтому для FAT32 RootDirSectors всегда 0
        LDI     DATA,$10
        RCALL   BCDE_A
        MOVW    XL,WL
RDFAT03:PUSH    XH              ;RootDirSectors в X
        PUSH    XL
        LDD     DATA,Z+16       ;BPB_NumFATs
        STS     FAT_MANYFAT,DATA
        LDSW    FAT_SEC_FAT+0
        LDSX    FAT_SEC_FAT+2
        DEC     DATA
RDF031: LSL     WL
        ROL     WH
        ROL     XL
        ROL     XH
        DEC     DATA
        BRNE    RDF031
        POP     TMP2
        POP     TMP3
                                ;полный размер FAT-области в секторах
        RCALL   HLDEPBC         ;прибавили RootDirSectors
        LDD     TMP2,Z+14       ;BPB_RsvdSecCnt
        LDD     TMP3,Z+15       ;BPB_RsvdSecCnt
        STS     FAT_RSVDSEC+0,TMP2
        STS     FAT_RSVDSEC+1,TMP3
        RCALL   HLDEPBC         ;прибавили BPB_RsvdSecCnt
        STSW    FAT_FRSTDAT+0
        STSX    FAT_FRSTDAT+2   ;положили номер первого сектора данных
        LDIZ    FAT_SEC_DSC
        RCALL   BCDEHLM         ;вычли из полного к-ва секторов раздела
        LDIZ    BUF4FAT
        LDD     DATA,Z+13       ;BPB_SecPerClus
        CPI     DATA,65
        BRCS    RDF032
        RJMP    SDERR4  ;ошибка FAT (кластер>32Kb, ограничения флешера)
RDF032: STS     FAT_BYTSSEC,DATA
        RCALL   BCDE_A          ;разделили на к-во секторов в кластере
        STSW    FAT_CLS_DSC+0
        STSX    FAT_CLS_DSC+2   ;положили кол-во кластеров на разделе

;microsoft-recommended FAT type determination (FAT12 <4085; FAT16 <65525; else FAT32)
        LDI     DATA,2
        TST     XH
        BRNE    RDFAT04
        TST     XL
        BRNE    RDFAT04
        CPI     WL,$F5
        CPC     WH,FF
        BRCC    RDFAT04
        LDI     DATA,1
        LDI     TEMP,$0F
        CPI     WL,$F5
        CPC     WH,TEMP
        BRCC    RDFAT04
        LDI     DATA,0
;alternative FAT type determination
;        PUSHX
;        PUSHW
;        LSL     WL
;        ROL     WH
;        ROL     XL
;        ROL     XH
;        RCALL   RASCHET
;        LDI     DATA,1
;        POPW
;        POPX
;        BREQ    RDFAT04
;        LSL     WL
;        ROL     WH
;        ROL     XL
;        ROL     XH
;        LSL     WL
;        ROL     WH
;        ROL     XL
;        ROL     XH
;        RCALL   RASCHET
;        LDI     DATA,2
;        BREQ    RDFAT04
;        CLR     DATA
RDFAT04:STS     FAT_CAL_FAT,DATA

;для FAT12/16 вычисляем адрес первого сектора директории
;для FAT32 берем по смещемию +44
;на выходе XW == кластер rootdir
        LDIW    0
        LDIX    0
        TST     DATA
        BREQ    FSRROO2
        DEC     DATA
        BREQ    FSRROO2
        LDD     WL,Z+44
        LDD     WH,Z+45
        LDD     XL,Z+46
        LDD     XH,Z+47
FSRROO2:STSW    FAT_ROOTCLS+0
        STSX    FAT_ROOTCLS+2   ;КЛАСТЕР root директории
        STSW    FAT_TEK_DIR+0
        STSX    FAT_TEK_DIR+2

        LDSW    FAT_RSVDSEC
        LDIX    0
        LDIZ    FAT_STARTRZ
        RCALL   BCDEHLP
        STSW    FAT_FATSTR0+0
        STSX    FAT_FATSTR0+2
        LDIZ    FAT_SEC_FAT
        RCALL   BCDEHLP
        STSW    FAT_FATSTR1+0
        STSX    FAT_FATSTR1+2

        LDSW    FAT_ROOTCLS+0
        LDSX    FAT_ROOTCLS+2
;
;--------------------------------------
;
CALCKCLSDIR:
        PUSHW
        PUSHX
        LDI     TEMP,1
        MOV     R0,WL
        OR      R0,WH
        OR      R0,XL
        OR      R0,XH
        BREQ    LASTCLS
NEXTCLS:PUSH    TEMP
        RCALL   RDFATZP
        RCALL   LST_CLS
        POP     TEMP
        BRCC    LASTCLS
        INC     TEMP
        RJMP    NEXTCLS
LASTCLS:STS     FAT_KCLSDIR,TEMP
        POPX
        POPW
        RET
;
;--------------------------------------
;
SD_CRC_OFF:
        LDI     DATA,$40+59 ;CMD59 (crc_on_off)
        RCALL   SD_WR_CMD
        TST     DATA
        BRNE    SD_CRC_OFF
        RET
;
;--------------------------------------
;
SD_SETBLKLEN:
        LDIZ    CMD16*2     ;CMD16 (set_blocklen)
        RCALL   SD_WR_PGM_6
        TST     DATA
        BRNE    SD_SETBLKLEN
        RET
;
;--------------------------------------
;out:   sreg.Z  SET==signature exist
;       Z==BUF4FAT+$01FF
FAT_CHKSIGN:
        LDIZ    BUF4FAT+$01FE
        LD      DATA,Z+
        CPI     DATA,$55
        LD      DATA,Z
        BRNE    FAT_CHKSIG9
        CPI     DATA,$AA
FAT_CHKSIG9:
        RET
;
;--------------------------------------
;чтение сектора данных
LOAD_DATA:
        SBRS    FLAGS1,3
        RJMP    LOAD_DAT1
        PUSH    FLAGS1
        CBR     FLAGS1,0B00001101
        SBR     FLAGS1,0B00000010
        LDIZ    MSG_TSD_READSECTOR*2
        CALL    PRINTSTRZ
        MOV     DATA,XH
        CALL    HEXBYTE
        MOV     DATA,XL
        CALL    HEXBYTE
        MOV     DATA,WH
        CALL    HEXBYTE
        MOV     DATA,WL
        CALL    HEXBYTE
        POP     FLAGS1
LOAD_DAT1:
        LDIZ    BUFSECT
        RCALL   SD_READ_SECTOR  ;читать один сектор
        BREQ    SDERR2
;        SBRS    FLAGS1,3
        RET
;        LDIZ    BUFSECT
;        CALL    UART_DUMP512
;        RET
;
;--------------------------------------
SDERR2: LDI     DATA,2  ;ошибка при чтении сектора
SD_ERROR:
        LDSZ    FAT_ERRHANDLER
        IJMP
;
;--------------------------------------
;чтение сектора служ.инф. (FAT/DIR/...)
LOADLST:
        LDIZ    FAT_LST0ZAP
        LD      DATA,Z+
        CP      DATA,WL
        BRNE    LOADLST2
        LD      DATA,Z+
        CP      DATA,WH
        BRNE    LOADLST2
        LD      DATA,Z+
        CP      DATA,XL
        BRNE    LOADLST2
        LD      DATA,Z+
        CP      DATA,XH
        BREQ    LOADLST9
LOADLST2:
        STSW    FAT_LST0ZAP+0
        STSX    FAT_LST0ZAP+2
        SBRS    FLAGS1,3
        RJMP    LOADLST1
        PUSH    FLAGS1
        CBR     FLAGS1,0B00001101
        SBR     FLAGS1,0B00000010
        LDIZ    MSG_TSD_READSECTOR*2
        CALL    PRINTSTRZ
        MOV     DATA,XH
        CALL    HEXBYTE
        MOV     DATA,XL
        CALL    HEXBYTE
        MOV     DATA,WH
        CALL    HEXBYTE
        MOV     DATA,WL
        CALL    HEXBYTE
        POP     FLAGS1
LOADLST1:
        LDIZ    BUF4FAT
        RCALL   SD_READ_SECTOR  ;читать один сектор
        BREQ    SDERR2
        LDIZ    BUF4FAT
        SBRS    FLAGS1,3
        RET
        CALL    UART_DUMP512
LOADLST9:
        LDIZ    BUF4FAT
        RET
;
;--------------------------------------
;чтение сектора dir по номеру описателя (W)
;на выходе: DATA=#ff (sreg.Z=0) выход за пределы dir
RDDIRSC:PUSHW
        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG

        LDIX    0
        LDI     DATA,$10
        RCALL   BCDE_A
        PUSH    WL
        LDS     DATA,FAT_BYTSSEC
        PUSH    DATA
        RCALL   BCDE_A
        LDS     DATA,FAT_KCLSDIR
        DEC     DATA
        CP      DATA,WL
        BRCC    RDDIRS3
        POP     DATA
        POP     DATA
        POPW
        SER     DATA
        TST     DATA
        RET
RDDIRS3:LDSX    FAT_TEK_DIR+2
        MOV     DATA,WL
        TST     DATA
        LDSW    FAT_TEK_DIR+0
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
        ADD     WL,DATA
        ADC     WH,NULL
        ADC     XL,NULL
        ADC     XH,NULL
        RCALL   LOADLST
        POPW
        CLR     DATA
        RET
;
;--------------------------------------
;out:   sreg.C == CLR - EOCmark
;(chng: TEMP)
LST_CLS:LDI     TEMP,$0F
        LDS     DATA,FAT_CAL_FAT
        TST     DATA
        BRNE    LST_CL1
        CPI     WL,$F7
        CPC     WH,TEMP
        RET
LST_CL1:DEC     DATA
        BRNE    LST_CL2
        CPI     WL,$F7
        CPC     WH,FF
        RET
LST_CL2:CPI     WL,$F7
        CPC     WH,FF
        CPC     XL,FF
        CPC     XH,TEMP
        RET
;
;--------------------------------------
;
RDFATZP:
        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG

        LDS     DATA,FAT_CAL_FAT
        TST     DATA
        BREQ    RDFATS0         ;FAT12
        DEC     DATA
        BREQ    RDFATS1         ;FAT16
;FAT32
        LSL     WL
        ROL     WH
        ROL     XL
        ROL     XH
        MOV     DATA,WL
        MOV     WL,WH
        MOV     WH,XL
        MOV     XL,XH
        CLR     XH
        RCALL   RDFATS2
        ADIW    ZL,1
        LD      XL,Z+
        LD      XH,Z
        ANDI    XH,$0F  ;
        RET
;FAT16
RDFATS1:LDIX    0
        MOV     DATA,WL
        MOV     WL,WH
        CLR     WH
RDFATS2:PUSH    DATA
        PUSHX
        LDIZ    FAT_FATSTR0
        RCALL   BCDEHLP
        RCALL   LOADLST
        POPX
        POP     DATA
        ADD     ZL,DATA
        ADC     ZH,NULL
        ADD     ZL,DATA
        ADC     ZH,NULL
        LD      WL,Z+
        LD      WH,Z
        RET
;FAT12
RDFATS0:MOVW    ZL,WL
        LSL     ZL
        ROL     ZH
        ADD     ZL,WL
        ADC     ZH,WH
        LSR     ZH
        ROR     ZL
        MOV     DATA,WL
        MOV     WL,ZH
        CLR     WH
        CLR     XL
        CLR     XH
        LSR     WL
        PUSH    DATA
        PUSHZ
        LDIZ    FAT_FATSTR0
        RCALL   BCDEHLP
        PUSHW
        RCALL   LOADLST
        POPW
        POPX
        ANDI    XH,$01
        ADD     ZL,XL
        ADC     ZH,XH
        LD      XL,Z+
        CPI     ZH,HIGH(BUF4FAT+512)
        BRNE    RDFATS4
        PUSH    XL
        LDIX    0
        ADIW    WL,1
        RCALL   LOADLST
        POP     XL
RDFATS4:POP     DATA
        LD      WH,Z
        MOV     WL,XL
        LDIX    0
        LSR     DATA
        BRCC    RDFATS3
        LSR     WH
        ROR     WL
        LSR     WH
        ROR     WL
        LSR     WH
        ROR     WL
        LSR     WH
        ROR     WL
RDFATS3:ANDI    WH,$0F
        RET
;
;--------------------------------------
;вычисление реального сектора
;на входе XW==номер FAT
;на выходе XW==адрес сектора
REALSEC:MOV     DATA,XH
        OR      DATA,XL
        OR      DATA,WH
        OR      DATA,WL
        BRNE    REALSE1
        LDIZ    FAT_FATSTR1
        LDSW    FAT_SEC_FAT+0
        LDSX    FAT_SEC_FAT+2
        RJMP    BCDEHLP
REALSE1:SBIW    WL,2            ;номер кластера-2
        SBC     XL,NULL
        SBC     XH,NULL
        LDS     DATA,FAT_BYTSSEC
        RJMP    REALSE2
REALSE3:LSL     WL
        ROL     WH
        ROL     XL
        ROL     XH
REALSE2:LSR     DATA
        BRCC    REALSE3
                                ;умножили на размер кластера
        LDIZ    FAT_STARTRZ
        RCALL   BCDEHLP         ;прибавили смещение от начала диска
        LDIZ    FAT_FRSTDAT
        RJMP    BCDEHLP         ;прибавили смещение от начала раздела
;
;--------------------------------------
;XW>>9 (деление на 512)
BCDE200:MOV     WL,WH
        MOV     WH,XL
        MOV     XL,XH
        LDI     XH,0
        LDI     DATA,1
; - - - - - - - - - - - - - - - - - - -
;XWDATA>>до"переноса"
;если в DATA вкл.только один бит, то получается
;XW=XW/DATA
BCDE_A1:LSR     XH
        ROR     XL
        ROR     WH
        ROR     WL
BCDE_A: ROR     DATA
        BRCC    BCDE_A1
        RET
;
;--------------------------------------
;XW=[Z]-XW
BCDEHLM:LD      DATA,Z+
        SUB     DATA,WL
        MOV     WL,DATA
        LD      DATA,Z+
        SBC     DATA,WH
        MOV     WH,DATA
        LD      DATA,Z+
        SBC     DATA,XL
        MOV     XL,DATA
        LD      DATA,Z
        SBC     DATA,XH
        MOV     XH,DATA
        RET
;
;--------------------------------------
;XW=XW+[Z]
BCDEHLP:LD      DATA,Z+
        ADD     WL,DATA
        LD      DATA,Z+
        ADC     WH,DATA
        LD      DATA,Z+
        ADC     XL,DATA
        LD      DATA,Z
        ADC     XH,DATA
        RET
;
;--------------------------------------
;XW=XW+TMP3TMP2
HLDEPBC:ADD     WL,TMP2
        ADC     WH,TMP3
        ADC     XL,NULL
        ADC     XH,NULL
        RET
;
;--------------------------------------
;
RASCHET:RCALL   BCDE200
        LDIZ    FAT_SEC_FAT
        RCALL   BCDEHLM
        MOV     DATA,WL
        ANDI    DATA,$F0
        OR      DATA,WH
        OR      DATA,XL
        OR      DATA,XH
        RET
;
;--------------------------------------
;чтение очередного сектора файла в BUFFER
;out:   DATA == 0 - считан последний сектор файла
NEXTSEC:
        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG

        LDIZ    FAT_KOL_CLS
        LD      DATA,Z+
        LD      TEMP,Z+
        OR      DATA,TEMP
        LD      TEMP,Z+
        OR      DATA,TEMP
        LD      TEMP,Z+
        OR      DATA,TEMP
        BREQ    LSTCLSF
        LDSW    FAT_TFILCLS+0
        LDSX    FAT_TFILCLS+2
        RCALL   REALSEC
        LDS     DATA,FAT_NUMSECK
        ADD     WL,DATA
        ADC     WH,NULL
        ADC     XL,NULL
        ADC     XH,NULL
        RCALL   LOAD_DATA
        LDSW    FAT_TFILCLS+0
        LDSX    FAT_TFILCLS+2
        LDS     DATA,FAT_NUMSECK
        INC     DATA
        STS     FAT_NUMSECK,DATA
        LDS     TEMP,FAT_BYTSSEC
        CP      TEMP,DATA
        BRNE    NEXT_OK

        STS     FAT_NUMSECK,NULL
        RCALL   RDFATZP
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2
        LDIZ    FAT_KOL_CLS
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

LSTCLSF:LDSW    FAT_TFILCLS+0
        LDSX    FAT_TFILCLS+2
        RCALL   REALSEC
        LDS     DATA,FAT_NUMSECK
        ADD     WL,DATA
        ADC     WH,NULL
        ADC     XL,NULL
        ADC     XH,NULL
        RCALL   LOAD_DATA
        LDS     DATA,FAT_NUMSECK
        INC     DATA
        STS     FAT_NUMSECK,DATA
        LDS     TEMP,FAT_MPHWOST
        SUB     DATA,TEMP
        RET
;
;--------------------------------------
;
