                save
                listing off   ; kein Listing Åber diesen File

;****************************************************************************
;*                                                                          *
;*   AS 1.40 - Datei STDDEF60.INC                                           *
;*   								            *
;*   Sinn : enthÑlt Makrodefinitionen fÅr den PowerPC                       *
;* 									    *
;*   letzte énderungen : 29. 5.1994                                         *
;*                       26.12.1994 PPC403-Erweiterungen                    *
;*                       28.12.1994 MPC505-Erweiterungen                    *
;*                       10. 3.1999 PPC403G[BC]-Erweiterungen               *
;*                       28. 3.1999 Korrektur: MMU erst ab PPC403GC         *
;*                                                                          *
;****************************************************************************

                ifndef  stddef60inc     ; verhindert Mehrfacheinbindung

stddef60inc     equ     1

                if      (MOMCPU<>1537)&&(MOMCPU<>24576)&&(MOMCPU<>1027)&&(MOMCPU<>1285)&&(MOMCPU<>16443)&&(MOMCPU<>16444)
                 fatal  "Falscher Prozessortyp eingestellt: nur MPC601, MPC505, PPC403 oder RS6000 erlaubt!"
		endif


                if      MOMPASS=1
                 message "PowerPC-Makro-Definitionen (C) 1994 Alfred Arnold"
                 switch  MOMCPU
                 case    0x6000
                  message "Zielsystem RS6000"
                 case    0x601
                  message "Zielsystem MPC601"
                 case    0x505
                  message "Zielsystem MPC505"
                 case    0x403,0x403c
                  message "Zielsystem PPC403Gx"
                 endcase
		endif

		
;============================================================================
; Device-Control-Register

__defdcr        macro   NAME,val,{NoExpand}
NAME            equ     val
mt{"NAME"}      macro      reg
                mtdcr   NAME,reg
                endm
mf{"NAME"}      macro      reg
                mfdcr   reg,NAME
                endm
		endm

                if      (MOMCPU=0x403)||(MOMCPU=0x403c)
                 __defdcr BEAR,0x90     ; Adresse Busfehler
                 __defdcr BESR,0x91     ; Syndrom Busfehler
                 __defdcr BR0,0x80      ; Bank-Register 0..7
                 __defdcr BR1,0x81
                 __defdcr BR2,0x82
                 __defdcr BR3,0x83
                 __defdcr BR4,0x84
                 __defdcr BR5,0x85
                 __defdcr BR6,0x86
                 __defdcr BR7,0x87
                 __defdcr DMACC0,0xc4   ; DMA KettenzÑhler (?)
                 __defdcr DMACR0,0xc0   ; DMA Steuerregister Kanal 0..3
                 __defdcr DMACR1,0xc8
                 __defdcr DMACR2,0xd0
                 __defdcr DMACR3,0xd8
                 __defdcr DMACT0,0xc1   ; DMA ZÑhlregister Kanal 0..3
                 __defdcr DMACT1,0xc9
                 __defdcr DMACT2,0xd1
                 __defdcr DMACT3,0xd9
                 __defdcr DMADA0,0xc2   ; DMA Zieladresse Kanal 0..3
                 __defdcr DMADA1,0xca
                 __defdcr DMADA2,0xd2
                 __defdcr DMADA3,0xda
                 __defdcr DMASA0,0xc3   ; DMA Quelladresse Kanal 0..3
                 __defdcr DMASA1,0xcb
                 __defdcr DMASA2,0xd3
                 __defdcr DMASA3,0xdb
                 __defdcr DMASR,0xe0    ; DMA Statusregister
                 __defdcr EXISR,0x40    ; Anzeige externer Interrupts
                 __defdcr EXIER,0x42    ; Freigabe externer Interrupts
                 __defdcr IOCR,0xa0     ; I/O-Konfiguration
                endif

;============================================================================
; Special-Purpose-Register

__defspr        macro   NAME,val,{NoExpand}
NAME            equ     val
mt{"NAME"}      macro      reg
                mtspr   NAME,reg
                endm
mf{"NAME"}      macro      reg
                mfspr   reg,NAME
                endm
		endm

                __defspr XER,0x001      ; Integer Exception Register
                __defspr LR,0x008       ; RÅcksprungadresse
                __defspr CTR,0x009      ; ZÑhlregister
                __defspr SRR0,0x01a     ; Save/Restore-Register
                __defspr SRR1,0x01b
                __defspr SPRG0,0x110    ; Special Purpose-Register
                __defspr SPRG1,0x111
                __defspr SPRG2,0x112
                __defspr SPRG3,0x113
                switch MOMCPU
                case 0x403,0x403c
                 __defspr DAC1,0x3f6    ; Datenadresse Vergleichsregister
                 __defspr DAC2,0x3f7
                 __defspr DBCR,0x3f2    ; Debug-Steuerregister
                 __defspr DBSR,0x3f0    ; Debug-Statusregister
                 __defspr DCCR,0x3fa    ; Steuerung Daten-Cache
                 __defspr DEAR,0x3d5    ; Exception-Adresse Datenzugriff
                 __defspr ESR,0x3d4     ; Exception-Syndrom Datenzugriff
                 __defspr EVPR,0x3d6    ; Exception-Vektor-PrÑfix
                 __defspr IAC1,0x3f4    ; Codeadresse Vergleichsregister
                 __defspr IAC2,0x3f5
                 __defspr ICCR,0x3fb    ; Steuerung Code-Cache
                 __defspr PBL1,0x3fc    ; Untergrenzen
                 __defspr PBL2,0x3fe
                 __defspr PBU1,0x3fd    ; Obergrenzen
                 __defspr PBU2,0x3ff
                 __defspr PIT,0x3db     ; Timer
                 __defspr PVR,0x11f     ; Prozessorversion
                 __defspr SRR2,0x3de    ; Save/Restore-Register
                 __defspr SRR3,0x3df
                 __defspr TBHI,0x3dc    ; Zeitbasis
                 __defspr TBLO,0x3dd
                 __defspr TCR,0x3da     ; Timer Steuerregister
                 __defspr TSR,0x3d8     ; Timer Statusregister
                 __defspr SGR,0x3b9     ; ????
                case 0x505
                 __defspr TBL,268       ; Zeitbasis
                 __defspr TBU,269
                 __defspr DSISR,18      ; zeigt Grund von Alignment-Exceptions an
                 __defspr DAR,19        ; Fehlerhafte Datenadresse nach Exception
                 __defspr DEC,22        ; zÑhlt mit 1 MHz
                 __defspr EIE,80        ; Freigabe externer Interrupts
                 __defspr EID,81        ; Sperre externer Interrupts
                 __defspr NRE,82        ; Non-recoverable Exception
                 __defspr TBL_S,284     ; nochmal Zeitbasis ?!
                 __defspr TBU_S,285
                 __defspr PVR,287       ; Prozessorversion
                 __defspr ICCST,560     ; Steuerung & Status Instruktionscache
                 __defspr ICADR,561     ; Instruktionscache Adre·register
                 __defspr ICDAT,562     ; Instruktionscache Datenregister
                 __defspr FPECR,1022    ; Gleitkommaexception
                 __defspr CMPA,144      ; Vergleicherwert A..D
                 __defspr CMPB,145
                 __defspr CMPC,146
                 __defspr CMPD,147
                 __defspr ECR,148       ; Grund der Debug-Exception
                 __defspr DER,149       ; Freigabe Debug-Features
                 __defspr COUNTA,150    ; Breakpoint ZÑhler
                 __defspr COUNTB,151
                 __defspr CMPE,152      ; Vergleicherwert E..G
                 __defspr CMPF,153
                 __defspr CMPG,154
                 __defspr CMPH,155
                 __defspr LCTRL1,156    ; Debug-Steuerung Vergleicher L-Bus
                 __defspr LCTRL2,157
                 __defspr ICTRL,158     ; Debug-Steuerung I-Bus
                 __defspr BAR,159       ; Breakpoint-Adresse
                 __defspr DPDR,630      ; Development-Port Daten
                 __defspr DPIR,631      ;     "        "   Instruktionen
                case 0x601
                 __defspr RTCU,0x004    ; ZÑhler
                 __defspr RTCL,0x005
                 __defspr DEC,0x006
                 __defspr DSISR,0x012
                 __defspr DAR,0x013
                 __defspr DEC2,0x016
                 __defspr SDR1,0x019
                 __defspr EAR,0x11a
                 __defspr BAT0U,0x210
                 __defspr BAT0L,0x211
                 __defspr BAT1U,0x212
                 __defspr BAT1L,0x213
                 __defspr BAT2U,0x214
                 __defspr BAT2L,0x215
                 __defspr BAT3U,0x216
                 __defspr BAT3L,0x217
                 __defspr HID0,0x3f0
                 __defspr HID1,0x3f1
                 __defspr HID2,0x3f2
                 __defspr HID5,0x3f5
                 __defspr HID15,0x3ff
                case 0x6000
                 __defspr MQ,0x000      ; obere HÑlfte Divident/Produkt
                endcase
                if MOMCPU=0x403c
                 __defspr pid, 0x3b1
                endif

;============================================================================
; serieller Port PPC403:

                if      (MOMCPU=0x403)||(MOMCPU=0x403c)
spls             equ     0x40000000     ; Leitungsstatus
sphs             equ     0x40000002     ; Status Handshake-Leitungen
brdh             equ     0x40000004     ; Baudratenteiler
brdl             equ     0x40000005
spctl            equ     0x40000006     ; Steuerregister
sprc             equ     0x40000007     ; Kommandoregister EmpfÑnger
sptc             equ     0x40000008     ; Kommandoregister Sender
sprb             equ     0x40000009     ; Sende/Empfangspuffer
sptb             equ     sprb
                endif

;============================================================================
; SIU MPC505:
; erinnert irgendwie an die vom 6833x...

                if      MOMCPU=0x505
siumcr           equ     0x8007fc00     ; Basissteuerregister
siutest1         equ     0x8007fc04
memmap           equ     0x8007fc20     ; Speicherlayout
specaddr         equ     0x8007fc24     ; erlauben Sperren spekulativer
specmask         equ     0x8007fc28     ; LadevorgÑnge
termstat         equ     0x8007fc2c
picsr            equ     0x8007fc40     ; Steuerung periodische Interrupts
pit              equ     0x8007fc44     ; ZÑhlwert periodischer Interrupt-Timer
bmcr             equ     0x8007fc48     ; Steuerung Bus-Monitor
rsr              equ     0x8007fc4c     ; Reset-Status
sccr             equ     0x8007fc50     ; Steuerung Systemtakt
sccsr            equ     0x8007fc54     ; Status Systemtakt
portbase         equ     0x8007fc60
ddrm             equ     portbase+0x00  ; Datenrichtungsregister Port M
pmpar            equ     portbase+0x04  ; Pinzuordnung Port M
portm            equ     portbase+0x08  ; Datenregister Port M
papar            equ     portbase+0x24  ; Pinzuordnung Port A+B
pbpar            equ     papar
porta            equ     portbase+0x28  ; Datenregister Port A+B
portb            equ     porta
ddri             equ     portbase+0x38  ; Datenrichtungsregister Port I..L
ddrj             equ     ddri
ddrk             equ     ddri
ddrl             equ     ddri
pipar            equ     portbase+0x38  ; Pinzuordnung Port I..L
pjpar            equ     pipar
pkpar            equ     pipar
plpar            equ     pipar
porti            equ     portbase+0x40  ; Datenregister Port I..L
portj            equ     porti
portk            equ     porti
portl            equ     porti
csbase           equ     0x8007fd00
csbtbar          equ     csbase+0xf8    ; Basisadresse Boot-EPROM
csbtsbbar        equ     csbase+0xf0
csbar1           equ     csbase+0xe0    ; Basisadressen /CS1../CS5
csbar2           equ     csbase+0xd8
csbar3           equ     csbase+0xd0
csbar4           equ     csbase+0xc8
csbar5           equ     csbase+0xc0
csbtor           equ     csbase+0xfc    ; Optionen Boot-EPROM
csbtsbor         equ     csbase+0xf4
csor0            equ     csbase+0xec    ; Optionen /CS1../CS11
csor1            equ     csbase+0xe4
csor2            equ     csbase+0xdc
csor3            equ     csbase+0xd4
csor4            equ     csbase+0xcc
csor5            equ     csbase+0xc4
csor6            equ     csbase+0xbc
csor7            equ     csbase+0xb4
csor8            equ     csbase+0xac
csor9            equ     csbase+0xa4
csor10           equ     csbase+0x9c
csor11           equ     csbase+0x94
                endif

