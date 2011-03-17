		save
		listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF2X.INC                                           *
;*   								            *
;*   Sinn : enthÑlt die globalen Register-Definitionen fÅr die              *
;*          TMS320C5x Prozessoren                                           *
;* 									    *
;*   letzte énderungen : 03.11.1995                                         *
;*                                                                          *
;****************************************************************************

		ifndef  stddef5xinc     ; verhindert Mehrfacheinbindung

stddef5xinc     equ     1

                if      (MOMCPU<>3279952) && (MOMCPU<>3279953) && (MOMCPU<>3279955)
		 fatal  "Falscher Prozessortyp eingestellt: nur 320C50, 320C51 und 320C53 erlaubt!"
		endif

                if      MOMPASS=1
		 message "TMS320C5x Register-Definitionen (C) 1995 Thomas Sailer"
		endif

;----------------------------------------------------------------------------
		segment	data

		org	4
                ;Core Processor Memory Mapped Registers
IMR             res     1
GREG            res     1
IFR             res     1
PMST            res     1
RPTC            res     1
BRCR            res     1
PASR            res     1
PAER            res     1
TREG0           res     1
TREG1           res     1
TREG2           res     1
DBMR            res     1
AR0             res     1
AR1             res     1
AR2             res     1
AR3             res     1
AR4             res     1
AR5             res     1
AR6             res     1
AR7             res     1
INDX            res     1
ARCR            res     1
CBSR1           res     1
CBER1           res     1
CBSR2           res     1
CBER2           res     1
CBCR            res     1
BMAR            res     1

                org     32
                ;Peripherial memory mapped registers
DRR		res	1
DXR		res	1
SPC             res     1

                org     36
TIM             res     1
PRD             res     1
TCR             res     1

                org     40
PDWSR           res     1
IOWSR           res     1
CWSR            res     1

                org     48
TRCV            res     1
TDXR            res     1
TSPC            res     1
TCSR            res     1
TRTA            res     1
TRAD            res     1

                org     80
PA0             res     1
PA1             res     1
PA2             res     1
PA3             res     1
PA4             res     1
PA5             res     1
PA6             res     1
PA7             res     1
PA8             res     1
PA9             res     1
PA10            res     1
PA11            res     1
PA12            res     1
PA13            res     1
PA14            res     1
PA15            res     1


;---------------------------------------------------------------------------

                endif

                restore                 ; wieder erlauben

