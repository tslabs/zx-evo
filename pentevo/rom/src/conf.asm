; TS-config extended ports definition
extp:           equ 0xAF
    
vconfig:        equ 0x00
vpage:          equ 0x01
xoffs:          equ 0x02
xoffsl:         equ 0x02
xoffsh:         equ 0x03
yoffs:          equ 0x04
yoffsl:         equ 0x04
yoffsh:         equ 0x05
tsconf:         equ 0x06
palsel:         equ 0x07
border:         equ 0x0F
    
page0:          equ 0x10
page1:          equ 0x11
page2:          equ 0x12
page3:          equ 0x13
fmaddr:         equ 0x15
tgpage:         equ 0x18
dmasal:         equ 0x1A
dmasah:         equ 0x1B
dmasax:         equ 0x1C
dmadal:         equ 0x1D
dmadah:         equ 0x1E
dmadax:         equ 0x1F
    
sysconf:        equ 0x20
memconf:        equ 0x21
hsint:          equ 0x22
vsint:          equ 0x23
vsintl:         equ 0x23
vsinth:         equ 0x24
intv:           equ 0x25
dmal:           equ 0x26
dmac:           equ 0x27

xtoverride:     equ 0x2A    
    
dma_zwt_en:     equ 0x40





; some constants
win0:           equ 0x0000
win1:           equ 0x4000
win2:           equ 0x8000
win3:           equ 0xC000
