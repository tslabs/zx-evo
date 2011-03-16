;
;--------------------------------------
;
MSG_TSD_OUT:
        .DB     $0D,$0A,"out ",0,0
MSG_TSD_IN:
        .DB     ", in ",0

MSG_TSD_CMD:
        .DB     $0D,$0A,$3B,"CMD",0,0

MSG_TSD_ACMD41:
        .DB     $0D,$0A,$3B,"ACMD41",0

MSG_TSD_CSUP:
        .DB     $0D,$0A,"CS up",0
MSG_TSD_CSDOWN:
        .DB     $0D,$0A,"CS down",0

MSG_TSD_MMC:
        .DB     "MMC",0
MSG_TSD_SDV1:
        .DB     "SD v1",0
MSG_TSD_SDSC:
        .DB     "SD v2+ Standard Capacity",0,0
MSG_TSD_SDHC:
        .DB     "SD v2+ High Capacity",0,0

MSG_TSD_OCR:
        .DB     "OCR: ",0
MSG_TSD_CSD:
        .DB     "CSD: ",0
MSG_TSD_CID0:
        .DB     "CID: ",0
MSG_TSD_CID1:
        .DB     "Manufacturer ID    ",0
MSG_TSD_CID2:
        .DB     "OEM/Application ID ",0
MSG_TSD_CID3:
        .DB     "Product name       ",0
MSG_TSD_CID4:
        .DB     "Product revision   ",0
MSG_TSD_CID5:
        .DB     "Product serial #   ",0
MSG_TSD_CID6:
        .DB     "Manufacturing date ",0
MSG_TSD_CID6B:
        .DB     ".20",0
MSG_TSD_CID6C:
        .DB     ".19",0
MSG_TSD_CRC:
        .DB     "CRC=",0,0
MSG_TSD_READSECTOR:
        .DB     $0D,$0A,$3B,"Read sector ",0
MSG_TSD_SKIP:
        .DB     $0D,$0A,$3B,"512 operations is skiped",0

WIND_TSD1:
        .DB     10,10,32,4,$9F,$01
WIND_TSD2:
        .DB     0,2,53,22,$DF,$00
;
;--------------------------------------
;
.EQU    TSD_BLS0        =4
.EQU    TSD_BLS1        =5
.EQU    TSD_ARG_ACMD41  =17
.EQU    TSD_CARDTYPE    =18
.EQU    TSD_Y           =19
;
;--------------------------------------
;
TESTSD: GETMEM  20
;
T_SD00: CBR     FLAGS1,0B00000011
        SBR     FLAGS1,0B00000100

        CALL    SCR_FADE
        LDI     XL,0
        LDI     XH,1
        CALL    SCR_SET_CURSOR
        LDI     DATA,$20
        LDI     TEMP,$7F
        LDI     COUNT,53
        CALL    SCR_FILL_CHAR_ATTR

        LDIZ    WIND_TSD1*2
        CALL    WINDOW
        LDIZ    MLMSG_TSD_MENU*2
        CALL    SCR_PRINTMLSTR

T_SD02: STH     TSD_Y,NULL
T_SD04: RCALL   T_SD_PUTCURSOR
T_SD01: RCALL   T_SD_SENSORS
        CALL    INKEY
        BREQ    T_SD01
        CPI     DATA,KEY_UP
        BREQ    T_SD02
        CPI     DATA,KEY_DOWN
        BREQ    T_SD03
        CPI     DATA,KEY_ENTER
        BREQ    T_SD0E
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    T_SD01
        CPI     DATA,KEY_ESC
        BRNE    T_SD01
        CBR     FLAGS1,0B00001000
        FREEMEM 20
        RET
;
T_SD03: STH     TSD_Y,ONE
        RJMP    T_SD04
;
T_SD0E: LDH     DATA,TSD_Y
        TST     DATA
        BREQ    T_SD10
        LDI     DATA,0B00001000
        EOR     FLAGS1,DATA
        RJMP    T_SD04
