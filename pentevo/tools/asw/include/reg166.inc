                save
                listing off   ; kein Listing ueber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei REG166.INC                                             *
;*   								            *
;*   Sinn : enthaelt SFR-, Makro- und Adressdefinitionen fuer 80C166/167    *
;*          OK, der Name faellt aus dem Rahmen, aber STDDEF16.INC war schon *
;*          belegt...                                                       *
;* 									    *
;*   letzte Aenderungen : 23. 6.1994                                        *
;*                       27. 6.1994 Makros                                  *
;*                       28. 6.1994 Bitadressen                             *
;*			  9. 7.1994 Interrupt-Register-Korrektur            *
;*                       10. 7.1994 80C167-Register                         *
;*                       10.12.1994 Korrekturen/Erweiterungen 80C166        *
;*                                  (Hinweis von A. Kipper/A.Bergmann)      *
;*                       xx.06.2003 Erweiterungen C167CS/ST10F168           *
;*                                  (M. Reibel)                             *
;*                                                                          *
;****************************************************************************
;* $Id: reg166.inc,v 1.1 2003/11/06 02:56:33 alfred Exp $                  *
;**************************************************************************** 
;* $Log: reg166.inc,v $
;* Revision 1.1  2003/11/06 02:56:33  alfred
;* - recreated
;*
;* Revision 1.2  2003/07/06 10:09:54  alfred
;* - added C167CS registers (Marc Reibel)
;*
;****************************************************************************

		ifndef  reg166inc       ; verhindert Mehrfacheinbindung

reg166inc       equ     1

		switch	MOMCPUNAME
     		case   	"80C166"
_n   		equ	166
     		case   	"80C167"
_n  	 	equ    	167
     		case   	"80C167CS"
_n   		equ    	168
     		elsecase
     		fatal  "Falscher Prozessortyp: nur 80C166, 80C167 oder 80C167CS erlaubt!"
     		endcase



                if      MOMPASS=1
		 message "80C166/167-SFR-Definitionen (C) 1994 Alfred Arnold"
		 message "Erweiterung 80C167CS            2003 Marc Reibel"
		 message "binde \{MOMCPUNAME}-SFRs ein"
		endif

; damit's etwas kuerzer wird:

DefIntBits      macro   BASE,adr,{NoExpand} ; Interruptregister+Bits darin def.
tmpstr166	set	"BASE"
{tmpstr166}IC   equ     adr
{tmpstr166}_GLVL bit    {tmpstr166}IC.0 ; Gruppenprioritaet
{tmpstr166}_ILVL bit    {tmpstr166}IC.2 ; Interruptprioritaet
{tmpstr166}IE   bit     {tmpstr166}IC.6 ; Interruptfreigabe
{tmpstr166}IR   bit     {tmpstr166}IC.7 ; Interruptanzeige
		endm

;----------------------------------------------------------------------------
; Spezialregister:
; Bitadressen nur fuer bitadressierbare Register !
; bei Bitfeldern nur Startbit !

                if      _n>=167
SYSCON           equ     0ff12h         ; Systemkonfiguration
XPERSHARE        bit     SYSCON.0       ; XBUS Peripherie auch auf externem BUS
VISIBLE          bit     SYSCON.1       ; XBUS Peripherie auf Portpins
XPEN             bit     SYSCON.2       ; XBUS Peripherie freigeben
BDRSTEN          bit     SYSCON.3       ; Bidirektionaler /RSTIN
OWDDIS           bit     SYSCON.4       ; Oscillator Watchdog ausschalten
CSCFG            bit     SYSCON.6       ; Chip Select Mudus (1=unlatched)
WRCFG            bit     SYSCON.7       ; /WRL und /WRH statt /WR und /BHE
CLKEN            bit     SYSCON.8       ; Systemtakt auf Pin geben
BYTDIS           bit     SYSCON.9       ; /BHE Pin sperren
ROMENT           bit     SYSCON.10      ; Internes Rom anschalten
SGTDIS           bit     SYSCON.11      ; Speichersegmentierung sperren
ROMS1            bit     SYSCON.12      ; Lage internes ROM


		if	 _n>=168
SYSCON1		 equ	 0f1dch		; Sleepmodus bei IDLE-Anweisung
					; (Bit 1 u. 0)
					; 00 normaler IDLE-Modus
					; 01 SLEEP-Modus
					; 11 SLEEP-Modus, RTC und Oszillator aus

SYSCON2		 equ	 0f1d0h		; Taktgenerierung
RCS		 bit	 SYSCON2.6	; RTC Taktgeber Hauptoszillator
SCS		 bit	 SYSCON2.7	; SDD Taktgeber Hauptoszillator
CLKLOCK		 bit	 SYSCON2.15	; Hauptoszillator/PLL stabil?

RSTCON		 equ	 0f1e0h		; Reset Kontroll-Register

IDCHIP		 equ	 0f07ch		; Chip Identifizierungs-Register
IDMANUF		 equ	 0f07eh	
IDMEM2		 equ	 0f076h
IDMEM		 equ	 0f07ah
IDPROG		 equ	 0f078h

FOCON		 equ	 0ffaah		; Clock Output Kontroll-Register

SYSCON3		 equ	 0f1d4h		; Peripheriemanagement
ADCDIS		 bit	 SYSCON3.0	; AD sperren
ASC0DIS		 bit	 SYSCON3.1	; Asynchronen Seriellen Port sperren
SSCDIS		 bit	 SYSCON3.2	; Synchronen Seriellen Port sperren
GPTDIS		 bit	 SYSCON3.3	; GPT Bloecke sperren
DFMDIS		 bit	 SYSCON3.4	; DFlash sperren (nur Flashvarianten)
PFMDIS		 bit	 SYSCON3.5	; PFlash sperren (nur Flashvarianten)
CC1DIS		 bit	 SYSCON3.6	; Capcom1 sperren 
CC2DIS		 bit	 SYSCON3.7	; Capcom2 sperren
PWMDIS		 bit	 SYSCON3.9	; PWM sperren
CAN1DIS		 bit	 SYSCON3.13	; CAN1 sperren
CAN2DIS		 bit	 SYSCON3.14	; CAN2 sperren
PCDDIS		 bit	 SYSCON3.15	; Peripherietaktgeber sperren
		endif

                elseif
SYSCON           equ     0ff0ch
MCTC             bit     SYSCON.0       ; Speicherzykluslaenge
RWDC             bit     SYSCON.4       ; Schreib/Lese-Verzoegerung
MTTC             bit     SYSCON.5       ; Tri-State-Zeit
BTYP             bit     SYSCON.6       ; Status der EBC-Pins
RDYEN            bit     SYSCON.12      ; /READY freigeben
                endif
ROMEN           bit     SYSCON.10       ; internes ROM freigeben
STKSZ           bit     SYSCON.13       ; Maximalgroesse Stack

                if      _n>=167
RP0H             equ     0f108h         ; Systemanlaufregister (nur Lesen)
CSSEL            bit     RP0H.1         ; Anzahl CS-Leitungen
SALSEL           bit     RP0H.3         ; Anzahl Adressleitungen ab A16
                endif

