                save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei STDDEF51.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR- und Bitdefinitionen fÅr die MCS-51-Prozessoren     *
;* 									    *
;*   letzte énderungen :  7. 1.1993                                         *
;*                       14. 1.1993 USING-Makro hinzugefÅgt   		    *
;*                       26. 1.1993 REGUSAGE-Variable ergÑnzt               *
;*			 21. 1.1993 Erzeugerhinweis			    *
;*                        5. 6.1993 IPC & IEC hinzugefÅgt                   *
;*                                  SAVE & RESTORE                          *
;*                                  Prozessorsperre (Idee von Matthias)     *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*                       12. 5.1994 DS80C320-Register hinzugefÅgt           *
;*                       25. 6.1995 87C750 hinzugefÅgt                      *
;*                       22. 3.1996 SFR's fur 80517 eingefÅgt               *
;*                                  (von Gabriel Jager)                     *
;*                       14. 6.1996 80517-Korrekturen wg. Klammerung        *
;*                       26. 3.1997 Abfrage auf MOMCPU                      *
;*                       19. 9.2000 added P6 for 515/517                    *
;*                                                                          *
;****************************************************************************

		ifndef  stddef51inc     ; verhindert Mehrfacheinbindung

stddef51inc     equ     1

                if      (MOMCPUNAME<>"87C750")&&(MOMCPUNAME<>"8051")&&(MOMCPUNAME<>"8052")&&(MOMCPUNAME<>"80C320")&&(MOMCPUNAME<>"80515")&&(MOMCPUNAME<>"80517")
                 fatal  "Falscher Prozessortyp eingestellt: nur 87C750, 8051, 8052, 80C320, 80515 oder 80517 erlaubt!"
		endif


                if      MOMPASS=1
                 message "MCS-51-SFR-Definitionen (C) 1993 Alfred Arnold/Gabriel Jager"
		 message "binde \{MOMCPU}-SFRs ein"
		endif

;----------------------------------------------------------------------------
; erstmal die Sachen, die es (fast) Åberall gibt :

P0              SFRB    80h             ; I/O-Ports
P1		SFRB	90h
P2              SFRB    0a0h
P3		SFRB	0b0h
RD		BIT	P3.7		; Port 3: Schreibleitung
WR		BIT	P3.6		;         Leseleitung
T1              BIT     P3.5            ;   Testleitung 1
T0		BIT	P3.4		;	  Testleitung 0
INT1		BIT	P3.3		;         ext. Interrupt 1
INT0		BIT	P3.2		;	  ext. Interrupt 0
TXD		BIT	P3.1		;	  ser. Ausgang
RXD		BIT	P3.0		;	  ser. Eingang
                if      MOMCPU=80C320H
TXD1		 BIT	P1.3		; zweiter ser. Ausgang
RXD1		 BIT	P1.2		; zweiter ser. Eingang
		endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

SP		SFR	81h		; Stapelzeiger
DPL		SFR	82h		; Datenzeiger Bit 0..7
DPH		SFR	83h		;      "      Bit 8..15
                if      MOMCPU=80C320H
DPL0		 SFR	DPL
DPH0		 SFR	DPH
DPL1		 SFR	84h		; zweiter Datenzeiger 80C320
DPH1		 SFR	DPL1+1
DPS		 SFR	86h		; Bit 0=Umschaltung DPTR0<-->DPTR1
		endif
PSW		SFRB	0d0h		; Statuswort
CY		BIT	PSW.7
AC		BIT	PSW.6
F0		BIT	PSW.5
RS1		BIT	PSW.4
RS0		BIT	PSW.3
OV		BIT	PSW.2
P		BIT	PSW.0
ACC		SFRB	0e0h		; Akkumulator
B		SFRB	0f0h		; Hilfsakku fÅr MUL/DIV

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

                if      MOMCPU=80C320H  ; Register zweite serielle Schnittstelle
SCON0		 SFR	SCON
SBUF0		 SFR    SBUF
SCON1		 SFR	0c0h		; Steuerregister
SBUF1            SFR    0c1h            ; Datenregister
		endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

PCON		SFR	87h		; "Power-Management"

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

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

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; kein Timer 2 beim 8051

		if	MOMCPU<>8051h
T2CON	 	 SFRB	0c8h		; Steuerregister Timer 2
TL2	 	 SFR	0cch		; Daten Timer 2
TH2	 	 SFR	0cdh
		 if	MOMCPU=8052h
RCAP2L	 	  SFR	0cah		; Capture-Register
RCAP2H	 	  SFR	0cbh
TF2		  BIT	T2CON.7		; Overflow Timer 2
EXF2		  BIT	T2CON.6		; Reload aufgetreten
RCLK		  BIT	T2CON.5		; Timer 2 liefert RxD-Takt
TCLK		  BIT	T2CON.4		; Timer 2 liefert TxD-Takt
EXEN2		  BIT	T2CON.3		; externer Enable Timer 2
TR2		  BIT	T2CON.2		; Timer 2 starten
CT2		  BIT	T2CON.1		; Timer 2 als ZÑhler
CPRL2		  BIT	T2CON.0		; Capture erlauben
		 elseif
CRCL		  SFR   0cah		; andere Namen beim 80515 !!
CRCH		  SFR	0cbh
T2PS		  BIT	T2CON.7
I3FR		  BIT	T2CON.6
I2FR		  BIT	T2CON.5
T2R1		  BIT	T2CON.4
T2R0		  BIT	T2CON.3
T2CM		  BIT	T2CON.2
T2I1		  BIT	T2CON.1
T2I0		  BIT	T2CON.0
		 endif
		endif

                if      MOMCPU=80C320H  ; Taktauswahl beim 80C320
CKCON		 SFR	8eh		; Bit 3,4,5 <--> Timer 0,1,2
		endif			; Bit 6,7 <--> Watchdog-Timeout

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; 80C320-Watchdog

                if      MOMCPU=80C320h
WDCON            SFRB   0d8h
RWT		 BIT 	WDCON.0		; Watchdog zurÅcksetzen
EWT		 BIT	WDCON.1		; Watchdog freigeben
WTRF		 BIT	WDCON.2		; Watchdog-Reset aufgetreten
WDIF		 BIT	WDCON.3		; Interrupt 512 Takte bis Reset aufgetreten
PFI		 BIT	WDCON.4		; Power-Fail-Interrupt aufgetreten ?
EPFI		 BIT	WDCON.5		; Power-Fail-Interrupt freigeben
POR		 BIT	WDCON.6
WD_SMOD		 BIT	WDCON.7
TA		 SFR	0c7h		; nacheinander AA 55 schreiben, um Zugriff auf
		endif			; Spezialregister zu bekommen

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
;
                          
                if      MOMCPU=80C320H
SADDR0           SFR    0a9h            ; Slave-Adresse fÅr seriellen Port 0
SADDR1           SFR    0aah            ; Slave-Adresse fÅr seriellen Port 1
SADEN0           SFR    0b9h            ; Bits in SADDR0 freigeben
SADEN1           SFR    0bah            ; Bits in SADDR1 freigeben
		endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; zusÑtzliche 80515/80517-Register

                if      (MOMCPU=80515h)||(MOMCPU=80517h)
P4		 SFRB	0e8h
P5		 SFRB   0f8h
P6               SFR    0DBh

CCEN		 SFR	0c1h
CCH3		 SFR	0c7h
CCL3		 SFR	0c6h
CCH2		 SFR	0c5h
CCL2		 SFR	0c4h
CCH1		 SFR	0c3h
CCL1		 SFR	0c2h

ADCON            SFRB   0d8h            ; andere Namen beim 80515/80517
BD		 BIT	ADCON.7
CLK		 BIT    ADCON.6
BSY		 BIT	ADCON.4
ADM		 BIT	ADCON.3
MX2		 BIT	ADCON.2
MX1		 BIT	ADCON.1
MX0		 BIT	ADCON.0
ADDAT		 SFR	0d9h
DAPR             SFR    0dah
		endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; zusÑtzliche 80517-Register

               if      MOMCPU=80517h

DPSEL            SFR    92h             ;Datenpointer Select
ADCON1           SFR    0dch            ;Kontrolle 1 Åber A/D-Wandler
CTCON            SFR    0e1h            ;Kontrolle Åber Compare-Timer
IEN2             SFR    9ah
ARCON            SFR    0efh
MD0              SFR    0e9h            ;]      Multiplikations-
MD1              SFR    0eah            ;]      und
MD2              SFR    0ebh            ;]      Divisionsregister
MD3              SFR    0ech            ;]
MD4              SFR    0edh            ;]      1 - 5
MD5              SFR    0eeh            ;]
CC4EN            SFR    0c9h
CCH4             SFR    0cfh
CCL4             SFR    0ceh
CMEN             SFR    0f6h
CMH0             SFR    0d3h
CMH1             SFR    0d5h
CMH2             SFR    0d7h
CMH3             SFR    0e3h
CMH4             SFR    0e5h
CMH5             SFR    0e7h
CMH6             SFR    0f3h
CMH7             SFR    0f5h
CML0             SFR    0d2h
CML1             SFR    0d4h
CML2             SFR    0d6h
CML3             SFR    0e2h
CML4             SFR    0e4h
CML5             SFR    0e6h
CML6             SFR    0f8h
CML7             SFR    0f4h
CMSEL            SFR    0f7h
CTRELH           SFR    0dfh
CTRELL           SFR    0deh
P6               SFR    0fah            ; ??? bitadressierbar
P7               SFR    0dbh            ; ??? bitadressierbar
P8               SFR    0ddH            ; ??? bitadressierbar
ADCON0           SFR    0d8h            ;Kontrolle 1 Åber A/D-Wandler
S0BUF            SFR    99h             ;]      Kontrolle
S0CON            SFR    98h             ;]      fÅr
S1BUF            SFR    9ch             ;]      die
S1CON            SFR    9bh             ;]      serielle
S1REL            SFR    9dh             ;]      Schnittstelle
WDTREL           SFR    86h             ;]      0 und 1

                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Interruptsteuerregister: nicht kompatibel zwischen 8051/52 und 80515/80517 !!!

                if      (MOMCPU=80515h)||(MOMCPU=80517h)
IEN0		 SFRB	0a8h
EAL              BIT    IEN0.7
WDT		 BIT	IEN0.6
ET2		 BIT	IEN0.5
ES		 BIT	IEN0.4
ET1		 BIT	IEN0.3
EX1		 BIT	IEN0.2
ET0		 BIT	IEN0.1
EX0		 BIT	IEN0.0
IEN1		 SFRB	0b8h
EXEN2		 BIT    IEN1.7
SWDT		 BIT	IEN1.6
EX6		 BIT	IEN1.5
EX5		 BIT	IEN1.4
EX4		 BIT	IEN1.3
EX3		 BIT	IEN1.2
EX2		 BIT	IEN1.1
EADC		 BIT	IEN1.0
IP0              SFR    0a9h
IP1              SFR    0b9h
IRCON		 SFRB	0c0h
EXF2             BIT    IRCON.7
TF2              BIT    IRCON.6
IEX6             BIT    IRCON.5
IEX5             BIT    IRCON.4
IEX4             BIT    IRCON.3
IEX3             BIT    IRCON.2
IEX2             BIT    IRCON.1
IADC             BIT    IRCON.0
		elseif
IE		 SFRB	0a8h		; Interruptfreigaben
IEC              SFRB   IE
EA		 BIT	IE.7		; generelle Interruptsperre
ES		 BIT    IE.4		; Interrupts ser. Schnittstelle freigeben
ET1		 BIT	IE.3		; Interrupt Timer 1 freigeben
EX1		 BIT	IE.2		; externen Interrupt 1 freigeben
ET0		 BIT	IE.1		; Interrupt Timer 0 freigeben
EX0		 BIT	IE.0		; externen Interrupt 0 freigeben
IP		 SFRB	0b8h		; InterruptprioritÑten
IPC              SFRB   IP
PS		 BIT	IP.4		; PrioritÑt serieller Interrupt
PT1		 BIT	IP.3		; PrioritÑt Interrupt Timer 1
PX1		 BIT	IP.2		; PrioritÑt externer Interrupt 1
PT0		 BIT	IP.1		; PrioritÑt Interrupt Timer 0
PX0		 BIT	IP.0		; PrioritÑt externer Interrupt 0
                 if     MOMCPU=8052h
ET2		  BIT	IE.5		; Interrupt Timer 2 freigeben
PT2		  BIT	IP.5		; PrioritÑt Interrupt Timer 2
		 endif
		endif

                if      MOMCPU=80C320H  ; erweiterte Interrupts 80C320
EIE              SFRB   0e8h
EWDI		 BIT 	EIE.4		; Watchdog-Interrupt freigeben
EX5		 BIT	EIE.3		; externe Interrupts 2..5 freigeben
EX4		 BIT	EIE.2
EX3		 BIT	EIE.1
EX2		 BIT	EIE.0
EIP              SFRB   0f8h
PWDI		 BIT 	EIP.4		; PrioritÑt Watchdog-Interrupt
PX5		 BIT	EIP.3		; PrioritÑt externer Interrupts 2..5
PX4		 BIT	EIP.2
PX3		 BIT	EIP.1
PX2		 BIT	EIP.0
EXIF		 SFR	91h		; erweitertes Interrupt-Flag-Register
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

