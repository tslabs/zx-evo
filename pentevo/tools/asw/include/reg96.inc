		save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG96.INC                                              *
;*   								            *
;*   Sinn : enthÑlt Makro-, SFR- und Adre·definitionen fÅr die              *
;*          MCS-96-Prozessoren                                              *
;*          OK, der Name fÑllt aus dem Rahmen, aber STDDEF96.INC war schon  *
;*          belegt...                                                       *
;* 									    *
;*   letzte énderungen : 20. 2.1994                                         *
;*                       28. 6.1994 80196 (KR) hinzugefÅgt                  *
;*                        9. 3.1997 80196N(T), 80296 hinzugefÅgt            *
;*                                                                          *
;****************************************************************************

		ifndef  reg96inc     	; verhindert Mehrfacheinbindung

reg96inc     	equ     1

                if      (MOMCPUNAME<>"8096")&&(MOMCPUNAME<>"80196")
                 fatal  "Falscher Prozessortyp eingestellt: nur 8096, 80196, 80196N oder 80299 erlaubt!"
		endif


                if      MOMPASS=1
		 message "MCS-96-SFR-Definitionen (C) 1994 Alfred Arnold"
		 message "binde \{MOMCPU}-SFRs ein"
		endif

;----------------------------------------------------------------------------
; CPU-Register:

R0		equ	0		; Nullregister
ZERO_REG        equ     R0
ONES_REG        equ     2               ; nur Einsen
SP    		equ	18h		; Stackpointer
                if      MOMCPU>=80196h
WSR              equ     14h            ; RAM-Window in Registerbank
                 if      MOMCPU>=80296h
WSR1              equ     15h           ; RAM-Window in untere HÑlfte Registerbank
                 endif
                endif

                if      MOMCPU>=80296h  ; Signalprozessorregister
ACC_00           equ     000ch          ; Akkumulator
ACC_02           equ     000eh
ACC_04           equ     0006h
ACC_STAT         equ     000bh
ICB0             equ     1fc3h          ; Indexregister
ICB1             equ     1fc7h
ICX0             equ     0010h
ICX1             equ     0016h
IDX0             equ     1fc0h
IDX1             equ     1fc4h
RPT_CNT          equ     0004h          ; ZÑhlregister
                endif

;----------------------------------------------------------------------------
; SFRs:

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; A/D-Wandler
                switch  MOMCPUNAME

                case    "80296"         ; Warnung unterdrÅcken


                case    "80196N"
AD_RESULT        equ     1faah          ; Ergebnis Wandlung (wortweise lesbar)
AD_COMMAND       equ     1fach          ; Steuerung AD-Wandler
AD_TEST          equ     1faeh          ; Justage
AD_TIME          equ     1fafh          ; Wandlungsgeschwindigkeit


                case    "80196"
AD_Result        equ     1faah          ; Ergebnis Wandlung (wortweise lesbar)
AD_Command       equ     1fach          ; Steuerung AD-Wandler
AD_Test          equ     1faeh          ; Justage
AD_Time          equ     1fafh          ; Wandlungsgeschwindigkeit


                case    "8096"
AD_Command       equ     2              ; A/D-Wandler Steuerregister
AD_Result_Lo     equ     2              ; Ergebnis A/D-Wandler
AD_Result_Hi     equ     3              ; NUR byteweise lesen!!


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; HSI/HSO/EPA

                switch  MOMCPUNAME


                case    "80296"
EPA_MASK         equ     1f9ch          ; Interruptmasken
EPA_PEND         equ     1f9eh          ; Interruptanzeigen
__CNT            set     0
                 rept    4
EPA{"\{__CNT}"}_CON equ  1f60h+(__CNT*4) ; Steuerung Vergleicher/Fangeinheiten
EPA{"\{__CNT}"}_TIME equ 1f62h+(__CNT*4) ; Zeitpunkt     "            "
__CNT             set    __CNT+1
                 endm


                case    "80196N"
COMP0_CON        equ     1f88h          ; Steuerung Vergleicher 0
COMP0_TIME       equ     1f8ah          ; Zeitpunkt Vergleicher 0

COMP1_CON        equ     1f8ch          ; dito
COMP1_CON        equ     1f8eh

EPA_MASK         equ     1fa0h          ; Interruptmasken
EPA_MASK1        equ     1fa4h
EPA_PEND         equ     1fa2h          ; Interruptanzeigen
EPA_PEND1        equ     1fa6h
__CNT            set     0
                 rept    10
EPA{"\{__CNT}"}_CON equ  1f60h+(__CNT*4) ; Steuerung Vergleicher/Fangeinheiten
EPA{"\{__CNT}"}_TIME equ 1f62h+(__CNT*4) ; Zeitpunkt     "            "
__CNT             set    __CNT+1
                 endm
EPAIPV           equ     1fa8h          ; Interrupt-PrioritÑt


                case    "80196"
EPA_Mask         equ     1fa0h          ; (w) EPA Interruptfreigabe 4..9, 0..1, overrun 0..9/0/1
EPA_Mask1        equ     1fa4h
EPA_Pend         equ     1fa2h          ; (w) EPA Interrupt aufgetreten 4..9, 0..1, overrun 0..9/0/1
EPA_Pend1        equ     1fa6h
EPAIpv           equ     1fa8h          ; PrioritÑt EPA-Interrupt

EPA_Control0     equ     1f60h          ; Steuerung Capture/Compare Kanal 0..9
EPA_Time0        equ     1f62h          ; (w) gespeicherter Zeitpunkt Kanal 0..9
EPA_Control1     equ     1f64h
EPA_Time1        equ     1f66h
EPA_Control2     equ     1f68h
EPA_Time2        equ     1f6ah
EPA_Control3     equ     1f6ch
EPA_Time3        equ     1f6eh
EPA_Control4     equ     1f70h
EPA_Time4        equ     1f72h
EPA_Control5     equ     1f74h
EPA_Time5        equ     1f76h
EPA_Control6     equ     1f78h
EPA_Time6        equ     1f7ah
EPA_Control7     equ     1f7ch
EPA_Time7        equ     1f7eh
EPA_Control8     equ     1f80h
EPA_Time8        equ     1f82h
EPA_Control9     equ     1f84h
EPA_Time9        equ     1f86h
Comp_Control0    equ     1f88h          ; Steuerung Compare-Kanal 0..1
Comp_Time0       equ     1f8ah          ; Vergleichswert Compare-Kanal 0..1
Comp_Control1    equ     1f8ch
Comp_Time1       equ     1f8eh


                case    "8096"
HSI_Time         equ     4              ; HSI Triggerzeitpunkt (nur Wort)
HSI_Mode         equ     3              ; HSI Modusregister
HSI_Status       equ     6              ; HSI Statusregister

HSO_Time         equ     4              ; HSO Zeiteinstellung (nur Wort)
HSO_Command      equ     6              ; HSO Steuerregister


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; serielles

                switch  MOMCPUNAME


                case    "80296"
SP_CON           equ     1fbbh          ; Steuerung SIO
SP_BAUD          equ     1fbch          ; (w) Baudrate SIO
SP_STATUS        equ     1fb9h          ; Status SIO
SBUF_RX          equ     1fb8h          ; Empfangsdaten
SBUF_TX          equ     1fbah          ; Sendedaten


                case    "80196N"
SP_CON           equ     1fbbh          ; Steuerung SIO
SP_BAUD          equ     1fbch          ; (w) Baudrate SIO
SP_STATUS        equ     1fb9h          ; Status SIO
SBUF_RX          equ     1fb8h          ; Empfangsdaten
SBUF_TX          equ     1fbah          ; Sendedaten

SSIO_BAUD        equ     1fb4h          ; Baudrate SSIO
SSIO0_BUF        equ     1fb0h          ; Datenregister SSIO0
SSIO1_BUF        equ     1fb2h          ; Datenregister SSIO1
SSIO0_CON        equ     1fb1h          ; Modus SSIO0
SSIO1_CON        equ     1fb3h          ; Modus SSIO1


                case    "80196"
SSIO_StB0        equ     1fb0h          ; Datenpuffer SSIO Kanal 0
SSIO_StCr0       equ     1fb1h          ; Steuerung Sender/EmpfÑnger SSIO Kanal 0
SSIO_StB1        equ     1fb2h          ; Datenpuffer SSIO Kanal 1
SSIO_StCr1       equ     1fb3h          ; Steuerung Sender/EmpfÑnger SSIO Kanal 1
SSIO_Baud        equ     1fb4h          ; Baudrate SSIO
SBuf_RX          equ     1fb8h          ; Empfangsdaten
SP_Stat          equ     1fb9h          ; Zustand SIO
SBuf_TX          equ     1fbah          ; Sendedaten
SP_Con           equ     1fbbh          ; Steuerung SIO
SP_Baud          equ     1fbch          ; (w) Baudrate SIO


                case    "8096"
SBUF             equ     7              ; UART Datenregister
SPStat           equ     11h            ; UART Statusregister
SPCon            equ     11h            ; UART Steuerregister
Baud_Reg         equ     0eh            ; UART Datenrate


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Interruptsteuerung

                switch  MOMCPUNAME


                case    "80296"
INT_MASK         equ     08h            ; Interruptmasken
INT_MASK1        equ     13h
INT_PEND         equ     09h            ; Interruptanzeigen
INT_PEND1        equ     12h
EXTINT_CON       equ     1fcch          ; Flankenwahl externe Interrupts
IN_PROG0         equ     1fc8h          ; Interruptbearbeitung lÑuft
IN_PROG1         equ     1fcah
INT_CON0         equ     1fe8h          ; PrioritÑtsfestlegung
INT_CON1         equ     1feah
INT_CON2         equ     1fech
INT_CON3         equ     1feeh
NMI_PEND         equ     1fc9h          ; NMI angefordert ?
VECT_ADDR        equ     1ff0h          ; Vektorbasis

                case    "80196N"
INT_MASK         equ     08h            ; Interruptmasken
INT_MASK1        equ     13h
INT_PEND         equ     09h            ; Interruptanzeigen
INT_PEND1        equ     12h

PTSSRV           equ     06h            ; (w) PTS-Interrupt bedient
PTSSEL           equ     04h            ; (w) Quellen PTS freigeben


                case    "80196"
Int_Mask_Lo      equ     08h            ; Interruptfreigabe
Int_Mask_Hi      equ     13h
Int_Pend_Lo      equ     09h            ; wartende Interrupts
Int_Pend_Hi      equ     12h

PTS_Srv          equ     06h            ; w PTS-Interrupt bedient
PTS_Select       equ     04h            ; w Quellen PTS freigeben


                case    "8096"
INT_Mask         equ     8              ; Interruptfreigabe
INT_Pending      equ     9              ; Interrupts aufgetreten ?


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Timer

Watchdog	equ	0ah		; Watchdog zurÅcksetzen

                switch  MOMCPUNAME


                case    "80296"
CON_REG0         equ     1fb6h          ; Steuerung PWM
PWM0_CONTROL     equ     1fb0h          ; Einschaltdauer PWM0
PWM1_CONTROL     equ     1fb2h
PWM2_CONTROL     equ     1fb4h

T1CONTROL        equ     1f90h          ; Steuerung Timer 1
TIMER1           equ     1f92h          ; (w) Daten Timer 1
T2CONTROL        equ     1f94h          ; Steuerung Timer 2
TIMER22          equ     1f96h          ; (w) Daten Timer 2


                case    "80196N"
T1CONTROL        equ     1f98h          ; Steuerung Timer 1
TIMER1           equ     1f9ah          ; (w) Daten Timer 1
T2CONTROL        equ     1f9ch          ; Steuerung Timer 2
TIMER22          equ     1f9eh          ; (w) Daten Timer 2


                case    "80196"
Timer1_Control   equ     1f98h          ; Steuerung Timer 1
Timer1           equ     1f9ah          ; (w) Daten Timer 1
Timer2_Control   equ     1f9ch          ; Steuerung Timer 2
Timer2           equ     1f9eh          ; (w) Daten Timer 2


                case    "8096"
Timer1           equ     0ah            ; Timer 1 (nur Wort)
Timer2           equ     0ch            ; Timer 2 (nur Wort)

PWM_Control      equ     17h            ; Einstellung Pulsweite PWM


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Ports

                switch  MOMCPUNAME


                case    "80296"

P1_DIR           equ     1fd2h          ; Richtungsregister Port 1
P1_MODE          equ     1fd0h          ; Auswahl Port 1 Spezialfunktion/IO-Port
P1_PIN           equ     1fd6h          ; Status Port 1 Pins
P1_REG           equ     1fd4h          ; Port 1 Ausgabedaten

P2_DIR           equ     1fd3h          ; Rest sinngemÑ·
P2_MODE          equ     1fd1h
P2_PIN           equ     1fd7h
P2_REG           equ     1fd5h

P3_DIR           equ     1fdah
P3_MODE          equ     1fd8h
P3_PIN           equ     1fdeh
P3_REG           equ     1fdch

P4_DIR           equ     1fdbh
P4_MODE          equ     1fd9h
P4_PIN           equ     1fdfh
P4_REG           equ     1fddh

EP_DIR           equ     1fe3h
EP_MODE          equ     1fe1h
EP_PIN           equ     1fe7h
EP_REG           equ     1fe5h


                case    "80196N"
P0_PIN           equ     1fdah          ; Daten Port 0

P1_DIR           equ     1fd2h          ; Richtungsregister Port 1
P1_MODE          equ     1fd0h          ; Auswahl Port 1 Spezialfunktion/IO-Port
P1_PIN           equ     1fd6h          ; Status Port 1 Pins
P1_REG           equ     1fd4h          ; Port 1 Ausgabedaten

P2_DIR           equ     1fcbh          ; Rest sinngemÑ·
P2_MODE          equ     1fc9h
P2_PIN           equ     1fcfh
P2_REG           equ     1fcdh

P3_PIN           equ     1ffeh
P3_REG           equ     1ffch

P4_PIN           equ     1fffh
P4_REG           equ     1ffdh

P34_DRV          equ     1ff4h          ; Umschaltung Port 3/4 als open-drain oder push-pull

P5_DIR           equ     1ff3h
P5_MODE          equ     1ff1h
P5_PIN           equ     1ff7h
P5_REG           equ     1ff5h

P6_DIR           equ     1fd3h
P6_MODE          equ     1fd1h
P6_PIN           equ     1fd7h
P6_REG           equ     1fd5h

EP_DIR           equ     1fe3h
EP_MODE          equ     1fe1h
EP_PIN           equ     1fe7h
EP_REG           equ     1fe5h


                case    "80196"
P0PIn            equ     1fdah          ; Daten Port 0

P1SSel           equ     1fd0h          ; Spezialfunktionen auf Port 1 ?
P1IO             equ     1fd2h          ; Port 1 Datenrichtungsregister
P1Reg            equ     1fd4h          ; Port 1 Datenausgabe
P1PIn            equ     1fd6h          ; Eingangszustand Port 1

P2SSel           equ     1fc9h          ; Rest sinngemÑ·
P2IO             equ     1fcbh
P2Reg            equ     1fcdh
P2PIn            equ     1fcfh

P3Reg            equ     1ffch
P3PIn            equ     1ffeh

P4Reg            equ     1ffdh
P4PIn            equ     1fffh

P5SSel           equ     1ff1h
P5IO             equ     1ff3h
P5Reg            equ     1ff5h
P5PIn            equ     1ff7h

P6SSel           equ     1fd1h
P6IO             equ     1fd3h
P6Reg            equ     1fd5h
P6PIn            equ     1fd7h


                case    "8096"
Port0            equ     0eh            ; Port 0
Port1            equ     0fh            ; Port 1
Port2            equ     10h            ; Port 2
Port3            equ     1ffeh          ; Port 3 (falls kein ext. Speicher)
Port4            equ     1fffh          ; Port 4 (falls kein ext. Speicher)

IOS0             equ     15h            ; I/O-Statusregister 0
IOS1             equ     16h            ; I/O-Statusregister 1
IOC0             equ     15h            ; I/O-Statusregister 0
IOC1             equ     16h            ; I/O-Statusregister 1


                endcase

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; Slave-Port

                switch  MOMCPUNAME


                case    "80296"         ; Warnung unterdrÅcken


                case    "80196N"
SLP_CMD          equ     1ffah          ; Kommandoregister
SLP_CON          equ     1ffbh
SLP_STAT         equ     1ff8h


                case    "80196"         ; Warnung unterdrÅcken


                case    "8096"
SlpStat          equ     1ff8h          ; Status Slave-Port
SlpCmd           equ     1ffah          ; Kommandoregister Slave-Port
SlpFunReg        equ     1ffbh          ; Slave-Port Steuerung


                endcase

