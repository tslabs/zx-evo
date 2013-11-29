                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF75.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR-Definitionen fÅr die ÊPD75K0-Familie                *
;* 									    *
;*   letzte énderungen :  2.12.1994                                         *
;*                                                                          *
;****************************************************************************

                ifndef  stddef75inc     ; verhindert Mehrfacheinbindung

stddef75inc     equ     1

;----------------------------------------------------------------------------
; Bequemlichkeit

__message       macro   msg,{NoExpand}
                if      MOMPASS=1
                 message msg
                endif
                endm

                __message "ÊPD75K0-Registerdefinitionen, (C) 1994 Alfred Arnold"

;----------------------------------------------------------------------------
; Die Prozessoren zu Gruppen zusammenfassen

                switch  MOMCPU
                case    480258
                 __message "Binde ÊPD75402-Register ein"
__family         equ     400

                case    479236,479238,479240
                 __message "Binde ÊPD750xx-Register ein"
__family         equ     000

                case     479848
                 __message "Binde ÊPD75268-Register ein"
__family         equ     260

                case     480004,480006,480008,480018,480022
                 __message "Binde ÊPD753xx-Register ein"
__family         equ     300

                case    480040
                 __message "Binde ÊPD75328-Register ein"
__family         equ     320

                case    479492,479494,479496,479506,479510
                 __message "Binde ÊPD751xx-Register ein"
__family         equ     100

                case    479750,479752,479762,479766
                 __message "Binde ÊPD752xx-Register ein"
__family         equ     200

                case    480530,480534
                 __message "Binde ÊPD755xx-Register ein"
__family         equ     500

                elsecase
                 fatal   "Fehler: Keine CPU aus der ÊPD75xxx-Familie definiert"
                endcase

;----------------------------------------------------------------------------
; da alle Interrupt-Register gleich sind, dafÅr ein Makro bauen

__defint        macro   NAME,base,{NoExpand}
__tmpnam        set     "NAME"
I{__tmpnam}     sfr     base
IE{__tmpnam}    bit     base.1
IRQ{__tmpnam}   bit     base.0
                endm

;----------------------------------------------------------------------------
; gemeinsame Register:

SP               sfr     0f80h          ; [8W] Stack-Pointer
BTM              sfr     0f85h          ; [4W] Modus Basis-Timer
BT               sfr     0f86h          ; [8R] ZÑhlwert Basis-Timer
IM0              sfr     0fb4h          ; [4W] INT0 Modusregister
                 __defint BT,0fb8h      ; [4]  Freigabe/Anzeige Interrupt BT
PORT0            sfr     0ff0h          ; [4R] Datenregister Port 0
PORT1            sfr     0ff1h          ; [4R] Datenregister Port 1
PORT2            sfr     0ff2h          ; [4]  Datenregister Port 2
PORT3            sfr     0ff3h          ; [4]  Datenregister Port 3
PORT5            sfr     0ff5h          ; [4]  Datenregister Port 5
PORT6            sfr     0ff6h          ; [4]  Datenregister Port 6

RESET            label   0000h          ; Resetvektor
VIBT             label   0002h          ; Interruptvektoradresse INTBT
                                        ; z.T. auch INT4
VI0              label   0004h          ; Interruptvektoradresse INT0
                                        ; z.T. auch INT1

;----------------------------------------------------------------------------

                if      __family=400
IME              bit     0fb2h.3        ; [8]  Interruptsperre (mit EI/DI ansprechen)
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
                 __defint CSI,0fbdh     ; [4]  Freigabe/Anzeige Interrupt CSI
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
                 __defint 2,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 2
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
POGA             sfr     0fdch          ; [8W] Steuerung Pull-Ups Port A
CSIM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
CSIE             bit     CSIM+1.3
COI              bit     CSIM+1.2
WUP              bit     CSIM+1.1
SBIC             sfr     0fe2h          ; [1]  Steuerung SBI
CMDD             bit     SBIC.3
RELD             bit     SBIC.2
CMDT             bit     SBIC.1
RELT             bit     SBIC.0
BSYE             bit     SBIC+1.3
ACKD             bit     SBIC+1.2
ACKE             bit     SBIC+1.1
ACKT             bit     SBIC+1.0
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
SVA              sfr     0fe6h          ; [8W] Knotenadresse auf seriellem Bus
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PMGB             sfr     0fech          ; [8W]      "        "

VICSI            label   0008h          ; Interruptvektoradresse INTCSI

RAMEnd           sfr     64             ; RAM-Grî·e
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=000
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
TOE0             bit     0fa2h.3        ; [1W] Freigabe Ausgabe Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST0             bit     PSW.2
MBE              bit     PSW.1
IME              bit     0fb2h.3        ; [8]  Interruptsperre (mit EI/DI ansprechen)
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
IM2              sfr     0fb6h          ; [4W] INT2 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint CSI,0fbdh     ; [4]  Freigabe/Anzeige Interrupt CSI
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
BSB0             sfr     0fc0h          ; [4]  bitsequentielle Puffer
BSB1             sfr     0fc1h
BSB2             sfr     0fc2h
BSB3             sfr     0fc3h
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
POGA             sfr     0fdch          ; [8W] Steuerung Pull-Ups Port A
POGB             sfr     0fdeh          ; [8W] Steuerung Pull-Ups Port B
CSIM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
CSIE             bit     CSIM+1.3
COI              bit     CSIM+1.2
WUP              bit     CSIM+1.1
SBIC             sfr     0fe2h          ; [1]  Steuerung SBI
CMDD             bit     SBIC.3
RELD             bit     SBIC.2
CMDT             bit     SBIC.1
RELT             bit     SBIC.0
BSYE             bit     SBIC+1.3
ACKD             bit     SBIC+1.2
ACKE             bit     SBIC+1.1
ACKT             bit     SBIC+1.0
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
SVA              sfr     0fe6h          ; [8W] Knotenadresse auf seriellem Bus
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PM7              bit     PMGB+1.3
PMGC             sfr     0feeh          ; [8W]      "        "
PM8              bit     PMGC.0
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
KR0              sfr     PORT6.0
KR1              sfr     PORT6.1
KR2              sfr     PORT6.2
KR3              sfr     PORT6.3
PORT7            sfr     0ff7h          ; [4]  Datenregister Port 7
KR4              sfr     PORT7.0
KR5              sfr     PORT7.1
KR6              sfr     PORT7.2
KR7              sfr     PORT7.3
PORT8            sfr     0ff8h          ; [4]  Datenregister Port 8

VI1              label   0006h          ; Interruptvektoradresse INT1
VICSI            label   0008h          ; Interruptvektoradresse INTCSI
VIT0             label   000ah          ; Interruptvektoradresse INTT0

RAMEnd           sfr     512            ; RAM-Grî·e
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=260
DSPM             sfr     0f88h          ; [4W] Modus Display
DIMS             sfr     0f89h          ; [4W] Dimmereinstellung Display
DIGS             sfr     0f8ah          ; [4]  Stellenauswahl Display
KSF              bit     DIGS.3
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST0             bit     PSW.2
MBE              bit     PSW.1
IME              bit     0fb2h.3        ; [8]  Interruptsperre (mit EI/DI ansprechen)
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
IEKS             bit     0fbbh.3        ; [1]  Freigabe/Anzeige Tastatur
IRQKS            bit     0fbbh.2
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint SIO,0fbdh     ; [4]  Freigabe/Anzeige Interrupt SIO
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
SIOM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
SSTART           sfr     01c0h          ; Beginn Anzeigespeicher
KS0              sfr     01fch          ; [8]  Tastaturregister
KS1              sfr     01feh          ; [4]
PORTH            sfr     01ffh          ; [4]  Datenregister Port H

