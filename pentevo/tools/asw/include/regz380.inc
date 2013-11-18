                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGZ380.INC                                            *
;*   								            *
;*   Sinn : enthÑlt Registerdefinitionen fÅr den Z380                       *
;*          Diese Register sind nur mit den Befehlen IN0,OUT0 und TSTIO     *
;*          erreichbar!!                                                    *
;* 									    *
;*   letzte énderungen : 12.11.1994                                         *
;*                        2. 1.1996 Register 0..16h                         *
;*                                  (Info von Leonhard Schneider)           *
;*                                                                          *
;****************************************************************************

                ifndef  regz380inc      ; verhindert Mehrfacheinbindung

reg380inc       equ     1

                if      (MOMCPU<>896)
                 fatal  "Falscher Prozessortyp eingestellt: nur Z380 erlaubt!"
		endif


                if      MOMPASS=1
                 message "Z380-Register-Definitionen (C) 1994 Alfred Arnold"
		endif

;----------------------------------------------------------------------------

LMCS0           port    00h             ; Lower Memory Chip Select Register
LMCS1           port    01h
UMCS0           port    02h             ; Upper Memory Chip Select Register
UMCS1           port    03h
MMCS0           port    04h             ; Midrange Memory Chip Select Register
MMCS1           port    05h
MMCS2           port    06h
MMCS3           port    07h
LMWR            port    08h             ; Lower Memory Waits Register
UMWR            port    09h             ; Upper Memory Waits Register
MMWR0           port    0ah             ; Midrange Memory Waits Register
MMWR1           port    0bh
MMWR2           port    0ch
MMWR3           port    0dh
IOWR            port    0eh             ; I/O Waits Register
RFWR            port    0fh             ; Refresh Waits Register
MSMER           port    10h             ; Memory Select Master Enable Register
IOCR0           port    11h             ; I/O Bus Control Register
IOCR1           port    12h
RFSHR0          port    13h             ; Refresh Register
RFSHR1          port    14h
RFSHR2          port    15h
SMCR            port    16h             ; Standby Mode Control Register
IER             port    17h             ; Interrupt-Freigaben
AVBR            port    18h             ; Offset Interruptvektoren ?!
TRPBK           port    19h             ; zeigt an, ob Trap oder Break ausgetreten

CHIPVERSION     port    0ffh            ; Chipversion (00=Z380MPU)


;----------------------------------------------------------------------------

                endif                   ; von IFDEF...
		restore                 ; wieder erlauben


