;01234567890123456789012345678901234567890123456789012
;            ZX Evolution Service [101005]            00
;┌───────────────┐┌──────────────────────────────────┐01    ┌───────────────┐
;│ Exit          ││..          │ <DIR>│31.12.09│23:58│02    │ Выход         │
;│ Retrieve all  ││НОВАЯП~1    │ <DIR>│31.12.09│23:58│03    │ Всё снова     │
;│ Erase chip    ││NEWFOL~1    │ <DIR>│31.12.09│23:58│04    │ Стереть м/сх. │
;│ Add job       ││testram  rom│  2048│31.12.09│23:58│05    │ Добав.задание │
;│ Execute jobs  ││filename rom│524288│31.12.09│23:58│06    │ Выполнить     │
;└───────────────┘│zxevo    rom│ 65536│31.12.09│23:58│07    └───────────────┘
;┌───────────────┐│trdos503 rom│ 16384│31.12.09│23:58│08    ┌───────────────┐
;│ Chip: M29F040 ││bigfile  bin│3214 K│31.12.09│23:58│09    │ ChipID: 20 E2 │
;│ CRC: 12345678 ││verybig  bin│4095 M│31.12.09│23:58│10    │ CRC: 12345678 │
;│ SDcard: FAT32 ││some     rom│   123│31.12.09│23:58│11    │  No SD-card!  │
;│ Erase...      ││onemore  bin│  9876│31.12.09│23:58│12    │ Verify...     │
;└───────────────┘└──────────────────────────────────┘13    └───────────────┘
;┌────────────────── [√] Erase chip ─────────────────┐14
;│gluk     rom trdos610 rom basic128 rom basic48  rom│15     секторов(1)  нач.кластер(4)  имя(8+3)  |  итого на ячейку 16
;│80  12345678 ............ ............ ............│16                                            |  итого на всё   512
;│............ ............ ............ ............│17
;│............ ............ ............ ............│18
;│............ ............ ............ ............│19
;│............ ............ ............ ............│20
;│............ ............ ............ ............│21
;│............ ............ ............ ............│22
;└───────────────────────────────────────────────────┘23
;               http://www.NedoPC.com/                24


MSG_FL_CHIP:
        .DB     $16, 2, 9,$15,$9F,"Chip",0
MSG_FL_ID:
        .DB     "ID: ",0,0
MSG_FL_M29F040:
        .DB     ": M29F040",0
MSG_FL_AM29F040:
        .DB     ":Am29F040",0
MSG_FL_CRC:
        .DB     $16, 2,10,$15,$9F,"CRC: "        ,0,0
MSG_FL_SDCARD:
        .DB     $16, 2,11,        "SDcard: FAT"  ,0,0
MSG_FL_ERASECHIP:
        .DB     $16,19,14,$15,$9F," [",$FB,"] Erase chip ",0
MSG_FL_ERRPOS:
        .DB     $16, 1,11,$15,$AE,0
;
WIND_FL_MENU:
        .DB     0,1,17,7,$9F,$00
WIND_FL_STATUS:
        .DB     0,8,17,6,$9F,$00
WIND_FL_FILEPANEL:
        .DB     17,1,36,13,$9F,$00
WIND_FL_CONTENT:
        .DB     0,14,53,10,$9F,$00
WIND_FL_RESULT_OK:
        .DB     11,4,31,6,$CF,$01
WIND_FL_RESULT_FAIL:
        .DB     11,4,31,6,$AF,$01
;
FL_UNKNOWN:
        .DB     "    ????   ",0
FL_EMPTY:
        .DB     "   empty   ",0
FL_ZXBAS48:
        .DB     $A8,$02,$99,$0C ;0C9902A8
        .DB     "zx basic48 ",0
FL_ZXBAS128:
        .DB     $59,$D5,$91,$5C ;5C91D559
        .DB     "zx basic128",0
FL_TRDOS:
        .DB     $E3,$39,$3C,$F2 ;F23C39E3
        .DB     "tr-dos     ",0
FL_ALCOGLUKPEN:
        .DB     $44,$6F,$D7,$87 ;87D76F44
        .DB     "alcoglukpen",0
FL_HEGLUK:
        .DB     "HEGL"
        .DB     "hegluk     ",0
FL_ATM2CPM:
        .DB     $1E,$65,$1E,$B3 ;B31E651E
        .DB     "atm2_cpm   ",0
FL_XBIOSMENU:
        .DB     $BC,$A4,$2C,$29 ;292CA4BC
        .DB     "xbios stmnu",0
FL_VTRDOS:
        .DB     $27,$2E,$23,$68 ;68232E27
        .DB     "vtr-dos    ",0
;
MSG_FP_DIR:
        .DB     " <DIR>",$B3,0
;
;
;
;--------------------------------------
;
.EQU    FL_CONTENT      =MEGABUFFER
.EQU    FL_BUFFER       =FL_CONTENT+512
.DSEG
FL_TMP0:        .BYTE   2
FL_TMP2:        .BYTE   1
FL_STACK:       .BYTE   2
FL_Y:           .BYTE   2
.CSEG
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
        LDI     WL,$00
        LDI     WH,$00
        LDI     TMP2,$00
        RCALL   F_IN
        MOV     ZL,DATA
        LDI     WL,$01
        RCALL   F_IN
        MOV     ZH,DATA
        RJMP    F_RST
;
;--------------------------------------
;запись 512 байт во Flash-ROM из буфера BUFSECT
;in:    WL,WH == address (LO,MID)
;out:   W+512
F_WRITE512:
        LDIZ    BUFSECT
        LDIX    512

F_W5122:LD      DATA,Z
        CPI     DATA,$FF
        BREQ    F_W5123
        LDI     DATA,$A0
        RCALL   F_CMD
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000001
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_LOADDR
        MOV     DATA,WL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_MIDADDR
        MOV     DATA,WH
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
F_W5121:RCALL   FPGA_SAME_REG
        LD      TEMP,Z
        EOR     DATA,TEMP
        SBRC    DATA,7
        RJMP    F_W5121

F_W5123:ADIW    ZL,1
        ADIW    WL,1
        SBIW    XL,1
        BRNE    F_W5122

        RET
;
;--------------------------------------
;стирание всего чипа Flash-ROM
F_CHIPERASE:
        LDI     DATA,$80
        RCALL   F_CMD
        LDI     DATA,$10
        RCALL   F_CMD
F_ERAS1:LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        RCALL   FPGA_REG
F_ERAS9:RCALL   FPGA_SAME_REG
        SBRS    DATA,7
        RJMP    F_ERAS9
;
; - - - - - - - - - - - - - - - - - - -
;сброс Flash-ROM чипа
F_RST:  LDI     DATA,$F0
        RCALL   F_CMD
        DELAY_US 5
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        RCALL   FPGA_REG
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
        POP     DATA
F_WRD:  LDI     TEMP,FLASH_DATA
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000101
        RCALL   FPGA_REG
        LDI     DATA,0B00000001
        RJMP    FPGA_SAME_REG
;
;--------------------------------------
;чтение одного байта Flash-ROM
;in:    WL,WH,TMP2 == address
;out:   DATA == data
F_IN:   LDI     TEMP,FLASH_HIADDR
        MOV     DATA,TMP2
        RCALL   FPGA_REG
F_IN2:  LDI     TEMP,FLASH_MIDADDR
        MOV     DATA,WH
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_LOADDR
        MOV     DATA,WL
        RCALL   FPGA_REG
        LDI     TEMP,FLASH_DATA
        LDI     DATA,$FF
        RJMP    FPGA_REG
;
;--------------------------------------
;in:    WH,TMP2 - адрес (mid,hi) в Flash-ROM
;       Z - куда (в ОЗУ AVR-а)
;       X - сколько байт
F_READFLASH:; + ещё посчитать crc32, + ещё ...
        CLR     WL
F_RDFL1:RCALL   F_IN
        ST      Z+,DATA
        AND     COUNT,DATA
        CALL    CRC32_UPDATE
        ADIW    WL,1
        ADC     TMP2,NULL
        SBIW    XL,1
        BRNE    F_RDFL1
        RET
