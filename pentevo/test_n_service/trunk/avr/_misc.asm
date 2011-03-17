;
;--------------------------------------
;деление целочисл. без знака  32 бит на 16 бит
;in:    ZX - делимое
;       W  - делитель
;out:   ZX - результат
;       TEMPDATA - остаток
;chng:  COUNT
DIV3216U:
        LDI     COUNT,33
        CLR     DATA
        SUB     TEMP,TEMP
D3216U_1:
        ROL     XL
        ROL     XH
        ROL     ZL
        ROL     ZH
        DEC     COUNT
        BREQ    D3216U_3
        ROL     DATA
        ROL     TEMP
        SUB     DATA,WL
        SBC     TEMP,WH
        BRCC    D3216U_2
        ADD     DATA,WL
        ADC     TEMP,WH
        CLC
        RJMP    D3216U_1
D3216U_2:
        SEC
        RJMP    D3216U_1
D3216U_3:
        RET
;
;--------------------------------------
;out:   DATA == п.случайное число
RANDOM: PUSHW
        PUSH    TEMP
        LDS     WL,RND+2
        LDS     WH,RND+3
        STS     RND+3,WL
        ROL     WL
        ROL     WH
        MOV     DATA,WH
        ROL     WL
        ROL     WH
        ROL     WL
        ROL     WH
        ROL     WL
        ROL     WH
        EOR     DATA,WH
        LDS     WL,RND+1
        STS     RND+2,WL
        LDS     WL,RND+0
        STS     RND+1,WL
        STS     RND+0,DATA
        POP     TEMP
        POPW
        RET
;
;--------------------------------------
;
CLRPINS:LDIZ    $0020
CLRPIN1:ST      Z+,NULL
        CPI     ZL,$3C
        BRNE    CLRPIN1
        LDI     ZL,$61
CLRPIN2:ST      Z+,NULL
        CPI     ZL,$66
        BRNE    CLRPIN2
        RET
;
;--------------------------------------
;in:    W == address
;out:   DATA == data
EEPROM_READ:
        SBIC    EECR,EEWE
        RJMP    EEPROM_READ
        OUT     EEARH,WH
        OUT     EEARL,WL
        SBI     EECR,EERE
        IN      DATA,EEDR
        RET
;
;--------------------------------------
;in:    W == address
;       DATA == data
EEPROM_WRITE:
        MOV     TEMP,DATA
        RCALL   EEPROM_READ
        CP      TEMP,DATA
        BREQ    WREE9
        OUT     EEARH,WH
        OUT     EEARL,WL
        OUT     EEDR,TEMP
        SBI     EECR,EEMWE
        SBI     EECR,EEWE
WREE9:  RET
;
;--------------------------------------
;
CRC32_INIT:
        STD     Y+0,FF
        STD     Y+1,FF
        STD     Y+2,FF
        STD     Y+3,FF
        RET
;
CRC32_UPDATE:
        PUSHZ
        LDD     TEMP,Y+0
        EOR     DATA,TEMP
        CLR     TEMP
        LSL     DATA
        ROL     TEMP
        LSL     DATA
        ROL     TEMP
        LDIZ    TAB32*2 ;в младших 64K
        ADD     ZL,DATA
        ADC     ZH,TEMP
        LPM     DATA,Z+
        LDD     TEMP,Y+1
        EOR     DATA,TEMP
        STD     Y+0,DATA
        LPM     DATA,Z+
        LDD     TEMP,Y+2
        EOR     DATA,TEMP
        STD     Y+1,DATA
        LPM     DATA,Z+
        LDD     TEMP,Y+3
        EOR     DATA,TEMP
        STD     Y+2,DATA
        LPM     DATA,Z
        STD     Y+3,DATA
        POPZ
        RET
;
RAM_CRC32:
        RCALL   CRC32_INIT
        RCALL   RAM_CRC32_UPDATE
;
CRC32_RELEASE:
        LDD     R0,Y+0
        COM     R0
        STD     Y+0,R0
        LDD     R1,Y+1
        COM     R1
        STD     Y+1,R1
        LDD     R2,Y+2
        COM     R2
        STD     Y+2,R2
        LDD     R3,Y+3
        COM     R3
        STD     Y+3,R3
        RET
;
RAM_CRC32_UPDATE:
        LD      DATA,Z+
        RCALL   CRC32_UPDATE
        SBIW    XL,1
        BRNE    RAM_CRC32_UPDATE
        RET
;
TAB32:  .DW     $0000,$0000,$3096,$7707,$612C,$EE0E,$51BA,$9909
        .DW     $C419,$076D,$F48F,$706A,$A535,$E963,$95A3,$9E64
        .DW     $8832,$0EDB,$B8A4,$79DC,$E91E,$E0D5,$D988,$97D2
        .DW     $4C2B,$09B6,$7CBD,$7EB1,$2D07,$E7B8,$1D91,$90BF
        .DW     $1064,$1DB7,$20F2,$6AB0,$7148,$F3B9,$41DE,$84BE
        .DW     $D47D,$1ADA,$E4EB,$6DDD,$B551,$F4D4,$85C7,$83D3
        .DW     $9856,$136C,$A8C0,$646B,$F97A,$FD62,$C9EC,$8A65
        .DW     $5C4F,$1401,$6CD9,$6306,$3D63,$FA0F,$0DF5,$8D08
        .DW     $20C8,$3B6E,$105E,$4C69,$41E4,$D560,$7172,$A267
        .DW     $E4D1,$3C03,$D447,$4B04,$85FD,$D20D,$B56B,$A50A
        .DW     $A8FA,$35B5,$986C,$42B2,$C9D6,$DBBB,$F940,$ACBC
        .DW     $6CE3,$32D8,$5C75,$45DF,$0DCF,$DCD6,$3D59,$ABD1
        .DW     $30AC,$26D9,$003A,$51DE,$5180,$C8D7,$6116,$BFD0
        .DW     $F4B5,$21B4,$C423,$56B3,$9599,$CFBA,$A50F,$B8BD
        .DW     $B89E,$2802,$8808,$5F05,$D9B2,$C60C,$E924,$B10B
        .DW     $7C87,$2F6F,$4C11,$5868,$1DAB,$C161,$2D3D,$B666
        .DW     $4190,$76DC,$7106,$01DB,$20BC,$98D2,$102A,$EFD5
        .DW     $8589,$71B1,$B51F,$06B6,$E4A5,$9FBF,$D433,$E8B8
        .DW     $C9A2,$7807,$F934,$0F00,$A88E,$9609,$9818,$E10E
        .DW     $0DBB,$7F6A,$3D2D,$086D,$6C97,$9164,$5C01,$E663
        .DW     $51F4,$6B6B,$6162,$1C6C,$30D8,$8565,$004E,$F262
        .DW     $95ED,$6C06,$A57B,$1B01,$F4C1,$8208,$C457,$F50F
        .DW     $D9C6,$65B0,$E950,$12B7,$B8EA,$8BBE,$887C,$FCB9
        .DW     $1DDF,$62DD,$2D49,$15DA,$7CF3,$8CD3,$4C65,$FBD4
        .DW     $6158,$4DB2,$51CE,$3AB5,$0074,$A3BC,$30E2,$D4BB
        .DW     $A541,$4ADF,$95D7,$3DD8,$C46D,$A4D1,$F4FB,$D3D6
        .DW     $E96A,$4369,$D9FC,$346E,$8846,$AD67,$B8D0,$DA60
        .DW     $2D73,$4404,$1DE5,$3303,$4C5F,$AA0A,$7CC9,$DD0D
        .DW     $713C,$5005,$41AA,$2702,$1010,$BE0B,$2086,$C90C
        .DW     $B525,$5768,$85B3,$206F,$D409,$B966,$E49F,$CE61
        .DW     $F90E,$5EDE,$C998,$29D9,$9822,$B0D0,$A8B4,$C7D7
        .DW     $3D17,$59B3,$0D81,$2EB4,$5C3B,$B7BD,$6CAD,$C0BA
        .DW     $8320,$EDB8,$B3B6,$9ABF,$E20C,$03B6,$D29A,$74B1
        .DW     $4739,$EAD5,$77AF,$9DD2,$2615,$04DB,$1683,$73DC
        .DW     $0B12,$E363,$3B84,$9464,$6A3E,$0D6D,$5AA8,$7A6A
        .DW     $CF0B,$E40E,$FF9D,$9309,$AE27,$0A00,$9EB1,$7D07
        .DW     $9344,$F00F,$A3D2,$8708,$F268,$1E01,$C2FE,$6906
        .DW     $575D,$F762,$67CB,$8065,$3671,$196C,$06E7,$6E6B
        .DW     $1B76,$FED4,$2BE0,$89D3,$7A5A,$10DA,$4ACC,$67DD
        .DW     $DF6F,$F9B9,$EFF9,$8EBE,$BE43,$17B7,$8ED5,$60B0
        .DW     $A3E8,$D6D6,$937E,$A1D1,$C2C4,$38D8,$F252,$4FDF
        .DW     $67F1,$D1BB,$5767,$A6BC,$06DD,$3FB5,$364B,$48B2
        .DW     $2BDA,$D80D,$1B4C,$AF0A,$4AF6,$3603,$7A60,$4104
        .DW     $EFC3,$DF60,$DF55,$A867,$8EEF,$316E,$BE79,$4669
        .DW     $B38C,$CB61,$831A,$BC66,$D2A0,$256F,$E236,$5268
        .DW     $7795,$CC0C,$4703,$BB0B,$16B9,$2202,$262F,$5505
        .DW     $3BBE,$C5BA,$0B28,$B2BD,$5A92,$2BB4,$6A04,$5CB3
        .DW     $FFA7,$C2D7,$CF31,$B5D0,$9E8B,$2CD9,$AE1D,$5BDE
        .DW     $C2B0,$9B64,$F226,$EC63,$A39C,$756A,$930A,$026D
        .DW     $06A9,$9C09,$363F,$EB0E,$6785,$7207,$5713,$0500
        .DW     $4A82,$95BF,$7A14,$E2B8,$2BAE,$7BB1,$1B38,$0CB6
        .DW     $8E9B,$92D2,$BE0D,$E5D5,$EFB7,$7CDC,$DF21,$0BDB
        .DW     $D2D4,$86D3,$E242,$F1D4,$B3F8,$68DD,$836E,$1FDA
        .DW     $16CD,$81BE,$265B,$F6B9,$77E1,$6FB0,$4777,$18B7
        .DW     $5AE6,$8808,$6A70,$FF0F,$3BCA,$6606,$0B5C,$1101
        .DW     $9EFF,$8F65,$AE69,$F862,$FFD3,$616B,$CF45,$166C
        .DW     $E278,$A00A,$D2EE,$D70D,$8354,$4E04,$B3C2,$3903
        .DW     $2661,$A767,$16F7,$D060,$474D,$4969,$77DB,$3E6E
        .DW     $6A4A,$AED1,$5ADC,$D9D6,$0B66,$40DF,$3BF0,$37D8
        .DW     $AE53,$A9BC,$9EC5,$DEBB,$CF7F,$47B2,$FFE9,$30B5
        .DW     $F21C,$BDBD,$C28A,$CABA,$9330,$53B3,$A3A6,$24B4
        .DW     $3605,$BAD0,$0693,$CDD7,$5729,$54DE,$67BF,$23D9
        .DW     $7A2E,$B366,$4AB8,$C461,$1B02,$5D68,$2B94,$2A6F
        .DW     $BE37,$B40B,$8EA1,$C30C,$DF1B,$5A05,$EF8D,$2D02
;
;--------------------------------------
;
