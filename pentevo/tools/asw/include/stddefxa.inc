                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei STDDEFXA.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR- und Bitdefinitionen fÅr die XA-Prozessoren         *
;* 									    *
;*   letzte énderungen : 29. 6.1996                                         *
;*                       18. 8.1998 bei P_51, F0_51 und F1_51 den Postfix   *
;*                                  wieder entfernt, da diese sich mit kei- *
;*                                  nen XA-Bits namensmaessig "beissen"     *
;*                                                                          *
;****************************************************************************

                ifndef  stddefxainc     ; verhindert Mehrfacheinbindung

stddefxainc     equ     1

                if      (MOMCPUNAME<>"XAG3")&&(MOMCPUNAME<>"XAG1")&&(MOMCPUNAME<>"XAG1")
                 fatal  "Falscher Prozessortyp eingestellt: nur XAG1, XAG2 oder XAG3 erlaubt!"
		endif


                if      MOMPASS=1
                 message "Philips-XA-SFR-Definitionen (C) 1996 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; Prozessorkern

cs              port    443h            ; Code-Segment
ds              port    441h            ; Daten-Segment
es              port    442h            ; Extra-Segment

ssel            port    403h            ; Auswahl-Bits, ob [R0..R6] mit ES/DS arbeiten
r0seg           bit     ssel.0
r1seg           bit     ssel.1
r2seg           bit     ssel.2
r3seg           bit     ssel.3
r4seg           bit     ssel.4
r5seg           bit     ssel.5
r6seg           bit     ssel.6
eswen           bit     ssel.7          ; Schreibzugriffe Åber ES erlauben/sperren (Usermode)

pswl            port    400h            ; Prozessorstatus(L):
z               bit     pswl.0          ;  Zero-Flag
n               bit     pswl.1          ;  Negativ-Flag
v               bit     pswl.2          ;  Overflow-Flag
ac              bit     pswl.6          ;  Hilfs-Carry (fÅr DAA)
c               bit     pswl.7          ;  Carry

pswh            port    401h            ; Prozessorstatus(H):
im0             bit     pswh.0          ;  Interrupt-Maske Bit 0
im1             bit     pswh.1          ;  Interrupt-Maske Bit 1
im2             bit     pswh.2          ;  Interrupt-Maske Bit 2
im3             bit     pswh.3          ;  Interrupt-Maske Bit 3
rs0             bit     pswh.4          ;  Register-Bank-Auswahl Bit 0
rs1             bit     pswh.5          ;  Register-Bank-Auswahl Bit 1
tm              bit     pswh.6          ;  CPU im Einzelschritt-Modus
sm              bit     pswh.7          ;  CPU im Supervisor-Mode

psw51           port    402h            ; emuliertes 8051-PSW:
p               bit     psw51.0         ;  ParitÑtsbit
f1              bit     psw51.1         ;  User-Flag 1
v_51            bit     psw51.2         ;  Overflow-Flag
rs0_51          bit     psw51.3         ;  Register-Bank-Auswahl Bit 0
rs1_51          bit     psw51.4         ;  Register-Bank-Auswahl Bit 1
f0              bit     psw51.5         ;  User-Flag 0
ac_51           bit     psw51.6         ;  Hilfs-Carry (fÅr DAA)
c_51            bit     psw51.7         ;  Carry

pcon            port    404h            ; Steuerung Power-Modi
idl             bit     pcon.0          ;  Idle-Modus aktivieren
pd              bit     pcon.1          ;  Powerdown-Modus aktivieren

;----------------------------------------------------------------------------
; Chip-Konfiguration

scr             port    440h            ; Systemkonfiguration
;pz             bit     scr.0           ;  Adre·rÑume auf 64K begrenzen
;cm             bit     scr.1           ;  8051-kompatibles Registermapping
;pt0            bit     scr.2           ;  Vorteiler-Einstellung
;pt1            bit     scr.3

bcr             port    46ah            ; Bus-Konfiguration
;bc0            bit     bcr.0           ;  Einstellung 12/16/20/24 Adre·leitungen
;bc1            bit     bcr.1
;bc2            bit     bcr.2           ;  Einstellung 8/16 Datenleitungen
;busd           bit     bcr.3           ;  Bus komplett abschalten
;waitd          bit     bcr.4           ;  WAIT-Pin ignorieren