;----------------------------------------------------------------------------
; Peripheral Control Unit MPC505:

                if      MOMCPU=0x505
pcubase          equ     0x8007ef80
pcumcr           equ     pcubase+0x00   ; Basiskonfiguration
tstmsra          equ     pcubase+0x10
tstmsrb          equ     tstmsra
tstcntrab        equ     pcubase+0x14
tstreps          equ     tstcntrab
tstcreg1         equ     pcubase+0x18
tstcreg2         equ     tstcreg1
tstdreg          equ     pcubase+0x1c
irqpend          equ     pcubase+0x20   ; auszufÅhrende Interrupts
irqand           equ     pcubase+0x24   ; freigegebene&auszufÅhrende Interrupts
irqenable        equ     pcubase+0x28   ; freigegebene Interrupts
pitqil           equ     pcubase+0x2c   ; Interruptebene PortQ/PIT
swsr             equ     pcubase+0x40   ; Reload Watchdog auslîsen
swcr             equ     pcubase+0x44   ; Steuerung Watchdog
swtc             equ     swcr
swr              equ     pcubase+0x48
pqedgdat         equ     pcubase+0x50   ; Flankenauswahl PortQ
pqpar            equ     pcubase+0x54   ; Pinzuordnung PortQ
                endif

;----------------------------------------------------------------------------
; SRAM-Modul MPC505:

                if      MOMCPU=0x505
srammcr          equ     0x8007f00      ; Basiskonfiguration SRAM
                endif

;============================================================================
; SUBI geht sowohl mit 2 als auch 3 Argumenten

subi            macro   dest,src,VAL
                if      "VAL"=""
                 addi   dest,dest,-src
                elseif
                 addi   dest,src,-VAL
                endif
                endm

;----------------------------------------------------------------------------
; Vergleiche

cmpw            macro   cr,REG1,REG2
                if      "REG2"=""
                cmp     0,0,cr,REG1
                elseif
                cmp     cr,0,REG1,REG2
                endif
                endm

cmpwi           macro   cr,REG1,IMM
                if      "IMM"=""
                cmpi    0,0,cr,IMM
                elseif
                cmpi    cr,0,REG1,imm
                endif
                endm

cmplw           macro   cr,REG1,REG2
                if      "REG2"=""
                cmpl    0,0,cr,REG1
                elseif
                cmpl    cr,0,REG1,REG2
                endif
                endm

cmplwi          macro   cr,REG1,IMM
                if      "IMM"=""
                cmpli   0,0,cr,IMM
                elseif
                cmpli   cr,0,REG1,IMM
                endif
                endm

;----------------------------------------------------------------------------
; erweiterte Befehle Bedingungsregister

crset           macro   bx
                creqv   bx,bx,bx
                endm

crnot           macro   bx,by
                crnor   bx,by,by
                endm

crmove          macro   bx,by
                cror    bx,by,by
                endm

;----------------------------------------------------------------------------
; erweiterte Befehle Logik

not             macro   dest,SRC
                if      "SRC"=""
                 nor    dest,dest
                elseif
                 nor    dest,SRC,SRC
                endif
                endm