VI1              label   0006h          ; Interruptvektoradresse INT1
VISIO            label   0008h          ; Interruptvektoradresse INTSIO
VIT0             label   000ah          ; Interruptvektoradresse INTT0
VIKS             label   000eh          ; Interruptvektoradresse INTKS

RAMEnd           sfr     512            ; RAM-Grî·e
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=300
LCDM             sfr     0f8ch          ; [8W] Modus LC-Display
LCDC             sfr     0f8eh          ; [4W] Steuerung LC-Display
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
TOE0             bit     0fa2h.3        ; [1W] Freigabe Ausgabe Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST0             bit     PSW.2
MBE              bit     PSW.1
IME              bit     0fb2h.3        ; [8]  Interruptsperre (mit EI/DI ansprechen)
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
IM2              sfr     0fb6h          ; [4W] INT2 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint CSI,0fbdh     ; [4]  Freigabe/Anzeige Interrupt CSI
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
BSB0             sfr     0fc0h          ; [4]  bitsequentielle Puffer
BSB1             sfr     0fc1h
BSB2             sfr     0fc2h
BSB3             sfr     0fc3h
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
POGA             sfr     0fdch          ; [8W] Steuerung Pull-Ups Port A
CSIM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
CSIE             bit     CSIM+1.3
COI              bit     CSIM+1.2
WUP              bit     CSIM+1.1
SBIC             sfr     0fe2h          ; [1]  Steuerung SBI
CMDD             bit     SBIC.3
RELD             bit     SBIC.2
CMDT             bit     SBIC.1
RELT             bit     SBIC.0
BSYE             bit     SBIC+1.3
ACKD             bit     SBIC+1.2
ACKE             bit     SBIC+1.1
ACKT             bit     SBIC+1.0
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
SVA              sfr     0fe6h          ; [8W] Knotenadresse auf seriellem Bus
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PM7              bit     PMGB+1.3
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
KR0              sfr     PORT6.0
KR1              sfr     PORT6.1
KR2              sfr     PORT6.2
KR3              sfr     PORT6.3
PORT7            sfr     0ff7h          ; [4]  Datenregister Port 7
KR4              sfr     PORT7.0
KR5              sfr     PORT7.1
KR6              sfr     PORT7.2
KR7              sfr     PORT7.3

VI1              label   0006h          ; Interruptvektoradresse INT1
VICSI            label   0008h          ; Interruptvektoradresse INTCSI
VIT0             label   000ah          ; Interruptvektoradresse INTT0

RAMEnd           sfr     512            ; RAM-Grî·e
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=320
LCDM             sfr     0f8ch          ; [8W] Modus LC-Display
LCDC             sfr     0f8eh          ; [4W] Steuerung LC-Display
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
TOE0             bit     0fa2h.3        ; [1W] Freigabe Ausgabe Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST0             bit     PSW.2
MBE              bit     PSW.1
IME              bit     0fb2h.3        ; [8]  Interruptsperre (mit EI/DI ansprechen)
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
IM2              sfr     0fb6h          ; [4W] INT2 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint CSI,0fbdh     ; [4]  Freigabe/Anzeige Interrupt CSI
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
BSB0             sfr     0fc0h          ; [4]  bitsequentielle Puffer
BSB1             sfr     0fc1h
BSB2             sfr     0fc2h
BSB3             sfr     0fc3h
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
ADM              sfr     0fd8h          ; [1]  Steuerung A/D-Wandler
SOC              sfr     ADM.3
EOC              sfr     ADM.2
SA               sfr     0fdah
POGA             sfr     0fdch          ; [8W] Steuerung Pull-Ups Port A
POBG             sfr     0fdeh          ; [8W] Steuerung Pull-Ups Port B
CSIM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
CSIE             bit     CSIM+1.3
COI              bit     CSIM+1.2
WUP              bit     CSIM+1.1
SBIC             sfr     0fe2h          ; [1]  Steuerung SBI
CMDD             bit     SBIC.3
RELD             bit     SBIC.2
CMDT             bit     SBIC.1
RELT             bit     SBIC.0
BSYE             bit     SBIC+1.3
ACKD             bit     SBIC+1.2
ACKE             bit     SBIC+1.1
ACKT             bit     SBIC+1.0
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
SVA              sfr     0fe6h          ; [8W] Knotenadresse auf seriellem Bus
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PM7              bit     PMGB+1.3
PMGC             sfr     0feeh          ; [8W]      "        "
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
KR0              sfr     PORT6.0
KR1              sfr     PORT6.1
KR2              sfr     PORT6.2
KR3              sfr     PORT6.3
PORT7            sfr     0ff7h          ; [4]  Datenregister Port 7
KR4              sfr     PORT7.0
KR5              sfr     PORT7.1
KR6              sfr     PORT7.2
KR7              sfr     PORT7.3
PORT8            sfr     0ff8h          ; [4]  Datenregister Port 8

