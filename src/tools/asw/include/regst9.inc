                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGST9.INC                                             *
;*                                                                          *
;*   Sinn : enthÑlt SFR-, Makro- und Adre·definitionen fÅr die ST9-Familie  *
;* 									    *
;*   letzte énderungen :  6. 2.1997                                         *
;*                                                                          *
;****************************************************************************

                ifndef  regst9inc       ; verhindert Mehrfacheinbindung

regst9inc       equ     1

                if      (MOMCPUNAME<>"ST9020")&&(MOMCPUNAME<>"ST9030")&&(MOMCPUNAME<>"ST9040")&&(MOMCPUNAME<>"ST9050")
                 fatal  "Falscher Prozessortyp eingestellt: nur ST9020,ST9030,ST9040 oder ST9050 erlaubt!"
		endif

                if      MOMPASS=1
                 message "ST9-SFR-Definitionen (C) 1997 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; RegisterbÑnke

__CNT           set     0
                rept    16
BK{"\{__CNT}"}0 equ     __CNT*2
BK{"\{__CNT}"}1 equ     __CNT*2+1
__CNT           set     __CNT+1
                endm

BK_SYS          equ     BKE0            ; Group system definition
BK_F            equ     BKF0            ; page register definition

;----------------------------------------------------------------------------
; Definition eines Bits:
; dies nutzt die interne Definition von Bitsymbolen aus: rrrrbbbi

__defbit        macro   NAME,REG,BITPOS
NAME            bit     ((REG&15)<<4)+(BITPOS<<1)
{"NAME"}m       equ     1<<BITPOS
                endm

;----------------------------------------------------------------------------
; Systemgruppe

FCW             reg     RR230           ; flag and control word
CICR            reg     R230            ; central interrupt control register
                __defbit gcen,CICR,7    ; global counter enable
                __defbit tlip,CICR,6    ; top level interrupt pending bit
                __defbit tli,CICR,5     ; top level interrupt bit
                __defbit ien,CICR,4     ; interrupt enable flag
                __defbit iam,CICR,3     ; interrupt arbitration mode
                __defbit cpl2,CICR,2    ; current priority level bit 2
                __defbit cpl1,CICR,1    ; current priority level bit 1
                __defbit cpl0,CICR,0    ; current priority level bit 0
cplm            equ     cpl2m|cpl1m|cpl0m ; current priority level

FLAGR           reg     R231            ; flags register
                __defbit c,FLAGR,7      ; carry flag
                __defbit z,FLAGR,6      ; zero flag
                __defbit s,FLAGR,5      ; sign flag
                __defbit v,FLAGR,4      ; overflow flag
                __defbit d,FLAGR,3      ; decimal adjust flag
                __defbit h,FLAGR,2      ; half carry flag
                __defbit uf,FLAGR,1     ; user flag 1
                __defbit dp,FLAGR,0     ; data/program memory flag

RPP             reg     RR232           ; register pointer pair

RP0R            reg     R232            ; register pointer #0
                __defbit rp0s,RP0R,2    ; register pointer selector

RP1R            reg     R233            ; register pointer #1
                __defbit rp1s,RP1R,2    ; register pointer selector

PPR             reg     R234            ; page pointer register

MODER           reg     R235            ; mode register
                __defbit ssp,MODER,7    ; system stack pointer flag (int/ext)
                __defbit usp,MODER,6    ; user stack pointer flag (int/ext)
                __defbit div2,MODER,5   ; external clock divided by 2
                __defbit prs2,MODER,4   ; internal clock prescaling bit 2
                __defbit prs1,MODER,3   ; internal clock prescaling bit 1
                __defbit prs0,MODER,2   ; internal clock prescaling bit 0
                __defbit brqen,MODER,1  ; bus request enable
                __defbit himp,MODER,0   ; high impedance enable
prsm            equ     prs2m|prs1m|prs0m ; internal clock prescaler

USPR            reg     RR236           ; user stack pointer
USPHR           reg     R236
USPLR           reg     R237

SSPR            reg     RR238           ; system stack pointer
SSPHR           reg     R238
SSPLR           reg     R239

