                save
                listing off     ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.39 - Datei STDDEF90.INC                                           *
;*                                                                          *
;*   Sinn : enthÑlt Register- und Speicheradressen fÅr TLCS-90-Prozessoren  *
;*          Port- bzw. Registernamen nach Toshiba-Konvention                *
;*          aus dem Handbuch zur Prozessorfamilie TLCS-90                   *
;*                                                                          *
;*   Author Alfred Arnold                                                   *
;*                                                                          *
;*   letzte énderungen :  1.11.1993                                         *
;*                                                                          *
;****************************************************************************

                ifndef  stddef90inc     ; verhindert Mehrfacheinbindung

stddef90inc     equ     1

                if      (MOMCPU<>9486657)
		 fatal  "Falscher Prozessortyp eingestellt: nur 90C141 erlaubt"
                endif

                if      MOMPASS=1
                 message "TLCS-90-Adre·definitionen"
                 message "(C) 1993 Alfred Arnold"
                endif

;----------------------------------------------------------------------------
; hier geht's los...

;Ports

P0              equ     0ffc0h
P1              equ     0ffc1h
P01CR           equ     0ffc2h  ; ==IRFL !!

P2              equ     0ffc4h
P2CR            equ     0ffc5h

P3              equ     0ffc6h
P3CR            equ     0ffc7h

P4              equ     0ffc8h
P4CR            equ     0ffc9h

P5              equ     0ffcah

P6              equ     0ffcch
P7              equ     0ffcdh
P67CR           equ     0ffceh

P8              equ     0ffd0h
P8CR            equ     0ffd1h

; Timer

TMOD            equ     0ffdah
TCLK            equ     0ffd8h
TRUN            equ     0ffdbh
TFFCR           equ     0ffd9h
T4MOD           equ     0ffe4h
T4FFCR          equ     0ffe5h
TREG0           equ     0ffd4h
TREG1           equ     0ffd5h
TREG2           equ     0ffd6h
TREG3           equ     0ffd7h
TREG4L          equ     0ffe0h
TREG4H          equ     0ffe1h
TREG5L          equ     0ffe2h
TREG5H          equ     0ffe3h
CAP1L           equ     0ffdch
CAP1H           equ     0ffddh
CAP2L           equ     0ffdeh
CAL2H           equ     0ffdfh

; Pattern

SMMOD           equ     0ffcbh
SMCR            equ     0ffcfh

; Seriell

SCMOD           equ     0ffe9h  ; Betriebsart
SCCR            equ     0ffeah  ; Steuerregister
SCBUF           equ     0ffebh  ; Datenregister

; Watchdog, Misc

BX              equ     0ffech  ; Bank-Register IX
BY              equ     0ffedh  ;               IY

WDMOD           equ     0ffd2h  ; Watchdog, Halt-Modus
WDCR            equ     0ffd3h  ; Steuerregister

; AD-Wandler

ADMOD           equ     0ffefh  ; Betriebsart
ADREG           equ     0ffeeh  ; Datenregister

; Interrupt

IRFL            equ     0ffc2h  ; Request Flip-Flops
IRFH            equ     0ffc3h
INTEL           equ     0ffe6h  ; Freigabe Interrupts
INTEH           equ     0ffe7h
DMAEH           equ     0ffe8h  ; Freigabe Micro-DMA

; feste Speicherbereiche

IRAM            equ     0fec0H    ; Internal RAM 256 Byte...
IRAMEND         equ     0ffbfH
IEAREA          equ     00000H    ; Interrupt Entry Area...
IEAREAEND       equ     0007FH    ; ...up to 007FH
IROM            equ     00000H    ; Internal (P)ROM 8K...
IROMEND         equ     01FFFH    ;
EXTMEM          equ     IROMEND+1 ; External Memory
MEMEND          equ     IRAM-1    ; ...up to FFFFH

;---------------------------------------------------------------------------
; das war's...

                endif

                restore


