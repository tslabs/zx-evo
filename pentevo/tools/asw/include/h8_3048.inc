                save
                listing off
; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG3048.INC                                            *
;*   								            *
;*   Sinn : enthÑlt SFR-, Makro- und Adreadefinitionen fÅr H8/3048          *
;* 									    *
;*   letzte énderungen :  24.10.1995                                        *
;*                                                                          *
;****************************************************************************

                ifndef  reg3048inc       ; verhindert Mehrfacheinbindung

reg532inc       equ     1

                if      (MOMCPUNAME<>"HD6413309")&&(MOMCPUNAME<>"H8/300H")
                 fatal  "Falscher Prozessortyp eingestellt: nur H8/300H erlaubt!"
		endif


                if      MOMPASS=1
                 message "H8/3048-SFR-Definitionen, (C) 1995 Christian Stelter"
		endif


;-----------------------------------------------------------------------------
; MCU-Operating-Modes: (Sec.3 p.55-68 & Sec.20 p.615-628)


MDCR            equ     $fff1           ; Arbeitsmodus CPU
SYSCR           equ     $fff2           ; Standby-Modusregister
MSTCR           equ     $ff5e           ; Module standby control register

;MDCR-Register

MD0             equ     0
MD1             equ     1
MD2             equ     2


;SYSCR-Register

SSBY            equ     7               ; Software-Standby
STS2            equ     6               ; Standby-Timer Select
STS1            equ     5
STS0            equ     4
UE              equ     3               ; User bit enable
NMIEG           equ     2               ; NMI-edge
RAME            equ     0               ; internes RAM freigeben


;MSTCR-Register

PSTOP           equ     7               ; Phi-clock stop
MSTOP5          equ     5               ; Module standby
MSTOP4          equ     4
MSTOP3          equ     3
MSTOP2          equ     2
MSTOP1          equ     1
MSTOP0          equ     0






;-----------------------------------------------------------------------------
; Bus-Controller (Sec.6 p.107-142)


ABWCR           equ     $ffec           ; Bus width control register

ASTCR           equ     $ffed           ; Access state control register

WCR             equ     $ffee           ; Wait control register
WMS0            equ     2               ; Modus
WMS1            equ     3
WC0             equ     0               ; Anzahl Waitstates
WC1             equ     1

WCER            equ     $ffef           ; Wait state controller enable reg.

BRCR            equ     $fff3           ; Bus release control register
A23E            equ     7               ; Address 23 enable
A22E            equ     6               ;         22
A21E            equ     5               ;         21
BRLE            equ     0               ; Bus release enable


CSCR            equ     $ff5f           ; Chip select control register
CS7E            equ     7               ; Chip-select 7 enabel
CS6E            equ     6
CS5E            equ     5
CS4E            equ     4





;-----------------------------------------------------------------------------
; Interrupt-Controller:

ISCR            equ     $fff4           ; IRQ sense control register
IER             equ     $fff5           ; IRQ enable register
ISR             equ     $fff6           ; IRQ status register
IPRA            equ     $fff8           ; PrioritÑtssteuerung
IPRB            equ     $fff9           ;


;-----------------------------------------------------------------------------
; Lage Exception und Interrupt-Vektoren: (Sec.4 p.69-78)
;

__defvec        macro   Name,Num
Name             equ     Num<<2
                endm

                __defvec Reset,0
                __defvec NMI,7
                __defvec TRAP0,8
                __defvec TRAP1,9
                __defvec TRAP2,10
                __defvec TRAP3,11
                __defvec IRQ0,12
                __defvec IRQ1,13
                __defvec IRQ2,14
                __defvec IRQ3,15
                __defvec IRQ4,16
                __defvec IRQ5,17
                __defvec WOVI,20
                __defvec CMI,21
                __defvec IMIA0,24
                __defvec IMIB0,25
                __defvec OVI0,26
                __defvec IMIA1,28
                __defvec IMIB1,29
                __defvec OVI1,30
                __defvec IMIA2,32
                __defvec IMIB2,33
                __defvec OVI2,34
                __defvec IMIA3,36
                __defvec IMIB3,37
                __defvec OVI3,38
                __defvec IMIA4,40
                __defvec IMIB4,41
                __defvec OVI4,42
                __defvec DEND0A,44
                ;__defvec DEND0A,45
                __defvec DEND1B,46
                ;__defvec DEND1B,47
                __defvec ERI0,52
                __defvec RXI0,53
                __defvec TXI0,54
                __defvec TEI0,55
                __defvec ERI1,56
                __defvec RXI1,57
                __defvec TXI1,58
                __defvec TEI1,59
                __defvec ADI,60


;-----------------------------------------------------------------------------
; DMA-Controller (Sec.6 p.181-238)

DTEA            equ     $fff4           ; Freigabe Datentransfers
DTEB            equ     $fff5
DTEC            equ     $fff6
DTED            equ     $fff7

__defdma        macro   Base,Name
MAR{Name}AR     equ     Base            ; Memory address register AR
MAR{Name}ER     equ     Base+1          ; Memory address register AE
MAR{Name}AL     equ     Base+2          ; Memory address register AL
MAR{Name}AH     equ     Base+3          ; Memory address register AH
ETCR{Name}AH    equ     Base+4          ; Execute transfer count register AH
ETCR{Name}AL    equ     Base+5          ;                                 AL
IOAR{Name}A     equ     Base+6          ; I/O address register A
DTCR{Name}A     equ     Base+7          ; Data transfer control register A
MAR{Name}BR     equ     Base+8          ; Memory address register BR
MAR{Name}BE     equ     Base+9          ; Memory address register BE
MAR{Name}BH     equ     Base+10         ; Memory address register BH
MAR{Name}BL     equ     Base+11         ; Memory address register BL
ETCR{Name}BH    equ     Base+12         ; Excute transfer count register BH
ETCR{Name}BL    equ     Base+13         ; Excute transfer count register BL
IOAR{Name}B     equ     Base+14         ; I/O address register B
DTCR{Name}B     equ     Base+15         ; Data transfer control register
                endm

                __defdma $ff20,"0"
                __defdma $ff30,"1"



; DTCR-Register

; short address-mode
DTE             equ     7               ; Data transfer enable
DTSZ            equ     6               ; Data transfer size
DTID            equ     5               ; Data transfer inc/dec
RPE             equ     4               ; Repeat enable 
DTIE            equ     3               ; Data transfer interrupt enable
DTS2            equ     2               ; Data transfer select
DTS1            equ     1
DTS0            equ     0

; full address mode
SAID            equ     5               ; Source address inc/dec
SAIE            equ     4               ; Source address inc/dec enable
DTS2A           equ     2               ; Data transfer select
DTS1A           equ     1
DTS0A           equ     0

; DTCRB-Register
DTME            equ     7               ; Data transfer master enable
DAID            equ     5               ; Destination address inc/dec bit
DAIE            equ     4               ;                             enable
TMS             equ     3               ; Transfer mode select
DTS2B           equ     2               ; Data transfer select
DTS1B           equ     1
DTS0B           equ     0




;-----------------------------------------------------------------------------
; I/O-Ports: (Sec.9 p.239-280)

P1DDR           equ     $ffc0           ; Datenrichtung Port 1
P1DR            equ     $ffc2           ; Daten Port 1

P2DDR           equ     $ffc1           ; Datenrichtung Port 2
P2DR            equ     $ffc3           ; Daten Port 2
P2PCR           equ     $ffd8           ; Input pull-up control register port 3

P3DDR           equ     $ffc4           ; Datenrichtung Port 3
P3DR            equ     $ffc6           ; Daten Port 3

P4DDR           equ     $ffc5           ; Datenrichtung Port 4
P4DR            equ     $ffc7           ; Daten Port 4
P4PCR           equ     $ffda           ; Input pull-up control register port 4

P5DDR           equ     $ffc8           ; Datenrichtung Port 5
P5DR            equ     $ffca           ; Daten Port 5
P5PCR           equ     $ffcb           ; Input pull-up control register port 5

P6DDR           equ     $ffc9           ; Datenrichtung Port 6
P6DR            equ     $ffcb           ; Daten Port 6

P8DDR           equ     $ffcd           ; Datenrichtung Port 8
P8DR            equ     $ffcf           ; Daten Port 8

P9DDR           equ     $ffd0           ; Datenrichtung Port 9
P9DR            equ     $ffd2           ; Daten Port 9

PADDR           equ     $ffd1           ; Datenrichtung Port A
PADR            equ     $ffd3           ; Daten Port A

PBDDR           equ     $ffd4           ; Datenrichtung Port B
PBDR            equ     $ffd6           ; Daten Port B

;------------------------------------------------------------------------------
;Integrated Timer Unit (ITU): (Sec.10 p.281-380)



;common
TSTR            equ     $ff60           ; Timer start register
TSNC            equ     $ff61           ; Timer synchro register
TMDR            equ     $ff62           ; Timer mode register
TFCR            equ     $ff63           ; Timer function control register
TOER            equ     $ff90           ; Timer output master enable register
TOCR            equ     $ff91           ; Timer output control register



__deftimer      macro   Base,Name
TCR{Name}       equ     Base            ; Timer control register
TIOR{Name}      equ     Base+1          ; Timer I/O control register
TIER{Name}      equ     Base+2          ; Timer interrupt enable register
TSR{Name}       equ     Base+3          ; Timer status register
TCNT{Name}H     equ     Base+4          ; Timer counter H
TCNT{Name}L     equ     Base+5          ; Timer counter L
GRA{Name}H      equ     Base+6          ; General register A (high)
GRA{Name}L      equ     Base+7          ; General register A (low)
GRB{Name}H      equ     Base+8          ; General register B (high)
GRB{Name}L      equ     Base+9          ; General register B (low)
                endm

                __deftimer $ff64,"0"
                __deftimer $ff6e,"1"
                __deftimer $ff78,"2"
                __deftimer $ff82,"3"

BRA3H           equ     $ff8c           ; Buffer register A3 (high)
BRA3L           equ     $ff8d           ; Buffer register A3 (low)
BRB3H           equ     $ff8e           ; Buffer register B3 (high)
BRB3L           equ     $ff8f           ; Buffer register B3 (low)

                __deftimer $ff82,"4"

BRA4H           equ     $ff9c           ; Buffer register A4 (high)
BRA4L           equ     $ff9d           ; Buffer register A4 (low)
BRB4H           equ     $ff9e           ; Buffer register B4 (high)
BRB4L           equ     $ff9f           ; Buffer register B4 (low)




;TMDR-Register

MDF             equ     6               ; Phase counting mode flag
FDIR            equ     5               ; Flag direction
PWM4            equ     4               ; PWM mode
PWM3            equ     3
PWM2            equ     2
PWM1            equ     1
PWM0            equ     0


;TFCR-Register

CMD1            equ     5               ; Combination mode
CMD0            equ     4
BFB4            equ     3               ; Buffer mode B4
BFA4            equ     2               ; Buffer mode A4
BFB3            equ     1               ; Buffer mode B3
BFA3            equ     0               ; Buffer mode A3


;TOER-Register

EXB4            equ     5               ; Master enable TOCXB4
EXA4            equ     4               ; Master enable TOCXA4
EB3             equ     3               ; Master enable TIOCB3
EB4             equ     2               ; Master enable TIOCB4
EA4             equ     1               ; Master enable TIOCA4
EA3             equ     0               ; Master enable TIOCA3


;TOCR-Register

XTGD            equ     4               ; External trigger disable
OLS4            equ     1               ; Output level select 4
OLS3            equ     0               ; Output level select 3


;TCR-Register

CCLR1           equ     6               ; Counter clear
CCLR0           equ     5
CKEG1           equ     4               ; Counter edge
CKEG0           equ     3
TPSC2           equ     2               ; Timer prescaler
TPSC1           equ     1
TPSC0           equ     0


;TIOR-Register

IOB2            equ     6               ; I/O control B2
IOB1            equ     5               ; I/O control B1
IOB0            equ     4               ; I/O control B0
IOA2            equ     2               ; I/O control A2
IOA1            equ     1               ; I/O control A1
IOA0            equ     0               ; I/O control A0


;TSR-Register

OVF             equ     2               ; Overflow flag
IMFB            equ     1               ; Input capture / compare match flag B
IMFA            equ     0               ; Input capture / compare match flag A


;TIER-Register

OVIE            equ     2               ; Overflow interrupt enable
IMIEB           equ     1               ; Input capture / compare match interrupt enable B
IMIEA           equ     0               ; Input capture / compare match interrupt enable A

;-----------------------------------------------------------------------------
;Programmable Timing Pattern Controller (Sec.11 p.381-406)

TPMR            equ     $ffa0           ; TPC output mode register
TPCR            equ     $ffa1           ; TPC output control register
NDERB           equ     $ffa2           ; Next data enable register B
NDERA           equ     $ffa3           ; Next data enable register A
NDRA            equ     $ffa5           ; Next data register A
NDRB            equ     $ffa4           ; Next data register B
NDRA1           equ     $ffa5           ; Next data register A group 1
NDRA0           equ     $ffa7           ; Next data register A group 0
NDRB3           equ     $ffa4           ; Next data register B group 3
NDRB2           equ     $ffa6           ; Next data register B group 2

;-----------------------------------------------------------------------------
; Watchdog: (Sec.12 p.407-422)

WDT_TCSR        equ     $ffa8           ; Timer control/status register
WDT_TCNT        equ     $ffa9           ; Timer counter
WDT_RSTCSR      equ     $ffab           ; Reset control/status register
WDT_RSTCSRW     equ     $ffaa           ; dito, zum setzen wordzugriffe (p.415)


;TCSR-Register

WDT_OVF         equ     7               ; Overflow Flag
WDT_WTIT        equ     6               ; Timer mode select
WDT_TME         equ     5               ; Timer enable
WDT_CKS2        equ     2               ; Clock select
WDT_CKS1        equ     1
WDT_CKS0        equ     0


;RSTCSR-Register

WDT_WRST        equ     7               ; Watchdog timer reset
WDT_RSTOE       equ     6               ; Reset output enable


;-----------------------------------------------------------------------------
; serielle Schnittstelle: (Sec.13 p.423-482)
__defSCI        macro   Base,Name
SMR{Name}       equ     Base            ; Serial mode register
BRR{Name}       equ     Base+1          ; Bit rate register
SCR{Name}       equ     Base+2          ; Serial control register
TDR{Name}       equ     Base+3          ; Transmit data register
SSR{Name}       equ     Base+4          ; Serial status register
RDR{Name}       equ     Base+5          ; Receive data register
                endm

                __defSCI $ffb0,"0"
                __defSCI $ffb8,"1"

;SMR-Register

CA              equ     7               ; Communication mode
CHR             equ     6               ; Character length
PE              equ     5               ; Parity enable
OE              equ     4               ; Parity mode
STOP            equ     3               ; Stop bit length
MP              equ     2               ; Multiprocessor mode
CKS1            equ     1               ; Clock select 1
CKS0            equ     0


;SCR-Register

TIE             equ     7               ; Transmit interrupt enable
RIE             equ     6               ; Receive      "        "
TE              equ     5               ; Transmit enable
RE              equ     4               ; Receive enable
MPIE            equ     3               ; Multiprozessor interrupt enable
TEIE            equ     2               ; Transmit-end interrupt enable
CKE1            equ     1               ; Clock enable 1
CKE0            equ     0               ;


;SSR-Register

TDRE            equ     7               ; Transmit data register empty
RDRF            equ     6               ; Receive data register full
ORER            equ     5               ; Overrun error
FER             equ     4               ; Framing error
PER             equ     3               ; Parity error
TEND            equ     2               ; Transmit end
MPB             equ     1               ; Multiprocessor bit
MPBT            equ     0               ; Multiprocessor bit transfer



;-----------------------------------------------------------------------------
;Smart Card interface



;not implemented yet



;-----------------------------------------------------------------------------
; A/D-Wandler: (Sec.15 p.505-526)

ADDRA           equ     $ffe0
ADDRAH          equ     $ffe0           ;
ADDRAL          equ     $ffe1           ;

ADDRB           equ     $ffe2
ADDRBH          equ     $ffe2
ADDRBL          equ     $ffe3

ADDRC           equ     $ffe4
ADDRCH          equ     $ffe4
ADDRCL          equ     $ffe5


ADDRD           equ     $ffe6
ADDRDH          equ     $ffe6
ADDRDL          equ     $ffe7



ADCSR           equ     $ffe8           ; Steuer/Statusregister:

ADF             equ     7               ; Wandlung abgeschlossen
ADIE            equ     6               ; Interrupt bei Wandelende?
ADST            equ     5               ; Wandlung starten
SCAN            equ     4               ; Scan-Modus
CKS             equ     3               ; Wandlungszeit
CH2             equ     2               ; Kanalauswahl
CH1             equ     1
CH0             equ     0


ADCR            equ     $ffe9           ; A/D control register

TRGE            equ     7               ; Trigger enable


;-----------------------------------------------------------------------------
;D/A-Wandler (Sec.16 p.527-533)

DADR0           equ     $ffdc           ; D/A data register 0
DADR1           equ     $ffdd           ; D/A data register 1
DACR            equ     $ffde           ; D/A control register
DASTCR          equ     $ff5c           ; D/A standby control register


;DACR-Register

DAOE1           equ     7               ; D/A output enable
DAOE0           equ     6
DAE             equ     5               ; D/A enable


;DASTCR-Register

DASTE           equ     0               ; D/A standby enable




;-----------------------------------------------------------------------------
;Clock-Pulse Generator (Sec.19 p.607-614)

DIVCR           equ     $ff5d           ; Divison control register


DIV1            equ     1
DIV0            equ     0



;-----------------------------------------------------------------------------
;-----------------------------------------------------------------------------
                endif
                restore