;----------------------------------------------------------------------------
; EEPROM

                if      MOMCPUNAME="ST9040"

EEP_PG          equ     0               ; EEPROM register page

EECR            reg     R241            ; EEPROM control register
                __defbit verify,EECR,6  ; EEPROM verify mode
                __defbit eestby,EECR,5  ; EEPROM stand-by
                __defbit eeien,EECR,4   ; EEPROM interrupt enable
                __defbit pllst,EECR,3   ; Parallel write start
                __defbit pllen,EECR,2   ; Parallel write enable
                __defbit eebusy,EECR,1  ; EEPROM busy
                __defbit eewen,EECR,0   ; EEPROM write enable

                endif

;----------------------------------------------------------------------------
; Interrupts

EXINT_PG        equ     0               ; EXTERNAL interrupt register page

EITR            reg     R242            ; External interrupt trigger level register
                __defbit tea0,EITR,0    ; Trigger Event A0 bit
                __defbit tea1,EITR,1    ; Trigger Event A1 bit
                __defbit teb0,EITR,2    ; Trigger Event B0 bit
                __defbit teb1,EITR,3    ; Trigger Event B1 bit
                __defbit tec0,EITR,4    ; Trigger Event C0 bit
                __defbit tec1,EITR,5    ; Trigger Event C1 bit
                __defbit ted0,EITR,6    ; Trigger Event D0 bit
                __defbit ted1,EITR,7    ; Trigger Event D1 bit

EIPR            reg     R243            ; External interrupt pending register
                __defbit ipa0,EIPR,0    ; Interrupt Pending bit Channel A0
                __defbit ipa1,EIPR,1    ; Interrupt Pending bit " A1
                __defbit ipb0,EIPR,2    ; Interrupt Pending bit " B0
                __defbit ipb1,EIPR,3    ; Interrupt Pending bit " B1
                __defbit ipc0,EIPR,4    ; Interrupt Pending bit " C0
                __defbit ipc1,EIPR,5    ; Interrupt Pending bit " C1
                __defbit ipd0,EIPR,6    ; Interrupt Pending bit " D0
                __defbit ipd1,EIPR,7    ; Interrupt Pending bit " D1

EIMR            reg     R244            ; External interrupt mask register
                __defbit ima0,EIMR,0    ; Int. A0 bit
                __defbit ima1,EIMR,1    ; Int. A1 bit
                __defbit imb0,EIMR,2    ; Int. B0 bit
                __defbit imb1,EIMR,3    ; Int. B1 bit
                __defbit imc0,EIMR,4    ; Int. C0 bit
                __defbit imc1,EIMR,5    ; Int. C1 bit
                __defbit imd0,EIMR,6    ; Int. D0 bit
                __defbit imd1,EIMR,7    ; Int. D1 bit

EIPLR           reg     R245            ; Ext. interrupt priority level register

EIVR            reg     R246            ; External interrupt vector register
                __defbit ewen,EIVR,0    ; External wait enable
                __defbit ia0s,EIVR,1    ; Interrupt A0 selection
                __defbit tlis,EIVR,2    ; Top level input selection
                __defbit tltev,EIVR,3   ; Top level trigger event

NICR            reg     R247            ; Nested interrupt control register

                __defbit tlnm,NICR,7    ; Top level not maskable

;----------------------------------------------------------------------------
; Watchdog

WDT_PG          equ     0               ; Timer Watchdog page

WDTR            reg     RR248           ; TWD timer constant register.
WDTHR           reg     R248            ; TWD timer high constant register
WDTLR           reg     R249            ; TWD timer low constant register

WDTPR           reg     R250            ; TWD timer prescaler constant register

WDTCR           reg     R251            ; TWD timer control register
                __defbit WD_stsp8,WDTCR,7 ; TWD start stop.
                __defbit WD_sc,WDTCR,6    ; TWD single continuous mode.
                __defbit WD_inmd1,WDTCR,5 ; Input mode 1
                __defbit WD_inmd2,WDTCR,4 ; Input mode 2
                __defbit WD_inen,WDTCR,3  ; TWD input section enable/disable.
                __defbit WD_outmd,WDTCR,2 ; TWD output mode.
                __defbit WD_wrout,WDTCR,1 ; TWD output bit.
                __defbit WD_outen,WDTCR,0 ; TWD output enable.
inm_evc         equ     0               ; TWD input mode event counter.
inm_g           equ     010h            ; TWD input mode gated.
inm_t           equ     020h            ; TWD input mode triggerable.
inm_r           equ     030h            ; TWD input mode retriggerable.

WCR             reg     R252            ; Wait control register
                __defbit WD_wden,WCR,6  ; TWD timer enable.
wdm2            equ     1 << 5          ; Data Memory Wait Cycle
wdm1            equ     1 << 4
wdm0            equ     1 << 3
wpm2            equ     1 << 2          ; Program Memory Wait Cycle
wpm1            equ     1 << 1
wpm0            equ     1 << 0
dmwc1           equ     wdm0            ; 1 wait cycle on Data M.
dmwc2           equ     wdm1            ; 2 wait cycles on Data M.
dmwc3           equ     wdm1|wdm0       ; 3 wait cycles on Data M.
dmwc4           equ     wdm2            ; 4 wait cycles on Data M.
dmwc5           equ     wdm2|wdm0       ; 5 wait cycles on Data M.
dmwc6           equ     wdm2|wdm1       ; 6 wait cycles on Data M.
dmwc7           equ     wdm2|wdm1|wdm0  ; 7 wait cycles on Data M.
pmwc1           equ     wpm0            ; 1 wait cycle on Prog M.
pmwc2           equ     wpm1            ; 2 wait cycles on Prog M.
pmwc3           equ     wpm1|wpm0       ; 3 wait cycles on Prog M.
pmwc4           equ     wpm2            ; 4 wait cycles on Prog M.
pmwc5           equ     wpm2|wpm0       ; 5 wait cycles on Prog M.
pmwc6           equ     wpm2|wpm1       ; 6 wait cycles on Prog M.
pmwc7           equ     wpm2|wpm1|wpm0  ; 7 wait cycles on Prog M.

;----------------------------------------------------------------------------
; SPI

SPI_PG          equ     0               ; SPI register page
SPIDR           reg     R253            ; SPI Data register

SPICR           reg     R254            ; SPI Control register
                __defbit SP_spen,SPICR,7 ; Serial Peripheral Enable.
                __defbit SP_bms,SPICR,6  ; SBUS/I2C bus Mode Selector.
                __defbit SP_arb,SPICR,5  ; Arbitration flag bit.
                __defbit SP_busy,SPICR,4 ; SPI busy flag.
                __defbit SP_cpol,SPICR,3 ; SPI transmission clock polarity
                __defbit SP_cpha,SPICR,2 ; SPI transmission clock phase
                __defbit SP_spr1,SPICR,1 ; SPI rate bit 1
                __defbit SP_spr0,SPICR,0 ; SPI rate bit 0

SP_8            equ     0               ; SPI clock divider 8 = 1500 kHz (12MHz)
SP_16           equ     1               ; SPI clock divider 16 = 750 kHz (12MHz)
SP_128          equ     2               ; SPI clock divider 128 = 93.75 kHz (12MHz)
SP_256          equ     3               ; SPI clock divider 256 = 46.87 kHz (12MHz)
RW_PG           equ     0               ; R/W signal programming page

;----------------------------------------------------------------------------
; Read/Write Register

                if      MOMCPUNAME="ST9050"

RWR             reg     R255            ; R/W signal programming register
                __defbit RW_rw,RWR,0    ; R/W bit
                __defbit RW_bs,RWR,1    ; Bank switch port timing

                endif

;----------------------------------------------------------------------------
; Ports

P0C_PG          equ     2               ; Port 0 control registers page

P0DR            reg     R224            ; Port 0 data register
P0C0R           reg     R240            ; Port 0 control register 0
P0C1R           reg     R241            ; Port 0 control register 1
P0C2R           reg     R242            ; Port 0 control register 2

P1C_PG          equ     2               ; Port 1 control registers page
P1DR            reg     R225            ; Port 1 data register
P1C0R           reg     R244            ; Port 1 control register 0
P1C1R           reg     R245            ; Port 1 control register 1
P1C2R           reg     R246            ; Port 1 control register 2

P2C_PG          equ     2               ; Port 2 control registers page
P2DR            reg     R226            ; Port 2 data register
BS_DSR          reg     R226            ; Bank Switch data segment register
P2C0R           reg     R248            ; Port 2 control register 0
BS_DDSR         reg     R248            ; Bank Switch Data DMA segment register
P2C1R           reg     R249            ; Port 2 control register 1
BS_PDSR         reg     R249            ; Bank Switch Program DMA segment Register
P2C2R           reg     R250            ; Port 2 control register 2

P3C_PG          equ     2               ; Port 3 control registers page
P3DR            reg     R227            ; Port 3 data register
BS_PSR          reg     R227            ; Bank Switch Program Segment Register
P3C0R           reg     R252            ; Port 3 control register 0
P3C1R           reg     R253            ; Port 3 control register 1
P3C2R           reg     R254            ; Port 3 control register 2

P4C_PG          equ     3               ; Port 4 control registers page
P4DR            reg     R228            ; Port 4 data register
P4C0R           reg     R240            ; Port 4 control register 0
P4C1R           reg     R241            ; Port 4 control register 1
P4C2R           reg     R242            ; Port 4 control register 2

P5C_PG          equ     3               ; Port 5 control registers page
P5DR            reg     R229            ; Port 5 data register
P5C0R           reg     R244            ; Port 5 control register 0
P5C1R           reg     R245            ; Port 5 control register 1
P5C2R           reg     R246            ; Port 5 control register 2

P6C_PG          equ     3               ; Port 6 control registers page
P6D_PG          equ     3               ; Port 6 data register page
P6DR            reg     R251            ; Port 6 data register
P6C0R           reg     R248            ; Port 6 control register 0
P6C1R           reg     R249            ; Port 6 control register 1
P6C2R           reg     R250            ; Port 6 control register 2

P7C_PG          equ     3               ; Port 7 control registers page
P7D_PG          equ     3               ; Port 7 data register page
P7DR            reg     R255            ; Port 7 data register
P7C0R           reg     R252            ; Port 7 control register 0
P7C1R           reg     R253            ; Port 7 control register 1
P7C2R           reg     R254            ; Port 7 control register 2

P8C_PG          equ     43              ; Port 8 control registers page
P8D_PG          equ     43              ; Port 8 data register page
P8DR            reg     R251            ; Port 8 data register
P8C0R           reg     R248            ; Port 8 control register 0
P8C1R           reg     R249            ; Port 8 control register 1
P8C2R           reg     R250            ; Port 8 control register 2

P9C_PG          equ     43              ; Port 9 control registers page
P9D_PG          equ     43              ; Port 9 data register page
P9DR            reg     R255            ; Port 9 data register
P9C0R           reg     R252            ; Port 9 control register 0
P9C1R           reg     R253            ; Port 9 control register 1
P9C2R           reg     R254            ; Port 9 control register 2

HDCTL2R         reg     R251            ; Port 2 handshake DMA control register
HDCTL3R         reg     R255            ; Port 3 handshake DMA control register
HDCTL4R         reg     R243            ; Port 4 handshake DMA control register
HDCTL5R         reg     R247            ; Port 5 handshake DMA control register

;Handshake DMA control register configuration.
hsdis           equ     0E0h            ; Handshake disabled mask
hso2            equ     0C0h            ; Handshake output 2 lines mask
hso1            equ     040h            ; Handshake output 1 line mask
hsi2            equ     0A0h            ; Handshake input 2 lines mask
hsi1            equ     020h            ; Handshake input 1 line mask
hsb             equ     000h            ; Handshake bidirectional mask
den             equ     000h            ; DMA enable mask
ddi             equ     010h            ; DMA disable mask
ddw             equ     000h            ; Data direction output mask (write)
ddr             equ     008h            ; Data direction input mask (read)
dst             equ     004h            ; DMA strobe on chip event mask
dcp0            equ     000h            ; DMA channel capture0 mask
dcm0            equ     002h            ; DMA channel compare0 mask

