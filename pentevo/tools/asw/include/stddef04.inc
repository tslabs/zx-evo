		save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei STDDEF04.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR-Definitionen fÅr den 6804-Prozessor                 *
;*          Falls Sie noch ein paar schîne Makros suchen, schauen Sie sich  *
;*          einmal in STDDEF62.INC um !					    *
;* 									    *
;*   letzte énderungen : 28.11.1993                                         *
;*                                                                          *
;****************************************************************************

		ifndef  stddef04inc     ; verhindert Mehrfacheinbindung

stddef04inc     equ     1

                if      (MOMCPU<>26628)
		 fatal  "Falscher Prozessortyp eingestellt: nur 6804 erlaubt!"
		endif

                if      MOMPASS=1
		 message "6804-SFR-Definitionen (C) 1993 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; erstmal die Sachen, die es Åberall gibt :

XP              SFR     $80             ; Registeradressen (die kennt AS aber
YP              SFR     $81             ; auch schon intern, nur der Voll-
RegA            SFR     $ff             ; stÑndigkeit halber)

PA              SFR     $00             ; Port A
DDRA            SFR     $04
PB              SFR     $01             ; Port B
DDRB            SFR     $05
PC		SFR	$02
DDRC		SFR	$06

TSCR            SFR     $09             ; Timer : Steuerung/Status
TCR             SFR     $fe		;         Daten
TPR             SFR     $fd		;         Vorteiler

USERROM		SFR	$20		; benutzerdefiniertes ROM (bis $5f)
USERRAM		SFR	$82		; freies RAM (bis $9f)

;---------------------------------------------------------------------------

                endif

                restore                 ; wieder erlauben