btrl            port    468h            ; Einstellung Bus-Timing(L)
;dra0           bit     btrl.0          ;  GesamtlÑnge Lesezyklus
;dra1           bit     btrl.1
;dr0            bit     btrl.2          ;  LÑnge Lesezyklus ohne ALE
;dr1            bit     btrl.3
;dwa0           bit     btrl.4          ;  GesamtlÑnge Schreibzyklus
;dwa1           bit     btrl.5
;dw0            bit     btrl.6          ;  LÑnge Schreibzyklus ohne ALE
;dw1            bit     btrl.7

btrh            port    469h            ; Einstellung Bus-Timing(L)
;cra0           bit     btrl.0          ;  GesamtlÑnge Codelesezyklus
;cra1           bit     btrl.1
;cr0            bit     btrl.2          ;  LÑnge Codelesezyklus ohne ALE
;cr1            bit     btrl.3
;alew           bit     btrl.5          ;  LÑnge ALE-Puls
;wm0            bit     btrl.6          ;  LÑnge WR-Puls
;wm1            bit     btrl.7

;----------------------------------------------------------------------------
; Interrupt-Steuerung

iel             port    426h            ; Interruptfreigabe(L):
ex0             bit     iel.0           ;  externer Interrupt 0
et0             bit     iel.1           ;  Timer 0
ex1             bit     iel.2           ;  externer Interrupt 1
et1             bit     iel.3           ;  Timer 1
et2             bit     iel.4           ;  Timer 2
ea              bit     iel.7           ;  global

ieh             port    427h            ; Interruptfreigabe(H):
eri0            bit     ieh.0           ;  Empfang UART0
eti0            bit     ieh.1           ;  Versand UART0
eri1            bit     ieh.2           ;  Empfang UART1
eti1            bit     ieh.3           ;  Versand UART1

ipa0            port    4a0h            ; InterruptprioritÑten(0):
;px0            bit     ipa0.0          ;  externer Interrupt 0 (4 Bits)
;pt0            bit     ipa0.4          ;  Timer 0 (4 Bits)

ipa1            port    4a1h            ; InterruptprioritÑten(1):
;px1            bit     ipa1.0          ;  externer Interrupt 1 (4 Bits)
;pt1            bit     ipa1.4          ;  Timer 1 (4 Bits)

ipa2            port    4a2h            ; InterruptprioritÑten(2):
;pt2            bit     ipa1.0          ;  Timer 2 (4 Bits)

ipa4            port    4a4h            ; InterruptprioritÑten(4):
;pri0           bit     ipa4.0          ;  Empfang UART0 (4 Bits)
;pti0           bit     ipa4.4          ;  Versand UART0 (4 Bits)

ipa5            port    4a5h            ; InterruptprioritÑten(4):
;pri1           bit     ipa5.0          ;  Empfang UART1 (4 Bits)
;pti1           bit     ipa5.4          ;  Versand UART1 (4 Bits)

swe             port    403h            ; Freigabe Software-Interrupts
swe1            bit     swe.0
swe2            bit     swe.1
swe3            bit     swe.2
swe4            bit     swe.3
swe5            bit     swe.4
swe6            bit     swe.5
swe7            bit     swe.6

swr             port    42ah            ; Auslîsung Software-Interrupts
swr1            bit     swr.0
swr2            bit     swr.1
swr3            bit     swr.2
swr4            bit     swr.3
swr5            bit     swr.4
swr6            bit     swr.5
swr7            bit     swr.6

;----------------------------------------------------------------------------
; Ports

p0              port    430h            ; Datenregister Port 0
ad0             bit     p0.0            ;  Datenbit 0/Adre·bit 4
ad1             bit     p0.1            ;  Datenbit 1/Adre·bit 5
ad2             bit     p0.2            ;  Datenbit 2/Adre·bit 6
ad3             bit     p0.3            ;  Datenbit 3/Adre·bit 7
ad4             bit     p0.4            ;  Datenbit 4/Adre·bit 8
ad5             bit     p0.5            ;  Datenbit 5/Adre·bit 9
ad6             bit     p0.6            ;  Datenbit 6/Adre·bit 10
ad7             bit     p0.7            ;  Datenbit 7/Adre·bit 11