;----------------------------------------------------------------------------
; Multi function timer

T0D_PG          equ     10              ; MFTimer 0 data registers page
T0C_PG          equ     9               ; MFTimer 0 control registers page
T1D_PG          equ     8               ; MFTimer 1 data registers page
T1C_PG          equ     9               ; MFTimer 1 control registers page
T2D_PG          equ     14              ; MFTimer 2 data registers page
T2C_PG          equ     13              ; MFTimer 2 control registers page
T3D_PG          equ     12              ; MFTimer 3 data registers page
T3C_PG          equ     13              ; MFTimer 3 control registers page

T_REG0R         reg     RR240           ; MFTimer REG0 load and capture register.
T_REG0HR        reg     R240            ; Register 0 high register
T_REG0LR        reg     R241            ; Register 0 low register
T_REG1R         reg     RR242           ; MFTimer REG1 load constant
T_REG1HR        reg     R242            ; Register 1 high register
T_REG1LR        reg     R243            ; Register 1 low register
T_CMP0R         reg     RR244           ; MFTimer CMP0 store compare constant.
T_CMP0HR        reg     R244            ; Compare 0 high register
T_CMP0LR        reg     R245            ; Compare 0 low register
T_CMP1R         reg     RR246           ; MFTimer CMP1 store compare constant.
T_CMP1HR        reg     R246            ; Compare 1 high register
T_CMP1LR        reg     R247            ; Compare 1 low register

T_TCR           reg     R248             ; MFTimer Control Register.
                __defbit T_cs,T_TCR,0    ; Counter status
                __defbit T_of0,T_TCR,1   ; over/underflow on CAP on REG0
                __defbit T_udcs,T_TCR,2  ; up/down count status
                __defbit T_udc,T_TCR,3   ; up/down count
                __defbit T_ccl,T_TCR,4   ; Counter clear
                __defbit T_ccmp0,T_TCR,5 ; Clear on compare 0
                __defbit T_ccp0,T_TCR,6  ; Clear on capture
                __defbit T_cen,T_TCR,7   ; Counter enable

T_TMR           reg     R249            ; MFTimer Mode Register.
                __defbit T_co,T_TMR,0   ; Continuous/one shot bit
                __defbit T_ren,T_TMR,1  ; retrigger enable bit
                __defbit T_eck,T_TMR,2  ; Enable clocking mode bit
                __defbit T_rm0,T_TMR,3  ; register 0 mode bit
                __defbit T_rm1,T_TMR,4  ; register 1 mode bit
                __defbit T_bm,T_TMR,5   ; bivalue mode bit
                __defbit T_oe0,T_TMR,6  ; output 0 enable bit
                __defbit T_oe1,T_TMR,7  ; output 1 enable bit

T_ICR           reg     R250            ; MFTimer External Input Control Register.

exb_f           equ     01h             ; External B falling edge sensitive mask
exb_r           equ     02h             ; External B rising edge sensitive mask
exb_rf          equ     03h             ; External B falling and rising edge mask
exa_f           equ     04h             ; External A falling edge sensitive mask
exa_r           equ     08h             ; External A rising edge sensitive mask
exa_rf          equ     0Ch             ; External A falling and rising edge mask
ab_ii           equ     00h             ; A I/O B I/O mask
ab_it           equ     10h             ; A I/O B trigger mask
ab_gi           equ     20h             ; A gate B I/O mask
ab_gt           equ     30h             ; A gate B trigger mask
ab_ie           equ     40h             ; A I/O B external clock mask
ab_ti           equ     50h             ; A trigger B I/O mask
ab_ge           equ     60h             ; A gate B external clock mask
ab_tt           equ     70h             ; A trigger B trigger mask
ab_cucd         equ     80h             ; A clock up B clock down mask
ab_ue           equ     90h             ; A clock up/down B external clock mask
ab_tutd         equ     0A0h            ; A trigger up B trigger down mask
ab_ui           equ     0B0h            ; A up/down clock B I/O mask
ab_aa           equ     0C0h            ; A autodiscr. B autodiscr. mask
ab_te           equ     0D0h            ; A trigger B external clock mask
ab_et           equ     0E0h            ; A external clock B trigger mask
ab_tg           equ     0F0h            ; A trigger B gate mask