VI1              label   0006h          ; Interruptvektoradresse INT1
VICSI            label   0008h          ; Interruptvektoradresse INTCSI
VIT0             label   000ah          ; Interruptvektoradresse INTT0

RAMEnd           sfr     512            ; RAM-Grî·e
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=100
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
TOE0             bit     0fa2h.3        ; [1W] Freigabe Ausgabe Timer 0
TO0              bit     0fa2h.2        ; [1W] Ausgabe Timer 0
TOF0             bit     0fa2h.1        ; [1W] Ausgabeflipflop Timer 0
TI0              bit     0fa2h.0        ; [1W] Eingabe Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
TM1              sfr     0fa8h          ; [8]  Modus Timer 1
TOE1             bit     0faah.3        ; [1W] Freigabe Ausgabe Timer 1
TO1              bit     0faah.2        ; [1W] Ausgabe Timer 1
TOF1             bit     0faah.1        ; [1W] Ausgabeflipflop Timer 1
TI1              bit     0faah.0        ; [1W] Eingabe Timer 1
T1               sfr     0fach          ; [8R] ZÑhlwert Timer 1
TMOD1            sfr     0faeh          ; [8W] Moduloregister Timer 1
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST1             bit     PSW.3
IST0             bit     PSW.2
MBE              bit     PSW.1
RBE              bit     PSW.0
IPS              sfr     0fb2h          ; [4W] InterruptprioritÑten
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
IET1             bit     IT0.3
IRQT1            bit     IT0.2
                 __defint SIO,0fbdh     ; [4]  Freigabe/Anzeige Interrupt SIO
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
IE3              bit     I2.3           ;      Freigabe/Anzeige Interrupt 3
IRQ3             bit     I2.2
BSB0             sfr     0fc0h          ; [4]  bitsequentielle Puffer
BSB1             sfr     0fc1h
BSB2             sfr     0fc2h
BSB3             sfr     0fc3h
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
PONF             bit     0fd1h.0        ; [1]  Power-on-Flag
PTHM             sfr     0fd6h          ; [8]  Schwellwerteinstellung
SIOM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PM7              bit     PMGB+1.3
PMGC             sfr     0feeh          ; [8W]      "        "
PM8              bit     PMGC.0
PM9              bit     PMGC.1
PM12             bit     PMGC+1.0
PM13             bit     PMGC+1.1
PM14             bit     PMGC+1.2
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
PORT7            sfr     0ff7h          ; [4]  Datenregister Port 7
PORT8            sfr     0ff8h          ; [4]  Datenregister Port 8
PORT9            sfr     0ff9h          ; [4]  Datenregister Port 9
PORT12           sfr     0ffch          ; [4]  Datenregister Port 12
PORT13           sfr     0ffdh          ; [4]  Datenregister Port 13
PORT14           sfr     0ffeh          ; [4]  Datenregister Port 14

VISIO            label   0006h          ; Interruptvektoradresse INTSIO
VIT0             label   0008h          ; Interruptvektoradresse INTT0
VIT1             label   000ah          ; Interruptvektoradresse INTT1

                 if      MOMCPU<75108h   ; RAM-Grî·e
