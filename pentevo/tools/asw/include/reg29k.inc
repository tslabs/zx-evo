                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG29K.INC                                             *
;*   								            *
;*   Sinn : enthÑlt Adre·definitionen fÅr die 2924x-Prozessoren             *
;* 									    *
;*   letzte énderungen : 20. 7.1995                                         *
;*                                                                          *
;****************************************************************************

                ifndef  reg29kinc       ; verhindert Mehrfacheinbindung

reg29kinc       equ     1

                if      (MOMCPU<>168512)&&(MOMCPU<>168515)&&(MOMCPU<>168517)
                 fatal  "Falscher Prozessortyp eingestellt: nur AM29240, AM29243 oder AM29245 erlaubt!"
		endif


                if      MOMPASS=1
                 message "AM2924x-SFR-Definitionen (C) 1995 Alfred Arnold"
                 message "binde AM\{MOMCPU}-SFRs ein"
		endif

;----------------------------------------------------------------------------
; Registerbasis

RegBase         equ     0x80000000

;----------------------------------------------------------------------------
; ROM-Controller

RMCT            equ     RegBase+0x00    ; ROM-Steuerregister
RMCF            equ     RegBase+0x04    ; ROM-Konfigurationsregister

;----------------------------------------------------------------------------
; DRAM-Controller

DRCT            equ     RegBase+0x08    ; DRAM-Steuerregister
DRCF            equ     RegBase+0x0c    ; DRAM-Konfigurationsregister

;----------------------------------------------------------------------------
; PIA

PICT0           equ     RegBase+0x20    ; PIA Steuerregister 0
PICT1           equ     RegBase+0x24    ; PIA Steuerregister 1

;----------------------------------------------------------------------------
; DMA-Controller

DMCT0           equ     RegBase+0x30    ; Steuerregister Kanal 0
DMAD0           equ     RegBase+0x34    ; Adre·register Kanal 0
TAD0            equ     RegBase+0x70    ; Queued-Adre·register Kanal 0
DMCN0           equ     RegBase+0x38    ; ZÑhlregister Kanal 0
TCN0            equ     RegBase+0x3c    ; Queued-ZÑhlregister Kanal 0
DMCT1           equ     RegBase+0x40    ; Steuerregister Kanal 1
DMAD1           equ     RegBase+0x44    ; Adre·register Kanal 1
TAD1            equ     RegBase+0x74    ; Queued-Adre·register Kanal 1
DMCN1           equ     RegBase+0x48    ; ZÑhlregister Kanal 1
TCN1            equ     RegBase+0x4c    ; Queued-ZÑhlregister Kanal 1
                if      MOMCPU<>0x29245
DMCT2            equ     RegBase+0x50   ; Steuerregister Kanal 2
DMAD2            equ     RegBase+0x54   ; Adre·register Kanal 2
TAD2             equ     RegBase+0x78   ; Queued-Adre·register Kanal 2
DMCN2            equ     RegBase+0x58   ; ZÑhlregister Kanal 2
TCN2             equ     RegBase+0x5c   ; Queued-ZÑhlregister Kanal 2
DMCT3            equ     RegBase+0x60   ; Steuerregister Kanal 3
DMAD3            equ     RegBase+0x64   ; Adre·register Kanal 3
TAD3             equ     RegBase+0x7c   ; Queued-Adre·register Kanal 3
DMCN3            equ     RegBase+0x68   ; ZÑhlregister Kanal 3
TCN3             equ     RegBase+0x6c   ; Queued-ZÑhlregister Kanal 3
                endif

;----------------------------------------------------------------------------
; PIO

POCT            equ     RegBase+0xd0    ; PIO-Steuerregister
PIN             equ     RegBase+0xd4    ; PIO-Eingangsregister
POUT            equ     RegBase+0xd8    ; PIO-Ausgangsregister
POEN            equ     RegBase+0xdc    ; PIO-Richtungssteuerung

;----------------------------------------------------------------------------
; Parallelport

PPCT            equ     RegBase+0xc0    ; Steuerregister
PPST            equ     RegBase+0xc8    ; Statusrtegister
PPDT            equ     RegBase+0xc4    ; Datenregister

;----------------------------------------------------------------------------
; serielle Ports

SPCTA           equ     RegBase+0x80    ; Steuerregister Kanal A
SPSTA           equ     RegBase+0x84    ; Statusregister Kanal A
SPTHA           equ     RegBase+0x88    ; Senderegister Kanal A
SPRBA           equ     RegBase+0x8c    ; Empfangsregister Kanal A
BAUDA           equ     RegBase+0x90    ; Baudratenregister Kanal A
                if      MOMCPU<>0x29245
SPCTB            equ     RegBase+0xa0   ; Steuerregister Kanal A
SPSTB            equ     RegBase+0xa4   ; Statusregister Kanal A
SPTHB            equ     RegBase+0xa8   ; Senderegister Kanal A
SPRBB            equ     RegBase+0xac   ; Empfangsregister Kanal A
BAUDB            equ     RegBase+0xb0   ; Baudratenregister Kanal A
                endif

;----------------------------------------------------------------------------
; Video-Interface

                if      MOMCPU<>0x29243
VCT              equ     RegBase+0xe0   ; Steuerregister
TOP              equ     RegBase+0xe4   ; Zeilennummer oberer Rand
SIDE             equ     RegBase+0xe8   ; Spaltennummer linker/rechter Rand
VDT              equ     RegBase+0xec   ; Datenregister
                endif

;----------------------------------------------------------------------------
; Interrupt-Steuerung

ICT             equ     RegBase+0x28    ; Steuerregister
IMASK           equ     RegBase+0x2c    ; Maskenregister


                endif

                restore
