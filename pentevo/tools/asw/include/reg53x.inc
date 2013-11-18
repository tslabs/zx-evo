                save
;                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG53x.INC                                             *
;*   								            *
;*   Sinn : enthÑlt Register- und Adre·definitionen fÅr H8/53x              *
;* 									    *
;*   letzte énderungen : 12.11.1995                                         *
;*                                                                          *
;****************************************************************************

		ifndef  reg53xinc       ; verhindert Mehrfacheinbindung

reg53xinc       equ     1

                if      (MOMCPUNAME<>"HD6475328")&&(MOMCPUNAME<>"HD6475348")&&(MOMCPUNAME<>"HD6475368")&&(MOMCPUNAME<>"HD6475388")
                 fatal  "Falscher Prozessortyp eingestellt: nur HD6475328, HD6475348, HD6475368 oder HD6475388 erlaubt!"
		endif


                if      MOMPASS=1
		 message "H8/53x-Definitionen (C) 1994 Alfred Arnold"
		endif

__cpunum        equ     MOMCPU-$6475000 ; ist halt bequemer...

		if	__cpunum=$328	; Peripherie verschiebt sich beim
__regbase	 equ	$ff00		; H8/532 um eine Seite nach oben
                elseif
__regbase	 equ	$fe00
		endif

;----------------------------------------------------------------------------
; Adressen:

		if 	__cpunum=$328
IRAM		 equ	$fb80		; Anfang internes RAM (1 kByte)
IRAMEND		 equ	$ff7f		; Ende internes RAM
                elseif
IRAM		 equ	$f680		; Anfang internes RAM (2 kByte)
IRAMEND		 equ	$fe7f		; Ende internes RAM
		endif

IROM		equ	$0000		; Anfang internes ROM
                if	__cpunum=$368
IROMEND		 equ	$f67f		; Ende internes ROM (62 kByte)
                elseif
IROMEND		 equ	$7fff		; Ende internes ROM (32 kByte)
		endif


;----------------------------------------------------------------------------
; CPU-Konfiguration:

		if	__cpunum=$328
__sysbase	 equ	$fff8
                elseif
__sysbase	 equ	$ff10
		endif

WCR		equ	__sysbase+0		; Wait-state control register
WC0		equ	0		; Anzahl Wartezyklen (rw)
WC1		equ	1
WMS0		equ	2		; Wartemodus (rw)
WMS1		equ	3

RAMCR		equ	__sysbase+1	; RAM control register
RAME		equ	7		; Freigabe internes RAM

MDCR		equ	__sysbase+2	; Mode control register
MDS0		equ	0		; Modusauswahl (r)
MDS1		equ	1
MDS2		equ	2

SBYCR		equ	__sysbase+3	; Software standby control register
SSBY		equ	7		; Sleep/Standby-Moduswahl (rw)

;----------------------------------------------------------------------------
; Interruptsteuerung:

		if	__cpunum=$328
IPRA		 equ	$fff0		; Interrupt-PrioritÑt IRQ1 / IRQ0 (rw)
IPRB		 equ	$fff1		; Interrupt-PrioritÑt FRT2 / FRT1 (rw)
IPRC		 equ	$fff2		; Interrupt-PrioritÑt 8-Bit-Timer / FRT3 (rw)
IPRD		 equ	$fff3		; Interrupt-PrioritÑt AD-Wandler / SCI (rw)
                elseif
IPRA		 equ	$ff00		; Interrupt-PrioritÑt IRQ1 / IRQ0|I.-Timer (rw)
IPRB		 equ	$ff01		; Interrupt-PrioritÑt IRQ5|IRQ4 / IRQ3|IRQ2 (rw)
IPRC		 equ	$ff02		; Interrupt-PrioritÑt FRT2 / FRT1 (rw)
IPRD		 equ	$ff03		; Interrupt-PrioritÑt 8-Bit-Timer / FTR3 (rw)
IPRE		 equ	$ff04		; Interrupt-PrioritÑt SCI2 / SCI1 (rw)
IPRF		 equ	$ff05		; Interrupt-PrioritÑt - / AD-Wandler
                endif