; - - - - - - - - - - - - - - - - - - -
T_SD10:
        CBR     FLAGS1,0B00000101
        SBR     FLAGS1,0B00000010
        RCALL   T_SD_CRLF_DC
        RCALL   T_SD_CRLF_DC
        LDIZ    MSG_TITLE1*2
        CALL    PRINTSTRZ
        CALL    PRINT_SHORT_VERS
        RCALL   T_SD_CRLF_DC
        SBR     FLAGS1,0B00000110
        RCALL   T_SD_SENSORS

        LDIZ    WIND_TSD2*2
        CALL    WINDOW
        LDI     XL,1
        LDI     XH,3
        STH     TSD_Y,XH
        CALL    SCR_SET_CURSOR
        RCALL   T_SD_CRLF_DC
        LDIZ    MLMSG_TSD_INIT*2
        CALL    PRINTMLSTR

        STS     SD_CARDTYPE,NULL
        STH     TSD_CARDTYPE,NULL

        RCALL   T_SD_CSUP
        LDI     TEMP,SD_CS1
        SER     DATA
        CALL    FPGA_REG
        LDI     TEMP,32
        CALL    SD_RD_DUMMY

        RCALL   T_SD_CSDOWN
        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG
        SER     COUNT
T_SD11:
        LDI     DATA,0
        RCALL   T_SD_CMDXX
        LDIZ    CMD00*2     ;CMD0 (go_idle_state)
        CALL    SD_WR_PGM_6
        DEC     COUNT
        BRNE    T_SD12

        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_NOCARD*2
        CALL    PRINTMLSTR
        RJMP    T_SD90
T_SD12:
        CPI     DATA,$01
        BRNE    T_SD11

        LDI     DATA,8
        RCALL   T_SD_CMDXX
        LDIZ    CMD08*2     ;CMD8 (send_if_cond)
        CALL    SD_WR_PGM_6
        LDI     WL,$00
        SBRS    DATA,2
        LDI     WL,$40
        STH     TSD_ARG_ACMD41,WL
        LDI     TEMP,4
        CALL    SD_RD_DUMMY

T_SD13: RCALL   T_SD_ACMD41
        LDI     DATA,$40|55 ;CMD55
        CALL    SD_WR_CMD
        LDI     TEMP,2
        CALL    SD_RD_DUMMY
        LDI     DATA,$40|41 ;ACMD41 (sd_send_op_cond)
        CALL    SD_EXCHANGE
        LDH     DATA,TSD_ARG_ACMD41
        CALL    SD_EXCHANGE
        CALL    SD_WR_CMX4
        TST     DATA
        BREQ    T_SD15
        SBRS    DATA,2
        RJMP    T_SD13

T_SD14: LDI     DATA,1
        RCALL   T_SD_CMDXX
        LDI     DATA,$40|1  ;CMD1 (send_op_cond)
        CALL    SD_WR_CMD
        TST     DATA
        BRNE    T_SD14
        RCALL   T_SD_CRC_OFF
        RCALL   T_SD_SETBLKLEN
        LDI     TEMP,0B00010000
        LDIZ    MSG_TSD_MMC*2
        RJMP    T_SD18

T_SD15: RCALL   T_SD_CRC_OFF
        RCALL   T_SD_SETBLKLEN

        LDI     TEMP,0B00000001
        LDIZ    MSG_TSD_SDV1*2
        LDH     DATA,TSD_ARG_ACMD41
        TST     DATA
        BREQ    T_SD18
        LDI     DATA,58
        RCALL   T_SD_CMDXX
        LDI     DATA,$40|58 ;CMD58 (read_ocr)
        CALL    SD_WR_CMD
        MOVW    XL,YL
        LDI     COUNT,6
T_SD16: CALL    SD_RECEIVE
        ST      X+,DATA
        DEC     COUNT
        BRNE    T_SD16

        LDI     TEMP,0B00000010
        LDIZ    MSG_TSD_SDSC*2
        LDH     DATA,0
        SBRS    DATA,6
        RJMP    T_SD18
        LDI     TEMP,0B00000110
        LDIZ    MSG_TSD_SDHC*2

T_SD18: STS     SD_CARDTYPE,TEMP
        STH     TSD_CARDTYPE,TEMP
        PUSHZ
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_FOUNDCARD*2
        CALL    PRINTMLSTR
        POPZ
        CALL    PRINTSTRZ

        LDH     TEMP,TSD_CARDTYPE
        SBRS    TEMP,1
        RJMP    T_SD20
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_OCR*2
        CALL    PRINTSTRZ
        MOVW    XL,YL
        LDI     COUNT,4
T_SD19: LD      DATA,X+
        CALL    HEXBYTE_FOR_DUMP
        DEC     COUNT
        BRNE    T_SD19
