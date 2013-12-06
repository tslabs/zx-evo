
; ------- definitions

; -- TS-config port regs

VCONFIG         equ $00AF
STATUS          equ $00AF
VPAGE           equ $01AF
XOFFSL          equ $02AF
XOFFSH          equ $03AF
YOFFSL          equ $04AF
YOFFSH          equ $05AF
TSCONFIG        equ $06AF
PALSEL          equ $07AF
BORDER          equ $0FAF
PAGE0           equ $10AF
PAGE1           equ $11AF
PAGE2           equ $12AF
PAGE3           equ $13AF
FMADDR          equ $15AF
TMPAGE          equ $16AF
T0GPAGE         equ $17AF
T1GPAGE         equ $18AF
SGPAGE          equ $19AF
DMASADDRL       equ $1AAF
DMASADDRH       equ $1BAF
DMASADDRX       equ $1CAF
DMADADDRL       equ $1DAF
DMADADDRH       equ $1EAF
DMADADDRX       equ $1FAF
SYSCONFIG       equ $20AF
MEMCONFIG       equ $21AF
HSINT           equ $22AF
VSINTL          equ $23AF
VSINTH          equ $24AF
IM2VECT         equ $25AF
DMALEN          equ $26AF
DMACTR          equ $27AF
DMASTATUS       equ $27AF
DMANUM          equ $28AF
FDDVIRT         equ $29AF
INTMASK         equ $2AAF

; TS parameters
FM_EN           equ $10

; VIDEO
VID_256X192     equ $00
VID_320X200     equ $40
VID_320X240     equ $80
VID_360X288     equ $C0

VID_ZX          equ $00
VID_16C         equ $01
VID_256C        equ $02
VID_TEXT        equ $03
VID_NOGFX       equ $20

; TSU
TSU_T0ZEN		equ $04
TSU_T1ZEN		equ $08
TSU_T0EN		equ $20
TSU_T1EN		equ $40
TSU_SEN			equ $80

; SYSTEM
SYS_ZCLK3_5		equ $00
SYS_ZCLK7		equ $01
SYS_ZCLK14		equ $02

SYS_CACHEEN		equ $04

; MEMORY
MEM_ROM128		equ $01
MEM_W0WE		equ $02
MEM_W0MAP_N		equ $04
MEM_W0RAM		equ $08

MEM_LCK512		equ $00
MEM_LCK128		equ $40
MEM_LCKAUTO		equ $80
MEM_LCK1024		equ $C0

; INT
INT_FRAME		equ $00
INT_LINE		equ $10
INT_DMA			equ $20

; DMA
DMA_WNR         equ $80
DMA_SALGN       equ $20
DMA_DALGN       equ $10
DMA_ASZ         equ $08

DMA_RAM         equ $01
DMA_BLT         equ $81
DMA_SPI_RAM     equ $02
DMA_RAM_SPI     equ $82
DMA_IDE_RAM     equ $03
DMA_RAM_IDE     equ $83
DMA_FILL_RAM    equ $04
DMA_RAM_CRAM    equ $84
DMA_RAM_SFILE   equ $85

