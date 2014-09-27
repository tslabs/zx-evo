
; -----------------------------------
; -- RAM addresses
stackp          equ h'6000
res_buf         equ h'5000
sys_var         equ h'5C00
rslsys_addr     equ h'4000

; -----------------------------------
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

; -----------------------------------
; -- TS-config regs
xstatus         equ 0x00
dstatus         equ 0x27

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
tmpage          equ 0x08
t0gpage         equ 0x09
t1gpage         equ 0x0A
sgpage          equ 0x0B
border          equ 0x0F
page0           equ 0x10
page1           equ 0x11
page2           equ 0x12
page3           equ 0x13
fmaddr          equ 0x15
dmasal          equ 0x1A
dmasah          equ 0x1B
dmasax          equ 0x1C
dmadal          equ 0x1D
dmadah          equ 0x1E
dmadax          equ 0x1F
sysconf         equ 0x20
memconf         equ 0x21
hsint           equ 0x22
vsint           equ 0x23    ;alias
vsintl          equ 0x23
vsinth          equ 0x24
intv            equ 0x25
dmalen          equ 0x26
dmactr          equ 0x27
dmanum          equ 0x28
fddvirt         equ 0x29

; -----------------------------------
; TS parameters
fm_en           equ 0x10

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

; DMA modes
dma_wnr         equ 0x80
dma_zwt         equ 0x40
dma_salgn       equ 0x20
dma_dalgn       equ 0x10
dma_asz         equ 0x08
dma_dev_mem     equ 0x01
dma_dev_spi     equ 0x02
dma_dev_ide     equ 0x03
dma_dev_crm     equ 0x04
dma_dev_sfl     equ 0x05

; -----------------------------------
; -- RAM windows
win0            equ 0x0000
win1            equ 0x4000
win2            equ 0x8000
win3            equ 0xC000

; -----------------------------------
; -- h/w constants
pal_seg         equ h'00        ; memory segment used for CRAM addressing

; -- video config
txpage          equ h'F6
pal_sel         equ h'F

; -- pages config
vrompage        equ h'F8

; -----------------------------------
; -- UI colors
box_norm        equ h'8F
opt_norm        equ h'89
opt_hgl         equ h'79
sel_norm        equ h'8F
err_norm        equ h'0A

; -----------------------------------
; events
ev_kb_help      equ h'01
ev_kb_up        equ h'02
ev_kb_down      equ h'03
ev_kb_enter     equ h'04

