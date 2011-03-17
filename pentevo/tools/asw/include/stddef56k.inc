                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF56.INC                                           *
;*   								            *
;*   Sinn : enthÑlt SFR-Definitionen fÅr den DSP56000-Prozessor             *
;* 									    *
;*   letzte énderungen : 30.12.1993                                         *
;*                                                                          *
;****************************************************************************

                ifndef  stddef56inc     ; verhindert Mehrfacheinbindung

stddef56inc     equ     1

                if      (MOMCPU<>352256)
                 fatal  "Falscher Prozessortyp eingestellt: nur 56000 erlaubt!"
		endif

                if      MOMPASS=1
                 message "DSP56000-SFR-Definitionen (C) 1993 Alfred Arnold"
		endif

;---------------------------------------------------------------------------

pbc		xsfr	$ffe0		; Port B Bussteuerregister
pbddr		xsfr	$ffe2		; Port B Datenrichtungsregister
pbd		xsfr	$ffe4		; Port B Datenregister

pcc		xsfr	$ffe1		; Port C Steuerregister
pcddr		xsfr	$ffe3		; Port C Datenrichtungsregister
pcd		xsfr	$ffe5		; Port C Datenregister

hcr		xsfr	$ffe8		; Host-Steuerregister
hsr		xsfr	$ffe9		; Host-Statusregister
htx		xsfr	$ffeb		; Host-Senderegister
hrx		xsfr	htx		; Host-Empfangsregister

cra		xsfr	$ffec		; SSI Steuerregister A
crb		xsfr	$ffed		; SSI Steuerregister B
tsr		xsfr	$ffee		; SSI Status/Zeitschlitzregister

tx		xsfr	$ffef		; SCI Senderegister
rx		xsfr	tx		; SCI Empfangsregister
scr		xsfr	$fff0		; SCI Schnittstellensteuerregister
ssr		xsfr	$fff1		; SCI Schittstellenstatusregister
sccr		xsfr	$fff2		; SCI Steuerregister
stxa		xsfr	$fff3		; SCI Sendedatenadre·register
srx		xsfr	$fff4		; SCI Empfangsregister Basisadresse
stx		xsfr	srx		; SCI Senderegister Basisadresse
srx_lo		xsfr	srx		; SCI Empfangsregister in Bit 0..7
stx_lo		xsfr	stx		; SCI Senderegister in Bit 0..7
srx_mid		xsfr	srx+1		; SCI Empfangsregister in Bit 8..15
stx_mid		xsfr	stx+1		; SCI Senderegister in Bit 8..15
srx_hi		xsfr	srx+2		; SCI Empfangsregister in Bit 16..23
stx_hi		xsfr	stx+2		; SCI Senderegister in Bit 16..23

bcr		xsfr	$fffe		; Bussteuerregister
ipr		xsfr	$ffff		; Interrupt-PrioritÑtenregister

;---------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben


