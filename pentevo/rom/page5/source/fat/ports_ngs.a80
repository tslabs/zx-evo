
; ports description and include file for
; NeoGS software projects, v0.3
;
;
; bits degisnation:
; B_* -bit position (0,1,2,3,4,5,6,7)
; M_* -bit mask (1,2,4,8,0X10,
;                           0X20,0X40,0X80)
;
; C_* - constants to be used
;
;
; part of NeoGS project
;
; (c) 2008 NedoPC

;---------------------------------------

;ZX-side ports

GSCOM		EQU 0XBB		; write-only, command for NGS

GSSTAT		EQU 0XBB		; read-only, command and data bits
				; (positions given immediately below)

B_CBIT		EQU 0		; Command position
M_CBIT		EQU 1		; BIT:AND Mask

B_DBIT		EQU 7		; Data position
M_DBIT		EQU 0X80		; BIT and mask

GSDAT		EQU 0XB3		; read-write
				; data transfer register for NGS

GSCTR		EQU 0X33		; write-only, control register for NGS:
				; constants available given immediately below

C_GRST		EQU 0X80		; reset constant to be written into

C_GNMI		EQU 0X40		; NMI constant to be written into GSCTR

C_GLED		EQU 0X20		; LED toggle constant

;---------------------------------------

;GS-side ports

MPAG		EQU 0X00		; write-only, Memory PAGe ;port (big
				; pages at 8000-FFFF or small at 8000-BFFF)

MPAGEX		EQU 0X10		; write-only, Memory PAGe EXtended
				; (only small pages at C000-FFFF)

ZXCMD		EQU 0X01		; read-only, ZX CoMmanD port: here is
				; the byte written by ZX into GSCOM

ZXDATRD		EQU 0X02		; read-only, ZX DATa ReaD: a byte
				; written by ZX into GSDAT appears here
				; upon reading this port, data bit is cleared

ZXDATWR		EQU 0X03		; write-only, ZX DATa WRite: a byte
				; written here is available for ZX in
				; GSDAT upon writing here, data bit is set

ZXSTAT		EQU 0X04		; read-only, read ZX STATus: command and
				; data bits. positions are defined by
				; *_CBIT and *_DBIT above

CLRCBIT		EQU 0X05		; read-write, upon either reading or
				; writing this port, the Command BIT is CLeaRed
VOL1		EQU 0X06
VOL2		EQU 0X07
VOL3		EQU 0X08
VOL4		EQU 0X09
VOL5		EQU 0X16
VOL6		EQU 0X17
VOL7		EQU 0X18
VOL8		EQU 0X19		; write-only, volumes for sound channels 1-8

; following two ports are useless and
; very odd. They have been made just
; because they were on the original GS
; and for that strange case when
; somebody too crazy have used them.
; Nevertheless, DO NOT USE THEM! They
; can disappear or even radically change
; functionality in future firmware
; releases.

DPORT1		EQU 0X0A		; DAMNPORT1
				; writing or reading this port sets data
				; bit to the inverse of bit 0 into MPAG
				; port

DPORT2		EQU 0X0B		; DAMNPORT2
				; the same as DAMNPORT1, but instead
				; command bit involved, which is made
				; equal to 5th bit of VOL4

LEDCTR		EQU 0X01		; write-only, controls on-board LED.
				; D0=0 - LED is on, D0=1 - LED is off
				; reset state is LED on.

GSCFG0		EQU 0X0F		; read-write, GS ConFiG port 0: acts as
				; memory cell, reads previously written
				; value. Bits and fields follow:

B_NOROM		EQU 0		; =0 - there is ROM everywhere except 0X4000-7FFF,
				; =1 - the RAM is all around
M_NOROM		EQU 1

B_RAMRO		EQU 1		; =1 - ram absolute adresses 0X0000-7FFF
				; (zeroth big page) are write-protected
M_RAMRO		EQU 2

B_8CHAN		EQU 2		; B_8CHANS
				; =1 - 8 channels mode
M_8CHAN		EQU 4		; M_8CHANS

B_EXPAG		EQU 3		; =1 - extended paging: both MPAG and
				; MPAGEX are used to switch two memory windows
M_EXPAG		EQU 8

B_CKSL0		EQU 4		; B_CKSEL0
				; these bits should be set according to
				; the C_**MHZ constants below
M_CKSL0		EQU 0X10		; M_CKSEL0

B_CKSL1		EQU 5		; B_CKSEL1
M_CKSL1		EQU 0X20		; M_CKSEL1

C_10MHZ		EQU 0X30
C_12MHZ		EQU 0X10
C_20MHZ		EQU 0X20
C_24MHZ		EQU 0X00

B_PAN4C		EQU 6		; B_PAN4CH
				; =1 - 4 channels, panning (every
				; channel is on left and right with two volumes)
M_PAN4C		EQU 0X40		; M_PAN4CH

B_INV7B		EQU 7		;B_INV7B
				; =1 - invert 7th bit of sample before
				; putting them to MUL/DAC
M_INV7B		EQU 0X80

B_SNCLR		EQU 7		; B_SETNCLR
M_SNCLR		EQU 0X80		; M_SETNCLR

SCTRL		EQU 0X11		; Serial ConTRoL: read-write, read:
				; current state of below bits, write - see GS_info

B_SDNCS		EQU 0
M_SDNCS		EQU 1

B_MCNCS		EQU 1
M_MCNCS		EQU 2

B_MPXRS		EQU 2
M_MPXRS		EQU 4

B_MCSP0		EQU 3		; B_MCSPD0
M_MCSP0		EQU 8		; M_MCSPD0

B_MDHLF		EQU 4
M_MDHLF		EQU 0X10

B_MCSP1		EQU 5		; B_MCSPD1
M_MCSP1		EQU 0X20		; M_MCSPD1

SSTAT		EQU 0X12		; Serial STATus: read-only, reads state of below bits

B_MDDRQ		EQU 0
M_MDDRQ		EQU 1

B_SDDET		EQU 1
M_SDDET		EQU 2

B_SDWP		EQU 2
M_SDWP		EQU 4

B_MCRDY		EQU 3
M_MCRDY		EQU 8

SD_SEND		EQU 0X13		; SD card SEND, write-only, when
				; written, byte transfer starts with
				; written byte

SD_READ		EQU 0X13		; SD card READ, read-only, reads byte
				; received in previous byte transfer

SD_RSTR		EQU 0X14		; SD card Read and STaRt, read-only,
				; reads previously received byte and
				; starts new byte transfer with 0XFF

MD_SEND		EQU 0X14		; Mp3 Data SEND, write-only, sends byte
				; to the mp3 data interface

MC_SEND		EQU 0X15		; Mp3 Control SEND, write-only, sends
				; byte to the mp3 control interface

MC_READ		EQU 0X15		; Mp3 Control READ, read-only, reads
				; byte that was received during
				; previous sending of byte

DMA_MOD		EQU 0X1B		; DMA MODULE

DMA_HAD		EQU 0X1C		; DMA High ADdress

DMA_MAD		EQU 0X1D		; DMA Middle ADdress

DMA_LAD		EQU 0X1E		; DMA Low ADdress

DMA_CST		EQU 0X1F		; DMA Control and STate
