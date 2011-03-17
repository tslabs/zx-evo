                save
                listing off

;****************************************************************************
;*                                                                          *
;*   REGGP32.INC                                                            *
;*   Register Definitions for MC68HC908GP32                                 *
;*   by Oliver Thamm - othamm@hc08web.de				    *
;*   Changed: May 22 2002	                                            *
;*                                                                          *
;****************************************************************************

                ifndef  reggp32inc

reggp32inc      equ     1

                if      (MOMCPUNAME<>"68HC08")
                 fatal  "Invalid Processor Selection: only 68HC08 allowed!"
		endif


PTA 		equ $0000 ; Port A Data Register
PTB 		equ $0001 ; Port B Data Register
PTC 		equ $0002 ; Port C Data Register
PTD 		equ $0003 ; Port D Data Register
DDRA		equ $0004 ; Data Direction Register A
DDRB		equ $0005 ; Data Direction Register B
DDRC		equ $0006 ; Data Direction Register C
DDRD		equ $0007 ; Data Direction Register D
PTE 		equ $0008 ; Port E Data Register
DDRE		equ $000c ; Data Direction Register E
PTAPUE		equ $000d ; Port A Input Pullup Enable Register
PTCPUE		equ $000e ; Port C Input Pullup Enable Register
PTDPUE		equ $000f ; Port D Input Pullup Enable Register
SPCR		equ $0010 ; SPI Control Register
SPSCR		equ $0011 ; SPI Status and Control Register
SPDR		equ $0012 ; SPI Data Register
SCC1		equ $0013 ; SCI Control Register 1
SCC2		equ $0014 ; SCI Control Register 2
SCC3		equ $0015 ; SCI Control Register 3
SCS1		equ $0016 ; SCI Status Register 1
SCS2		equ $0017 ; SCI Status Register 2
SCDR		equ $0018 ; SCI Data Register
SCBR		equ $0019 ; SCI Baud Rate Register
INTKBSCR	equ $001a ; Keyboard Status and Control Register
INTKBIER	equ $001b ; Keyboard Interrupt Enable Register
TBCR		equ $001c ; Time Base Module Control Register
INTSCR		equ $001d ; IRQ Status and Control Register
CONFIG2		equ $001e ; Configuration Register 2
CONFIG1		equ $001f ; Configuration Register 1
T1SC		equ $0020 ; Timer 1 Status and Control Register
T1CNT		equ $0021 ; Timer 1 Counter Register
T1CNTH		equ $0021 ; Timer 1 Counter Register High
T1CNTL		equ $0022 ; Timer 1 Counter Register Low
T1MOD		equ $0023 ; Timer 1 Counter Modulo Register
T1MODH		equ $0023 ; Timer 1 Counter Modulo Register High
T1MODL		equ $0024 ; Timer 1 Counter Modulo Register Low
T1SC0		equ $0025 ; Timer 1 Channel 0 Status and Control Register
T1CH0		equ $0026 ; Timer 1 Channel 0 Register
T1CH0H		equ $0026 ; Timer 1 Channel 0 Register High
T1CH0L		equ $0027 ; Timer 1 Channel 0 Register Low
T1SC1		equ $0028 ; Timer 1 Channel 1 Status and Control Register
T1CH1		equ $0029 ; Timer 1 Channel 1 Register
T1CH1H		equ $0029 ; Timer 1 Channel 1 Register High
T1CH1L		equ $002a ; Timer 1 Channel 1 Register Low
T2SC		equ $002b ; Timer 2 Status and Control Register
T2CNT		equ $002c ; Timer 2 Counter Register
T2CNTH		equ $002c ; Timer 2 Counter Register High
T2CNTL		equ $002d ; Timer 2 Counter Register Low
T2MOD		equ $002e ; Timer 2 Counter Modulo Register
T2MODH		equ $002e ; Timer 2 Counter Modulo Register High
T2MODL		equ $002f ; Timer 2 Counter Modulo Register Low
T2SC0		equ $0030 ; Timer 2 Channel 0 Status and Control Register
T2CH0		equ $0031 ; Timer 2 Channel 0 Register
T2CH0H		equ $0031 ; Timer 2 Channel 0 Register High
T2CH0L		equ $0032 ; Timer 2 Channel 0 Register Low
T2SC1		equ $0033 ; Timer 2 Channel 1 Status and Control Register
T2CH1		equ $0034 ; Timer 2 Channel 1 Register
T2CH1H		equ $0034 ; Timer 2 Channel 1 Register High
T2CH1L		equ $0035 ; Timer 2 Channel 1 Register Low
PCTL		equ $0036 ; PLL Control Register
PBWC		equ $0037 ; PLL Bandwidth Control Register
PMS 		equ $0038 ; PLL Multiplier Select Register
PMSH		equ $0038 ; PLL Multiplier Select Register High
PMSL		equ $0039 ; PLL Multiplier Select Register Low
PMRS		equ $003a ; PLL VCO Select Range Register
PMDS		equ $003b ; PLL Reference Divider Select Register
ADSCR		equ $003c ; A/D Status and Control Register
ADR 		equ $003d ; A/D Data Register
ADCLK		equ $003e ; A/D Clock Register
SBSR		equ $fe00 ; SIM Break Status Register
SRSR		equ $fe01 ; SIM Reset Status Register
SUBAR		equ $fe02 ; SIM Upper Byte Addess Register
SBFCR		equ $fe03 ; SIM Break Flag Control Register
INT1		equ $fe04 ; Interrupt Status Register 1
INT2		equ $fe05 ; Interrupt Status Register 2
INT3		equ $fe06 ; Interrupt Status Register 3
FLCR		equ $fe08 ; Flash Control Register
BRK 		equ $fe09 ; Break Address Register
BRKH		equ $fe09 ; Break Address Register Low
BRKL		equ $fe0a ; Break Address Register High
BRKSCR		equ $fe0b ; Break Status and Control Register
LVISR		equ $fe0c ; LVI Status Register
FLBPR		equ $ff7e ; Flash Block Protect Register
COPCTL		equ $ffff ; COP Control Register


                endif
		restore

