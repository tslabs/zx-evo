	export _sprites_start
	export _sprites_stop


;win0: background layer; screen procedure
;win1: screen layers 0,2
;win2: screen layers 1,3
;win3: code,stack,interrupt



;копирование видимого экрана в теневой

copy_visible_to_shadow
	ld a,(_screenActive)
    ld bc,#1FAF
    out (c),a
    ld b,#1C
    xor #18
    out (c),a
    ld h,0
    ld b,#1A
    out (c),h
    inc b
    out (c),h
    inc b
    inc b
    out (c),h
    inc b
    out (c),h
    ld b,#26
    ld a,79
    out (c),a
    ld b,#28
    ld a,199
    out (c),a
    ld a,%00110001
    ld bc,#27AF
    out (c),a
    rst 8 ; DMA
    ret    

;копирование экрана в буфер фона для спрайтов
;этот буфер используется для стирания спрайтов
;и имеет специальный формат типа H=x L=y
;буфер занимает четыре страницы для ускорения стирания

;convert_screen
;	ld a,e
;	call setSlot1
;	ld a,d
;	call setSlot2
;   ld hl,#4000
;   ld de,#8000
;   ld b,0
;.l1	ld c,160
;    call _fast_ldir
;    inc d
;    inc h
;    ld l,c
;    ld e,c
;    dec hx
;    jp nz,.l1
;	ret



;запуск спрайтов
;очищает список спрайтов
;копирует видимый экран в теневой
;копирует теневой экран в буфер спрайтов
;разрешает вывод спрайтов

_sprites_start
    push ix
	xor a
	ld (spritesActive),a

	ld hl,_sprqueue
	ld de,_sprqueue+1
	ld bc,511
	ld (hl),255
	ldir
    MROMEnable
	call copy_visible_to_shadow
    ld de,#0C08
    ld h,0
    ld bc,#1AAF
    out (c),h
    inc b
    out (c),h
    inc b
    out (c),e
    inc b
    out (c),h
    inc b
    out (c),h
    inc b
    out (c),d
    ld b,#26
    ld a,79
    out (c),a
    ld b,#28
    ld a,199
    out (c),a
    ld a,%00110001
    ld bc,#27AF
    out (c),a
    rst 8 ; DMA
    MROMDisable
	ld a,1
	ld (spritesActive),a
    pop ix
	ret


;остановка спрайтов

_sprites_stop
	xor a
	ld (spritesActive),a
	ret



	   align 256
taby    dup 200-16
        db low (((low $)) & #3f)
        edup
        dup 256-(low $)
        db 184 & #3f
        edup

        dup 200-16
        db  (((low $)) >> 6)
        edup
        dup 256-(low $)
        db (184 >> 6)
        edup

toutd
        db SPBUF_PAGE1;2
        db SPBUF_PAGE2;1
        db SPBUF_PAGE3;0
        db SPBUF_PAGE0;3
        db SPBUF_PAGE1;2
        db SPBUF_PAGE2;1
        db SPBUF_PAGE3;0


respr    
        MROMEnable
        call respr_
        MROMDisable
        ret

respr_  ld hl,_sprqueue
		ld a,(_screenActive)
		and 8
		jr nz,$+3
		inc h
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
        ld a,(de) ;addrh(y)
        ld b,a
        inc d
        ld a,(de) ;addrl(y)
        inc d ;'tabx
        ld e,(hl) ;x
        exd 
        push bc
        push hl
        exx
        ld e,a
        ld a,(_screenActive)
        or e
        ld bc,#1FAF
        out (c),a
        ld a,#0c
        or e
        ld b,#1C
        out (c),a
        pop hl ; l - x
        pop de ; d - y
        ld b,#1A
        out (c),l
        inc b
        out (c),d
        ld b,#1D
        out (c),l
        inc b
        out (c),d
        ld b,#26
        ld a,4
        out (c),a
        ld b,#28
        ld a,15
        out (c),a
        ld b,#27
        ld a,%00110001
        out (c),a
        rst 8
        exx
        jp respr0

prspr
        ld iy,prspr0
        ld de,_sprqueue
        ld a,(_screenActive)
        and 8
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
        ld bc,MEM_SLOT0 
        out (c),a
        ld a,(de) ;addrl(id)
        ld lx,a
        inc d
        ld a,(de) ;addrh(id)
        ld hx,a ;ix=spraddr
        inc d
        ld a,(de) ;pg(id)
        out (c),a

        ld a,(_screenActive)
        ld b,a
        ld a,(hl)
        inc l
        cp 224
        jr c,$+4
        ld a,224
        ld d,a
        rlca
        rlca
        and 3
        or b
        ld bc,MEM_SLOT1
        ld (_memSlot1),a        
        out (c),a
        ld bc,MEM_SLOT2        
        inc a
        ld (_memSlot2),a        
        out (c),a
        res 7,d
        set 6,d
        ld e,(hl)
        push de
        exd
        exx
        ld bc,0
        jp (ix)

		align 256