not.            macro   dest,SRC
                if      "SRC"=""
                 nor.   dest,dest
                elseif
                 nor.   dest,SRC,SRC
                endif
                endm

mr              macro   dest,src
                or      dest,src,src
                endm

mr.             macro   dest,src
                or.     dest,src,src
                endm

nop             macro
                ori     0,0,0
                endm

;----------------------------------------------------------------------------
; vereinfachte Einschiebebefehle

inslwi          macro   ra,rs,n,b
                rlwimi  ra,rs,32-b,b,b+n-1
                endm
inslwi.         macro   ra,rs,n,b
                rlwimi. ra,rs,32-b,b,b+n-1
                endm

insrwi          macro   ra,rs,n,b
                rlwimi  ra,rs,32-b-n,b,b+n-1
                endm
insrwi.         macro   ra,rs,n,b
                rlwimi. ra,rs,32-b-n,b,b+n-1
                endm

__defins1       macro   NAME,par1,par2,par3,{NoExpand}
{"NAME"}        macro   ra,rs,n
                rlwinm  ra,rs,par1,par2,par3
                endm
{"NAME"}.       macro   ra,rs,n
                rlwinm. ra,rs,par1,par2,par3
                endm
                endm

__defins2       macro   NAME,par1,par2,par3,{NoExpand}
{"NAME"}        macro   ra,rs,b,n
                rlwinm  ra,rs,par1,par2,par3
                endm
{"NAME"}.       macro   ra,rs,b,n
                rlwinm. ra,rs,par1,par2,par3
                endm
                endm

                __defins1 clrlwi,0,n,31
                __defins2 clrlslwi,n,b-n,31-n
                __defins1 clrrwi,0,0,31-n
                __defins2 extlwi,b,0,n-1
                __defins2 extrwi,b+n,32-n,31
                __defins1 rotlwi,n,0,31
                __defins1 rotrwi,32-n,0,31
                __defins1 slwi,n,0,31-n
                __defins1 srwi,32-n,n,31

rotlw           macro   ra,rs,rb
                rlwnm   ra,rs,rb,0,31
                endm
rotlw.          macro   ra,rs,rb
                rlwnm.  ra,rs,rb,0,31
                endm

;----------------------------------------------------------------------------
; vereinfachte SprÅnge

__defjmp1       macro   NAME,m1,m2,{NoExpand}
{"NAME"}        macro   adr
                bc      m1,m2,adr
                endm
{"NAME"}a       macro   adr
                bca     m1,m2,adr
                endm
{"NAME"}l       macro   adr
                bcl     m1,m2,adr
                endm
{"NAME"}la      macro   adr
                bcla    m1,m2,adr
                endm
                endm

                __defjmp1 bdnz,16,0
                __defjmp1 bdz,18,0

__defjmp2       macro   NAME,m1,{NoExpand}
{"NAME"}        macro   cr,adr
                bc      m1,cr,adr
                endm
{"NAME"}a       macro   cr,adr
                bca     m1,cr,adr
                endm
{"NAME"}l       macro   cr,adr
                bcl     m1,cr,adr
                endm
{"NAME"}la      macro   cr,adr
                bcla    m1,cr,adr
                endm
                endm

                __defjmp2 bdnzf,0
                __defjmp2 bdnzt,8
                __defjmp2 bdzf,2
                __defjmp2 bdzt,10
                __defjmp2 bf,4
                __defjmp2 bt,12