;
;======================================
;
.EQU    CRC32_0         =0
.EQU    CRC32_1         =1
.EQU    CRC32_2         =2
.EQU    CRC32_3         =3
.EQU    FLSH_ADR1       =4
.EQU    FLSH_ADR2       =5
.EQU    FLSH_COUNT      =6
.EQU    FLFP_BUFADR0    =7
.EQU    FLFP_BUFADR1    =8
.EQU    FLFP_TOTAL      =9
.EQU    FLFP_TOP        =10
.EQU    FLFP_SELECT     =11
.EQU    FLFP_CURSOR     =12
.EQU    FLSH_TEMP0      =13
.EQU    FLSH_TEMP1      =14
.EQU    FLSH_TEMP2      =15
.EQU    FLSH_TEMP3      =16
.EQU    FLSH_START      =17
.EQU    FLSH_SIZE       =18
.EQU    FLSH_ERASE      =19
.EQU    FLMNU_FLAGS     =20
.EQU    FLMNU_CURSOR    =21
.EQU    FLMEMSIZE       =22
;
.EQU    FLFP_HEIGHT     =11
.EQU    FLFP_WIDTH      =34
.EQU    FLFP_XPOS       =18
.EQU    FLFP_YPOS       =2
;
FLASHER:
        GETMEM  FLMEMSIZE
FL_REVERT:
        ANDI    FLAGS1,0B11111100
        STH     FLSH_ERASE,NULL
        STH     FLMNU_FLAGS,NULL
        LDI     DATA,0B10000010
        STH     FLMNU_CURSOR,DATA

        LDIX    FL_CONTENT
        LDI     COUNT,32
FL_CLRCNT2:
        LDI     DATA,$80
        ST      X+,DATA
        LDI     TEMP,15
FL_CLRCNT1:
        ST      X+,NULL
        DEC     TEMP
        BRNE    FL_CLRCNT1
        DEC     COUNT
        BRNE    FL_CLRCNT2

        LDIZ    WIND_FL_MENU*2
        CALL    WINDOW
        LDIZ    WIND_FL_STATUS*2
        CALL    WINDOW
        LDIZ    WIND_FL_FILEPANEL*2
        CALL    WINDOW
        LDIZ    WIND_FL_CONTENT*2
        CALL    WINDOW

        LDIZ    MLMSG_FL_MENU*2
        CALL    SCR_PRINTMLSTR
        RCALL   FLMENU_PUTCURSOR
        LDI     DATA,0B00001111
        STH     FLMNU_FLAGS,DATA
; - - - - - - - - - - - - - - - - - - -
        LDIZ    MSG_FL_CHIP*2
        CALL    SCR_PRINTSTRZ

        RCALL   F_ID
        CPI     ZL,$01
        BRNE    FL_DET_CHIP1
        CPI     ZH,$A4
        BRNE    FL_DET_CHIP1
        LDIZ    MSG_FL_AM29F040*2
        RJMP    FL_DET_CHIP2
FL_DET_CHIP1:
        CPI     ZL,$20
        BRNE    FL_DET_CHIP8
        CPI     ZH,$E2
        BRNE    FL_DET_CHIP8
        LDIZ    MSG_FL_M29F040*2
FL_DET_CHIP2:
        CALL    SCR_PRINTSTRZ
        RJMP    FL_DET_CHIP9

FL_DET_CHIP8:
        PUSHZ
        LDIZ    MSG_FL_ID*2
        CALL    SCR_PRINTSTRZ
        POPZ
        MOV     DATA,ZL
        CALL    HEXBYTE
        LDI     DATA,$20
        CALL    PUTCHAR
        MOV     DATA,ZH
        CALL    HEXBYTE
FL_DET_CHIP9:
; - - - - - - - - - - - - - - - - - - -
        LDIZ    MLMSG_FL_READROM*2
        CALL    SCR_PRINTMLSTR
        STH     FLSH_START,FF
        STH     FLSH_SIZE,NULL
        RCALL   FL_SHOWCONTENT

        RCALL   CRC32_INIT
        CLR     COUNT
        CLR     WH
        CLR     TMP2
FL_DET_ROM_6:
        STH     FLSH_COUNT,COUNT
        STH     FLSH_ADR1,WH
        STH     FLSH_ADR2,TMP2
        LDIZ    FL_UNKNOWN*2
        STSZ    FL_TMP0

        LDIZ    FL_BUFFER
        LDIX    $0400
        SER     COUNT
        RCALL   F_READFLASH
        STS     FL_TMP2,COUNT

        GETMEM  4
        LDIZ    FL_BUFFER+$0096
        LDIX    $016F
        RCALL   RAM_CRC32
        LDIZ    FL_ZXBAS48*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_1

        LDIZ    FL_BUFFER+$0009
        LDIX    $002F
        RCALL   RAM_CRC32
        LDIZ    FL_ZXBAS128*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_1

        LDIZ    FL_BUFFER+$0363
        LDIX    $0008
        RCALL   RAM_CRC32
        LDIZ    FL_VTRDOS*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_1

        LDIZ    FL_BUFFER+$0000
        LDIX    $0007
        RCALL   RAM_CRC32
        LDIZ    FL_XBIOSMENU*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_1

        LDIZ    FL_BUFFER+$0000
        LDIX    $0038
        RCALL   RAM_CRC32
        LDIZ    FL_ATM2CPM*2
        RCALL   FL_CRC_CMP
        BRNE    FL_DET_ROM_2
FL_DET_ROM_1:
        STSZ    FL_TMP0
FL_DET_ROM_2:
        FREEMEM 4

        CLR     WL
        LDH     WH,FLSH_ADR1
        ADDI    WH,$04
        LDH     TMP2,FLSH_ADR2
        LDIX    $3800
        LDS     COUNT,FL_TMP2
FL_CHKEMPT1:
        RCALL   F_IN
        AND     COUNT,DATA
        RCALL   CRC32_UPDATE
        ADIW    WL,1
        SBIW    XL,1
        BRNE    FL_CHKEMPT1
        STS     FL_TMP2,COUNT

        LDH     WH,FLSH_ADR1
        ADDI    WH,$3C
        LDH     TMP2,FLSH_ADR2
        LDIZ    FL_BUFFER
        LDIX    $0400
        RCALL   F_READFLASH
        LDIZ    FL_EMPTY*2
        INC     COUNT
        BRNE    FL_DET_ROM_3
        STSZ    FL_TMP0
FL_DET_ROM_3:

        GETMEM  4
        LDIZ    FL_BUFFER+$03F8
        LDD     R0,Z+0
        LDD     R1,Z+1
        LDD     R2,Z+2
        LDD     R3,Z+3
        LDIZ    FL_HEGLUK*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_4

        LDIZ    FL_BUFFER+$012F
        LDIX    $0209
        RCALL   RAM_CRC32
        LDIZ    FL_ALCOGLUKPEN*2
        RCALL   FL_CRC_CMP
        BREQ    FL_DET_ROM_4

        LDIZ    FL_BUFFER+$02C2
        LDIX    $012E
        RCALL   RAM_CRC32
        LDIZ    FL_TRDOS*2
        RCALL   FL_CRC_CMP
        BRNE    FL_DET_ROM_5
FL_DET_ROM_4:
        STSZ    FL_TMP0
FL_DET_ROM_5:
        FREEMEM 4

        LDSZ    FL_TMP0
        LDIX    FL_CONTENT+5
        LDH     DATA,FLSH_COUNT
        LDI     TEMP,16
        MUL     DATA,TEMP
        ADD     XL,R0
        ADC     XH,R1
        LDI     COUNT,11
FL_DET_ROM_8:
        LPM     DATA,Z+
        ST      X+,DATA
        DEC     COUNT
        BRNE    FL_DET_ROM_8
        RCALL   FL_SHOWCONTENT

        CALL    INKEY
        BREQ    FL_DET_ROM_7
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    FL_DET_ROM_7
        CPI     DATA,KEY_ESC
        BRNE    FL_DET_ROM_7
        RJMP    FLSH_EXIT