PSW             equ     0ff10h          ; Flagregister
N               bit     PSW.0           ; negatives Ergebnis ?
C               bit     PSW.1           ; Carry ?
V               bit     PSW.2           ; Overflow ?
Z               bit     PSW.3           ; Ergebnis Null ?
E               bit     PSW.4           ; Tabellenende ?
MULIP           bit     PSW.5           ; Multiplikation/Division unterbrochen ?
USR0            bit     PSW.6           ; benutzerdefiniert
		if      _n>=167
HLDEN		 bit	 PSW.10		; Hold freigeben
		endif
IEN             bit     PSW.11          ; Interrupts freigeben
ILVL            bit     PSW.12          ; mom. bediente Interruptebene

TFR             equ     0ffach          ; Trap Flagregister
ILLBUS          bit     TFR.0           ; externer Bus benutzt, aber nicht konfiguriert
ILLINA          bit     TFR.1           ; Codezugriff auf ungerade Adresse
ILLOPA          bit     TFR.2           ; Wortzugriff auf ungerade Adresse
PRTFLT          bit     TFR.3           ; Schutzverletzung
UNDOPC          bit     TFR.7           ; undefinierter Maschinenbefehl
STKUF           bit     TFR.13          ; Stack-Unterlauf
STKOF           bit     TFR.14          ; Stack-Ueberlauf
NMI             bit     TFR.15          ; nicht maskierbarer Interrupt
                if      _n>=167
EXICON           equ     0f1c0h         ; External Steuerung externer Interrupt-Controller
		if	_n>=168
EXISEL		 equ	 0f1dah		; External Interrupt Source Register
		endif
EXI0ES           bit     EXICON.0       ; Flankensteuerung Interrupt 0
EXI1ES           bit     EXICON.2       ; Flankensteuerung Interrupt 1
EXI2ES           bit     EXICON.4       ; Flankensteuerung Interrupt 2
EXI3ES           bit     EXICON.6       ; Flankensteuerung Interrupt 3
EXI4ES           bit     EXICON.8       ; Flankensteuerung Interrupt 4
EXI5ES           bit     EXICON.10      ; Flankensteuerung Interrupt 5
EXI6ES           bit     EXICON.12      ; Flankensteuerung Interrupt 6
EXI7ES           bit     EXICON.14      ; Flankensteuerung Interrupt 7
                endif

DPP0            equ     0fe00h          ; Pagingregister
DPP1            equ     0fe02h
DPP2            equ     0fe04h
DPP3            equ     0fe06h

CSP             equ     0fe08h          ; Bits 16.. Programmzaehler

MDH             equ     0fe0ch          ; Multiplizier/Dividierregister
MDL             equ     0fe0eh
MDC             equ     0ff0eh          ; Steuerung Multiplizierer/Dividierer
MDRIU           bit     MDC.4           ; Multiplizierer in Benutzung

CP              equ     0fe10h          ; Registerkontextzeiger

SP              equ     0fe12h          ; Stackpointer
STKOV           equ     0fe14h          ; obere Grenze Stackpointer
STKUN           equ     0fe16h          ; untere Grenze Stackpointer

ZEROS           equ     0ff1ch          ; konstant Nullen
ONES            equ     0ff1eh          ; konstant Einsen

;----------------------------------------------------------------------------
; Speicher-Interface

                if      _n>=167
ADDRSEL1         equ     0fe18h         ; Adressdekoder Register 1..4
ADDRSEL2         equ     0fe1ah
ADDRSEL3         equ     0fe1ch
ADDRSEL4         equ     0fe1eh

BUSCON0          equ     0ff0ch         ; Buskonfiguration Register 0..4
MCTC0            bit     BUSCON0.0      ; Speicherzykluslaenge
RWDC0            bit     BUSCON0.4      ; Schreib/Leseverzoegerung
MTTC0            bit     BUSCON0.5      ; Tri-State-Zeit
BTYP0            bit     BUSCON0.6      ; Status EBC-Pins
ALECTL0          bit     BUSCON0.9      ; ALE Verl„ngerung
BUSACT0          bit     BUSCON0.10     ; Bus aktiv-Steuerbit
RDYEN0           bit     BUSCON0.12     ; /READY benutzen
BUSCON1          equ     0ff14h
MCTC1            bit     BUSCON1.0      ; Speicherzykluslaenge
RWDC1            bit     BUSCON1.4      ; Schreib/Leseverzoegerung
MTTC1            bit     BUSCON1.5      ; Tri-State-Zeit
BTYP1            bit     BUSCON1.6      ; Status EBC-Pins
ALECTL1          bit     BUSCON1.9      ; ALE Verlaengerung
BUSACT1          bit     BUSCON1.10     ; Bus aktiv-Steuerbit
RDYEN1           bit     BUSCON1.12     ; /READY benutzen
CSREN1           bit     BUSCON1.14     ; Arbeitsweise CS-Pins
CSWEN1           bit     BUSCON1.15
BUSCON2          equ     0ff16h
MCTC2            bit     BUSCON2.0      ; s.o.
RWDC2            bit     BUSCON2.4
MTTC2            bit     BUSCON2.5
BTYP2            bit     BUSCON2.6
ALECTL2          bit     BUSCON2.9
BUSACT2          bit     BUSCON2.10
RDYEN2           bit     BUSCON2.12
CSREN2           bit     BUSCON2.14
CSWEN2           bit     BUSCON2.15
BUSCON3          equ     0ff18h
MCTC3            bit     BUSCON3.0
RWDC3            bit     BUSCON3.4
MTTC3            bit     BUSCON3.5
BTYP3            bit     BUSCON3.6
ALECTL3          bit     BUSCON3.9
BUSACT3          bit     BUSCON3.10
RDYEN3           bit     BUSCON3.12
CSREN3           bit     BUSCON3.14
CSWEN3           bit     BUSCON3.15
BUSCON4          equ     0ff1ah
MCTC4            bit     BUSCON4.0
RWDC4            bit     BUSCON4.4
MTTC4            bit     BUSCON4.5
BTYP4            bit     BUSCON4.6
ALECTL4          bit     BUSCON4.9
BUSACT4          bit     BUSCON4.10
RDYEN4           bit     BUSCON4.12
CSREN4           bit     BUSCON4.14
CSWEN4		 bit     BUSCON4.15

		if	 _n>=168
XPERCON		 equ	 0f024h		; XPeripherie
XPER0_M		 equ	 0001h		; CAN1 (nach Reset an)
XPER1_M		 equ	 0002h		; CAN2 (nach Reset aus)
XPER10_M	 equ	 0400h		; XRAM 2 KByte (nach Reset an)
XPER11_M	 equ	 0800h		; XRAM 6 KByte (nach Reset aus)!!!
XPER14_M	 equ	 4000h		; 4KByte DFlash (nach Reset aus)!!!

ISNC		 equ	 0f1deh		; Interrupt SubNode Kontroll-Register
RTCIR		 bit	 ISNC.0		; RTC Interrupt Request Flag
RTCIE		 bit	 ISNC.1		; RTC Interrupt freigeben
PLLIR		 bit	 ISNC.2		; PLL Interrupt Request Flag
PLLIE		 bit	 ISNC.3		; PLL Interrupt freigeben
		endif

                 DefIntBits XP0,0f186h  ; Interruptsteuerung Peripheriegeraet 0
                 DefIntBits XP1,0f18eh  ; Interruptsteuerung Peripheriegeraet 1
                 DefIntBits XP2,0f196h  ; Interruptsteuerung Peripheriegeraet 2
					; Flash Terminierung (Flashvarianten) 
                 DefIntBits XP3,0f19eh  ; Interruptsteuerung Peripheriegeraet 3
                endif

;----------------------------------------------------------------------------
; Ports:

                if      _n>=167
PICON		 equ	 0f1c4h
                endif

                if      _n>=167
P0L              equ     0ff00h         ; Port 0 Datenregister
P0H              equ     0ff02h
DP0L             equ     0f100h         ; Port 0 Datenrichtungsregister
DP0H             equ     0f102h
		if	 _n>=168
POCON0H		 equ	 0f082h
POCON0L		 equ	 0f080h
		endif
                elseif
P0               equ     0ff00h
DP0              equ     0ff02h
                endif

                if      _n>=167
P1L              equ     0ff04h         ; Port 1 Datenregister
P1H              equ     0ff06h
DP1L             equ     0f104h         ; Port 1 Datenrichtungsregister
DP1H             equ     0f106h
		if	 _n>=168
P1DIDIS		 equ	 0fea4h		; Port 1 Digital-Input sperren
		endif
		if	 _n>=168
POCON1H		 equ	 0f086h		; Port 1 Ausgangstreiber
POCON1L		 equ	 0f084h
		endif
                elseif
P1               equ     0ff04h 
DP1              equ     0ff06h
                endif

P2              equ     0ffc0h          ; Port 2 Datenregister
DP2             equ     0ffc2h          ; Port 2 Datenrichtungsregister
                if      _n>=167
ODP2             equ     0f1c2h         ; Port 2 Open-Drain-Einstellung
P2LIN		 bit	 PICON.0	; Port 2 Eingangsschwelle Bit 0..7
P2HIN		 bit	 PICON.1	; Port 2 Eingangsschwelle Bit 8..15
		if	 _n>=168
POCON20		 equ	 0f0aah		; Port 2 Ded. Pin Kontroll-Register
					; /RSTOUT,CLKOUT/FOUT,ALE,
					; /WH,/RD,/BHE,/WH		
POCON2		 equ	 0f088h		; Port 2 Ausgangstreiber
		endif
                endif

P3              equ     0ffc4h          ; Port 3 Datenregister
DP3             equ     0ffc6h          ; Port 3 Datenrichtungsregister
                if      _n>=167
ODP3             equ     0f1c6h         ; Port 3 Open-Drain-Einstellung
P3LIN		 bit	 PICON.2	; Port 3 Eingangsschwelle Bit 0..7
P3HIN		 bit	 PICON.3	; Port 3 Eingangsschwelle Bit 8..15
		if	 _n>=168
POCON3		 equ	 0f08ah		; Port 3 Ausgangstreiber
		endif
                endif

                if      _n>=167
P4               equ     0ffc8h         ; Port 4 Datenregister
DP4              equ     0ffcah         ; Port 4 Datenrichtungsregister
ODP4		 equ	 0f1cah		; Port 4 Open-Drain-Einstellung
P4LIN		 bit	 PICON.4	; Port 4 Eingangsschwelle
		if	 _n>=168
POCON4		 equ	 0f08ch		; Port 4 Ausgangstreiber
		endif
                elseif
P4               equ     0ff08h
DP4              equ     0ff0ah
                endif

P5              equ     0ffa2h          ; Port 5 Datenregister (nur Eingang)

                if      _n>=167
P5DIDIS		 equ	 0ffa4h
		endif

                if      _n>=167
P6               equ     0ffcch         ; Port 6 Datenregister
DP6              equ     0ffceh         ; Port 6 Datenrichtungsregister
ODP6             equ     0f1ceh         ; Port 6 Open-Drain-Einstellung
P6LIN		 bit	 PICON.5	; Port 6 Eingangsschwelle
		if	 _n>=168
POCON6		 equ	 0f08eh		; Port 6 Ausgangstreiber
		endif
P7               equ     0ffd0h         ; Port 7 Datenregister
DP7              equ     0ffd2h         ; Port 7 Datenrichtungsregister
ODP7             equ     0f1d2h         ; Port 7 Open-Drain-Einstellung
P7LIN		 bit	 PICON.6	; Port 7 Eingangsschwelle
		if	 _n>=168
POCON7		 equ	 0f090h		; Port 7 Ausgangstreiber
		endif
P8               equ     0ffd4h         ; Port 8 Datenregister
DP8              equ     0ffd6h         ; Port 8 Datenrichtungsregister
ODP8             equ     0f1d6h         ; Port 8 Open-Drain-Einstellung
P8LIN		 bit	 PICON.7	; Port 8 Eingangsschwelle
		if	 _n>=168
POCON8		 equ	 0f092h		; Port 8 Ausgangstreiber
		endif
                endif

;----------------------------------------------------------------------------
; Timer:

T0              equ     0fe50h          ; CAPCOM Timer 0
T0REL           equ     0fe54h          ; Timer 0 Reload Register
T01CON          equ     0ff50h          ; Steuerregister Timer 0+1
T0I             bit     T01CON.0        ; Timer 0 Eingangssignalwahl
T0M             bit     T01CON.3        ; Timer 0 Modus
T0R             bit     T01CON.6        ; Timer 0 starten/stoppen
T1I             bit     T01CON.8        ; Timer 1 Eingangssignalwahl
T1M             bit     T01CON.11       ; Timer 1 Modus
T1R             bit     T01CON.14       ; Timer 1 starten/stoppen
                DefIntBits T0,0ff9ch    ; Timer 0 Interruptsteuerung

T1              equ     0fe52h          ; CAPCOM Timer 1
T1REL           equ     0fe56h          ; Timer 1 Reload Register
                DefIntBits T1,0ff9eh    ; Timer 1 Interruptsteuerung

T2              equ     0fe40h          ; Timer 2
T2CON           equ     0ff40h          ; Steuerregister Timer 2
T2I             bit     T2CON.0         ; Eingangssignal Timer 2
T2M             bit     T2CON.3         ; Modus Timer 2
T2R             bit     T2CON.6         ; Timer 2 starten/stoppen
T2UD            bit     T2CON.7         ; Timer 2 Zaehlrichtung
                if      _n>=167
T2UDE            bit    T2CON.8         ; Timer 2 externe Zaehlrichtungswahl
                endif
		DefIntBits T2,0ff60h    ; Timer 2 Interruptsteuerung

T3              equ     0fe42h          ; Timer 3
T3CON           equ     0ff42h          ; Steuerregister Timer 3
T3I             bit     T3CON.0         ; Eingangssignal Timer 3
T3M             bit     T3CON.3         ; Modus Timer 3
T3R             bit     T3CON.6         ; Timer 3 starten/stoppen
T3UD            bit     T3CON.7         ; Timer 3 Zaehlrichtung
T3UDE           bit     T3CON.8         ; Timer 3 externe Zaehlrichtungswahl
T3OE            bit     T3CON.9         ; Timer 3 Ausgang freischalten
T3OTL           bit     T3CON.10        ; Timer 3 uebergelaufen ?
		DefIntBits T3,0ff62h    ; Timer 3 Interruptsteuerung

T4              equ     0fe44h          ; Timer 4
T4CON           equ     0ff44h          ; Steuerregister Timer 4
T4I             bit     T4CON.0         ; Eingangssignal Timer 4
T4M             bit     T4CON.3         ; Modus Timer 4
T4R             bit     T4CON.6         ; Timer 4 starten/stoppen
T4UD            bit     T4CON.7         ; Timer 4 Zaehlrichtung
                if      _n>=167
T4UDE            bit    T4CON.8         ; Timer 4 externe Zaehlrichtungswahl
                endif
                DefIntBits T4,0ff64h    ; Timer 4 Interruptsteuerung

T5              equ     0fe46h          ; Timer 5
T5CON           equ     0ff46h          ; Steuerregister Timer 5
T5I             bit     T5CON.0         ; Eingangssignal Timer 5
T5M             bit     T5CON.3         ; Modus Timer 5
T5R             bit     T5CON.6         ; Timer 5 starten/stoppen
T5UD            bit     T5CON.7         ; Timer 5 Zaehlrichtung
                if      _n>=167
T5UDE            bit    T5CON.8         ; Timer 5 externe Zaehlrichtungswahl
                endif
CI              bit     T5CON.12        ; CAPREL Eingangswahl
T5CLR           bit     T5CON.14        ; Timer 5 bei Fang loeschen ?
T5SC            bit     T5CON.15        ; Timer 5 Fangen freigeben
		DefIntBits T5,0ff66h    ; Timer 5 Interruptsteuerung

T6              equ     0fe48h          ; Timer 6
T6CON           equ     0ff48h          ; Steuerregister Timer 6
T6I             bit     T6CON.0         ; Eingangssignal Timer 6
                if      _n>=167
T6M              bit    T6CON.3         ; Modus Timer 6
                endif
T6R             bit     T6CON.6         ; Timer 6 starten/stoppen
T6UD            bit     T6CON.7         ; Timer 6 Z„hlrichtung
                if      _n>=167
T6UDE            bit    T6CON.8         ; Timer 6 externe Zaehlrichtungswahl
                endif
T6OE            bit     T6CON.9         ; Timer 6 Ausgang freischalten
T6OTL           bit     T6CON.10        ; Timer 6 uebergelaufen ?
T6SR            bit     T6CON.15        ; Timer 6 Reload
		DefIntBits T6,0ff68h    ; Timer 6 Interruptsteuerung

                if      _n>=167
T78CON           equ     0ff20h         ; Steuerregister Timer 7+8

T7               equ     0f050h         ; Timer 7
T7REL            equ     0f054h         ; Timer 7 Reload Register
T7M		 bit	 T78CON.3	; Timer 7 Timer/Counter Mode
                 DefIntBits T7,0f17ah   ; Timer 7 Interruptsteuerung

T8               equ     0f052h         ; CAPCOM Timer 8
T8REL            equ     0f056h         ; Timer 8 Reload Register
T8M		 bit	 T78CON.11	; Timer 8 Timer/Counter Mode
                 DefIntBits T8,0f17ch   ; Timer 8 Interruptsteuerung
		if	 _n>=168
T14		 equ	 0f0d2h		; RTC Timer 14 
T14REL		 equ	 0f0d0h		; RTC Timer 14 Reload Register
RTCL		 equ	 0f0d4h		; RTC Low Word
RTCH		 equ	 0f0d6h		; RTC High Word
		endif
                endif

;-----------------------------------------------------------------------
; Capture/Compare-Einheit:

CAPREL          equ     0fe4ah		; Capture/Reload-Register
CC0             equ     0fe80h          ; CAPCOM Register
CC1             equ     0fe82h
CC2             equ     0fe84h
CC3             equ     0fe86h
CC4             equ     0fe88h
CC5             equ     0fe8ah
CC6             equ     0fe8ch
CC7             equ     0fe8eh
CC8             equ     0fe90h
CC9             equ     0fe92h
CC10            equ     0fe94h
CC11            equ     0fe96h
CC12            equ     0fe98h
CC13            equ     0fe9ah
CC14            equ     0fe9ch
CC15            equ     0fe9eh
                if      _n>=167
CC16             equ     0fe60h         ; weitere CAPCOM Register
CC17             equ     0fe62h
CC18             equ     0fe64h
CC19             equ     0fe66h
CC20             equ     0fe68h
CC21             equ     0fe6ah
CC22             equ     0fe6ch
CC23             equ     0fe6eh
CC24             equ     0fe70h
CC25             equ     0fe72h
CC26             equ     0fe74h
CC27             equ     0fe76h
CC28             equ     0fe78h
CC29             equ     0fe7ah
CC30             equ     0fe7ch
CC31             equ     0fe7eh
                endif

CCM0            equ     0ff52h          ; CAPCOM Modusregister
CCMOD0          bit     CCM0.0          ; CC0 Modusauswahl
ACC0            bit     CCM0.3          ; CC0 Timer 0/1 zuordnen
CCMOD1          bit     CCM0.4          ; CC1 Modusauswahl
ACC1            bit     CCM0.7          ; CC1 Timer 0/1 zuordnen
CCMOD2          bit     CCM0.8          ; CC2 Modusauswahl
ACC2            bit     CCM0.11         ; CC2 Timer 0/1 zuordnen
CCMOD3          bit     CCM0.12         ; CC3 Modusauswahl
ACC3            bit     CCM0.15         ; CC3 Timer 0/1 zuordnen

CCM1            equ     0ff54h
CCMOD4          bit     CCM1.0          ; CC4 Modusauswahl
ACC4            bit     CCM1.3          ; CC4 Timer 0/1 zuordnen
CCMOD5          bit     CCM1.4          ; CC5 Modusauswahl
ACC5            bit     CCM1.7          ; CC5 Timer 0/1 zuordnen
CCMOD6          bit     CCM1.8          ; CC6 Modusauswahl
ACC6            bit     CCM1.11         ; CC6 Timer 0/1 zuordnen
CCMOD7          bit     CCM1.12         ; CC7 Modusauswahl
ACC7            bit     CCM1.15         ; CC7 Timer 0/1 zuordnen

CCM2            equ     0ff56h
CCMOD8          bit     CCM2.0          ; CC8 Modusauswahl
ACC8            bit     CCM2.3          ; CC8 Timer 0/1 zuordnen
CCMOD9          bit     CCM2.4          ; CC9 Modusauswahl
ACC9            bit     CCM2.7          ; CC9 Timer 0/1 zuordnen
CCMOD10         bit     CCM2.8          ; CC10 Modusauswahl
ACC10           bit     CCM2.11         ; CC10 Timer 0/1 zuordnen
CCMOD11         bit     CCM2.12         ; CC11 Modusauswahl
ACC11           bit     CCM2.15         ; CC11 Timer 0/1 zuordnen

CCM3            equ     0ff58h
CCMOD12         bit     CCM3.0          ; CC12 Modusauswahl
ACC12           bit     CCM3.3          ; CC12 Timer 0/1 zuordnen
CCMOD13         bit     CCM3.4          ; CC13 Modusauswahl
ACC13           bit     CCM3.7          ; CC13 Timer 0/1 zuordnen
CCMOD14         bit     CCM3.8          ; CC14 Modusauswahl
ACC14           bit     CCM3.11         ; CC14 Timer 0/1 zuordnen
CCMOD15         bit     CCM3.12         ; CC15 Modusauswahl
ACC15           bit     CCM3.15         ; CC15 Timer 0/1 zuordnen

CCM4            equ     0ff22h
CCMOD16         bit     CCM4.0          ; CC16 Modusauswahl
ACC16           bit     CCM4.3          ; CC16 Timer 0/1 zuordnen
CCMOD17         bit     CCM4.4          ; CC17 Modusauswahl
ACC17           bit     CCM4.7          ; CC17 Timer 0/1 zuordnen
CCMOD18         bit     CCM4.8          ; CC18 Modusauswahl
ACC18           bit     CCM4.11         ; CC18 Timer 0/1 zuordnen
CCMOD19         bit     CCM4.12         ; CC19 Modusauswahl
ACC19           bit     CCM4.15         ; CC19 Timer 0/1 zuordnen

CCM5            equ     0ff24h
CCMOD20         bit     CCM5.0          ; CC20 Modusauswahl
ACC20           bit     CCM5.3          ; CC20 Timer 0/1 zuordnen
CCMOD21         bit     CCM5.4          ; CC21 Modusauswahl
ACC21           bit     CCM5.7          ; CC21 Timer 0/1 zuordnen
CCMOD22         bit     CCM5.8          ; CC22 Modusauswahl
ACC22           bit     CCM5.11         ; CC22 Timer 0/1 zuordnen
CCMOD23         bit     CCM5.12         ; CC23 Modusauswahl
ACC23           bit     CCM5.15         ; CC23 Timer 0/1 zuordnen

CCM6            equ     0ff26h
CCMOD24         bit     CCM6.0          ; CC24 Modusauswahl
ACC24           bit     CCM6.3          ; CC24 Timer 0/1 zuordnen
CCMOD25         bit     CCM6.4          ; CC25 Modusauswahl
ACC25           bit     CCM6.7          ; CC25 Timer 0/1 zuordnen
CCMOD26         bit     CCM6.8          ; CC26 Modusauswahl
ACC26           bit     CCM6.11         ; CC26 Timer 0/1 zuordnen
CCMOD27         bit     CCM6.12         ; CC27 Modusauswahl
ACC27           bit     CCM6.15         ; CC27 Timer 0/1 zuordnen

CCM7            equ     0ff28h
CCMOD28         bit     CCM7.0          ; CC28 Modusauswahl
ACC28           bit     CCM7.3          ; CC28 Timer 0/1 zuordnen
CCMOD29         bit     CCM7.4          ; CC29 Modusauswahl
ACC29           bit     CCM7.7          ; CC29 Timer 0/1 zuordnen
CCMOD30         bit     CCM7.8          ; CC30 Modusauswahl
ACC30           bit     CCM7.11         ; CC30 Timer 0/1 zuordnen
CCMOD31         bit     CCM7.12         ; CC31 Modusauswahl
ACC31           bit     CCM7.15         ; CC31 Timer 0/1 zuordnen

		DefIntBits CR,0ff6ah    ; CAPREL Interruptsteuerregister

		DefIntBits CC0,0ff78h   ; Interruptsteuerung CAPCOM-Kanaele
		DefIntBits CC1,0ff7ah
                DefIntBits CC2,0ff7ch
                DefIntBits CC3,0ff7eh
                DefIntBits CC4,0ff80h
                DefIntBits CC5,0ff82h
                DefIntBits CC6,0ff84h
                DefIntBits CC7,0ff86h
                DefIntBits CC8,0ff88h
                DefIntBits CC9,0ff8ah
		DefIntBits CC10,0ff8ch
		DefIntBits CC11,0ff8eh
		DefIntBits CC12,0ff90h
		DefIntBits CC13,0ff92h
		DefIntBits CC14,0ff94h
		DefIntBits CC15,0ff96h
                if      _n>=167
                 DefIntBits CC16,0f160h ; Interruptsteuerung weiterer CAPCOM-Kanaele
                 DefIntBits CC17,0f162h
                 DefIntBits CC18,0f164h
                 DefIntBits CC19,0f166h
                 DefIntBits CC20,0f168h
                 DefIntBits CC21,0f16ah
                 DefIntBits CC22,0f16ch
                 DefIntBits CC23,0f16eh
                 DefIntBits CC24,0f170h
                 DefIntBits CC25,0f172h
                 DefIntBits CC26,0f174h
                 DefIntBits CC27,0f176h
                 DefIntBits CC28,0f178h
                 DefIntBits CC29,0f184h
                 DefIntBits CC30,0f18ch
                 DefIntBits CC31,0f194h
                endif

PECC0           equ     0fec0h          ; PEC Kanalsteuerregister
PECC1           equ     0fec2h
PECC2           equ     0fec4h
PECC3           equ     0fec6h
PECC4           equ     0fec8h
PECC5           equ     0fecah
PECC6           equ     0fecch
PECC7           equ     0feceh

SRCP0           equ    0fce0h           ; PEC Quell-Register
SRCP1           equ    0fce4h
SRCP2           equ    0fce8h
SRCP3           equ    0fcech
SRCP4           equ    0fcf0h
SRCP5           equ    0fcf4h
SRCP6           equ    0fcf8h
SRCP7           equ    0fcfch
DSTP0           equ    0fce2h		; PEC Ziel-Register
DSTP1           equ    0fce6h
DSTP2           equ    0fceah
DSTP3           equ    0fceeh
DSTP4           equ    0fcf2h
DSTP5           equ    0fcf6h
DSTP6           equ    0fcfah
DSTP7           equ    0fcfeh

;----------------------------------------------------------------------------
; Pulsweitenmodulator:

                if      _n>=167
PP0              equ     0f038h         ; PWM Periode 0
PT0              equ     0f030h         ; PWM Zaehler 0
PW0              equ     0fe30h         ; PWM Pulsweite 0
PP1              equ     0f03ah         ; PWM Periode 1
PT1              equ     0f032h         ; PWM Zaehler 1
PW1              equ     0fe30h         ; PWM Pulsweite 1
PP2              equ     0f03ch         ; PWM Periode 2
PT2              equ     0f034h         ; PWM Zaehler 2
PW2              equ     0fe30h         ; PWM Pulsweite 2
PP3              equ     0f03eh         ; PWM Periode 3
PT3              equ     0f036h         ; PWM Zaehler 3
PW3              equ     0fe30h         ; PWM Pulsweite 3
PWMCON0          equ     0ff30h         ; PWM Seuerregister 0
PTR0             bit     PWMCON0.0      ; PT0 laufen lassen
PTR1             bit     PWMCON0.1      ; PT1 laufen lassen
PTR2             bit     PWMCON0.2      ; PT2 laufen lassen
PTR3             bit     PWMCON0.3      ; PT3 laufen lassen
PTI0             bit     PWMCON0.4      ; PT0 Eingangswahl
PTI1             bit     PWMCON0.5      ; PT1 Eingangswahl
PTI2             bit     PWMCON0.6      ; PT2 Eingangswahl
PTI3             bit     PWMCON0.7      ; PT3 Eingangswahl
PIE0             bit     PWMCON0.8      ; PT0 Interupts freigeben
PIE1             bit     PWMCON0.9      ; PT1 Interupts freigeben
PIE2             bit     PWMCON0.10     ; PT2 Interupts freigeben
PIE3             bit     PWMCON0.11     ; PT3 Interupts freigeben
PIR0             bit     PWMCON0.12     ; PT0 Interupt aufgetreten
PIR1             bit     PWMCON0.13     ; PT1 Interupt aufgetreten
PIR2             bit     PWMCON0.14     ; PT2 Interupt aufgetreten
PIR3             bit     PWMCON0.15     ; PT3 Interupt aufgetreten
PWMCON1          equ     0ff32h         ; PWM Steuerregister 1
PEN0             bit     PWMCON1.0      ; PT0 Ausgang freigeben
PEN1             bit     PWMCON1.1      ; PT1 Ausgang freigeben
PEN2             bit     PWMCON1.2      ; PT2 Ausgang freigeben
PEN3             bit     PWMCON1.3      ; PT3 Ausgang freigeben
PM0              bit     PWMCON1.4      ; PT0 Modus
PM1              bit     PWMCON1.5      ; PT1 Modus
PM2              bit     PWMCON1.6      ; PT2 Modus
PM3              bit     PWMCON1.7      ; PT3 Modus
PB01             bit     PWMCON1.12     ; PT0&1 Burst-Modus
PS2              bit     PWMCON1.14     ; PT2 Einzelimpulsmodus
PS3              bit     PWMCON1.15     ; PT3 Einzelimpulsmodus
                 DefIntBits PWM,0f17eh  ; PWM Interruptsteuerung
                endif

