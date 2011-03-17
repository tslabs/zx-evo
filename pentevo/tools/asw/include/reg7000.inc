                save
;                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.41 - Datei REG7000.INC                                            *
;*   								            *
;*   Sinn : enthÑlt SFR- und Bitdefinitionen fÅr die TMS70Cxx-Prozessoren   *
;* 									    *
;*   letzte énderungen : 15. 2.1997                                         *
;*									    *
;****************************************************************************

                ifndef  reg7000inc      ; verhindert Mehrfacheinbindung

reg7000inc      equ     1

                switch  MOMCPUNAME
                case    "TMS70C40"
IROM             equ     0f000h
__group          equ     1
                case    "TMS70C20"
IROM             equ     0f800h
__group          equ     1
                case    "TMS70C00"
IROM             equ     10000h
__group          equ     1
                case    "TMS70CT40"
IROM             equ     0f000h
__group          equ     2
                case    "TMS70CT20"
IROM             equ     0f800h
__group          equ     2
                case    "TMS70C82"
IROM             equ     0e000h
__group          equ     3
                case    "TMS70C42"
IROM             equ     0f000h
__group          equ     3
                case    "TMS70C02"
IROM             equ     0f800h
__group          equ     3
                case    "TMS70C48"
IROM             equ     0f000h
__group          equ     4
                case    "TMS70C08"
IROM             equ     10000h
__group          equ     4
                elsecase
                 fatal  "Fehler: nur TMS70Cxx-Prozessoren erlaubt!"
                endcase

                if      MOMPASS=1
                 message "TMS7000-Register-Definitionen (C) 1997 Alfred Arnold"
		endif

;----------------------------------------------------------------------------
; Speicherbereiche

IRAM            equ     0
IROMEND         equ     0ffffh
                if      __group<=2
IRAMEND          equ     127
                elseif
IRAMEND          equ     255
                endif

;----------------------------------------------------------------------------
; Peripherie

IOCNT0          equ     p0              ; I/O Control Register 0
APORT            equ     p4             ; Port A Data
BPORT            equ     p6             ; Port B Data
CPORT            equ     p8             ; Port C Data
CDDR             equ     p9             ; Port C Data Direction Register
DPORT            equ     p10            ; Port D Data
DDDR             equ     p11            ; Port D Data Direction Register

                if      __group<=2
T1DATA           equ     p2             ; Timer 1 Data
T1CTL            equ     p3             ; Timer 1 Control
                endif

                if      __group>=3
IOCNT2           equ     p1             ; I/O Control Register 2
IOCNT1           equ     p2             ; I/O Control Register 1
ADDR             equ     p5             ; Port A Data Direction Register
T1MSDATA         equ     p12            ; Timer 1 MSB Dec. Reload / Readout Latch
T1LSDATA         equ     p13            ; Timer 1 LSB Reload / Dec. Value
T1CTL1           equ     p14            ; Timer 1 Control Register 1 / MSB Readout Latch
T1CTL0           equ     p15            ; Timer 1 Control Register 0 / LSB Capture Latch
T2MSDATA         equ     p16            ; Timer 2 MSB Dec. Reload / Readout Latch
T2LSDATA         equ     p17            ; Timer 2 LSB Reload / Dec. Value
T2CTL1           equ     p18            ; Timer 2 Control Register 1 / MSB Readout Latch
T2CTL0           equ     p19            ; Timer 2 Control Register 0 / LSB Capture Latch
SMODE            equ     p20            ; Serial Port Mode Control Register
SCTL0            equ     p21            ; Serial Port Control Register 0
SSTAT            equ     p22            ; Serial Port Status Register
T3DATA           equ     p23            ; Timer 3 Reload Reg. / Decr. Value
SCTL1            equ     p24            ; Serial Port Control Register 1
RXBUF            equ     p25            ; Receiver Buffer
TXBUF            equ     p26            ; Transmitter Buffer
                endif

                if      __group=4
EPORT            equ     p28            ; Port E Data
EDDR             equ     p29            ; Port E Data Direction Register
FPORT            equ     p30            ; Port F Data
FDDR             equ     p31            ; Port F Data Direction Register
GPORT            equ     p32            ; Port G Data
GDDR             equ     p33            ; Port G Data Direction Register
                endif

;----------------------------------------------------------------------------

		endif

		restore                 ; wieder erlauben


