                save
		listing off   ; no listing over this file

;****************************************************************************
;*                                                                          *
;*   AS 1.42 - File REG251.INC                                              *
;*   								            *
;*   Purpose : contains SFR/bit definions for MCS-251 Processors            *
;* 									    *
;* $Id: reg251.inc,v 1.2 2006/05/01 09:24:30 alfred Exp $                  *
;*                                                                          *
;* $Log: reg251.inc,v $
;* Revision 1.2  2006/05/01 09:24:30  alfred
;* - correct location of SPH
;*
;* Revision 1.1  2003/11/06 02:56:33  alfred
;* - recreated
;*
;* Revision 1.2  2003/06/22 13:18:05  alfred
;* - translated to english, added 251T registers
;*                                                     *
;*                                                                          *
;****************************************************************************

                ifndef  reg251inc       ; avoids multiple inclusion

reg251inc       equ     1

                if      (MOMCPUNAME<>"80C251")&&(MOMCPUNAME<>"80C251T")
                 fatal  "Falscher Prozessortyp eingestellt: nur 80C251(T) erlaubt!"
		endif


                if      MOMPASS=1
                 message "MCS-251-SFR-Definitionen (C) 2003 Alfred Arnold"
		 message "binde \{MOMCPUNAME}-SFRs ein"
		endif

;----------------------------------------------------------------------------
; CPU Core:
; ACC = A = R11
; B = R10
; SP/SPH = SPX = DR60
; DPL/DPH/DPXL = DPX = DR56
; CY is being used by AS itself

ACC             port    0e0h            ; accumulator

B               port    0f0h            ; B register

PSW             port    0d0h            ; 8051-compatible PSW
CARRY           bit     PSW.7           ; carry
AC              bit     PSW.6           ; auxiliary carry
F0              bit     PSW.5           ; flag 0
RS1             bit     PSW.4           ; register bank select
RS0             bit     PSW.3
OV              bit     PSW.2           ; overflow flag
UD              bit     PSW.1           ; user flag
P               bit     PSW.0           ; parity flag

PSW1            port    0d1h            ; extended PSW (bits 2-4,6-7 as in PSW)
N               bit     PSW1.5          ; negatives result?
Z               bit     PSW1.1          ; zero result?

SP              port    81h             ; stack pointer (bits 0..7)
SPH             port    0beh            ; stack pointer (bits 8..15)
DPL             port    082h            ; data pointer (bits 0..7)
DPH             port    083h            ; data pointer (bits 8..15)
DPXL            port    084h            ; data pointer (bits 15..23)

PCON            port    087h            ; power down modes etc.
SMOD1           bit     PCON.7          ; doubles baud rate in modes 1..3
SMOD0           bit     PCON.6          ; toggles SCON.7 as FE/SM0
POF             bit     PCON.4          ; power outage ?
GF1             bit     PCON.3          ; general flag 1
GF0             bit     PCON.2          ; general flag 0
PD              bit     PCON.1          ; switch to powerdown mode
IDL             bit     PCON.0          ; switch to idle mode

IE              port    0a8h            ; interrupt enables (should be IE0,
                                        ; but that is already occupied)
EA              bit     IE.7            ; general interrupt mask
EC              bit     IE.6            ; enable PCA interrupts
ET2             bit     IE.5            ; enable timer 2 interrupts
ES              bit     IE.4            ; enable serial port interrupts
ET1             bit     IE.3            ; enable timer 1 interrupts
EX1             bit     IE.2            ; enable external interrupt 1
ET0             bit     IE.1            ; enable timer 0 interrupts
EX0             bit     IE.0            ; enable external interrupt 0

IPH0            port    0b7h            ; interrupt priorities
IPL0            port    0b8h

                if      MOMCPUNAME="80C251T"
IPH1             port    0b3h
IPL1             port    0b2h
IE01             port    0b1h           ; It's called IE1 in the manual but
                                        ; that's the name of a bit elsewhere;
                                        ; Should probably find a better name
                                        ; for this one, or change IE1
		endif


;----------------------------------------------------------------------------
; ports with extra functions:

P0              port    080h            ; port 0

P1              port    090h            ; port 1
T2              bit     P1.0            ; in/output timer 2
T2EX            bit     P1.1            ; trigger timer 2
ECI             bit     P1.2            ; external PCA clock
CEX0            bit     P1.3            ; in/output PCA module 0
CEX1            bit     P1.4            ; in/output PCA module 1
CEX2            bit     P1.5            ; in/output PCA module 2
CEX3            bit     P1.6            ; in/output PCA module 3
CEX4            bit     P1.7            ; in/output PCA module 4

P2              port    0a0h            ; port 2

P3              port    0b0h            ; port 3
RXD             bit     P3.0            ; serial receive line
TXD             bit     P3.1            ; serial transmit lie
INT0            bit     P3.2            ; external interrupt 0
INT1            bit     P3.3            ; external interrupt 1
T0              bit     P3.4            ; clock input timer 0
T1              bit     P3.5            ; clock input timer 1
WR              bit     P3.6            ; read line
RD              bit     P3.7            ; write line

;----------------------------------------------------------------------------
; serial port:

SCON            port    098h            ; configuration
FE              bit     SCON.7          ; wrong stop bit ?
SM0             bit     SCON.7          ; mode selection
SM1             bit     SCON.6
SM2             bit     SCON.5
REN             bit     SCON.4          ; enable receiver
TB8             bit     SCON.3          ; transmit data bit 8
RB8             bit     SCON.2          ; receive data bit 8
TI              bit     SCON.1          ; character sent completely?
RI              bit     SCON.0          ; character received completely?

SBUF            port    099h            ; data register
SADEN           port    0b9h            ; slave address mask
SADDR           port    0a9h            ; slave address

                if      MOMCPUNAME="80C251T"
;----------------------------------------------------------------------------
; second serial port:

SADEN1           port    0bah            ; slave address mask
SADDR1           port    0aah            ; slave address
SCON1            port    09ah            ; configuration, similar to SCON
SBUF1            port    09bh            ; data register
BGCON            port    08fh
                endif

;----------------------------------------------------------------------------
; counters/watchdog:

TL0             port    08ah            ; counter value counter 0
TH0             port    08ch
TL1             port    08bh            ; counter value counter 1
TH1             port    08dh

TCON            port    088h            ; counter 0/1 control
TF1             bit     TCON.7          ; timer 1 overflow?
TR1             bit     TCON.6          ; start/stop timer 1
TF0             bit     TCON.5          ; timer 0 overflow?
TR0             bit     TCON.4          ; start/stop timer 0
IE1             bit     TCON.3          ; external interrupt 1 occured?
IT1             bit     TCON.2          ; edge/level trigger ext. interrupt 1
IE0             bit     TCON.1          ; external interrupt 0 occured?
IT0             bit     TCON.0          ; edge/level trigger ext. Interrupt 0

TMOD            port    089h            ; counter 0/1 control/mode
M00             bit     TMOD.0          ; timer 0 mode
M10             bit     TMOD.1
CT0             bit     TMOD.2          ; counter/timer select timer 0
GATE0           bit     TMOD.3          ; enable mode timer 0
M01             bit     TMOD.4          ; timer 1 mode
M11             bit     TMOD.5
CT1             bit     TMOD.6          ; counter/timer select timer 1
GATE1           bit     TMOD.7          ; enable mode timer 1


TL2             port    0cch            ; counter value counter 2
TH2             port    0cdh

T2CON           port    0c8h            ; counter 2 control
TF2             bit     T2CON.7         ; timer 2 overflow?
EXF2            bit     T2CON.6         ; edge occured on T2EX?
RCLK            bit     T2CON.5         ; use timer 2 or 1 for serial receive clock?
TCLK            bit     T2CON.4         ; use timer 2 or 1 for serial transmit clock?
EXEN2           bit     T2CON.3         ; enable capture via T2EX
TR2             bit     T2CON.2         ; start/stop timer 2
CT2             bit     T2CON.1         ; counter/timer select timer 2
CPRL2           bit     T2CON.0         ; capture or reload via T2EX

T2MOD           port    0c9h            ; counter 2 control/mode
T2OE            bit     T2MOD.1         ; enable timer 2 output on T2
DCEN            bit     T2MOD.0         ; counting direction timer 2

RCAP2L          port    0cah            ; reload/capture value timer 2
RCAP2H          port    0cbh

WDTRST          port    0a6h            ; reset watchdog
WCON            port    0a7h            ; watchdog control

;----------------------------------------------------------------------------
; counter field:

CCON            port    0d8h            ; control register
CF              bit     CCON.7          ; PCA overflow?
CR              bit     CCON.6          ; start/stop PCA timer
CCF4            bit     CCON.4          ; did PCA module 4..0 trigger ?
CCF3            bit     CCON.3
CCF2            bit     CCON.2
CCF1            bit     CCON.1
CCF0            bit     CCON.0

CMOD            port    0d9h            ; mode register
CIDL            bit     CMOD.7          ; turn off PCA during idle mode
WDTE            bit     CMOD.6          ; enable watchdog output on PCA module 4
CPS1            bit     CMOD.2          ; PCA clock select
CPS0            bit     CMOD.1
ECF             bit     CMOD.0          ; interrupt enable

CL              port    0e9h            ; counter value
CH              port    0f9h

__defpcamodule  macro   Offset,NUM
CCAPM{NUM}      port    0d0h+Offset     ; mode register
ECOM{NUM}       bit     CCAPM0.6        ; enable comparator module x
CAPP{NUM}       bit     CCAPM0.5        ; enable capture function module x (pos. edge)
CAPN{NUM}       bit     CCAPM0.4        ; enable capture function module x (neg. edge)
MAT{NUM}        bit     CCAPM0.3        ; enable equality interrupt module x
TOG{NUM}        bit     CCAPM0.2        ; enable high-speed output mode module x
PWM{NUM}        bit     CCAPM0.1        ; enable PWM modus module x
ECCF{NUM}       bit     CCAPM0.0        ; enable interrupts via CCFx
CCAP{NUM}L      port    0e0h+Offset     ; compare/capture value module x
CCAP{NUM}H      port    0f0h+Offset
                endm

                __defpcamodule 0ah,"0"
                __defpcamodule 0bh,"1"
                __defpcamodule 0ch,"2"
                __defpcamodule 0dh,"3"
                __defpcamodule 0eh,"4"

;----------------------------------------------------------------------------

                endif

                restore                 ; enable listing again
