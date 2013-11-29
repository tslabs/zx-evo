                save
                listing off   ; kein Listing ueber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGAVR.INC                                             *
;*                                                                          *
;*   Sinn : enth„lt SFR- und Bitdefinitionen fuer die AVR-Prozessoren       *
;*                                                                          *
;*   letzte Žnderungen :  6. 7.1996                                         *
;*                        8. 6.1997 Anpassung an die endgueltigen Versionen *
;*                       19.11.1999 Konstantenmodus fuer AVR jetzt HEX      *
;*                       26. 2.2000 MomCPU-->MOMCPU fuer case-sensitiven    *
;*                                  Modus                                   *
;*                                                                          *
;****************************************************************************
;* $Id: regavr.inc,v 1.1 2003/11/06 02:56:33 alfred Exp $                   *
;****************************************************************************
;* $Log: regavr.inc,v $
;* Revision 1.1  2003/11/06 02:56:33  alfred
;* - recreated
;*
;* Revision 1.4  2003/07/21 18:05:14  alfred
;* - fixed ADCH value; removed umlauts
;*
;* Revision 1.3  2002/05/09 12:51:37  alfred
;* - added MEGA16 regs
;*
;* Revision 1.2  2002/05/01 15:58:26  alfred
;* - added MEGA8 regs
;*
;****************************************************************************

                ifndef  __regavrinc     ; avoid multiple inclusion
__regavrinc	equ     1

; set internal CPU code and memory addresses in one step

		switch	MOMCPUNAME
		case	"AT90S1200"
__cpucode	  equ	  0x011200
eeend             equ     63		; end address EEPROM
iramend           equ     95,data	; end address SRAM
iromend           label   1023		; end address EPROM
		case	"AT90S2313"
__cpucode	  equ	  0x012313
eeend             equ     127
iramend           equ     0xdf,data
iromend           label   2047
		case	"AT90S4414"
__cpucode	  equ	  0x014414
eeend             equ     255
iramend           equ     0x15f,data
iromend           label   4095
		case	"AT90S8515"
__cpucode	  equ	  0x018515
eeend             equ     511
iramend           equ     0x25f,data
iromend           label   8191
		case	"ATMEGA8"
__cpucode	  equ	  0x020008
eeend             equ     511
iramend           equ     0x45f,data
iromend           label   8191
		case	"ATMEGA16"
__cpucode	  equ	  0x020010
eeend             equ     511
iramend           equ     0x45f,data
iromend           label   16383
		elsecase
                  fatal  "Falscher Prozessortyp eingestellt: nur AT90S1200, AT90S2313, AT90S4414, AT90S8515, ATMEGA8 oder ATMEGA16 erlaubt!"
                endcase

ismega		function code,(code>>16)=2

                if      MOMPASS=1
                 message "Atmel-AVR-SFR-Definitionen (C) 1996,2002 Alfred Arnold"
                endif

;----------------------------------------------------------------------------
; constant memory addresses

eestart         equ     0               ; start address internal EEPROM
iram            equ     96,data         ; start address internal SRAM
                                        ; (behind mapped I/O)
irom            label   0               ; start address internal EPROM

;----------------------------------------------------------------------------
; Prozessorkern

sreg            port    0x3f            ; Statusregister:
c               equ     0               ;  Carry
z               equ     1               ;  Ergebnis Null
n               equ     2               ;  Ergebnis negativ
v               equ     3               ;  Zweierkomplement-Ueberlauf
s               equ     4               ;  Vorzeichen
h               equ     5               ;  Halfcarry
t               equ     6               ;  Bitspeicher
i               equ     7               ;  globale Interruptsperre

; size of stack pointer depends on size of internal data space
; (if present at all)

                if      __cpucode>=0x012313
spl              equ     0x3d           ; Stapelzeiger (LSB)
                 if      iramend>=256
sph               equ     0x3e          ;              (MSB)
                 endif
                endif

;----------------------------------------------------------------------------
; Chip-Konfiguration

mcucr           port    0x35            ; CPU-Steuerung:
isc00           equ     0               ;  Flankenwahl INT0
isc01           equ     1               ;  Flanken/Pegeltriggerung INT0
                if      __cpucode>=0x012313
isc10             equ     2             ;  Flankenwahl INT1
isc11             equ     3             ;  Flanken/Pegeltriggerung INT1
                endif
		if 	ismega(__cpucode)
; note: for the Mega16, bits 6 & 7 are interchanged, I guess that's an
; error in the data sheet...
se		  equ     7		; sleep enable
sm2		  equ	  6		; sleep mode select
sm1		  equ	  5
sm0		  equ	  4
		else ; !ismega
sm                equ     4             ;  Idle/Powerdown-Modus waehlen
se                equ     5             ;  Sleep-Modus freigeben
                  if      __cpucode>=0x014414
srw                 equ     6           ;  Wait-State-Wahl externes SRAM
sre                 equ     7           ;  Freigabe externes SRAM
                  endif
		endif

		if	ismega(__cpucode)
osccal		  port	  0x31		; oscillator calibration

mcucsr		  port	  0x34
                  if	  __cpucode>=0x020010
jtd		    equ	    7		; JTAG reset flag
isc2		    equ	    6		; interrupt sense control 2
		  endif
wdrf		  equ	  3		; watchdog reset occured
borf		  equ	  2		; brown-out occured
extrf		  equ	  1		; external reset occured
porf		  equ	  0		; power-on reset occured

spmcr		  port	  0x37		; store program memory ctrl register
spmie		  equ	  7		; interrupt enable
rwwsb		  equ	  6		; read-while-write section busy
rwwsre		  equ	  4		; read-while-write section rd enable
blbset		  equ	  3		; boot lock bit set
pgwrt		  equ	  2		; page write
pgers		  equ	  1		; page erase
spmen		  equ	  0		; store program memory enable
		endif

;----------------------------------------------------------------------------
; Interrupt-Steuerung

		if 	ismega(__cpucode)
gicr		 port	 0x3b
ivce		 equ	 0		; interrupt vector change enable
ivsel		 equ	 1		; interrupt vector select
int0		 equ	 6		; enable x'tal interrupt 0
int1		 equ	 7		; enable x'tal interrupt 1
                 if	 __cpucode>=0x020010
int2		  equ	  5		; x'tal interrupt 2
		 endif
		elseif
gimsk            port    0x3b           ; generelle Interrupt-Maske:
int0             equ     6              ;  externer Interrupt 0
                 if      __cpucode>=0x012313
int1              equ     7             ;  externer Interrupt 1
                 endif
		endif

                if      __cpucode>=0x012313
gifr		 port	 0x3a           ; generelle Interrupt-Flags:
intf0		 equ	 6		;  externer Interrupt 0
intf1            equ     7              ;  externer Interrupt 1
                 ifdef	 int2
intf2		  equ	  int2		; x'tal interrupt 2
		 endif
                endif

; who the heck decided to rearrange all bits for the Megas?

timsk           port    0x39            ; Timer-Interrupt-Maske:
                if      ismega(__cpucode)
toie0            equ     0
                elseif
toie0            equ     1              ;  šberlauf Timer 0
                endif
                if      __cpucode>=0x012313
                 if      __cpucode>=0x014414
                  if 	  ismega(__cpucode)
ocie1b             equ     3            ;  Vergleich B Timer 1
                  elseif
ocie1b             equ     5            ;  Vergleich B Timer 1
                  endif
                 endif
                 if	  ismega(__cpucode)
toie1		  equ	  2
ticie1		  equ	  5
toie2		  equ	  6
ocie2		  equ	  7
ocie1a            equ     4
                 elseif
toie1             equ     7             ;  Ueberlauf Timer 1
ticie1            equ     3             ;  Fang Timer 1
ocie1a            equ     6             ;  Vergleich Timer 1
		 endif
                endif

tifr            port    0x38            ; Timer-Interrupt-Flags:
tov0            equ     toie0		;  Ueberlauf Timer 0
                if      __cpucode>=0x012313
ocf1a            equ     ocie1a         ;  Vergleich A Timer 1
tov1             equ     toie1          ;  Ueberlauf Timer 1
icf1             equ     ticie1         ;  Fang Timer 1
		endif
                if      __cpucode>=0x014414
ocf1b            equ     ocie1b         ;  Vergleich B Timer 1
                endif
		if 	ismega(__cpucode)
tov2		 equ	 toie2
ocf2		 equ	 ocie2
		endif


;----------------------------------------------------------------------------
; parallele Ports

		if      ismega(__cpucode)
sfior		 equ	 0x30
psr10		 equ	 0		; prescaler reset T0/T1
psr2		 equ	 1		; ditto T2
pud		 equ	 2		; pullup disable
acme		 equ	 3		; analog comparator multiplexer enable
adhsm		 equ	 4		; ADC high speed mode
		 if	 __cpucode>=0x020010
adts0		  equ	  5
adts1		  equ	  6
adts2		  equ	  7
		 endif
		endif

                if      (__cpucode=0x014414)||(__cpucode=0x018515)||(__cpucode=0x020010)
porta            port    0x1b           ; Datenregister Port A
ddra             port    0x1a           ; Datenrichtungsregister Port A
pina             port    0x19           ; Leseregister Port A
                endif

portb           port    0x18            ; Datenregister Port B
ddrb            port    0x17            ; Datenrichtungsregister Port B
pinb            port    0x16            ; Leseregister Port B

                if      __cpucode>=0x014414
portc            port    0x15           ; Datenregister Port C
ddrc             port    0x14           ; Datenrichtungsregister Port C
pinc             port    0x13           ; Leseregister Port C
                endif

portd           port    0x12            ; Datenregister Port D
ddrd            port    0x11            ; Datenrichtungsregister Port D
pind            port    0x10            ; Leseregister Port D

;----------------------------------------------------------------------------
; Timer

tccr0           port    0x33            ; Steuerregister Timer 0:
cs00            equ     0               ;  Vorteilereinstellung
cs01            equ     1
cs02            equ     2
		if	__cpucode=0x020010
wgm01		 equ	 3		; waveform generation mode
wgm00		 equ	 6
com00		 equ	 4		; compare/match output mode
com01		 equ	 5
foc0		 equ	 7		; force output compare
		endif

tcnt0           port    0x32            ; Z„hlregister Timer 0

ocr0		port	0x3c

                if      __cpucode>=0x012313
tccr1a           port    0x2f           ; Steuerregister A Timer 1:
pwm10            equ     0              ;  Modus Pulsweitenmodulator
wgm10		 equ	 pwm10
pwm11            equ     1
wgm11		 equ	 pwm11
com1a0           equ     6              ;  Vergleichsmodus A
com1a1           equ     7
                 if      __cpucode>=0x014414
com1b0            equ     4             ;  Vergleichsmodus B
com1b1            equ     5
                 endif
		 if 	 ismega(__cpucode)
foc1b		  equ	  2
foc1a		  equ	  3
		 endif

tccr1b           port    0x2e           ; Steuerregister B Timer 1:
cs10             equ     0              ; Vorteilereinstellung
cs11             equ     1
cs12             equ     2
ctc1             equ     3              ; nach Gleichheit zuruecksetzen ?
wgm12		 equ	 ctc1
		 if      ismega(__cpucode)
wgm13		  equ	  4
		 endif
ices1            equ     6              ; Flankenwahl Fang
icnc1            equ     7              ; Rauschfilter fuer Fangfunktion

tcnt1l           port    0x2c           ; Zaehlregister Timer 1 (LSB)
tcnt1h           port    0x2d           ;                       (MSB)

                 if      __cpucode>=0x014414
ocr1al            port    0x2a          ; Vergleichsregister A Timer 1 (LSB)
ocr1ah            port    0x2b          ;                              (MSB)
ocr1bl            port    0x28          ; Vergleichsregister B Timer 1 (LSB)
ocr1bh            port    0x29          ;                              (MSB)
                 elseif
ocr1l             port    0x2a          ; Vergleichsregister Timer 1 (LSB)
ocr1h             port    0x2b          ;                            (MSB)
                 endif

		 if	 ismega(__cpucode)
icr1l             port    0x26
icr1h             port    0x27
		 elseif
icr1l             port    0x24          ; Fangwert Timer 1 (LSB)
icr1h             port    0x25          ;                  (MSB)
		 endif
                endif

		if 	ismega(__cpucode)
tccr2		 port	 0x26		; control register timer 2
cs20		 equ	 0		; prescaler
cs21		 equ	 1
cs22		 equ	 2
wgm21		 equ	 3
com20		 equ	 4
com21		 equ	 5
wgm20		 equ	 6
foc2		 equ	 7

tcnt2		 port	 0x24		; timer value
ocr2		 port	 0x23		; output compare value

assr		 port	 0x22		; asynchronous status register
as2		 equ	 3		; asynchronous timer 2
tcn2ub		 equ	 2		; timer/counter 2 update busy
ocr2ub		 equ	 1		; output compare register 2 update busy
tcr2ub		 equ	 0		; timer/counter ctrl register 2upd busy
		endif

;----------------------------------------------------------------------------
; Watchdog

wdtcr           port    0x21            ; Watchdog-Steuerregister:
wdp0            equ     0               ;  Vorteiler
wdp1            equ     1
wdp2            equ     2
wde             equ     3               ; Freigabe
		if	__cpucode=0x010008
wdce		  equ	  4		; watchdog change enable
                elseif 	__cpucode>=0x012313
wdttoe		  equ	  4		; zur Sperre gebraucht
                endif

;----------------------------------------------------------------------------
; serielle Ports

                if      __cpucode>=0x012312
udr              port    0x0c           ; Datenregister UART

usr              port    0x0b           ; Statusregister UART:
ucsra		 port	 usr
		 if	 ismega(__cpucode)
mpcm		  equ	  0		; multiprocessor communication mode
u2x		  equ	  1		; double speed
pe		  equ	  2		; parity error
		 endif
or               equ     3              ;  Empfaengerueberlauf
dor		 equ	 or
fe               equ     4              ;  Framing-Fehler
udre             equ     5              ;  Datenregister wieder frei
txc              equ     6              ;  Sendung komplett
rxc              equ     7              ;  Empfang komplett

ucr              port    0x0a           ; Steuerregister UART:
ucsrb		 port	 ucr
txb8             equ     0              ;  Sendebit 8
rxb8             equ     1              ;  Empfangsbit 8
chr9             equ     2              ;  auf 9-Bit-Datenwerte umschalten
ucsz2		 equ	 chr9
txen             equ     3              ;  Sender freigeben
rxen             equ     4              ;  Empfaenger freigeben
udrie            equ     5              ;  Interrupts bei freiem Datenregister freigeben
txcie            equ     6              ;  Interrupts nach Versand freigeben
rxcie            equ     7              ;  Interrupts nach Empfang freigeben

		 if      ismega(__cpucode)
ucsrc		  port	  0x20		; control register C
ursel		  equ	  7		; register select
umsel		  equ	  6		; sync/asnyc mode
upm1		  equ	  5		; parity mode
upm0		  equ	  4
usbs		  equ	  3		; stop bit select
ucsz1		  equ	  2		; character size
ucsz0		  equ	  1
ucpol		  equ	  0		; clock polarity
		 endif

ubrr             port    0x09           ; Baudratengenerator
		 if	 ismega(__cpucode)
ubrrl		  port	  ubrr
ubrrh		  port	  ucsrc
		 endif
                endif

                if      __cpucode>=0x014414
spcr             port    0x0d           ; SPI Steuerregister:
spr0             equ     0              ;  Wahl Taktfrequenz
spr1             equ     1
cpha             equ     2              ;  Taktphase
cpol             equ     3              ;  Taktpolaritaet
mstr             equ     4              ;  Master/Slave-Wahl
dord             equ     5              ;  Bitreihenfolge
spe              equ     6              ;  SPI freigeben
spie             equ     7              ;  Interruptfreigabe SPI

spsr             port    0x0e           ; SPI Statusregister:
		 if	 ismega(__cpucode)
spi2x		  equ	  0		; double speed mode
		 endif
wcol             equ     6              ;  Schreibkollision ?
spif             equ     7              ;  SPI-Interrupt aufgetreten ?

spdr             port    0x0f           ; SPI Datenregister
                endif

		if	ismega(__cpucode)
twbr		 port	 0x00		; bit rate register

twcr		 port	 0x36		; control register
twint		 equ	 7		; interrupt flag
twea		 equ	 6		; enable acknowledge bit
twsta		 equ	 5		; start condition
twsto		 equ	 4		; stop condition
twwc		 equ	 3		; write collision flag
twen		 equ	 2		; enable bit
twie		 equ	 0		; interupt enable

twsr		 port	 0x01		; status register
tws7		 equ	 7		; status
tws6		 equ	 6
tws5		 equ	 5
tws4		 equ	 4
tws3		 equ	 3
twps1		 equ	 1		; prescaler
twps0		 equ	 0

twdr		 port	 0x03		; data register

twar		 port	 0x02		; (slave) address register
twgce		 equ	 0		; general call recognition bit
		endif

;----------------------------------------------------------------------------
; Analogkomparator

acsr            port    0x08            ; Komparator-Steuer/Statusregister:
acis0           equ     0               ;  Interrupt-Modus
acis1           equ     1
		if	__cpucode>=0x012313
acic             equ     2              ;  Komparator als Fangsignal fuer Timer 1 benutzen
		endif
acie            equ     3               ;  Interrupt freigeben
aci             equ     4               ;  Interrupt aufgetreten ?
aco             equ     5               ;  Komparatorausgang
		if      ismega(__cpucode)
acbg		 equ	 6		;  bandgap select
		endif
acd		equ	7		;  Strom abschalten

;----------------------------------------------------------------------------
; A/D converter

		if      ismega(__cpucode)
admux		 port	 0x07		; multiplexer selection
refs1		 equ	 7		; reference selection bits
refs0		 equ	 6
adlar		 equ	 5		; left adjust right
mux3		 equ	 3		; multiplexer
mux2		 equ	 2
mux1		 equ	 1
mux0		 equ	 0

		 if 	 __cpucode >=0x020010
adcsra		  port	  0x06		; control/status register
adate		  equ	  5		; auto trigger enable
		 elseif
adcsr		  port	  0x06		; control/status register
adfr		  equ	  5		; free running select
		  endif
aden		 equ	 7		; enable ADC
adsc		 equ	 6		; start conversion
adif		 equ	 4		; interrupt flag
adie		 equ	 3		; interrupt enable
adps2		 equ	 2		; prescaler select
adps1		 equ	 1
adps0		 equ	 0

adch		 port	 0x05		; data register
adcl		 port	 0x04
		endif

;----------------------------------------------------------------------------
; JTAG

		if	__cpucode >=0x020010
ocdr		 port	 osccal		; debug register
		endif

;----------------------------------------------------------------------------
; EEPROM

		if eeend>=256
eearl		 port	 0x1e		; Adressregister
eearh		 port	 0x1f
		elseif 
eear             port    0x1e
		endif

eedr            port    0x1d            ; Datenregister

eecr            port    0x1c            ; Steuerregister:
eere            equ     0               ;  Lesefreigabe
eewe            equ     1               ;  Schreibfreigabe
		if	__cpucode>=0x012313
eemwe		 equ	 2
		endif
		if 	__cpucode=0x020008
eerie		 equ	 3		; enable ready interrupt
		endif

;----------------------------------------------------------------------------
; Vektoren
; Leider verschieben sich Vektoren bei den hoeheren Prozessoren.
; Warum nur, Atmel, warum ?

vec_reset       label   0               ; Reset-Einsprung
                switch  __cpucode
                case    0x011200
vec_int0         label   1              ; Einsprung ext. Interrupt 0
vec_tm0ovf       label   2              ; Einsprung Ueberlauf Timer 0
vec_anacomp      label   3              ; Einsprung Analog-Komparator
                case    0x012313
vec_int0         label   1              ; Einsprung ext. Interrupt 0
vec_int1         label   2              ; Einsprung ext. Interrupt 2
vec_tm1capt      label   3              ; Einsprung Fang Timer 1
vec_tm1comp      label   4              ; Einsprung Vergleich Timer 1
vec_tm1ovf       label   5              ; Einsprung Ueberlauf Timer 1
vec_tm0ovf       label   6              ; Einsprung Ueberlauf Timer 0
vec_uartrx       label   7              ; Einsprung UART Empfang komplett
vec_uartudre     label   8              ; Einsprung UART Datenregister leer
vec_uarttx       label   9              ; Einsprung UART Sendung komplett
vec_anacomp      label   10             ; Einsprung Analog-Komparator
                case    0x014414,0x018515
vec_int0         label   1              ; Einsprung ext. Interrupt 0
vec_int1         label   2              ; Einsprung ext. Interrupt 2
vec_tm1capt      label   3              ; Einsprung Fang Timer 1
vec_tm1compa     label   4              ; Einsprung Vergleich A Timer 1
vec_tm1compb     label   5              ; Einsprung Vergleich A Timer 1
vec_tm1ovf       label   6              ; Einsprung Ueberlauf Timer 1
vec_tm0ovf       label   7              ; Einsprung Ueberlauf Timer 0
vec_spi          label   8              ; Einsprung SPI-Interrupt
vec_uartrx       label   9              ; Einsprung UART Empfang komplett
vec_uartudre     label   10             ; Einsprung UART Datenregister leer
vec_uarttx       label   11             ; Einsprung UART Sendung komplett
vec_anacomp      label   12             ; Einsprung Analog-Komparator
		case	0x020008
vec_int0         label   1              ; Einsprung ext. Interrupt 0
vec_int1	 label	 2		; x'tal interrupt 1
vec_tm2comp	 label 	 3 		; timer 2 compare match
vec_tm2ovf	 label	 4		; timer 2 overflow
vec_tm1capt	 label	 5		; timer 1 capture
vec_tm1compa	 label	 6		; timer 1 compare match A
vec_tm1compb	 label	 7		; timer 1 compare match B
vec_tm1ovf	 label	 8		; timer 1 overflow
vec_tm0ovf	 label	 9		; timer 0 overflow
vec_spi		 label   10		; SPI transfer complete
vec_uartrx	 label	 11		; UART Rx complete
vec_uartudre	 label   12		; UART data register empty
vec_uarttx	 label	 13		; UART Tx complete
vec_adc		 label	 14		; ADC conversion complete
vec_eerdy	 label	 15		; EEPROM ready
vec_anacomp	 label	 16		; analog comparator
vec_twi		 label	 17		; two-wire interface
vec_spm_rdy	 label	 18		; store program memory ready
		case	0x020010
vec_int0	 label	 2		; x'tal interrupt 0
vec_int1	 label	 4		; x'tal interrupt 1
vec_tm2comp	 label 	 6 		; timer 2 compare match
vec_tm2ovf	 label	 8		; timer 2 overflow
vec_tm1capt	 label	 10		; timer 1 capture
vec_tm1compa	 label	 12		; timer 1 compare match A
vec_tm1compb	 label	 14		; timer 1 compare match B
vec_tm1ovf	 label	 16		; timer 1 overflow
vec_tm0ovf	 label	 18		; timer 0 overflow
vec_spi		 label   20		; SPI transfer complete
vec_uartrx	 label	 22		; UART Rx complete
vec_uartudre	 label   24		; UART data register empty
vec_uarttx	 label	 26		; UART Tx complete
vec_adc		 label	 28		; ADC conversion complete
vec_eerdy	 label	 30		; EEPROM ready
vec_anacomp	 label	 32		; analog comparator
vec_twi		 label	 34		; two-wire interface
vec_int2	 label	 36		; x'tal interrupt 2
vec_tm0comp	 label	 38		; timer 0 compare match
vec_spm_rdy	 label	 40		; store program memory ready
                endcase

;----------------------------------------------------------------------------

                endif			; __regavrinc

                restore                 ; wieder erlauben