FL_DET_ROM_7:
        LDH     WH,FLSH_ADR1
        LDH     TMP2,FLSH_ADR2
        LDI     TEMP,$40
        ADD     WH,TEMP
        ADC     TMP2,NULL
        LDH     COUNT,FLSH_COUNT
        INC     COUNT
        SBRS    COUNT,5 ; COUNT==32 ?
        RJMP    FL_DET_ROM_6

        RCALL   CRC32_RELEASE
        LDIZ    MSG_FL_CRC*2
        CALL    SCR_PRINTSTRZ
        LDH     DATA,CRC32_3
        CALL    HEXBYTE
        LDH     DATA,CRC32_2
        CALL    HEXBYTE
        LDH     DATA,CRC32_1
        CALL    HEXBYTE
        LDH     DATA,CRC32_0
        CALL    HEXBYTE
; - - - - - - - - - - - - - - - - - - -
        LDIZ    MLMSG_FL_SDINIT*2
        CALL    SCR_PRINTMLSTR

        IN      TEMP,SPL
        STS     FL_STACK+0,TEMP
        IN      TEMP,SPH
        STS     FL_STACK+1,TEMP
        STS     FL_Y+0,YL
        STS     FL_Y+1,YH
        LDIZ    FL_ERRHANDLER
        RCALL   SD_FAT_INIT

        PUSHX
        PUSHW
        LDIZ    MSG_FL_SDCARD*2
        CALL    SCR_PRINTSTRZ
        LDS     TMP2,FAT_CAL_FAT
        LDI     DATA,$31 ;"1"
        LDI     TEMP,$32 ;"2"
        TST     TMP2
        BREQ    FP_SDI1
        LDI     TEMP,$36 ;"6"
        DEC     TMP2
        BREQ    FP_SDI1
        LDI     DATA,$33 ;"3"
        LDI     TEMP,$32 ;"2"
FP_SDI1:PUSH    TEMP
        CALL    SCR_PUTCHAR
        POP     DATA
        CALL    SCR_PUTCHAR
        POPW
        POPX

        RCALL   FP_RD_DIR
        STH     FLFP_CURSOR,FF
        RCALL   FP_OUT
        STH     FLFP_CURSOR,NULL
; - - - - - - - - - - - - - - - - - - -
FLMENU1:LDH     DATA,FLMNU_CURSOR
        ANDI    DATA,0B00011111
FLMENU2:STH     FLMNU_CURSOR,DATA
        RCALL   FLMENU_PUTCURSOR
FLMENU0:CALL    WAITKEY
        CPI     DATA,KEY_UP
        BREQ    FLMENU_UP
        CPI     DATA,KEY_DOWN
        BREQ    FLMENU_DOWN
        CPI     DATA,KEY_ENTER
        BREQ    FLMENU_ENTER
        CPI     DATA,KEY_ESC
        BRNE    FLMENU0
        LDI     DATA,0B00000001
        RJMP    FLMENU2
;
FLMENU_UP:
        LDH     DATA,FLMNU_CURSOR
FLMENU_U1:
        LSR     DATA
        BRCS    FLMENU0
        LDH     TEMP,FLMNU_FLAGS
        AND     TEMP,DATA
        BRNE    FLMENU_U2
        RJMP    FLMENU_U1
FLMENU_U2:
        STH     FLMNU_CURSOR,DATA
        RJMP    FLMENU1
;
FLMENU_DOWN:
        LDH     DATA,FLMNU_CURSOR
FLMENU_D1:
        LSL     DATA
        BRCS    FLMENU0
        LDH     TEMP,FLMNU_FLAGS
        AND     TEMP,DATA
        BRNE    FLMENU_U2
        RJMP    FLMENU_D1
;
FLMENU_ENTER:
        LDH     DATA,FLMNU_CURSOR
        LDH     TEMP,FLMNU_FLAGS
        AND     DATA,TEMP
        BREQ    FLMENU0
        LSR     DATA
        BRCC    FLMENU_E1
FLSH_EXIT:
        FREEMEM FLMEMSIZE
        RET
FLMENU_E1:
        LSR     DATA
        BRCC    FLMENU_E2
        RJMP    FL_REVERT
FLMENU_E2:
        LSR     DATA
        BRCC    FLMENU_E3
        RJMP    FL_ERASEJOB
FLMENU_E3:
        LSR     DATA
        BRCC    FLMENU_E4
        LDH     DATA,FLMNU_CURSOR
        ORI     DATA,0B10000000
        STH     FLMNU_CURSOR,DATA
        RCALL   FLMENU_PUTCURSOR
        RJMP    FP_FS1
FLMENU_E4:
        LSR     DATA
        BRCS    FLMENU_E5
        RJMP    FLMENU0
;
FLMENU_E5: ; execute job(s)
        LDH     DATA,FLMNU_CURSOR
        ORI     DATA,0B10000000
        STH     FLMNU_CURSOR,DATA
        RCALL   FLMENU_PUTCURSOR

        LDIZ    MLMSG_FL_SURE*2
        CALL    SCR_PRINTMLSTR
        CALL    WAITKEY
        SBRC    TEMP,PS2K_BIT_EXTKEY
        RJMP    FL_EX01
        CPI     DATA,KEY_Y
        BREQ    FL_EX09
FL_EX01:LDI     XL,2
        LDI     XH,12
        CALL    SCR_SET_CURSOR
        LDI     DATA,$20
        LDI     TEMP,$9F
        LDI     COUNT,13
        CALL    SCR_FILL_CHAR_ATTR
        RJMP    FLMENU1
FL_EX09:
;
        LDI     TEMP,$0E
        CALL    SCR_SET_ATTR
        LDH     DATA,FLSH_ERASE
        LDI     COUNT,0
FL_EX12:LSR     DATA
        BRCC    FL_EX11

        PUSH    DATA
        PUSH    COUNT
        LSL     COUNT
        LSL     COUNT
        PUSH    COUNT
        RCALL   FL_SHW_SETCURSOR
        LDI     DATA,$45;"E"
        LDI     COUNT,12
        CALL    SCR_FILL_CHAR
        POP     COUNT
        INC     COUNT
        PUSH    COUNT
        RCALL   FL_SHW_SETCURSOR
        LDI     DATA,$45;"E"
        LDI     COUNT,12
        CALL    SCR_FILL_CHAR
        POP     COUNT
        INC     COUNT
        PUSH    COUNT
        RCALL   FL_SHW_SETCURSOR
        LDI     DATA,$45;"E"
        LDI     COUNT,12
        CALL    SCR_FILL_CHAR
        POP     COUNT
        INC     COUNT
        RCALL   FL_SHW_SETCURSOR
        LDI     DATA,$45;"E"
        LDI     COUNT,12
        CALL    SCR_FILL_CHAR
        POP     COUNT
        POP     DATA

FL_EX11:INC     COUNT
        CPI     COUNT,8
        BRNE    FL_EX12

        LDIZ    MLMSG_FL_ERASE*2
        CALL    SCR_PRINTMLSTR
        LDH     DATA,FLSH_ERASE
        CPI     DATA,$FF
        BRNE    FL_EX15
        RCALL   F_CHIPERASE
        RJMP    FL_EX20
FL_EX15:
        LDI     TMP2,$FF
FL_EX16:INC     TMP2
        LSR     DATA
        BRCC    FL_EX16
        PUSH    DATA
        PUSH    TMP2
        LDI     DATA,$80
        RCALL   F_CMD
        LDI     TEMP,FLASH_HIADDR
        POP     DATA
        PUSH    DATA
        CALL    FPGA_REG
        LDI     DATA,$30
        RCALL   F_CMD
        POP     TMP2
        POP     DATA
FL_EX18:INC     TMP2
        CPI     TMP2,8
        BRCC    FL_EX17
        LSR     DATA
        BRCC    FL_EX18
        PUSH    DATA
        PUSH    TMP2
        LDI     TEMP,FLASH_HIADDR
        MOV     DATA,TMP2
        CALL    FPGA_REG
        LDI     DATA,$30
        RCALL   F_WRD
        POP     TMP2
        POP     DATA
        RJMP    FL_EX18
FL_EX17:
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        CALL    FPGA_REG
        LDI     TEMP,FLASH_DATA
        CALL    FPGA_REG
