		save
		listing off     ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF47.INC                                           *
;*                                                                          *
;*   Sinn : enthÑlt den "erweiterten Befehlssatz fÅr die                    *
;*          TLCS-47(0(A*))-Reihe                                            *
;*                                                                          *
;*   Author Alfred Arnold                                                   *
;*                                                                          *
;*   letzte énderungen : 22.1.1994                                          *
;*                                                                          *
;****************************************************************************

		ifndef  stddef47inc     ; verhindert Mehrfacheinbindung

stddef47inc     equ     1

                if      (MOMCPU<>293888)&&(MOMCPU<>4656128)&&(MOMCPU<>74492928)
		 fatal  "Falscher Prozessortyp eingestellt: nur 47C00, 470C00, oder 470AC00 erlaubt"
		endif

                if      MOMPASS=1
		 message "TLCS-47-Makrodefinitionen"
		 message "(C) 1994 Alfred Arnold"
                 message "Zielprozessor: \{MOMCPU}"
		endif

;----------------------------------------------------------------------------
; hier geht's los...

; bedingte SprÅnge


bz		macro	adr		; Sprung, falls ZF=1
		testp	zf
		b	adr
		endm


bnz		macro	adr		; Sprung, falls ZF=0
		testp	zf
		b	s1
		b	adr
s1:
		endm


bc		macro	adr		; Sprung, falls CF=1
		testp	cf
		b	adr
		endm


bnc		macro	adr		; Sprung, falls CF=0
		test	cf
		b	adr
		endm


be		macro	dest,src,adr	; Vergleich & Sprung, falls =
		cmpr	dest,src
		testp	zf
		b	adr
		endm


bne		macro	dest,src,adr	; Vergleich & Sprung, falls <>
		cmpr	dest,src
                if      UpString("DEST")<>"A"
		 testp	zf
		 b	s1
		endif
		b	adr
s1:
		endm


bge		macro	dest,src,adr	; Vergleich & Sprung, falls >=
		cmpr	dest,src
		testp	zf
		b	adr
		test	cf
		b	adr
		endm


bgt		macro	dest,src,adr	; Vergleich und Sprung, falls >
		cmpr	dest,src
		test	cf
		b	adr
		endm


ble		macro	dest,src,adr	; Vergleich und Sprung, falls <=
		cmpr	dest,src
		testp	cf
		b	adr
		endm


blt		macro	dest,src,adr	; Vergleich und Sprung, falls <
		cmpr	dest,src
		testp	zf
		b	s1
		testp	cf
		b	adr
s1:
		endm


; Unterprogrammaufrufe


callss		macro	vector		; per Vektor
		if	vector=0
		 calls	86h
		elseif
		 calls	(vector<<3)+6
		endif
		endm


callz		macro	adr		; falls ZF=1
		testp	zf
		b	s1
		b	s2
s1:		call	adr
s2:
		endm


callnz		macro	adr		; falls ZF=0
		testp	zf
		b	s1
		call	adr
s1:
		endm


callc		macro	adr		; falls CF=1
		test	cf
		b	s1
		call	adr
s1:
		endm


callnc		macro	adr		; falls CF=0
		testp	cf
		b	s1
		call	adr
s1:
		endm


; UnterprogrammrÅcksprÅnge


retz		macro			; falls ZF=1
		testp	zf
		b	s1
		b	s2
s1:		ret
s2:
		endm


retnz		macro			; falls ZF=0
		testp	zf
		b	s1
		ret
s1:
		endm


retc		macro			; falls CF=1
		test	cf
		b	s1
		ret
s1:
		endm


retnc		macro			; falls CF=0
		testp	cf
		b	s1
		ret
s1:
		endm


; InterruptrÅcksprÅnge


retiz		macro			; falls ZF=1
		testp	zf
		b	s1
		b	s2
s1:		reti
s2:
		endm


retinz		macro			; falls ZF=0
		testp	zf
		b	s1
		reti
s1:
		endm


retic		macro			; falls CF=1
		test	cf
		b	s1
		reti
s1:
		endm


retinc		macro			; falls CF=0
		testp	cf
		b	s1
		reti
s1:
		endm


; erweiterte Schiebebefehle


shl		macro	op,cnt		; log. Linksshift
                if      UpString("OP")<>"A"
		 xch	a,op
		endif
		rept	cnt
		 test	cf
		 rolc	a
		endm
                if      UpString("OP")<>"A"
		 xch	a,op
		endif
		endm

shr		macro	op,cnt		; log. Rechtsshift
                if      UpString("OP")<>"A"
		 xch	a,op
		endif
		rept	cnt
		 test	cf
		 rorc	a
		endm
                if      UpString("OP")<>"A"
		 xch	a,op
		endif
		endm


; Interruptsperren


ei		macro
		eiclr	il,3fh
		endm

di		macro
		diclr	il,3fh
		endm

;---------------------------------------------------------------------------
; das war's...

		endif

		restore


