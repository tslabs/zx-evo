                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei STDDEF62.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR- und Makrodefinitionen fÅr die ST62xx-Prozessoren   *
;* 									    *
;*   letzte énderungen :  7. 8.1993                                         *
;*                       11. 9.1993 Meldung nur im 1.Pass                   *
;*                       21.11.1993 ROMWIN eingebaut                        *
;*                                                                          *
;****************************************************************************

		ifndef  stddef62inc     ; verhindert Mehrfacheinbindung

stddef62inc     equ     1

                if      (MOMCPU<>25104)&&(MOMCPU<>25109)&&(MOMCPU<>25120)&&(MOMCPU<>25125)
		 fatal  "Falscher Prozessortyp eingestellt: nur ST6210, ST6215, ST6220 oder ST6225 erlaubt!"
		endif

                if      MOMPASS=1
		 message "ST62xx-SFR-Definitionen (C) 1993 Alfred Arnold"
		 message "Zielprozessor: ST\{MOMCPU}"
		endif

;----------------------------------------------------------------------------
; erstmal die Sachen, die es Åberall gibt :

RegX            SFR     80h             ; Registeradressen (die kennt AS aber
RegY            SFR     81h             ; auch schon intern, nur der Voll-
RegV            SFR     82h             ; stÑndigkeit halber)
RegW            SFR     83h
RegA            SFR     0ffh

RomBase         SFR     40h             ; Basisadresse ROM-Fenster (besser
RDW             SFR     0c9h            ; aber das ASSUME-Feature nutzen)

PA              SFR     0c0h            ; Port A
DDRA            SFR     0c4h
OPTA            SFR     0cch
PB              SFR     0c1h            ; Port B
DDRB            SFR     0c5h
OPTB            SFR     0cdh

IOPT            SFR     0c8h            ; Interrupt-Steuerung

ADDATA          SFR     0d0h            ; A/D-Wandler
ADCTRL          SFR     0d1h

TPSC            SFR     0d2h            ; Timer
TDATA           SFR     0d3h
TSCR            SFR     0d4h

WDOG            SFR     0d8h            ; Watchdog

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Port C nur mit den 28-Pinnern:

                if      (MOMCPU=6215H)||(MOMCPU=6225H)
PC               SFR    0c2h            ; Port C
DDRC             SFR    0c6h
OPTC             SFR    0ceh
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; hiermit versuche ich, etwas die Leerstellen im Befehlssatz aufzufangen.
; das ist nur ein Angebot an Ihre Bequemlichkeit, niemand zwingt Sie, diese
; Makros zu benutzen, wenn Sie es im Einzelfall kÅrzer kînnen!

; Addition/Subtraktion mit Carry: erster Operand mu· A (Akku) sein!

adc             macro   dst,src
		jrnc    NoCarry
		inc     dst
NoCarry:        add     dst,src         ; Rest normal addieren
		endm

sbc             macro   dst,src
		jrnc    NoCarry
		dec     dst
NoCarry:        sub     dst,src
		endm

; ODER-VerknÅpfung:
; arbeitet nach der Regel von De Morgan: A OR B = /((/A) AND (/B))
; verwendet W-Register
; dst mu· A (Akku) sein!

or              macro   dst,src
		com     dst
		ld      w,dst
		ld      dst,src
		com     dst
		and     dst,w
		com     dst
		endm

; EXOR-VerknÅpfung:
; arbeitet nach dem Prinzip der 4 NAND-Gatter, mu· also selber nicht auf
; das OR-Makro zugreifen
; verwendet V- und W-Register
; dst mu· A (Akku) sein!

xor             macro   dst,src
		ld      v,dst           ; Op1 retten
		and     dst,src         ; Hilfsergebnis A NAND B bilden
		com     dst
		ld      w,dst           ; in W retten
		ld      dst,v           ; Op1 nochmal holen
		and     dst,w           ; 1. Teilergebnis A NAND W bilden
		com     dst
		ld      v,dst           ; in V retten
		ld      dst,src         ; 2. Teilergebnis B NAND W bilden
		and     dst,w
		com     dst
		and     dst,v
		com     dst
		endif
		endm

; rechts mit Carry rotieren
; ok, das ist brute force, aber mir ist nix besseres eingefallen...
; dst mu· Akku (A) sein

rrc             macro   dst
		rept    7
		 rlc    dst
		endm
		endm

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; FÅr diejenigen, die auf einmal den ROMWIN-Befehl vermissen, hier eine
; kleine öberbrÅckungshilfe:

romwin		macro	adr
		assume	rombase:adr>>6
		endm

;---------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben

