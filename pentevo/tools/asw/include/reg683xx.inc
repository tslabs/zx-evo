                save
                listing off             ; kein Listing Åber diesen File
                macexp  off             ; spart noch ein bi·chen Zeit

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei REG96.INC                                              *
;*   								            *
;*   Sinn : enthÑlt Registeradre·definitionen fÅr 68332 und 68340           *
;* 									    *
;*   letzte énderungen : 18.11.1994                                         *
;*                                                                          *
;****************************************************************************

                ifndef  reg68332inc     ; verhindert Mehrfacheinbindung

reg6833xinc     equ     1

                if      (MOMCPUNAME<>"68332")&&(MOMCPUNAME<>"68340")&&(MOMCPUNAME<>"68360")
                 fatal   "Falscher Prozessortyp eingestellt: nur 68332 oder 68340 erlaubt!"
		endif


                if      MOMPASS=1
                 message "CPU32-Register-Definitionen (C) 1994 Alfred Arnold"
                 message "binde \{MOMCPU}-Register ein"
		endif

;-----------------------------------------------------------------------------
; Basis ist entweder $fffa00 oder $7fa000, das mu· man vorher setzen
; (oder mit dem Default leben :-)
; beim 68340 kann man ganz frei wÑhlen
; Da der 68332 A31..A24 nicht herausfÅhrt, kînnte man evtl. auch die Register
; ab $fffffa00 erreichen und kurze Adressen verwenden.  Das mÅ·te man mal
; ausprobieren...alternativ kann man man aber auch die Basis auf 0 setzen,
; bevor man diese Datei einbindet und dann die Symbole registerrelativ
; verwenden.

                ifndef  SIMBase
                 if      MOMCPU=$68332
SIMBase           equ     $fffa00
                 elseif
SIMBase           equ     $000000
                 endif
                endif

;=============================================================================
; Da sich 68360, 68340 und 68332 sehr deutlich im Registermodell unterscheiden,
; habe ich mir keine MÅhe gemacht, Register fÅr Register auseinanderzufieseln,
; was gemeinsam ist und was nicht

                switch  MOMCPUNAME

;-----------------------------------------------------------------------------

                case    "68360"

;-----------------------------------------------------------------------------

MBAR            equ     $0003ff00       ; [L] Startadresse Peripherie (CPU-Space!)
MBARE           equ     $0003ff04       ; [L] Sperrung/Freigabe MBAR

RegBase         equ     SIMBase+$1000   ; Startadresse Rgister

MCR             equ     RegBase+$0000   ; [L] Modulkonfiguration SIM

AVR             equ     RegBase+$0008   ; [B] Freigabe Autovektor-Interrupts
RSR             equ     RegBase+$0009   ; [B] Reset-Status
CLK0CR          equ     RegBase+$000c   ; [B] Steuerung Taktausgang 2 & 1
PLLCR           equ     RegBase+$0010   ; [W] PLL-Steuerung
CDVCR           equ     RegBase+$0014   ; [W] Steuerung "langsamer" Takt
PEPAR           equ     RegBase+$0016   ; [W] Zuordnung der I/O-Pins Port E
SYPCR           equ     RegBase+$0022   ; [B] Systemmonitore, Bustimimg
SWIV            equ     RegBase+$0023   ; [B] Interruptvektor Watchdog
PICR            equ     RegBase+$0026   ; [W] Interruptebene+ -vektor period. Interrupt
PITR            equ     RegBase+$002a   ; [W] ZÑhlwert & Vorteiler period. Interrupt
SWSR            equ     RegBase+$002f   ; [B] Watchdog rÅcksetzen
BKAR            equ     RegBase+$0030   ; [L] Breakpoint-Adresse
BKCR            equ     RegBase+$0034   ; [L] Breakpoint-Steuerung

GMR             equ     RegBase+$0040   ; [L] globale Steuerung Memory-Controller
MSTAT           equ     RegBase+$0044   ; [W] Status Memory-Controller
BR0             equ     RegBase+$0050   ; [L] SRAM/DRAM-Basis CS0
OR0             equ     RegBase+$0054   ; [L] Optionen DRAM/SRAM CS0
BR1             equ     RegBase+$0060   ; [L] SRAM/DRAM-Basis CS1
OR1             equ     RegBase+$0064   ; [L] Optionen DRAM/SRAM CS1
BR2             equ     RegBase+$0070   ; [L] SRAM/DRAM-Basis CS2
OR2             equ     RegBase+$0074   ; [L] Optionen DRAM/SRAM CS2
BR3             equ     RegBase+$0080   ; [L] SRAM/DRAM-Basis CS3
OR3             equ     RegBase+$0084   ; [L] Optionen DRAM/SRAM CS3
BR4             equ     RegBase+$0090   ; [L] SRAM/DRAM-Basis CS4
OR4             equ     RegBase+$0094   ; [L] Optionen DRAM/SRAM CS4
BR5             equ     RegBase+$00a0   ; [L] SRAM/DRAM-Basis CS5
OR5             equ     RegBase+$00a4   ; [L] Optionen DRAM/SRAM CS5
BR6             equ     RegBase+$00b0   ; [L] SRAM/DRAM-Basis CS6
OR6             equ     RegBase+$00b4   ; [L] Optionen DRAM/SRAM CS6
BR7             equ     RegBase+$00c0   ; [L] SRAM/DRAM-Basis CS7
OR7             equ     RegBase+$00c4   ; [L] Optionen DRAM/SRAM CS7

;-----------------------------------------------------------------------------
; Communications Controller:

RAMBase         equ     SIMBase         ; [ ] RAM-Basisadresse

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; IDMA:

IDMA1Base       equ     RAMBase+$0e70
IDMA2Base       equ     RAMBase+$0f70

