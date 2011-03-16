                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF3X.INC                                           *
;*   								            *
;*   Sinn : enthÑlt Register- und Adre·definitionen fÅr die TMS320C3x-CPUs  *
;* 									    *
;*   letzte énderungen : 27. 9.1994                                         *
;*									    *
;****************************************************************************

                ifndef  stddef3xinc     ; verhindert Mehrfacheinbindung

stddef3xinc     equ     1

                if      (MOMCPU<>3279920)&&(MOMCPU<>3279921)
                 fatal  "Falscher Prozessortyp eingestellt: nur 320C30/320C31 erlaubt!"
		endif


                if      MOMPASS=1
                 message "TMS320C3x-Definitionen (C) 1994 Alfred Arnold"
		endif

;------------------------------------------------------------------------------
; Timer

T0CTRL          equ     808020h
T0CNT           equ     808024h
T0PERIOD        equ     808028h

T1CTRL          equ     808030h
T1CNT           equ     808034h
T1PERIOD        equ     808038h

;------------------------------------------------------------------------------
; serielle Ports

S0CTRL          equ     808040h
S0TXPORTCTRL    equ     808042h
S0RXPORTCTRL    equ     808043h
S0TIMERCTRL     equ     808044h
S0TIMERCNT      equ     808045h
S0TIMERPERIOD   equ     808046h
S0TBUF          equ     808048h
S0RBUF          equ     80804ch

                if      MOMCPU=320C30h
S1CTRL           equ     808050h
S1TXPORTCTRL     equ     808052h
S1RXPORTCTRL     equ     808053h
S1TIMERCTRL      equ     808054h
S1TIMERCNT       equ     808055h
S1TIMERPERIOD    equ     808056h
S1TBUF           equ     808058h
S1RBUF           equ     80805ch
                endif

;------------------------------------------------------------------------------
; DMA

DMACTRL         equ     808000h
DMASRCADR       equ     808004h
DMADESTADR      equ     808006h
DMACNT          equ     808008h

;------------------------------------------------------------------------------
; Adressen Interrupt-Vektoren

INTVEC_RESET    equ     0
INTVEC_INT0     equ     1
INTVEC_INT1     equ     2
INTVEC_INT2     equ     3
INTVEC_INT3     equ     4
INTVEC_XINT0    equ     5
INTVEC_RINT0    equ     6
                if      MOMCPU=320C30h
INTVEC_XINT1     equ     7
INTVEC_RINT1     equ     8
                endif
INTVEC_TINT0    equ     9
INTVEC_TINT1    equ     0ah
INTVEC_DINT     equ     0bh
__TMPINTVEC     set     0
                rept    28
INTVEC_TRAP{"\{__TMPINTVEC}"} equ __TMPINTVEC+20h
__TMPINTVEC     set     __TMPINTVEC+1
                endm

                endif

                restore                 ; Listing wieder erlauben


