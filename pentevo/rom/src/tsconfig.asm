
; -----------------------------------
; -- RAM addresses
stackp          equ 0x6000
res_buf         equ 0x5000
sys_var         equ 0x5C00
rslsys_addr     equ 0x4000

; -----------------------------------
; -- ports
EXTP            equ 0xAF
PF7             equ 0xF7

; -- F7 port regs
SHADOW          equ 0xEF
NVADDR          equ 0xDF
NVDATA          equ 0xBF

; F7 parameters
SHADOW_ON       equ 0x80
SHADOW_OFF      equ 0x00

; -----------------------------------
; -- TS-config regs
STATUS          equ 0x00AF
DSTATUS         equ 0x27AF

VCONFIG         equ 0x00AF
VPAGE           equ 0x01AF
XOFFS           equ 0x02AF
XOFFSL          equ 0x02AF
XOFFSH          equ 0x03AF
YOFFS           equ 0x04AF
YOFFSL          equ 0x04AF
YOFFSH          equ 0x05AF
TSCONF          equ 0x06AF
PALSEL          equ 0x07AF
TMPAGE          equ 0x08AF
T0GPAGE         equ 0x09AF
T1GPAGE         equ 0x0AAF
SGPAGE          equ 0x0BAF
BORDER          equ 0x0FAF
PAGE0           equ 0x10AF
PAGE1           equ 0x11AF
PAGE2           equ 0x12AF
PAGE3           equ 0x13AF
FMADDR          equ 0x15AF
DMASADDRL       equ 0x1AAF
DMASADDRH       equ 0x1BAF
DMASADDRX       equ 0x1CAF
DMADADDRL       equ 0x1DAF
DMADADDRH       equ 0x1EAF
DMADADDRX       equ 0x1FAF
SYSCONFIG       equ 0x20AF
MEMCONFIG       equ 0x21AF
HSINT           equ 0x22AF
VSINT           equ 0x23AF    ;alias
VSINTL          equ 0x23AF
VSINTH          equ 0x24AF
INTV            equ 0x25AF
DMALEN          equ 0x26AF
DMACTR          equ 0x27AF
DMANUM          equ 0x28AF
FDDVIRT         equ 0x29AF

; -----------------------------------
; TS parameters
FM_EN           equ 0x10

; video modes
RRES_256X192    equ 0x00
RRES_320X200    equ 0x40
RRES_320X240    equ 0x80
RRES_360X288    equ 0xC0
MODE_ZX         equ 0
MODE_16C        equ 1
MODE_256C       equ 2
MODE_TEXT       equ 3
MODE_NOGFX      equ 0x20

; DMA modes
DMA_WNR         equ 0x80
DMA_ZWT         equ 0x40
DMA_SALGN       equ 0x20
DMA_DALGN       equ 0x10
DMA_ASZ         equ 0x08
DMA_DEV_MEM     equ 0x01
DMA_DEV_SPI     equ 0x02
DMA_DEV_IDE     equ 0x03
DMA_DEV_CRM     equ 0x04
DMA_DEV_SFL     equ 0x05

; -----------------------------------
; -- RAM windows
WIN0            equ 0x0000
WIN1            equ 0x4000
WIN2            equ 0x8000
WIN3            equ 0xC000

; -----------------------------------
; -- h/w constants
pal_seg         equ 0x00        ; memory segment used for CRAM addressing

; -- video config
txpage          equ 0xF6
pal_sel         equ 0xF

; -- pages config
vrompage        equ 0xF8

; -----------------------------------
; -- UI colors
box_norm        equ 0x8F
opt_norm        equ 0x89
opt_hgl         equ 0x79
sel_norm        equ 0x8F
err_norm        equ 0x0A

; -----------------------------------
; events
ev_kb_help      equ 0x01
ev_kb_up        equ 0x02
ev_kb_down      equ 0x03
ev_kb_enter     equ 0x04