;----------------------------------------------------------------------------
; Data Transfer Controller:

		if	__cpunum=$328
DTEA		 equ	$fff4		; Umschaltung normaler/DTC-Interrupt
DTEB		 equ	$fff5		; (Belegung der Nibbles analog zu IPRx)
DTEC		 equ	$fff6
DTED		 equ	$fff7
                elseif
DTEA		 equ	$ff08
DTEB		 equ	$ff09
DTEC		 equ	$ff0a
DTED		 equ	$ff0b
DTEE		 equ	$ff0c
DTEF		 equ	$ff0d
                endif

;----------------------------------------------------------------------------
; Ports:

__portbase	equ	__regbase+$80

P1DR		equ	__portbase+2	; Datenregister Port 1 (rw/r)
P1DDR		equ	__portbase+0	; Datenrichtungsregister Port 1 (w)
		if	__cpunum=$328
P1CR		 equ	$fffc		; Steuerregister Port 1 (rw)
                elseif
SYSCR1		 equ	$fefc		; Systemsteuerregister 1 (rw)
		endif
BRLE		equ	3		; P1CR/SYSCR1: Bus-Release-Modus
NMIEG		equ	4		;              Flanke, auf die NMI-Eingang triggert
IRQ0E		equ	5		;              Nutzung P15 als IRQ0 ?
IRQ1E		equ	6		;              Nutzung P16 als IRQ1 ?

P2DR		equ	__portbase+3	; Datenregister Port 2 (rw)
P2DDR		equ	__portbase+1	; Datenrichtungsregister Port 2 (w)

P3DR		equ	__portbase+6	; Datenregister Port 3 (rw)
P3DDR		equ	__portbase+4	; Datenrichtungsregister Port 3 (w)

P4DR		equ	__portbase+7	; Datenregister Port 4 (rw)
P4DDR		equ	__portbase+5	; Datenrichtungsregister Port 4 (w)

P5DR		equ	__portbase+10	; Datenregister Port 5 (rw)
P5DDR		equ	__portbase+8	; Datenrichtungsregister Port 5 (w)

P6DR		equ	__portbase+11	; Datenregister Port 6 (rw)
P6DDR		equ	__portbase+9	; Datenrichtungsregister Port 6 (w)
		if	__cpunum<>$328
SYSCR2		 equ	$fefd		; Systemsteuerregister 2 (w)
P9SCI2E		 equ	0		; P92..P94 fÅr SCI2 nutzen ?
P9PWME		 equ	1		; P92..P94 fÅr PWM nutzen ?
P6PWME		 equ	2		; P61..P63 als PWM-Ausgang nutzen ?
IRQ2E		 equ	3		; Nutzung P60 als IRQ2 ?
IRQ3E		 equ	4		; Nutzung P61 als IRQ3 ?
IRQ4E		 equ	5		; Nutzung P62 als IRQ4 ?
IRQ5E		 equ	6		; Nutzung P63 als IRQ5 ?
                endif

P7DR            equ     __portbase+14   ; Datenregister Port 7 (rw)
P7DDR		equ	__portbase+12	; Datenrichtungsregister Port 7 (w)

P8DR		equ	__portbase+15	; Datenregister Port 8 (r)

                if      __cpunum=$388
P9DR             equ    __portbase+18   ; Datenregister Port 9 (r)

PADR             equ    __portbase+19   ; Datenregister Port A (rw)
PADDR            equ    __portbase+17   ; Datenrichtungsregister Port A (w)

PBDR             equ    __portbase+22   ; Datenregister Port B (rw)
PBDDR            equ    __portbase+20   ; Datenrichtungsregister Port B (w)
PBPCR            equ    __portbase+24   ; Pullup-Register Port B (w)

PCDR             equ    __portbase+23   ; Datenregister Port C (rw)
PCDDR            equ    __portbase+21   ; Datenrichtungsregister Port C (w)
PCPCR            equ    __portbase+25   ; Pullup-Register Port C (w)
                elseif  __cpunum=$328
P9DR             equ    __portbase+$7f  ; Datenregister Port 9 (rw)
P9DDR            equ    __portbase+$7e  ; Datenrichtungsregister Port 9 (w)
                elseif
