		save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF37.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR- und Bitdefinitionen fÅr die TMS370-Prozessoren     *
;* 									    *
;*   letzte énderungen : 12. 5.1993                                         *
;*									    *
;****************************************************************************

		ifndef  stddef37inc     ; verhindert Mehrfacheinbindung

stddef37inc     equ     1

                if      (MOMCPU<>57720848)&&(MOMCPU<>57720864)&&(MOMCPU<>57720880)&&(MOMCPU<>57720896)&&(MOMCPU<>57720912)
		 fatal  "Falscher Prozessortyp eingestellt: nur 370C010, 370C020, 370C030, 370C040 oder 370C050 erlaubt!"
		endif


                if      MOMPASS=1
		 message "TMS370-Register-Definitionen (C) 1994 Alfred Arnold"
		 message "binde \{MOMCPU}-SFRs ein"
		endif

; Das mit diesen langen Namen ist ja wohl nicht ernst gemeint ???

;----------------------------------------------------------------------------
; Systemsteuerung

sccr0		equ	p010
cold_start	dbit	7,sccr0	; Kaltstart ?
osc_power	dbit	6,sccr0	; Stromaufnahme Oszillator reduzieren
pf_auto_wait	dbit	5,sccr0 ; Wartezyklen fÅr externe Peripherie
osc_flt_flag	dbit	4,sccr0 ; Fehler in Oszillatorschaltung ?
mc_pin_wpo	dbit	3,sccr0 ; Spannung an MC hoch genug ?
mc_pin_data	dbit	2,sccr0	; Zustand des MC-Anschlusses
up_uc_mode	dbit	0,sccr0	; Mikroprozessor/Mikrokontroller-Modus

sccr1		equ	p011
auto_wait_disable dbit	4,sccr1	; Wartezyklen fÅr externen Speicher
memory_disable	dbit	2,sccr1	; int. ROM abschalten

sccr2		equ	p012
halt_standby	dbit	7,sccr2 ; Halt oder /Standby-Modus
pwrdwn_idle	dbit	6,sccr2 ; /Idle oder Halt/Standby-Modus
bus_stest	dbit	4,sccr2	; Bus-Selbsttest ??
cpu_stest	dbit	3,sccr2 ; Prozessor-Selbsttest ??
int1_nmi	dbit	1,sccr2	; Interrupt 1 maskierbar/nicht maskierbar
privilege_disable dbit	0,sccr2	; privilegierten Modus verlassen

;----------------------------------------------------------------------------
; digitaler IO

aport1		equ	p020
aport2		equ	p021
adata		equ	p022	; Port A Datenregister
adir		equ	p023	; Port A Datenrichtungsregister

                if      (MOMCPU=370C020h)||(MOMCPU>370C030h)
bport1		 equ	p024
bport2		 equ	p025
bdata		 equ	p026	; Port B Datenregister
bdir		 equ	p027	; Port B Datenrichtungsregister
		endif

                if      (MOMCPU=370C020h)||(MOMCPU=370C050h)
cport1		 equ	p028
cport2		 equ	p029
cdata		 equ	p02a	; Port C Datenregister
cdir		 equ	p02b	; Port C Datenrichtungsregister
		endif

dport1		equ	p02c
dport2		equ	p02d
ddata		equ	p02e	; Port D Datenregister
ddir		equ	p02f	; Port D Datenrichtungsregister

;----------------------------------------------------------------------------
; Interrupts

int1		equ	p017
int1_enable	dbit	0,int1	; Freigabe externer Interrupt 1
int1_priority	dbit	1,int1	; PrioritÑt externer Interrupt 1
int1_polarity	dbit	2,int1	; PolaritÑt externer Interrupt 1
int1_pin_data	dbit	6,int1	; Zustand Interrupt 1-Pin
int1_flag	dbit	7,int1	; externer Interrupt 1 aufgetreten

int2		equ	p018
int2_enable	dbit	0,int2	; Freigabe externer Interrupt 2
int2_priority	dbit	1,int2	; PrioritÑt externer Interrupt 2
int2_polarity	dbit	2,int2	; PolaritÑt externer Interrupt 2
int2_data_out	dbit	3,int2	; Ausgabewert Interrupt 2-Pin
int2_data_dir	dbit	4,int2	; Datenrichtung Interrupt 2-Pin
int2_pin_data	dbit	6,int2	; Zustand Interrupt 2-Pin
int2_flag	dbit	7,int2	; externer Interrupt 2 aufgetreten

int3		equ	p019
int3_enable	dbit	0,int3	; Freigabe externer Interrupt 3
int3_priority	dbit	1,int3	; PrioritÑt externer Interrupt 3
int3_polarity	dbit	2,int3	; PolaritÑt externer Interrupt 3
int3_data_out	dbit	3,int3	; Ausgabewert Interrupt 3-Pin
int3_data_dir	dbit	4,int3	; Datenrichtung Interrupt 3-Pin
int3_pin_data	dbit	6,int3	; Zustand Interrupt 3-Pin
int3_flag	dbit	7,int3	; externer Interrupt 3 aufgetreten

;----------------------------------------------------------------------------
; (E)EPROM

deectl		equ	p01a
ee_busy		dbit	7,deectl ; EEPROM nicht zugreifbar
ap		dbit	2,deectl ; Blockprogrammierung freigeben
w1w0		dbit	1,deectl ; Einsen oder Nullen programmieren ?
ee_exe		dbit	0,deectl ; Vorgang ausfÅhren

epctl0		equ	p01c
ep0_busy	dbit	7,epctl0 ; EPROM Teil 1 nicht zugreifbar
ep0_vpps	dbit	6,epctl0 ; Brennspannung freigeben
ep0_w0		dbit	1,epctl0 ; Schreiben von Nullen freigeben
ep0_exe		dbit	0,epctl0 ; Schreibvorgang ausfÅhren

epctl1		equ	p01e
ep1_busy	dbit	7,epctl1 ; EPROM Teil 2 nicht zugreifbar
ep1_vpps	dbit	6,epctl1 ; Brennspannung freigeben
ep1_w0		dbit	1,epctl1 ; Schreiben von Nullen freigeben
ep1_exe		dbit	0,epctl1 ; Schreibvorgang ausfÅhren

;----------------------------------------------------------------------------
; Timer 1

t1ctl1		equ	p049	; steuern auch Watchdog
t1ctl2		equ	p04a

                if      MOMCPU<>370C030h
t1cntrh		 equ	p040	; ZÑhlerregister
t1cntrl		 equ	p041

t1ch		 equ	p042	; Vergleichsregister
t1cl		 equ	p043

t1cch		 equ	p044	; Vergleichs/Fangregister
t1ccl		 equ	p045

t1_input_select0 dbit	0,t1ctl1 ; Taktquelle
t1_input_select1 dbit	1,t1ctl1
t1_input_select2 dbit	2,t1ctl1

t1_sw_reset	 dbit	0,t1ctl2 ; ZÑhler zurÅcksetzen
t1_ovrfl_int_flag dbit	3,t1ctl2 ; Overflow aufgetreten ?
t1_ovrfl_int_ena dbit	4,t1ctl2 ; Overflow-Interrupts freigeben

t1ctl3		 equ	p04b
t1c1_int_ena	 dbit	0,t1ctl3 ; Interrupts Vergleichsregister freigeben
t1c2_int_ena	 dbit	1,t1ctl3 ; Interrupts Vergleichs(/Fang)register freigeben
t1edge_int_ena	 dbit	2,t1ctl3 ; Interrupts bei Flanke an T1IC/CR freigeben
t1c1_int_flag	 dbit	5,t1ctl3 ; Interrupt durch Vergleichsregister ?
t1c2_int_flag	 dbit	6,t1ctl3 ; Interrupt durch Vergleichs(/Fang)register ?
t1edge_int_flag	 dbit	7,t1ctl3 ; Interrupt durch Flanke an T1IC/CR ?

t1ctl4		 equ	p04c
t1edge_det_ena	 dbit	0,t1ctl4 ; Flankendetektor an T1IC/CR freigeben
t1cr_rst_ena	 dbit	1,t1ctl4 ; externen Reset freigeben
t1edge_polarity	 dbit	2,t1ctl4 ; Flankenwahl T1IC/CR
t1cr_out_ena	 dbit	3,t1ctl4 ; Umschaltung PWM durch T1IC/CR freigeben
t1c1_rst_ena     dbit   4,t1ctl4 ; RÅcksetzen durch Vergleicher freigeben
t1c2_out_ena	 dbit	5,t1ctl4 ; Umschaltung PWM durch Vergleichs(/Fang)register freigeben
t1c1_out_ena	 dbit	6,t1ctl4 ; Umschaltung PWM durch Vergleichsregister freigeben
t1_mode		 dbit	7,t1ctl4 ; Modus Timer 1

t1pc1		 equ	p04d
t1evt_data_dir	 dbit	0,t1pc1	; Timer 1 Event-Pin Datenrichtung
t1evt_function	 dbit	1,t1pc1	; Timer 1 Event-Pin Funktionsumschaltung
t1evt_data_out	 dbit	2,t1pc1 ; Timer 1 Event-Pin Datenausgang
t1evt_data_in	 dbit	3,t1pc1 ; Timer 1 Event-Pin Dateneingang

t1pc2		 equ	p04e
t1iccr_data_dir	 dbit	0,t1pc2 ; T1IC/CR-Pin Datenrichtung
t1iccr_function	 dbit	1,t1pc2 ; T1IC/CR-Pin Funktionsumschaltung
t1iccr_data_out	 dbit	2,t1pc2 ; T1IC/CR-Pin Datenausgang
t1iccr_data_in	 dbit	3,t1pc2 ; T1IC/CR-Pin Dateneingang
t1pwm_data_dir	 dbit	4,t1pc2 ; T1PWM-Pin Datenrichtung
t1pwm_function	 dbit	5,t1pc2 ; T1PWM-Pin Funktionsumschaltung
t1pwm_data_out	 dbit	2,t1pc2 ; T1PWM-Pin Datenausgang
t1pwm_data_in	 dbit	7,t1pc2 ; T1PWM-Pin Dateneingang

t1pri		 equ	p04f
t1_priority	 dbit	6,t1pri ; PrioritÑt Interrupts Timer 1
t1_stest	 dbit	7,t1pri ; Selbsttest Timer 1
		endif

;----------------------------------------------------------------------------
; Timer 2

                if MOMCPU>370C030H
t2cntrh		 equ	p060	; ZÑhlerregister
t2cntrl		 equ	p061

t2ch		 equ	p062	; Vergleichsregister
t2cl		 equ	p063

t2cch		 equ	p064	; Vergleichs/Fangregister
t2ccl		 equ	p065

t2ich		 equ	p066	; Fangregister
t2icl		 equ	p067

t2ctl1		 equ	p06a
t2_sw_reset	 dbit	0,t2ctl1 ; ZÑhler zurÅcksetzen
t2_input_select0 dbit	1,t2ctl1 ; Taktquelle
t2_input_select1 dbit	2,t2ctl1
t2_ovrfl_int_flag dbit	3,t2ctl1 ; Overflow aufgetreten ?
t2_ovrfl_int_ena dbit	4,t2ctl1 ; Overflow-Interrupts freigeben

t2ctl2		 equ	p06b
t2c1_int_ena	 dbit	0,t2ctl2 ; Interrupts Vergleichsregister freigeben
t2c2_int_ena	 dbit	1,t2ctl2 ; Interrupts Vergleichs(/Fang)register freigeben
t2edge1_int_ena	 dbit	2,t2ctl2 ; Interrupts bei Flanke an T2IC1/CR freigeben
t2c1_int_flag	 dbit	5,t2ctl2 ; Interrupt durch Vergleichsregister ?
t2c2_int_flag	 dbit	6,t2ctl2 ; Interrupt durch Vergleichs(/Fang)register ?
t2edge1_int_flag dbit	7,t2ctl2 ; Interrupt durch Flanke an T2IC1/CR ?

t2ctl3		 equ	p06c
t2edge1_det_ena	 dbit	0,t2ctl3 ; Flankendetektor an T2IC1/CR freigeben
t2edge1_rst_ena	 dbit	1,t2ctl3 ; externen Reset freigeben
t2edge2_det_ena	 dbit	1,t2ctl3 ; Flankendetektor an T2IC2/CR freigeben
t2edge1_polarity dbit	2,t2ctl3 ; Flankenwahl T2IC1/CR
t2edge1_out_ena	 dbit	3,t2ctl3 ; Umschaltung PWM durch T2IC1/CR freigeben
t2edge2_polarity dbit	3,t2ctl3 ; Flankenwahl T2IC2/CR
t2c1_rst_ena	 dbit	4,t2ctl3 ; RÅcksetzen durch Vergleicher freigeben
t2c2_out_ena	 dbit	5,t2ctl3 ; Umschaltung PWM durch Vergleichs(/Fang)register freigeben
t2c1_out_ena	 dbit	6,t2ctl3 ; Umschaltung PWM durch Vergleichsregister freigeben
t2_mode		 dbit	7,t2ctl3 ; Modus Timer 2

t2pc1		 equ	p06d
t2evt_data_dir	 dbit	0,t2pc1	; Timer 2 Event-Pin Datenrichtung
t2evt_function	 dbit	1,t2pc1	; Timer 2 Event-Pin Funktionsumschaltung
t2evt_data_out	 dbit	2,t2pc1 ; Timer 2 Event-Pin Datenausgang
t2evt_data_in	 dbit	3,t2pc1 ; Timer 2 Event-Pin Dateneingang

t2pc2		 equ	p06e
t2ic1cr_data_dir dbit	0,t1pc2 ; T2IC1/CR-Pin Datenrichtung
t2ic1cr_function dbit	1,t1pc2 ; T2IC1/CR-Pin Funktionsumschaltung
t2ic1cr_data_out dbit	2,t1pc2 ; T2IC1/CR-Pin Datenausgang
t2ic1cr_data_in	 dbit	3,t1pc2 ; T2IC1/CR-Pin Dateneingang
t2ic2cr_data_dir dbit	4,t1pc2 ; T2IC2/CR-Pin Datenrichtung
t2ic2cr_function dbit	5,t1pc2 ; T2IC2/CR-Pin Funktionsumschaltung
t2ic2cr_data_out dbit	6,t1pc2 ; T2IC2/CR-Pin Datenausgang
t2ic2cr_data_in	 dbit	7,t1pc2 ; T2IC2/CR-Pin Dateneingang
t2pwm_data_dir	 dbit	4,t1pc2 ; T2PWM-Pin Datenrichtung
t2pwm_function	 dbit	5,t1pc2 ; T2PWM-Pin Funktionsumschaltung
t2pwm_data_out	 dbit	6,t1pc2 ; T2PWM-Pin Datenausgang
t2pwm_data_in	 dbit	7,t1pc2 ; T2PWM-Pin Dateneingang

t2pri		 equ	p06f
t2_priority	 dbit	6,t2pri ; PrioritÑt Interrupts Timer 2
t2_stest	 dbit	7,t2pri ; Selbsttest Timer 2
		endif

;----------------------------------------------------------------------------
; Watchdog

                if      MOMCPU<>370C030h
wdcntrh          equ    p046    ; ZÑhlregister
wdcntrl          equ    p047

wdrst            equ    p048    ; RÅcksetz-SchlÅsselregister

wd_input_select0 dbit	4,t1ctl1 ; Taktquelle
wd_input_select1 dbit	5,t1ctl1
wd_input_select2 dbit	6,t1ctl1
wd_ovrfl_tap_sel dbit	7,t1ctl1 ; 15- oder 16-bittig zÑhlen
wd_ovrfl_int_flag dbit	5,t1ctl2 ; Watchdog-Reset aufgetreten ?
wd_ovrfl_int_ena dbit	6,t1ctl2 ; Watchdog-Interrupts freigeben
wd_ovrfl_rst_ena dbit   7,t1ctl2 ; Watchdog-Resets freigeben
                endif

;----------------------------------------------------------------------------
; SCI

                if (MOMCPU=370C020h)||(MOMCPU>370C030h)
sciccr		 equ	p050
sci_char0	 dbit	0,sciccr ; ZeichenlÑnge
sci_char1	 dbit	1,sciccr
sci_char2	 dbit	2,sciccr
address_idle_wup dbit	3,sciccr ; Multiprozessormodus
async_iosync	 dbit	4,sciccr ; Betriebsart: synchron/asynchron
parity_enable	 dbit	5,sciccr ; mit/ohne ParitÑt
even_odd_parity	 dbit	6,sciccr ; gerade/ungerade ParitÑt
stop_bits	 dbit	7,sciccr ; Anzahl Stopbits

scictl		 equ	p051
rxena		 dbit	0,scictl ; EmpfÑngerfreigabe
txena		 dbit	1,scictl ; Senderfreigabe
sleep		 dbit	2,scictl ; Sleep-Modus SCI
txwake		 dbit	3,scictl ; Sender aufwecken
clock		 dbit   4,scictl ; interner/externer Takt SCI
sci_sw_reset	 dbit	5,scictl ; Software-Reset

baud_msb	 equ	p052	 ; Baudratengenerator
baud_lsb	 equ	p053

txctl		 equ	p054
sci_tx_int_ena	 dbit	0,txctl	 ; Interrupts durch Sender freigeben
tx_empty	 dbit	6,txctl  ; Sender komplett leer ?
txrdy		 dbit	7,txctl  ; neues Datum kann geschrieben werden

rxctl		 equ	p055
sci_rx_int_ena	 dbit	0,rxctl	 ; Interrupts durch EmpfÑnger freigeben
rxwake		 dbit	1,rxctl	 ; Ist EmpfÑnger aufgeweckt worden ?
sci_pe		 dbit	2,rxctl	 ; ParitÑtsfehler ?
sci_oe		 dbit	3,rxctl	 ; öberlauffehler ?
sci_fe		 dbit	4,rxctl	 ; kein korrektes Stopbit ?
brkdt		 dbit	5,rxctl	 ; Break aufgetreten ?
rxrdy		 dbit	6,rxctl	 ; Zeichen empfangen ?
rx_error	 dbit	7,rxctl	 ; Empfangsfehler aufgetreten ?

rxbuf		 equ	p057	 ; Datenregister
txbuf		 equ	p059

scipc1		 equ	p05d
sciclk_data_dir	 dbit	0,scipc1 ; SCICLK-Pin Datenrichtung
sciclk_function	 dbit	1,scipc1 ; SCICLK-Pin Funktionsumschaltung
sciclk_data_out	 dbit	2,scipc1 ; SCICLK-Pin Datenausgang
sciclk_data_in	 dbit	3,scipc1 ; SCICLK-Pin Dateneingang

scipc2		 equ	p05e
scirxd_data_dir	 dbit	0,scipc2 ; SCIRXD-Pin Datenrichtung
scirxd_function	 dbit	1,scipc2 ; SCIRXD-Pin Funktionsumschaltung
scirxd_data_out	 dbit	2,scipc2 ; SCIRXD-Pin Datenausgang
scirxd_data_in	 dbit	3,scipc2 ; SCIRXD-Pin Dateneingang
scitxd_data_dir	 dbit	4,scipc2 ; SCITXD-Pin Datenrichtung
scitxd_function	 dbit	5,scipc2 ; SCITXD-Pin Funktionsumschaltung
scitxd_data_out	 dbit	6,scipc2 ; SCITXD-Pin Datenausgang
scitxd_data_in	 dbit	7,scipc2 ; SCITXD-Pin Dateneingang

scipri		 equ	p05f
sci_espen	 dbit	4,scipri ; SCI emulator suspend ?!
scirx_priority	 dbit	5,scipri ; SCI Empfangsinterrupt PrioritÑt
scitx_priority	 dbit	6,scipri ; SCI Sendeinterrupt PrioritÑt
sci_test	 dbit	7,scipri ; SCI Selbsttest
		endif

;----------------------------------------------------------------------------
; SPI

                if (MOMCPU<370C030h)||(MOMCPU=370C050h)
spiccr           equ    p030
spi_char0        dbit   0,spiccr ; ZeichenlÑnge
spi_char1        dbit   1,spiccr
spi_char2        dbit   2,spiccr
spi_bit_rate0    dbit   3,spiccr ; Bitrate
spi_bit_rate1    dbit   4,spiccr
spi_bit_rate2    dbit   5,spiccr
clock_polarity   dbit   6,spiccr ; PolaritÑt Taktsignal
spi_sw_reset     dbit   7,spiccr ; Software-Reset

spictl           equ    p031
spi_int_ena      dbit   0,spictl ; Interrupt-Freigabe
talk             dbit   1,spictl ; Slave/Master-Senderfreigabe
master_slave     dbit   2,spictl ; Slave/Master-Umschaltung
spi_int_flag     dbit   6,spictl ; Interrupt aufgetreten ?
receiver_overrun dbit   7,spictl ; EMpfÑngerÅberlauf ?

spibuf           equ    p037     ; EmpfÑngerpuffer
spidat           equ    p039     ; Senderegister

spipc1           equ    p03d
spiclk_data_dir  dbit   0,spipc1 ; SPICLK-Pin Datenrichtung
spiclk_function  dbit   1,spipc1 ; SPICLK-Pin Funktionsumschaltung
spiclk_data_out  dbit   2,spipc1 ; SPICLK-Pin Datenausgang
spiclk_data_in   dbit   3,spipc1 ; SPICLK-Pin Dateneingang

spipc2           equ    p03e
spisomi_data_dir dbit   0,spipc2 ; SPISOMI-Pin Datenrichtung
spisomi_function dbit   1,spipc2 ; SPISOMI-Pin Funktionsumschaltung
spisomi_data_out dbit   2,spipc2 ; SPISOMI-Pin Datenausgang
spisomi_data_in  dbit   3,spipc2 ; SPISOMI-Pin Dateneingang
spimosi_data_dir dbit   4,spipc2 ; SPIMOSI-Pin Datenrichtung
spimosi_function dbit   5,spipc2 ; SPIMOSI-Pin Funktionsumschaltung
spimosi_data_out dbit   6,spipc2 ; SPIMOSI-Pin Datenausgang
spimosi_data_in  dbit   7,spipc2 ; SPIMOSI-Pin Dateneingang

spipri           equ    p03f
spi_espen        dbit   5,spipri ; Emulator unterdrÅcken ?!
spi_priority     dbit   6,spipri ; Interrupt-PrioritÑt
spi_stest        dbit   7,spipri ; Selbsttest
                endif

;----------------------------------------------------------------------------
; A/D-Wandler

                if      MOMCPU>370C030h