;----------------------------------------------------------------------------
; Vektoren/Sonderadressen

                if      MOMCPU>=80196h
EPAIntX_Vec      equ     2000h          ; Kanal 4...9/0/1 EPA
EPAInt3_Vec      equ     2002h          ; Kanal 3 EPA
EPAInt2_Vec      equ     2004h          ; Kanal 2 EPA
EPAInt1_Vec      equ     2006h          ; Kanal 1 EPA
EPAInt0_Vec      equ     2008h          ; Kanal 0 EPA
AD_Complete_Vec  equ     200ah          ; A/D-Wandlung abgeschlossen
OBE_Slp_Vec      equ     200ch          ; Ausgabepuffer leer
IBF_Slp_Vec      equ     200eh          ; Eingabepuffer voll
Trap_Vec         equ     2010h          ; Software-Trap
Ill_Opcode_Vec   equ     2012h          ; undefinierter Opcode
CMD_Full_Vec     equ     2030h          ; Kommandopuffer voll
SSIO0_Trans_Vec  equ     2032h          ; Transfer SSIO0 abgeschlossen
SSIO1_Trans_Vec  equ     2034h          ; Transfer SSIO1 abgeschlossen
SIO_TrInt_Vec    equ     2036h          ; SIO hat Zeichen versandt
SIO_RcInt_Vec    equ     2038h          ; SIO hat Zeichen empfangen
ExtInt_Vec       equ     203ch          ; externer Interrupt
NMI_Vec          equ     203eh          ; nicht mask. Interrupt
                elseif
TOverInt_Vec     equ     2000h          ; Vektor TimerÅberlauf
ADInt_Vec        equ     2002h          ; Vektor A/D-Wandlung beendet
HSIDataInt_Vec   equ     2004h          ; Vektor HSI-Daten verfÅgbar
HSOInt_Vec       equ     2006h          ; Vektor HSO
HSI0Int_Vec      equ     2008h          ; Vektor HSI.0
TimeInt_Vec      equ     200ah          ; Vektor fÅr Timer
SerialInt_Vec    equ     200ch          ; Vektor serieller Interrupt
ExtInt_Vec       equ     200eh          ; Vektor externer Interrupt
SoftInt_Vec      equ     2010h          ; TRAP Interruptvektor
Self_Jump        equ     201ah          ; Endlosschleife
                endif

;----------------------------------------------------------------------------
; Speicherbereiche, Spezialregister

                switch  MOMCPUNAME


                case    "80296"
IRAM             equ     1ah            ; Beginn internes RAM
IRAMEND          equ     1ffh           ; Ende internes RAM
CODERAM          equ     0f800h         ; Beginn Code-RAM
CODERAMEND       equ     0ffffh         ; Ende Code-RAM
EXTMEM1          equ     400h           ; Beginn externer Speicher Bereich 1
EXTMEM1END       equ     1bffh          ; Ende externer Speicher Bereich 1
EXTMEM2          equ     02000h         ; Beginn externer Speicher Bereich 2
EXTMEM2END       equ     0f7ffh         ; Ende externer Speicher Bereich 2
EXTMEM3          equ     0ff0400h       ; Beginn externer Speicher Bereich 3
EXTMEM3END       equ     0fff7ffh       ; Ende externer Speicher Bereich 3
CCB0             equ     0ff2018h       ; Chip-Konfiguration
CCB1             equ     0ff201ah       ; erw. Chip-Konfiguration
Reset_Location   equ     0ff2080h       ; Startadresse nach Reset


                case    "80196N"
