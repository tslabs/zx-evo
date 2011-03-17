                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei STDDEF16.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR- und Makrodefinitionen fÅr die PIC 16C5x-Familie    *
;* 									    *
;*   letzte énderungen : 13. 4.1993                                         *
;*                        5. 6.1993 SAVE & RESTORE                          *
;*                                  Prozessorsperre (Idee von Matthias)     *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*									    *
;****************************************************************************

		ifndef  stddef16inc     ; verhindert Mehrfacheinbindung

stddef16inc     equ     1

                if      (MOMCPU<93268)&&(MOMCPU>93271)
		 fatal  "Falscher Prozessortyp eingestellt: nur 16C54..16C57 erlaubt"
		endif

                if      MOMPASS=1
		 message "PIC16C5x-Definitionen (C) 1993 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; erstmal die Spezialregister :

Indirect	SFR	0		; indirekte Adressierung
PCLo            SFR     2               ; PC Bits 0..7
RTCC            SFR     1               ; ZÑhler
Status		SFR	3		; Status-Register
FSR		SFR	4		; File-Select-Register
Port_A		SFR	5		; Ports
Port_B		SFR	6
Port_C		SFR	7

;----------------------------------------------------------------------------
; Bits im Statusregister

Flag_C		EQU	0		; Carry
Flag_DC		EQU	1		; Digit-Carry
Flag_Z		EQU	2		; Zero

PA1		EQU	5		; Bank-Select Bit 9
PA2		EQU	6		;  "     "     "  10


;----------------------------------------------------------------------------
; Flags setzen/lîschen:

clrc		macro			; Carry lîschen
		bcf	Status,Flag_C
		endm

setc		macro			; Carry setzen
		bsf	Status,Flag_C
		endm

clrdc		macro			; Digit-Carry lîschen
		bcf	Status,Flag_DC
		endm

setdc		macro			; Digit_Carry setzen
		bsf	Status,Flag_DC
		endm

clrz		macro			; Zero-Flag lîschen
		bcf	Status,Flag_Z
		endm

setz		macro			; Zero-Flag setzen
                bsf     Status,Flag_Z
		endm

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; einfachere Skip-Befehle

skpc		macro			; Sprung, falls C=1
		btfss	Status,Flag_C
		endm

skpnc		macro			; Sprung, falls C=0
		btfsc	Status,Flag_C
		endm

skpdc		macro			; Sprung, falls DC=1
		btfss	Status,Flag_DC
		endm

skpndc		macro			; Sprung, falls DC=0
		btfsc	Status,Flag_DC
		endm

skpz		macro			; Sprung, falls Z=1
		btfss	Status,Flag_Z
		endm

skpnz		macro			; Sprung, falls Z=0
		btfsc	Status,Flag_Z
		endm

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; bedingte SprÅnge:
; VORSICHT: Mehrbyteanweisungen, nicht skip-bar!!!!

b		macro	target		; unbedingter Sprung
		goto	target
		endm

bc		macro	target		; Sprung, falls C=1
		btfsc	Status,Flag_C
		goto	target
		endm

bnc		macro	target		; Sprung, falls C=0
		btfss	Status,Flag_C
		goto	target
		endm

bdc		macro	target		; Sprung, falls DC=1
		btfsc	Status,Flag_DC
		goto	target
		endm

bndc		macro	target		; Sprung, falls DC=0
		btfss	Status,Flag_DC
		goto	target
		endm

bz		macro	target		; Sprung, falls Z=1
		btfss	Status,Flag_Z
		goto	target
		endm

bnz		macro	target		; Sprung, falls Z=0
		btfsc	Status,Flag_Z
		goto	target
		endm


;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Carry-Arithmetik
; wieder VORSICHT!!!

addcf		macro	reg,dest	; Carry zu Register addieren
		btfsc	Status,Flag_C
		incf	reg,dest
		endm

adddcf		macro	reg,dest	; Digit-Carry zu Register addieren
		btfsc	Status,Flag_DC
		incf	reg,dest
		endm

subcf		macro	reg,dest	; Carry von Register subtrahieren
		btfsc	Status,Flag_C
		decf	reg,dest
		endm

subdcf		macro	reg,dest	; Digit-Carry von Register subtrahieren
		btfsc	Status,Flag_DC
		decf	reg,dest
		endm

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; vermischtes...

movfw		macro	reg		; Transfer Register-->W
		movf	reg,0
		endm

negf		macro	reg,dest	; Zweierkomplement
		comf	reg,1
		incf	reg,dest
		endm

tstf		macro	reg		; Flags entspr. Register bilden
		movf	reg,1
		endm

		endif

                restore                 ; wieder erlauben