RAMEnd            sfr     320
                 elseif
RAMEnd            sfr     512         
                 endif
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=200
DSPM             sfr     0f88h          ; [4W] Modus Display
DIMS             sfr     0f89h          ; [4W] Dimmereinstellung Display
DIGS             sfr     0f8ah          ; [4]  Stellenauswahl Display
KSF              bit     DIGS.3
TPGM             sfr     0f90h          ; [8W] Modus Pulsgenerator
MODL             sfr     0f94h          ; [8]  Modulo-Wert Pulsgenerator
MODH             sfr     0f96h          ; [8]
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST1             bit     PSW.3
IST0             bit     PSW.2
MBE              bit     PSW.1
RBE              bit     PSW.0
IPS              sfr     0fb2h          ; [4W] InterruptprioritÑten
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
                 __defint TPG,0fbbh     ; [4]  Freigabe/Anzeige Interrupt TPG
IEKS             bit     ITPG.3         ;      Freigabe/Anzeige Interrupt KS
IRQKS            bit     ITPG.2
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint SIO,0fbdh     ; [4]  Freigabe/Anzeige Interrupt SIO
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
PONF             bit     0fd1h.0        ; [1]  Power-on-Flag
SIOM             sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
SIO              sfr     0fe4h          ; [8]  Datenregister SIO
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
SSTART           sfr     01c0h          ; Beginn Anzeigespeicher
KS0              sfr     01fch          ; [8]  Tastaturregister
KS1              sfr     01feh          ; [4]
PORTH            sfr     01ffh          ; [4]  Datenregister Port H

VI1              label   0006h          ; Interruptvektoradresse INT1
VISIO            label   0008h          ; Interruptvektoradresse INTSIO
VIT0             label   000ah          ; Interruptvektoradresse INTT0
VITPG            label   000ch          ; Interruptvektoradresse INTTPG
VIKS             label   000eh          ; Interruptvektoradresse INTKS

                 if      MOMCPU<75108h   ; RAM-Grî·e
RAMEnd            sfr     396
                 elseif  MOMCPU<75212h
RAMEnd            sfr     497
                 elseif
RAMEnd            sfr     512         
                 endif
                endif

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

                if      __family=500
TPGM             sfr     0f90h          ; [8W] Modus Pulsgenerator
MODL             sfr     0f94h          ; [8]  Modulo-Wert Pulsgenerator
MODH             sfr     0f96h          ; [8]
WM               sfr     0f98h          ; [8]  Modus Watchdog
TM0              sfr     0fa0h          ; [8]  Modus Timer 0
TOE0             bit     0fa2h.3        ; [1W] Freigabe Ausgabe Timer 0
T0               sfr     0fa4h          ; [8R] ZÑhlwert Timer 0
TMOD0            sfr     0fa6h          ; [8W] Moduloregister Timer 0
PSW              sfr     0fb0h          ; [4]  Programmstatus
IST1             bit     PSW.3
IST0             bit     PSW.2
MBE              bit     PSW.1
RBE              bit     PSW.0
IPS              sfr     0fb2h          ; [4W] InterruptprioritÑten
PCC              sfr     0fb3h          ; [4W] Steuerung Prozessortakt
IM1              sfr     0fb5h          ; [4W] INT1 Modusregister
IM2              sfr     0fb6h          ; [4W] INT2 Modusregister
SCC              sfr     0fb7h          ; [1W] Steuerung Systemtakt
IE4              bit     IBT.3          ;      Freigabe/Anzeige Interrupt 4
IRQ4             bit     IBT.2
EOT              bit     0fb9h.0
                 __defint W,0fbah       ; [4]  Freigabe/Anzeige Interrupt W
                 __defint TPG,0fbbh     ; [4]  Freigabe/Anzeige Interrupt TPG
                 __defint T0,0fbch      ; [4]  Freigabe/Anzeige Interrupt T0
                 __defint CSIO,0fbdh    ; [4]  Freigabe/Anzeige Interrupt CSIO
                 __defint 0,0fbeh       ; [4]  Freigabe/Anzeige Interrupt 0
