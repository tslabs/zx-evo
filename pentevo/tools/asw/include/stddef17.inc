                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei STDDEF17.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR-Definitionen fÅr die PIC 17C4x-Familie              *
;*          Die Makrodefinitionen kann man bei Bedarf aus STDDEF16.INC      *
;*          kopieren							    *
;* 									    *
;*   letzte énderungen : 24. 4.1993                                         *
;*                        5. 6.1993 SAVE & RESTORE                          *
;*                                  Prozessorsperre (Idee von Matthias)     *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*									    *
;****************************************************************************

		ifndef  stddef17inc     ; verhindert Mehrfacheinbindung

stddef17inc     equ     1

                if      (MOMCPU<>97346)
		 fatal  "Falscher Prozessortyp eingestellt: nur 17C42 erlaubt"
		endif

                if      MOMPASS=1
		 message "PIC17C4x-Definitionen (C) 1993 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; CPU-Register

Indirect0       SFR     0               ; indirekte Adressierung 1
FSR0            SFR     1
Indirect1       SFR     8               ; indirekte Adressierung 2
FSR1            SFR     9
PCL             SFR     2               ; Bit 0..7 ProgrammzÑhler
PCLATH          SFR     3               ; Hilfsregister PC Bit 8..15
W               SFR     10              ; Akkumulator
TBLPTRL         SFR     13              ; Hilfsregister, um Daten
TBLPTRH         SFR     14              ; aus Codebereich zu lesen
ALUSTA          SFR     4               ; Flags, FSR-Modi
CPUSTA          SFR     6               ; CPU-Status
INTSTA          SFR     7               ; Interrupt-Status

;------------------------------------------------------------------------------
; ZÑhler

RTCSTA          SFR     5               ; Steuerregister
RTCCL           SFR     11              ; Daten
RTCCH           SFR     12

;------------------------------------------------------------------------------

BSR             SFR     15              ; Bankauswahl Rest

;------------------------------------------------------------------------------
; Interruptsteuerung

PIR             SFR     $16             ; Interrupt-Anforderungen
PIE             SFR     $17             ; Interrupt-Freigaben

;------------------------------------------------------------------------------
; Ports

PORTA           SFR     $10             ; Datenregister
PORTB           SFR     $12
PORTC           SFR     $11
PORTD           SFR     $13
PORTE           SFR     $15

DDRB            SFR     $11             ; Richtungsregister
DDRC            SFR     $10
DDRD            SFR     $12
DDRE            SFR     $14

;------------------------------------------------------------------------------
; serieller Port

RCSTA           SFR     $13             ; EmpfÑngerstatus
RCREG           SFR     $14             ; EmpfÑngerdaten
TXSTA           SFR     $15             ; Senderstatus
TXREG           SFR     $16             ; Sendedaten
SPBRG           SFR     $17             ; Teiler fÅr SIO

;------------------------------------------------------------------------------
; Timer

TMR1            SFR     $10             ; Daten Timer 1
TMR2            SFR     $11             ; Daten Timer 2
TMR3L           SFR     $12             ; Daten Timer 3
TMR3H           SFR     $13

PR1             SFR     $14             ; Periode Timer 1
PR2             SFR     $15             ; Periode Timer 2
PR3L            SFR     $16             ; Periode Timer 3
PR3H            SFR     $17

CA1L            SFR     $16             ; Fangwert 1
CA1H            SFR     $17
CA2L            SFR     $14             ; Fangwert 1
CA2H            SFR     $15

PW1DCL          SFR     $10             ; PWM 1 Daten
PW1DCH          SFR     $12
PW2DCL          SFR     $11             ; PWM 2 Daten
PW2DCH          SFR     $13

TCON1           SFR     $16             ; Steuerregister 1
TCON2           SFR     $17             ; Steuerregister 2

                endif

                restore                 ; wieder erlauben