FL_EX19:CALL    FPGA_SAME_REG
        SBRS    DATA,3
        RJMP    FL_EX19
        RCALL   F_ERAS1
FL_EX20:
;
        LDIZ    MLMSG_FL_WRITE*2
        CALL    SCR_PRINTMLSTR
        LDI     TEMP,$0A
        CALL    SCR_SET_ATTR

        LDI     COUNT,0
FL_EX30:STH     FLSH_COUNT,COUNT
        LDIZ    FL_CONTENT
        LDI     TEMP,16
        MUL     TEMP,COUNT
        ADD     ZL,R0
        ADC     ZH,R1
        LD      WL,Z
        ANDI    WL,$3F
        BREQ    FL_EX390
        CPI     WL,33
        BRCS    FL_EX31
FL_EX390:RJMP   FL_EX39
FL_EX31:STH     FLSH_SIZE,WL
        CLR     WH
        CLR     XL
        CLR     XH
        LD      TEMP,Z
        SBRC    TEMP,6
        ADIW    WL,32
        SBIW    WL,1
        LDS     DATA,FAT_BYTSSEC
        DEC     DATA
        AND     DATA,WL
        INC     DATA
        STS     FAT_MPHWOST,DATA
        LDS     DATA,FAT_BYTSSEC
        RCALL   BCDE_A
        STSW    FAT_KOL_CLS+0
        STSX    FAT_KOL_CLS+2
        STS     FAT_NUMSECK,NULL
        LDD     WL,Z+1
        LDD     WH,Z+2
        LDD     XL,Z+3
        LDD     XH,Z+4
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2

        LD      TEMP,Z
        SBRS    TEMP,6
        RJMP    FL_EX32
        LDI     COUNT,32
FL_EX33:PUSH    COUNT
        RCALL   NEXTSEC
        POP     COUNT
        DEC     COUNT
        BRNE    FL_EX33

FL_EX32:LDH     COUNT,FLSH_COUNT
        RCALL   FL_SHW_SETCURSOR
        CLR     WH
        LDH     TMP2,FLSH_COUNT
        LSR     TMP2
        ROR     WH
        LSR     TMP2
        ROR     WH
        LDI     TEMP,FLASH_HIADDR
        MOV     DATA,TMP2
        CALL    FPGA_REG
        LDIX    1365 ;16384/12
FL_EX35:STH     FLSH_TEMP0,XL
        STH     FLSH_TEMP1,XH
        PUSH    WH
        RCALL   NEXTSEC
        POP     WH
        CLR     WL
        RCALL   F_WRITE512
        LDH     XL,FLSH_TEMP0
        LDH     XH,FLSH_TEMP1
        SUBI    XH,2 ;HIGH(512)
        BRCC    FL_EX34
        LDI     DATA,$57;"W"
        CALL    SCR_PUTCHAR
        LDI     TEMP,LOW(1365)
        ADD     XL,TEMP
        LDI     TEMP,HIGH(1365)
        ADC     XH,TEMP
FL_EX34:LDH     TEMP,FLSH_SIZE
        DEC     TEMP
        STH     FLSH_SIZE,TEMP
        BRNE    FL_EX35

FL_EX39:LDH     COUNT,FLSH_COUNT
        INC     COUNT
        SBRS    COUNT,5 ;COUNT=32
        RJMP    FL_EX30
;
        RCALL   F_RST
        LDI     TEMP,FLASH_CTRL
        LDI     DATA,0B00000011
        CALL    FPGA_REG

        LDIZ    MLMSG_FL_VERIFY*2
        CALL    SCR_PRINTMLSTR

        LDI     COUNT,0
FL_EX40:STH     FLSH_COUNT,COUNT
        LDIZ    FL_CONTENT
        LDI     TEMP,16
        MUL     TEMP,COUNT
        ADD     ZL,R0
        ADC     ZH,R1
        LD      WL,Z
        ANDI    WL,$3F
        BREQ    FL_EX490
        CPI     WL,33
        BRCS    FL_EX41
FL_EX490:RJMP   FL_EX49
FL_EX41:STH     FLSH_SIZE,WL
        CLR     WH
        CLR     XL
        CLR     XH
        LD      TEMP,Z
        SBRC    TEMP,6
        ADIW    WL,32
        SBIW    WL,1
        LDS     DATA,FAT_BYTSSEC
        DEC     DATA
        AND     DATA,WL
        INC     DATA
        STS     FAT_MPHWOST,DATA
        LDS     DATA,FAT_BYTSSEC
        RCALL   BCDE_A
        STSW    FAT_KOL_CLS+0
        STSX    FAT_KOL_CLS+2
        STS     FAT_NUMSECK,NULL
        LDD     WL,Z+1
        LDD     WH,Z+2
        LDD     XL,Z+3
        LDD     XH,Z+4
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2

        LD      TEMP,Z
        SBRS    TEMP,6
        RJMP    FL_EX42
        LDI     COUNT,32
FL_EX43:PUSH    COUNT
        RCALL   NEXTSEC
        POP     COUNT
        DEC     COUNT
        BRNE    FL_EX43

FL_EX42:LDH     COUNT,FLSH_COUNT
        RCALL   FL_SHW_SETCURSOR
        CLR     WH
        LDH     TMP2,FLSH_COUNT
        LSR     TMP2
        ROR     WH
        LSR     TMP2
        ROR     WH
        LDI     TEMP,FLASH_HIADDR
        MOV     DATA,TMP2
        CALL    FPGA_REG
        STH     FLSH_TEMP2,NULL
        STH     FLSH_TEMP3,NULL
        LDIX    1365 ;16384/12
FL_EX45:STH     FLSH_TEMP0,XL
        STH     FLSH_TEMP1,XH
        PUSH    WH
        RCALL   NEXTSEC
        POP     WH
        CLR     WL

        LDIZ    BUFSECT
        LDIX    512
FL_EX47:RCALL   F_IN2
        LD      TEMP,Z+
        CP      DATA,TEMP
        BREQ    FL_EX46
        STH     FLSH_TEMP2,ONE
        STH     FLSH_TEMP3,ONE
FL_EX46:ADIW    WL,1
        SBIW    XL,1
        BRNE    FL_EX47

        LDH     XL,FLSH_TEMP0
        LDH     XH,FLSH_TEMP1
        SUBI    XH,2 ;HIGH(512)
        BRCC    FL_EX44
        LDH     DATA,FLSH_TEMP2
        LDI     TEMP,$0C
        SBRC    DATA,0
        LDI     TEMP,$AE
        CALL    SCR_SET_ATTR
        LDI     DATA,$56;"V"
        CALL    SCR_PUTCHAR
        STH     FLSH_TEMP2,NULL
        LDI     TEMP,LOW(1365)
        ADD     XL,TEMP
        LDI     TEMP,HIGH(1365)
        ADC     XH,TEMP
FL_EX44:LDH     TEMP,FLSH_SIZE
        DEC     TEMP
        STH     FLSH_SIZE,TEMP
        BRNE    FL_EX45

FL_EX49:LDH     COUNT,FLSH_COUNT
        INC     COUNT
        SBRS    COUNT,5 ;COUNT=32
        RJMP    FL_EX40
;
        LDIZ    MLMSG_FL_COMPLETE*2
        CALL    SCR_PRINTMLSTR

        LDIZ    WIND_FL_RESULT_OK*2
        LDH     DATA,FLSH_TEMP3
        TST     DATA
        BREQ    FL_EX91
        LDIZ    WIND_FL_RESULT_FAIL*2
FL_EX91:CALL    WINDOW
        LDIZ    MLMSG_FLRES0*2
        CALL    SCR_PRINTMLSTR

        LDIZ    MLMSG_FLRES1*2
        LDH     DATA,FLSH_TEMP3
        BREQ    FL_EX92
        LDIZ    MLMSG_FLRES2*2
FL_EX92:CALL    SCR_PRINTMLSTR

        CALL    WAITKEY
        RJMP    FL_REVERT
