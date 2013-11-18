;   83C552 processor definition file
;   ================================

; NOTE: 80c552 is not a predefined processor type in AS.
; to use this file, add '-alias 80c552=8052' to the command line
; or key file

        if      MOMCPUNAME<>"80C552"
         fatal   "80C552.INC: Falscher Prozessortyp, nur 80C552 erlaubt"
        endif

        if      MOMPASS=1
	 message "80C552-Registerdefinitionen"
	 message "(C) 1996 Alfred Arnold"
	endif

;-----------------------------------------------------------------------------

ACC	sfrb 	0E0H
B	sfrb 	0F0H
SP	sfr 	081H
DPL	sfr 	082H
DPH	sfr 	083H

PSW	sfrb 	0D0H
P	bit	PSW.0
F1	bit	PSW.1
OV	bit	PSW.2
RS0	bit	PSW.3
RS1	bit	PSW.4
F0	bit	PSW.5
AC	bit	PSW.6
CY	bit	PSW.7

;-----------------------------------------------------------------------------

PCON	sfr 	087H

;-----------------------------------------------------------------------------

IEN0	sfrb 	0A8H
EX0	bit	IEN0.0
ET0	bit	IEN0.1
EX1	bit	IEN0.2
ET1	bit	IEN0.3
ES0	bit	IEN0.4
ES1	bit	IEN0.5
EAD	bit	IEN0.6
EA	bit	IEN0.7

IEN1	sfrb 	0E8H
ECT0	bit	IEN1.0
ECT1	bit	IEN1.1
ECT2	bit	IEN1.2
ECT3	bit	IEN1.3
ECM0	bit	IEN1.4
ECM1	bit	IEN1.5
ECM2	bit	IEN1.6
ET2	bit	IEN1.7

IP0	sfrb 	0B8H
PX0	bit	IP0.0
PT0	bit	IP0.1
PX1	bit	IP0.2
PT1	bit	IP0.3
PS0	bit	IP0.4
PS1	bit	IP0.5
PAD	bit	IP0.6

IP1	sfrb 	0F8H
PCT0	bit	IP1.0
PCT1	bit	IP1.1
PCT2	bit	IP1.2
PCT3	bit	IP1.3
PCM0	bit	IP1.4
PCM1	bit	IP1.5
PCM2	bit	IP1.6
PT2	bit	IP1.7

;-----------------------------------------------------------------------------

P0	sfrb 	080H

P1	sfrb 	090H
CT0I	bit	P1.0
CT1I	bit	P1.1
CT2I	bit	P1.2
CT3I	bit	P1.3
T2	bit	P1.4
RT2	bit	P1.5
SCL	bit	P1.6
SDA	bit	P1.7

P2	sfrb 	0A0H

P3	sfrb 	0B0H
RXD	bit	P3.0
TXD	bit	P3.1
INT0	bit	P3.2
INT1	bit	P3.3
T0	bit	P3.4
T1	bit	P3.5
WR	bit	P3.6
RD	bit	P3.7

P4	sfrb 	0C0H
CMSR0	bit	P4.0
CMSR1	bit	P4.1
CMSR2	bit	P4.2
CMSR3	bit	P4.3
CMSR4	bit	P4.4
CMSR5	bit	P4.5
CMT0	bit	P4.6
CMT1	bit	P4.7

P5	sfr 	0C4H

;-----------------------------------------------------------------------------

TCON	sfrb 	088H
IT0	bit	TCON.0
IE0	bit	TCON.1
IT1	bit	TCON.2
IE1	bit	TCON.3
TR0	bit	TCON.4
TF0	bit	TCON.5
TR1	bit	TCON.6
TF1	bit	TCON.7

TMOD	sfr 	089H
TL0	sfr 	08AH
TL1	sfr 	08BH
TH0	sfr 	08CH
TH1	sfr 	08DH

CML0	sfr 	0A9H
CML1	sfr 	0AAH
CML2	sfr 	0ABH
CTL0	sfr 	0ACH
CTL1	sfr 	0ADH
CTL2	sfr 	0AEH
CTL3	sfr 	0AFH

TM2IR	sfrb 	0C8H
CTI0	bit	TM2IR.0
CTI1	bit	TM2IR.1
CTI2	bit	TM2IR.2
CTI3	bit	TM2IR.3
CMI0	bit	TM2IR.4
CMI1	bit	TM2IR.5
CMI2	bit	TM2IR.6
T2OV	bit	TM2IR.7

CMH0	sfr 	0C9H
CMH1	sfr 	0CAH
CMH2	sfr 	0CBH
CTH0	sfr 	0CCH
CTH1	sfr 	0CDH
CTH2	sfr 	0CEH
CTH3	sfr 	0CFH

TM2CON	sfr 	0EAH
CTCON	sfr 	0EBH
TML2	sfr 	0ECH
TMH2	sfr 	0EDH

;-----------------------------------------------------------------------------

S0CON	sfrb 	098H
RI	bit	S0CON.0
TI	bit	S0CON.1
RB8	bit	S0CON.2
TB8	bit	S0CON.3
REN	bit	S0CON.4
SM2	bit	S0CON.5
SM1	bit	S0CON.6
SM0	bit	S0CON.7

S0BUF	sfr 	099H

S1CON	sfrb 	0D8H
CR0	bit	S1CON.0
CR1	bit	S1CON.1
AA	bit	S1CON.2
SI	bit	S1CON.3
ST0	bit	S1CON.4
STA	bit	S1CON.5
ENS1	bit	S1CON.6
CR2	bit	S1CON.7

S1STA	sfr 	0D9H
S1DAT	sfr 	0DAH
S1ADR	sfr 	0DBH

;-----------------------------------------------------------------------------

;-----------------------------------------------------------------------------

ADCON	sfr 	0C5H
ADCH	sfr 	0C6H

;-----------------------------------------------------------------------------

STE	sfr 	0EEH
RTE	sfr 	0EFH
PWM0	sfr 	0FCH
PWM1	sfr 	0FDH
PWMP	sfr 	0FEH
T3	sfr 	0FFH

;-----------------------------------------------------------------------------

RESET	label	000H
EXTI0	label	003H
TIMER0	label	00BH
EXTI1	label	013H
TIMER1	label	01BH
SINT	label	023H
I2CBUS	label	02BH
T2CAP0	label	033H
T2CAP1	label	03BH
T2CAP2	label	043H
T2CAP3	label	04BH
ADCONV	label	053H
T2CMP0	label	05BH
T2CMP1	label	063H
T2CMP2	label	06BH
T2OVER	label	073H
