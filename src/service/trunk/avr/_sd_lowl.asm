.EQU    CMD_17          =$51    ;read_single_block
.EQU    ACMD_41         =$69    ;sd_send_op_cond
;
CMD00:  .DB     $40,$00,$00,$00,$00,$95
CMD08:  .DB     $48,$00,$00,$01,$AA,$87
CMD16:  .DB     $50,$00,$00,$02,$00,$FF
CMD55:  .DB     $77,$00,$00,$00,$00,$FF ;app_cmd
CMD58:  .DB     $7A,$00,$00,$00,$00,$FF ;read_ocr
CMD59:  .DB     $7B,$00,$00,$00,$00,$FF ;crc_on_off
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
;       X,W - №сектора
;out:   sreg.Z - SET==error
SD_READ_SECTOR:
        LDS     DATA,SD_HC
        SBRC    DATA,6
        RJMP    SDRDSE1
        LSL     WL
        ROL     WH
        ROL     XL
        MOV     XH,XL
        MOV     XL,WH
        MOV     WH,WL
        CLR     WL
SDRDSE1:
        LDI     DATA,CMD_17
        RCALL   SD_EXCHANGE
        MOV     DATA,XH
        RCALL   SD_EXCHANGE
        MOV     DATA,XL
        RCALL   SD_EXCHANGE
        MOV     DATA,WH
        RCALL   SD_EXCHANGE
        MOV     DATA,WL
        RCALL   SD_EXCHANGE
        SER     DATA
        RCALL   SD_EXCHANGE

        SER     WL
SDRDSE2:RCALL   SD_WAIT_NOTFF
        DEC     WL
        BREQ    SDRDSE8
        CPI     DATA,$FE
        BRNE    SDRDSE2

        LDIW    512
SDRDSE3:RCALL   SD_RECEIVE
        ST      Z+,DATA
        SBIW    WL,1
        BRNE    SDRDSE3

        LDI     TEMP,2
        RCALL   SD_RD_DUMMY
;SDRDSE4:RCALL   SD_WAIT_NOTFF
;        CPI     DATA,$FF
;        BRNE    SDRDSE4
        CLZ
        RET
;ошибка при чтении сектора
SDRDSE8:RET
;
;--------------------------------------
;