p1              port    431h            ; Datenregister Port 1
a0              bit     p1.0            ;  Adre·leitung 0 (8-Bit-Modus)
wrh             bit     p1.0            ;  Schreibimpuls D8-D15 (16-Bit-Modus)
a1              bit     p1.1            ;  Adre·leitung 1
a2              bit     p1.2            ;  Adre·leitung 2
a3              bit     p1.3            ;  Adre·leitung 3
rxd1            bit     p1.4            ;  Empfangsleitung UART1
txd1            bit     p1.5            ;  Sendeleitung UART1
t2              bit     p1.6            ;  Timer 2 Ein/Ausgang
t2ex            bit     p1.7            ;  Trigger Timer 2

p2              port    432h            ; Datenregister Port 2
ad8             bit     p2.0            ;  Datenbit 8/Adre·bit 12
ad9             bit     p2.1            ;  Datenbit 9/Adre·bit 13
ad10            bit     p2.2            ;  Datenbit 10/Adre·bit 14
ad11            bit     p2.3            ;  Datenbit 11/Adre·bit 15
ad12            bit     p2.4            ;  Datenbit 12/Adre·bit 16
ad13            bit     p2.5            ;  Datenbit 13/Adre·bit 17
ad14            bit     p2.6            ;  Datenbit 14/Adre·bit 18
ad15            bit     p2.7            ;  Datenbit 15/Adre·bit 19

p3              port    433h            ; Datenregister Port 3
rxd0            bit     p3.0            ;  Empfangsleitung UART0
txd0            bit     p3.1            ;  Sendeleitung UART0
int0            bit     p3.2            ;  externer Interrupt 0
int1            bit     p3.3            ;  externer Interrupt 1
t0              bit     p3.4            ;  Timer 0 Ein/Ausgang
t1              bit     p3.5            ;  Timer 1 Ein/Ausgang
wr              bit     p3.6            ;  Schreibimpuls D0-7
rd              bit     p3.7            ;  Leseimpuls D0-15

p0cfga          port    470h            ; Konfigurationsbits A fÅr Port 0
p1cfga          port    471h            ; Konfigurationsbits A fÅr Port 1
p2cfga          port    472h            ; Konfigurationsbits A fÅr Port 2
p3cfga          port    473h            ; Konfigurationsbits A fÅr Port 3

p0cfgb          port    4f0h            ; Konfigurationsbits B fÅr Port 0
p1cfgb          port    4f1h            ; Konfigurationsbits B fÅr Port 1
p2cfgb          port    4f2h            ; Konfigurationsbits B fÅr Port 2
p3cfgb          port    4f3h            ; Konfigurationsbits B fÅr Port 3

;----------------------------------------------------------------------------
; Timer

tl0             port    450h            ; ZÑhlregister Timer 0
th0             port    451h
rtl0            port    454h            ; Reload Timer 0
rth0            port    455h

tl1             port    452h            ; ZÑhlregister Timer 1
th1             port    453h
rtl1            port    456h            ; Reload Timer 1
rth1            port    457h

tmod            port    45ch            ; Modusregister Timer 0/1
;t0_m0          bit     tmod.0          ;  Betriebsart Timer 0
;t0_m1          bit     tmod.1
;ct_t0          bit     tmod.2          ;  Timer 0 als Timer oder ZÑhler
;t0_gate        bit     tmod.3          ;  Freigabe Timer 0
;t1_m0          bit     tmod.4          ;  Betriebsart Timer 1
;t1_m1          bit     tmod.5
;ct_t1          bit     tmod.6          ;  Timer 1 als Timer oder ZÑhler
;t1_gate        bit     tmod.7          ;  Freigabe Timer 1

