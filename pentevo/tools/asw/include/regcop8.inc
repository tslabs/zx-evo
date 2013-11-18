                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REGCOP8.INC                                            *
;*   								            *
;*   Sinn : enthÑlt Registerdefinitionen fÅr die COP8-Familie               *
;* 									    *
;*   letzte énderungen :  2. 9.1996                                         *
;*                        8. 2.1997 MOmPass=1...                            *
;*                                                                          *
;****************************************************************************

                ifndef  regcop8inc      ; verhindert Mehrfacheinbindung

regcop8inc      equ     1

                if      MOMPASS=1
                 switch  MOMCPUNAME
                 case    "COP87L84"
                  message "including COP87L84-registers"
                 elsecase
                  fatal   "invalid processor type: only COP87L84 allowed!"
                 endcase
                endif

;----------------------------------------------------------------------------
; Prozessorkern

__REG           set     0
                rept    10
R{"\{__REG}"}   equ __REG+0xf0
__REG           set     __REG+1,data
                endm
                rept    6
R1{"\{__REG-10}"} equ __REG+0xf0
__REG           set     __REG+1,data
                endm

psw             sfr     0xef            ; Flags
gie             equ      0              ; globale Interruptfreigabe
exen            equ      1              ; externe Interruptfreigabe
busy            equ      2              ; Microwire busy ?
expnd           equ      3              ; externer Interrupt angefordert ?
t1ena           equ      4              ; Timer 1 Interruptfreigabe
t1pnda          equ      5              ; Timer 1 Interrupt angefordert ?
c               equ      6              ; Carry
hc              equ      7              ; Halfcarry
x               sfr     0xfc            ; X-Register
sp              sfr     0xfd            ; Stackpointer
b               sfr     0xfe            ; B-Register

;----------------------------------------------------------------------------
; Peripherie-Steuerregister

cntrl           sfr     0xee            ; globale Steuerung
sl0             equ      0              ; Taktteiler Microwire
sl1             equ      1
iedg            equ      2              ; Flankenwahl externer Interrupt
msel            equ      3              ; G4/G5 fÅr Microwire benutzen
t1c0            equ      4              ; Timer 1 Interrupt/Start-Stop
t1c1            equ      5              ; Moduswahl Timer 1
t1c2            equ      6
t1c3            equ      7

icntrl          sfr     0xe8            ; Fortsetzung...
t1enb           equ      0              ; Freigabe Fang-Interrupt Timer 1
t1pndb          equ      1              ; Fang-Interrupt Timer 1 aufgetreten ?
uwen            equ      2              ; Freigabe MicroWire-Interrupt
uwpnd           equ      3              ; MicroWire-Interrupt aufgetreten ?
t0en            equ      4              ; Freigabe Interrupt Timer 0
t0pndb          equ      5              ; Timer 0-Interrupt aufgetreten ?
lpen            equ      6              ; Freigabe Port L-Interrupt

;----------------------------------------------------------------------------
; Timer

t1rblo          sfr     0xe6            ; Autoload-Wert B Timer 1
t1rbhi          sfr     0xe7

tmr1lo          sfr     0xea            ; ZÑhlwert Timer 0
tmr1hi          sfr     0xeb

t1ralo          sfr     0xec            ; Autoload-Wert A Timer 1
t1rahi          sfr     0xed

;----------------------------------------------------------------------------
; PWM

pscal           sfr     0xa0            ; Vorteiler PWM

rlon            sfr     0xa1            ; On-Anteil PWM

pwmcon          sfr     0xa2            ; Steuerregister PWM
pwen0           equ      0              ; PWM0 auf I/O-Port geben
pwen1           equ      1              ; PWM1 auf I/O-Port geben
pwon            equ      2              ; PWM starten/stoppen
pwmd            equ      3              ; PWM-Modus
pwie            equ      4              ; Interrupt-Freigabe PWM
pwpnd           equ      5              ; Interrupt PWM aufgetreten ?
esel            equ      6              ; Flankenwahl PWM

;----------------------------------------------------------------------------
; MicroWire-Interface

wkedg           sfr     0xc8
wken            sfr     0xc9
wkpnd           sfr     0xca

sior            sfr     0xe9            ; Schieberegister

;----------------------------------------------------------------------------
; CAN Interface

txd1            sfr     0xb0            ; Sendedaten (Byte 1,3,5,7,...)
txd2            sfr     0xb1            ; Sendedaten (Byte 2,4,6,8,...)

tdlc            sfr     0xb2            ; SendelÑngen-/Identifier(L)-Register
tdlc0           equ      0              ; SendelÑnge
tdlc1           equ      1
tdlc2           equ      2
tdlc3           equ      3
tid0            equ      4              ; Sendeidentifikation (Bit 0..3)
tid1            equ      5
tid2            equ      6
tid3            equ      7

