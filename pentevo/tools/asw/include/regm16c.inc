                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGM16C.INC                                            *
;*   								            *
;*   Sinn : enthÑlt Registerdefinitionen fÅr den M16C, Modell M30600M8      *
;* 									    *
;*   letzte énderungen : 30. 8.1996                                         *
;*                        8. 2.1997 if MomPass...                           *
;*                        7. 7.1999 added two more M16C family CPU entries  *
;*                                                                          *
;****************************************************************************

                ifndef  regm16cinc      ; verhindert Mehrfacheinbindung

regm16cinc      equ     1

                if      MOMPASS=1
                 switch  MOMCPUNAME
                 case    "M16C"
                  fatal   "please be more specific; do not use the generic processor type for this header file!"
                 case    "M30600M8"
                  message "including M30600M8-registers"
		 case    "M30610"
		  message "including M30610 registers"
		 case 	 "M30620"
		  message "including M30620 registers"
                 elsecase
                  fatal   "invalid processor type: only M30600M8, M30610, or M30620 allowed!"
                 endcase
                endif

;----------------------------------------------------------------------------
; benîtigte Makros

__bitreg        macro   Name,Adr,Mask
Name            equ     Adr
                irp     BIT,0,1,2,3,4,5,6,7
                if      Mask&(1<<BIT)
Name{"BIT"}     equ     Adr<<3+BIT
                endif
                endm
                endm

;----------------------------------------------------------------------------
; Prozessorsteuerung

                __bitreg PM,0004h,0ffh  ; CPU-Modusregister
                __bitreg CM0,0006h,0ffh ; Systemtaktsteuerung 0
                __bitreg CM1,0007h,02bh ; Systemtaktsteuerung 1
PRCR            equ     000ah           ; Schutzregister
PRC0            equ     PRCR<<3+0       ;  Schreibschutz CM0/CM1
PRC1            equ     PRCR<<3+1       ;  Schreibschutz PM
PRC2            equ     PRCR<<3+2       ;  Schreibschutz PD9

;----------------------------------------------------------------------------
; Ports

P0              equ     03e0h           ; Datenregister Port 0
PD0             equ     03e2h           ; Datenrichtungsregister Port 0

P1              equ     03e1h           ; Datenregister Port 1
PD1             equ     03e3h           ; Datenrichtungsregister Port 1

P2              equ     03e4h           ; Datenregister Port 2
PD2             equ     03e6h           ; Datenrichtungsregister Port 2

P3              equ     03e5h           ; Datenregister Port 3
PD3             equ     03e7h           ; Datenrichtungsregister Port 3

P4              equ     03e8h           ; Datenregister Port 4
PD4             equ     03eah           ; Datenrichtungsregister Port 4

P5              equ     03e9h           ; Datenregister Port 5
PD5             equ     03ebh           ; Datenrichtungsregister Port 5

P6              equ     03ech           ; Datenregister Port 6
PD6             equ     03eeh           ; Datenrichtungsregister Port 6

P7              equ     03edh           ; Datenregister Port 7
PD7             equ     03efh           ; Datenrichtungsregister Port 7

P8              equ     03f0h           ; Datenregister Port 8
PD8             equ     03f2h           ; Datenrichtungsregister Port 8

P9              equ     03f1h           ; Datenregister Port 9
PD9             equ     03f3h           ; Datenrichtungsregister Port 9

P10             equ     03f4h           ; Datenregister Port 10
PD10            equ     03f6h           ; Datenrichtungsregister Port 10

PUR0            equ     03fch           ; Pull-Up-Register
PUR1            equ     03fdh
PUR2            equ     03feh
PUR3            equ     03ffh

;----------------------------------------------------------------------------
; Interruptsteuerung

DM0IC           equ     004bh           ; DMA-Kanal 0
DM1IC           equ     004ch           ; DMA-Kanal 1
KUPIC           equ     004dh           ; Keyboard input
ADIC            equ     004eh           ; A/D-Wandler
S0TIC           equ     0051h           ; Senden UART0
S0RIC           equ     0052h           ; Empfang UART0
S1TIC           equ     0053h           ; Senden UART1
S1RIC           equ     0054h           ; Empfang UART1
TA0IC           equ     0055h           ; Timer A0
TA1IC           equ     0056h           ; Timer A1
TA2IC           equ     0057h           ; Timer A2
TA3IC           equ     0058h           ; Timer A3
TA4IC           equ     0059h           ; Timer A4
TB0IC           equ     005ah           ; Timer B0
TB1IC           equ     005bh           ; Timer B1
TB2IC           equ     005ch           ; Timer B2
INT0IC          equ     005dh           ; externer Imterrupt 0
INT1IC          equ     005eh           ; externer Interrupt 1
INT2IC          equ     005fh           ; externer Interrupt 2