tcon            port    410h            ; Timer-Steuerregister 0/1
it0             bit     tcon.0          ;  Flanken/Zustandstriggerung Interrupt 0
ie0             bit     tcon.1          ;  Flankendetektor Interrupt 0
it1             bit     tcon.2          ;  Flanken/Zustandstriggerung Interrupt 1
ie1             bit     tcon.3          ;  Flankendetektor Interrupt 1
tr0             bit     tcon.4          ;  Timer 0 einschalten/stoppen
tf0             bit     tcon.5          ;  öberlauf Timer 0 ?
tr1             bit     tcon.6          ;  Timer 1 einschalten/stoppen
tf1             bit     tcon.7          ;  öberlauf Timer 1 ?

tstat           port    411h            ; Timer-Status 0/1
t0oe            bit     tstat.0         ;  Takt von Timer 0 an T0 ausgeben
t1oe            bit     tstat.1         ;  Takt von Timer 1 an T1 ausgeben

tl2             port    458h            ; ZÑhlregister Timer 2
th2             port    459h

t2capl          port    45ah            ; Fangwert Timer 2
t2caph          port    45bh

t2mod           port    419h            ; Modus Timer 2
dcen            bit     t2mod.0         ;  ZÑhlrichtung Timer 2 auf/durch T2EX bestimmt
t2oe            bit     t2mod.1         ;  Takt von Timer 2 an T2 ausgeben
tclk1           bit     t2mod.4         ;  UART1 benutzt Timer2 statt Timer 1 zum Senden
rclk1           bit     t2mod.5         ;  UART1 benutzt Timer2 statt Timer 1 zum Empfangen

t2con           port    418h            ; Timer-Steurregister 2
cp_rl2          bit     t2con.0         ;  Timer 2 Fang/Neuladen ?
ct_t2           bit     t2con.1         ;  Timer 2 als Timer oder ZÑhler
tr2             bit     t2con.2         ;  Timer 2 einschalten/stoppen
exen2           bit     t2con.3         ;  Capture/Reload Åber T2EX erlauben
tclk0           bit     t2con.4         ;  UART0 benutzt Timer2 statt Timer 1 zum Senden
rclk0           bit     t2con.5         ;  UART0 benutzt Timer2 statt Timer 1 zum Empfangen
exf2            bit     t2con.6         ;  Capture/Reload aufgetreten ?
tf2             bit     t2con.7         ;  öberlauf Timer 2 ?

;----------------------------------------------------------------------------
; Watchdog

wdcon           port    41fh            ; Konfiguration Watchdog
wdtof           bit     wdcon.1         ;  Timeout aufgetreten ?
wdrun           bit     wdcon.2         ;  Watchdog einschalten
pre0            bit     wdcon.5         ;  Vorteiler
pre1            bit     wdcon.6
pre2            bit     wdcon.7

wdl             port    45fh            ; Reload-Wert

wfeed1          port    45dh            ; RÅcksetzregister 1 (a5h)
wfeed2          port    45eh            ; RÅcksetzregister 2 (5ah)

;----------------------------------------------------------------------------
; serielle Ports

s0buf           port    460h            ; Datenregister UART0

s0addr          port    461h            ; Slave-Adresse UART0
s0aden          port    462h            ; Adre·maske UART0

s0stat          port    421h            ; Status UART0
stint0          bit     s0stat.0        ;  Interruptfreigabe UART0
oe0             bit     s0stat.1        ;  EmpfÑngerÅberlauf UART0
br0             bit     s0stat.2        ;  Break auf UART0 empfangen?
fe0             bit     s0stat.3        ;  kein gÅltiges Stopbit auf UART0 ?

s0con           port    420h            ; Steuerung UART0
ri_0            bit     s0con.0         ;  Zeichen auf UART0 empfangen ?
ti_0            bit     s0con.1         ;  Zeichen auf UART0 verschickt ?
rb8_0           bit     s0con.2         ;  auf UART0 empfangenes, 9. Bit
tb8_0           bit     s0con.3         ;  auf UART0 verschicktes, 9. Bit
ren_0           bit     s0con.4         ;  EmpfÑnger UART0 freigeben
sm2_0           bit     s0con.5         ;  Multiprozessormodus auf UART0 freigeben
sm1_0           bit     s0con.6         ;  Moduseinstellung UART0
sm0_0           bit     s0con.7

