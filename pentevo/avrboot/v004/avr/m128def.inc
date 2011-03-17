;***************************************************************************
;* A P P L I C A T I O N   N O T E   F O R   T H E   A V R   F A M I L Y
;* 
;* Number				: AVR000
;* File Name			: "m128def.inc"
;* Title				: Register/Bit Definitions for the ATmega128
;* Date                 : 07.09.2001
;* Version              : 1.0
;* Support E-mail		: avr@atmel.no
;* Target MCU			: ATmega128
;*
;* DESCRIPTION
;* When including this file in the assembly program file, all I/O register	
;* names and I/O register bit names appearing in the data book can be used.
;* In addition, the six registers forming the three data pointers X, Y and
;* Z have been assigned names XL - ZH. Highest RAM address for Internal 
;* SRAM is also defined 
;*
;* The Register names are represented by their hexadecimal address.
;* 
;* The Register Bit names are represented by their bit number (0-7).
;* 
;* Please observe the difference in using the bit names with instructions
;* such as "sbr"/"cbr" (set/clear bit in register) and "sbrs"/"sbrc" 
;* (skip if bit in register set/cleared). The following example illustrates
;* this:
;* 
;* in	r16,PORTB		;read PORTB latch
;* sbr	r16,(1<<PB6)+(1<<PB5)	;set PB6 and PB5 (use masks, not bit#)
;* out  PORTB,r16		;output to PORTB
;*
;* in	r16,TIFR		;read the Timer Interrupt Flag Register
;* sbrc	r16,TOV0		;test the overflow flag (use bit#)
;* rjmp	TOV0_is_set		;jump if set
;* ...				;otherwise do something else
;***************************************************************************

;**** Specify Device ****
.device ATmega128

;*****************************************************************************
; I/O Register Definitions
;*****************************************************************************

;**** Memory Mapped I/O Register Definitions ($FF-$60) ****
.equ	UCSR1C	= $9D
.equ	UDR1	= $9C
.equ	UCSR1A	= $9B
.equ	UCSR1B	= $9A
.equ	UBRR1L	= $99
.equ	UBRR1H	= $98
.equ	UCSR0C	= $95
.equ	UBRR0H	= $90
.equ	TCCR3C	= $8C
.equ	TCCR3A	= $8B
.equ	TCCR3B	= $8A
.equ	TCNT3H	= $89
.equ	TCNT3L	= $88
.equ	OCR3AH	= $87
.equ	OCR3AL	= $86
.equ	OCR3BH	= $85
.equ	OCR3BL	= $84
.equ	OCR3CH	= $83
.equ	OCR3CL	= $82
.equ	ICR3H	= $81
.equ	ICR3L	= $80
.equ	ETIMSK	= $7D
.equ	ETIFR	= $7C
.equ	TCCR1C	= $7A
.equ	OCR1CH	= $79
.equ	OCR1CL	= $78
.equ	TWCR	= $74
.equ	TWDR	= $73
.equ	TWAR	= $72
.equ	TWSR	= $71
.equ	TWBR	= $70
.equ	OSCCAL	= $6F
.equ	XMCRA	= $6D
.equ	XMCRB	= $6C
.equ	EICRA	= $6A
.equ	SPMCSR	= $68
.equ	SPMCR	= $68		; old name for SPMCSR
.equ	PORTG	= $65
.equ	DDRG	= $64
.equ	PING	= $63
.equ	PORTF	= $62
.equ	DDRF	= $61

;**** I/O Register Definitions ($3F-$00) ****
.equ	SREG	= $3F
.equ	SPH		= $3E
.equ	SPL		= $3D
.equ	XDIV	= $3C
.equ	RAMPZ	= $3B
.equ	EICRB	= $3A
.equ	EIMSK	= $39
.equ	GIMSK	= $39		; old name for EIMSK
.equ	GICR	= $39		; old name for EIMSK
.equ	EIFR	= $38
.equ	GIFR	= $38		; old name for EIFR
.equ	TIMSK	= $37
.equ	TIFR	= $36
.equ	MCUCR	= $35
.equ	MCUCSR	= $34
.equ	TCCR0	= $33
.equ	TCNT0	= $32
.equ	OCR0	= $31
.equ	ASSR	= $30
.equ	TCCR1A	= $2F
.equ	TCCR1B	= $2E
.equ	TCNT1H	= $2D
.equ	TCNT1L	= $2C
.equ	OCR1AH	= $2B
.equ	OCR1AL	= $2A
.equ	OCR1BH	= $29
.equ	OCR1BL	= $28
.equ	ICR1H	= $27
.equ	ICR1L	= $26
.equ	TCCR2	= $25
.equ	TCNT2	= $24
.equ	OCR2	= $23
.equ	OCDR	= $22		; New
.equ	WDTCR	= $21
.equ	SFIOR	= $20		; New
.equ	EEARH	= $1F
.equ	EEARL	= $1E
.equ	EEDR	= $1D
.equ	EECR	= $1C
.equ	PORTA	= $1B
.equ	DDRA	= $1A
.equ	PINA	= $19
.equ	PORTB	= $18
.equ	DDRB	= $17
.equ	PINB	= $16
.equ	PORTC	= $15
.equ	DDRC	= $14		; New
.equ	PINC	= $13		; New
.equ	PORTD	= $12
.equ	DDRD	= $11
.equ	PIND	= $10
.equ	SPDR	= $0F
.equ	SPSR	= $0E
.equ	SPCR	= $0D
.equ	UDR0	= $0C
.equ	UCSR0A	= $0B
.equ	UCSR0B	= $0A
.equ	UBRR0L	= $09
.equ	ACSR	= $08
.equ	ADMUX	= $07
.equ	ADCSR	= $06
.equ	ADCH	= $05
.equ	ADCL	= $04
.equ	PORTE	= $03
.equ	DDRE	= $02
.equ	PINE	= $01
.equ	PINF	= $00


;*****************************************************************************
; Bit Definitions
;*****************************************************************************

;**** MCU Control ****
.equ	SRE		= 7		; MCUCR
.equ	SRW10	= 6
.equ	SE		= 5
.equ	SM1		= 4
.equ	SM0		= 3
.equ	SM2		= 2
.equ	IVSEL	= 1
.equ	IVCE	= 0

.equ	JTD		= 7		; MCUCSR
.equ	JTRF	= 4	
.equ	WDRF	= 3
.equ	BORF	= 2
.equ	EXTRF	= 1
.equ	PORF	= 0

.equ	SRL2	=6		; XMCRA
.equ	SRL1	=5
.equ	SRL0	=4
.equ	SRW01	=3
.equ	SRW00	=2
.equ	SRW11	=1

.equ	XMBK	= 7		; XMCRB
.equ	XMM2	= 2
.equ	XMM1	= 1
.equ	XMM0	= 0

.equ	SPMIE	=7		; SPMCSR
.equ	ASB		=6		; backwards compatiblity
.equ	ASRE	=4		; backwards compatiblity
.equ	RWWSB	=6
.equ	RWWSRE	=4	
.equ	BLBSET	=3
.equ	PGWRT	=2
.equ	PGERS	=1
.equ	SPMEN	=0

.equ	IDRD	= 7		; OCDR
.equ	OCDR6	= 6
.equ	OCDR5	= 5
.equ	OCDR4	= 4	
.equ	OCDR3	= 3
.equ	OCDR2	= 2
.equ	OCDR1	= 1
.equ	OCDR0	= 0

.equ	XDIVEN	= 7		; XDIV
.equ	XDIV6	= 6
.equ	XDIV5	= 5
.equ	XDIV4	= 4
.equ	XDIV3	= 3
.equ	XDIV2	= 2
.equ	XDIV1	= 1
.equ	XDIV0	= 0

.equ	TSM		= 7		; SFIOR
.equ	ADHSM	= 4
.equ	ACME	= 3
.equ	PUD		= 2
.equ	PSR0	= 1
.equ	PSR1	= 0
.equ	PSR2	= 0
.equ	PSR3	= 0
.equ	PSR321	= 0	

;**** Analog to Digital Converter ****
.equ	ADEN	= 7		; ADCSR
.equ	ADSC	= 6
.equ	ADFR	= 5
.equ	ADIF	= 4
.equ	ADIE	= 3
.equ	ADPS2	= 2
.equ	ADPS1	= 1
.equ	ADPS0	= 0

.equ    REFS1   =7		; ADMUX
.equ    REFS0   =6
.equ    ADLAR   =5
.equ    MUX4    =4
.equ    MUX3    =3
.equ    MUX2    =2
.equ    MUX1    =1
.equ    MUX0    =0

;**** Analog Comparator ****
.equ	ACD		= 7		; ACSR
.equ	ACBG	= 6
.equ	ACO		= 5
.equ	ACI		= 4
.equ	ACIE	= 3
.equ	ACIC	= 2
.equ	ACIS1	= 1
.equ	ACIS0	= 0
	

;**** External Interrupts ****
.equ	INT7	= 7		; EIMSK
.equ	INT6	= 6
.equ	INT5	= 5
.equ	INT4	= 4
.equ	INT3	= 3
.equ	INT2	= 2
.equ	INT1	= 1
.equ	INT0	= 0

.equ	INTF7	= 7		; EIFR
.equ	INTF6	= 6
.equ	INTF5	= 5
.equ	INTF4	= 4
.equ	INTF3	= 3
.equ	INTF2	= 2
.equ	INTF1	= 1
.equ	INTF0	= 0

.equ	ISC71	= 7		; EICRB
.equ	ISC70	= 6
.equ	ISC61	= 5
.equ	ISC60	= 4
.equ	ISC51	= 3
.equ	ISC50	= 2
.equ	ISC41	= 1
.equ	ISC40	= 0

.equ	ISC31	= 7		; EICRA
.equ	ISC30	= 6
.equ	ISC21	= 5
.equ	ISC20	= 4
.equ	ISC11	= 3
.equ	ISC10	= 2
.equ	ISC01	= 1
.equ	ISC00	= 0

;**** Timer Interrupts ****
.equ	OCIE2	= 7		; TIMSK
.equ	TOIE2	= 6
.equ	TICIE1	= 5
.equ	OCIE1A	= 4
.equ	OCIE1B	= 3
.equ	TOIE1	= 2
.equ	OCIE0	= 1
.equ	TOIE0	= 0

.equ	TICIE3	= 5		; ETIMSK
.equ	OCIE3A	= 4
.equ	OCIE3B	= 3
.equ	TOIE3	= 2
.equ	OCIE3C	= 1
.equ	OCIE1C	= 0

.equ	OCF2	= 7		; TIFR
.equ	TOV2	= 6
.equ	ICF1	= 5
.equ	OCF1A	= 4
.equ	OCF1B	= 3
.equ	TOV1	= 2
.equ	OCF0	= 1
.equ	TOV0	= 0

.equ	ICF3	= 5		; ETIFR
.equ	OCF3A	= 4
.equ	OCF3B	= 3
.equ	TOV3	= 2
.equ	OCF3C	= 1
.equ	OCF1C	= 0

;**** Asynchronous Timer ****
.equ	AS0		= 3		; ASSR
.equ	TCN0UB	= 2
.equ	OCR0UB	= 1
.equ	TCR0UB	= 0

;**** Timer 0 ****
.equ    FOC0    = 7		; TCCR0
.equ	WGM00	= 6
.equ	COM01	= 5
.equ	COM00	= 4
.equ	WGM01	= 3
.equ	CS02	= 2
.equ	CS01	= 1
.equ	CS00	= 0

;**** Timer 1 ****
.equ	COM1A1	= 7		; TCCR1A
.equ	COM1A0	= 6
.equ	COM1B1	= 5
.equ	COM1B0	= 4
.equ	COM1C1	= 3
.equ	COM1C0	= 2
.equ	PWM11	= 1		; OBSOLETE! Use WGM11
.equ	PWM10	= 0		; OBSOLETE! Use WGM10
.equ	WGM11	= 1
.equ	WGM10	= 0

.equ	ICNC1	= 7		; TCCR1B
.equ	ICES1	= 6
.equ	CTC11	= 4		; OBSOLETE! Use WGM13
.equ	CTC10	= 3		; OBSOLETE! Use WGM12
.equ	WGM13	= 4
.equ	WGM12	= 3
.equ	CS12	= 2
.equ	CS11	= 1
.equ	CS10	= 0

.equ	FOC1A	= 7		; TCCR1C
.equ	FOC1B	= 6
.equ	FOC1C	= 5

;**** Timer 2 ****
.equ	FOC2	= 7		; TCCR2
.equ	WGM20	= 6
.equ	COM21	= 5
.equ	COM20	= 4
.equ	WGM21	= 3
.equ	CS22	= 2
.equ	CS21	= 1
.equ	CS20	= 0

;**** Timer 3 ****
.equ	COM3A1	= 7		; TCCR3A
.equ	COM3A0	= 6
.equ	COM3B1	= 5
.equ	COM3B0	= 4
.equ	COM3C1	= 3
.equ	COM3C0	= 2
.equ	PWM31	= 1		; OBSOLETE! Use WGM31
.equ	PWM30	= 0		; OBSOLETE! Use WGM30
.equ	WGM31	= 1
.equ	WGM30	= 0

.equ	ICNC3	= 7		; TCCR3B
.equ	ICES3	= 6
.equ	CTC31	= 4		; OBSOLETE! Use WGM33
.equ	CTC30	= 3		; OBSOLETE! Use WGM32
.equ	WGM33	= 4
.equ	WGM32	= 3
.equ	CS32	= 2
.equ	CS31	= 1
.equ	CS30	= 0

.equ	FOC3A	= 7		; TCCR3C
.equ	FOC3B	= 6
.equ	FOC3C	= 5

;**** Watchdog Timer ****
.equ	WDCE	= 4		; WDTCR
.equ	WDTOE	= 4		; For Mega103 compability
.equ	WDE	= 3
.equ	WDP2	= 2
.equ	WDP1	= 1
.equ	WDP0	= 0

;**** EEPROM  Control Register ****
.equ	EERIE	= 3		; EECR
.equ	EEMWE	= 2
.equ	EEWE	= 1
.equ	EERE	= 0

;**** USART 0 and USART 1 ****
.equ	RXC	= 7		; (UCSRA0/1)
.equ	TXC	= 6
.equ	UDRE	= 5
.equ	FE	= 4
.equ	DOR	= 3
.equ	PE	= 2		; OBSOLETED!
.equ	U2X	= 1
.equ	MPCM	= 0

.equ	RXC0	= 7		; (UCSR0A)
.equ	TXC0	= 6
.equ	UDRE0	= 5
.equ	FE0	= 4
.equ	DOR0	= 3
.equ	UPE0	= 2
.equ	U2X0	= 1
.equ	MPCM0	= 0

.equ	RXC1	= 7		; (UCSR1A)
.equ	TXC1	= 6
.equ	UDRE1	= 5
.equ	FE1	= 4
.equ	DOR1	= 3
.equ	UPE1	= 2
.equ	U2X1	= 1
.equ	MPCM1	= 0

.equ	RXCIE	= 7		; (UCSRB0/1)
.equ	TXCIE	= 6
.equ	UDRIE	= 5
.equ	RXEN	= 4
.equ	TXEN	= 3
.equ	UCSZ2	= 2
.equ	RXB8	= 1
.equ	TXB8	= 0

.equ	RXCIE0	= 7		; (UCSR0B)
.equ	TXCIE0	= 6
.equ	UDRIE0	= 5
.equ	RXEN0	= 4
.equ	TXEN0	= 3
.equ	UCSZ02	= 2
.equ	RXB80	= 1
.equ	TXB80	= 0

.equ	RXCIE1	= 7		; (UCSR1B)
.equ	TXCIE1	= 6
.equ	UDRIE1	= 5
.equ	RXEN1	= 4
.equ	TXEN1	= 3
.equ	UCSZ12	= 2
.equ	RXB81	= 1
.equ	TXB81	= 0

.equ	UMSEL	= 6		; (UCSRC0/1)
.equ	UPM1	= 5
.equ	UPM0	= 4
.equ	USBS	= 3
.equ	UCSZ1	= 2
.equ	UCSZ0	= 1
.equ	UCPOL	= 0

.equ	UMSEL0	= 6		; (UCSR0C)
.equ	UPM01	= 5
.equ	UPM00	= 4
.equ	USBS0	= 3
.equ	UCSZ01	= 2
.equ	UCSZ00	= 1
.equ	UCPOL0	= 0

.equ	UMSEL1	= 6		; (UCSR1C)
.equ	UPM11	= 5
.equ	UPM10	= 4
.equ	USBS1	= 3
.equ	UCSZ11	= 2
.equ	UCSZ10	= 1
.equ	UCPOL1	= 0

	
;**** SPI ****
.equ	SPIE	= 7		; SPCR
.equ	SPE	= 6
.equ	DORD	= 5
.equ	MSTR	= 4
.equ	CPOL	= 3
.equ	CPHA	= 2
.equ	SPR1	= 1
.equ	SPR0	= 0

.equ	SPIF	= 7		; SPSR
.equ	WCOL	= 6
.equ	SPI2X	= 0

;**** TWI ****	
.equ	TWINT	= 7		;TWCR
.equ	TWEA	= 6
.equ	TWSTA	= 5
.equ	TWSTO	= 4
.equ	TWWC	= 3
.equ    TWEN    = 2
.equ	TWIE	= 0

.equ	TWS7	= 7		; TWSR
.equ	TWS6	= 6
.equ	TWS5	= 5
.equ	TWS4	= 4
.equ	TWS3	= 3
.equ	TWPS1	= 1
.equ	TWPS0	= 0

.equ	TWA6	= 7
.equ	TWA5	= 6
.equ	TWA4	= 5
.equ	TWA3	= 4
.equ	TWA2	= 3
.equ	TWA1	= 2
.equ	TWA0	= 1
.equ	TWGCE	= 0		; TWAR

		
;**** PORT A ****
.equ	PA7	= 7		; PORTA
.equ	PA6	= 6
.equ	PA5	= 5
.equ	PA4	= 4
.equ	PA3	= 3
.equ	PA2	= 2
.equ	PA1	= 1
.equ	PA0	= 0
.equ	PORTA7	= 7
.equ	PORTA6	= 6
.equ	PORTA5	= 5
.equ	PORTA4	= 4
.equ	PORTA3	= 3
.equ	PORTA2	= 2
.equ	PORTA1	= 1
.equ	PORTA0	= 0

.equ	DDA7	= 7		; DDRA
.equ	DDA6	= 6
.equ	DDA5	= 5
.equ	DDA4	= 4
.equ	DDA3	= 3
.equ	DDA2	= 2
.equ	DDA1	= 1
.equ	DDA0	= 0

.equ	PINA7	= 7		; PINA
.equ	PINA6	= 6
.equ	PINA5	= 5
.equ	PINA4	= 4
.equ	PINA3	= 3
.equ	PINA2	= 2
.equ	PINA1	= 1
.equ	PINA0	= 0

;**** PORT B ****
.equ	PB7	= 7		; PORTB
.equ	PB6	= 6
.equ	PB5	= 5
.equ	PB4	= 4
.equ	PB3	= 3
.equ	PB2	= 2
.equ	PB1	= 1
.equ	PB0	= 0
.equ	PORTB7	= 7
.equ	PORTB6	= 6
.equ	PORTB5	= 5
.equ	PORTB4	= 4
.equ	PORTB3	= 3
.equ	PORTB2	= 2
.equ	PORTB1	= 1
.equ	PORTB0	= 0

.equ	DDB7	= 7		; DDRB
.equ	DDB6	= 6
.equ	DDB5	= 5
.equ	DDB4	= 4
.equ	DDB3	= 3
.equ	DDB2	= 2
.equ	DDB1	= 1
.equ	DDB0	= 0

.equ	PINB7	= 7		; PINB
.equ	PINB6	= 6
.equ	PINB5	= 5
.equ	PINB4	= 4
.equ	PINB3	= 3
.equ	PINB2	= 2
.equ	PINB1	= 1
.equ	PINB0	= 0

;**** PORT C ****
.equ	PC7	= 7		; PORTC
.equ	PC6	= 6
.equ	PC5	= 5
.equ	PC4	= 4
.equ	PC3	= 3
.equ	PC2	= 2
.equ	PC1	= 1
.equ	PC0	= 0
.equ	PORTC7	= 7
.equ	PORTC6	= 6
.equ	PORTC5	= 5
.equ	PORTC4	= 4
.equ	PORTC3	= 3
.equ	PORTC2	= 2
.equ	PORTC1	= 1
.equ	PORTC0	= 0

.equ	DDC7	= 7		; DDRC
.equ	DDC6	= 6
.equ	DDC5	= 5
.equ	DDC4	= 4
.equ	DDC3	= 3
.equ	DDC2	= 2
.equ	DDC1	= 1
.equ	DDC0	= 0

.equ	PINC7	= 7		; PINC
.equ	PINC6	= 6
.equ	PINC5	= 5
.equ	PINC4	= 4
.equ	PINC3	= 3
.equ	PINC2	= 2
.equ	PINC1	= 1
.equ	PINC0	= 0

;**** PORT D ****
.equ	PD7	= 7		; PORTD
.equ	PD6	= 6
.equ	PD5	= 5
.equ	PD4	= 4
.equ	PD3	= 3
.equ	PD2	= 2
.equ	PD1	= 1
.equ	PD0	= 0
.equ	PORTD7	= 7
.equ	PORTD6	= 6
.equ	PORTD5	= 5
.equ	PORTD4	= 4
.equ	PORTD3	= 3
.equ	PORTD2	= 2
.equ	PORTD1	= 1
.equ	PORTD0	= 0

.equ	DDD7	= 7		; DDRD
.equ	DDD6	= 6
.equ	DDD5	= 5
.equ	DDD4	= 4
.equ	DDD3	= 3
.equ	DDD2	= 2
.equ	DDD1	= 1
.equ	DDD0	= 0

.equ	PIND7	= 7		; PIND
.equ	PIND6	= 6
.equ	PIND5	= 5
.equ	PIND4	= 4
.equ	PIND3	= 3
.equ	PIND2	= 2
.equ	PIND1	= 1
.equ	PIND0	= 0

;**** PORT E ****
.equ	PE7	= 7		; PORTE
.equ	PE6	= 6
.equ	PE5	= 5
.equ	PE4	= 4
.equ	PE3	= 3
.equ	PE2	= 2
.equ	PE1	= 1
.equ	PE0	= 0
.equ	PORTE7	= 7		; PORTE
.equ	PORTE6	= 6
.equ	PORTE5	= 5
.equ	PORTE4	= 4
.equ	PORTE3	= 3
.equ	PORTE2	= 2
.equ	PORTE1	= 1
.equ	PORTE0	= 0

.equ	DDE7	= 7		; DDRE
.equ	DDE6	= 6
.equ	DDE5	= 5
.equ	DDE4	= 4
.equ	DDE3	= 3
.equ	DDE2	= 2
.equ	DDE1	= 1
.equ	DDE0	= 0

.equ	PINE7	= 7		; PINE
.equ	PINE6	= 6
.equ	PINE5	= 5
.equ	PINE4	= 4
.equ	PINE3	= 3
.equ	PINE2	= 2
.equ	PINE1	= 1
.equ	PINE0	= 0

;**** PORT F ****
.equ	PF7	= 7		; PORTF
.equ	PF6	= 6
.equ	PF5	= 5
.equ	PF4	= 4
.equ	PF3	= 3
.equ	PF2	= 2
.equ	PF1	= 1
.equ	PF0	= 0
.equ	PORTF7	= 7
.equ	PORTF6	= 6
.equ	PORTF5	= 5
.equ	PORTF4	= 4
.equ	PORTF3	= 3
.equ	PORTF2	= 2
.equ	PORTF1	= 1
.equ	PORTF0	= 0

.equ	DDF7	= 7		; DDRF
.equ	DDF6	= 6
.equ	DDF5	= 5
.equ	DDF4	= 4
.equ	DDF3	= 3
.equ	DDF2	= 2
.equ	DDF1	= 1
.equ	DDF0	= 0

.equ	PINF7	= 7		; PINF
.equ	PINF6	= 6
.equ	PINF5	= 5
.equ	PINF4	= 4
.equ	PINF3	= 3
.equ	PINF2	= 2
.equ	PINF1	= 1
.equ	PINF0	= 0

;**** PORT G ****
.equ	PG4	= 4		; PORTG
.equ	PG3	= 3
.equ	PG2	= 2
.equ	PG1	= 1
.equ	PG0	= 0

.equ	DDG4	= 4		; DDRG
.equ	DDG3	= 3
.equ	DDG2	= 2
.equ	DDG1	= 1
.equ	DDG0	= 0

.equ	PING4	= 4		; PING
.equ	PING3	= 3
.equ	PING2	= 2
.equ	PING1	= 1
.equ	PING0	= 0

;**** RAM Page Z Select Register **** 
.equ RAMPZ0 = 0 



;*****************************************************************************
; CPU Register Declarations
;*****************************************************************************

.def	XL	= r26		; X pointer low
.def	XH	= r27		; X pointer high
.def	YL	= r28		; Y pointer low
.def	YH	= r29		; Y pointer high
.def	ZL	= r30		; Z pointer low
.def	ZH	= r31		; Z pointer high


;*****************************************************************************
; Data Memory Declarations
;*****************************************************************************

.equ 	RAMEND	= $10ff		; Highest internal data memory (SRAM) address.
.equ	EEPROMEND = $0fff       ; Highest EEPROM address.
	
;*****************************************************************************
; Program Memory Declarations
;*****************************************************************************

.equ    FLASHEND = $FFFF	;  Highest program memory (flash) address
	                        ; (When addressed as 16 bit words)
		
;**** Boot Vectors ****
			 ;  byte groups
			 ;  /--\/--\/--\/--\ 
.equ 	SMALLBOOTSTART	= 0b1111111000000000	; ($FE00) Smallest boot block is 512W
.equ 	SECONDBOOTSTART	= 0b1111110000000000	; ($FC00) 2'nd boot block size is 1KW
.equ 	THIRDBOOTSTART	= 0b1111100000000000	; ($F800) Third boot block size is 2KW
.equ 	LARGEBOOTSTART	= 0b1111000000000000	; ($F000) Largest boot block is 4KW


;**** Page Size ****
.equ	PAGESIZE	= 128	; Number of WORDS in a page


;**** Interrupt Vectors ****
.equ	INT0addr  = $002	; External Interrupt0 Vector Address
.equ	INT1addr  = $004	; External Interrupt1 Vector Address
.equ	INT2addr  = $006	; External Interrupt2 Vector Address
.equ	INT3addr  = $008	; External Interrupt3 Vector Address
.equ	INT4addr  = $00a	; External Interrupt4 Vector Address
.equ	INT5addr  = $00c	; External Interrupt5 Vector Address
.equ	INT6addr  = $00e	; External Interrupt6 Vector Address
.equ	INT7addr  = $010	; External Interrupt7 Vector Address
.equ	OC2addr   = $012	; Output Compare2 Interrupt Vector Address
.equ	OVF2addr  = $014	; Overflow2 Interrupt Vector Address
.equ	ICP1addr  = $016	; Input Capture1 Interrupt Vector Address
.equ	OC1Aaddr  = $018	; Output Compare1A Interrupt Vector Address
.equ	OC1Baddr  = $01a	; Output Compare1B Interrupt Vector Address
.equ	OVF1addr  = $01c	; Overflow1 Interrupt Vector Address
.equ	OC0addr   = $01e	; Output Compare0 Interrupt Vector Address
.equ	OVF0addr  = $020	; Overflow0 Interrupt Vector Address
.equ	SPIaddr   = $022	; SPI Interrupt Vector Address
.equ	URXC0addr = $024	; USART0 Receive Complete Interrupt Vector Address
.equ	UDRE0addr = $026	; USART0 Data Register Empty Interrupt Vector Address
.equ	UTXC0addr = $028	; USART0 Transmit Complete Interrupt Vector Address
.equ    ADCCaddr  = $02a	; ADC Conversion Complete Handle
.equ	ERDYaddr  = $02c	; EEPROM Write Complete Handle
.equ	ACIaddr   = $02e	; Analog Comparator Interrupt Vector Address

.equ	OC1Caddr  = $030	; Output Compare1C Interrupt Vector Address
.equ	ICP3addr  = $032	; Input Capture3 Interrupt Vector Address
.equ	OC3Aaddr  = $034	; Output Compare3A Interrupt Vector Address
.equ	OC3Baddr  = $036	; Output Compare3B Interrupt Vector Address
.equ	OC3Caddr  = $038	; Output Compare3C Interrupt Vector Address
.equ	OVF3addr  = $03A	; Overflow3 Interrupt Vector Address
.equ	URXC1addr = $03C	; USART1 Receive Complete Interrupt Vector Address
.equ	UDRE1addr = $03E	; USART1 Data Register Empty Interrupt Vector Address
.equ	UTXC1addr = $040	; USART1 Transmit Complete Interrupt Vector Address
.equ    TWIaddr   = $042	; TWI Interrupt Vector Address
.equ	SPMRaddr  = $044	; Store Program Memory Ready Interrupt Vector Address


;**** End of File ****