tid             sfr     0xb3            ; Sendeidentifikationsregister
tid4            equ      0              ; Sendeidentifikation (Bit 4..10)
tid5            equ      1
tid6            equ      2
tid7            equ      3
tid8            equ      4
tid9            equ      5
tid10           equ      6
trtr            equ      7              ; Senderahmen remote ?

rxd1            sfr     0xb4            ; Empfangsdaten (Byte 1,3,5,7,...)
rxd2            sfr     0xb5            ; Empfangsdaten (Byte 2,4,6,8,...)

ridl            sfr     0xb6            ; EmpfangslÑngen/Identifier(L)-Register
rdlc0           equ      0              ; EmpfangslÑnge
rdlc1           equ      1
rdlc2           equ      2
rdlc3           equ      3
rid0            equ      4              ; Empfangsidentifikation (Bit 0..3)
rid1            equ      5
rid2            equ      6
rid3            equ      7

rid             sfr     0xb7            ; Empfangsidentifikationsregister
rid4            equ      0              ; Empfangsidentifikation (Bit 4..10)
rid5            equ      1
rid6            equ      2
rid7            equ      3
rid8            equ      4
rid9            equ      5
rid10           equ      6

cscal           sfr     0xb8            ; Vorteiler CAN-Takt

ctim            sfr     0xb9            ; CAN-Bus-Timing-Register
ps0             equ      2              ; Phase Segment
ps1             equ      3
ps2             equ      4
pps0            equ      5              ; Propagation Segment
pps1            equ      6
pps2            equ      7

cbus            sfr     0xba            ; CAN-Bus-Timing-Register
fmod            equ      1              ; Fault Confinement Mode
rxred0          equ      2              ; Referenzspannung an Rx0 anlegen
rxref1          equ      3              ; Referenzspannung an Tx0 anlegen
txen0           equ      4              ; TxD Ausgangstreiber freigeben
txen1           equ      5
riaf            equ      6              ; Empfangsfilter freigeben

tcntl           sfr     0xbb            ; CAN-Bus-Steuer/Statusregister
txss            equ      0              ; Sender starten/stoppen
rie             equ      1              ; Freigabe Empfangs-Interrupt
tie             equ      2              ; Freigabe Sende-Interrupt
ceie            equ      3              ; Freigabe CAN-Interrupt
rerr            equ      4              ; Empfangsfehler
terr            equ      5              ; Sendefehler
ns0             equ      6              ; Knoten-Status
ns1             equ      7

rtstat          sfr     0xbc            ; CAN-Bus-Sender/EmpfÑngerstatus
rbf             equ      0              ; Empfangspuffer voll ?
rcv             equ      1              ; Empfang lÑuft ?
rfv             equ      2              ; empfangener Rahmen gÅltig ?
rorn            equ      3              ; EmpfÑngerÅberlauf ?
rold            equ      4              ; EmpfÑngerrahmenÅberlauf ?
rrtr            equ      5              ; Remote-Bit in empfangenem Rahmen gesetzt ?
txpnd           equ      6              ; Sendung lÑuft ?
tbe             equ      7              ; Sendepuffer leer ?

tec             sfr     0xbd            ; SendefehlerzÑhler
rec             sfr     0xbe            ; EmpfangsfehlerzÑhler

;----------------------------------------------------------------------------
; Komparatoren

cmpsl           sfr     0xd3            ; Steuerregister Komparatoren
cmp1en          equ      1              ; Komparator 1 aktivieren
cmp1rd          equ      2              ; Ausgangswert Komparator 1
cmp1oe          equ      3              ; Ausgang Komparator 1 herausgeben
cmp2en          equ      4              ; Komparator 2 aktivieren
cmp2rd          equ      5              ; Ausgangswert Komparator 2
cmp2oe          equ      6              ; Ausgang Komparator 2 herausgeben
cmp2sel         equ      7              ; Ausgabe Komparator 2 auf L3/L5

;----------------------------------------------------------------------------
; Ports

portld          sfr     0xd0            ; Ausgaberegister Port L
portlc          sfr     0xd1            ; Konfigurationsregister Port L
portlp          sfr     0xd2            ; Leseregister Port L

portgd          sfr     0xd4            ; Ausgaberegister Port G
portgc          sfr     0xd5            ; Konfigurationsregister Port G
portgp          sfr     0xd6            ; Leseregister Port G

portd           sfr     0xdc            ; Ausgaberegister Port D

;----------------------------------------------------------------------------
; Vektoradressen

;----------------------------------------------------------------------------
; Speicheradressen

iram            sfr     0x00            ; Bereich internes RAM
iramend         sfr     0x2f

;----------------------------------------------------------------------------

                endif

                restore         ; Listing wieder an


