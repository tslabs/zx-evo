
; ------- definitions

; -- TS-config port regs

VCONFIG         equ $00AF
STATUS          equ $00AF
VPAGE           equ $01AF
GXOFFSL         equ $02AF
GXOFFSH         equ $03AF
GYOFFSL         equ $04AF
GYOFFSH         equ $05AF
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
DMALEN          equ $26AF
DMACTR          equ $27AF
DMASTATUS       equ $27AF
DMANUM          equ $28AF
FDDVIRT         equ $29AF
INTMASK         equ $2AAF
T0XOFFSL        equ $40AF
T0XOFFSH        equ $41AF
T0YOFFSL        equ $42AF
T0YOFFSH        equ $43AF
T1XOFFSL        equ $44AF
T1XOFFSH        equ $45AF
T1YOFFSL        equ $46AF
T1YOFFSH        equ $47AF

; TS parameters
FM_EN           equ $10

; VIDEO
VID_256X192     equ $00
VID_320X200     equ $40
VID_320X240     equ $80
VID_360X288     equ $C0
VID_RASTER_BS	equ 6

VID_ZX          equ $00
VID_16C         equ $01
VID_256C        equ $02
VID_TEXT        equ $03
VID_NOGFX       equ $20
VID_MODE_BS		equ 0

; PALSEL
PAL_GPAL_MASK	equ $0F
PAL_GPAL_BS		equ 0
PAL_T0PAL_MASK	equ $30
PAL_T0PAL_BS	equ 4
PAL_T1PAL_MASK	equ $C0
PAL_T1PAL_BS	equ 6

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
SYS_ZCLK_BS		equ 0

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
MEM_LCK_BS		equ 6

; INT
INT_VEC_FRAME	equ $FF
INT_VEC_LINE	equ $FD
INT_VEC_DMA		equ $FB

INT_MSK_FRAME	equ $01
INT_MSK_LINE	equ $02
INT_MSK_DMA		equ $04

; DMA
DMA_WNR         equ $80
DMA_SALGN       equ $20
DMA_DALGN       equ $10
DMA_ASZ         equ $08

DMA_RAM         equ $01
DMA_BLT         equ $81
DMA_FILL	    equ $04
DMA_SPI_RAM     equ $02
DMA_RAM_SPI     equ $82
DMA_IDE_RAM     equ $03
DMA_RAM_IDE     equ $83
DMA_RAM_CRAM    equ $84
DMA_RAM_SFILE   equ $85

; SPRITES
SP_XF			equ $80
SP_YF			equ $80
SP_LEAP			equ $40
SP_ACT			equ $20

SP_SIZE8		equ $00
SP_SIZE16		equ $02
SP_SIZE24		equ $04
SP_SIZE32		equ $06
SP_SIZE40		equ $08
SP_SIZE48		equ $0A
SP_SIZE56		equ $0C
SP_SIZE64		equ $0E
SP_SIZE_BS		equ 1

SP_PAL_MASK		equ $F0

SP_XF_W			equ $8000
SP_YF_W			equ $8000
SP_LEAP_W		equ $4000
SP_ACT_W		equ $2000

SP_SIZE8_W		equ $0000
SP_SIZE16_W		equ $0200
SP_SIZE24_W		equ $0400
SP_SIZE32_W		equ $0600
SP_SIZE40_W		equ $0800
SP_SIZE48_W		equ $0A00
SP_SIZE56_W		equ $0C00
SP_SIZE64_W		equ $0E00
SP_SIZE_BS_W	equ 9

SP_X_MASK_W		equ $01FF
SP_Y_MASK_W		equ $01FF
SP_TNUM_MASK_W	equ $0FFF
SP_PAL_MASK_W	equ $F000

; TILES
TL_XF			equ $40
TL_YF			equ $80
TL_PAL_MASK		equ $30
TL_PAL_BS		equ 4

TL_XF_W			equ $4000
TL_YF_W			equ $8000

TL_TNUM_MASK_W	equ $0FFF
TL_PAL_MASK_W	equ $3000
TL_PAL_BS_W		equ 12