T_PRSR          reg     R251            ; MFTimer prescaler register

T_OACR          reg     R252            ; MFTimer Output A Control Register.

cev             equ     02h             ; on chip event bit on COMPARE 0 mask

T_OBCR          reg     R253            ; MFTimer Output B Control Register.

op              equ     01h             ; output preset bit mask
oev             equ     02h             ; on chip event bit on OVF/UDF mask
ou_set          equ     00h             ; overflow underflow set mask
ou_tog          equ     04h             ; overflow underflow toggle mask
ou_res          equ     08h             ; overflow underflow reset mask
ou_nop          equ     0Ch             ; overflow underflow nop mask
c1_set          equ     00h             ; Compare 1 set mask
c1_tog          equ     10h             ; Compare 1 toggle mask
c1_res          equ     20h             ; Compare 1 reset mask
c1_nop          equ     30h             ; Compare 1 nop mask
c0_set          equ     00h             ; Compare 0 set mask
c0_tog          equ     40h             ; Compare 0 toggle mask
c0_res          equ     80h             ; Compare 0 reset mask
c0_nop          equ     0C0h            ; Compare 0 nop mask

T_FLAGR         reg     R254            ; MFTimer Flags Register.
                __defbit T_ao,T_FLAGR,0   ; and/or on capture interrupt
                __defbit T_ocm0,T_FLAGR,1 ; overrun compare 0
                __defbit T_ocp0,T_FLAGR,2 ; overrun capture 0
                __defbit T_ouf,T_FLAGR,3  ; overflow underflow flag
                __defbit T_cm1,T_FLAGR,4  ; successful compare 1
                __defbit T_cm0,T_FLAGR,5  ; successful compare 0
                __defbit T_cp1,T_FLAGR,6  ; successful capture 1
                __defbit T_cp0,T_FLAGR,7  ; successful capture 0

T_IDMR          equ     R255            ; MFTimer Interrupt DMA Mask Register.
                __defbit T_oui,T_IDMR,0   ; overflow underflow interrupt
                __defbit T_cm1i,T_IDMR,1  ; Compare 1 interrupt
                __defbit T_cm0i,T_IDMR,2  ; Compare 0 interrupt
                __defbit T_cm0d,T_IDMR,3  ; Compare 0 DMA
                __defbit T_cp1i,T_IDMR,4  ; Capture 1 interrupt
                __defbit T_cp0i,T_IDMR,5  ; Capture 0 interrupt
                __defbit T_cp0d,T_IDMR,6  ; Capture 0 DMA
                __defbit T_gtien,T_IDMR,7 ; global timer interrupt enable

T0_DCPR         reg     R240            ; MFTimer 0 DMA Counter Pointer Register.
T1_DCPR         reg     R244            ; MFTimer 1 DMA Counter Pointer Register.
T0_DAPR         reg     R241            ; MFTimer 0 DMA Address Pointer Register.
T1_DAPR         reg     R245            ; MFTimer 1 DMA Address Pointer Register.
T0_IVR          reg     R242            ; MFTimer 0 Interrupt Vector Register.
T1_IVR          reg     R246            ; MFTimer 1 Interrupt Vector Register.
T0_IDCR         reg     R243            ; MFTimer 0 Interrupt/DMA Control Register.
T1_IDCR         reg     R247            ; MFTimer 1 Interrupt/DMA Control Register.
T2_DCPR         reg     R240            ; MFTimer 2 DMA Counter Pointer Register.
T3_DCPR         reg     R244            ; MFTimer 3 DMA Counter Pointer Register.
T2_DAPR         reg     R241            ; MFTimer 2 DMA Address Pointer Register.
T3_DAPR         reg     R245            ; MFTimer 3 DMA Address Pointer Register.
T2_IVR          reg     R242            ; MFTimer 2 Interrupt Vector Register.
T3_IVR          reg     R246            ; MFTimer 3 Interrupt Vector Register.
T2_IDCR         reg     R243            ; MFTimer 2 Interrupt/DMA Control Register.
T3_IDCR         reg     R247            ; MFTimer 3 Interrupt/DMA Control Register.

plm             equ     07h             ; Priority level mask
swen            equ     08h             ; Swap function enable mask
dctd            equ     10h             ; DMA compare transaction destination mask
dcts            equ     20h             ; DMA capture transaction source mask
cme             equ     40h             ; Compare 0 end of block mask
cpe             equ     80h             ; Capture 0 end of block mask

T_IOCR          reg     R248            ; MFTimer I/O connection register

sc0             equ     01h             ; TxOUTA and TxINA connection bit (even)

sc1             equ     02h             ; TxOUTA and TxINA connection bit (odd)

;----------------------------------------------------------------------------
; A/D-Wandler

                if      MOMCPUNAME<>"ST9020"

AD0_PG          equ     63              ; A/D converter registers page
AD1_PG          equ     62              ; second A/D unit

AD_D0R          reg     R240            ; Channel 0 data register
AD_D1R          reg     R241            ; Channel 1 data register
AD_D2R          reg     R242            ; Channel 2 data register
AD_D3R          reg     R243            ; Channel 3 data register
AD_D4R          reg     R244            ; Channel 4 data register
AD_D5R          reg     R245            ; Channel 5 data register
AD_D6R          reg     R246            ; Channel 6 data register
AD_D7R          reg     R247            ; Channel 7 data register
AD_LT6R         reg     R248            ; Channel 6 lower threshold register
AD_LT7R         reg     R249            ; Channel 7 lower threshold register
AD_UT6R         reg     R250            ; Channel 6 upper threshold register
AD_UT7R         reg     R251            ; Channel 7 upper threshold register

AD_CRR          reg     R252            ; Compare result register
                __defbit AD_c6l,AD_CRR,4 ; Compare channel 6 lower bit
                __defbit AD_c7l,AD_CRR,5 ; Compare channel 7 lower bit
                __defbit AD_c6u,AD_CRR,6 ; Compare channel 6 upper bit
                __defbit AD_c7u,AD_CRR,7 ; Compare channel 7 upper bit

AD_CLR          reg     R253            ; Control logic register
                __defbit AD_st,AD_CLR,0   ; start/stop bit
                __defbit AD_cont,AD_CLR,1 ; Continuous mode
                __defbit AD_pow,AD_CLR,2  ; power up/down control
                __defbit AD_intg,AD_CLR,3 ; internal trigger
                __defbit AD_extg,AD_CLR,4 ; External trigger

sch             equ     0E0h            ; scan channel selection mask

AD_ICR          reg     R254            ; interrupt control register
                __defbit AD_awdi,AD_ICR,4 ; analog watch-dog interrupt
                __defbit AD_eci,AD_ICR,5  ; End of count interrupt
                __defbit AD_awd,AD_ICR,6  ; analog watch-dog pending flag
                __defbit AD_ecv,AD_ICR,7  ; End of conversion pending flag

AD_prl          equ     07h             ; priority level mask

AD_IVR          reg     R255            ; interrupt vector register

                endif

;----------------------------------------------------------------------------
; Serielle Schnittstelle

SCI1_PG         equ     24              ; SCI1 control registers page
SCI2_PG         equ     25              ; SCI2 control registers page
SCI3_PG         equ     26              ; SCI3 control registers page
SCI4_PG         equ     27              ; SCI4 control registers page

S_RDCPR         reg     R240            ; receive DMA counter pointer register
S_RDAPR         reg     R241            ; receive DMA address pointer register
S_TDCPR         reg     R242            ; transmit DMA counter pointer register
S_TDAPR         reg     R243            ; transmit DMA address pointer register
S_IVR           reg     R244            ; interrupt vector register
S_ACR           reg     R245            ; address compare register