;
;--------------------------------------
;
FL_ERRHANDLER:
        CLI
        LDS     TEMP,FL_STACK+0
        OUT     SPL,TEMP
        LDS     TEMP,FL_STACK+1
        OUT     SPH,TEMP
        LDS     YL,FL_Y+0
        LDS     YH,FL_Y+1
        SEI
        PUSH    DATA
        LDIZ    MSG_FL_ERRPOS*2
        CALL    SCR_PRINTSTRZ
        POP     DATA
        LDIZ    MLMSG_FL_SDERROR1*2
        CPI     DATA,1
        BREQ    FL_ERRHNDL1
        LDIZ    MLMSG_FL_SDERROR2*2
        CPI     DATA,2
        BREQ    FL_ERRHNDL1
        LDIZ    MLMSG_FL_SDERROR3*2
        CPI     DATA,3
        BREQ    FL_ERRHNDL1
        LDIZ    MLMSG_FL_SDERRORX*2
FL_ERRHNDL1:
        CALL    SCR_PRINTMLSTR
        LDI     DATA,0B00000111
        STH     FLMNU_FLAGS,DATA
        LDI     DATA,0B00000010
        RJMP    FLMENU2
;
;--------------------------------------
;
FL_ERASEJOB:
        LDIX    FL_CONTENT
        LDI     TEMP,0B00000001
FL_EJB5:LDH     DATA,FLSH_ERASE
        AND     DATA,TEMP
        BRNE    FL_EJB1
        LDH     DATA,FLSH_ERASE
        OR      DATA,TEMP
        STH     FLSH_ERASE,DATA
        LDI     COUNT,4
FL_EJB3:ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        LDIZ    FL_EMPTY*2
        LDI     WL,11
FL_EJB2:LPM     DATA,Z+
        ST      X+,DATA
        DEC     WL
        BRNE    FL_EJB2
        DEC     COUNT
        BRNE    FL_EJB3
        RJMP    FL_EJB4
FL_EJB1:ADIW    XL,32
        ADIW    XL,32
FL_EJB4:LSL     TEMP
        BRCC    FL_EJB5
        LDH     DATA,FLMNU_FLAGS
        ORI     DATA,0B00010000
        STH     FLMNU_FLAGS,DATA
        RCALL   FL_SHOWCONTENT
        RJMP    FLMENU1
;
;--------------------------------------
;
FP_FS1: RCALL   FP_OUT
FP_FS0: CALL    WAITKEY
        CPI     DATA,KEY_UP
        BREQ    FP_1_UP
        CPI     DATA,KEY_DOWN
        BREQ    FP_1_DOWN
        CPI     DATA,KEY_PAGEUP
        BREQ    FP_P_UP
        CPI     DATA,KEY_PAGEDOWN
        BREQ    FP_P_DOWN
        CPI     DATA,KEY_HOME
        BREQ    FP_BEGIN
        CPI     DATA,KEY_END
        BREQ    FP_END
        CPI     DATA,KEY_ENTER
        BREQ    FP_ENTER
        CPI     DATA,KEY_ESC
        BRNE    FP_FS0

        LDH     XH,FLFP_CURSOR
        ADDI    XH,FLFP_YPOS
        LDI     XL,FLFP_XPOS
        CALL    SCR_SET_CURSOR
        LDI     TEMP,$9F
        LDI     COUNT,FLFP_WIDTH
        CALL    SCR_FILL_ATTR
        RJMP    FLMENU1
;
FP_END: RJMP    FP_END0
FP_ENTER:RJMP   FP_ENTER0
;
FP_1_UP:
        LDH     DATA,FLFP_SELECT
        TST     DATA
        BREQ    FP_FS0
        DEC     DATA
        STH     FLFP_SELECT,DATA
        LDH     DATA,FLFP_CURSOR
        TST     DATA
        BREQ    FP_1_UP1
        DEC     DATA
        STH     FLFP_CURSOR,DATA
        RJMP    FP_1_UP9
FP_1_UP1:
        LDH     DATA,FLFP_TOP
        DEC     DATA
        STH     FLFP_TOP,DATA
FP_1_UP9:
        RJMP    FP_FS1
;
FP_1_DOWN:
        LDH     DATA,FLFP_SELECT
        LDH     TEMP,FLFP_TOTAL
        INC     DATA
        CP      DATA,TEMP
        BRCC    FP_FS0
        STH     FLFP_SELECT,DATA
        LDH     DATA,FLFP_CURSOR
        CPI     DATA,FLFP_HEIGHT-1
        BRCC    FP_1_DOWN1
        INC     DATA
        STH     FLFP_CURSOR,DATA
        RJMP    FP_1_DOWN9
FP_1_DOWN1:
        LDH     DATA,FLFP_TOP
        INC     DATA
        STH     FLFP_TOP,DATA
FP_1_DOWN9:
        RJMP    FP_FS1
;
FP_P_UP:
        LDH     DATA,FLFP_SELECT
        SUBI    DATA,FLFP_HEIGHT-1
        BRCC    FP_P_UP1
FP_BEGIN:
        CLR     DATA
FP_P_UP1:
        STH     FLFP_SELECT,DATA
        STH     FLFP_TOP,DATA
        STH     FLFP_CURSOR,NULL
        RJMP    FP_FS1
;
FP_P_DOWN:
        LDH     TEMP,FLFP_TOTAL
        CPI     TEMP,FLFP_HEIGHT+1
        BRCC    FP_P_DOWN1
        LDI     DATA,FLFP_HEIGHT-1
        STH     FLFP_SELECT,DATA
        STH     FLFP_TOP,NULL
        STH     FLFP_CURSOR,DATA
        RJMP    FP_P_DOWN9
FP_P_DOWN1:
        LDH     DATA,FLFP_SELECT
        ADDI    DATA,FLFP_HEIGHT-1
        CP      DATA,TEMP
        BRCS    FP_P_DOWN2
        MOV     DATA,TEMP
        DEC     DATA
FP_P_DOWN2:
        STH     FLFP_SELECT,DATA
        LDI     TEMP,FLFP_HEIGHT-1
        STH     FLFP_CURSOR,TEMP
        SUB     DATA,TEMP
        STH     FLFP_TOP,DATA
FP_P_DOWN9:
        RJMP    FP_FS1
;
FP_END0:
        LDH     TEMP,FLFP_TOTAL
        CPI     TEMP,FLFP_HEIGHT+1
        BRCC    FP_END1
        LDI     DATA,FLFP_HEIGHT-1
        STH     FLFP_SELECT,DATA
        STH     FLFP_TOP,NULL
        STH     FLFP_CURSOR,DATA
        RJMP    FP_END9
FP_END1:
        MOV     DATA,TEMP
        DEC     DATA
        STH     FLFP_SELECT,DATA
        LDI     TEMP,FLFP_HEIGHT-1
        STH     FLFP_CURSOR,TEMP
        SUB     DATA,TEMP
        STH     FLFP_TOP,DATA
FP_END9:
        RJMP    FP_FS1
;
FP_ENTER0:
        LDH     DATA,FLFP_SELECT
        LDIZ    FL_BUFFER
        LDI     TEMP,32
        MUL     DATA,TEMP
        ADD     ZL,R0
        ADC     ZH,R1
        LDD     DATA,Z+11
        TST     DATA
        BREQ    FP_ENTER9
        LDD     WL,Z+26
        LDD     WH,Z+27
        LDD     XL,Z+20
        LDD     XH,Z+21
        STSW    FAT_TEK_DIR+0
        STSX    FAT_TEK_DIR+2
        RCALL   FP_RD_DIR
        STH     FLFP_CURSOR,NULL
        RJMP    FP_FS1
FP_ENTER9:
        STH     FLFP_BUFADR0,ZL
        STH     FLFP_BUFADR1,ZH
        LDD     WL,Z+28
        LDD     WH,Z+29
        LDD     XL,Z+30
        LDI     DATA,LOW(16383)
        LDI     TEMP,HIGH(16383)
        ADD     WL,DATA
        ADC     WH,TEMP
        ADC     XL,NULL
        LSL     WH
        ROL     XL
        LSL     WH
        ROL     XL
        STH     FLSH_SIZE,XL
        STH     FLSH_START,NULL

        LDH     XH,FLFP_CURSOR
        ADDI    XH,FLFP_YPOS
        LDI     XL,FLFP_XPOS
        CALL    SCR_SET_CURSOR
        LDI     TEMP,$1F
        LDI     COUNT,FLFP_WIDTH
        CALL    SCR_FILL_ATTR
;
FLMAP10:RCALL   FL_SHOWCONTENT

FLMAP11:CALL    WAITKEY
        CPI     DATA,KEY_UP
        BREQ    FLMAP_UP
        CPI     DATA,KEY_DOWN
        BREQ    FLMAP_DOWN
        CPI     DATA,KEY_LEFT
        BREQ    FLMAP_LEFT
        CPI     DATA,KEY_RIGHT
        BREQ    FLMAP_RIGHT
        CPI     DATA,KEY_ENTER
        BREQ    FLMAP_ENTER
        CPI     DATA,KEY_ESC
        BRNE    FLMAP11
        RJMP    FLMAP_EXIT
;
FLMAP_LEFT:
        LDH     DATA,FLSH_START
        TST     DATA
        BREQ    FLMAP11
        DEC     DATA
        STH     FLSH_START,DATA
        RJMP    FLMAP10
;
FLMAP_RIGHT:
        LDH     DATA,FLSH_START
        LDH     TEMP,FLSH_SIZE
        ADD     TEMP,DATA
        CPI     TEMP,32
        BRCC    FLMAP11
        INC     DATA
        STH     FLSH_START,DATA
        RJMP    FLMAP10
;
FLMAP_UP:
        LDH     DATA,FLSH_START
        SUBI    DATA,4
        BRCC    FLMAP_U1
        CLR     DATA
FLMAP_U1:
        STH     FLSH_START,DATA
        RJMP    FLMAP10
;
FLMAP_DOWN:
        LDH     DATA,FLSH_START
        LDH     TEMP,FLSH_SIZE
        ADD     DATA,TEMP
        ADDI    DATA,4
        CPI     DATA,32
        BRCS    FLMAP_D1
        LDI     DATA,32
FLMAP_D1:
        SUB     DATA,TEMP
        STH     FLSH_START,DATA
        RJMP    FLMAP10
;
FLMAP_ENTER:
        LDIZ    FL_CONTENT
        LDH     DATA,FLSH_START
        LDI     TEMP,16
        MUL     DATA,TEMP
        ADD     ZL,R0
        ADC     ZH,R1
        STH     FLSH_ADR1,ZL
        STH     FLSH_ADR2,ZH

        LDS     TEMP,FAT_BYTSSEC
        STS     FAT_NUMSECK,TEMP

        LDH     ZL,FLFP_BUFADR0
        LDH     ZH,FLFP_BUFADR1
        LDD     WL,Z+26
        LDD     WH,Z+27
        LDD     XL,Z+20
        LDD     XH,Z+21
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2
        LDD     WL,Z+28
        LDD     WH,Z+29
        LDD     XL,Z+30
        LDD     XH,Z+31
        LDI     TMP2,LOW(511)
        LDI     TMP3,HIGH(511)
        RCALL   HLDEPBC
        RCALL   BCDE200         ;получили кол-во секторов
;-------
FL_FS0: LDI     COUNT,32
        SBIW    WL,32
        BRCS    FL_FS9
        BREQ    FL_FS9
        STH     FLSH_TEMP0,WL
        STH     FLSH_TEMP1,WH
        RCALL   FL_CPTR

        LDI     COUNT,32

FL_FS2: STH     FLSH_COUNT,COUNT
        LDS     DATA,FAT_NUMSECK
        DEC     DATA
        BRNE    FL_FS3

        LDSW    FAT_TFILCLS+0
        LDSX    FAT_TFILCLS+2
        CALL    RDFATZP
        STSW    FAT_TFILCLS+0
        STSX    FAT_TFILCLS+2
        LDS     DATA,FAT_BYTSSEC
FL_FS3: STS     FAT_NUMSECK,DATA

        LDH     COUNT,FLSH_COUNT
        DEC     COUNT
        BRNE    FL_FS2

        LDH     WL,FLSH_TEMP0
        LDH     WH,FLSH_TEMP1
        RJMP    FL_FS0

FL_FS9: ADIW    WL,32
        MOV     COUNT,WL
        RCALL   FL_CPTR
;
FLMAP_EXIT:
        STH     FLSH_START,FF
        STH     FLSH_SIZE,NULL
        RCALL   FL_SHOWCONTENT
        RJMP    FLMENU1
;
;======================================
;
FL_CPTR:PUSH    COUNT
        LDI     TEMP,$01
        LDH     DATA,FLSH_START
        LSR     DATA
        LSR     DATA
        TST     DATA
        BREQ    FL_CPT1
FL_CPT2:LSL     TEMP
        DEC     DATA
        BRNE    FL_CPT2
FL_CPT1:LDH     DATA,FLSH_ERASE
        AND     DATA,TEMP
        BRNE    FL_CPT5
        LDH     DATA,FLSH_ERASE
        OR      DATA,TEMP
        STH     FLSH_ERASE,DATA
        LDIX    FL_CONTENT
        LDH     DATA,FLSH_START
        ANDI    DATA,0B00011100
        LDI     TEMP,16
        MUL     DATA,TEMP
        ADD     XL,R0
        ADC     XH,R1
        LDI     COUNT,4
FL_CPT4:ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        ST      X+,NULL
        LDIZ    FL_EMPTY*2
        LDI     TEMP,11
FL_CPT3:LPM     DATA,Z+
        ST      X+,DATA
        DEC     TEMP
        BRNE    FL_CPT3
        DEC     COUNT
        BRNE    FL_CPT4
FL_CPT5:POP     COUNT

        LDS     DATA,FAT_BYTSSEC
        LDS     TEMP,FAT_NUMSECK
        CP      DATA,TEMP
        BREQ    FL_CPT6
        ORI     COUNT,$40
FL_CPT6:LDH     XL,FLSH_ADR1
        LDH     XH,FLSH_ADR2
        ST      X+,COUNT
        LDS     DATA,FAT_TFILCLS+0
        ST      X+,DATA
        LDS     DATA,FAT_TFILCLS+1
        ST      X+,DATA
        LDS     DATA,FAT_TFILCLS+2
        ST      X+,DATA
        LDS     DATA,FAT_TFILCLS+3
        ST      X+,DATA
        LDH     ZL,FLFP_BUFADR0
        LDH     ZH,FLFP_BUFADR1
        LDI     COUNT,11
FL_CPT7:LD      DATA,Z+
        ST      X+,DATA
        DEC     COUNT
        BRNE    FL_CPT7
        STH     FLSH_ADR1,XL
        STH     FLSH_ADR2,XH
        LDH     DATA,FLMNU_FLAGS
        ORI     DATA,0B00010000
        STH     FLMNU_FLAGS,DATA
        LDH     DATA,FLSH_START
        INC     DATA
        STH     FLSH_START,DATA
        RET
;
;======================================
;
FLMENU_PUTCURSOR:
        CLR     COUNT
        LDI     TEMP,0B00000001
FPM_PC4:STH     FLSH_COUNT,COUNT
        STH     FLSH_TEMP0,TEMP
        MOV     XH,COUNT
        ADDI    XH,2
        LDI     XL,1
        CALL    SCR_SET_CURSOR
        LDH     DATA,FLSH_TEMP0
        LDH     XL,FLMNU_FLAGS
        LDI     TEMP,$9F
        AND     XL,DATA
        BRNE    FPM_PC1
        LDI     TEMP,$97
FPM_PC1:LDH     XL,FLMNU_CURSOR
        AND     DATA,XL
        BREQ    FPM_PC2
        LSL     XL
        BRCS    FPM_PC3
        LDI     TEMP,$F0
        RJMP    FPM_PC2
FPM_PC3:ANDI    TEMP,$1F
FPM_PC2:LDI     COUNT,15
        CALL    SCR_FILL_ATTR
        LDH     COUNT,FLSH_COUNT
        INC     COUNT
        LDH     TEMP,FLSH_TEMP0
        LSL     TEMP
        SBRS    TEMP,5
        RJMP    FPM_PC4
        RET
