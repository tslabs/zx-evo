#lib	<z80.lib>
;Screen management program
;some special things
;***************************************
          org     32000
;Minden pont melle jobbra is tesz egyet
duplicate_screen
          ld      c,192
		  ld	  hl,(buff1)
          ld      hl,4000H
out_loop
          or      a
          ld      b,32
inn_loop
          ld      a,(hl)
          ld      d,a
          rra
          or      (hl)
          ld      (hl),a
          inc     hl
          ld      a,d
          rra
          djnz    inn_loop
          dec     c
          jr      nz,out_loop
;****************************
          ld      b,192
          ld      c,0
vertical_loop
          push    bc
          ld      a,c
          call    adress_in_screen
          push    hl
          ld      de,buff1
          ld      bc,32
          ldir
          pop     hl
          ld      de,buff2
          ld      b,32
vertic_inn_lop
          ld      a,(de)
          or      (hl)
          ld      (hl),a
          inc     de
          inc     hl
          djnz    vertic_inn_lop
          ld      bc,32
          ld      hl,buff1
          ld      de,buff2
          ldir
          pop     bc
          inc     c
          djnz    vertical_loop
          ret
adress_in_screen
          ld      l,a
          ld      h,0
          add     hl,hl
          ld      de,table
          add     hl,de
          ld      a,(hl)
          inc     hl
          ld      h,(hl)
          ld      l,a
          ret

buff1     db     32
buff2     db     32
table
         dw 16384
         dw 16640
         dw 16896
         dw 17152
         dw 22496
		 dw	$