;
T_SD20: LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG
        LDI     DATA,9
        RCALL   T_SD_CMDXX
        LDI     DATA,$40|9  ;CMD9 (send_csd)
        CALL    SD_WR_CMD
        TST     DATA
        BRNE    T_SD30
        CALL    SD_WAIT_NOTFF
        CPI     DATA,$FF
        BREQ    T_SD30
        MOVW    XL,YL
        LDI     COUNT,17
T_SD21: CALL    SD_RECEIVE
        ST      X+,DATA
        DEC     COUNT
        BRNE    T_SD21

        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CSD*2
        CALL    PRINTSTRZ
        MOVW    XL,YL
        LDI     COUNT,15
T_SD22: LD      DATA,X+
        CALL    HEXBYTE_FOR_DUMP
        DEC     COUNT
        BRNE    T_SD22

T_SD30: LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG
        LDI     DATA,10
        RCALL   T_SD_CMDXX
        LDI     DATA,$40|10 ;CMD10 (send_cid)
        CALL    SD_WR_CMD
        TST     DATA
        BRNE    T_SD39
        CALL    SD_WAIT_NOTFF
        CPI     DATA,$FF
        BRNE    T_SD31
T_SD39: RJMP    T_SD90
T_SD31: MOVW    XL,YL
        LDI     COUNT,17
T_SD32: CALL    SD_RECEIVE
        ST      X+,DATA
        DEC     COUNT
        BRNE    T_SD32

        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID0*2
        CALL    PRINTSTRZ
        MOVW    XL,YL
        LDI     COUNT,15
T_SD33: LD      DATA,X+
        CALL    HEXBYTE_FOR_DUMP
        DEC     COUNT
        BRNE    T_SD33

        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID1*2
        CALL    PRINTSTRZ
        LDH     DATA,0
        CALL    HEXBYTE
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID2*2
        CALL    PRINTSTRZ
        LDH     BITS,TSD_CARDTYPE
        LDH     DATA,1
        SBRS    BITS,4
        CALL    PUTCHAR_FOR_DUMP
        SBRC    BITS,4
        CALL    HEXBYTE
        LDH     DATA,2
        SBRS    BITS,4
        CALL    PUTCHAR_FOR_DUMP
        SBRC    BITS,4
        CALL    HEXBYTE
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID3*2
        CALL    PRINTSTRZ
        MOVW    XL,YL
        ADIW    XL,3
        LDI     COUNT,5
        SBRC    BITS,4
        LDI     COUNT,6
T_SD34: LD      DATA,X+
        CALL    PUTCHAR
        DEC     COUNT
        BRNE    T_SD34
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID4*2
        CALL    PRINTSTRZ
        LDH     DATA,8
        SBRC    BITS,4
        LDH     DATA,9
        SWAP    DATA
        CALL    HEXHALF
        LDI     DATA,$2E ;"."
        CALL    PUTCHAR
        LDH     DATA,8
        SBRC    BITS,4
        LDH     DATA,9
        CALL    HEXHALF
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID5*2
        CALL    PRINTSTRZ
        MOVW    XL,YL
        ADIW    XL,9
        SBRC    BITS,4
        ADIW    XL,1
        LDI     COUNT,4
T_SD35: LD      DATA,X+
        CALL    HEXBYTE
        DEC     COUNT
        BRNE    T_SD35
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CID6*2
        CALL    PRINTSTRZ
        LDH     DATA,14
        SBRC    BITS,4
        SWAP    DATA
        ANDI    DATA,$0F
        CALL    DECBYTE
        LDIZ    MSG_TSD_CID6B*2
        LDH     DATA,14
        SBRC    BITS,4
        RJMP    T_SD36
        LDH     TEMP,13
        ANDI    DATA,$F0
        ANDI    TEMP,$0F
        OR      DATA,TEMP
        SWAP    DATA
        PUSH    DATA
        CALL    PRINTSTRZ
        POP     DATA
        CALL    DECBYTE
        RJMP    T_SD38
T_SD36: ANDI    DATA,$0F
        SUBI    DATA,3
        BRCC    T_SD37
        ADDI    DATA,100
        LDIZ    MSG_TSD_CID6C*2
T_SD37: PUSH    DATA
        CALL    PRINTSTRZ
        POP     DATA
        CALL    DECBYTE
T_SD38:
; - - - - - - - - - - - - - - - - - - -
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_DETECT*2
        CALL    PRINTMLSTR

        IN      TEMP,SPL
        STS     GLB_STACK+0,TEMP
        IN      TEMP,SPH
        STS     GLB_STACK+1,TEMP
        STS     GLB_Y+0,YL
        STS     GLB_Y+1,YH
        LDIZ    T_SD_ERRORHANDL
        STSZ    FAT_ERRHANDLER
        LDI     TEMP,SD_CS0
        SER     DATA
        CALL    FPGA_REG
        CALL    WC_FAT

        PUSHX
        PUSHW
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_FOUNDFAT*2
        CALL    PRINTMLSTR
        LDS     TEMP,FAT_CAL_FAT
        LDI     DATA,$12
        TST     TEMP
        BREQ    T_SDF01
        LDI     DATA,$16
        DEC     TEMP
        BREQ    T_SDF01
        LDI     DATA,$32
T_SDF01:CALL    HEXBYTE
        POPW
        POPX
; - - - - - - - - - - - - - - - - - - -
;поиск файла в директории
        LDIW    0               ;номер описателя файла
        CALL    RDDIRSC
        RJMP    T_SD_FNDF_2
T_SD_FNDF_1:
        ADIW    WL,1            ;номерописателя++
        ADIW    ZL,$20          ;следующий описатель
        CPI     ZH,HIGH(BUF4FAT+512);
                                ;вылезли за сектор?
        BRNE    T_SD_FNDF_2     ;нет ещё
        CALL    RDDIRSC         ;считываем следующий
        BRNE    T_SD_FNDF_E     ;кончились сектора в директории
T_SD_FNDF_2:
        LD      TEMP,Z          ;первый символ
        CPI     TEMP,$E5        ;удалён?
        BREQ    T_SD_FNDF_1
        TST     TEMP            ;пустой описатель? (конец списка)
        BREQ    T_SD_FNDF_E
        LDD     DATA,Z+$0B      ;атрибуты
        ANDI    DATA,0B11011000
        BRNE    T_SD_FNDF_1
        PUSH    ZL
        MOVW    XL,ZL
        LDIZ    T_SD_FILENAME*2
T_SD_DALSHE:
        LPM     DATA,Z+
        TST     DATA
        BREQ    T_SD_NASHEL
        LD      TEMP,X+
        CP      DATA,TEMP
        BREQ    T_SD_DALSHE
        MOV     ZH,XH
        POP     ZL
        RJMP    T_SD_FNDF_1
T_SD_NASHEL:
        MOV     ZH,XH
        POP     ZL
        LDD     WL,Z+$1C
        LDD     WH,Z+$1D
        LDD     XL,Z+$1E
        LDD     XH,Z+$1F
        MOV     DATA,WL
        OR      DATA,WH
        OR      DATA,XL
        OR      DATA,XH
        BRNE    T_SD_FNDF_9     ;пустой файл
T_SD_FNDF_E:
        RJMP    T_SD90
T_SD_FILENAME:
        .DB     "TESTFILEBIN",0
T_SD_FNDF_9:
        LDI     TMP2,LOW(511)
        LDI     TMP3,HIGH(511)
        CALL    HLDEPBC
        STH     TSD_BLS0,WL
        STH     TSD_BLS1,WH
        CALL    BCDE200         ;кол-во секторов
        SBIW    WL,1
        SBC     XL,NULL
        SBC     XH,NULL
        LDS     DATA,FAT_BYTSSEC
        DEC     DATA
        AND     DATA,WL
        INC     DATA
        STS     FAT_MPHWOST,DATA    ;кол-во секторов в последнем кластере
        LDS     DATA,FAT_BYTSSEC
        CALL    BCDE_A
        STSW    FAT_KOL_CLS+0
        STSX    FAT_KOL_CLS+2
        STS     FAT_NUMSECK,NULL
        LDD     WL,Z+$1A
        LDD     WH,Z+$1B
        LDD     XL,Z+$14
        LDD     XH,Z+$15        ;номер первого кластера файла
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2
; - - - - - - - - - - - - - - - - - - -
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_READFILE*2
        CALL    PRINTMLSTR

        STS     NEWFRAME,NULL
        LDI     DATA,0B00000010
        MOV     INT6VECT,DATA
        LDI     TEMP,INT_CONTROL
        CALL    FPGA_REG

        CALL    CRC32_INIT