S_IMR           reg     R246            ; interrupt mask register
                __defbit S_txdi,S_IMR,0  ; transmitter data interrupt
                __defbit S_rxdi,S_IMR,1  ; receiver data interrupt
                __defbit S_rxb,S_IMR,2   ; receiver break
                __defbit S_rxa,S_IMR,3   ; receiver address
                __defbit S_rxe,S_IMR,4   ; receiver error
                __defbit S_txeob,S_IMR,5 ; transmit end of block
                __defbit S_rxeob,S_IMR,6 ; receive end of block
                __defbit S_hsn,S_IMR,7   ; Holding or shift register empty.

S_ISR           reg     R247            ; interrupt status register
                __defbit S_txsem,S_ISR,0 ; transmit shift register empty
                __defbit S_txhem,S_ISR,1 ; transmit hold register empty
                __defbit S_rxdp,S_ISR,2  ; received data pending bit
                __defbit S_rxbp,S_ISR,3  ; received break pending bit
                __defbit S_rxap,S_ISR,4  ; received address pending bit
                __defbit S_pe,S_ISR,5    ; parity error pending bit
                __defbit S_fe,S_ISR,6    ; framing error pending bit
                __defbit S_oe,S_ISR,7    ; overrun error pending bit

S_RXBR          reg     R248            ; receive buffer register
S_TXBR          reg     R248            ; transmit buffer register

S_IDPR          reg     R249            ; interrupt/DMA priority register
                __defbit S_txd,S_IDPR,3  ; transmitter DMA
                __defbit S_rxd,S_IDPR,4  ; receiver DMA
                __defbit S_sa,S_IDPR,5   ; set address
                __defbit S_sb,S_IDPR,6   ; set break
                __defbit S_amen,S_IDPR,7 ; address mode enable
S_pri           equ     07h             ; interrupt/DMA priority mask

S_CHCR          reg     R250            ; Character configuration register

wl5             equ     000h            ; 5 bits data word mask
wl6             equ     001h            ; 6 bits data word mask
wl7             equ     002h            ; 7 bits data word mask
wl8             equ     003h            ; 8 bits data word mask
sb10            equ     000h            ; 1.0 stop bit mask
sb15            equ     004h            ; 1.5 stop bit mask
sb20            equ     008h            ; 2.0 stop bit mask
sb25            equ     00Ch            ; 2.5 stop bit mask
ab              equ     010h            ; address bit insertion mask
pen             equ     020h            ; parity enable mask
ep              equ     040h            ; Even parity mask
oddp            equ     000h            ; odd parity mask
am              equ     080h            ; address mode mask

S_CCR           reg     R251            ; Clock configuration register
                __defbit S_stpen,S_CCR,0 ; stick parity enable
                __defbit S_lben,S_CCR,1  ; loop back enable
                __defbit S_aen,S_CCR,2   ; auto echo enable
                __defbit S_cd,S_CCR,3    ; Clock divider
                __defbit S_xbrg,S_CCR,4  ; External baud rate generator source
                __defbit S_xrx,S_CCR,5   ; External receiver source
                __defbit S_oclk,S_CCR,6  ; output clock selection
                __defbit S_txclk,S_CCR,7 ; transmit clock selection

S_BRGR          reg     RR252           ; baud rate generator register
S_BRGHR         reg     R252            ; baud rate generator reg. high
S_BRGLR         reg     R253            ; baud rate generator reg. low

;----------------------------------------------------------------------------
; Security Register:

SEC_PG          equ     59              ; Security register page

SECR            reg     R255
                __defbit tlck,SECR,0  ; test lock bit
                __defbit wf1,SECR,1   ; write fuse 1 bit
                __defbit hlck,SECR,2  ; hardware lock bit
                __defbit wf2,SECR,3   ; write fuse 2 bit
                __defbit f2tst,SECR,4 ; select fuse 2 bit
                __defbit slck,SECR,7  ; software lock bit

;----------------------------------------------------------------------------

                endif

                restore                 ; Listing wieder an