__defjmp3       macro   NAME,mask,ofs,{NoExpand}
{"NAME"}        macro   cr,ADR
                if      "ADR"=""
                 bc      mask,ofs,cr
                elseif
                 bc      mask,cr*4+ofs,adr
                endif
                endm
{"NAME"}a       macro   cr,ADR
                if      "ADR"=""
                 bca     mask,ofs,cr
                elseif
                 bca     mask,cr*4+ofs,adr
                endif
                endm
{"NAME"}l       macro   cr,ADR
                if      "ADR"=""
                 bcl     mask,ofs,cr
                elseif
                 bcl     mask,cr*4+ofs,adr
                endif
                endm
{"NAME"}la      macro   cr,ADR
                if      "ADR"=""
                 bcla    mask,ofs,cr
                elseif
                 bcla    mask,cr*4+ofs,adr
                endif
                endm
{"NAME"}ctr     macro   CR
                if      "CR"=""
                 bcctr   mask,ofs
                elseif
                 bc      mask,CR*4+ofs
                endif
                endm
{"NAME"}ctrl    macro   CR
                if      "CR"=""
                 bcl     mask,ofs
                elseif
                 bcl     mask,CR*4+ofs
                endif
                endm
{"NAME"}lr      macro   CR
                if      "CR"=""
                bclr    mask,ofs
                elseif
                bclr    mask,4*CR+ofs
                endif
                endm
{"NAME"}lrl     macro   CR
                if      "CR"=""
                bclrl   mask,ofs
                elseif
                bclrl   mask,4*CR+ofs
                endif
                endm
                endm

                __defjmp3 beq,12,2
                __defjmp3 bge,4,0
                __defjmp3 bgt,12,1
                __defjmp3 ble,4,1
                __defjmp3 blt,12,0
                __defjmp3 bne,4,2
                __defjmp3 bng,4,1
                __defjmp3 bnl,4,0
                __defjmp3 bns,4,3
                __defjmp3 bnu,4,3
                __defjmp3 bso,12,3
                __defjmp3 bun,12,3

bctr            macro
                bcctr   20,0
                endm
bctrl           macro
                bcctrl  20,0
                endm

__defjmp4       macro   NAME,mask,{NoExpand}
{"NAME"}        macro   cr
                bcctr   mask,cr
                endm
{"NAME"}l       macro   cr
                bcctrl  mask,cr
                endm
                endm

                __defjmp4 bfctr,4
                __defjmp4 btctr,12

__defjmp6       macro   NAME,mask,bit,{NoExpand}
{"NAME"}        macro
                bclr    mask,bit
                endm
{"NAME"}l       macro
                bclrl   mask,bit
                endm
                endm

                __defjmp6 blr,20,0
                __defjmp6 bdnzlr,16,0
                __defjmp6 bdzlr,18,0

__defjmp7       macro   NAME,mask,{NoExpand}
{"NAME"}        macro   cr
                bclr    mask,cr
                endm
{"NAME"}l       macro   cr
                bclrl   mask,cr
                endm
                endm

                __defjmp7 bdnzflr,0
                __defjmp7 bdnztlr,8
                __defjmp7 bdzflr,2
                __defjmp7 bdztlr,10
                __defjmp7 bflr,4
                __defjmp7 btlr,12

;-------------------------------------------------------------------------
; Traps

trap            macro   ra,rb
                tw      31,ra,rb
                endm

__deftrap       macro   NAME,mask,{NoExpand}
{"NAME"}        macro   ra,rb
                tw      mask,ra,rb
                endm
{"NAME"}i       macro   ra,im
                twi     mask,ra,im
                endm
                endm

                __deftrap tweq,4
                __deftrap twge,12
                __deftrap twgt,8
                __deftrap twle,20
                __deftrap twlge,5
                __deftrap twlgt,1
                __deftrap twlle,6
                __deftrap twllt,2
                __deftrap twlng,6
                __deftrap twlnl,5
                __deftrap twlt,16
                __deftrap twne,24
                __deftrap twng,20
                __deftrap twnl,12

;-------------------------------------------------------------------------
; MMU-Makros PPC403G[BC]

		if (MOMCPU=0x403c)||(MOMCPU=0x403c)

tlbrehi		macro	rt,ra
		tlbre	rt,ra,0
		endm

tlbrelo		macro	rt,ra
		tlbre	rt,ra,1
		endm

tlbwehi		macro	rt,ra
		tlbwe	rt,ra,0
		endm

tlbwelo		macro	rt,ra
		tlbwe	rt,ra,1
		endm

		endif

;=========================================================================

                endif

                restore                 ; Listing wieder erlauben

