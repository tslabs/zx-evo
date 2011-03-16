                save
                listing off

;*****************************************************************************
;   REGHC12.INC
;   Register Definitions for HC812A4 & HC912B32
;   Source: MC68HC812A4, MC68HC912B32 Technical Summary (Motorola 1996)
;   27.01.1997
;   Oliver Thamm (othamm@aol.com)
;*****************************************************************************

                ifndef  reghc12inc      ; verhindert Mehrfacheinbindung

reghc12inc      equ     1

                if      (MOMCPUNAME<>"68HC12")
                 fatal  "Falscher Prozessortyp eingestellt: nur 68HC12 erlaubt!"
		endif

                if      MOMPASS=1
                 message "68HC812A4/68HC912B32-Registerdefinitionen"
                 message "(C) 1996,1997 Oliver Thamm"
		endif

;-----------------------------------------------------------------------------

REGBASE	equ	$0000		; Below: [A=HC812A4|B=HC912B32]

PORTA	equ	REGBASE+$0000	; [A|B] Port A Register
PORTB	equ	REGBASE+$0001	; [A|B] Port B Register
DDRA	equ	REGBASE+$0002	; [A|B] Port A Data Direction Register
DDRB	equ	REGBASE+$0003	; [A|B] Port B Data Direction Register
PORTC	equ	REGBASE+$0004	; [A|-] Port C Register
PORTD	equ	REGBASE+$0005	; [A|-] Port D Register
DDRC	equ	REGBASE+$0006	; [A|-] Port C Data Direction Register
DDRD	equ	REGBASE+$0007	; [A|-] Port D Data Direction Register
PORTE	equ	REGBASE+$0008	; [A|B] Port E Register
DDRE	equ	REGBASE+$0009	; [A|B] Port E Data Direction Register
PEAR	equ	REGBASE+$000a	; [A|B] Port E Assignment Register
MODE	equ	REGBASE+$000b	; [A|B] Mode Register
PUCR	equ	REGBASE+$000c	; [A|B] Pull Up Control Register
RDRIV	equ	REGBASE+$000d	; [A|B] Reduced Drive of I/O Lines

INITRM	equ	REGBASE+$0010	; [A|B] Initialization of Internal RAM Position Register 
INITRG	equ	REGBASE+$0011	; [A|B] Initialization of Internal Register Position Register
INITEE	equ	REGBASE+$0012	; [A|B] Initialization of Internal EEPROM Position Register
MISC	equ	REGBASE+$0013	; [A|B] Miscellaneous Mapping Register
RTICTL	equ	REGBASE+$0014	; [A|B] Real-Time Interrupt Control Register
RTIFLG	equ	REGBASE+$0015	; [A|B] Real-Time Interrupt Flag Register
COPCTL	equ	REGBASE+$0016	; [A|B] COP Control Register
COPRST	equ	REGBASE+$0017	; [A|B] Arm/Reset COP Timer Register
ITST0	equ	REGBASE+$0018	; [A|B] Reserved
ITST1	equ	REGBASE+$0019	; [A|B] Reserved
ITST2	equ	REGBASE+$001a	; [A|B] Reserved
ITST3	equ	REGBASE+$001b	; [A|B] Reserved

INTCR	equ	REGBASE+$001e	; [A|B] Interrupt Control Register
HPRIO	equ	REGBASE+$001f	; [A|B] Highest Priority I Interrupt
KWIED	equ	REGBASE+$0020	; [A|-] Key Wakeup Port D Interrupt Enable Register
BRKCT0	equ	REGBASE+$0020	; [-|B] Breakpoint Control Register 0
KWIFD	equ	REGBASE+$0021	; [A|-] Key Wakeup Port D Flag Register
BRKCT1	equ	REGBASE+$0021	; [-|B] Breakpoint Control Register 1
BRKAH	equ	REGBASE+$0022	; [-|B] Breakpoint Address Register (High Byte)
BRKAL	equ	REGBASE+$0023	; [-|B] Breakpoint Address Register (Low Byte)
PORTH	equ	REGBASE+$0024	; [A|-] Port H Register
BRKDH	equ	REGBASE+$0024	; [-|B] Breakpoint Data Register (High Byte)
DDRH	equ	REGBASE+$0025	; [A|-] Port H Data Direction Register
BRKDL	equ	REGBASE+$0025	; [-|B] Breakpoint Data Register (Low Byte)
KWIEH	equ	REGBASE+$0026	; [A|-] Key Wakeup Port H Interrupt Enable Register
KWIFH	equ	REGBASE+$0027	; [A|-] Key Wakeup Port H Flag Register
PORTJ	equ	REGBASE+$0028	; [A|-] Port J Register
DDRJ	equ	REGBASE+$0029	; [A|-] Port J Data Direction Register
KWIEJ	equ	REGBASE+$002a	; [A|-] Key Wakeup Port J Interrupt Enable Register
KWIFJ	equ	REGBASE+$002b	; [A|-] Key Wakeup Port J Flag Register
KPOLJ	equ	REGBASE+$002c	; [A|-] Key Wakeup Port J Polarity Register
PUPSJ	equ	REGBASE+$002d	; [A|-] Key Wakeup Port J Pull-Up/Pulldown Select Register
PULEJ	equ	REGBASE+$002e	; [A|-] Key Wakeup Port J Pull-Up/Pulldown Enable Register

PORTF	equ	REGBASE+$0030	; [A|-] Port F Register
PORTG	equ	REGBASE+$0031	; [A|-] Port G Register
DDRF	equ	REGBASE+$0032	; [A|-] Port F Data Direction Register
DDRG	equ	REGBASE+$0033	; [A|-] Port G Data Direction Register
DPAGE	equ	REGBASE+$0034	; [A|-] Data Page Register
PPAGE	equ	REGBASE+$0035	; [A|-] Program Page Register
EPAGE	equ	REGBASE+$0036	; [A|-] Extra Page Register
WINDEF	equ	REGBASE+$0037	; [A|-] Window Definition Register
MXAR	equ	REGBASE+$0038	; [A|-] Memory Expansion Assignment Register

CSCTL0	equ	REGBASE+$003c	; [A|-] Chip Select Control Register 0
CSCTL1	equ	REGBASE+$003d	; [A|-] Chip Select Control Register 1
CSSTR0	equ	REGBASE+$003e	; [A|-] Chip Select Stretch Register 0
CSSTR1	equ	REGBASE+$003f	; [A|-] Chip Select Stretch Register 1
LDV	equ	REGBASE+$0040	; [A|-] Loop Divider Registers (Word)
PWCLK	equ	REGBASE+$0040	; [-|B] PWM Clocks and Concatenate
PWPOL	equ	REGBASE+$0041	; [-|B] PWM Clock Select and Polarity
RDV	equ	REGBASE+$0042	; [A|-] Reference Divider Register (Word)
PWEN	equ	REGBASE+$0042	; [-|B] PWM Enable
PWPRES	equ	REGBASE+$0043	; [-|B] PWM Prescale Counter
PWSCAL0	equ	REGBASE+$0044	; [-|B] PWM Scale Register 0
PWSCNT0	equ	REGBASE+$0045	; [-|B] PWM Scale Counter 0 Value
PWSCAL1	equ	REGBASE+$0046	; [-|B] PWM Scale Register 1
CLKCTL	equ	REGBASE+$0047	; [A|-] Clock Control Register
PWSCNT1	equ	REGBASE+$0047	; [-|B] PWM Scale Counter 1 Value
PWCNT0	equ	REGBASE+$0048	; [-|B] PWM Channel 0 Counter
PWCNT1	equ	REGBASE+$0049	; [-|B] PWM Channel 1 Counter
PWCNT2	equ	REGBASE+$004a	; [-|B] PWM Channel 2 Counter
PWCNT3	equ	REGBASE+$004b	; [-|B] PWM Channel 3 Counter
PWPER0	equ	REGBASE+$004c	; [-|B] PWM Channel 0 Period Register
PWPER1	equ	REGBASE+$004d	; [-|B] PWM Channel 1 Period Register
PWPER2	equ	REGBASE+$004e	; [-|B] PWM Channel 2 Period Register
PWPER3	equ	REGBASE+$004f	; [-|B] PWM Channel 3 Period Register
PWDTY0	equ	REGBASE+$0050	; [-|B] PWM Channel 0 Duty Register
PWDTY1	equ	REGBASE+$0051	; [-|B] PWM Channel 1 Duty Register
PWDTY2	equ	REGBASE+$0052	; [-|B] PWM Channel 2 Duty Register
PWDTY3	equ	REGBASE+$0053	; [-|B] PWM Channel 3 Duty Register
PWCTL	equ	REGBASE+$0054	; [-|B] PWM Control Register
PWTST	equ	REGBASE+$0055	; [-|B] PWM Special Mode Register
PORTP	equ	REGBASE+$0056	; [-|B] Port P Data Register
DDRP	equ	REGBASE+$0057	; [-|B] Port P Data Direction Register

ATDCTL0	equ	REGBASE+$0060	; [A|B] Reserved
ATDCTL1	equ	REGBASE+$0061	; [A|B] Reserved
ATDCTL2	equ	REGBASE+$0062	; [A|B] ATD Control Register 2
ATDCTL3	equ	REGBASE+$0063	; [A|B] ATD Control Register 3
ATDCTL4	equ	REGBASE+$0064	; [A|B] ATD Control Register 4
ATDCTL5	equ	REGBASE+$0065	; [A|B] ATD Control Register 5
ATDSTAT	equ	REGBASE+$0066	; [A|B] ATD Status Register (Word)
ATDTEST	equ	REGBASE+$0068	; [A|B] ATD Test Register (Word)

PORTAD	equ	REGBASE+$006f	; [A|B] Port AD Data Input Register

ADR0H	equ	REGBASE+$0070	; [A|B] A/D Converter Result Register 0

ADR1H	equ	REGBASE+$0072	; [A|B] A/D Converter Result Register 1

ADR2H	equ	REGBASE+$0074	; [A|B] A/D Converter Result Register 2

ADR3H	equ	REGBASE+$0076	; [A|B] A/D Converter Result Register 3

ADR4H	equ	REGBASE+$0078	; [A|B] A/D Converter Result Register 4

ADR5H	equ	REGBASE+$007a	; [A|B] A/D Converter Result Register 5

ADR6H	equ	REGBASE+$007c	; [A|B] A/D Converter Result Register 6

ADR7H	equ	REGBASE+$007e	; [A|B] A/D Converter Result Register 7

TIOS	equ	REGBASE+$0080	; [A|B] Timer Input Capture/Output Compare Select
CFORC	equ	REGBASE+$0081	; [A|B] Timer Compare Force Register
OC7M	equ	REGBASE+$0082	; [A|B] Output Compare 7 Mask Register
OC7D	equ	REGBASE+$0083	; [A|B] Output Compare 7 Data Register
TCNT	equ	REGBASE+$0084	; [A|B] Timer Count Register (Word)
TSCR	equ	REGBASE+$0086	; [A|B] Timer System Control Register
TQCR	equ	REGBASE+$0087	; [A|B] Reserved
TCTL1	equ	REGBASE+$0088	; [A|B] Timer Control Register 1
TCTL2	equ	REGBASE+$0089	; [A|B] Timer Control Register 2
TCTL3	equ	REGBASE+$008a	; [A|B] Timer Control Register 3
TCTL4	equ	REGBASE+$008b	; [A|B] Timer Control Register 4
TMSK1	equ	REGBASE+$008c	; [A|B] Timer Interrupt Mask 1
TMSK2	equ	REGBASE+$008d	; [A|B] Timer Interrupt Mask 2
TFLG1	equ	REGBASE+$008e	; [A|B] Timer Interrupt Flag 1
TFLG2	equ	REGBASE+$008f	; [A|B] Timer Interrupt Flag 2
TC0	equ	REGBASE+$0090	; [A|B] Timer Input Capture/Output Compare Register 0 (Word)
TC1	equ	REGBASE+$0092	; [A|B] Timer Input Capture/Output Compare Register 1 (Word)
TC2	equ	REGBASE+$0094	; [A|B] Timer Input Capture/Output Compare Register 2 (Word)
TC3	equ	REGBASE+$0096	; [A|B] Timer Input Capture/Output Compare Register 3 (Word)
TC4	equ	REGBASE+$0098	; [A|B] Timer Input Capture/Output Compare Register 4 (Word)
TC5	equ	REGBASE+$009a	; [A|B] Timer Input Capture/Output Compare Register 5 (Word)
TC6	equ	REGBASE+$009c	; [A|B] Timer Input Capture/Output Compare Register 6 (Word)
TC7	equ	REGBASE+$009e	; [A|B] Timer Input Capture/Output Compare Register 7 (Word)
PACTL	equ	REGBASE+$00a0	; [A|B] Pulse Accumulator Control Register
PAFLG	equ	REGBASE+$00a1	; [A|B] Pulse Accumulator Flag Register
PACNT	equ	REGBASE+$00a2	; [A|B] 16-bit Pulse Accumulator Count Register (Word)

TIMTST	equ	REGBASE+$00ad	; [A|B] Timer Test Register
PORTT	equ	REGBASE+$00ae	; [A|B] Port T Register
DDRT	equ	REGBASE+$00af	; [A|B] Port T Data Direction Register

SC0BDH	equ	REGBASE+$00c0	; [A|B] SCI 0 Baud Rate Control Register High
SC0BDL	equ	REGBASE+$00c1	; [A|B] SCI 0 Baud Rate Control Register Low
SC0CR1	equ	REGBASE+$00c2	; [A|B] SCI 0 Control Register 1
SC0CR2	equ	REGBASE+$00c3	; [A|B] SCI 0 Control Register 2
SC0SR1	equ	REGBASE+$00c4	; [A|B] SCI 0 Status Register 1
SC0SR2	equ	REGBASE+$00c5	; [A|B] SCI 0 Status Register 2
SC0DRH	equ	REGBASE+$00c6	; [A|B] SCI 0 Data Register High
SC0DRL	equ	REGBASE+$00c7	; [A|B] SCI 0 Data Register Low
SC1BDH	equ	REGBASE+$00c8	; [A|-] SCI 1 Baud Rate Control Register High
SC1BDL	equ	REGBASE+$00c9	; [A|-] SCI 1 Baud Rate Control Register Low
SC1CR1	equ	REGBASE+$00ca	; [A|-] SCI 1 Control Register 1
SC1CR2	equ	REGBASE+$00cb	; [A|-] SCI 1 Control Register 2
SC1SR1	equ	REGBASE+$00cc	; [A|-] SCI 1 Status Register 1
SC1SR2	equ	REGBASE+$00cd	; [A|-] SCI 1 Status Register 2
SC1DRH	equ	REGBASE+$00ce	; [A|-] SCI 1 Data Register High
SC1DRL	equ	REGBASE+$00cf	; [A|-] SCI 1 Data Register Low
SP0CR1	equ	REGBASE+$00d0	; [A|B] SPI Control Register 1
SP0CR2	equ	REGBASE+$00d1	; [A|B] SPI Control Register 2
SP0BR	equ	REGBASE+$00d2	; [A|B] SPI Baud Rate Register
SP0SR	equ	REGBASE+$00d3	; [A|B] SPI Status Register

SP0DR	equ	REGBASE+$00d5	; [A|B] SPI Data Register
PORTS	equ	REGBASE+$00d6	; [A|B] Port S Register
DDRS	equ	REGBASE+$00d7	; [A|B] Port S Data Direction Register

PURDS	equ	REGBASE+$00db	; [-|B] Pullup and Reduced Drive for Port S

EEMCR	equ	REGBASE+$00f0	; [A|B] EEPROM Module Configuration
EEPROT	equ	REGBASE+$00f1	; [A|B] EEPROM Block Protect
EETST	equ	REGBASE+$00f2	; [A|B] EEPROM Test
EEPROG	equ	REGBASE+$00f3	; [A|B] EEPROM Control
FEELCK	equ	REGBASE+$00f4	; [-|B] Flash EEPROM Lock Control Register
FEEMCR	equ	REGBASE+$00f5	; [-|B] Flash EEPROM Module Configuration Register
FEETST	equ	REGBASE+$00f6	; [-|B] Flash EEPROM Module Test Register
FEECTL	equ	REGBASE+$00f7	; [-|B] Flash EEPROM Control Register
BCR1	equ	REGBASE+$00f8	; [-|B] BDLC Control Register 1
BSVR	equ	REGBASE+$00f9	; [-|B] BDLC State Vector Register
BCR2	equ	REGBASE+$00fa	; [-|B] BDLC Control Register 2
BDR	equ	REGBASE+$00fb	; [-|B] BDLC Data Register
BARD	equ	REGBASE+$00fc	; [-|B] BDLC Analog Roundtrip Delay Register
DLCSCR	equ	REGBASE+$00fd	; [-|B] Port DLC Control Register
PORTDLC	equ	REGBASE+$00fe	; [-|B] Port DLC Data Register
DDRDLC	equ	REGBASE+$00ff	; [-|B] Port DLC Data Direction Register

;-----------------------------------------------------------------------------

                endif                   ; von IFDEF...
		restore                 ; wieder erlauben


