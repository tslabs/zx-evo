
; ext modules
extern	font8
extern	pal32
extern	scrl0
extern	scrl1
#include "conf.asm"
#include "macro.asm"


; params
im2_addr:   equ 0x6FFF
im2_addrh:  equ 0x6F
im2_vect:   equ 0xFF


        rseg	CODE
        
; start initialisation
        di
        call ports_init
        call scrl0_init
        call scrl1_init
        call im2_init
        ei
        ret


; lower scroll init

scrl1_init:
; vpage 8 will be used for text mode

; copy font to page 9
        xtr
        xt page3, 0x09          ; RAM page 9 on at #C000
        ld hl, font8
        ld de, win3
        ld bc, 0x800        ; 2048 bytes of font
        ldir
        
; copy text to page 8
        xtr
        xt page3, 0x08
        ld hl, scrl1
        ld de, win3
        
        ld b, 64                ; num of lines
s1i_01:
        ld a, b
        and 15                  ; color selection
        jr nz, s1i_00
        inc a
s1i_00:
        push bc
        ld bc, 128
        ldir
        
; prepare attributes for lower scroller
        push hl
        ld h, d
        ld l, e
        inc de
        ld (hl), a
        ld c, 127
        ldir
        pop hl

        pop bc
        djnz s1i_01
        
; palette for textmod init
        xtr
        xt fmaddr, 0x10		; open FMAddr window for writing at 0x0000

; generate textmode palette into cells 0xD0..DF
        ld b, 15
        ld hl, 0x300                ; 48k ROM
        ld de, 0x1A0

        xor a
        ld (de), a
        inc de
        ld (de), a
        inc de
        
s1i_02:
        ld a,r
        xor (hl)
        inc hl
        and 0x18                  ;gggBBbbb
        ld (de), a
        inc de
        ld a,r
        xor (hl)
        inc hl
        and 0x63                  ;0RRrrrGG
        ld (de), a
        inc de
        djnz s1i_02

; close FMAddr window
        xtr
        xt fmaddr, 0

        xor a
        ld (scr1_offs), a   ; nulling offset
        ld (scr1_offs+1), a
        ld (scr1_cnt), a
        ret


scrl0_init:
; vpage 16 will be used for 16c

; nulling the scroll buffer - 8192 bytes (256x32)
        xtr
        xt page3, 0x10          ; RAM page 16 at #C000
        ld hl, win3
        ld de, win3+1
        ld bc, 8191
        ld (hl), 0
        ldir

        
; palette for 16c initialisation
        xtr
        xt fmaddr, 0x10     ; open FMAddr window for writing at 0x0000
        ld hl, pal32
        ld de, 0x1C0
        ld bc, 0x20
        ldir                ; copy 16c palette into cells 0xE0..EF

; close FMAddr window
        xtr
        xt fmaddr, 0

        xor a
        ld (scr0_offs), a   ; nulling offset and counter
        ld (scr0_offs+1), a
        ld hl, scrl0
        ld (scr0_addr), hl  ; set initial addr
        ret
        
        
; interrupts initialisation
im2_init:
; prepare IM2 table
        ld hl, int0
        ld (im2_addr), hl
        
; set INT to start of frame
        
        ld a, im2_addrh
        ld i, a
        im 2
        ret

        
; ports initialisation
ports_init:
        xtr
        xt hsint, 0     ;set int position to line start
        xtw vsint, 0

; something to add here
        ret


; int0: start of frame
; coordinates 0, 0
int0:
        rst 56                      ; keyboard
        di
        
        push af
        push bc
        push hl

        xtr
        xt palsel, 15               ; pal 15
        xt vpage, 16                ; vpage = 16
        xt vconfig, 0               ; mode 6912
        xthl xoffsl, scr0_offs      ; X offset = (scr0_offs)
        xtw yoffs, 0                ; Y offset = 0
        xtw vsint, 39               ; move INT and its address to next
        ld hl, int1
        ld (im2_addr), hl
        
        call scrl1_proc            ; scrolls processing
        call scrl0_proc
        call z, scrl0_char         ; if next move will be last (Z=1) we need to update next char

        pop hl
        pop bc
        pop af
        ei
        ret
        
        
