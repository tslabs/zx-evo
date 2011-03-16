                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG78K0.INC                                            *
;*   								            *
;*   Sinn : enthÑlt Registerdefinitionen fÅr 78K0 (uPD78070A)               *
;* 									    *
;*   letzte énderungen :  1.10.1996                                         *
;*                                                                          *
;****************************************************************************

                ifndef  regz78k0inc      ; verhindert Mehrfacheinbindung

reg8k0inc       equ     1

                if      (MOMCPUNAME<>"78070")
                 fatal  "Falscher Prozessortyp eingestellt: nur 78070 erlaubt!"
		endif


                if      MOMPASS=1
                 message "78K0-Register-Definitionen (C) 1996 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; CPU-Kern

SP              equ     0ff1ch          ; Stackpointer

PSW             equ     0ff1eh          ; Statuswort

;----------------------------------------------------------------------------
; Taktgenerator

PCC             equ     0fffbh          ; Steuerung Prozessortakt

OSMS            equ     0fff2h          ; Taktteiler ein/ausschalten

OSTS            equ     0fffah          ; Wartezeit, um nach STOP-Auflîsung

;----------------------------------------------------------------------------
; Bus-Interface

EBTS            equ     0ff3fh          ; Multiplex an/aus (fest auf aus)

IMS             equ     0fff0h          ; Grî·e internes RAM (fest 1K)

MM              equ     0fff8h          ; Wait-State-Steuerung

;----------------------------------------------------------------------------
; Interrupt-Steuerung

INTM0           equ     0ffech          ; Flankenwahl INTP0...INTP2
INTM1           equ     0ffedh          ; Flankenwahl INTP3...INTP6
SCS             equ     0ff47h          ; Auswahl Abtastrate INTP0

IF0L            equ     0ffe0h          ; Interrupts aufgetreten ?
IF0H            equ     0ffe1h
IF0             equ     IF0L
IF1L            equ     0ffe2h

MK0L            equ     0ffe4h          ; Maskierungsregister
MK0H            equ     0ffe5h
MK0             equ     IF0L
MK1L            equ     0ffe6h

PR0L            equ     0ffe8h          ; PrioritÑten
PR0H            equ     0ffe9h
PR0             equ     IF0L
PR1L            equ     0ffeah

;----------------------------------------------------------------------------
; Ports

P0              equ     0ff00h          ; Datenregister Port 0
PM0             equ     0ff20h          ; Datenrichtungsregister Port 0

P1              equ     0ff01h          ; Datenregister Port 1
PM1             equ     0ff21h          ; Datenrichtungsregister Port 1

P2              equ     0ff02h          ; Datenregister Port 2
PM2             equ     0ff22h          ; Datenrichtungsregister Port 2

P3              equ     0ff03h          ; Datenregister Port 3
PM3             equ     0ff23h          ; Datenrichtungsregister Port 3

P6              equ     0ff06h          ; Datenregister Port 6
PM6             equ     0ff26h          ; Datenrichtungsregister Port 6

P7              equ     0ff07h          ; Datenregister Port 7
PM7             equ     0ff27h          ; Datenrichtungsregister Port 7

P9              equ     0ff09h          ; Datenregister Port 9
PM9             equ     0ff29h          ; Datenrichtungsregister Port 9

P10             equ     0ff0ah          ; Datenregister Port 10
PM10            equ     0ff2ah          ; Datenrichtungsregister Port10

P12             equ     0ff0ch          ; Datenregister Port 12
PM12            equ     0ff2ch          ; Datenrichtungsregister Port 12

P13             equ     0ff0dh          ; Datenregister Port 13
PM13            equ     0ff2dh          ; Datenrichtungsregister Port 13

PUOH            equ     0fff3h          ; Pull-Up-Register Port 0..7
PUOL            equ     0fff7h          ; Pull-Up-Register Port 8..13

;----------------------------------------------------------------------------
; Timer

TM0             equ     0ff14h          ; ZÑhlwert Timer 0
CR00            equ     0ff10h          ; Vergleichs/Fangregister 0 Timer 0
CR01            equ     0ff12h          ; Vergleichs/Fangregister 1 Timer 0
TCL0            equ     0ff40h          ; Taktauswahl Timer 0
TMC0            equ     0ff48h          ; Betriebsart Timer 0
CRC0            equ     0ff4ch          ; Steuerung Fang/Vergleichsregister Timer 0
TOC0            equ     0ff4eh          ; Ausgabesteuerung Timer 0

TM1             equ     0ff18h          ; ZÑhlwert Timer 1
CR10            equ     0ff16h          ; Vergleichs/Fangregister Timer 1
TCL1            equ     0ff41h          ; Taktauswahl Timer 1
TMC1            equ     0ff49h          ; Betriebsart Timer 1

TM2             equ     0ff19h          ; ZÑhlwert Timer 2
CR20            equ     0ff17h          ; Vergleichs/Fangregister Timer 2
TCL2            equ     0ff42h          ; Taktauswahl Timer 2 / Uhren-Timer / Watchdog / Summer
TMC2            equ     0ff4ah          ; Betriebsart Timer 2 / Uhren-Timer

TMS             equ     TM1             ; ZÑhlwert Timer 1+2 im 16-Bit-Modus
TOC1            equ     0ff4fh          ; Ausgabesteuerung Timer 1+2

TM5             equ     0ff51h          ; ZÑhlwert Timer 5
CR50            equ     0ff50h          ; Vergleichsregister Timer 5
TCL5            equ     0ff52h          ; Taktauswahl Timer 5
TMC5            equ     0ff53h          ; Betriebsart Timer 5

TM6             equ     0ff55h          ; ZÑhlwert Timer 6
CR60            equ     0ff54h          ; Vergleichsregister Timer 6
TCL6            equ     0ff56h          ; Taktauswahl Timer 6
TMC6            equ     0ff57h          ; Betriebsart Timer 6

;----------------------------------------------------------------------------
; Watchdog

WDTM            equ     0fff9h          ; Betriebsart

;----------------------------------------------------------------------------
; serielle Schnittstelle

; Kanal 0
SIO0            equ     0ff1ah          ; Datenregister
SVA             equ     0ff62h          ; Slave-Adresse
TCL3            equ     0ff43h          ; Taktauswahl Kanal 0/1
CSIM0           equ     0ff60h          ; Betriebsart
SBIC            equ     0ff61h          ; Bus-Steuerung
SINT            equ     0ff63h          ; Interrupt-Timing

; Kanal 1
SIO1            equ     0ff1bh          ; Datenregister
ADTP            equ     0ff6ah          ; Adre·zeiger bei Blockempfang/versand
CSIM1           equ     0ff68h          ; Betriebsart
ADTC            equ     0ff69h          ; Steuerung Blockbetrieb
ADTI            equ     0ff6bh          ; Geschwindigkeit Blockversand

; Kanal 2
TXS             equ     0ff74h          ; Datenregister
RXB             equ     0ff74h
SIO2            equ     TXS
ASIM            equ     0ff70h          ; Schnittstellenparameter
ASIS            equ     0ff71h          ; Statusregister
CSIM2           equ     0ff72h          ; Betriebsart
BRGC            equ     0ff73h          ; Baudratenwahl

;----------------------------------------------------------------------------
; A/D-Wandler

ADCR            equ     0ff1fh          ; Ergebnisregister Wandlung

ADM             equ     0ff80h          ; Betriebsart

ADIS            equ     0ff84h          ; Umschaltung Ports als AnalogeingÑnge

;----------------------------------------------------------------------------
; D/A-Wandler

DACS0           equ     0ff90h          ; Analogwert Wandler 0

DACS1           equ     0ff91h          ; Analogwert Wandler 1

DAM             equ     0ff96h          ; Betriebsart

;----------------------------------------------------------------------------
; Echtzeitausgabe

RTBL            equ     0ff30h          ; Ausgabedaten
RTBH            equ     0ff31h
RTPM            equ     0ff34h          ; Betriebsart
RTPC            equ     0ff36h          ; Steuerung

;----------------------------------------------------------------------------
; Vektoradressen

Vec_Reset       equ     0000h           ; Reset
Vec_INTWDT      equ     0004h           ; Watchdog
Vec_INTP0       equ     0006h           ; externe Interrupts
Vec_INTP1       equ     0008h
Vec_INTP2       equ     000ah
Vec_INTP3       equ     000ch
Vec_INTP4       equ     000eh
Vec_INTP5       equ     0010h
Vec_INTP6       equ     0012h
Vec_INTCSI0     equ     0014h           ; serielle Schnittstellen
Vec_INTCSI1     equ     0016h
Vec_INTSER      equ     0018h
Vec_INTSR       equ     001ah
Vec_INTCSI2     equ     Vec_INTSR
Vec_INTST       equ     001ch
Vec_INTTM3      equ     001eh           ; Uhren-Timer
Vec_INTTM00     equ     0020h           ; Timer
Vec_INTTM01     equ     0022h
Vec_INTTM1      equ     0024h           ; Vergleicher Timer 1
Vec_INTTM2      equ     0026h           ; Vergleicher Timer 2
Vec_INTAD       equ     0028h           ; A/D-Wandler
Vec_INTTM5      equ     002ah           ; Vergleicher Timer 5
Vec_INTTM6      equ     002ch           ; Vergleicher Timer 6
Vec_BRK         equ     003eh           ; BRK-Befehl

;----------------------------------------------------------------------------
; Speicheradressen

BUFRAM          equ     0fac0h          ; Puffer-RAM
BUFRAMEND       equ     0fadfh

IRAM            equ     0fb00h          ; internes RAM
IRAMEND         equ     0fedfh          ; dahinter sind die Register gemappt!

;----------------------------------------------------------------------------

                endif                   ; von IFDEF...
		restore                 ; wieder erlauben



