                save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei 80C50x.INC                                             *
;*   								            *
;*   Sinn : enthÑlt SFR- und Bitdefinitionen fÅr die SAB C50x-Prozessoren   *
;           (Ableitung aus STDDEF51.INC)                                    *
;* 									    *
;*   letzte énderungen : 26. 3.1997 Grundsteinlegung                        *
;*                        8. 6.1997 Korrekturen von Herrn Schmid            * 
;*                                                                          *
;****************************************************************************

                ifndef  reg50xinc       ; verhindert Mehrfacheinbindung

reg50xinc       equ     1

                if      (MOMCPUNAME<>"80C501")&&(MOMCPUNAME<>"80C502")&&(MOMCPUNAME<>"80C504")
                 fatal  "Falscher Prozessortyp eingestellt: nur 80C501, 80C502 oder 80C504 erlaubt!"
		endif


                if      MOMPASS=1
                 message "SAB C50x-SFR-Definitionen (C) 1993 Alfred Arnold, Steffen Schmid"
                 message "binde \{MOMCPUNAME}-SFRs ein"
                endif

;----------------------------------------------------------------------------
; Prozessorkern

ACC             SFRB    0e0h            ; Akkumulator
B               SFRB    0f0h            ; Hilfsakku fÅr MUL/DIV
SP              SFR     81h             ; Stapelzeiger
DPL             SFR     82h             ; Datenzeiger Bit 0..7
DPH             SFR     83h             ;      "      Bit 8..15
PSW             SFRB    0d0h            ; Statuswort
CY              BIT     PSW.7
AC              BIT     PSW.6
F0              BIT     PSW.5
RS1             BIT     PSW.4
RS0             BIT     PSW.3
OV              BIT     PSW.2
P               BIT     PSW.0
SYSCON          SFR     0b1h            ; Systemkonfiguration
PCON            SFR     87h             ; "Power-Management"
PCON1           SFRB    88h
EWPD            BIT     PCON1.7         ; Freigabe Wake-Up durch INT0

                if      MOMCPUNAME="80C502"
XPAGE            SFR     91h            ; XRAM-Ansteuerung 80C502
XCON             SFR     94h
DPSEL            SFR     92h
                endif

;----------------------------------------------------------------------------
; Ports

P0              SFRB    80h             ; I/O-Ports
P1		SFRB	90h
P2              SFRB    0a0h
P3		SFRB	0b0h
T2EX            BIT     P1.1
T2              BIT     P1.0
                if      MOMCPUNAME="80C504"
P1ANA            SFRB   90h             ; Selektion Port 1/3 als analoge Eingabe
P3ANA            SFRB   0b0h
EAN0             BIT    P1ANA.0
EAN1             BIT    P1ANA.1
EAN2             BIT    P1ANA.2
EAN3             BIT    P1ANA.3
EAN4             BIT    P1ANA.2
EAN5             BIT    P1ANA.3
EAN6             BIT    P1ANA.4
EAN7             BIT    P1ANA.5
                endif
RD              BIT     P3.7            ; Port 3: Schreibleitung
WR		BIT	P3.6		;         Leseleitung
T1              BIT     P3.5            ;         Testleitung 1
T0		BIT	P3.4		;	  Testleitung 0
INT1		BIT	P3.3		;         ext. Interrupt 1
INT0		BIT	P3.2		;	  ext. Interrupt 0
TXD		BIT	P3.1		;	  ser. Ausgang
RXD		BIT	P3.0		;	  ser. Eingang

;----------------------------------------------------------------------------
; serielles

SCON		SFRB	98h		; ser. Schnittstelle: Steuerregister
SM0		BIT     SCON.7          ; Betriebsarten
SM1		BIT     SCON.6
SM2		BIT     SCON.5
REN		BIT	SCON.4		; Empfangssperre
TB8		BIT	SCON.3		; 9. zu sendendes Bit
RB8		BIT	SCON.2		; 9. empfangenes Bit
TI		BIT	SCON.1		; Senderinterruptflag
RI		BIT	SCON.0		; EmpfÑngerinterruptflag
SBUF		SFR	99h		;  "         "        Datenregister
                if      MOMCPUNAME="80C502"
SRELL            SFR     0aah           ; Wert fÅr Baudratengenerator
SRELH            SFR     0bah
BAUD             SFRB    0d8h
BD               BIT     BAUD.7         ; Baudratengenerator aktivieren
                ENDIF

;----------------------------------------------------------------------------
; Timer

TCON		SFRB	88h		; Timer 0/1 Steuerregister
TF1		BIT	TCON.7		; öberlauf Timer 1
TR1		BIT	TCON.6		; Timer 1 laufenlassen
TF0		BIT	TCON.5		; öberlauf Timer 0
TR0		BIT	TCON.4		; Timer 0 laufenlassen
IE1		BIT	TCON.3		; Externer Interrupt 1 aufgetreten
IT1		BIT	TCON.2		; Einstellung Triggerflanke externer Interrupt 1
IE0		BIT	TCON.1		; Externer Interrupt 0 aufgetreten
IT0		BIT	TCON.0		; Einstellung Triggerflanke externer Interrupt 0
TMOD		SFR 	89h		; Timer 0/1 Betriebsartenregister
TL0		SFR	8ah		; Daten Timer 0
TL1		SFR	8bh
TH0		SFR	8ch		; Daten Timer 1
TH1		SFR	8dh
T2CON           SFRB    0c8h            ; Steuerregister Timer 2
TL2             SFR     0cch            ; Daten Timer 2
TH2             SFR     0cdh
RC2L            SFR     0cah            ; Capture-Register
RC2H            SFR     0cbh
TF2             BIT     T2CON.7         ; Overflow Timer 2
EXF2            BIT     T2CON.6         ; Reload aufgetreten
RCLK            BIT     T2CON.5         ; Timer 2 liefert RxD-Takt
TCLK            BIT     T2CON.4         ; Timer 2 liefert TxD-Takt
EXEN2           BIT     T2CON.3         ; externer Enable Timer 2
TR2             BIT     T2CON.2         ; Timer 2 starten
CT2             BIT     T2CON.1         ; Timer 2 als ZÑhler
CPRL2           BIT     T2CON.0         ; Capture erlauben

;---------------------------------------------------------------------------
; Watchdog

                if      MOMCPU>=80C504h
WDCON            SFRB    0c0h           ; Watchdog Steuerung
SWDT             BIT     WDCON.0        ; Watchdog starten
WDT              BIT     WDCON.1        ; Watchdog zurÅcksetzen
WDTS             BIT     WDCON.2        ; Watchdog-Reset-Flagge
OWDS             BIT     WDCON.3        ; dito Oszillator-Watchdog
WDTREL           SFR     86h
                endif

;---------------------------------------------------------------------------
; Capture/Compare-Einheit

                if      MOMCPUNAME="80C504"
CT1CON           SFR     0e1h           ; Steuerung Vergleicher 1
CCPL             SFR     0deh           ; Periode Vergleicher 1
CCPH             SFR     0dfh
CT1OFL           SFR     0e6h           ; Offset Vergleicher 1
CT1OFH           SFR     0e7h
CMSEL0           SFR     0e3h           ; Modus
CMSEL1           SFR     0e4h
COINI            SFR     0e2h           ; Initialisierung Vergleicherausgang
TRCON            SFR     0cfh           ; Trap-Freigabe
CCL0             SFR     0c2h           ; Vergleichs-/Fangwert 0
CCH0             SFR     0c3h
CCL1             SFR     0c4h           ; Vergleichs-/Fangwert 1
CCH1             SFR     0c5h
CCL2             SFR     0c6h           ; Vergleichs-/Fangwert 2
CCH2             SFR     0c7h
CCIR             SFR     0e5h           ; Interruptflags
CCIE             SFR     0d6h           ; Interruptmasken
CT2CON           SFR     0c1h           ; Steuerung Vergleicher 2
CP2L             SFR     0d2h           ; Periode Vergleicher 2
CP2H             SFR     0d3h
CMP2L            SFR     0d4h           ; Vergleichs-/Fangwert Timer 2
CMP2H            SFR     0d5h
BCON             SFR     0d7h           ; Steuerung Blockkommunikation
                endif

;---------------------------------------------------------------------------
; A/D-Wandler

                if      MOMCPUNAME="80C504"
ADCON0           SFRB    0d8h           ; Konfiguration
IADC             BIT     ADCON0.5       ; Interruptflagge (Ende Konversion)
BSY              BIT     ADCON0.4       ; Busy-Flag
ADM              BIT     ADCON0.3       ; einfache/kontinuierliche Wandlung
MX2              BIT     ADCON0.2       ; Kanalwahl
MX1              BIT     ADCON0.1
MX0              BIT     ADCON0.0
ADCON1           SFR     0dch
ADDATH           SFR     0d9h           ; Daten
ADDATL           SFR     0dah
		endif

;-------------------------------------------------------------------------
; Interruptsteuerregister:

IEN0            SFRB    0a8h            ; Freigaben
IE              SFRB    IEN0
EA              BIT     IEN0.7          ; generelle Sperre
ET2             BIT     IEN0.5          ; Timer 2
ES              BIT     IEN0.4          ; ser. Schnittstelle
ET1             BIT     IEN0.3          ; öberlauf Timer 1
EX1             BIT     IEN0.2          ; ext. Interrupt 1
ET0             BIT     IEN0.1          ; Timer 0 öberlauf
EX0             BIT     IEN0.0          ; ext. Interrupt 0
IP0             SFRB    0b8h            ; PrioritÑten
IP              SFRB    IP0
PT2             BIT     IP0.5           ; siehe Freigabebits
PS              BIT     IP0.4
PT1             BIT     IP0.3
PX1             BIT     IP0.2
PT0             BIT     IP0.1
PX0             BIT     IP0.0
                if      MOMCPUNAME="80C504"
IEN1             SFR     0a9h
IP1              SFR     0b9h
ITCON            SFR     09ah           ; Triggerbedingungen
                endif

;---------------------------------------------------------------------------
; Da man die Register beim 8051 nicht direkt pushen und poppen kann, mu· man
; dies Åber direkte Adressen tun. Da dazu die Kenntnis der gewÑhlten Regis-
; terbank erforderlich ist, steht hier das Makro USING zur VerfÅgung, wel-
; ches in den Symbolen AR0..AR7 die momentanen Registeradressen ablegt.
; USING erwartet als Parameter die gewÑhlte Banknummer.

Bank0		equ	0		; nur der Schînheit halber...
Bank1		equ	1
Bank2		equ	2
Bank3		equ	3

using		macro	bank
		if	(bank<0)||(bank>3)          ; nur 0..3 erlaubt
                 error  "Falsche Banknummer: \{BANK}"
                endif

                ifdef	RegUsage	; BuchfÅhrung Åber benutzte BÑnke
RegUsage	 set	RegUsage|(2^bank)
		elseif
RegUsage	 set	2^bank
		endif

ar0		set	bank*8		; Symbole besetzen
ar1		set	ar0+1
ar2		set	ar0+2
ar3		set	ar0+3
ar4		set	ar0+4
ar5		set	ar0+5
ar6		set	ar0+6
ar7		set	ar0+7
		endm

                endif

                restore                 ; wieder erlauben

