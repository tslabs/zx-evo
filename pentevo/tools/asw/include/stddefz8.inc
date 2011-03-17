		save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei Z8.INC                                                 *
;*   								            *
;*   Sinn : enthÑlt RegisterkÅrzel fÅr den Z8-Prozessor                     *
;* 									    *
;*   letzte énderungen : 16.12.1993 RÅdiger Mersiovsky                      *
;*                       25.12.1993 Copyrightvermerk                        *
;*                                                                          *
;****************************************************************************

                ifndef  stddefz8inc     ; verhindert Mehrfacheinbindung

stddefz8inc     equ     1

                if      (MOMCPU<>34305)
                 fatal  "Falscher Prozessortyp eingestellt: nur Z8601 erlaubt!"
		endif

                if      MOMPASS=1
                 message "Z8-KÅrzel-Definitionen"
                 message "(C) 1993 RÅdiger Mersiovsky"
		endif

;----------------------------------------------------------------------------

P0              EQU	00H		; Port 0
P1		EQU	01H		; Port 1
P2		EQU	02H		; Port 2
P3		EQU	03H		; Port 3
SIO		EQU	0F0H		; serielles Ein-Ausgaberegister
FLAGS		EQU	0FCH		; Flagregister
SPH		EQU	0FEH		; Stackpointer, Highteil
SPL		EQU	0FFH		; Stackpointer, Lowteil
TMR		EQU	0F1H		; ZÑhler/Zeitgeberbetriebsart
T0		EQU	0F4H		; ZÑhler/Zeitgeberregister Kanal 0
T1		EQU	0F2H		; ZÑhler/Zeitgeberregister Kanal 1
PRE0		EQU	0F5H		; T0-Vorteilerregister
PRE1		EQU	0F3H		; T1-Vorteilerregister
P01M		EQU	0F8H		; Tor 0, Tor 1 Betriebsart
P2M		EQU	0F6H		; Tor 2 Betriebsart
P3M		EQU	0F7H		; Tor 3 Betriebsart
IMR		EQU	0FBH		; Interrupt-Maskierungsregister
IPR		EQU	0F9H		; Interrupt-PrioritÑtsregister
IRQ		EQU	0FAH		; Interrupt-Anmelderegister
RP		EQU	0FDH		; Registerpointer

;---------------------------------------------------------------------------

                endif

                restore                 ; wieder erlauben

