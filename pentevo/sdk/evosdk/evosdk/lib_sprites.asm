	export _sprites_start
	export _sprites_stop


;win0: background layer; screen procedure
;win1: screen layers 0,2
;win2: screen layers 1,3
;win3: code,stack,interrupt



;копирование видимого экрана в теневой

copy_visible_to_shadow
	ld a,(_screenActive)

	ld bc,MEM_SLOT2
	ld (_memSlot2),a
	out (c),a

	ld b,high MEM_SLOT1
	xor 2
	ld (_memSlot1),a
	out (c),a

	ld hl,16384
	ld de,32768
	ld b,h
	ld c,l
	call _fast_ldir

	ld a,(_screenActive)

	ld bc,MEM_SLOT2
	sub 4
	ld (_memSlot2),a
	out (c),a

	ld b,high MEM_SLOT1
	xor 2
	ld (_memSlot1),a
	out (c),a

	ld hl,16384
	ld de,32768
	ld b,h
	ld c,l
	jp _fast_ldir



;копирование экрана в буфер фона для спрайтов
;этот буфер используется для стирания спрайтов
;и имеет специальный формат типа H=x L=y
;буфер занимает четыре страницы для ускорения стирания

convert_screen
	push bc
	ld a,e
	call setSlot1
	ld a,d
	call setSlot2

	ld c,200
	ex de,hl
	ld hl,#8000
.l1
	push hl
	ld b,40
.l2
	ld a,(de)
	ld (hl),a
	inc de
	inc h
	djnz .l2
	pop hl
	inc l
	dec c
	jr nz,.l1

	pop bc
	ret



;запуск спрайтов
;очищает список спрайтов
;копирует видимый экран в теневой
;копирует теневой экран в буфер спрайтов
;разрешает вывод спрайтов

_sprites_start
	xor a
	ld (spritesActive),a

	ld hl,_sprqueue
	ld de,_sprqueue+1
	ld bc,511
	ld (hl),255
	ldir

	call copy_visible_to_shadow

	ld e,SCR_PAGE1
	ld d,SPBUF_PAGE0
	ld hl,#4000
	call convert_screen
	ld e,SCR_PAGE5
	ld d,SPBUF_PAGE1
	ld hl,#4000
	call convert_screen
	ld e,SCR_PAGE1
	ld d,SPBUF_PAGE2
	ld hl,#6000
	call convert_screen
	ld e,SCR_PAGE5
	ld d,SPBUF_PAGE3
	ld hl,#6000
	call convert_screen

	MRestoreMemMap12

	ld a,1
	ld (spritesActive),a

	ret



;остановка спрайтов

_sprites_stop
	xor a
	ld (spritesActive),a
	ret



	align 256
taby
        dup 200-16
        db high ((low $)*40)+#40
        edup
        dup 256-(low $)
        db high ((200-16)*40)+#40
        edup

        dup 200-16
        db low ((low $)*40)
        edup
        dup 256-(low $)
        db low ((200-16)*40)
        edup
;tabx
        dup 160-8
        db (low $)/4
        edup
        dup 256-(low $)
        db (160-8)/4
        edup
;tprsprx
        dup 64
        db low prsprx0
        db low prsprx1
        db low prsprx2
        db low prsprx3
        edup
tresprx
        dup 64
        db low resprx0
        db low resprx1
        db low resprx2
        db low resprx3
        edup

       ;ds .(-$)
toutd
        db SPBUF_PAGE1;2
        db SPBUF_PAGE2;1
        db SPBUF_PAGE3;0
        db SPBUF_PAGE0;3
        db SPBUF_PAGE1;2
        db SPBUF_PAGE2;1
        db SPBUF_PAGE3;0

resprx0
	;display /h,$
         ld h,a ;H=x
         ex af,af'
         ld l,a ;L=y
        ld a,b
        push bc
         push hl
       add a,64
       ld b,a
        push bc
         push hl
       sub 32
        ld b,a
        push bc
         push hl
       add a,64
       ld b,a
        push bc
         push hl
        ld bc,MEM_SLOT0 ;background window port
        ld hl,toutd+6 ;для outd ;можно dec l
        jp (ix)

resprx1
	;display /h,$
         ld h,a
         ex af,af'
         ld l,a ;L=y
       ld a,b
       add a,64
       ld b,a
        push bc
         push hl
       sub 32
       ld b,a
        push bc
         push hl
       add a,64
       ld b,a
        push bc
         push hl
       sub 96
        ld b,a
        inc bc
        push bc
         inc h
         push hl
        ld bc,MEM_SLOT0 ;background window port
        ld hl,toutd+3 ;для outd ;можно b
        jp (ix)

resprx2
	;display /h,$
         ld h,a
         ex af,af'
         ld l,a ;L=y
       ld a,b
       add a,32
       ld b,a
        push bc
         push hl
       add a,64
       ld b,a
        push bc
         push hl
       sub 96
        ld b,a
        inc bc
        push bc
         inc h
         push hl
       ld a,b
       add a,64
       ld b,a
        push bc
         push hl
        ld bc,MEM_SLOT0 ;background window port
        ld hl,toutd+4 ;для outd ;можно c
        jp (ix)