adctl            equ    p070
ad_input_select0 dbit   0,adctl  ; Kanalauswahl
ad_input_select1 dbit   1,adctl
ad_input_select2 dbit   2,adctl
ref_volt_select0 dbit   3,adctl  ; Auswahl Referenzspannung
ref_volt_select1 dbit   4,adctl
ref_volt_select2 dbit   5,adctl
sample_start     dbit   6,adctl  ; Sample-Phase beginnen (wird autom. gelîscht)
convert_start    dbit   7,adctl  ; Wandlungs-Phase beginnen

adstat           equ    p071
ad_int_ena       dbit   0,adstat ; Interrupt-Freigabe
ad_int_flag      dbit   1,adstat ; Interrupt aufgetreten ?
ad_ready         dbit   2,adstat ; AD-Wandler bereit ?

addata           equ    p072     ; Wandlungsergebnis
adin             equ    p07d     ; digitale Eingangsdaten, wenn nicht analog genutzt
adena            equ    p07e     ; AnalogeingÑnge freischalten

adpri            equ    p07f
ad_espen         dbit   5,adpri  ; Emulator unterdrÅcken ?!
ad_priority      dbit   6,adpri  ; Interrupt-PrioritÑt
ad_stest         dbit   7,adpri  ; Selbsttest
                endif

;----------------------------------------------------------------------------
; PACT

                if      MOMCPU=370C030h
pact_scr         equ    p040
pact_prescale_select0 dbit 0,pact_scr   ; Vorteilerauswahl
pact_prescale_select1 dbit 1,pact_scr
pact_prescale_select2 dbit 2,pact_scr
pact_prescale_select3 dbit 3,pact_scr
fast_mode_select dbit   4,pact_scr      ; Vorteiler durch 8 aus
cmd_def_area_ena dbit   5,pact_scr      ; Zugriff auf Dual-Port-RAM freigeben
deftim_ovrfl_int_flag dbit 6,pact_scr   ; TimerÅberlauf aufgetreten ?
deftim_ovrfl_int_ena  dbit 7,pact_scr   ; Interrupts bei TimerÅberlauf freigeben

cdstart          equ    p041
cmd_def_area2    dbit   2,cdstart       ; Startadresse Kommando/Definitionsbereich
cmd_def_area3    dbit   3,cdstart
cmd_def_area4    dbit   4,cdstart
cmd_def_area5    dbit   5,cdstart
cmd_def_area_int_ena dbit 7,cdstart     ; Interrupts aus dem Bereich freigeben

cdend            equ    p042
cmd_def_area_end2 dbit  2,cdend         ; Endadresse Kommando/Definitionsbereich
cmd_def_area_end3 dbit  3,cdend
cmd_def_area_end4 dbit  4,cdend
cmd_def_area_end5 dbit  5,cdend
cmd_def_area_end6 dbit  6,cdend

bufptr           equ    p043
buffer_pointer1  dbit   1,bufptr        ; Adresse Pufferzeiger
buffer_pointer2  dbit   2,bufptr
buffer_pointer3  dbit   3,bufptr
buffer_pointer4  dbit   4,bufptr
buffer_pointer5  dbit   5,bufptr

scictlp          equ    p045
sci_sw_reset      dbit  0,scictlp       ; Software Reset
sci_tx_int_ena    dbit  2,scictlp       ; Sendeinterrupts freigeben
sci_rx_int_ena    dbit  3,scictlp       ; EmpfÑngerinterrupts freigeben
pact_fe           dbit  4,scictlp       ; Rahmenfehler ?
pact_parity       dbit  5,scictlp       ; ParitÑtsfehler ?
pact_txrdy        dbit  6,scictlp       ; Sender leer ?
pact_rxrdy        dbit  7,scictlp       ; Empfangspuffer voll ?

rxbufp           equ    p046            ; Mini-SCI Empfangspuffer
txbufp           equ    p047            ; Mini-SCI Sendepuffer

opstate          equ    p048            ; Status Ausgabepins
cdflags          equ    p049            ; Interrupts durch Kommandos 0..7 aufgetreten ?

