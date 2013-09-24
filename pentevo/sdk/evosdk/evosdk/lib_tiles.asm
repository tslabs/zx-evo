	export _draw_tile
	export _draw_image
	export _select_image
	export _draw_tile_key
	export _color_key



	macro MDrawTile

	ld bc,-16384+40
	dup 8
	ld a,(de)	;#4xxx
	ld (hl),a
	inc e
	set 5,h
	ld a,(de)	;#6xxx
	ld (hl),a
	inc e
	set 7,h
	res 6,h
	ld a,(de)	;#axxx
	ld (hl),a
	inc e
	res 5,h
	ld a,(de)	;#8xxx
	ld (hl),a
	inc e
	add hl,bc
	edup
	org $-2

	endm



	macro MDrawTileGetAddrs

	ld hl,(tileOffset)
	add hl,de
	ex de,hl
	
	ld h,high scrTable
	ld l,b
	ld a,c
	add a,(hl)
	set 5,l
	ld h,(hl)
	ld l,a

	ifdef EVO
	
	ld a,d
	srl a
	add a,low ~GFX_PAGE
	ld bc,MEM_SLOT0
	cpl
	out (c),a
	
	else
	
	ld a,d
	srl a
	add a,low (GFX_PAGE^127)
	ld bc,MEM_SLOT0
	xor 127
	out (c),a
	
	endif

	ld a,e
	rrca
	rrca
	rrca
	ld e,a
	and 31
	bit 0,d
	jr z,$+4
	or #20
	ld d,a
	ld a,e
	and #e0
	ld e,a

	endm



	macro MCopyTileColumnFromBuf
	ld bc,40
	dup 8
	ld a,(de)
	ld (hl),a
	inc e
	add hl,bc
	edup
	endm



	macro MCopyTileColumnToBuf
	ld bc,40
	dup 8
	ld a,(hl)
	ld (de),a
	inc e
	add hl,bc
	edup
	endm



;копирование тайла из теневого экрана в буфер спрайтов
;d=x,e=y

updateOneTileToBuffer
	exa
	push de

	ld a,d				;получить из de адрес в экране
	ld h,high scrTable
	ld l,e
	add a,(hl)
	set 5,l
	ld h,(hl)
	ld l,a

	sla e
	sla e
	sla e

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE0
	out (c),a

	MCopyTileColumnToBuf	;столбец 0
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-7*40+16384
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE1
	out (c),a

	MCopyTileColumnToBuf	;столбец 1
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-(7*40+8192)
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE2
	out (c),a

	MCopyTileColumnToBuf	;столбец 2
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-7*40+16384
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE3
	out (c),a

	MCopyTileColumnToBuf	;столбец 3
	org $-2

	pop de
	exa
	ret



;копирование тайла из буфера спрайта на теневой экран
;d=x,e=y

updateOneTileFromBuffer
	exa
	push de

	ld a,d				;получить из de адрес в экране
	ld h,high scrTable
	ld l,e
	add a,(hl)
	set 5,l
	ld h,(hl)
	ld l,a

	sla e
	sla e
	sla e

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE0
	out (c),a

	MCopyTileColumnFromBuf	;столбец 0
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-7*40+16384
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE1
	out (c),a

	MCopyTileColumnFromBuf	;столбец 1
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-(7*40+8192)
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE2
	out (c),a

	MCopyTileColumnFromBuf	;столбец 2
	org $-2
	ld a,e
	sub 7
	ld e,a
	ld bc,-7*40+16384
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,SPBUF_PAGE3
	out (c),a

	MCopyTileColumnFromBuf	;столбец 3
	org $-2

	pop de
	exa
	ret



updateTilesToBuffer
	ld a,(tileUpdate)
	or a
	ret z

	ld hl,tileUpdateMap
	ld e,0	;y
.clearUpdMap0
	ld d,0	;x
.clearUpdMap1
	ld a,(hl)
	or a
	jp nz,.rowChange
	ld a,d
	add a,8
	ld d,a
	jp .noRowChange
.rowChange
	push hl
	dup 8
	rra
	call c,updateOneTileToBuffer
	inc d
	edup
	pop hl
	ld a,d
.noRowChange
	inc l
	cp 40
	jp nz,.clearUpdMap1
	inc l
	inc l
	inc l
	inc e
	ld a,e
	cp 25
	jp nz,.clearUpdMap0

	ret



updateTilesFromBuffer
	ld a,(tileUpdate)
	or a
	ret z
	xor a
	ld (tileUpdate),a

	ld hl,tileUpdateMap
	ld e,0	;y
.clearUpdMap0
	ld d,0	;x
.clearUpdMap1
	ld a,(hl)
	or a
	jp nz,.rowChange
	ld a,d
	add a,8
	ld d,a
	jp .noRowChange
.rowChange
	push hl
	dup 8
	rra
	call c,updateOneTileFromBuffer
	inc d
	edup
	pop hl
	ld (hl),0
	ld a,d
.noRowChange
	inc l
	cp 40
	jp nz,.clearUpdMap1
	inc l
	inc l
	inc l
	inc e
	ld a,e
	cp 25
	jp nz,.clearUpdMap0

	ret



;выбор изображения для отрисовки тайлов

_select_image
	ld h,0
	add hl,hl
	add hl,hl
	ld bc,IMG_LIST
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,PAL_PAGE
	out (c),a

	ld e,(hl)	;tile
	inc l
	ld d,(hl)
	ld (tileOffset),de

	ld a,CC_PAGE0
	out (c),a

	ret



;установка флага обновления тайла
;c=X, b=y, не меняет bc и de

setTileUpdateMapF
	ld (tileUpdate),a	;A всегда не 0 на входе
setTileUpdateMap
	ld h,high tileUpdateXTable
	ld l,c
	ld a,(hl)
	set 6,l
	exa
	ld a,b
	add a,a
	add a,a
	add a,a
	add a,(hl)
	ld l,a
	ld h,high tileUpdateMap
	exa
	or (hl)
	ld (hl),a
	ret



;c=X, b=Y, de=tile
;координаты в тайлах

_draw_tile
	ld a,(spritesActive)
	or a
	call nz,setTileUpdateMapF
	MDrawTileGetAddrs
	MSetShadowScreen
	MDrawTile
	MRestoreMemMap012
	ret



;установка прозрачного цвета для draw_tile_key

_color_key
	ld b,high colorMaskTable
	ld a,(bc)
	ld (_draw_tile_key.keyAB0),a
	ld (_draw_tile_key.keyAB1),a
	ld (_draw_tile_key.keyAB2),a
	ld (_draw_tile_key.keyAB3),a
	set 4,c
	ld a,(bc)
	ld (_draw_tile_key.keyA0),a
	ld (_draw_tile_key.keyA1),a
	ld (_draw_tile_key.keyA2),a
	ld (_draw_tile_key.keyA3),a
	set 5,c
	ld a,(bc)
	ld (_draw_tile_key.keyB0),a
	ld (_draw_tile_key.keyB1),a
	ld (_draw_tile_key.keyB2),a
	ld (_draw_tile_key.keyB3),a
	ret



;отрисовка тайла с прозрачными пикселями
;c=X, b=Y, de=tile
;координаты в тайлах

_draw_tile_key
	ld a,(spritesActive)
	or a
	call nz,setTileUpdateMapF
	MDrawTileGetAddrs
	MSetShadowScreen

	ld a,8
.loop
	exa
.column0
	ld a,(de)
.keyAB0=$+1
	cp 0
	jr z,.column0done
	and %01000111
.keyA0=$+1
	cp 0
	jr z,.skipA0
	ld c,a
	ld a,(hl)
	and %10111000
	or c
	ld (hl),a
.skipA0
	ld a,(de)
	and %10111000
.keyB0=$+1
	cp 0
	jr z,.column0done
	ld c,a
	ld a,(hl)
	and %01000111
	or c
	ld (hl),a
.column0done
	inc e
	set 5,h

.column1
	ld a,(de)
.keyAB1=$+1
	cp 0
	jr z,.column1done
	and %01000111
.keyA1=$+1
	cp 0
	jr z,.skipA1
	ld c,a
	ld a,(hl)
	and %10111000
	or c
	ld (hl),a
.skipA1
	ld a,(de)
	and %10111000
.keyB1=$+1
	cp 0
	jr z,.column1done
	ld c,a
	ld a,(hl)
	and %01000111
	or c
	ld (hl),a
.column1done
	inc e
	res 6,h
	set 7,h

.column2
	ld a,(de)
.keyAB2=$+1
	cp 0
	jr z,.column2done
	and %01000111
.keyA2=$+1
	cp 0
	jr z,.skipA2
	ld c,a
	ld a,(hl)
	and %10111000
	or c
	ld (hl),a
.skipA2
	ld a,(de)
	and %10111000
.keyB2=$+1
	cp 0
	jr z,.column2done
	ld c,a
	ld a,(hl)
	and %01000111
	or c
	ld (hl),a
.column2done
	inc e
	res 5,h

.column3
	ld a,(de)
.keyAB3=$+1
	cp 0
	jr z,.column3done
	and %01000111
.keyA3=$+1
	cp 0
	jr z,.skipA3
	ld c,a
	ld a,(hl)
	and %10111000
	or c
	ld (hl),a
.skipA3
	ld a,(de)
	and %10111000
.keyB3=$+1
	cp 0
	jr z,.column3done
	ld c,a
	ld a,(hl)
	and %01000111
	or c
	ld (hl),a
.column3done
	inc e
	ld bc,-16384+40
	add hl,bc
	exa
	dec a
	jp nz,.loop

	MRestoreMemMap012
	ret



;отрисовка изображения целиком
;эта процедура быстрее чем вывод отдельных тайлов
;a=id, c=X, b=Y

_draw_image
	push bc

	ld h,0
	ld l,a
	add hl,hl
	add hl,hl
	ld bc,IMG_LIST
	add hl,bc

	ld bc,MEM_SLOT0
	ld a,PAL_PAGE
	out (c),a

	ld e,(hl)	;tile
	inc l
	ld d,(hl)
	inc l
	ld c,(hl)	;width
	inc l
	ld b,(hl)	;height

	ld h,0
	ld l,c
	ld (.wdt),hl

	pop hl

	ld a,l
	cp 40
	jp nc,.done
	ld a,h
	cp 25
	jp nc,.done

	ld a,c
	add a,l
	cp 40
	jr c,.noHClip
	ld a,40
	sub l
	ld c,a
.noHClip
	ld a,c
	or a
	jp z,.done

	ld a,b
	add a,h
	cp 25
	jr c,.noVClip
	ld a,25
	sub h
	ld b,a
.noVClip
	ld a,b
	or a
	jp z,.done

	push bc

	call setShadowScreen

	pop bc
	push bc	;размер
	push hl	;координаты

	ld a,l
	ld l,h
	ld h,high scrTable
	add a,(hl)
	set 5,l
	ld h,(hl)
	ld l,a

.loopv
	push bc
	push hl
	push de
	ld a,c
	exa

	ifdef EVO
	
	ld a,d
	srl a
	add a,low ~GFX_PAGE
	cpl
	ld (.page),a
	ld bc,MEM_SLOT0
	out (c),a
	else
	
	ld a,d
	srl a
	add a,low (GFX_PAGE^127)
	xor 127
	ld (.page),a
	ld bc,MEM_SLOT0
	out (c),a
	
	endif

	ld a,e
	rrca
	rrca
	rrca
	ld e,a
	and 31
	bit 0,d
	jr z,$+4
	or #20
	ld d,a
	ld a,e
	and #e0
	ld e,a

	exa
.looph
	exa
	MDrawTile

	inc e
	jp nz,.noPageChange
	inc d
	bit 6,d
	jp z,.noPageChange
	res 6,d
.page=$+1
	ld a,0
	dec a
	ld bc,MEM_SLOT0
	out (c),a
	ld (.page),a

.noPageChange
	ld bc,-(16384+7*40-1)
	add hl,bc
	exa
	dec a
	jp nz,.looph

	pop de
.wdt=$+1
	ld hl,0
	add hl,de
	ex de,hl
	pop hl
	ld bc,8*40
	add hl,bc
	pop bc
	dec b
	jp nz,.loopv

	ld bc,MEM_SLOT1
	ld a,CC_PAGE1
	ld (_memSlot1),a
	out (c),a

	ld b,high MEM_SLOT2
	ld a,CC_PAGE2
	ld (_memSlot2),a
	out (c),a

	pop bc	;координаты начала изображения B=y C=x
	pop hl	;размеры выводимой части

	;если спрайты разрешены, помечаем выведенные тайлы в карте изменившихся тайлов

	ld a,(spritesActive)
	or a
	jr z,.done
	ld (tileUpdate),a

.setUpd1
	push bc
	push hl
.setUpd2
	push hl
	call setTileUpdateMap
	pop hl
	inc c
	dec l
	jp nz,.setUpd2
	pop hl
	pop bc
	inc b
	dec h
	jp nz,.setUpd1

.done
	ld bc,MEM_SLOT0
	ld a,CC_PAGE0
	out (c),a
	ret