T_SD_RDFILE1:
        CALL    NEXTSEC
        TST     DATA
        BREQ    T_SD_RDFILE2
        LDIZ    BUFSECT
        LDIX    512
        CALL    RAM_CRC32_UPDATE
        LDS     DATA,NEWFRAME
        TST     DATA
        BREQ    T_SD_RDFILE1
        STS     NEWFRAME,NULL
        LDI     XL,30
        LDH     XH,TSD_Y
        STH     TSD_Y,XH
        CALL    SCR_SET_CURSOR
        PUSH    FLAGS1
        CBR     FLAGS1,0B00000011
        SBR     FLAGS1,0B00000100
        LDS     DATA,FAT_KOL_CLS+3
        CALL    HEXBYTE
        LDS     DATA,FAT_KOL_CLS+2
        CALL    HEXBYTE
        LDS     DATA,FAT_KOL_CLS+1
        CALL    HEXBYTE
        LDS     DATA,FAT_KOL_CLS+0
        CALL    HEXBYTE
        POP     FLAGS1
        RJMP    T_SD_RDFILE1

T_SD_RDFILE2:
        LDIZ    BUFSECT
        LDH     XL,TSD_BLS0
        LDH     XH,TSD_BLS1
        ANDI    XH,$01
        ADIW    XL,1
        CALL    RAM_CRC32_UPDATE
        CALL    CRC32_RELEASE
        CLR     INT6VECT
        CLR     DATA
        LDI     TEMP,INT_CONTROL
        CALL    FPGA_REG
        LDI     XL,30
        LDH     XH,TSD_Y
        STH     TSD_Y,XH
        CALL    SCR_SET_CURSOR
        LDI     DATA,$20
        LDI     COUNT,8
        CALL    SCR_FILL_CHAR
        RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MSG_TSD_CRC*2
        CALL    PRINTSTRZ
        LDH     DATA,3
        CALL    HEXBYTE
        LDH     DATA,2
        CALL    HEXBYTE
        LDH     DATA,1
        CALL    HEXBYTE
        LDH     DATA,0
        CALL    HEXBYTE

        RJMP    T_SD90
; - - - - - - - - - - - - - - - - - - -
T_SD_ERRORHANDL:
        CLI
        LDS     TEMP,GLB_STACK+0
        OUT     SPL,TEMP
        LDS     TEMP,GLB_STACK+1
        OUT     SPH,TEMP
        LDS     YL,GLB_Y+0
        LDS     YH,GLB_Y+1
        SEI
        PUSH    DATA
        RCALL   T_SD_SCR_CRLF_DC
        POP     DATA
        LDIZ    MLMSG_FL_SDERROR1*2
        CPI     DATA,1
        BREQ    T_SD_ERRHNDL1
        LDIZ    MLMSG_FL_SDERROR2*2
        CPI     DATA,2
        BREQ    T_SD_ERRHNDL1
        LDIZ    MLMSG_FL_SDERROR3*2
        CPI     DATA,3
        BREQ    T_SD_ERRHNDL1
        LDIZ    MLMSG_FL_SDERROR4*2
        CPI     DATA,4
        BREQ    T_SD_ERRHNDL1
        LDIZ    MLMSG_FL_SDERRORX*2
T_SD_ERRHNDL1:
        CALL    PRINTMLSTR
; - - - - - - - - - - - - - - - - - - -
T_SD90: RCALL   T_SD_SCR_CRLF_DC
        LDIZ    MLMSG_TSD_COMPLETE*2
        CALL    PRINTMLSTR
        CBR     FLAGS1,0B00000011
        SBR     FLAGS1,0B00000100
T_SD99: RCALL   T_SD_SENSORS
        CALL    INKEY
        BREQ    T_SD99
        RJMP    T_SD00
;
;--------------------------------------
;
T_SD_PUTCURSOR:
        CLR     COUNT
TSD_PC2:MOV     XH,COUNT
        ADDI    XH,11
        LDI     XL,11
        CALL    SCR_SET_CURSOR
        PUSH    COUNT
        LDH     DATA,TSD_Y
        LDI     TEMP,$9F
        CP      COUNT,DATA
        BRNE    TSD_PC1
        LDI     TEMP,$F0
TSD_PC1:LDI     COUNT,30
        CALL    SCR_FILL_ATTR
        POP     COUNT
        INC     COUNT
        CPI     COUNT,2
        BRNE    TSD_PC2

        LDI     XL,13
        LDI     XH,12
        CALL    SCR_SET_CURSOR
        LDI     DATA,$20 ;" "
        SBRC    FLAGS1,3
        LDI     DATA,$FB ;"√"
        CALL    SCR_PUTCHAR
        RET
;
;--------------------------------------
;
T_SD_SETBLKLEN:
        LDI     DATA,16
        RCALL   T_SD_CMDXX
        LDIZ    CMD16*2     ;CMD16 (set_blocklen)
        CALL    SD_WR_PGM_6
        TST     DATA
        BRNE    T_SD_SETBLKLEN
        RET
;
;--------------------------------------
;
T_SD_CRC_OFF:
        LDI     DATA,59
        RCALL   T_SD_CMDXX
        LDI     DATA,$40|59 ;CMD59 (crc_on_off)
        CALL    SD_WR_CMD
        TST     DATA
        BRNE    T_SD_CRC_OFF
        RET
;
;--------------------------------------
;
T_SD_CMDXX:
        SBRS    FLAGS1,3
        RET
        PUSH    FLAGS1
        CBR     FLAGS1,0B00000101
        SBR     FLAGS1,0B00000010
        PUSH    DATA
        LDIZ    MSG_TSD_CMD*2
        CALL    PRINTSTRZ
        POP     DATA
        CALL    DECBYTE
        POP     FLAGS1
        RET
;
;--------------------------------------
;
T_SD_ACMD41:
        SBRS    FLAGS1,3
        RET
        PUSH    FLAGS1
        CBR     FLAGS1,0B00000101
        SBR     FLAGS1,0B00000010
        LDIZ    MSG_TSD_ACMD41*2
        CALL    PRINTSTRZ
        POP     FLAGS1
        RET
;
;--------------------------------------
;
T_SD_CSUP:
        SBRS    FLAGS1,3
        RET
        PUSH    FLAGS1
        CBR     FLAGS1,0B00000101
        SBR     FLAGS1,0B00000010
        LDIZ    MSG_TSD_CSUP*2
        CALL    PRINTSTRZ
        POP     FLAGS1
        RET
;
;--------------------------------------
;
T_SD_CSDOWN:
        SBRS    FLAGS1,3
        RET
        PUSH    FLAGS1
        CBR     FLAGS1,0B00000101
        SBR     FLAGS1,0B00000010
        LDIZ    MSG_TSD_CSDOWN*2
        CALL    PRINTSTRZ
        POP     FLAGS1
        RET
;
;--------------------------------------
;
T_SD_CRLF_DC:
        RCALL   UART_CRLF
        SBRS    FLAGS1,3
        RET
        LDI     DATA,$3B ;";"
        RJMP    UART_PUTCHAR
;
;--------------------------------------
;
T_SD_SCR_CRLF_DC:
        LDI     XL,1
        LDH     XH,TSD_Y
        INC     XH
        STH     TSD_Y,XH
        CALL    SCR_SET_CURSOR
        RCALL   UART_CRLF
        SBRS    FLAGS1,3
        RET
        LDI     DATA,$3B ;";"
        RJMP    UART_PUTCHAR
;
;--------------------------------------
;
T_SD_SENSORS:
        LDI     XL,0
        LDI     XH,1
        CALL    SCR_SET_CURSOR
        LDI     TEMP,$7F
        CALL    SCR_SET_ATTR
        LDIZ    MLMSG_SENSORS*2
        CALL    PRINTMLSTR
        LDI     TEMP,$7A
        LDIZ    MLMSG_S_NOCARD*2
        SBIC    PINB,5
        RJMP    T_SDSENS1
        LDI     TEMP,$7C
        LDIZ    MLMSG_S_INSERTED*2
T_SDSENS1:
        CALL    SCR_SET_ATTR
        CALL    PRINTMLSTR
        LDI     TEMP,$7A
        LDIZ    MLMSG_S_READONLY*2
        SBIC    PINB,4
        RJMP    T_SDSENS2
        LDI     TEMP,$7C
        LDIZ    MLMSG_S_WRITEEN*2
T_SDSENS2:
        CALL    SCR_SET_ATTR
        CALL    PRINTMLSTR
        RET
;
;--------------------------------------
;
