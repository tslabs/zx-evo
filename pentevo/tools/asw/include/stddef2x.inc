		save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF2X.INC                                           *
;*   								            *
;*   Sinn : enthÑlt die globalen Register-Definitionen fÅr die              *
;*          TMS320C2x Prozessoren                                           *
;* 									    *
;*   letzte énderungen : 27.02.1994                                         *
;*                                                                          *
;****************************************************************************

		ifndef  stddef2xinc     ; verhindert Mehrfacheinbindung

stddef2xinc     equ     1

                if      (MOMCPUNAME<>"320C25") && (MOMCPUNAME<>"320C26") && (MOMCPUNAME<>"320C28")
		 fatal  "Falscher Prozessortyp eingestellt: nur 320C25, 320C26 und 320C28 erlaubt!"
		endif

                if      MOMPASS=1
		 message "TMS320C2x Register-Definitionen (C) 1994 Thomas Sailer"
		endif

;----------------------------------------------------------------------------
		segment	data
		org	0

DRR		res	1
DXR		res	1
TIM		res	1
PRD		res	1
IMR		res	1
GREG		res	1

                if (MOMCPUNAME="320C26")
B2_D		equ	60h
B0_D		equ	200h
B1_D		equ	400h
B3_D		equ	600h
B0_P		equ	0fa00h
B1_P		equ	0fc00h
B3_P		equ	0fe00h
		endif

;---------------------------------------------------------------------------

                endif

                restore                 ; wieder erlauben