;
;======================================
;
FP_RD_DIR:
        LDIZ    FL_BUFFER
        STH     FLFP_BUFADR0,ZL
        STH     FLFP_BUFADR1,ZH
        STH     FLFP_TOTAL,NULL
        STH     FLFP_TOP,NULL
        STH     FLFP_SELECT,NULL

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

        LDIW    0
        RCALL   RDDIRSC
;поиск файла в директории
;       LDIW    0               ;номер описателя файла
        RJMP    FP_RDD2

FP_RDD1:ADIW    WL,1            ;номерописателя++
        ADIW    ZL,$20          ;следующий описатель
        CPI     ZH,HIGH(BUF4FAT+512);
                                ;вылезли за сектор?
        BRNE    FP_RDD2         ;нет ещё
        CALL    RDDIRSC         ;считываем следующий
        BRNE    FP_RDDE         ;кончились сектора в директории

FP_RDD2:LD      TEMP,Z          ;первый символ
        CPI     TEMP,$E5        ;удалён?
        BREQ    FP_RDD1
        TST     TEMP            ;пустой описатель? (конец списка)
        BREQ    FP_RDDE
        CPI     TEMP,$2E ;"."
        BRNE    FP_RDD3
        LDD     TEMP,Z+1
        CPI     TEMP,$2E ;"."
        BRNE    FP_RDD1

FP_RDD3:LDD     DATA,Z+$0B      ;атрибуты
        ANDI    DATA,0B11011110
        BREQ    FP_RDD4         ;файл
        CPI     DATA,$10        ;директория
        BRNE    FP_RDD1
        RJMP    FP_RDD7

FP_RDDE:RJMP    FP_RDDX

FP_RDD4:LDD     DATA,Z+8
        CPI     DATA,$52 ;"R"
        BREQ    FP_RDD5
        CPI     DATA,$42 ;"B"
        BRNE    FP_RDD1
        LDD     DATA,Z+9
        CPI     DATA,$49 ;"I"
        BRNE    FP_RDD1
        LDD     DATA,Z+10
        CPI     DATA,$4E ;"N"
        BREQ    FP_RDD6
FP_RDD1A:RJMP   FP_RDD1
FP_RDD5:LDD     DATA,Z+9
        CPI     DATA,$4F ;"O"
        BRNE    FP_RDD1
        LDD     DATA,Z+10
        CPI     DATA,$4D ;"M"
        BRNE    FP_RDD1

FP_RDD6:LDD     DATA,Z+31       ;длина
        TST     DATA
        BRNE    FP_RDD1
        LDD     DATA,Z+30
        LDD     R1,Z+29
        LDD     R0,Z+28
        SUB     R0,ONE
        SBC     R1,NULL
        SBC     DATA,NULL
        BRCS    FP_RDD1
        CPI     DATA,$08
        BRCC    FP_RDD1

FP_RDD7:LDH     XL,FLFP_BUFADR0
        LDH     XH,FLFP_BUFADR1
        LDI     COUNT,32
        PUSHZ
FP_RDD8:LD      DATA,Z+
        ST      X+,DATA
        DEC     COUNT
        BRNE    FP_RDD8
        POPZ
        STH     FLFP_BUFADR0,XL
        STH     FLFP_BUFADR1,XH
        LDH     COUNT,FLFP_TOTAL
        INC     COUNT
        STH     FLFP_TOTAL,COUNT
        CPI     COUNT,48
        BRCC    FP_SORT
        RJMP    FP_RDD1

FP_RDDX:LDH     COUNT,FLFP_TOTAL
        CPI     COUNT,1
        BREQ    FP_NOSORT
        TST     COUNT
        BRNE    FP_SORT
        LDH     DATA,FLMNU_FLAGS
        ANDI    DATA,0B00000111
        STH     FLMNU_FLAGS,DATA
        RJMP    FP_NOFILES
FP_SORT:
FP_RDDY:
        LDIZ    FL_BUFFER
        CLR     TMP2
        LDH     TMP3,FLFP_TOTAL
        DEC     TMP3
        LD      DATA,Z
        CPI     DATA,$2E
        BRNE    FP_RDDZ
        INC     TMP2
FP_RDDZ:RCALL   FSORT
FP_NOFILES:
FP_NOSORT:
;
        LDIZ    FL_BUFFER
        LDH     COUNT,FLFP_TOTAL
FP_LO2: LDD     DATA,Z+11
        ANDI    DATA,0B11011110
        STD     Z+11,DATA
        BRNE    FP_LO1
        LDI     TEMP,11
FP_LO3: LD      DATA,Z
        RCALL   TOLOWER
        ST      Z+,DATA
        DEC     TEMP
        BRNE    FP_LO3
        ADIW    ZL,21
        RJMP    FP_LO4
FP_LO1: ADIW    ZL,32
FP_LO4: DEC     COUNT
        BRNE    FP_LO2
        RET
;
;======================================
;
FP_OUT: CLR     COUNT
FPOUT00:PUSH    COUNT
        MOV     XH,COUNT
        ADDI    XH,FLFP_YPOS
        LDI     XL,FLFP_XPOS
        CALL    SCR_SET_CURSOR

        LDH     DATA,FLFP_CURSOR
        LDI     TEMP,$9F
        CP      DATA,COUNT
        BRNE    FPOUT04
        LDI     TEMP,$F0
FPOUT04:CALL    SCR_SET_ATTR

        LDH     DATA,FLFP_TOP
        ADD     DATA,COUNT
        LDH     TEMP,FLFP_TOTAL
        CP      DATA,TEMP
        BRCS    FPOUT01
        OR      COUNT,TEMP
        BRNE    FPOUT02

        LDIZ    MLMSG_FP_NOFILES*2
        CALL    SCR_PRINTMLSTR
        LDI     COUNT,FLFP_WIDTH-12 ;22
        RJMP    FPOUT03
FPOUT02:LDI     COUNT,FLFP_WIDTH ;34
FPOUT03:LDI     DATA,$20
        CALL    SCR_FILL_CHAR
        RJMP    FPOUT90

FPOUT01:LDIZ    FL_BUFFER
        LDI     TEMP,32
        MUL     DATA,TEMP
        ADD     ZL,R0
        ADC     ZH,R1
        PUSH    ZL
        LDI     COUNT,8
        CALL    SCR_PRNRAMSTRN
        LDI     DATA,$20
        CALL    SCR_PUTCHAR
        LDI     COUNT,3
        CALL    SCR_PRNRAMSTRN
        LDI     DATA,$B3 ;"│"
        CALL    SCR_PUTCHAR
        POP     ZL
        ;here Z=0
        PUSHZ
        LDD     DATA,Z+11
        TST     DATA
        BREQ    FPOUT11
        LDIZ    MSG_FP_DIR*2
        CALL    SCR_PRINTSTRZ
        RJMP    FPOUT12
FPOUT11:LDD     WL,Z+28
        LDD     WH,Z+29
        LDD     XL,Z+30
        RCALL   SCR_DEC1M
        LDI     DATA,$B3 ;"│"
        CALL    SCR_PUTCHAR
FPOUT12:POPZ

        LDD     DATA,Z+24
        ANDI    DATA,$1F
        CALL    DECBYTE
        LDI     DATA,$2E ;"."
        CALL    SCR_PUTCHAR
        LDD     DATA,Z+24
        LDD     TEMP,Z+25
        ROR     TEMP
        ROR     DATA
        SWAP    DATA
        ANDI    DATA,$0F
        CALL    DECBYTE
        LDI     DATA,$2E ;"."
        CALL    SCR_PUTCHAR
        LDD     DATA,Z+25
        LSR     DATA
        ADDI    DATA,80 ;+1980
FPOUT14:CPI     DATA,100
        BRCS    FPOUT13
        SUBI    DATA,100
        RJMP    FPOUT14
FPOUT13:CALL    DECBYTE
        LDI     DATA,$B3 ;"│"
        CALL    SCR_PUTCHAR

        LDD     DATA,Z+23
        LSR     DATA
        LSR     DATA
        LSR     DATA
        CALL    DECBYTE
        LDI     DATA,$3A ;":"
        CALL    SCR_PUTCHAR
        LDD     TEMP,Z+22
        LDD     DATA,Z+23
        ROL     TEMP
        ROL     DATA
        ROL     TEMP
        ROL     DATA
        ROL     TEMP
        ROL     DATA
        ANDI    DATA,$3F
        CALL    DECBYTE

FPOUT90:POP     COUNT
        INC     COUNT
        CPI     COUNT,FLFP_HEIGHT
        BRCC    FPOUT91
        RJMP    FPOUT00
FPOUT91:
        RET
;
;======================================
;For CodePage866 only!!!
;in:    DATA
;out:   DATA
TOLOWER:CPI     DATA,$41
        BRCS    TOLOW9   ;$00..."@"
        CPI     DATA,$5B
        BRCS    TOLOW8   ;"A"..."Z"
        CPI     DATA,$80
        BRCS    TOLOW9   ;"["...""
        CPI     DATA,$90
        BRCS    TOLOW8   ;"А"..."П"
        CPI     DATA,$A0
        BRCS    TOLOW7   ;"Р"..."Я"
        CPI     DATA,$F0
        BRCS    TOLOW9   ;"а"..."я"
        CPI     DATA,$F8
        BRCC    TOLOW9   ;$F8...$FF
        ORI     DATA,$01 ;"Ё"..."ў"
        RET
TOLOW7: ADDI    DATA,$80
        RET
TOLOW8: ADDI    DATA,$20
TOLOW9: RET
;
;======================================
;out number (up to 999999) in dec (right justify)
;in:    XL,WH,WL == number
SCR_DEC1M:
        LDIZ    DEC1MTAB*2
        LDI     COUNT,5
        MOV     R2,ONE
        CLR     DATA
DEC1M5: LPM     R0,Z+
        LPM     R1,Z+
DEC1M2: SUB     WL,R0
        SBC     WH,R1
        SBC     XL,R2
        BRCS    DEC1M1
        INC     DATA
        RJMP    DEC1M2
DEC1M1: ADD     WL,R0
        ADC     WH,R1
        ADC     XL,R2
        TST     DATA
        BRNE    DEC1M3
        LDI     DATA,$20
        CALL    SCR_PUTCHAR
        CLR     DATA
        RJMP    DEC1M4
DEC1M3: ORI     DATA,$30
        CALL    SCR_PUTCHAR
        LDI     DATA,$30
DEC1M4: CLR     R2
        DEC     COUNT
        BRNE    DEC1M5
        MOV     DATA,WL
        ORI     DATA,$30
        JMP     SCR_PUTCHAR
DEC1MTAB:.DW    $86A0,10000,1000,100,10
;
;======================================
;in:    Z == buffer ptr
;       TMP2 == lo index
;       TMP3 == hi index (TMP2!=TMP3)
FSORT:  MOV     R2,TMP2
        MOV     R3,TMP3
        MOV     R4,TMP2
        ADD     R4,TMP3
        LSR     R4
FSRT_00:
        MOV     WL,R4
        MOV     WH,R2
FSRT_02:RCALL   FCOMP
        BRCC    FSRT_01
        INC     WH
        RJMP    FSRT_02
FSRT_01:MOV     R2,WH

        MOV     WL,R3
        MOV     WH,R4
FSRT_04:RCALL   FCOMP
        BRCC    FSRT_03
        DEC     WL
        RJMP    FSRT_04
FSRT_03:MOV     R3,WL

        CP      R3,R2
        BRLT    FSRT_10
        CP      R2,R3
        BRGE    FSRT_06

        PUSHZ
        MOVW    XL,ZL
        LDI     COUNT,32
        MUL     R2,COUNT
        ADD     XL,R0
        ADC     XH,R1
        MUL     R3,COUNT
        ADD     ZL,R0
        ADC     ZH,R1
FSRT_05:LD      DATA,X
        LD      TEMP,Z
        ST      X+,TEMP
        ST      Z+,DATA
        DEC     COUNT
        BRNE    FSRT_05
        POPZ

FSRT_06:INC     R2
        DEC     R3
        CP      R3,R2
        BRGE    FSRT_00

FSRT_10:CP      R2,TMP3
        BRGE    FSRT_11
        PUSH    TMP2
        PUSH    R3
        MOV     TMP2,R2
        RCALL   FSORT
        POP     R3
        POP     TMP2
FSRT_11:CP      TMP2,R3
        BRGE    FSRT_12
        MOV     TMP3,R3
        RCALL   FSORT
FSRT_12:RET
;
;======================================
;in:    Z - array (item = filedescriptor = 32bytes)
;       WL, WH - lo & hi indexes
;out:   sreg.C - SET - [WL]>[WH], CLR - [WL]<=[WH]
;chng:  COUNT,DATA,TEMP
FCOMP:  PUSHZ
        MOVW    XL,ZL
        LDI     DATA,32
        MUL     WL,DATA
        ADD     XL,R0
        ADC     XH,R1
        MUL     WH,DATA
        ADD     ZL,R0
        ADC     ZH,R1
        LDD     TEMP,Z+11
        ANDI    TEMP,0B11011110
        ADDI    XL,11
        LD      DATA,X
        ANDI    DATA,0B11011110
        CP      DATA,TEMP
        BRNE    FCMP9
        SUBI    XL,11
        LDI     COUNT,11
FCMP1:  LD      DATA,X+
        LD      TEMP,Z+
        CP      TEMP,DATA
        BRNE    FCMP9
        DEC     COUNT
        BRNE    FCMP1
FCMP9:  POPZ
        RET
;
;======================================
;
RAM_CRC32:
        RCALL   CRC32_INIT
RAM_C32:LD      DATA,Z+
        RCALL   CRC32_UPDATE
        SBIW    XL,1
        BRNE    RAM_C32
        RJMP    CRC32_RELEASE
;
;======================================
;
FL_CRC_CMP:
        LPM     DATA,Z+
        CP      DATA,R0
        BRNE    FL_CRCCMP1
        LPM     DATA,Z+
        CP      DATA,R1
        BRNE    FL_CRCCMP1
        LPM     DATA,Z+
        CP      DATA,R2
        BRNE    FL_CRCCMP1
        LPM     DATA,Z+
        CP      DATA,R3
        BRNE    FL_CRCCMP1
FL_CRCCMP1:
        RET
;
;======================================
;
FL_SHOWCONTENT:
        LDIZ    FL_CONTENT
        CLR     COUNT

FL_SHW1:STH     FLSH_TEMP2,COUNT
        RCALL   FL_SHW_SETCURSOR
        LD      DATA,Z
        LDI     TEMP,$0E
        SBRC    DATA,7
        LDI     TEMP,$07
        LDH     COUNT,FLSH_TEMP2
        LDH     DATA,FLSH_START
        CP      COUNT,DATA
        BRCS    FL_SHW5
        LDH     XH,FLSH_SIZE
        ADD     DATA,XH
        CP      COUNT,DATA
        BRCC    FL_SHW5
        LDI     TEMP,$F0
FL_SHW5:LDH     XH,FLSH_TEMP2
        LDI     DATA,$10
        SBRC    XH,2
        EOR     TEMP,DATA
        CALL    SCR_SET_ATTR

        ADIW    ZL,5
        LDI     COUNT,8
        CALL    SCR_PRNRAMSTRN
        LDI     DATA,$20
        CALL    SCR_PUTCHAR
        LDI     COUNT,3
        CALL    SCR_PRNRAMSTRN

        LDH     COUNT,FLSH_TEMP2
        INC     COUNT
        CPI     COUNT,32
        BRCS    FL_SHW1

        LDH     DATA,FLSH_ERASE
        CPI     DATA,$FF
        BRNE    FL_SHW6
        LDIZ    MSG_FL_ERASECHIP*2
        CALL    SCR_PRINTSTRZ
FL_SHW6:RET
;
;======================================
;
FL_SHW_SETCURSOR:
        MOV     XL,COUNT
        ANDI    XL,$03
        LDI     TEMP,13
        MUL     XL,TEMP
        MOV     XL,R0
        INC     XL
        MOV     XH,COUNT
        ANDI    XH,$1C
        LSR     XH
        LSR     XH
        ADDI    XH,15
        JMP     SCR_SET_CURSOR
;
;======================================
;