IE1              bit     I0.3           ;      Freigabe/Anzeige Interrupt 1
IRQ1             bit     I0.2
                 __defint 2,0fbfh       ; [4]  Freigabe/Anzeige Interrupt 2
BSB0             sfr     0fc0h          ; [4]  bitsequentielle Puffer
BSB1             sfr     0fc1h
BSB2             sfr     0fc2h
BSB3             sfr     0fc3h
CSIM1            sfr     0fc8h          ; [8W] Betriebsart serielle Schnittstelle
CSIE1            bit     CSIM1+1.3
SIO1             sfr     0fcch          ; [8]  Datenregister SIO
CLOM             sfr     0fd0h          ; [4W] Taktausgabe Modusregister
ADM              sfr     0fd8h          ; [1]  Steuerung A/D-Wandler
SOC              sfr     ADM.3
EOC              sfr     ADM.2
SA               sfr     0fdah
POGA             sfr     0fdch          ; [8W] Steuerung Pull-Ups Port A
CSIM0            sfr     0fe0h          ; [8W] Betriebsart serielle Schnittstelle
CSIE             bit     CSIM+1.3
COI              bit     CSIM+1.2
WUP              bit     CSIM+1.1
SBIC             sfr     0fe2h          ; [1]  Steuerung SBI
CMDD             bit     SBIC.3
RELD             bit     SBIC.2
CMDT             bit     SBIC.1
RELT             bit     SBIC.0
BSYE             bit     SBIC+1.3
ACKD             bit     SBIC+1.2
ACKE             bit     SBIC+1.1
ACKT             bit     SBIC+1.0
SIO0             sfr     0fe4h          ; [8]  Datenregister SIO
SVA              sfr     0fe6h          ; [8W] Knotenadresse auf seriellem Bus
PMGA             sfr     0fe8h          ; [8W] Betriebsart Ports
PM33             bit     PMGA.3
PM32             bit     PMGA.2
PM31             bit     PMGA.1
PM30             bit     PMGA.0
PM63             bit     PMGA+1.3
PM62             bit     PMGA+1.2
PM61             bit     PMGA+1.1
PM60             bit     PMGA+1.0
PMGB             sfr     0fech          ; [8W]      "        "
PM2              bit     PMGB.2
PM4              bit     PMGB+1.0
PM5              bit     PMGB+1.1
PM7              bit     PMGB+1.3
PMGC             sfr     0feeh          ; [8W]      "        "
PM8              bit     PMGC.0
PM9              bit     PMGC.1
PM12             bit     PMGC+1.0
PM13             bit     PMGC+1.1
PM14             bit     PMGC+1.2
PORT4            sfr     0ff4h          ; [4]  Datenregister Port 4
PORT7            sfr     0ff7h          ; [4]  Datenregister Port 7
PORT8            sfr     0ff8h          ; [4R] Datenregister Port 8
PORT9            sfr     0ff9h          ; [4]  Datenregister Port 9
PORT10           sfr     0ffah          ; [4]  Datenregister Port 10
PORT11           sfr     0ffbh          ; [4]  Datenregister Port 11
PORT12           sfr     0ffch          ; [4]  Datenregister Port 12
PORT13           sfr     0ffdh          ; [4]  Datenregister Port 13
PORT14           sfr     0ffeh          ; [4]  Datenregister Port 14
PORT15           sfr     0fffh          ; [4R] Datenregister Port 15

VI1              label   0006h          ; Interruptvektoradresse INT1
VICSIO           label   0008h          ; Interruptvektoradresse INTCSIO
VIT0             label   000ah          ; Interruptvektoradresse INTT0
VITPG            label   000ch          ; Interruptvektoradresse INTTPG

RAMEnd           sfr     512            ; RAM-Grî·e
                endif

                endif

		restore                 ; wieder erlauben