cpctl1           equ    p04a
cp1_capt_falling_edge dbit 0,cpctl1     ; Fangen bei fallender Flanke an CP1
cp1_capt_rising_edge dbit 1,cpctl1      ; Fangen bei steigender Flanke an CP1
cp1_int_flag     dbit   2,cpctl1        ; Flanke an CP1 aufgetreten ?
cp1_int_ena      dbit   3,cpctl1        ; Interrupts durch CP1 freigeben
cp2_capt_falling_edge dbit 4,cpctl1     ; Fangen bei fallender Flanke an CP2
cp2_capt_rising_edge dbit 5,cpctl1      ; Fangen bei steigender Flanke an CP2
cp2_int_flag     dbit   6,cpctl1        ; Flanke an CP2 aufgetreten ?
cp2_int_ena      dbit   7,cpctl1        ; Interrupts durch CP2 freigeben

cpctl2           equ    p04b
cp3_capt_falling_edge dbit 0,cpctl2     ; Fangen bei fallender Flanke an CP3
cp3_capt_rising_edge dbit 1,cpctl2      ; Fangen bei steigender Flanke an CP3
cp3_int_flag     dbit   2,cpctl2        ; Flanke an CP3 aufgetreten ?
cp3_int_ena      dbit   3,cpctl2        ; Interrupts durch CP3 freigeben
cp4_capt_falling_edge dbit 4,cpctl2     ; Fangen bei fallender Flanke an CP4
cp4_capt_rising_edge dbit 5,cpctl2      ; Fangen bei steigender Flanke an CP4
cp4_int_flag     dbit   6,cpctl2        ; Flanke an CP4 aufgetreten ?
cp4_int_ena      dbit   7,cpctl2        ; Interrupts durch CP4 freigeben

cpctl3           equ    p04c
cp5_capt_falling_edge dbit 0,cpctl3     ; Fangen bei fallender Flanke an CP5
cp5_capt_rising_edge dbit 1,cpctl3      ; Fangen bei steigender Flanke an CP5
cp5_int_flag     dbit   2,cpctl3        ; Flanke an CP5 aufgetreten ?
cp5_int_ena      dbit   3,cpctl3        ; Interrupts durch CP5 freigeben
cp6_capt_falling_edge dbit 4,cpctl3     ; Fangen bei fallender Flanke an CP6
cp6_capt_rising_edge dbit 5,cpctl3      ; Fangen bei steigender Flanke an CP6
cp6_int_flag     dbit   6,cpctl3        ; Flanke an CP6 aufgetreten ?
cp6_int_ena      dbit   7,cpctl3        ; Interrupts durch CP6 freigeben

cppre            equ    p04d
op_set_clr_select dbit  0,cppre         ; Ausgabepins per Sofware lîschen/setzen?
event_counter_sw_reset dbit 1,cppre     ; EreigniszÑhler zurÅcksetzen
cp6_event_only   dbit   2,cppre         ; CP6 nur fÅr EreigniszÑhler
input_capt_prescale_select0 dbit 3,cppre; Vorteiler fÅr CP3..CP6
input_capt_prescale_select1 dbit 4,cppre
input_capt_prescale_select2 dbit 5,cppre
buffer_half_full_int_flag dbit 6,cppre  ; Interrupts, da· Puffer halb/ganz voll ?
buffer_half_full_int_ena dbit 7,cppre   ; obige Interrupts freischalten

wdrst            equ    p04e

pactpri          equ    p04f
wd_prescale_select0 dbit 0,pactpri      ; Vorteiler Watchdog
wd_prescale_select1 dbit 1,pactpri
pact_mode_select dbit 2,pactpri         ; PACT in Modus A oder B
pact_group_3_priority dbit 3,pactpri    ; PrioritÑten der Interruptgruppen
pact_group_2_priority dbit 4,pactpri
pact_group_1_priority dbit 5,pactpri
pact_stest       dbit   7,pactpri       ; Selbsttest
                endif

;----------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben

