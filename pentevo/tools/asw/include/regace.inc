                save
                listing off   		; do not include this file into listing

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - File REGACE.INC                                              *
;*   								            *
;*   Purpose : address definitions for ACE CPUs                             *
;* 									    *
;*   last changes : 15. 8.1999                                              *
;*                                                                          *
;****************************************************************************

		ifndef	regaceinc	; avoid multiple inclusion
regaceinc	equ	1

		if	(MOMCPUNAME<>"ACE1101")&&(MOMCPU<>"ACE1202")
		 fatal	"wrong processor type: supports only ACE1101 and ACE1202"
		endif

		if	MOMPASS=1
		 message "ACE1101/1202 register definitions"
		 message "(C) 1999 Alfred Arnold"
		 message "Target is \{MOMCPUNAME}"
		endif

;----------------------------------------------------------------------------

; the following symbols are claimed to be reserved, but I couldn't find their
; meaning anywhere:
; BPSEL DAT0 HBC PSELRD HPATTERN LPATTERN

; Addresses

		switch 	MOMCPUNAME
		case	"ACE1101"
CODEEND		equ	3ffh
		case	"ACE1202"
CODEEND		equ	7ffh
		endcase

T0VLO		equ	CODEEND-9
T0VHI		equ	CODEEND-8
T1VLO		equ	CODEEND-7
T1VHI		equ	CODEEND-6
WUVLO		equ	CODEEND-5
WUVHI		equ	CODEEND-4
SWVLO		equ	CODEEND-3
SWVLO		equ	CODEEND-2

; CPU Core

XHI		sfr	0beh
XLO		sfr	0bfh
PMC		sfr	0c0h
SP		sfr	0ceh
STATUS		sfr	0cfh

; LBD

LBD		sfr	0bdh

; Clock

HALT		sfr	0b7h

; Timer 0

WDSVR		sfr	0b5h
T0CNTRL		sfr	0b6h

; Timer 1

T1RALO		sfr	0aah
T1RAHI		sfr	0abh
TMR1LO		sfr	0ach
TMR1HI		sfr	0adh
T1CTRL		sfr	0aeh

; MIWU

WKEDG		sfr	0afh
WKPND		sfr	0b0h
WKEN		sfr	0b1h

; I/O

PORTGD		sfr	0b2h
PORTGC		sfr	0b3h
PORTGP		sfr	0b4h

;----------------------------------------------------------------------------

		endif			; of regaceinc