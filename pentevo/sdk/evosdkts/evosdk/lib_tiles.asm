	export _draw_tile
	export _draw_image
	export _select_image
	export _draw_tile_key
	export _color_key


;копирование тайла из теневого экрана в буфер спрайтов
;d=x,e=y

updateOneTileToBuffer
	exa
	push de
	sla e
	sla e
	sla e ; y * 8
    
    ld hl,_screenActive
    ld a,e
    rlca
    rlca
    and 3
    or (hl)
	ld bc,#1CAF
    out (c),a
	ld bc,#1FAF
    and 3
    or #0c
    out (c),a

    ld a,e
    and #3f
    ld h,a
    ld a,d
    add a,a
    add a,a ; x * 4
    ld l,a

    ld bc,#1AAF
    out (c),l
    inc b
    out (c),h
    ld b,#1D
    out (c),l
    inc b
    out (c),h

    ld b,#26
    ld a,#1
    out (c),a
    ld b,#28
    ld a,7
    out (c),a
    ld b,#27
	ld a,%00110001
	out (c),a
	rst 8
	pop de
	exa
	ret



;копирование тайла из буфера спрайта на теневой экран
;d=x,e=y

updateOneTileFromBuffer
	exa
	push de
	sla e
	sla e
	sla e ; y * 8
    
    ld hl,_screenActive
    ld a,e
    rlca
    rlca
    and 3
    or (hl)
	ld bc,#1FAF
    out (c),a
	ld bc,#1CAF
    and 3
    or #0c
    out (c),a

    ld a,e
    and #3f
    ld h,a
    ld a,d
    add a,a
    add a,a ; x * 4
    ld l,a

    ld bc,#1AAF
    out (c),l
    inc b
    out (c),h
    ld b,#1D
    out (c),l
    inc b
    out (c),h

    ld b,#26
    ld a,#1
    out (c),a
    ld b,#28
    ld a,7
    out (c),a
    ld b,#27
	ld a,%00110001
	out (c),a
	rst 8
	pop de
	exa
	ret

updateTilesToBuffer
	ld a,(tileUpdate)
	or a
	ret z
	MROMEnable
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
	MROMDisable
	ret



updateTilesFromBuffer
	ld a,(tileUpdate)
	or a
	ret z
	xor a
	ld (tileUpdate),a
	MROMEnable

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
	MROMDisable
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
	ld hl,(tileOffset)
	add hl,de
	ld a,h
	srl a
	add a,GFX_PAGE
	push bc
	ld bc,#1CAF ; 
	out (c),a
	dup 5
	add hl,hl
	edup
	dec b
	out (c),h
	dec b
	out (c),l
	pop bc
	ld a,c
	add a,a
	add a,a
	ld e,a
	ld l,b
	ld h,high scrTable
	ld d,(hl)
	set 5,l
	ld a,(hl)
	ld hl,_screenActive
	or (hl)
	ld bc,#1FAF
	out (c),a
	dec b
	out (c),d
	dec b
	out (c),e
	ld b,#26
	ld a,1
	out (c),a
	inc b
	inc b
	ld a,7
	out (c),a
	MROMEnable
	ld bc,#27AF
	ld a,%00010001
	out (c),a
	rst 8
	MROMDisable
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
	ld hl,(tileOffset)
	add hl,de
	ld a,h
	srl a
	add a,GFX_PAGE
	push bc
	ld bc,MEM_SLOT0
	out (c),a
	pop bc
	dup 5
	add hl,hl
	edup
	res 7,h
	res 6,h
	ld a,c
	add a,a
	add a,a
	ld e,a
	ld a,b
	add a,a
	add a,a
	add a,a
	ld d,a
	res 7,d
	set 6,d
	rlca
	rlca
	and 3
	ld c,a
	ld a,(_screenActive)
	or c

	ld bc,MEM_SLOT1
	ld (_memSlot1),a
	out (c),a
	ex de,hl

	ld a,8
.loop
	exa
.column0
	ld a,(de)
.keyAB0=$+1
	cp 0
	jr z,.column0done
	and %11110000
.keyA0=$+1
	cp 0
	jr z,.skipA0
	ld c,a
	ld a,(hl)
	and %00001111
	or c
	ld (hl),a
.skipA0
	ld a,(de)
	and %00001111
.keyB0=$+1
	cp 0
	jr z,.column0done
	ld c,a
	ld a,(hl)
	and %11110000
	or c
	ld (hl),a
.column0done
	inc e
	inc l 

.column1
	ld a,(de)
.keyAB1=$+1
	cp 0
	jr z,.column1done
	and %11110000
.keyA1=$+1
	cp 0
	jr z,.skipA1
	ld c,a
	ld a,(hl)
	and %00001111
	or c
	ld (hl),a
.skipA1
	ld a,(de)
	and %00001111
.keyB1=$+1
	cp 0
	jr z,.column1done
	ld c,a
	ld a,(hl)
	and %11110000
	or c
	ld (hl),a
.column1done
	inc e
	inc l

.column2
	ld a,(de)
.keyAB2=$+1
	cp 0
	jr z,.column2done
	and %11110000
.keyA2=$+1
	cp 0
	jr z,.skipA2
	ld c,a
	ld a,(hl)
	and %00001111
	or c
	ld (hl),a
.skipA2
	ld a,(de)
	and %00001111
.keyB2=$+1
	cp 0
	jr z,.column2done
	ld c,a
	ld a,(hl)
	and %11110000
	or c
	ld (hl),a
.column2done
	inc e
	inc l

.column3
	ld a,(de)
.keyAB3=$+1
	cp 0
	jr z,.column3done
	and %11110000
.keyA3=$+1
	cp 0
	jr z,.skipA3
	ld c,a
	ld a,(hl)
	and %00001111
	or c
	ld (hl),a
.skipA3
	ld a,(de)
	and %00001111
.keyB3=$+1
	cp 0
	jr z,.column3done
	ld c,a
	ld a,(hl)
	and %11110000
	or c
	ld (hl),a
.column3done
	inc e
	ld bc,253
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

	push bc
	MROMEnable
	pop bc

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

	push bc	;размер
	push hl	;координаты


	ld a,l
	add a,a
	add a,a
	ld (.vaddr),a
	ld (.vaddrrest),a
	ld l,h
	ld h,high scrTable
	ld a,(hl)
	ld (.vaddr+1),a
	set 5,l
	ld a,(hl)
	ld hl,_screenActive
	or (hl)
	ld (.vpage),a
.loopv
	push bc
	push de
	ld a,c
	exa

	ld bc,#1CAF
	ld a,d
	srl a
	add a,GFX_PAGE
	out (c),a
	dec b
	ex de,hl
	dup 5
	add hl,hl
	edup
	out (c),h
	dec b
	out (c),l
	exa
.looph
	exa

	ld bc,#1DAF
	ld hl,0
.vaddr equ $-2
	out (c),l
	inc b
	out (c),h
	inc b
	ld a,0
.vpage equ $-1
	out (c),a
	ld b,#26
	ld a,1
	out (c),a
	ld b,#28
	ld a,7
	out (c),a
	dec b
	ld a,%00010001
	out (c),a
	rst 8
	ld a,(.vaddr)
	add a,4
	ld (.vaddr),a

	exa
	dec a
	jp nz,.looph

	pop de
.wdt=$+1
	ld hl,0
	add hl,de
	ex de,hl
	ld a,0
.vaddrrest equ $-1
	ld (.vaddr),a
	ld a,(.vaddr+1)
	add a,8
	bit 6,a
	jr z,.noVPageChange
	res 6,a
	push af
	ld a,(.vpage)
	inc a
	ld (.vpage),a
	pop af
.noVPageChange
	ld (.vaddr+1),a
	pop bc
	dec b
	jp nz,.loopv

	MROMDisable

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