; int1: begin of scroller 0
; coordinates 0, 39
int1:
        push af
        push bc
        push hl
        
        xtr
        xt vconfig, 0xC1		; mode 16c, 360x288
        xt palsel, 14			; pal 14
        xtw vsint, 71	        ; move INT and its address to next
        ld hl, int2
        ld (im2_addr), hl
        
        pop hl
        pop bc
        pop af
        ei
        ret

        
; int2: end of scroller 0
; coordinates 0, 71
int2:
        push af
        push bc
        push hl
        
        xtr
        xt palsel, 15			; pal 15
        xt vpage, 5				; vpage = 5
        xt vconfig, 0			; mode 6912
        xtw xoffs, 0            ; X offset = 0
        xtw yoffs, 0            ; Y offset = 0
        xtw vsint, 271          ; move INT and its address to next
        ld hl, int3
        ld (im2_addr), hl
        
        pop hl
        pop bc
        pop af
        ei
        ret

        
; int3: begin of scroller 1
; coordinates 0, 271

int3:
        push af
        push bc
        push hl

        xtr
        xthl yoffsl, scr1_offs     ; Y offset = scr1_offs
        xt vconfig, 0xC3           ; mode text, 360x288
        xt palsel, 13              ; pal 13
        xt vpage, 8                ; vpage = 8
        xtw vsint, 0               ; move INT and its address to next
        ld hl, int0
        ld (im2_addr), hl
        
        pop hl
        pop bc
        pop af
        ei
        ret

        
; scroll 0 processing
scrl0_proc:

        ld a, (scr0_offs)
        add a, 4
        ld (scr0_offs), a
        ld b,a
        
        ld a, (scr0_offs+1)
        adc a, 0
        and 1
        ld (scr0_offs+1), a
        
        ld a, b
        and 0x1F
        cp 0x1C
        ret
        
        
; update next char
scrl0_char:
        ; ei                          ; since this procedure takes a while, we must not miss next INT
        push de
        
        ld hl, (scr0_addr)
        inc hl
s0c_01:
        ld a, (hl)
        or a
        jr nz, s0c_00
        
        ld hl, scrl0
        jr s0c_01
s0c_00:
        ld (scr0_addr), hl
        
; calculate address for char within font
        sub 32
        ld h, a
        ld l, 0
        add hl, hl          ; 512 bytes per char
        ; here bits 7:6 in H are intensionally ignored - they are ignored by HW

; calculate page
; 16c 32x32 font loaded to the pages 10 and 11
        rlca
        rlca
        rlca
        and 1
        or 10

        xtbc dmasal
        out (c), l
        inc b
        out (c), h
        inc b
        out (c), a
        xt dmal, 7      ; DMA burt = 16 bytes with

; calculate address in screen        
        ld a, (scr0_offs+1)
        rrca
        ld a, (scr0_offs)
        rra
        add a, 180
        ld e, a
        ld d, 0xC0
        xt dmadax, 0x10             ; page 16
        
; copying 16x32 bytes of char
        ld a, dma_zwt_en | 1         ; DMA burt with RAM-to-RAM with wait
        ld l, 32
s0c_02:
        ld b, dmadal
        out (c), e
        inc b
        out (c), d
        ld b, dmac
        out (c), a
        inc d
        dec l
        jr nz, s0c_02
        
        pop de
        ret


; scroll 1 processing
scrl1_proc:
        ld a, (scr1_cnt)
        inc a
        and 3
        ld (scr1_cnt), a
        ret nz
        
        ld hl, (scr1_offs)
        inc hl
        ld (scr1_offs), hl
        ret

        
; local variables
scr0_offs:          equ $           ; 2 bytes
scr0_addr:          equ $+2         ; 2 bytes
scr1_offs:          equ $+4         ; 2 bytes
scr1_cnt:           equ $+6         ; 1 byte

		
        end