P9DR             equ    __portbase+$ff  ; Datenregister Port 9 (rw)
P9DDR            equ    __portbase+$fe  ; Datenrichtungsregister Port 9 (w)
                endif

;----------------------------------------------------------------------------
; Timer:

__deftimer      macro   Base,NAME
TCR{NAME}	equ	Base+0		; Steuerregister (rw)
TCSR{NAME}	equ	Base+1		; Steuer/Statusregister (rw/r)
FRC{NAME}	equ	Base+2		; ZÑhlerregister (rw, 16 Bit)
OCRA{NAME}	equ	Base+4		; Vergleicher A (rw, 16 Bit)
OCRB{NAME}	equ	Base+6		; Vergleicher B (rw, 16 Bit)
ICR{NAME}	equ	Base+8		; Eingabe-Fangregister (r, 16 Bit)
		endm

		__deftimer __regbase+$90,"1"
		__deftimer __regbase+$a0,"2"
		__deftimer __regbase+$b0,"3"

CKS0		equ	0		; TCRx: Taktquellenwahl (rw)
CKS1		equ	1
OEA		equ	2		;       Vergleichsergebnis OCRA ausgeben ? (rw)
OEB		equ	3		;       Vergleichsergebnis OCRB ausgeben ? (rw)
OVIE		equ	4		;       Interrupt bei FRC-öberlauf auslîsen? (rw)
OCIEA		equ	5		;       Interrupt bei Gleichheit mit OCRA auslîsen? (rw)
OCIEB		equ	6		;       Interrupt bei Gleichheit mit OCRB auslîsen? (rw)
ICIE		equ	7		;       Interrupt bei ICR-Fang auslîsen? (rw)

CCLRA		equ	0		; TCSRx: FRC bei Gleichheit mit OCRA rÅcksetzen ? (rw)
IEDG		equ	1		;	 Eingabefang bei welcher Flanke ? (rw)
OLVLA		equ	2		;	 PolaritÑt fÅr OCRA-Ausgangssignal (rw)
OLVLB		equ	3		;	 PolaritÑt fÅr OCRB-Ausgangssignal (rw)
OVF		equ	4		;	 FRC-öberlauf aufgetreten ? (r)
OCFA		equ	5		;        war FRC=OCRA ? (r)
OCFB		equ	6		;        war FRC=OCRB ? (r)
ICF		equ	7		;        Eingabe-Fang aufgetreten ? (r)

; 8-Bit-Timer:

__tcntbase	equ	__regbase+$d0

TCR4		equ	__tcntbase+0	; Steuerregister (rw)
CKS2		equ	2		; Taktauswahl (Rest identisch zu FRCx)
CCLR0		equ	3		; RÅcksetzmodus
CCLR1		equ	4
OVIE_4		equ	5		; Interrupt bei öberlauf auslîsen ?
CMIEA		equ	6		; Interrupt bei TCNT=TCORA?
CMIEB		equ	7		; Interrupt bei TCNT=TCORB?

TCSR4		equ	__tcntbase+1	; Steuer/Statusregister (rw/r)
OS0		equ	0		; Auswirkung von TCORA=TCNT auf TMO
OS1		equ	1
OS2		equ	2		; Auswirkung von TCORB=TCNT auf TMO
OS3		equ	3
OVF_4		equ	5		; öberlauf aufgetreten
CMFA		equ	6		; TCNT=TCORA aufgetreten
CMFB		equ	7		; TCNT=TCORB aufgetreten

TCORA		equ	__tcntbase+2	; Zeitkonstante 1 (rw)
TCORB		equ	__tcntbase+3	; Zeitkonstante 2 (rw)
TCNT		equ	__tcntbase+4	; ZÑhlwert (rw)

;----------------------------------------------------------------------------
; Pulsweitenmodulatoren:

__defpwm        macro   Base,NAME
PW{NAME}_TCR	equ	Base		; Steuerregister (rw)
PW{NAME}_DTR	equ	Base+1		; Pulsweitenregister (rw)
PW{NAME}_TCNT	equ	Base+2		; ZÑhlwert (rw)
		endm

		__defpwm __regbase+$c0,"1"
		__defpwm __regbase+$c4,"2"
		__defpwm __regbase+$c8,"3"

OS		equ	6		; PWx_TCR: Logik des Ausgangs
OE		equ	7		;          Ausgang freischalten

;----------------------------------------------------------------------------
; Watchdog:

__wdtbase	equ	__regbase+$ec

WDT_TCSR_R	equ	__wdtbase+0	; Steuer/Statusregister (unterschied-
WDT_TCSR_W	equ	__wdtbase+1	; liche Schreib/Leseadressen)
TME		equ	5		; Timer freigeben
WTIT		equ	6		; Watchdog/Timermodus
WDT_OVF		equ	7		; Watchdog-öberlauf ?

WDT_TCNT	equ	__wdtbase+1	; ZÑhlregister (rw)

		if	__cpunum<>$328
RSTCSR_W	 equ	$ff14		; Reset-Register (unterschiedliche
RSTCSR_R	 equ	$ff15		; Schreib/Leseadressen)
RSTOE		 equ	6		; Watchdog-Reset nach au·en geben ?
WRST		 equ	7               ; Reset durch Watchdog ?
                endif

;----------------------------------------------------------------------------
; serielle Schnittstelle:

__defsci        macro   Base,NAME
SMR{NAME}	equ	Base+0		; Modusregister (rw)
BRR{NAME}	equ	Base+1		; Bitratenregister (rw)
SCR{NAME}	equ	Base+2		; Steuerregister (rw)
TDR{NAME}	equ	Base+3		; Sendedaten (rw)
SSR{NAME}	equ	Base+4		; Statusregister (rw)
RDR{NAME}	equ	Base+5		; Empfangsdaten (r)
		endm

		if	__cpunum=$328
		 __defsci __regbase+$d8,""
                elseif
		 __defsci __regbase+$d8,"1"
		 __defsci __regbase+$f0,"2"
		endif

STOP		equ	3		; SMRx: #Stopp-Bits
O_E		equ	4		;       odd/even Parity
PE		equ	5		;       mit/ohne ParitÑt
CHR		equ	6		;       7/8 Datenbits
C_A		equ	7		;       synchron/asynchron

CKE0		equ	0		; SCRx: bei synchroner Kommunikation Takt an SCK ausgeben ?
CKE1		equ	1		;       Takt intern/extern (-->SCK Ein-oder Ausgang) ?
RE		equ	4		;       EmpfÑnger freigeben
TE		equ	5		;       Sender freigeben
RIE		equ	6		;       Empfangs-Interrupt freigeben
TIE		equ	7		;       Sende-Interrupt freigeben

PER		equ	3		; SSRx: ParitÑts-Fehler
FER		equ	4		;       ungÅltiger Rahmen
ORER		equ	5		;       EmpfÑnger-öberlauf
RDRF		equ     6		;       EmpfÑnger voll ?
TDRE		equ	7		;       Sender leer ?

;----------------------------------------------------------------------------
; A/D-Wandler

__adbase	equ	__regbase+$e0

ADDRA		equ	__adbase+0	; Wandelergebnis Kanal A (r, 16Bit)
ADDRB		equ	__adbase+2	; Wandelergebnis Kanal B (r, 16Bit)
ADDRC		equ	__adbase+4	; Wandelergebnis Kanal C (r, 16Bit)
ADDRD		equ	__adbase+6	; Wandelergebnis Kanal D (r, 16Bit)

ADCSR		equ	__adbase+8	; Steuer/Statusregister (rw)
CH0		equ	0		; Kanalauswahl
CH1		equ	1
CH2		equ	2
CKS		equ	3		; Wandelzeit 274/138 Takte
SCAN		equ	4		; Single Channel/Scan-Modus
ADST		equ	5		; Wandler starten/stoppen
ADIE		equ	6		; Interrupt bei Wandelende auslîsen ?
ADF		equ	7		; Wandlung abgeschlossen ?

		if	__cpunum<>$328
ADCR		 equ	__adbase+9	; Steuerregister (rw)
TRGE             equ    7               ; externer Trigger ?
		endif

;----------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben
