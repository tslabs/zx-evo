
  output "main.bin"
  include "tsconfig.asm"

BALL_SIZE       equ     10
BAT_SIZE        equ     50
BAT_MAX_SPEED   equ     15

                org     $6000
                jp      main

  include "ft_func.asm"

main            ei
                ld      hl, vm_800_600_60Hz
                ; ld      hl, vm_1024_768_76Hz
                call    ft_init

                ft_wr16 FT_REG_SOUND,0x6100
                ft_wr8  FT_REG_PLAY,1
                ft_wr8  FT_REG_VOL_SOUND,0xff

                ft_wr8  FT_REG_INT_MASK,FT_INT_SWAP
                ft_wr8  FT_REG_INT_EN,1

                call    load_displaylist

                ld      bc,VCONFIG
                ld      a,VID_FT812
                out     (c),a
                ld      bc,INTMASK
                ld      a,INT_MSK_LINE
                out     (c),a

;---------------

loop
                ; --- for hardware interrupt
                ; halt
                ; ft_rd8  FT_REG_INT_FLAGS
                ; ---
                
                ; --- for software interrupt polling
                call wait_int
                ; ---

                ld      a,4
                out     (0xfe),a
                call    load_displaylist
                ld      a,1
                out     (0xfe),a
;------ ball x-move and x-collisions
                ld      de,(ball_dirx)
                ld      a,d
                or      a
                jp      m,l_06

                ld      hl,(ball_x)
                ld      bc,740
                xor     a
                sbc     hl,bc
                jp      m,l_02
                ld      bc,10
                xor     a
                sbc     hl,bc
                jp      p,l_02

                ld      hl,(ball_y)
                ld      bc,BALL_SIZE
                add     hl,bc
                ld      bc,(rbat)
                xor     a
                sbc     hl,bc
                jp      c,l_12
                ld      bc,BAT_SIZE+BALL_SIZE
                xor     a
                sbc     hl,bc
                jp      nc,l_12
                ld      a,1
                ld      (snd_fx),a
                ld      hl,(ball_diry)
                ld      de,(rbat_dir)
                sra     d
                rr      e
                jr      nz,l_01
                ld      e,1
l_01
                ld      a,r
                and     0x02
                xor     e
                ld      e,a
                add     hl,de
                ld      (ball_diry),hl
                ld      hl,-10
                ld      (ball_dirx),hl
                ex      de,hl
                jp      l_12
l_02
                ld      hl,(ball_x)
                ld      bc,790
                xor     a
                sbc     hl,bc
                jp      m,l_05
                ld      bc,10
                xor     a
                sbc     hl,bc
                jp      p,l_05
                ;rbat lose
                ld      a,2
                ld      (snd_fx),a
                ld      a,(lscore)
                cp      0x99
                jr      nz,l_03
                xor     a
                ld      (lscore),a
                ld      (rscore),a
                jr      l_04
l_03            add     a,1
                daa
                ld      (lscore),a
l_04            exx
                call    printscore
                exx
                jp      l_12
l_05
                ld      hl,(ball_x)
                ld      bc,1010
                xor     a
                sbc     hl,bc
                jp      m,l_12
                ;lbat feed
                ld      a,1
                ld      (snd_fx),a
                ld      hl,(lbat)
                ld      bc,BAT_SIZE/2
                add     hl,bc
                ld      (ball_y),hl
                ld      hl,(lbat_dir)
                ld      a,r
                and     0x02
                xor     l
                ld      l,a
                ld      (ball_diry),hl
                ld      hl,40
                ld      (ball_x),hl
                jp      l_12

