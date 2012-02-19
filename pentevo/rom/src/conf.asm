
; ------- definitions

; -- ports
extp            equ 0xAF
pf7             equ 0xF7

    
; -- F7 port regs
shadow          equ 0xEF
nvaddr          equ 0xDF
nvdata          equ 0xBF


; F7 parameters
shadow_on       equ 0x80
shadow_off      equ 0x00


; -- TS-config port regs
vconf           equ 0x00
vpage           equ 0x01
xoffs           equ 0x02
xoffsl          equ 0x02
xoffsh          equ 0x03
yoffs           equ 0x04
yoffsl          equ 0x04
yoffsh          equ 0x05
tsconf          equ 0x06
palsel          equ 0x07
border          equ 0x0F
    
page0           equ 0x10
page1           equ 0x11
page2           equ 0x12
page3           equ 0x13
fmaddr          equ 0x15
tgpage          equ 0x18
dmasal          equ 0x1A
dmasah          equ 0x1B
dmasax          equ 0x1C
dmadal          equ 0x1D
dmadah          equ 0x1E
dmadax          equ 0x1F
sysconf         equ 0x20
memconf         equ 0x21
hsint           equ 0x22
vsint           equ 0x23
vsintl          equ 0x23
vsinth          equ 0x24
intv            equ 0x25
dmal            equ 0x26
dmac            equ 0x27
xtoverride      equ 0x2A


; TS parameters
fm_en           equ 0x10
dma_zwt_en      equ 0x40


; video modes
rres_256x192    equ 0x00
rres_320x200    equ 0x40
rres_320x240    equ 0x80
rres_360x288    equ 0xC0
mode_zx         equ 0
mode_16c        equ 1
mode_256c       equ 2
mode_text       equ 3
mode_nogfx      equ 0x20
    
; -- RAM windows
win0            equ 0x0000
win1            equ 0x4000
win2            equ 0x8000
win3            equ 0xC000


; -- addresses
pal_addr        equ h'6000      ; \ 
fat_bufs        equ h'4000      ; | 
res_buf         equ h'5C00      ; |
nv_buf          equ h'5D00      ; | LSB should be 0 !!
vars            equ h'5B00      ; /

stck            equ h'5BFF
nv_1st          equ h'B0


; -- video config
txpage          equ h'FF
pal_sel         equ h'F


; -- UI colors
box_norm        equ h'8F
opt_norm        equ h'89
opt_hgl         equ h'79
sel_norm        equ h'8F


; events
ev_kb_help      equ h'01
ev_kb_up        equ h'02
ev_kb_down      equ h'03
ev_kb_enter     equ h'04