resprx3
	;display /h,$
         ld h,a
        ld l,b
        ld a,b
        add a,96
        ld b,a
        push bc
        ld b,l
         ex af,af'
         ld l,a ;L=y
         push hl
        inc bc
        push bc
         inc h
         push hl
        ld a,b
       add a,64
       ld b,a
        push bc
         push hl
        sub 32
        ld b,a
        push bc
         push hl
        ld bc,MEM_SLOT0 ;#37f7 ;background window port
        ld hl,toutd+5 ;для outd ;можно h
        jp (ix)

prsprx0
	;display /h,$
        ld b,a
        push bc
       add a,64
       ld b,a
        push bc
       sub 32
        ld b,a
        push bc
       add a,64
       ld b,a
        push bc
        exx
        jp (ix)

prsprx1
	;display /h,$
        ld h,a
        add a,64
        ld b,a
        push bc
        sub 32
        ld b,a
        push bc
       add a,64
       ld b,a
        push bc
        ld b,h
        inc bc
        push bc
        exx
        jp (ix)

prsprx2
	;display /h,$
        ld h,a
        add a,32
        ld b,a
        push bc
       add a,64
       ld b,a
        push bc
        ld b,h
        inc bc
        push bc
       ld a,b
       add a,64
       ld b,a
        push bc
        exx
        jp (ix)

prsprx3
	;display /h,$
        ld h,a
        add a,96
        ld b,a
        push bc
        ld b,h
        inc bc
        push bc
       ld a,b
       add a,64
       ld b,a
        push bc
        sub 32
        ld b,a
        push bc
        exx
        jp (ix)

respr
        ld hl,_sprqueue
		ld a,(_screenActive)
		and 2
		jr nz,$+3
		inc h
        exx
        ld ix,squareremover
        ld iy,respr0
        ld bc,40
        exx
  jr respr0go
respr0
  inc e
  ret z ;end of queue
        exd
respr0go
  ld d,(hl) ;high id
  inc d
  ret z ;end of queue
        inc l
	inc l
        ld d,high taby
        ld e,(hl) ;y
        inc l
         ld a,e
         ex af,af'
        ld a,(de) ;addrh(y)
        ld b,a
        inc d
        ld a,(de) ;addrl(y)
        inc d ;'tabx
        ld e,(hl) ;x
        exd
        add a,(hl) ;x/4
        ld c,a
        adc a,b
        sub c
        ld b,a
         ld a,(hl) ;x/4
        ld h,high tresprx ;(4 ветки)
        ld l,(hl)
        ld h,high resprx0 ;todo inc h
        jp (hl)



prspr
        ld iy,prspr0
        ld bc,40
        exx
        ld de,_sprqueue
		ld a,(_screenActive)
		and 2
		jr nz,$+3
		inc d
  jr prspr0go
prspr0
        exx
  inc e
  ret z ;end of queue
prspr0go
        exd
        ld d,(hl) ;high id
  inc d
  ret z ;end of queue
         inc l
         ld e,(hl)
         inc l
      ld a,SPTBL_PAGE
      ld bc,MEM_SLOT0 ;#37f7 ;sprites window port
      out (c),a
        ld a,(de) ;addrl(id)
        ld lx,a
        inc d
        ld a,(de) ;addrh(id)
        ld hx,a ;ix=spraddr
        inc d
      ld a,(de) ;pg(id)
	ifdef ATM
	xor 128
	endif
      out (c),a
        ld d,high taby
        ld e,(hl) ;y
        inc hl
        ld a,(de) ;addrh(y)
        ld b,a
        inc d
        ld a,(de) ;addrl(y)
        inc d ;'tx
        ld e,(hl) ;x
        exd
        add a,(hl) ;x/4
        ld c,a
        adc a,b
        sub c
        inc h ;'tprsprx (4 ветки)
        ld l,(hl)
        ld h,high prsprx0
        jp (hl)



squareremover
       dup 4
        ;outd ;background page
		ld a,(hl)
		out (c),a
		dec l
        exx
        pop de ;buf
        pop hl ;screen

       dup 7

        ld a,(de)
        ld (hl),a
        inc hl
        inc d
        ld a,(de)
        ld (hl),a
        inc e
        add hl,bc
        ld a,(de)
        ld (hl),a
        dec hl
        dec d
        ld a,(de)
        ld (hl),a
        inc e
        add hl,bc

       edup ;7*2 lines
        ld a,(de)
        ld (hl),a
        inc hl
        inc d
        ld a,(de)
        ld (hl),a
        inc e
        add hl,bc
        ld a,(de)
        ld (hl),a
        dec hl
        dec d
        ld a,(de)
        ld (hl),a

        exx
       edup ;4 layers
        jp (iy) ;respr0