l_06
                ld      hl,(ball_x)
                ld      bc,50
                xor     a
                sbc     hl,bc
                jp      m,l_08
                ld      bc,10
                xor     a
                sbc     hl,bc
                jp      p,l_08

                ld      hl,(ball_y)
                ld      bc,BALL_SIZE
                add     hl,bc
                ld      bc,(lbat)
                xor     a
                sbc     hl,bc
                jp      c,l_12
                ld      bc,BAT_SIZE+BALL_SIZE
                xor     a
                sbc     hl,bc
                jp      nc,l_12
                ld      a,1
                ld      (snd_fx),a
                ld      hl,(ball_diry)
                ld      de,(lbat_dir)
                sra     d
                rr      e
                jr      nz,l_07
                ld      e,1
l_07
                ld      a,r
                and     0x02
                xor     e
                ld      e,a
                add     hl,de
                ld      (ball_diry),hl
                ld      hl,10
                ld      (ball_dirx),hl
                ex      de,hl
                jp      l_12
l_08
                ld      hl,(ball_x)
                bit     7,h
                jp      nz,l_11
                ld      bc,10
                xor     a
                sbc     hl,bc
                jp      p,l_11
                ;lbat lose
                ld      a,2
                ld      (snd_fx),a
                ld      a,(rscore)
                cp      0x99
                jr      nz,l_09
                xor     a
                ld      (lscore),a
                ld      (rscore),a
                jr      l_10
l_09            add     a,1
                daa
                ld      (rscore),a
l_10            exx
                call    printscore
                exx
                jp      l_12
l_11
                ld      hl,(ball_x)
                ld      bc,-220
                xor     a
                sbc     hl,bc
                jp      p,l_12
                ;rbat feed
                ld      a,1
                ld      (snd_fx),a
                ld      hl,(rbat)
                ld      bc,BAT_SIZE/2
                add     hl,bc
                ld      (ball_y),hl
                ld      hl,(rbat_dir)
                ld      a,r
                and     0x02
                xor     l
                ld      l,a
                ld      (ball_diry),hl
                ld      hl,750
                ld      (ball_x),hl

l_12
                ld      hl,(ball_x)
                add     hl,de
                ld      (ball_x),hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                set     6,h
                ld      (dl_ball1+2),hl
                ld      de,(BALL_SIZE<<4)>>1
                add     hl,de
                ld      (dl_ball2+2),hl
;------ ball y-move
                ld      de,(ball_diry)
                ld      hl,(ball_y)
                xor     a
                adc     hl,de
                ld      (ball_y),hl
                jp      m,l_23

                ld      bc,600-BALL_SIZE
                xor     a
                sbc     hl,bc
                jp      m,l_26

                ld      a,1
                ld      (snd_fx),a
                ld      (ball_y),bc
                ld      a,e
                cp      8
                jr      c,l_21
                ld      a,8
l_21            dec     a
                jr      nz,l_22
                inc     a
l_22            ld      e,a
                ld      hl,0
                xor     a
                sbc     hl,de
                ld      (ball_diry),hl
                jp      l_26
l_23
                ld      a,1
                ld      (snd_fx),a
                ld      hl,0
                ld      (ball_y),hl
                xor     a
                sbc     hl,de
                ld      a,l
                cp      8
                jr      c,l_24
                ld      a,8
l_24            dec     a
                jr      nz,l_25
                inc     a
l_25            ld      l,a
                ld      (ball_diry),hl
l_26
                ld      hl,(ball_y)
                add     hl,hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                ld      (dl_ball1),hl
                ld      de,BALL_SIZE<<4
                add     hl,de
                ld      (dl_ball2),hl
;------ left bat move and AI
                ld      a,(ball_dirx+1)
                or      a
                jp      p,l_31

                ld      bc,1
                ld      hl,(ball_y)
                ld      de,(lbat)
                xor     a
                sbc     hl,de
                jp      c,l_33
                ld      de,BAT_SIZE-BALL_SIZE
                xor     a
                sbc     hl,de
                jp      nc,l_34
l_31
                ld      de,1
                ld      hl,(lbat_dir)
                bit     7,h
                jr      nz,l_32
                xor     a
                sbc     hl,de
                jp      z,l_35
                xor     a
                sbc     hl,de
                jp      z,l_35
                ld      (lbat_dir),hl
                jp      l_35
l_32            xor     a
                adc     hl,de
                jp      z,l_35
                xor     a
                adc     hl,de
                jp      z,l_35
                ld      (lbat_dir),hl
                jp      l_35
l_33
                ld      hl,(lbat_dir)
                xor     a
                sbc     hl,bc
                ld      (lbat_dir),hl
                ld      de,-BAT_MAX_SPEED
                xor     a
                sbc     hl,de
                jp      p,l_35
                ex      de,hl
                ld      (lbat_dir),hl
                jp      l_35
l_34
                ld      hl,(lbat_dir)
                add     hl,bc
                ld      (lbat_dir),hl
                ld      de,BAT_MAX_SPEED
                xor     a
                sbc     hl,de
                jp      m,l_35
                ex      de,hl
                ld      (lbat_dir),hl
l_35
                ld      hl,(lbat)
                ld      de,(lbat_dir)
                add     hl,de
                bit     7,h
                jp      z,l_36

                ld      hl,1
                ld      (lbat_dir),hl
                ld      hl,0
                jp      l_37
l_36
                ex      de,hl
                ld      hl,-(600-BAT_SIZE)
                add     hl,de
                ex      de,hl
                bit     7,d
                jp      nz,l_37

                ld      hl,-1
                ld      (lbat_dir),hl
                ld      hl,600-BAT_SIZE
l_37
                ld      (lbat),hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                ld      (dl_lbat1),hl
                ld      de,BAT_SIZE<<4
                add     hl,de
                ld      (dl_lbat2),hl
;------ right bat move and AI
                ld      a,(ball_dirx+1)
                or      a
                jp      m,l_41

                ld      bc,1
                ld      hl,(ball_y)
                ld      de,(rbat)
                xor     a
                sbc     hl,de
                jp      c,l_43
                ld      de,BAT_SIZE-BALL_SIZE
                xor     a
                sbc     hl,de
                jp      nc,l_44
l_41
                ld      de,1
                ld      hl,(rbat_dir)
                bit     7,h
                jr      nz,l_42
                xor     a
                sbc     hl,de
                jp      z,l_45
                xor     a
                sbc     hl,de
                jp      z,l_45
                ld      (rbat_dir),hl
                jp      l_45
l_42            xor     a
                adc     hl,de
                jp      z,l_45
                xor     a
                adc     hl,de
                jp      z,l_45
                ld      (rbat_dir),hl
                jp      l_45
l_43
                ld      hl,(rbat_dir)
                xor     a
                sbc     hl,bc
                ld      (rbat_dir),hl
                ld      de,-BAT_MAX_SPEED
                xor     a
                sbc     hl,de
                jp      p,l_45
                ex      de,hl
                ld      (rbat_dir),hl
                jp      l_45
l_44
                ld      hl,(rbat_dir)
                add     hl,bc
                ld      (rbat_dir),hl
                ld      de,BAT_MAX_SPEED
                xor     a
                sbc     hl,de
                jp      m,l_45
                ex      de,hl
                ld      (rbat_dir),hl
l_45
                ld      hl,(rbat)
                ld      de,(rbat_dir)
                add     hl,de
                bit     7,h
                jp      z,l_46

                ld      hl,1
                ld      (rbat_dir),hl
                ld      hl,0
                jp      l_47
l_46
                ex      de,hl
                ld      hl,-(600-BAT_SIZE)
                add     hl,de
                ex      de,hl
                bit     7,d
                jp      nz,l_47

                ld      hl,-1
                ld      (rbat_dir),hl
                ld      hl,600-BAT_SIZE
l_47
                ld      (rbat),hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                add     hl,hl
                ld      (dl_rbat1),hl
                ld      de,BAT_SIZE<<4
                add     hl,de
                ld      (dl_rbat2),hl
;------
                ld      a,2
                out     (0xfe),a

                ld      a,(snd_fx)
                or      a
                jr      z,nosfx
                dec     a
                jr      nz,sfx2
                ft_wr16 FT_REG_SOUND,0x0051
                jr      sfx0
sfx2            ft_wr16 FT_REG_SOUND,0x0050
sfx0            ft_wr8  FT_REG_PLAY,1
                xor     a
                ld      (snd_fx),a
nosfx
                xor     a
                out     (0xfe),a
                jp      loop

;---------------

printscore      ld      hl,dl_lscore
                ld      de,4
                ld      a,(lscore)
                call    bcdbyte
                ld      a,(rscore)
bcdbyte         push    af
                rrca
                rrca
                rrca
                rrca
                call    bcdhalf
                pop     af
bcdhalf         and     0x0f
                add     a,0xb0
                ld      (hl),a
                add     hl,de
                ret

;---------------

load_displaylist:
                FT_ON
                ld a, (FT_RAM_DL >> 16) | 0x80
                out (SPI_DATA), a
                ld a, (FT_RAM_DL >> 8) & 0xFF
                out (SPI_DATA), a
                ld a, FT_RAM_DL & 0xFF
                out (SPI_DATA), a
                ld hl, dl1_start
                ld a, DL1_SIZE
                ld c, SPI_DATA
ldl1            outi
                outi
                outi
                outi
                dec a
                jr nz,ldl1
                FT_OFF

                FT_ON
                ld a, (FT_REG_DLSWAP >> 16) | 0x80
                out (SPI_DATA), a
                ld a, (FT_REG_DLSWAP >> 8) & 0xFF
                out (SPI_DATA), a
                ld a, FT_REG_DLSWAP & 0xFF
                out (SPI_DATA), a
                ld a, FT_DLSWAP_FRAME
                out (SPI_DATA), a
                FT_OFF
                ret

wait_int        ft_rd8  FT_REG_INT_FLAGS
                and FT_INT_SWAP
                jr z, wait_int
                ret

;--------------- displaylist data

dl1_start
                FT_CLEAR_COLOR_RGB   0,  21,   0
                FT_CLEAR 1, 1, 1

                FT_COLOR_A 255

                FT_BEGIN FT_RECTS
                FT_LINE_WIDTH 15
                FT_COLOR_RGB   0, 85,   0
                FT_VERTEX2F   0 << 4,   0 << 4
                FT_VERTEX2F 799 << 4,   9 << 4
                FT_VERTEX2F   0 << 4, 590 << 4
                FT_VERTEX2F 799 << 4, 599 << 4
                FT_VERTEX2F 395 << 4,   0 << 4
                FT_VERTEX2F 404 << 4, 599 << 4
                FT_END

                FT_BEGIN FT_BITMAPS
dl_lscore       FT_VERTEX2II 300,  20, 31, '0'
                FT_VERTEX2II 325,  20, 31, '0'
                FT_VERTEX2II 450,  20, 31, '0'
                FT_VERTEX2II 475,  20, 31, '0'
                FT_END

                FT_BEGIN FT_RECTS
                FT_COLOR_RGB 255, 255, 255
dl_ball1        FT_VERTEX2F 400 << 4, 300 << 4
dl_ball2        FT_VERTEX2F 409 << 4, 309 << 4
                FT_COLOR_RGB   0, 255, 255
dl_lbat1        FT_VERTEX2F 40 << 4, 260 << 4
dl_lbat2        FT_VERTEX2F 49 << 4, 309 << 4
                FT_COLOR_RGB 255, 255,   0
dl_rbat1        FT_VERTEX2F 750 << 4, 300 << 4
dl_rbat2        FT_VERTEX2F 759 << 4, 349 << 4
                FT_END

                FT_DISPLAY

DL1_SIZE        equ     ($-dl1_start)>>2

;---------------

snd_fx          db      0

lscore          db      0
rscore          db      0

ball_x          dw      400
ball_y          dw      300
ball_dirx       dw      -10
ball_diry       dw      -2

lbat            dw      260
lbat_dir        dw      -1
rbat            dw      300
rbat_dir        dw      1
