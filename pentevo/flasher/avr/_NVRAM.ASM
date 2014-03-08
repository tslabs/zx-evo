;
;--------------------------------------
;
.EQU    RTC_ADDRESS             =$A0    ; Address of PCF8583 RTC chip.
.EQU    RTC_COMMON_MODE_REG     =$FE    ; RTC's register for common modes.
;.EQU    MODE_VGA                =$01    ; VGA mode (0 - not set/1 - set).

.EQU    TW_START                =$08
.EQU    TW_REP_START            =$10
.EQU    TW_MT_SLA_ACK           =$18
.EQU    TW_MT_DATA_ACK          =$28
.EQU    TW_MR_SLA_ACK           =$40

.MACRO  TW_SEND_STOP
        LDI     TEMP,(1<<TWINT)|(1<<TWEN)|(1<<TWSTO)
        STS     TWCR,TEMP
.ENDMACRO
;
;--------------------------------------
;out:   DATA == mode (bit.0 - CLR == TV mode, SET == VGA mode)
NVRAM_READ_MODE:
;init i2c
;SCL frequency = CPU clk/ ( 16 + 2* (TWBR) * 4^(TWPS) )
;11052000 / (16 + 2*48 ) = 98678,5Hz (100000Hz recommended for PCF8583)
        STS     TWSR,NULL
        LDI     TEMP,48
        STS     TWBR,TEMP

;reset RTC
;write 0 to control/status register [0] on PCF8583

        RCALL   TW_SEND_START
        ANDI    DATA,TW_START|TW_REP_START
        BREQ    RTC_ERROR1

        LDI     DATA,RTC_ADDRESS
        RCALL   TW_SEND_ADDR
        CPI     DATA,TW_MT_SLA_ACK
        BRNE    RTC_ERROR1

        LDI     DATA,0
        RCALL   TW_SEND_DATA
        CPI     DATA,TW_MT_DATA_ACK
        BRNE    RTC_ERROR1

        LDI     DATA,0
        RCALL   TW_SEND_DATA
RTC_ERROR1:
        TW_SEND_STOP

;restore mode register from NVRAM

        RCALL   TW_SEND_START
        ANDI    DATA,TW_START|TW_REP_START
        BREQ    RTC_ERROR2

        LDI     DATA,RTC_ADDRESS
        RCALL   TW_SEND_ADDR
        CPI     DATA,TW_MT_SLA_ACK
        BRNE    RTC_ERROR2

        LDI     DATA,RTC_COMMON_MODE_REG
        RCALL   TW_SEND_DATA
        CPI     DATA,TW_MT_DATA_ACK
        BRNE    RTC_ERROR2

        RCALL   TW_SEND_START
        CPI     DATA,TW_REP_START

        LDI     DATA,RTC_ADDRESS|$01
        RCALL   TW_SEND_ADDR
        CPI     DATA,TW_MR_SLA_ACK
        BRNE    RTC_ERROR2

        RCALL   TW_READ_DATA
        RJMP    RTC_OK2
RTC_ERROR2:
        LDI     DATA,0
RTC_OK2:
        TW_SEND_STOP
        RET
;
;--------------------------------------
;out:   DATA == i2c status
TW_SEND_START:
;start transmit
        LDI     DATA,(1<<TWINT)|(1<<TWSTA)|(1<<TWEN)
        STS     TWCR,DATA
;wait for flag
TW_SS_WAIT:
        LDS     DATA,TWCR
        SBRS    DATA,TWINT
        RJMP    TW_SS_WAIT
;return status
        LDS     DATA,TWSR
        ANDI    DATA,$F8
        RET
;
;--------------------------------------
;in:    DATA == data/address
;out:   DATA == i2c status
TW_SEND_ADDR:
TW_SEND_DATA:
;set data/address
        STS     TWDR,DATA
;enable transmit
        LDI     DATA,(1<<TWINT)|(1<<TWEN)
        STS     TWCR,DATA
;wait for end transmit
TW_SD_WAIT:
        LDS     DATA,TWCR
        SBRS    DATA,TWINT
        RJMP    TW_SD_WAIT
;return status
        LDS     DATA,TWSR
        ANDI    DATA,$F8
        RET
;
;--------------------------------------
;out:   DATA == data
TW_READ_DATA:
;enable transmit
        LDI     DATA,(1<<TWINT)|(1<<TWEN)
        STS     TWCR,DATA
;wait for flag set
TW_RD_WAIT:
        LDS     DATA,TWCR
        SBRS    DATA,TWINT
        RJMP    TW_RD_WAIT
;get data
        LDS     DATA,TWDR
;NOT return status
;        LDS     DATA,TWSR
;        ANDI    DATA,$F8
        RET
;
;--------------------------------------
;