IRAM             equ     1ah            ; Beginn internes RAM
IRAMEND          equ     3ffh           ; Ende internes RAM
IROM             equ     0ff2000h       ; Beginn internes Programm-ROM
IROMEND          equ     0ff9fffh       ; Ende internes Programm-ROM
EXTMEM1          equ     600h           ; Beginn externer Speicher Bereich 1
EXTMEM1END       equ     1effh          ; Ende externer Speicher Bereich 1
EXTMEM2          equ     0a000h         ; Beginn externer Speicher Bereich 2
EXTMEM2END       equ     0ffffh         ; Ende externer Speicher Bereich 2
EXTMEM3          equ     0ff0100h       ; Beginn externer Speicher Bereich 3
EXTMEM3END       equ     0ff03ffh       ; Ende externer Speicher Bereich 3
EXTMEM4          equ     0ff0600h       ; Beginn externer Speicher Bereich 4
EXTMEM4END       equ     0ff1fffh       ; Ende externer Speicher Bereich 4
EXTMEM5          equ     0ffa000h       ; Beginn externer Speicher Bereich 5
EXTMEM5END       equ     0ffffffh       ; Ende externer Speicher Bereich 5
CCB0             equ     0ff2018h       ; Chip-Konfiguration
CCB1             equ     0ff201ah       ; erw. Chip-Konfiguration
CCB2             equ     0ff201ch
Security_Key     equ     0ff2020h       ; SchlÅssel
Reset_Location   equ     0ff2080h       ; Startadresse nach Reset
USFR             equ     1ff6h          ; ???
IRAM_CON         equ     1fe0h          ; Konfiguration Code-RAM


                case    "80196"
IRAM             equ     1ah            ; Beginn internes RAM
IRAMEND          equ     1ffh           ; Ende internes RAM
PDRAM            equ     IRAM           ; Beginn Power-Down-RAM
PDRAMEND         equ     1ffh           ; Ende Power-Down-RAM
CODERAM          equ     400h           ; Beginn Code-RAM (KEIN Windowing!)
CODERAMEND       equ     4ffh           ; Ende Code-RAM
IROM             equ     2000h          ; Beginn internes Programm-ROM
IROMEND          equ     3fffh          ; Ende internes Programm-ROM
EXTMEM1          equ     500h           ; Beginn externer Speicher Bereich 1
EXTMEM1END       equ     1effh          ; Ende externer Speicher Bereich 1
EXTMEM2          equ     4000h          ; Beginn externer Speicher Bereich 2
EXTMEM2END       equ     0ffffh         ; Ende externer Speicher Bereich 2
CCB              equ     2018h          ; Chip-Konfiguration
CCB1             equ     201ah          ; erw. Chip-Konfiguration
Security_Key     equ     2020h          ; SchlÅssel
Reset_Location   equ     2080h          ; Startadresse nach Reset
USFR             equ     1ff6h          ; ???


                case    "8096"
IRAM             equ     1ah            ; Beginn internes RAM
IRAMEND          equ     0ffh           ; Ende internes RAM
PDRAM            equ     0f0h           ; Beginn Power-Down-RAM
PDRAMEND         equ     0ffh           ; Ende Power-Down-RAM
IROM             equ     2000h          ; Beginn internes Programm-ROM
IROMEND          equ     3fffh          ; Ende internes Programm-ROM
EXTMEM1          equ     100h           ; Beginn externer Speicher Bereich 1
EXTMEM1END       equ     1effh          ; Ende externer Speicher Bereich 1
EXTMEM2          equ     4000h          ; Beginn externer Speicher Bereich 2
EXTMEM2END       equ     0ffffh         ; Ende externer Speicher Bereich 2
CCB              equ     2018h          ; Chip-Konfiguration
Security_Key     equ     2020h          ; SchlÅssel
Reset_Location   equ     2080h          ; Startadresse nach Reset


                endcase

Security_Key_End equ    Security_Key+15

;--------------------------------------------------------------------------
; lange SprÅnge

__DefLongJmp    macro   new,old
new             macro   Adr,{NoExpand}
                old     Skip
                ljmp    Adr
Skip:
                endm
                endm

                __DefLongJmp bc,jnc
                __DefLongJmp be,jne
                __DefLongJmp bge,jlt
                __DefLongJmp bgt,jle
                __DefLongJmp bh,jnh
                __DefLongJmp ble,jgt
                __DefLongJmp blt,jge
                __DefLongJmp bnc,jc
                __DefLongJmp bne,je
                __DefLongJmp bnh,jh
                __DefLongJmp bnst,jst
                __DefLongJmp bnv,jv
                __DefLongJmp bnvt,jvt
                __DefLongJmp bst,jnst
                __DefLongJmp bv,jnv
                __DefLongJmp bvt,jnvt

;--------------------------------------------------------------------------

                endif

		restore                 ; wieder erlauben

