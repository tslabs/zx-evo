		save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF18.INC                                           *
;*   								            *
;*   Sinn : enthÑlt die SFR-Definitionen fÅr die PIC 16C8x-Familie    	    *
;*          Die Makrodefinitionen kann man bei Bedarf aus STDDEF16.INC      *
;*          kopieren							    *
;* 									    *
;*   letzte énderungen : 14.11.1993                                         *
;*                       27. 2.1993 PIC16C64-Register                       *
;*                                  IND0-->INDF umbenannt                   *
;*									    *
;****************************************************************************

		ifndef  stddef18inc     ; verhindert Mehrfacheinbindung

stddef18inc     equ     1

                if      (MOMCPU<93316)&&(MOMCPU<>93284)
                 fatal  "Falscher Prozessortyp eingestellt: nur 16C84 oder 16C64 erlaubt"
		endif

                if      MOMPASS=1
		 message "PIC16C8x-Definitionen (C) 1993 Alfred Arnold"
                 message "binde \{MOMCPU}-Register ein"
		endif

;----------------------------------------------------------------------------
; erstmal die Spezialregister :

INDF            SFR     0               ; indirekte Adressierung
PCL             SFR     2               ; PC Bits 0..7
PCLATH          SFR     10              ; obere HÑlfte ProgrammzÑhler
Status          SFR     3               ; Status-Register
FSR             SFR     4               ; File-Select-Register
OPTION          SFR     $81             ; Chipkonfiguration
                if      MOMCPU=$16C64
PCON             SFR    $8e
                endif

;----------------------------------------------------------------------------
; Ports

PortA           SFR     5               ; Datenregister Port A
TrisA           SFR     $85             ; Datenrichtungsregister Port A
PortB           SFR     6               ; Datenregister Port C
TrisB           SFR     $86             ; Datenrichtungsregister Port B
                if      MOMCPU=$16C64
PortC            SFR    7               ; Datenregister Port C
TrisC            SFR    $87             ; Datenrichtungsregister Port C
PortD            SFR    8               ; Datenregister Port D
TrisD            SFR    $88             ; Datenrichtungsregister Port D
PortE            SFR    9               ; Datenregister Port E
TrisE            SFR    $89             ; Datenrichtungsregister Port E
                endif

;----------------------------------------------------------------------------
; Timer

                if      MOMCPU=$16C64
TMR0             SFR    1               ; ZÑhler 0
TMR1L            SFR    14              ; ZÑhler 1
TMR1H            SFR    15
TMR2             SFR    17              ; ZÑhler 2
CCPR1L           SFR    $15             ; Vergleichs/Pulsweitenregister
CCPR1H           SFR    $16
T1CON            SFR    16              ; Steuerung ZÑhler 1
T2CON            SFR    18              ; Steuerung ZÑhler 2
PR2              SFR    $92             ; Periode ZÑhler 2
CCP1CON          SFR    $17             ; Modus Vergleicher
                elseif
RTCC             SFR    1               ; ZÑhler
                endif

;----------------------------------------------------------------------------
; serielle Schnittstelle

                if      MOMCPU=$16C64
SSPBuf           SFR    $13             ; serielles Datenregister
SSPCon           SFR    $14             ; Konfiguration
SSPAD0           SFR    $93             ; I2C-Adre·register
SSPSTAT          SFR    $94             ; Schnittstellenstatus
                endif

;----------------------------------------------------------------------------
; EEPROM

                if      MOMCPU=$16C84
EEData           SFR    8               ; EEPROM Datenregister
EEAdr            SFR    9               ; EEPROM Adre·register
EECON1           SFR    $88             ; Konfiguration EEPROM
EECON2           SFR    $89
                endif

;----------------------------------------------------------------------------
; Interrupts

INTCON          SFR     11              ; Interruptsteuerung
                if      MOMCPU=$16C64
PIR              SFR    12              ; Interrupts aufgetreten ?
PIE              SFR    $8c             ; Interruptfreigabe
                endif

;============================================================================
; Bits im Statusregister

Flag_C		EQU	0		; Carry
Flag_DC		EQU	1		; Digit-Carry
Flag_Z		EQU	2		; Zero
Flag_PD		EQU	3		; Aufgewacht aus SLEEP-Befehl ?
Flag_TO		EQU	4		; Durch Watchdog zurÅckgesetzt ?
Flag_RP0	EQU	5		; Registerbank Bit 8
                if      MOMCPU=$16C84
Flag_RP1         EQU    6               ; Registerbank Bit 9
Flag_IRP         EQU    7               ; Registerbnak Bit 9 (indirekt)
                endif

;----------------------------------------------------------------------------
; Bits in TrisE

                if      MOMCPU=$16C64
TRISE0           EQU    0               ; Datenrichtung RE0
TRISE1           EQU    1               ; Datenrichtung RE1
TRISE2           EQU    2               ; Datenrichtung RE2
PSPMODE          EQU    4               ; Modus Port D&E
IBOV             EQU    5               ; öberlauf Eingabepuffer
OBF              EQU    6               ; Ausgabepuffer voll
IBF              EQU    7               ; Eingabepuffer voll
                endif

;----------------------------------------------------------------------------
; Bits in T1CON

                if      MOMCPU=$16C64
TMR1ON           EQU    0               ; Timer 1 starten
TMR1CS           EQU    1               ; Timer 1 mit internem/externem Takt
T1INSYNC         EQU    2               ; Timer 1 extern synchronisieren
T1OSCEN          EQU    3               ; Oszillator Timer 1 freigeben
T1CKPS0          EQU    4               ; Vorteilerwahl Timer 1
T1CKPS1          EQU    5
                endif

; Bits in T2CON

                if      MOMCPU=$16C64
T2CKPS0          EQU    0               ; Vorteilerwahl Timer 2
T2CKPS1          EQU    1
TMR2ON           EQU    2               ; Timer 2 ein/ausschalten
TOUTPS0          EQU    3               ; Nachteilerwahl Timer 2
TOUTPS1          EQU    4
TOUTPS2          EQU    5
TOUTPS3          EQU    7
                endif

; Bits in CCP1CON

                if      MOMCPU=$16C64
CCP1M0           EQU    0               ; Modus CCP1
CCP1M1           EQU    1
CCP1M2           EQU    2
CCP1M3           EQU    3
CCP1Y            EQU    4               ; PWM Bits 0,1
CCP1X            EQU    5
                endif

;----------------------------------------------------------------------------
; Bits in SSPSTAT

                if      MOMCPU=$16C64
BF               EQU    0               ; Puffer voll ?
UA               EQU    1               ; neue Adresse mu· in SSPADD geschrieben werden
RW               EQU    2               ; I2C lesen/schreiben ?
S                EQU    3               ; Startbit gefunden ?
P                EQU    4               ; Stopbit gefunden
DA               EQU    5               ; I2C Datum/Adresse empfangen ?
                endif

; Bits in SSPCON

                if      MOMCPU=$16C64
SSPM0            EQU    0               ; Schnittstellenmodus
SSPM1            EQU    1
SSPM2            EQU    2
SSPM3            EQU    3
CKP              EQU    4               ; TaktpolaritÑt
SSPEN            EQU    5               ; Portfreigabe
SSPOV            EQU    6               ; EmpfÑgerÅberlauf
WCOL             EQU    7               ; Senderkollision
                endif

;----------------------------------------------------------------------------
; Bits in INTCON

RBIF		EQU	0		; auf 1, falls Pegelwechsel an PB4..7
INTF		EQU	1		; Interrupt aufgetreten
RTIF		EQU	2		; RTCC-Interrupt
RBIE		EQU	3		; Interrupts durch Bit 0 freigeben
INTE		EQU	4		; Interrupts durch Bit 1 freigeben
RTIE		EQU	5		; Interrupts durch Bit 2 freigeben
                if      MOMCPU=$16C84
EEIE             EQU    6               ; Interrupts vom EEPROM freigeben
                elseif
PEIE             EQU    6               ; Peripherie-Interrupts freigeben
                endif
GIE		EQU	7		; globale Interruptsperre

; Bits in PIE1

                if      MOMCPU=$16C64
TMR1IE           EQU    0               ; Interrupts Timer 1 freigeben
TMR2IE           EQU    1               ; Interrupts Timer 2 freigeben
CCP1IE           EQU    2               ; Interrupts CCP1 freigeben
SSPIE            EQU    3               ; Interrupts serielle Schnittstelle freigeben
PSPIE            EQU    7               ; Interrupts Parallelport freigeben
                endif

; Bits in PIR1

                if      MOMCPU=$16C64
TMR1IF           EQU    0               ; Interrupt Timer 1 aufgetreten
TMR2IF           EQU    1               ; Interrupt Timer 2 aufgetreten
CCP1IF           EQU    2               ; Interrupt CCP1 aufgetreten
SSPIF            EQU    3               ; Interrupt serielle Schnittstelle aufgetreten
PSPIF            EQU    7               ; Interrupt Parallelport aufgetreten
                endif

;----------------------------------------------------------------------------
; Bits in OPTION

PS0		EQU	0		; Einstellung Vorteiler
PS1		EQU	1
PS2		EQU	2
PSA		EQU	3		; Vorteile-Zuordnung
RTE		EQU	4		; Triggerflanke RTCC
RTS		EQU	5		; RTCC-Signalquelle
INTEDG		EQU	6		; INT-Triggerflanke
RBPU		EQU	7		; Pull-Ups an Port B freigeben

;Bits in PCON

                if      MOMCPU=$16C64
POR              EQU    1               ; 0=Power-On-Reset aufgetreten
                endif

;----------------------------------------------------------------------------
; Bits in EECON1

                if      MOMCPU=$16C84
RD               EQU    0               ; Lesezyklus starten
WR               EQU    1               ; Schreibzyklus starten
WREN             EQU    2               ; Schreibsperre
WRERR            EQU    3               ; Schreibvorgang abgebrochen
EEIF             EQU    4               ; signalisiert Schreibende
                endif

		endif

		restore                 ; wieder erlauben