s1stat          port    421h            ; Status UART1
stint1          bit     s1stat.0        ;  Interruptfreigabe UART1
oe1             bit     s1stat.1        ;  EmpfÑngerÅberlauf UART1
br1             bit     s1stat.2        ;  Break auf UART1 empfangen?
fe1             bit     s1stat.3        ;  kein gÅltiges Stopbit auf UART1 ?

s1con           port    424h            ; Steuerung UART1
ri_1            bit     s1con.0         ;  Zeichen auf UART1 empfangen ?
ti_1            bit     s1con.1         ;  Zeichen auf UART1 verschickt ?
rb8_1           bit     s1con.2         ;  auf UART1 empfangenes, 9. Bit
tb8_1           bit     s1con.3         ;  auf UART1 verschicktes, 9. Bit
ren_1           bit     s1con.4         ;  EmpfÑnger UART1 freigeben
sm2_1           bit     s1con.5         ;  Multiprozessormodus auf UART1 freigeben
sm1_1           bit     s1con.6         ;  Moduseinstellung UART1
sm0_1           bit     s1con.7

s1buf           port    464h            ; Datenregister UART1

s1addr          port    465h            ; Slave-Adresse UART1
s1aden          port    466h            ; Adre·maske UART1


;----------------------------------------------------------------------------
; Vektoren

vec_reset       label   0000h           ; Resetadresse
vec_bkpt        label   0004h           ; Breakpoint-Befehl
vec_trace       label   0008h           ; Einzelschrittverfolgung
vec_stkover     label   000ch           ; StapelÅberlauf
vec_divzero     label   0010h           ; Division durch 0
vec_reti        label   0014h           ; Privilegverletzung (RETI im User-Mode)
vec_trap0       label   0040h           ; TRAP 0
vec_trap1       label   0044h           ; TRAP 1
vec_trap2       label   0048h           ; TRAP 2
vec_trap3       label   004ch           ; TRAP 3
vec_trap4       label   0050h           ; TRAP 4
vec_trap5       label   0054h           ; TRAP 5
vec_trap6       label   0058h           ; TRAP 6
vec_trap7       label   005ch           ; TRAP 7
vec_trap8       label   0060h           ; TRAP 8
vec_trap9       label   0064h           ; TRAP 9
vec_trap10      label   0068h           ; TRAP 10
vec_trap11      label   006ch           ; TRAP 11
vec_trap12      label   0070h           ; TRAP 12
vec_trap13      label   0074h           ; TRAP 13
vec_trap14      label   0078h           ; TRAP 14
vec_trap15      label   007ch           ; TRAP 15

vec_ie0         label   0080h           ; externer Interrupt 0
vec_tf0         label   0084h           ; Interrupt durch Timer 0
vec_ie1         label   0088h           ; externer Interrupt 1
vec_tf1         label   008ch           ; Interrupt durch Timer 1
vec_tf2         label   0090h           ; Interrupt durch Timer 2
vec_ri0         label   00a0h           ; Empfang auf UART0
vec_ti0         label   00a4h           ; Sendung auf UART0
vec_ri1         label   00a8h           ; Empfang auf UART1
vec_ti1         label   00ach           ; Sendung auf UART1

vec_swr1        label   0100h           ; Software-Interrupt 1
vec_swr2        label   0104h           ; Software-Interrupt 2
vec_swr3        label   0108h           ; Software-Interrupt 3
vec_swr4        label   010ch           ; Software-Interrupt 4
vec_swr5        label   0110h           ; Software-Interrupt 5
vec_swr6        label   0114h           ; Software-Interrupt 6
vec_swr7        label   0118h           ; Software-Interrupt 7

;----------------------------------------------------------------------------
; Speicheradressen

irom            label   0               ; Beginn internes ROM
                switch  MOMCPUNAME      ; Ende internes ROM
                case    "XAG3"
iromend          label   7fffh          ; XA-G3: 32K ROM
                case    "XAG2"
iromend          label   3fffh          ; XA-G2: 16K ROM
                case    "XAG1"
iromend          label   1fffh          ; XA-G1:  8K ROM
                endcase

iram            equ     0,data          ; Beginn internes RAM
iramend         equ     1ffh,data       ; Ende internes RAM: immer 512 Byte

                endif

                restore                 ; wieder erlauben