ICCR            equ     RegBase+$0500   ; [W] Konfiguration IDMA-KanÑle
CMR1            equ     RegBase+$0504   ; [W] Modus IDMA1
CMR2            equ     RegBase+$0526   ; [W] Modus IDMA2
__defidma       macro   NAME,Adr,IDMABase
SAPR{NAME}      equ     Adr             ; [L] Quelladresse bei Speicherkopieraktionen
DAPR{NAME}      equ     Adr+4           ; [L] Zieladresse   "             "
BCR{NAME}       equ     Adr+8           ; [L] ZÑhlregister IDMA
FCR{NAME}       equ     Adr+12          ; [B] Funktionscodes
CMAR{NAME}      equ     Adr+14          ; [B] Kanalmaske
CSR{NAME}       equ     Adr+16          ; [B] Kanalstatus IDMA
IDMA{NAME}_IBASE         equ      IDMABase+0     ; [W] Basisadresse Deskriptor
IDMA{NAME}_IBPTR         equ      IDMABase+0     ; [W] Pointer Deskriptor
IDMA{NAME}_ISTATE        equ      IDMABase+0     ; [L] int. Status
IDMA{NAME}_ITEMP         equ      IDMABase+0     ; [L] temp. Speicher
                endm
                __defidma "1",RegBase+$508
                __defidma "2",RegBase+$528

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; SDMA:

SDSR            equ     RegBase+$051c   ; [B] Status SDMA
SDCR            equ     RegBase+$051e   ; [W] Konfiguration SDMA-KanÑle
SDAR            equ     RegBase+$0520   ; [L] SDMA-Adre·register

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; CPIC:

CICR            equ     RegBase+$0540   ; [L] Interrupt-Konfiguration
CIPR            equ     RegBase+$0544   ; [L] Interrupt-Anzeigen
CIMR            equ     RegBase+$0548   ; [L] Interrupt-Masken
CISR            equ     RegBase+$054c   ; [L] Anzeige, ob Interrupts in Bearbeitung

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; PIO:

PADIR           equ     RegBase+$0550   ; [W] Datenrichtungsregister Port A
PAPAR           equ     RegBase+$0552   ; [W] Zuordnung Port A
PAODR           equ     RegBase+$0554   ; [W] Steuerbits Port A als open drain
PADAT           equ     RegBase+$0556   ; [W] Datenregister Port A

PCDIR           equ     RegBase+$0560   ; [W] Datenrichtungsregister Port C
PCPAR           equ     RegBase+$0562   ; [W] Zuordnung Port C
PCSO            equ     RegBase+$0564   ; [W] Special Options Port C
PCDAT           equ     RegBase+$0566   ; [W] Datenregister Port C
PCINT           equ     RegBase+$0568   ; [W] Interruptsteuerung Port C

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; TIMER:

TimerBase       equ     RAMBase+$0db0

TGCR            equ     RegBase+$0560   ; [W] globale Konfiguration Timer
TMR1            equ     RegBase+$0590   ; [W] Modus Timer 1
TRR1            equ     RegBase+$0594   ; [W] Referenzwert Timer 1
TCR1            equ     RegBase+$0598   ; [W] Fangwert Timer 1
TCN1            equ     RegBase+$059c   ; [W] ZÑhlwert Timer 1
TER1            equ     RegBase+$05b0   ; [W] Ereignismeldung Timer 1
TMR2            equ     RegBase+$0592
TRR2            equ     RegBase+$0596
TCR2            equ     RegBase+$059a
TCN2            equ     RegBase+$059e
TER2            equ     RegBase+$05b2
TMR3            equ     RegBase+$05a0
TRR3            equ     RegBase+$05a4
TCR3            equ     RegBase+$05a8
TCN3            equ     RegBase+$05ac
TER3            equ     RegBase+$05b4
TMR4            equ     RegBase+$05a2
TRR4            equ     RegBase+$05a6
TCR4            equ     RegBase+$05aa
TCN4            equ     RegBase+$05ae
TER4            equ     RegBase+$05b6
TIMER_TM_BASE   equ     TimerBase+$00   ; [W] Basisadresse Tabelle
TIMER_TM_ptr    equ     TimerBase+$02   ; [W] Zeiger Tabelle
TIMER_R_TMR     equ     TimerBase+$04   ; [W] Modus
TIMER_R_TMV     equ     TimerBase+$06   ; [W] Valid-Register
TIMER_TM_cmd    equ     TimerBase+$08   ; [L] Kommandoregister
TIMER_TM_cnt    equ     TimerBase+$0c   ; [L] interner ZÑhler

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; CP:

MiscBase        equ     RAMBase+$0cb0

CR              equ     RegBase+$05c0   ; [W] Kommandoregister
RCCR            equ     RegBase+$05c4   ; [W] Konfiguration RISC-Controller
RTER            equ     RegBase+$05d6   ; [W] Timer-Events
RTMR            equ     RegBase+$05da   ; [W] Timer-Maske
CP_REV_num      equ     MiscBase        ; [W] Revisionsnummer Mikrocode

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; BRG:

BRGC1           equ     RegBase+$05f0   ; [L] Konfiguration Baudratengenerator 1
BRGC2           equ     RegBase+$05f4
BRGC3           equ     RegBase+$05f8
BRGC4           equ     RegBase+$05fc

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; SCC:

SCC1Base        equ     RAMBase+$0c00
SCC2Base        equ     RAMBase+$0d00
SCC3Base        equ     RAMBase+$0e00
SCC4Base        equ     RAMBase+$0f00

__defscc        macro   NAME,Adr,SCCBase
GSMR_L{NAME}    equ     Adr+0           ; [Q] Modus
GSMR_H{NAME}    equ     Adr+4
PSMR{NAME}      equ     Adr+8           ; [W] protokollspez. Modus
TODR{NAME}      equ     Adr+12          ; [W] Sendestart erzwingen
DSR{NAME}       equ     Adr+14          ; [W] Synchronisationsmuster SCCx
SCCE{NAME}      equ     Adr+16          ; [W] Ereignisregister UART
SCCM{NAME}      equ     Adr+20          ; [W] UART Ereignismaske
SCCS{NAME}      equ     Adr+23          ; [B] UART Status
SCC{NAME}_RBASE           equ     SCCBase+$00     ; [W] Startadresse Empfangspuffer
SCC{NAME}_TBASE           equ     SCCBase+$02     ; [W] Startadresse Sendepuffer
SCC{NAME}_RFCR            equ     SCCBase+$04     ; [B] Adre·raum Empfangen
SCC{NAME}_TFCR            equ     SCCBase+$05     ; [B] Adre·raum Senden
SCC{NAME}_MRBLR           equ     SCCBase+$06     ; [W] LÑnge Empfangspuffer
SCC{NAME}_RSTATE          equ     SCCBase+$08     ; [L] Status EmpfÑnger
SCC{NAME}_RBPTR           equ     SCCBase+$10     ; [W] Adre·zeiger Empfangen
SCC{NAME}_TSTATE          equ     SCCBase+$18     ; [L] Status Sender
SCC{NAME}_TBPTR           equ     SCCBase+$20     ; [W] Adre·zeiger Senden
SCC{NAME}_RCRC            equ     SCCBase+$28     ; [L] Empfangs-CRC
SCC{NAME}_TCRC            equ     SCCBase+$2c     ; [L] Sende-CRC
SCC{NAME}_MAX_IDL         equ     SCCBase+$38     ; [W] --UART-- Maximalzahl Idle-Zeichen
SCC{NAME}_IDLC            equ     SCCBase+$3a     ; [W] temp. Idle-ZÑhler
SCC{NAME}_BRKCR           equ     SCCBase+$3c     ; [W] Anzahl Sende-Breaks
SCC{NAME}_PAREC           equ     SCCBase+$3e     ; [W] ParitÑtsfehlerzÑhler
SCC{NAME}_FRMEC           equ     SCCBase+$40     ; [W] Rahmungs-FehlerzÑhler
SCC{NAME}_NOSEC           equ     SCCBase+$42     ; [W] RauschzÑhler
SCC{NAME}_BRKEC           equ     SCCBase+$44     ; [W] Break-Bedingung
SCC{NAME}_BRKLN           equ     SCCBase+$46     ; [W] LÑnge des letzten Breaks
SCC{NAME}_UADDR1          equ     SCCBase+$48     ; [W] Slave-Adressen
SCC{NAME}_UADDR2          equ     SCCBase+$4a     ; [W]
SCC{NAME}_RTEMP           equ     SCCBase+$4c     ; [W] temp. Speicher
SCC{NAME}_TOSEQ           equ     SCCBase+$4e     ; [W] out-of-sequence Zeichen
SCC{NAME}_CHARACTER1      equ     SCCBase+$50     ; [W] Zeichen, die Interrupts erzeugen
SCC{NAME}_CHARACTER2      equ     SCCBase+$52     ; [W]
SCC{NAME}_CHARACTER3      equ     SCCBase+$54     ; [W]
SCC{NAME}_CHARACTER4      equ     SCCBase+$56     ; [W]
SCC{NAME}_CHARACTER5      equ     SCCBase+$58     ; [W]
SCC{NAME}_CHARACTER6      equ     SCCBase+$5a     ; [W]
SCC{NAME}_CHARACTER7      equ     SCCBase+$5c     ; [W]
SCC{NAME}_CHARACTER8      equ     SCCBase+$5e     ; [W]
SCC{NAME}_RCCM            equ     SCCBase+$60     ; [W] Maske Empfangszeichen
SCC{NAME}_RCCR            equ     SCCBase+$62     ; [W] Empfangszeichen
SCC{NAME}_RLBC            equ     SCCBase+$64     ; [W] letztes Break-Zeichen
SCC{NAME}_C_MASK          equ     SCCBase+$34     ; [L] --HDLC-- CRC-Polynom
SCC{NAME}_C_PRES          equ     SCCBase+$38     ; [L] CRC-Startwert
SCC{NAME}_DISFC           equ     SCCBase+$3c     ; [W] ZÑhler fÅr verworfene Rahmen
SCC{NAME}_CRCEC           equ     SCCBase+$3e     ; [W] ZÑhler fÅr CRC-Fehler
SCC{NAME}_ABTSC           equ     SCCBase+$40     ; [W] ZÑhler fÅr AbbrÅche
SCC{NAME}_NMARC           equ     SCCBase+$42     ; [W] ZÑhler fÅr unpassende Adressen
SCC{NAME}_RETRC           equ     SCCBase+$44     ; [W] ZÑhler fÅr Retransmissionen
SCC{NAME}_MFLR            equ     SCCBase+$46     ; [W] MaximalrahmenlÑnge
SCC{NAME}_MAX_cnt         equ     SCCBase+$48     ; [W] LÑngenzÑhler
SCC{NAME}_RFTHR           equ     SCCBase+$4a     ; [W] Schwelle fÅr empfangene Rahmenzahl
SCC{NAME}_RFCNT           equ     SCCBase+$4c     ; [W] empfangene Rahmen
SCC{NAME}_HMASK           equ     SCCBase+$4e     ; [W] Adre·maske
SCC{NAME}_HADDR1          equ     SCCBase+$50     ; [W] Adressen
SCC{NAME}_HADDR2          equ     SCCBase+$52     ; [W]
SCC{NAME}_HADRR3          equ     SCCBase+$54     ; [W]
SCC{NAME}_HADDR4          equ     SCCBase+$56     ; [W]
SCC{NAME}_TMP             equ     SCCBase+$58     ; [W] TemporÑrspeicher
SCC{NAME}_TMP_MB          equ     SCCBase+$5a     ; [W]        "
SCC{NAME}_CRCC            equ     SCCBase+$34     ; [L] --BISYNC-- temp. Wert CRC
SCC{NAME}_PRCRC           equ     SCCBase+$38     ; [W] Empfangs-Vorgabe-CRC
SCC{NAME}_PTCRC           equ     SCCBase+$3a     ; [W] Sende-Vorgabe-CRC
SCC{NAME}_B_PAREC         equ     SCCBase+$3c     ; [W] ZÑhler Empfangs-ParitÑts-Fehler
SCC{NAME}_BSYNC           equ     SCCBase+$3e     ; [W] SYNC-Zeichen
SCC{NAME}_BDLE            equ     SCCBase+$40     ; [W] DLE-Zeichen
SCC{NAME}_B_CHARACTER1    equ     SCCBase+$42     ; [W] Steuerzeichen
SCC{NAME}_B_CHARACTER2    equ     SCCBase+$44     ; [W]
SCC{NAME}_B_CHARACTER3    equ     SCCBase+$46     ; [W]
SCC{NAME}_B_CHARACTER4    equ     SCCBase+$48     ; [W]
SCC{NAME}_B_CHARACTER5    equ     SCCBase+$4a     ; [W]
SCC{NAME}_B_CHARACTER6    equ     SCCBase+$4c     ; [W]
SCC{NAME}_B_CHARACTER7    equ     SCCBase+$4e     ; [W]
SCC{NAME}_B_CHARACTER8    equ     SCCBase+$50     ; [W]
SCC{NAME}_B_RCCM          equ     SCCBase+$52     ; [W] Empfangssteuerzeichenmaske
SCC{NAME}_CRC_P           equ     SCCBase+$30     ; [L] --Transparent-- CRC-Vorgabe
SCC{NAME}_CRC_C           equ     SCCBase+$34     ; [L] CRC-Konstante
SCC{NAME}_E_C_PRES        equ     SCCBase+$30     ; [L] --Ethernet-- CRC-Vorgabe
SCC{NAME}_E_C_MASK        equ     SCCBase+$34     ; [L] CRC-Maske
SCC{NAME}_E_CRCEC         equ     SCCBase+$38     ; [L] CRC-FehlerzÑhler
SCC{NAME}_ALEC            equ     SCCBase+$3c     ; [L] Alignment-FehlerzÑhler
SCC{NAME}_E_DISFC         equ     SCCBase+$40     ; [L] ZÑhler verworfene Rahmen
SCC{NAME}_PADS            equ     SCCBase+$44     ; [W] Padding-Zeichen kurze Rahmen
SCC{NAME}_RET_Lim         equ     SCCBase+$46     ; [W] Maximalzahl Retries
SCC{NAME}_RET_cnt         equ     SCCBase+$48     ; [W] Momentanzahl Retries
SCC{NAME}_E_MFLR          equ     SCCBase+$4a     ; [W] MaximalrahmenlÑnge
SCC{NAME}_MINFLR          equ     SCCBase+$4c     ; [W] MinimalrahmenlÑnge
SCC{NAME}_MAXD1           equ     SCCBase+$4e     ; [W] MaximallÑnge DMA1
SCC{NAME}_MAXD2           equ     SCCBase+$50     ; [W] MaximallÑnge DMA2
SCC{NAME}_MAXD            equ     SCCBase+$52     ; [W] Rx Max DMA
SCC{NAME}_DMA_cnt         equ     SCCBase+$54     ; [W] DMA-ZÑhler Empfang
SCC{NAME}_MAX_b           equ     SCCBase+$56     ; [W] Max BD Byte Count
SCC{NAME}_GADDR1          equ     SCCBase+$58     ; [W] Group Address Filter
SCC{NAME}_GADDR2          equ     SCCBase+$5a     ; [W]
SCC{NAME}_GADDR3          equ     SCCBase+$5c     ; [W]
SCC{NAME}_GADDR4          equ     SCCBase+$5e     ; [W]
SCC{NAME}_TBUF0.data0     equ     SCCBase+$60     ; [L] Save Areas - current frame
SCC{NAME}_TBUF0.data1     equ     SCCBase+$64     ; [L]
SCC{NAME}_TBUF0.rba0      equ     SCCBase+$68     ; [L]
SCC{NAME}_TBUF0.crc       equ     SCCBase+$6c     ; [L]
SCC{NAME}_TBUF0.bcnt      equ     SCCBase+$70     ; [W]
SCC{NAME}_PADDR1_H        equ     SCCBase+$72     ; [W] phys. Adresse
SCC{NAME}_PADDR1_M        equ     SCCBase+$74     ; [W]
SCC{NAME}_PADDR1_L        equ     SCCBase+$76     ; [W]
SCC{NAME}_P_Per           equ     SCCBase+$78     ; [W] Persistenz
SCC{NAME}_RFBD_ptr        equ     SCCBase+$7a     ; [W] Rx First BD Counter
SCC{NAME}_TFBD_ptr        equ     SCCBase+$7c     ; [W] Tx First BD Pointer
SCC{NAME}_TLBD_ptr        equ     SCCBase+$7e     ; [W] Tx Last BD Pointer
SCC{NAME}_TBUF1.data0     equ     SCCBase+$80     ; [L] Save Areas - next frame
SCC{NAME}_TBUF1.data1     equ     SCCBase+$84     ; [L]
SCC{NAME}_TBUF1.rba0      equ     SCCBase+$88     ; [L]
SCC{NAME}_TBUF1.crc       equ     SCCBase+$8c     ; [L]
SCC{NAME}_TBUF1.bcnt      equ     SCCBase+$90     ; [W]
SCC{NAME}_TX_len          equ     SCCBase+$92     ; [W] Tx Frame Length Counter
SCC{NAME}_IADDR1          equ     SCCBase+$94     ; [W] Individual address filters
SCC{NAME}_IADDR2          equ     SCCBase+$96     ; [W]
SCC{NAME}_IADDR3          equ     SCCBase+$98     ; [W]
SCC{NAME}_IADDR4          equ     SCCBase+$9a     ; [W]
SCC{NAME}_BOFF_CNT        equ     SCCBase+$9c     ; [W] Backoff ZÑhler
SCC{NAME}_TADDR_H         equ     SCCBase+$9e     ; [W] temp. Adresse
SCC{NAME}_TADDR_M         equ     SCCBase+$9a     ; [W]
SCC{NAME}_TADDR_L         equ     SCCBase+$a0     ; [W]
                endm
                __defscc "1",RegBase+$0600,SCC1Base
                __defscc "2",RegBase+$0620,SCC2Base
                __defscc "3",RegBase+$0640,SCC3Base
                __defscc "4",RegBase+$0660,SCC4Base

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; SMC:

SMC1Base        equ     RAMBase+$0e80
SMC2Base        equ     RAMBase+$0f80

__defsmc        macro   NAME,Adr,SMCBase
SMCMR{NAME}     equ     Adr+0           ; [W] Transparentmodus
SMCE{NAME}      equ     Adr+4           ; [B] Ereignisregister
SMCM{NAME}      equ     Adr+8           ; [W] Modus
SMC{NAME}_RBASE           equ     SMCBase+$00     ; [W] Empfangspufferdeskriptoradresse
SMC{NAME}_TBASE           equ     SMCBase+$02     ; [W] Sendepufferdeskriptoradresse
SMC{NAME}_RFCR            equ     SMCBase+$04     ; [B] Empfangs-Funktionscode
SMC{NAME}_TFCR            equ     SMCBase+$05     ; [B] Sende-Funktionscode
SMC{NAME}_MRBLR           equ     SMCBase+$06     ; [W] MaximallÑnge Empfangspuffer
SMC{NAME}_RSTATE          equ     SMCBase+$08     ; [L] interner EmpfÑngerstatus
SMC{NAME}_RBPTR           equ     SMCBase+$10     ; [W] Rx Buffer descriptor pointer
SMC{NAME}_TSTATE          equ     SMCBase+$18     ; [L] interner Senderstatus
SMC{NAME}_TBPTR           equ     SMCBase+$20     ; [W] Tx Buffer Descriptor Pointer
SMC{NAME}_MAX_IDL         equ     SMCBase+$28     ; [W] --UART-- MAximalzahl Idle-Zeichen
SMC{NAME}_IDLC            equ     SMCBase+$28     ; [W] Idle-ZÑhler
SMC{NAME}_BRKLN           equ     SMCBase+$28     ; [W] LÑnge letztes Break-Zeichen
SMC{NAME}_BRKEC           equ     SMCBase+$28     ; [W] Receive Break Condition Counter
SMC{NAME}_BRKCR           equ     SMCBase+$28     ; [W] Sende-Break-ZÑhler
SMC{NAME}_R_mask          equ     SMCBase+$28     ; [W] temp. Bitmaske
SMC{NAME}_M_RxBD          equ     SMCBase+$00     ; [W] --GCI-- Monitor Channel Rx
SMC{NAME}_M_TxBD          equ     SMCBase+$02     ; [W] Monitor Channel Tx
SMC{NAME}_CI_RxBD         equ     SMCBase+$04     ; [W] C/I Channel Rx
SMC{NAME}_CI_TxBD         equ     SMCBase+$06     ; [W] C/I Channel Tx
SMC{NAME}_M_RxD           equ     SMCBase+$0c     ; [W] Monitor Rx Data
SMC{NAME}_M_TxD           equ     SMCBase+$0e     ; [W] Monitor Tx Data
SMC{NAME}_CI_RxD          equ     SMCBase+$10     ; [W] C/I Rx Data
SMC{NAME}_CI_TxD          equ     SMCBase+$12     ; [W] C/I Tx Data
                endm
                __defsmc "1",RegBase+$0682,SMC1Base
                __defsmc "2",RegBase+$0692,SMC2Base

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; SPI:

SPIBase         equ     RAMBase+$0d80

SPMODE          equ     RegBase+$06a0   ; [W] Modusregister
SPIE            equ     RegBase+$06a6   ; [B] Ereignisregister
SPIM            equ     RegBase+$06aa   ; [B] Maskenregister
SPICOM          equ     RegBase+$06ad   ; [B] Kommandoregister
SPI_RBASE       equ     SPIBase+$00     ; [W] Empfangsdeskriptoradresse
SPI_TBASE       equ     SPIBase+$02     ; [W] Sendedeskriptoradresse
SPI_RFCR        equ     SPIBase+$04     ; [B] Empfangsfunktionscode
SPI_TFCR        equ     SPIBase+$05     ; [B] Sendefunktionscode
SPI_MRBLR       equ     SPIBase+$06     ; [W] MaximallÑnge Empfangspuffer
SPI_RSTATE      equ     SPIBase+$08     ; [L] EmpfÑngerstatus
SPI_RBPTR       equ     SPIBase+$10     ; [W] mom. aktiver Empfangsdeskriptor
SPI_TSTATE      equ     SPIBase+$18     ; [L] Senderstatus
SPI_TBPTR       equ     SPIBase+$20     ; [W] mom. aktiver Sendedeskriptor

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; PIP:

PIPBase         equ     SMC2Base

PIPC            equ     RegBase+$06b2   ; [W] Konfigurationsregister
PTPR            equ     RegBase+$06b6   ; [W] Timing-Parameter
PIPE            equ     SMCE2           ; [B] Ereignisregister, Åberlagert!!
PBDIR           equ     RegBase+$06b8   ; [L] Datenrichtungsregister Port B
PBPAR           equ     RegBase+$06bc   ; [L] Zuordnung Port B
PBODR           equ     RegBase+$06c2   ; [W] Steuerbits Port B als open drain
PBDAT           equ     RegBase+$06c4   ; [L] Datenregister Port B
PIP_RBASE       equ     PIPBase+$00     ; [W] Empfangsdeskriptoradresse
PIP_TBASE       equ     PIPBase+$02     ; [W] Sendedeskriptoradresse
PIP_CFCR        equ     PIPBase+$04     ; [B] Funktionscode
PIP_SMASK       equ     PIPBase+$05     ; [B] Statusmaske
PIP_MRBLR       equ     PIPBase+$06     ; [W] MaximallÑnge Empfangspuffer
PIP_RSTATE      equ     PIPBase+$08     ; [L] EmpfÑngerstatus
PIP_R_PTR       equ     PIPBase+$0c     ; [L] int. Empfangsdatenzeiger
PIP_RBPTR       equ     PIPBase+$10     ; [W] mom. Empfangsdeskriptor
PIP_R_CNT       equ     PIPBase+$12     ; [W] Empfangs-Byte-ZÑhler
PIP_RTEMP       equ     PIPBase+$14     ; [L] temp. Speicher
PIP_TSTATE      equ     PIPBase+$18     ; [L] Senderstatus
PIP_T_PTR       equ     PIPBase+$1c     ; [L] mom. Sendedatenzeiger
PIP_TBPTR       equ     PIPBase+$20     ; [W] mom. Sendedeskriptor
PIP_T_CNT       equ     PIPBase+$22     ; [W] Sende-Byte-ZÑhler
PIP_TTEMP       equ     PIPBase+$24     ; [L] temp. Speicher
PIP_MAX_SL      equ     PIPBase+$28     ; [W] max. Ruhezeit
PIP_SL_CNT      equ     PIPBase+$2a     ; [W] RuhezÑhler
PIP_CHARACTER1  equ     PIPBase+$2c     ; [W] Steuerzeichen
PIP_CHARACTER2  equ     PIPBase+$2e
PIP_CHARACTER3  equ     PIPBase+$30
PIP_CHARACTER4  equ     PIPBase+$32
PIP_CHARACTER5  equ     PIPBase+$34
PIP_CHARACTER6  equ     PIPBase+$36
PIP_CHARACTER7  equ     PIPBase+$38
PIP_CHARACTER8  equ     PIPBase+$3a
PIP_RCCM        equ     PIPBase+$3c     ; [W] Steuerzeichenmaske
PIP_RCCR        equ     PIPBase+$3e     ; [W] Steuerzeichenregister

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
; SI:

SIMODE          equ     RegBase+$06e0   ; [L] Modus serielles Interface
SIGMR           equ     RegBase+$06e4   ; [B] globale Moduseinstellung
SISTR           equ     RegBase+$06e6   ; [B] Adresse Router-RAM
SICMR           equ     RegBase+$06e7   ; [B] Kommandoregister serielles Interface
SICR            equ     RegBase+$06ec   ; [L] Taktverteilung serielles Interface
SIRP            equ     RegBase+$06f2   ; [L] RAM-Zeiger
SIRAM           equ     RegBase+$0700   ; [ ] Routing-RAM

;=============================================================================

                case    "68340"

;-----------------------------------------------------------------------------
; die Kommentare fallen hier evtl. etwas spartanischer aus, weil der Technical
; Summary von Motorola fÅr den 68340 noch nicht so ins Detail geht.

SIMBAR          equ     $0003ff00       ; [L] Einstellung Peripherieadressen

MCR             equ     SIMBase+$000    ; [W] Modulkonfiguration SIM

SYNCR           equ     SIMBase+$004    ; [W] Steuerung Taktfrequenz-Synthi
AVR             equ     SIMBase+$006    ; [B] Auto-Vektoren
RSR             equ     SIMBase+$007    ; [B] Reset Status

PORTA           equ     SIMBase+$011    ; [B] Datenregister Port A
DDRA            equ     SIMBase+$013    ; [B] Datenrichtungsregister Port A
PPRA1           equ     SIMBase+$015    ; [B] Festlegung der Funktionen der...
PPRA2           equ     SIMBase+$017    ; [B] ...Leitungen von Port A
PORTB           equ     SIMBase+$019    ; [B] Datenregister Port B
PORTB1          equ     SIMBase+$01b    ; [B] dito
DDRB            equ     SIMBase+$01d    ; [B] Datenrichtungsregister Port B
PPRARB          equ     SIMBase+$01f    ; [B] Festlegung Port B-Leitungen
SWIV            equ     SIMBase+$020    ; [B] Software-Vektoren
SYPCR           equ     SIMBase+$021    ; [B] Systemschutz
PICR            equ     SIMBase+$022    ; [W] Steuerung PIT
PITR            equ     SIMBase+$024    ; [W] Datenregister PIT
SWSR            equ     SIMBase+$027    ; [B] Software Service

;-----------------------------------------------------------------------------
; Chip Selects:

__cnt           set     0
                rept    4
__name           set     "\{__CNT}"
CS{__name}AM1    set     SIMBase+$040+__cnt*8 ; [W] Adressmaske 1 CSn
CS{__name}AM2    set     SIMBase+$042+__cnt*8 ; [W] Adressmaske 2 CSn
CS{__name}BA1    set     SIMBase+$044+__cnt*8 ; [W] Basisadresse 1 CSn
CS{__name}BA2    set     SIMBase+$046+__cnt*8 ; [W] Basisadresse 2 CSn
__cnt            set     __cnt+1
                endm

;-----------------------------------------------------------------------------
; DMA:

DMABase         equ     SIMBase+$780
DMAMCR1         equ     DMABase+$000    ; [W] Modulkonfiguration DMA-Kanal 1
DMAINTR1        equ     DMABase+$004    ; [W] Interrupts DMA-Kanal 1
DMACCR1         equ     DMABase+$008    ; [W] Steuerregister DMA-Kanal 1
DMACSR1         equ     DMABase+$00a    ; [B] Statusregister DMA-Kanal 1
DMAFCR1         equ     DMABase+$00b    ; [B] Funktionscoderegister DMA-Kanal 1
DMASAR1         equ     DMABase+$00c    ; [L] Quelladresse DMA-Kanal 1
DMADAR1         equ     DMABase+$010    ; [L] Zieladresse DMA-Kanal 1
DMABTC1         equ     DMABase+$014    ; [L] BytezÑhler DMA-Kanal 1
DMAMCR2         equ     DMABase+$020    ; Kanal 2 genauso, nur $20 weiter
DMAINTR2        equ     DMABase+$024
DMACCR2         equ     DMABase+$028
DMACSR2         equ     DMABase+$02a
DMAFCR2         equ     DMABase+$02b
DMASAR2         equ     DMABase+$02c
DMADAR2         equ     DMABase+$030
DMABTC2         equ     DMABase+$034

;-----------------------------------------------------------------------------
; serielles:

SMBase          equ     SIMBase+$700
SMMCR           equ     SMBase+$000     ; [W] Modulkonfiguration SM
SMILR           equ     SMBase+$004     ; [B] Interruptebene
SMIVR           equ     SMBase+$005     ; [B] Interruptvektor
SMIPCR          equ     SMBase+$014     ; [BR] Anzeige, ob Pegelwechsel an Leitungen
SMACR           equ     SMBase+$014     ; [BW] Hilfssteuerregister
SMISR           equ     SMBase+$015     ; [BR] Interruptanzeigen
SMIER           equ     SMBase+$015     ; [BW] Interuptfreigaben
SMOPCR          equ     SMBase+$01d     ; [BW] Steuerung Ausgabeports
SMIP            equ     SMBase+$01d     ; [BR] Status Eingabeports
SMOPS           equ     SMBase+$01e     ; [BW] Portbits einzeln setzen
SMOPR           equ     SMBase+$01f     ; [BW] Portbits einzeln zurÅcksetzen
SMMR1A          equ     SMBase+$010     ; [B] Modusregister 1 Kanal A
SMMR2A          equ     SMBase+$020     ; [B] Modusregister 2 Kanal A
SMCSRA          equ     SMBase+$011     ; [BR] Taktauswahl Kanal A
SMSRA           equ     SMBase+$011     ; [BW] Statusregister Kanal A
SMCRA           equ     SMBase+$012     ; [BW] Kommandoregister Kanal A
SMRBA           equ     SMBase+$013     ; [BR] Empfangsdatenregister Kanal A
SMTBA           equ     SMBase+$013     ; [BW] Sendedatenregister Kanal A
SMMR1B          equ     SMBase+$018     ; [B] Modusregister 1 Kanal B
SMMR2B          equ     SMBase+$021     ; [B] Modusregister 2 Kanal B
SMCSRB          equ     SMBase+$019     ; [BR] Taktauswahl Kanal B
SMSRB           equ     SMBase+$019     ; [BW] Statusregister Kanal B
SMCRB           equ     SMBase+$01a     ; [BW] Kommandoregister Kanal B
SMRBB           equ     SMBase+$01b     ; [BR] Empfangsdatenregister Kanal B
SMTBB           equ     SMBase+$01b     ; [BW] Sendedatenregister Kanal B

;-----------------------------------------------------------------------------
; Timer:

TMBase          equ     SIMBase+$600
TM1MCR          equ     TMBase+$000     ; [W] Modulkonfiguration Timer 1
TM1IR           equ     TMBase+$004     ; [W] Interruptkonfiguration Timer 1
TM1CR           equ     TMBase+$006     ; [W] Steuerung Timer 1
TM1SR           equ     TMBase+$008     ; [W] Status/Vorteiler Timer 1
TM1CNTR         equ     TMBase+$00a     ; [W] ZÑhlregister Timer 1
TM1PREL1        equ     TMBase+$00c     ; [W] Vorbelegung 1 Timer 1
TM1PREL2        equ     TMBase+$00e     ; [W] Vorbelegung 2 Timer 1
TM1COM          equ     TMBase+$010     ; [W] Vergleichsregister Timer 1
TM2MCR          equ     TMBase+$040     ; dito Kanal 2
TM2IR           equ     TMBase+$044
TM2CR           equ     TMBase+$046
TM2SR           equ     TMBase+$048
TM2CNTR         equ     TMBase+$04a
TM2PREL1        equ     TMBase+$04c
TM2PREL2        equ     TMBase+$04e
TM2COM          equ     TMBase+$050

;=============================================================================
; ab hier 68332-Register

                case    "68332"

;-----------------------------------------------------------------------------
; die fundamentalen SIM-Steuerregister

SIMCR           equ     SIMBase+$00     ; [W] MCU-Konfiguration
SIYPCR          equ     SIMBase+$21     ; [W] Watchdog, Bus-Monitorsteuerung
SWSR            equ     SIMBase+$27     ; [B] Watchdog zurÅcksetzen (mit $55/$aa)
PICR            equ     SIMBase+$22     ; [W] Interruptsteuerung Timer
PITR            equ     SIMBase+$24     ; [W] ZÑhlwert Timer

;-----------------------------------------------------------------------------
; der Synthesizer fÅr den Prozessortakt

SYNCR           equ     SIMBase+$04     ; [W] Taktfrequenz-Synthesizer-Steuerung

;-----------------------------------------------------------------------------
; die Chip-Select-AusgÑnge

CSPAR0          equ     SIMBase+$44     ; [W] Steuerung der AusgÑnge CSBOOT,CS0..CS5
CSPAR1          equ     SIMBase+$46     ; [W] Steuerung der AusgÑnge CS6..CS10
CSBARBT         equ     SIMBase+$48     ; [W] Startadresse Boot-ROM
CSORBT          equ     SIMBase+$4a     ; [W] Optionen Boot-ROM
__cnt           set     0
                rept    10              ; nur 0..9 so erzeugen, damit keine Hex-Namen
__name           set     "\{__CNT}"
CSBAR{__name}    equ     SIMBase+$4c+__cnt*4 ; [W] Startadresse CSn
CSOR{__name}     equ     SIMBase+$4e+__cnt*4 ; [W] Optionen CSn
__cnt            set     __cnt+1
                endm
CSBAR10         equ     SIMBase+$74     ; [W] Startadresse CS10
CSOR10          equ     SIMBase+$76     ; [W] Optionen CS10

;-----------------------------------------------------------------------------
; Nutzung der SIM-Bits als einfache I/O-Ports

PORTC           equ     SIMBase+$41     ; [B] Datenbits Port C
PORTE0          equ     SIMBase+$11     ; [B] Datenbits Port E
PORTE1          equ     SIMBase+$13     ; [B] dito
DDRE            equ     SIMBase+$15     ; [B] Datenrichtungsbits Port E
PEPAR           equ     SIMBase+$17     ; [B] Steuerung Port E-Pins als Ports oder Bussignale
PORTF0          equ     SIMBase+$19     ; [B] Datenbits Port F
PORTF1          equ     SIMBase+$1b     ; [B] dito
DDRF            equ     SIMBase+$1d     ; [B] Datenrichtungsbits Port F
PFPAR           equ     SIMBase+$1f     ; [B] Steuerung Port F-Pins als Ports oder Bussignale

;-----------------------------------------------------------------------------
; Register fÅr den Boundary-Scan-Test des SIM (for Motorola use only...)

SIMTR           equ     SIMBase+$02     ; [W] Testregister SIM
SIMTRE          equ     SIMBase+$08     ; [W] Testregister E-Takt
TSTMSRA         equ     SIMBase+$30     ; [W] Schieberegister A (Boundary Scan)
TSTMSRB         equ     SIMBase+$32     ; [W] Schieberegister B (Boundary Scan)
TSTSC           equ     SIMBase+$34     ; [W] Register Schiebezahl
TSTRC           equ     SIMBase+$36     ; [W] Register Wiederholungszahl
CREG            equ     SIMBase+$38     ; [W] Steuerregister Boundary Scan
DREG            equ     SIMBase+$3a     ; [W] verteiltes Register (?!)

;-----------------------------------------------------------------------------
; programmierbare Zeitgeber:

TPUBase         equ     SIMBase+$400    ; Basisadresse TPU-Registersatz
TPUMCR          equ     TPUBase+$00     ; [W] Basiskonfiguration TPU
TICR            equ     TPUBase+$08     ; [W] Interruptsteuerung TPU
CIER            equ     TPUBase+$0a     ; [W] TPU Interrupt-Freigaben
CISR            equ     TPUBase+$20     ; [W] TPU Interrupt-Anzeigen
CFSR0           equ     TPUBase+$0c     ; [W] TPU Betriebsarten Kanal 12..15
CFSR1           equ     TPUBase+$0e     ; [W] TPU Betriebsarten Kanal  8..11
CFSR2           equ     TPUBase+$10     ; [W] TPU Betriebsarten Kanal  4.. 7
CFSR3           equ     TPUBase+$12     ; [W] TPU Betriebsarten Kanal  0.. 3
HSQR0           equ     TPUBase+$14     ; [W] TPU Sub-Betriebsarten Kanal 8..15
HSQR1           equ     TPUBase+$16     ; [W] TPU Sub-Betriebsarten Kanal 0.. 7
HSRR0           equ     TPUBase+$18     ; [W] TPU Service-Anfragebits Kanal 8..15
HSRR1           equ     TPUBase+$1a     ; [W] TPU Service-Anfragebits Kanal 0.. 7
CPR0            equ     TPUBase+$1c     ; [W] TPU PrioritÑt Kanal 8..15
CPR1            equ     TPUBase+$1e     ; [W] TPU PrioritÑt Kanal 0.. 7
DSCR            equ     TPUBase+$04     ; [W] Register fÅr Entwicklung und Test ?!
DSSR            equ     TPUBase+$06     ; eine genauere Beschreibung habe ich
LR              equ     TPUBase+$22     ; leider nicht...
SGLR            equ     TPUBase+$24     ;
DCNR            equ     TPUBase+$26     ;
TCR             equ     TPUBase+$02     ;

;-----------------------------------------------------------------------------
; das Kommando-RAM fÅr die TPU:

TPURAMBase      equ     SIMBase+$100    ; Basisadresse TPURAM-Steuerregister
TRAMMCR         equ     TPURAMBase+$00  ; [B] Basiskonfiguration TPURAM
TRAMTST         equ     TPURAMBase+$02  ; [W] Testregister TPURAM
TRAMBAR         equ     TPURAMBase+$04  ; [W] Basisadresse TPURAM

;-----------------------------------------------------------------------------
; serielles:

QSMBase         equ     SIMBase+$200    ; Basisadresse serielle E/A-Einheit
QSMCR           equ     QSMBase+$00     ; [W] Basiskonfiguration QSM
QTEST           equ     QSMBase+$02     ; [W] QSM Testregister
QILR            equ     QSMBase+$04     ; [B] QSM InterruptprioritÑten
QIVR            equ     QSMBase+$05     ; [B] QSM Interruptvektor
PORTQS          equ     QSMBase+$15     ; [B] Datenbits QSM paralleler Port
PQSPAR          equ     QSMBase+$16     ; [B] Umschaltung Portbits QSM/paralleler Port
DDRQS           equ     QSMBase+$17     ; [B] Datenrichtungsregister Port QSM
SPCR0           equ     QSMBase+$18     ; [W] Steuerregister 0 QSPI
SPCR1           equ     QSMBase+$1a     ; [W] Steuerregister 1 QSPI
SPCR2           equ     QSMBase+$1c     ; [W] Steuerregister 2 QSPI
SPCR3           equ     QSMBase+$1e     ; [B] Steuerregister 3 QSPI
SPSR            equ     QSMBase+$1f     ; [B] Statusregister QSPI
__cnt           set     0               ; Definition QSPI-RAM
                rept    16
__name           set     "\{__CNT}"
RR{__name}       equ     QSMBase+$100+__cnt*2 ; [W] Daten-RAM Empfangsseite
TR{__name}       equ     QSMBase+$120+__cnt*2 ; [W] Daten-RAM Sendeseite
CR{__name}       equ     QSMBase+$140+__cnt   ; [B] Befehls-RAM
__cnt            set     __cnt+1
                endm
SCCR0           equ     QSMBase+$08     ; [W] Steuerregister 0 SCI
SCCR1           equ     QSMBase+$0a     ; [W] Steuerregister 1 SCI
SCSR            equ     QSMBase+$0c     ; [W] Statusregister SCI
SCDR            equ     QSMBase+$0e     ; [W] Datenregister SCI

;-----------------------------------------------------------------------------

                endcase                 ; von Prozessorunterscheidung

                endif                   ; von ifdef ...

		restore                 ; wieder erlauben