_ILVL0          equ     0               ; PrioritÑtsebene
_ILVL1          equ     1
_ILVL2          equ     2
_IR             equ     3               ; Interrupt aufgetreten ?
_POL            equ     4               ; PolaritÑtswahl (nur INTxIC)
_LVS            equ     5               ; Pegel/Flankentriggerung (nur INTxIC)

;----------------------------------------------------------------------------
; Chip-Selects

CSR             equ     0008h
CS0             equ     CSR<<3+0        ;  Freigabe Chip-Selects
CS1             equ     CSR<<3+1
CS2             equ     CSR<<3+2
CS3             equ     CSR<<3+3
CS0W            equ     CSR<<3+4        ;  mit / ohne Wait-State ?
CS1W            equ     CSR<<3+5
CS2W            equ     CSR<<3+6
CS3W            equ     CSR<<3+7

;----------------------------------------------------------------------------
; Adre·vergleicher

RMAD0           equ     0010h           ; Vergleichsadresse 0
RMAD1           equ     0014h           ; Vergleichsadresse 1
                __bitreg AIER,0009h,03h ; Interrupt-Freigaben

;----------------------------------------------------------------------------
; DMA-Controller

DM0SL           equ     03b8h           ; Request-Auswahl Kanal 0
DM1SL           equ     03bah           ; Request-Auswahl Kanal 1
_DSEL0          equ     0               ; Request-Grund
_DSEL1          equ     1
_DSEL2          equ     2
_DSEL3          equ     3
_DSR            equ     7               ; Software-Trigger

DM0CON          equ     002ch           ; Steuerregister Kanal 0
DM1CON          equ     003ch           ; Steuerregister Kanal 1
_DMBIT          equ     0               ; Transfereinheit (1/2 Byte)
_DMASL          equ     1               ; Einzel/Blocktransfer
_DMAS           equ     2               ; Request-Anzeige
_DMAE           equ     3               ; Freigabe
_DSD            equ     4               ; feste/wandernde Quelladresse
_DAD            equ     5               ; feste/wandernde Zieladresse

SAR0            equ     0020h           ; Quelladresse Kanal 0
DAR0            equ     0024h           ; Zieladresse Kanal 0
TCR0            equ     0028h           ; ZÑhler Kanal 0
SAR1            equ     0030h           ; Quelladresse Kanal 1
DAR1            equ     0034h           ; Zieladresse Kanal 1
TCR1            equ     0038h           ; ZÑhler Kanal 1

;----------------------------------------------------------------------------
; Timer

TA0MR           equ     0396h           ; Modusregister Timer A0
TA1MR           equ     0397h           ; Modusregister Timer A1
TA2MR           equ     0398h           ; Modusregister Timer A2
TA3MR           equ     0399h           ; Modusregister Timer A3
TA4MR           equ     039ah           ; Modusregister Timer A4
TB0MR           equ     039bh           ; Modusregister Timer B0
TB1MR           equ     039ch           ; Modusregister Timer B1
TB2MR           equ     039dh           ; Modusregister Timer B2
_TMOD0          equ     0               ; Moduswahl
_TMOD1          equ     1
_MR0            equ     2               ; Sub-Funktionsauswahl
_MR1            equ     3
_MR2            equ     4
_MR3            equ     5
_TCK0           equ     6               ; ZÑhlquellenwahl
_TCK1           equ     7

TA0             equ     0386h           ; ZÑhlregister Timer A0
TA1             equ     0388h           ; ZÑhlregister Timer A1
TA2             equ     038ah           ; ZÑhlregister Timer A2
TA3             equ     038ch           ; ZÑhlregister Timer A3
TA4             equ     038eh           ; ZÑhlregister Timer A4
TB0             equ     0390h           ; ZÑhlregister Timer B0
TB1             equ     0392h           ; ZÑhlregister Timer B1
TB2             equ     0394h           ; ZÑhlregister Timer B2

TABSR           equ     0380h           ; Start/Stop-Register:
TA0S            equ     TABSR<<3+0      ; Timer A0
TA1S            equ     TABSR<<3+1      ; Timer A1
TA2S            equ     TABSR<<3+2      ; Timer A2
TA3S            equ     TABSR<<3+3      ; Timer A3
TA4S            equ     TABSR<<3+4      ; Timer A4
TB0S            equ     TABSR<<3+5      ; Timer B0
TB1S            equ     TABSR<<3+6      ; Timer B1
TB2S            equ     TABSR<<3+7      ; Timer B2

CPSRF           equ     0381h           ; Vorteilerregister:
CPSR            equ     CPSRF<<3+7      ; Vorteiler zurÅcksetzen

ONSF            equ     0382h           ; Monoflop-Triggerbits
TA0OS           equ     ONSF<<3+0       ; Timer A0
TA1OS           equ     ONSF<<3+1       ; Timer A1
TA2OS           equ     ONSF<<3+2       ; Timer A2
TA3OS           equ     ONSF<<3+3       ; Timer A3
TA4OS           equ     ONSF<<3+4       ; Timer A4
TA0TGL          equ     ONSF<<3+6       ; Triggerwahl Timer A0
TA0TGH          equ     ONSF<<3+7

TRGSR           equ     0383h           ; Triggerwahlregister
TA1TGL          equ     TRGSR<<3+0      ; Triggerwahl Timer A1
TA1TGH          equ     TRGSR<<3+1
TA2TGL          equ     TRGSR<<3+2      ; Triggerwahl Timer A2
TA2TGH          equ     TRGSR<<3+3
TA3TGL          equ     TRGSR<<3+4      ; Triggerwahl Timer A3
TA3TGH          equ     TRGSR<<3+5
TA4TGL          equ     TRGSR<<3+6      ; Triggerwahl Timer A4
TA4TGH          equ     TRGSR<<3+7

UDF             equ     0384h           ; ZÑhlrichtungen(A) / Zweiphasenmodi(A2-4)
TA0UD           equ     UDF<<3+0        ; Timer A0
TA1UD           equ     UDF<<3+1        ; Timer A1
TA2UD           equ     UDF<<3+2        ; Timer A2
TA3UD           equ     UDF<<3+3        ; Timer A3
TA4UD           equ     UDF<<3+4        ; Timer A4
TA2P            equ     UDF<<3+5        ; Timer A2
TA3P            equ     UDF<<3+6        ; Timer A3
TA4P            equ     UDF<<3+7        ; Timer A4

;----------------------------------------------------------------------------
; Watchdog

WDTS            equ     000eh           ; Startwert
                __bitreg WDC,000fh,80h  ; Vorteilerwahl / obere ZÑhler-Bits

;----------------------------------------------------------------------------
; serielle Ports:

U0TB            equ     03a2h           ; Senderegister UART0
U1TB            equ     03aah           ; Senderegister UART1

U0RB            equ     03a6h           ; Empfangsregister UART0
U1RB            equ     03aeh           ; Empfangsregister UART1
_OER            equ     4               ; Overrun-Fehler ?
_FER            equ     5               ; Framing-Fehler ?
_PER            equ     6               ; ParitÑts-Fehler ?
_SUM            equ     7               ; Summenfehler ?

U0BRG           equ     03a1h           ; Baudratengenerator UART0
U1BRG           equ     03a9h           ; Baudratengenerator UART1

U0MR            equ     03a0h           ; Modusregister UART0
U1MR            equ     03a8h           ; Modusregister UART1
_SMD0           equ     0               ; Modus
_SMD1           equ     1
_SMD2           equ     2
_CKDIR          equ     3               ; interner/externer Takt
_STPS           equ     4               ; Anzahl Stopbits
_PRY            equ     5               ; ParitÑtswahl
_PRYE           equ     6               ; ParitÑtsart
_SLEP           equ     7               ; Sleep-Modus

U0C0            equ     03a4h           ; Steuerregister 0 UART0
U1C0            equ     03ach           ; Steuerregister 1 UART0
_CLK0           equ     0               ; Taktquelle Baudratengenerator
_CLK1           equ     1
_CRS            equ     2               ; RTS/CTS-Auswahl
_TXEPT          equ     3               ; Senderegister voll ?
_CRD            equ     4               ; CTS/RTS-Auswahl
_NCH            equ     5               ; TXD als Open-Collector ?
_CKPOL          equ     6               ; TaktpolaritÑtswahl
_UFORM          equ     7               ; Bitreihenfolge

U0C1            equ     03a5h           ; Steuerregister 1 UART0
U1C1            equ     03adh           ; Steuerregister 1 UART1
_TE             equ     0               ; Senderfreigabe
_TI             equ     1               ; Sendepuffer leer ?
_RE             equ     2               ; EmpfÑngerfreigabe
_RI             equ     3               ; Empfangspuffer voll ?

UCON            equ     03b0h           ; Steuerregister 2 UART0+1
U0IRS           equ     UCON<<3+0       ; Interruptgrund Sender UART0
U1IRS           equ     UCON<<3+1       ; Interruptgrund Sender UART1
U0RRM           equ     UCON<<3+2       ; kontinuierlicher Empfangsmodus UART0
U1RRM           equ     UCON<<3+3       ; kontinuierlicher Empfangsmodus UART1
CLKMD0          equ     UCON<<3+4       ; Taktausgabe an CLK1/CLKS1
CLKMD1          equ     UCON<<3+5
RCSP            equ     UCON<<3+6       ; RTS/CTS gemeinsam/einzeln

;----------------------------------------------------------------------------
; CRC-Generator

CRCD            equ     03bch           ; Ergebnis CRC-Berechnung

CRCIN           equ     03beh           ; Datumseingabe CRC-Berechnung

;----------------------------------------------------------------------------
; A/D-Wandler

AD0             equ     03c0h           ; Datenregister Kanal 0
AD1             equ     03c2h           ; Datenregister Kanal 1
AD2             equ     03c4h           ; Datenregister Kanal 2
AD3             equ     03c6h           ; Datenregister Kanal 3
AD4             equ     03c8h           ; Datenregister Kanal 4
AD5             equ     03cah           ; Datenregister Kanal 5
AD6             equ     03cch           ; Datenregister Kanal 6
AD7             equ     03ceh           ; Datenregister Kanal 7

ADCON0          equ     03d6h           ; A/D-Steuerregister 0
CH0             equ     ADCON0<<3+0     ; Kanalwahl
CH1             equ     ADCON0<<3+1
CH2             equ     ADCON0<<3+2
MD0             equ     ADCON0<<3+3     ; Modus
MD1             equ     ADCON0<<3+4
TRG             equ     ADCON0<<3+5     ; Soft/Hardwaretrigger
ADST            equ     ADCON0<<3+6     ; Konversion starten
CKS0            equ     ADCON0<<3+7     ; Frequenzwahl

ADCON1          equ     03d7h           ; A/D-Steuerregister 1
SCAN0           equ     ADCON1<<3+0     ; Sweep-Modus
SCAN1           equ     ADCON1<<3+1
MD2             equ     ADCON1<<3+2     ; Modus
BITS            equ     ADCON1<<3+3     ; Breitenwahl (8/10 Bit)
VCUT            equ     ADCON1<<3+5
OPA0            equ     ADCON1<<3+6
OPA1            equ     ADCON1<<3+7

ADCON2		equ	03d4h
SMP		equ	ADCON2<<3+0	; A-D Conversion method bit

;----------------------------------------------------------------------------
; D/A-Wandler

DA0             equ     03d8h           ; Wert D/A-Wandler 0
DA1             equ     03dah           ; Wert D/A-Wandler 1

DACON           equ     03dch           ; Steuerregister D/A-Wandler
DA0E            equ     DACON<<3+0      ; D/A-Wandler 0 freigeben
DA1E            equ     DACON<<3+1      ; D/A-Wandler 1 freigeben

;----------------------------------------------------------------------------
; feste Vektoren

Vec_UndefInstr  equ     0fffdch         ; UND ausgefÅhrt
Vec_Overflow    equ     0fffe0h         ; INTO+Overflow ausgefÅhrt
Vec_BRK         equ     0fffe4h         ; BRK ausgefÅhrt
Vec_AddrMatch   equ     0fffe8h         ; Adressengleichheit aufgetreten
Vec_SingleStep  equ     0fffech         ; Einzelschrittverarbeitung
Vec_WDT         equ     0ffff0h         ; Watchdog hat zugeschlagen
Vec_DBC         equ     0ffff4h
Vec_NMI         equ     0ffff8h         ; nichtmaskierbarer Interrupt
Vec_Reset       equ     0ffffch

;----------------------------------------------------------------------------
; Variable Vektoren; hier Offset zu INTB

Vec_BRK2        equ     0               ; BRK ausgefÅhrt (falls fester Vektor abgeschaltet)
Vec_DMA0        equ     44
Vec_DMA1        equ     48
Vec_KeyInp      equ     52
Vec_AD          equ     56
Vec_UART0T      equ     68
Vec_UART0R      equ     72
Vec_UART1T      equ     76
Vec_UART1R      equ     80
Vec_TA0         equ     84
Vec_TA1         equ     88
Vec_TA2         equ     92
Vec_TA3         equ     96
Vec_TA4         equ     100
Vec_TB0         equ     104
Vec_TB1         equ     108
Vec_TB2         equ     112
Vec_INT0        equ     116
Vec_INT1        equ     120
Vec_INT2        equ     124

;----------------------------------------------------------------------------
; Speicherbereiche

                switch  MOMCPUNAME
                case    "M30600M8"
IROM            equ     0f0000h         ; 64K ROM am Ende des Speicherraums
IROMEND         equ     0fffffh
IRAM            equ     000400h         ; 10K RAM hinter SFRs
IRAMEND         equ     002c00h
		case	"M30610"	; 128k ROM
IROM		equ	0e0000h
IROMEND		equ	0fffffh
IRAM		equ	000400h
IRAMEND		equ	002c00h		; 10k RAM
		case	"M30620"
IROM            equ     0e0000h
IROMEND         equ     0fffffh
IRAM            equ     000400h
IRAMEND         equ     002c00h         ; 10k RAM
                endcase

;----------------------------------------------------------------------------

                endif

                restore         ; Listing wieder an