;----------------------------------------------------------------------------
; A/D-Wandler:

ADDAT           equ     0fea0h          ; A/D-Wandler Ergebnis
                if      _n>=167
ADDAT2           equ    0f0a0h          ; A/D-Wandler Ergebnis 2
                endif
		DefIntBits ADC,0ff98h   ; A/D-Wandler Interruptsteuerung fuer Wandlungsende
		DefIntBits ADE,0ff9ah   ; A/D-Wandler Interruptsteuerung fuer Fehler
ADCON           equ     0ffa0h          ; A/D-Wandler Steuerregister
ADCH            bit     ADCON.0         ; A/D-Wandler Kanalwahl
ADM             bit     ADCON.4         ; A/D-Wandler Modus
ADST            bit     ADCON.7         ; A/D-Wandlung starten
ADBSY           bit     ADCON.8         ; A/D-Wandler beschaeftigt
                if      _n>=167
ADWR             bit     ADCON.9        ; A/D-Wandler erst wieder starten, wenn gelesen
ADCIN            bit     ADCON.10       ; A/D-Wandler Kanalinjektion (??...)
ADCRQ            bit     ADCON.11       ; A/D-Wandler Kanalinjektion angefordert
                endif

;----------------------------------------------------------------------------
; Watchdog:

WDT             equ     0feaeh          ; Watchdog-Zaehler
WDTCON          equ     0ffaeh          ; Watchdog Steuerregister
WDTIN           bit     WDTCON.0        ; Watchdog Eingangsfrequenz
WDTR            bit     WDTCON.1        ; Reset durch Watchdog ?
		if	_n>=167
SWR		 bit	 WDTCON.2	; Software-Reset?
SHWR		 bit	 WDTCON.3	; Kurzer Hardware-Reset?
LHWR		 bit	 WDTCON.4	; Langer Hardware-Reset?
PONR		 bit	 WDTCON.5 	; PowerON-Reset? (Flash-Varianten)
		endif
WDTREL          bit     WDTCON.8        ; Reload-Wert Watchdog

;----------------------------------------------------------------------------
; Serielles:

S0TBUF          equ     0feb0h          ; SIO0 Senderegister
S0RBUF          equ     0feb2h          ; SIO0 Empfangsregister
S0BG            equ     0feb4h          ; SIO0 Baudrateneinstellung
		DefIntBits S0T,0ff6ch   ; Interruptsteuerung Sendeteil SIO0
		DefIntBits S0R,0ff6eh   ; Interruptsteuerung Empfangsteil SIO0
		DefIntBits S0E,0ff70h   ; Interruptsteuerung Fehlererkennung SIO0

                if      _n>=167
		 DefIntBits  S0TB,0f19ch ; Interruptsteuerung Sendepuffer
		endif

S0CON           equ     0ffb0h          ; Steuerregister SIO0
S0M             bit     S0CON.0         ; SIO0 Modus
S0STP           bit     S0CON.3         ; SIO0 Anzahl Stopbits
S0REN           bit     S0CON.4         ; SIO0 Empfaenger freigeben
S0PEN           bit     S0CON.5         ; SIO0 mit Paritaet
S0FEN           bit     S0CON.6         ; SIO0 mit Rahmenpruefung
S0OEN           bit     S0CON.7         ; SIO0 mit Ueberlaufpruefung
S0PE            bit     S0CON.8         ; SIO0 Paritaetsfehler
S0FE            bit     S0CON.9         ; SIO0 Rahmenfehler
S0OE            bit     S0CON.10        ; SIO0 Ueberlauffehler
                if      _n>=167
S0ODD            bit     S0CON.12       ; SIO0 Odd Parity
S0BRS            bit     S0CON.13       ; SIO0 Baudratenfaktor
                endif
S0LB            bit     S0CON.14        ; SIO0 Rueckkopplung
S0R             bit     S0CON.15        ; SIO0 Baudratengenerator freigeben

                if      _n>=167
SSCTB            equ     0f0b0h         ; SSC Senderegister
SSCRB            equ     0f0b2h         ; SSC Empfangsregister
SSCBR            equ     0f0b4h         ; SSC Baudrateneinstellung
                 DefIntBits SSCT,0ff72h ; Interruptsteuerung Sendeteil SSC
                 DefIntBits SSCR,0ff74h ; Interruptsteuerung Empfangsteil SSC
                 DefIntBits SSCE,0ff76h ; Interruptsteuerung Fehlererkennung SSC
SSCCON           equ     0ffb2h         ; Steuerregister SSC
SSCBM            bit     SSCCON.0       ; Wortbreite
SSCBC            bit     SSCCON.0       ; Bitzahl
SSCHB            bit     SSCCON.4       ; Startbitsteuerung
SSCPH            bit     SSCCON.5       ; Taktphase
SSCP0            bit     SSCCON.6       ; Taktpolaritaet
SSCTEN           bit     SSCCON.8       ; Sendefehler freigeben
SSCTE            bit     SSCCON.8       ; Sendefehler aufgetreten ?
SSCREN           bit     SSCCON.9       ; Empfangsfehler freigeben
SSCRE            bit     SSCCON.9       ; Empfangsfehler aufgetreten ?
SSCPEN           bit     SSCCON.10      ; Phasenfehler freigeben
SSCPE            bit     SSCCON.10      ; Phasenfehler aufgetreten ?
SSCBEN           bit     SSCCON.11      ; Baudratenfehler freigeben
SSCBE            bit     SSCCON.11      ; Baudratenfehler freigegeben ?
SSCBSY           bit     SSCCON.12      ; SSC beschaeftigt ?
SSCMS            bit     SSCCON.14      ; SSC als Master betreiben
SSCEN            bit     SSCCON.15      ; SSC freigeben
                elseif
S1TBUF           equ     0feb8h         ; SIO1 Senderegister
S1RBUF           equ     0febah         ; SIO1 Empfangsregister
S1BG             equ     0febch         ; SIO1 Baudrateneinstellung
                 DefIntBits S1T,0ff72h  ; Interruptsteuerung Sendeteil SIO1
                 DefIntBits S1R,0ff74h  ; Interruptsteuerung Empfangsteil SIO1
                 DefIntBits S1E,0ff76h  ; Interruptsteuerung Fehlererkennung SIO1
S1CON            equ     0ffb8h         ; Steuerregister SIO1
S1M              bit     S1CON.0        ; SIO1 Modus
S1STP            bit     S1CON.3        ; SIO1 Anzahl Stopbits
S1REN            bit     S1CON.4        ; SIO1 Empfaenger freigeben
S1PEN            bit     S1CON.5        ; SIO1 mit Paritaet
S1FEN            bit     S1CON.6        ; SIO1 mit Rahmenpruefung
S1OEN            bit     S1CON.7        ; SIO1 mit Ueberlaufpruefung
S1PE             bit     S1CON.8        ; SIO1 Paritaetsfehler
S1FE             bit     S1CON.9        ; SIO1 Rahmenfehler
S1OE             bit     S1CON.10       ; SIO1 Ueberlauffehler
S1LB             bit     S1CON.14       ; SIO1 Rueckkopplung
S1R              bit     S1CON.15       ; SIO1 Baudratengenerator freigeben
                endif

;----------------------------------------------------------------------------
; Canbus 1 und 2

                if      _n>=167

;bitram;	 DefIntBits C1, 0ef02h	; CAN1 Interrupt Kontroll-Register 
C1BTR		 equ	0ef04h		; CAN1 Bit Timing Register
C1CSR		 equ	0ef00h		; CAN1 Kontroll-Status-Register
C1GMS		 equ	0ef06h		; CAN1 Global Mask Short
C1LAR1		 equ	0ef14h		; CAN1 Lower Arbitration Register
C1LAR2		 equ	0ef24h
C1LAR3		 equ	0ef34h
C1LAR4		 equ	0ef44h
C1LAR5		 equ	0ef54h
C1LAR6		 equ	0ef64h
C1LAR7		 equ	0ef74h
C1LAR8		 equ	0ef84h
C1LAR9		 equ	0ef94h
C1LAR10		 equ	0efa4h
C1LAR11		 equ	0efb4h
C1LAR12		 equ	0efc4h
C1LAR13		 equ	0efd4h
C1LAR14		 equ	0efe4h
C1LAR15		 equ	0eff4h
C1LGML		 equ	0ef0ah		; CAN1 Lower Global Mask Support Long
C1LMLM		 equ	0ef0eh		; CAN1 Lower Mask Of Last Message
C1MCFG1		 equ	0ef16h		; CAN1 Message Configuration Register
C1MCFG2		 equ	0ef26h
C1MCFG3		 equ	0ef36h
C1MCFG4		 equ	0ef46h
C1MCFG5		 equ	0ef56h
C1MCFG6		 equ	0ef66h
C1MCFG7		 equ	0ef76h
C1MCFG8		 equ	0ef86h
C1MCFG9		 equ	0ef96h
C1MCFG10	 equ	0efa6h		
C1MCFG11	 equ	0efb6h
C1MCFG12	 equ	0efc6h
C1MCFG13	 equ	0efd6h
C1MCFG14	 equ	0efe6h
C1MCFG15	 equ	0eff6h
C1MCR1		 equ	0ef10h		; CAN1 Message Control Register 
C1MCR2		 equ	0ef20h
C1MCR3		 equ	0ef30h
C1MCR4		 equ	0ef40h
C1MCR5		 equ	0ef50h
C1MCR6		 equ	0ef60h
C1MCR7		 equ	0ef70h
C1MCR8		 equ	0ef80h
C1MCR9		 equ	0ef90h
C1MCR10		 equ	0efa0h
C1MCR11		 equ	0efb0h
C1MCR12		 equ	0efc0h
C1MCR13		 equ	0efd0h
C1MCR14		 equ	0efe0h
C1MCR15		 equ	0eff0h
C1PCIR		 equ	0ef02h		; CAN1 Port Control and Interrupt Register 
C1UAR1		 equ	0ef12h		; CAN1 Upper Arbitration Register
C1UAR2		 equ	0ef22h
C1UAR3		 equ	0ef32h
C1UAR4		 equ	0ef42h
C1UAR5		 equ	0ef52h
C1UAR6		 equ	0ef62h
C1UAR7		 equ	0ef72h
C1UAR8		 equ	0ef82h
C1UAR9		 equ	0ef92h
C1UAR10		 equ	0efa2h
C1UAR11		 equ	0efb2h
C1UAR12		 equ	0efc2h
C1UAR13		 equ	0efd2h
C1UAR14		 equ	0efe2h
C1UAR15		 equ	0eff2h
C1UGML		 equ	0ef08h		; CAN1 Upper Global Mask Long
C1UMLM		 equ	0ef0ch		; CAN1 Upper Mask Of Last Message
		endif
		if	_n>=168

;bitram;	 DefIntBits C2, 0ee02h	; CAN2 Interrupt Kontroll-Register 
C2BTR		 equ	0ee04h		; CAN2 Bit Timing Register
C2CSR		 equ	0ee00h		; CAN2 Control Status Register
C2GMS		 equ	0ee06h		; CAN2 Global Mask Support
C2LAR1		 equ	0ee14h		; CAN2 Lower Arbitration Register
C2LAR2		 equ	0ee24h
C2LAR3		 equ	0ee34h
C2LAR4		 equ	0ee44h
C2LAR5		 equ	0ee54h
C2LAR6		 equ	0ee64h
C2LAR7		 equ	0ee74h
C2LAR8		 equ	0ee84h
C2LAR9		 equ	0ee94h
C2LAR10		 equ	0eea4h
C2LAR11		 equ	0eeb4h
C2LAR12		 equ	0eec4h
C2LAR13		 equ	0eed4h
C2LAR14		 equ	0eee4h
C2LAR15		 equ	0eef4h
C2LGML		 equ	0ee0ah		; CAN2 Lower Global Mask Support
C2LMLM		 equ	0ee0eh		; CAN2 Lower Mask Of Last Message
C2MCFG1		 equ	0ee16h		; CAN2 Message Configuration Register
C2MCFG2		 equ	0ee26h
C2MCFG3		 equ	0ee36h
C2MCFG4		 equ	0ee46h
C2MCFG5		 equ	0ee56h
C2MCFG6		 equ	0ee66h
C2MCFG7		 equ	0ee76h
C2MCFG8		 equ	0ee86h
C2MCFG9		 equ	0ee96h
C2MCFG10	 equ	0eea6h
C2MCFG11	 equ	0eeb6h
C2MCFG12	 equ	0eec6h
C2MCFG13	 equ	0eed6h
C2MCFG14	 equ	0eee6h
C2MCFG15	 equ	0eef6h
C2MCR1		 equ	0ee10h		; CAN2 Message Control Register
C2MCR2		 equ	0ee20h
C2MCR3		 equ	0ee30h
C2MCR4		 equ	0ee40h
C2MCR5		 equ	0ee50h
C2MCR6		 equ	0ee60h
C2MCR7		 equ	0ee70h
C2MCR8		 equ	0ee80h
C2MCR9		 equ	0ee90h
C2MCR10		 equ	0eea0h
C2MCR11		 equ	0eeb0h
C2MCR12		 equ	0eec0h
C2MCR13		 equ	0eed0h
C2MCR14		 equ	0eee0h
C2MCR15		 equ	0eef0h
C2PCIR		 equ	0ee02h		; CAN2 Port Control And Interrupt Register
C2UAR1		 equ	0ee12h		; CAN2 Upper Arbitration Register
C2UAR2		 equ	0ee22h
C2UAR3		 equ	0ee32h
C2UAR4		 equ	0ee42h
C2UAR5		 equ	0ee52h
C2UAR6		 equ	0ee62h
C2UAR7		 equ	0ee72h
C2UAR8		 equ	0ee82h
C2UAR9		 equ	0ee92h
C2UAR10		 equ	0eea2h
C2UAR11		 equ	0eeb2h
C2UAR12		 equ	0eec2h
C2UAR13		 equ	0eed2h
C2UAR14		 equ	0eee2h
C2UAR15		 equ	0eef2h
C2UGML		 equ	0ee08h		; CAN2 Upper Global Mask Support
C2UMLM		 equ	0ee0ch		; CAN2 Upper Mask Of Last Message
		endif


;----------------------------------------------------------------------------
; Vektoren/Sonderadressen

RESET           equ     000h            ; Reseteinsprung
NMITRAP         equ     008h            ; NMI-Einsprung
STOTRAP         equ     010h            ; Einsprung Stackueberlauf
STUTRAP         equ     018h            ; Einsprung Stackunterlauf
BTRAP           equ     028h            ; undef. Opcode, Protection Fault,
                                        ; unerlaubter Wortzugriff, ungueltige Instruktionsadresse,
                                        ; ungueltiger Buszugriff
CC0INT          equ     040h            ; Interrupteinspruenge CAPCOM
CC1INT          equ     044h
CC2INT          equ     048h
CC3INT          equ     04ch
CC4INT          equ     050h
CC5INT          equ     054h
CC6INT          equ     058h
CC7INT          equ     05ch
CC8INT          equ     060h
CC9INT          equ     064h
CC10INT         equ     068h
CC11INT         equ     06ch
CC12INT         equ     070h
CC13INT         equ     074h
CC14INT         equ     078h
CC15INT         equ     07ch
                if      _n>=167
CC16INT          equ     0c0h
CC17INT          equ     0c4h
CC18INT          equ     0c8h
CC19INT          equ     0cch
CC20INT          equ     0d0h
CC21INT          equ     0d4h
CC22INT          equ     0d8h
CC23INT          equ     0dch
CC24INT          equ     0e0h
CC25INT          equ     0e4h
CC26INT          equ     0e8h
CC27INT          equ     0ech
CC28INT          equ     0f0h
CC29INT          equ    0110h
CC30INT          equ    0114h
CC31INT          equ    0118h
		endif


T0INT           equ     080h            ; Interrupteinsprung Timer 0
T1INT           equ     084h            ; Interrupteinsprung Timer 1
T2INT           equ     088h            ; Interrupteinsprung Timer 2
T3INT           equ     08ch            ; Interrupteinsprung Timer 3
T4INT           equ     090h            ; Interrupteinsprung Timer 4
T5INT           equ     094h            ; Interrupteinsprung Timer 5
T6INT           equ     098h            ; Interrupteinsprung Timer 6
                if      _n>=167
T7INT            equ	 0f4h            ; Interrupteinsprung Timer 7
T8INT            equ	 0f8h            ; Interrupteinsprung Timer 8
		endif


CRINT           equ     09ch            ; Interrupteinsprung CAPREL
ADCINT          equ     0a0h            ; Interrupteinsprung A/D-Wandlung fertig
ADEINT          equ     0a4h            ; Interrupteinsprung A/D-Wandler-Ueberlauf
S0TINT          equ     0a8h            ; Interrupteinsprung SIO0 wieder sendebereit
S0RINT          equ     0ach            ; Interrupteinsprung SIO0 hat Zeichen empfangen
S0EINT          equ     0b0h            ; Interrupteinsprung SIO0 hat Fehler erkannt
S1TINT          equ     0b4h            ; Interrupteinsprung SIO1 wieder sendebereit
S1RINT          equ     0b8h            ; Interrupteinsprung SIO1 hat Zeichen empfangen
S1EINT          equ     0bch            ; Interrupteinsprung SIO1 hat Fehler erkannt
                if      _n>=167
S0TBINT		 equ	011ch		; ASC0 Transmit Buffer
PWMINT		 equ	0fch		; PWM Channel 0..3
XP0INT		 equ	0100h		; CAN1
XP1INT		 equ	0104h		; CAN2
XP2INT		 equ	0108h		; Unassigned Node
XP3INT		 equ	010ch		; PLL,OWD,RTC
		endif


;----------------------------------------------------------------------------
; Speicherbereiche
;
		switch 	 _n
		case	168
IRAM            equ     0f200h		; Beginn internes RAM
		case	167
IRAM            equ     0f600h
		elsecase
IRAM            equ     0fa00h
		endcase	
IRAMEND         equ     0fdffh          ; Ende      "      "


BITRAM          equ     0fd00h          ; Beginn bitadressierbarer IRAM-Bereich
BITRAMEND       equ     0fdffh          ; Ende           "          "     "

		if	_n>=167
BITRAM1           equ     0f100h         ; Beginn bitadressierbarer ESFR Bereich
BITRAM1END        equ     0f1ffh         ; Ende           "          "     "
BITRAM2           equ     0ff00h         ; Beginn bitadressierbarer SFR Bereich
BITRAM2END        equ     0ffffh         ; Ende           "          "     "
		endif


                if      _n>=167
PECPTR           equ     0fce0h         ; Begin PEC-Vektoren (optional)
PECPTREND        equ     0fcffh         ; Ende   "     "
                elseif
PECPTR           equ     0fde0h
PECPTREND        equ     0fdffh
                endif

SFRSTART         equ     0fe00h         ; Beginn SFR-Bereich
SFREND           equ     0ffffh         ; Ende SFR-Bereich
	
                if      _n>=167
ESFRSTART        equ     0f000h         ; Beginn erweiterter SFR-Bereich
ESFREND          equ     0f1ffh         ; Ende erweiterter SFR-Bereich
                endif

IROM            equ     0               ; Begin internes ROM
                if      _n>=167
IROMEND          equ     07fffh         ; Ende internes ROM (n. voll ausgebaut)
                elseif
IROMEND          equ     01fffh
                endif
		
		if	_n>=168
EEPROM		 equ	 08000h		; Internes 4 KByte EEprom
EEPROMEND	 equ	 08fffh		; Ende internes EEprom
		endif
		if	_n>=167
XRAM1		 equ	 0e000h		; Internes XRAM 2 KByte
XRAM1END	 equ	 0e7ffh		; Internes XRAM 2 KByte Ende
		endif
		if	_n>=168
XRAM2		 equ	 0c000h		; Internes XRAM 6 KByte
XRAM2END	 equ	 0d7ffh		; Internes XRAM 6 KByte Ende
		endif
                

;----------------------------------------------------------------------------
; Bequemlichkeitsmakros

clr             macro   op              ; Operand auf 0 setzen
                and     op,#0
                endm

bchg            macro   op              ; Bit invertieren
                bmovn   op,op
                endm

dec             macro   op              ; um eins runter
                sub     op,#1
                endm

inc             macro   op              ; um eins rauf
                add     op,#1
                endm

swapb           macro   op              ; Bytes tauschen
                ror     op,#8
                endm

;----------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben
