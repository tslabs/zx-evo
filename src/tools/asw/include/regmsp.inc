                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGMSP.INC                                             *
;*   					 			            *
;*   Sinn : enthÑlt Makro- und Registerdefinitionen fÅr den MSP430          *
;* 									    *
;*   letzte énderungen : 2002-01-11                                         *
;*                                                                          *
;****************************************************************************
;* $Id: regmsp.inc,v 1.3 2008/01/02 22:32:21 alfred Exp $                    
;****************************************************************************
;* $Log: regmsp.inc,v $
;* Revision 1.3  2008/01/02 22:32:21  alfred
;* - better heap checking for DOS target
;*
;* Revision 1.2  2007/12/31 13:23:53  alfred
;* - added some bit definitions
;*
;****************************************************************************

                ifndef  regmspinc      ; verhindert Mehrfacheinbindung

regmspinc       equ     1

                if      (MOMCPUNAME<>"MSP430")
                 fatal  "Falscher Prozessortyp eingestellt: nur MSP430 erlaubt!"
		endif

                if      MOMPASS=1
                 message "MSP430-Register+Befehlsdefinitionen (C) 1996/2007 Alfred Arnold, Jose Da Silva"
		endif

;----------------------------------------------------------------------------
; Arithmetik

adc             macro   op
                addc.attribute #0,op
                endm

dadc            macro   op
                dadd.attribute #0,op
                endm

dec             macro   op
                sub.attribute #1,op
                endm

decd            macro   op
                sub.attribute #2,op
                endm

inc             macro   op
                add.attribute #1,op
                endm

incd            macro   op
                add.attribute #2,op
                endm

sbc             macro   op
                subc.attribute #0,op
                endm

;----------------------------------------------------------------------------
; Logik

inv             macro   op
                xor.attribute #-1,op
                endm

rla             macro   op
                add.attribute op,op
                endm

rlc             macro   op
                addc.attribute op,op
                endm

;----------------------------------------------------------------------------
; Daten bewegen ;-)

clr             macro   op
                mov.attribute #0,op
                endm

clrc            macro
                bic     #1,sr
                endm

clrn            macro
                bic     #4,sr
                endm

clrz            macro
                bic     #2,sr
                endm

pop             macro   op
                mov     @sp+,op
                endm

setc            macro
                bis     #1,sr
                endm

setn            macro
                bis     #4,sr
                endm

setz            macro
                bis     #2,sr
                endm

tst             macro   op
                cmp.attribute #0,op
                endm

;----------------------------------------------------------------------------
; Sprungbefehle

br              macro   op
                mov     op,pc
                endm

dint            macro
                bic     #8,sr
                endm

eint            macro
                bis     #8,sr
                endm

nop             macro
                .word   04303h         ; den symbolischen Befehl wÅrde AS zurÅckweisen
                endm

ret             macro
                mov     @sp+,pc
                endm

jlo		macro	label
		jnc	label
		endm

jhs		macro	label
		jc	label
		endm

jeq		macro	label
		jz	label
		endm

;----------------------------------------------------------------------------
; General Memory Layout
; ----------------------
;
; 0000 - 000f : Special Function Registers
; 0010 - 00ff : 8bit Peripheral Modules
; 0100 - 01ff : 16bit Peripheral Modules
; 0200 - .... : RAM Memory
; .... - ffdf : Flash Memory
; ffe0 - ffff : Interrupt Vector Table
;
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
;Status Register

SCG1            equ     128             ; System Clock Generator 1. 1=turn_off_SMCLK
SCG0            equ     64              ; System Clock Generator 0. 1=turn_off_DCO
OSCOFF          equ     32              ; Oscillator Off. 1=turn_off_LFXT1CLK
CPUOFF          equ     16              ; CPU Off. 1=turn_off_CPU        (SR)
GIE             equ     8               ; General Interrupt Enable       (SR)

;----------------------------------------------------------------------------
; Special Function Register of MSP430x1xx Family, Byte Access

IE1             equ     000h            ; Interrupt Enable
ACCVIE          equ     32              ; flash-access interrupt enable  (IE1.5)
NMIIE           equ     16              ; NMI enable                     (IE1.4)
OFIE            equ     2               ; Osc-fault-interrupt enable     (IE1.1)
WDTIE           equ     1               ; Watchdog interrupt enable      (IE1.0)

IFG1            equ     002h            ; Interrupt Flag
NMIIFG          equ     16              ; set via !RST/NMI pin           (IFG1.4)
RSTIFG          equ     8               ; External reset interrupt flag  (IFG1.3)
PORIFG          equ     4               ; Power-on-reset interrupt flag  (IFG1.2)
OFIFG           equ     2               ; flag on oscillator fault       (IFG1.1)
WDTIFG          equ     1               ; watchdog or security key violation (IFG1.0)

ME1             equ     004h            ; Modul Enable

;MSP43012xx devices only, only for MSP43012xx devices.

IE2             equ     001h
UTXIE0          equ     2               ; USART0 transmit int-enable bit (IE2.2)
URXIE0          equ     1               ; USART0 receive int-enable bit  (IE2.1)

IFG2            equ     003h
UTXIFG0         equ     2               ; USART0 and SPI transmit flag   (IFG2.1)
URXIFG0         equ     1               ; USART0 and SPI receive flag    (IFG2.0)

ME2             equ     005h
UTXE0           equ     2               ; USART0 transmit enable bit     (ME2.1)
URXE0           equ     1               ; USART0 receive enable bit      (ME2.0)
USPIE0          equ     1               ; SPI transmit+receive enable    (ME2.0)

;----------------------------------------------------------------------------
; Digital I/O, Byte Access

P0IN            equ     010h            ; Leseregister (Pinzustand)
P0OUT           equ     011h            ; Schreibregister (Latches)
P0DIR           equ     012h            ; Richtungsregister
P0IFG           equ     013h            ; Interrupt-Flags
P0IES           equ     014h            ; Interrupf-Flankenwahl
P0IE            equ     015h            ; Interrupt-Freigaben

P3IN		equ	018h		; Input Register
P3OUT		equ	019h		; Output Register
P3DIR		equ	01Ah		; Direction Register
P3SEL		equ	01Bh		; Function select

P4IN		equ	01Ch		; Input Register
P4OUT		equ	01Dh		; Output Register
P4DIR		equ	01Eh		; Direction Register
P4SEL		equ	01Fh		; Function select

P1IN		equ	020h		; Input Register
P1OUT		equ	021h		; Output Register
P1DIR		equ	022h		; Direction Register
P1IFG		equ	023h		; Interrupt Flags
P1IES		equ	024h		; Interrupt Edge select
P1IE		equ	025h		; Interrupt enable
P1SEL		equ	026h		; Function select

P2IN		equ	028h		; Input Register
P2OUT		equ	029h		; Output Register
P2DIR		equ	02Ah		; Direction Register
P2IFG		equ	02Bh		; Interrupt Flags
P2IES		equ	02Ch		; Interrupt Edge select
P2IE		equ	02Dh		; Interrupt enable
P2SEL		equ	02Eh		; Function select

P5IN		equ	030h		; Input Register
P5OUT		equ	031h		; Output Register
P5DIR		equ	032h		; Direction Register
P5SEL		equ	033h		; Function select

P6IN		equ	034h		; Input Register
P6OUT		equ	035h		; Output Register
P6DIR		equ	036h		; Direction Register
P6SEL		equ	037h		; Function select

;----------------------------------------------------------------------------
; LCD-Interface

LCDCTL          equ     030h            ; Steuerung
LCD_Start       equ     031h            ; Startadresse
LCD_Stop        equ     03fh            ; Endadresse
__TMP           set     1               ; Einzeldefinitionen
                rept    9
LCD{"\{__TMP}"} equ     030h+__TMP
__TMP           set     __TMP+1
                endm
                rept    6
LCD1{"\{__TMP-10}"} equ 030h+__TMP
__TMP           set     __TMP+1
                endm
;----------------------------------------------------------------------------
; Timer

BTCTL           equ     040h            ; Basis-Steuerregister Timer 1

TCCTL           equ     042h
TCPLD           equ     043h            ; Vorladewert
TCDAT           equ     044h            ; ZÑhlwert

BTCNT1          equ     046h            ; ZÑhlregister
BTCNT2          equ     047h

TPCTL           equ     04Bh            ; Timer/Port Steuerregister
TPCNT1          equ     04Ch            ; ZÑhlregister
TPCNT2          equ     04Dh
TPD             equ     04Eh            ; Datenregister
TPE             equ     04Fh            ; Freigaberegister

;----------------------------------------------------------------------------
; Taktgenerator

SCFI0           equ     050h            ; Integrator
SCFI1           equ     051h
SCFQCTL         equ     052h            ; Multiplikator Quarzfrequenz
CBCTL           equ     053h            ; Puffersteuerung

;----------------------------------------------------------------------------
; EPROM Control Register, Byte Access

EPCTL           equ     054h            ; EPROM-Steuerung

;----------------------------------------------------------------------------
; Basic Clock Registers, Byte Access

DCOCTL		equ	056h
DCO2            equ     128             ;DCO freq select, see RSELx     (DCOCTL.7)
DCO1            equ     64              ;                               (DCOCTL.6)
DCO0            equ     32              ;                               (DCOCTL.5)
MOD4            equ     16              ;Modulator selection            (DCOCTL.4)
MOD3            equ     8               ;                               (DCOCTL.3)
MOD2            equ     4               ;                               (DCOCTL.2)
MOD1            equ     2               ;                               (DCOCTL.1)
MOD0            equ     1               ;                               (DCOCTL.0)

BCSCTL1		equ	057h
XT2OFF          equ     128             ; XT2 off. Turn off XT2 oscil    (BCSCTL1.7)
XTS             equ     64              ; LFXT1 mode. 0=LowFreq,1=HiFreq (BCSCTL1.6)
DIVA1           equ     32              ; Divider for ACLK.              (BCSCTL1.5)
DIVA0           equ     16              ; 00=/1, 01=/2, 10=/4, 11=/8     (BCSCTL1.4)
XT5V            equ     8               ; Unused. Always reset to zero   (BCSCTL1.3)
RSEL2           equ     4               ; Resistor select. internal R    (BCSCTL1.2)
RSEL1           equ     2               ; lowest R=0                     (BCSCTL1.1)
RSEL0           equ     1               ;                                (BCSCTL1.0)
RSEL_7          equ     7               ;                                (BCSCTL1.0-2)
RSEL_6          equ     6               ;                                (BCSCTL1.0-2)
RSEL_5          equ     5               ;                                (BCSCTL1.0-2)
RSEL_4          equ     4               ;                                (BCSCTL1.0-2)
RSEL_3          equ     3               ;                                (BCSCTL1.0-2)
RSEL_2          equ     2               ;                                (BCSCTL1.0-2)
RSEL_1          equ     1               ;                                (BCSCTL1.0-2)
RSEL_0          equ     0               ;                                (BCSCTL1.0-2)

BCSCTL2		equ	058h
SELM_3          equ     128+64          ; Select MCLK. 11=LFXT1CLK       (BCSCTL2.6.7)
SELM_2          equ     128             ; MCLK 10=XT2CLK or LFXT1CLK     (BCSCTL2.6.7)
SELM_1          equ     64              ; Select MCLK. 01=DCOCLK         (BCSCTL2.6.7)
SELM_0          equ     0               ; Select MCLK. 00=DCOCLK         (BCSCTL2.6.7)
SELM1           equ     128             ; Select MCLK. 00=01=DCOCLK      (BCSCTL2.7)
SELM0           equ     64              ; 10=XT2CLK or LFXT1CLK=11       (BCSCTL2.6)
DIVM1           equ     32              ; Divider for MCLK,              (BCSCTL2.5)
DIVM0           equ     16              ; 00=/1, 01=/2, 10=/4, 11=/8     (BCSCTL2.4)
SELS            equ     8               ; Select SMCLK 0=DCOCLK,1=XT2CLK (BCSCTL2.3)
DIVS1           equ     4               ; Divider for SMCLK,             (BCSCTL2.2)
DIVS0           equ     2               ; 00=/1, 01=/2, 10=/4, 11=/8     (BCSCTL2.1)
DCOR            equ     1               ; DCO resistor. 0=intern,1=extn  (BCSCTL2.0)

;----------------------------------------------------------------------------
; Comparator_A Registers, Byte Access

CACTL1		equ	059h		; Comparator A control register 1
CACTL2		equ	05Ah		; Comparator A control register 2
CAPD		equ	05Bh		; Comparator A port disable

;----------------------------------------------------------------------------
; PWM

PWMCTL          equ     058h            ; ZÑhlwert
PWMDTB          equ     059h            ; Pulsweite (Puffer)
PWMDTR          equ     05Ah            ; Pulsweite
PWMCNT          equ     05Bh            ; Steuerung

;----------------------------------------------------------------------------
; USART 0

U0CTL		equ	070h
U0TCTL		equ	071h
U0RCTL		equ	072h
U0MCTL		equ	073h
U0BR0		equ	074h
U0BR1		equ	075h
U0RXBUF		equ	076h
U0TXBUF		equ	077h

;----------------------------------------------------------------------------
; USART 1

U1CTL		equ	078h
U1TCTL		equ	079h
U1RCTL		equ	07Ah
U1MCTL		equ	07Bh
U1BR0		equ	07Ch
U1BR1		equ	07Dh
U1RXBUF		equ	07Eh
U1TXBUF		equ	07Fh

;----------------------------------------------------------------------------
; USART Register Bits

FE              equ     128             ; Framing error (low stop bit)   (UxRCTL.7)
PE              equ     64              ; Parity error (PE=0 if PENA=0)  (UxRCTL.6)
OE              equ     32              ; Overrun error (buffer overrun) (UxRCTL.5)
BRK             equ     16              ; Break detect flag              (UxRCTL.4)
URXEIE          equ     8               ; Rec err chars sets URXIFG)     (UxRCTL.3)
URXWIE          equ     4               ; Rec wakeup int-enable (URXIFG) (UxRCTL.2)
RXWAKE          equ     2               ; Rec wakeup flag                (UxRCTL.1)
RXERR           equ     1               ; Rec error flag (FE,PE,OE,BRK)  (UxRCTL.0)
                                          
CKPL            equ     64              ; Clock polarity 0=UCLKI=UCLK    (UxTCTL.6)
SSEL1           equ     32              ; Source 00=UCLKI, 01=ACLK       (UxTCTL.5)
SSEL0           equ     16              ; Source 10=SMCLKI, 11=SMCLK     (UxTCTL.4)
URXSE           equ     8               ; Receive start-edge, 1=enabled  (UxTCTL.3)
TXWAKE          equ     4               ; Transmitter wake, 0=data,1=adr (UxTCTL.2)
TXEPT           equ     1               ; Transmitter empty flag         (UxTCTL.0)
                                          
PENA            equ     128             ; Parity enable, 1=enabled       (UxCTL.7)
PEV             equ     64              ; Parity select, 1=even,0=odd    (UxCTL.6)
SPB             equ     32              ; Stop bit, 0=1stop,1=2stop      (UxCTL.5)
CHAR            equ     16              ; Char length, 0=7bit,1=8bit     (UxCTL.4)
LISTEN          equ     8               ; Listen enable, 1=loopback->RX  (UxCTL.3)
SYNC            equ     4               ; synch mode, 0=USART,1=SPI      (UxCTL.2)
MM              equ     2               ; Multiprocessor, 1=use_protocol (UxCTL.1)
SWRST           equ     1               ; software reset, 1=held_reset   (UxCTL.0)

;----------------------------------------------------------------------------
; ADC12 low bytes

ADC12MCTL0	equ	080h
ADC12MCTL1	equ	081h
ADC12MCTL2	equ	082h
ADC12MCTL3	equ	083h
ADC12MCTL4	equ	084h
ADC12MCTL5	equ	085h
ADC12MCTL6	equ	086h
ADC12MCTL7	equ	087h
ADC12MCTL8	equ	088h
ADC12MCTL9	equ	089h
ADC12MCTL10	equ	08Ah
ADC12MCTL11	equ	08Bh
ADC12MCTL12	equ	08Ch
ADC12MCTL13	equ	08Dh
ADC12MCTL14	equ	08Eh
ADC12MCTL15	equ	08Fh

;----------------------------------------------------------------------------
; LCD Registers

LCDC		equ	090h
__TMP           set     1               ; Einzeldefinitionen
                rept    19
LCDmemory{"\{__TMP}"} equ     LCDC+__TMP
__TMP           set     __TMP+1
                endm

;----------------------------------------------------------------------------
; A/D-Wandler, Word Access

AIN             equ     0110h           ; Eingaberegister
AEN             equ     0112h           ; Eingabefreigaben
ACTL            equ     0114h           ; Steuerung
ADAT            equ     0118h           ; Daten

;----------------------------------------------------------------------------
; Timer_B Interrupt Vector, Word Access

TBIV		equ	011Eh

;----------------------------------------------------------------------------
; Watchdog/Timer, Word Access

WDTCTL          equ     0120h
WDTHOLD         equ     128             ; Watchdog timer hold. 1=stopped (WDTCTL.6)
WDTNMIES        equ     64              ; NMI edge select 0=rise,1=fall  (WDTCTL.6)
WDTNMI          equ     32              ; NMI pin select, 0=!reset,1=NMI (WDTCTL.5)
WDTTMSEL        equ     16              ; mode select 0=watchdog,1=timer (WDTCTL.4)
WDTCNTCL        equ     8               ; Counter clear, 1=clear_counter (WDTCTL.3)
WDTSSEL         equ     4               ; Source select, 0=SMCLK,1=ACLK  (WDTCTL.2)
WDTIS1          equ     2               ; Watchdog timer interval select (WDTCTL.1)
WDTIS0          equ     1               ; 00=32768,01=8192,10=512,11=64  (WDTCTL.0)

;----------------------------------------------------------------------------
; Timer_A Interrupt Vector, Word Access

TAIV		equ	012Eh

;----------------------------------------------------------------------------
; Flash Control, Word Access

FCTL1		equ	0128h
FCTL2		equ	012Ah
FCTL3		equ	012Ch

;----------------------------------------------------------------------------
; Hardware Multiplier, Word Access

MPY		equ	0130h		; Multiply unsigned
MPYS		equ	0132h		; Multiply signed
MAC		equ	0134h		; MPY+ACC
MACS		equ	0136h		; MPYS+ACC
OP2		equ	0138h		; Second Operand
ResLo		equ	013Ah		; Result low word
ResHi		equ	013Ch		; Result high word
SumExt		equ	013Eh		; Sum extend

;----------------------------------------------------------------------------
; ADC12 high bytes, Word Access

ADC12MEM0	equ	0140h
ADC12MEM1	equ	0142h
ADC12MEM2	equ	0144h
ADC12MEM3	equ	0146h
ADC12MEM4	equ	0148h
ADC12MEM5	equ	014Ah
ADC12MEM6	equ	014Ch
ADC12MEM7	equ	014Eh
ADC12MEM8	equ	0150h
ADC12MEM9	equ	0152h
ADC12MEM10	equ	0154h
ADC12MEM11	equ	0156h
ADC12MEM12	equ	0158h
ADC12MEM13	equ	015Ah
ADC12MEM14	equ	015Ch
ADC12MEM15	equ	015Eh

;----------------------------------------------------------------------------
; Timer_A Registers, Word Access

TACTL		equ	0160h
CCTL0		equ	0162h
CCTL1		equ	0164h
CCTL2		equ	0166h
CCTL3		equ	0168h
CCTL4		equ	016Ah
TAR		equ	0170h
CCR0		equ	0172h
CCR1		equ	0174h
CCR2		equ	0176h
CCR3		equ	0178h
CCR4		equ	017Ah

;----------------------------------------------------------------------------
; Timer_B Registers, Word Access

TBCTL		equ	0180h
TBCCTL0		equ	0182h
TBCCTL1		equ	0184h
TBCCTL2		equ	0186h
TBCCTL3		equ	0188h
TBCCTL4		equ	018Ah
TBCCTL5		equ	018Ch
TBCCTL6		equ	018Eh
TBR		equ	0190h
TBCCR0		equ	0192h
TBCCR1		equ	0194h
TBCCR2		equ	0196h
TBCCR3		equ	0198h
TBCCR4		equ	019Ah
TBCCR5		equ	019Ch
TBCCR6		equ	019Eh

;----------------------------------------------------------------------------
; ADC12 Registers, Byte and Word Access

ADC12CTL0	equ	01A0h
ADC12CTL1	equ	01A2h
ADC12FG		equ	01A4h
ADC12IE		equ	01A6h

;----------------------------------------------------------------------------

                endif                   ; von IFDEF...
		restore                 ; wieder erlauben
