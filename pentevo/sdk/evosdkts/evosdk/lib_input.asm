	export _joystick
	export _keyboard
	export _mouse_apply_clip


	
;получение XY дельт движения стрелки мыши

poll_mouse_delta
	ld a,(_mouse_prev_dx)
	ld e,a
	ld bc,#fbdf	;дельта x
	in a,(c)
	ld d,a
	sub e
	ld (_mouse_dx),a
	ld a,d
	ld (_mouse_prev_dx),a

	ld a,(_mouse_prev_dy)
	ld b,#ff	;дельта y
	in e,(c)
	sub e
	ld (_mouse_dy),a
	ld a,e
	ld (_mouse_prev_dy),a

	ret



;применение зоны клиппинга мыши, вызывается при изменении координат или зоны

_mouse_apply_clip
	ld a,(_mouse_cx1)
	ld l,a
	ld a,(_mouse_cx2)
	ld h,a
	ld a,(_mouse_x)
	cp l
	jr nc,$+3
	ld a,l
	cp h
	jr c,$+3
	ld a,h
	ld (_mouse_x),a

	ld a,(_mouse_cy1)
	ld l,a
	ld a,(_mouse_cy2)
	ld h,a
	ld a,(_mouse_y)
	cp l
	jr nc,$+3
	ld a,l
	cp h
	jr c,$+3
	ld a,h
	ld (_mouse_y),a

	ret



;код опроса мыши, используется однократно в обработчике прерывания

	macro poll_mouse

	call poll_mouse_delta

	ld b,#fa	;кнопки мыши
	in a,(c)
	cpl
	and #7
	ld (_mouse_btn),a

	ld a,(_mouse_cx1)
	ld e,a
	ld a,(_mouse_cx2)
	ld d,a

	ld a,(_mouse_dx)
	ld c,a
	srl a
	bit 7,c
	jr z,$+4
	or #80
	ld c,a
	ld a,(_mouse_x)
	add a,c
	rl c
	jp nc,.clipRight
.clipLeft
	cp d
	jr c,$+3
	ld a,e
	cp e
	jr nc,.clipHDone
	ld a,e
	jp .clipHDone
.clipRight
	cp d
	jr c,.clipHDone
	ld a,d
.clipHDone
	ld (_mouse_x),a

	ld a,(_mouse_cy1)
	ld e,a
	ld a,(_mouse_cy2)
	ld d,a

	ld a,(_mouse_dy)
	ld c,a
	ld a,(_mouse_y)
	add a,c
	rl c
	jp nc,.clipDown
.clipUp
	cp d
	jr c,$+3
	ld a,e
	cp e
	jr nc,.clipVDone
	ld a,e
	jp .clipVDone
.clipDown
	cp d
	jr c,.clipVDone
	ld a,d
.clipVDone
	ld (_mouse_y),a

	endm



;функция опроса джойстиков и клавиатуры
;опрашивает Kempston и Cursor+Space

_joystick
	ld l,0

	ld bc,#fefe		;ряд cZXCV
	in a,(c)
	rra
	jr c,.noCaps	;caps не нажат

;cursor джойстик

	ld b,#f7		;ряд 12345
	in a,(c)
	and #10
	jr nz,$+4
	set 1,l
	ld b,#ef		;09876
	in a,(c)
	rra
	jr c,$+4
	set 4,l
	rra
	rra
	jr c,$+4
	set 0,l
	rra
	jr c,$+4
	set 3,l
	rra
	jr c,$+4
	set 2,l
.noCaps
	ld b,#7f		;ряд SpSymBNM
	in a,(c)
	rra
	jr c,$+4
	set 4,l
	
	ld a,l
	or a
	ret nz

;kempston джойстик

	in a,(31)
	and #1f
	ld l,a

	ret
	
	
	
;опрос клавиатуры, заполняет 40-байтный массив флагами состояния клавиш

keyboard_row
	in a,(c)
	cpl
	ld e,a
	ld b,5
.l0
	rr e
	sbc a,a
	ld c,a
	xor (ix)
	and c
	and 2
	ld (ix),c
	rr c
	jr nc,$+4
	or 1
	ld (hl),a
	inc ix
	inc hl
	djnz .l0
	ret
	
	
	
_keyboard
	push ix
	ex de,hl
	ld ix,keysPrevState
	ld bc,#fefe		;cZXCV
	call keyboard_row
	ld bc,#fdfe		;ASDFG
	call keyboard_row
	ld bc,#fbfe		;QWERT
	call keyboard_row
	ld bc,#f7fe		;12345
	call keyboard_row
	ld bc,#effe		;09876
	call keyboard_row
	ld bc,#dffe		;POIUY
	call keyboard_row
	ld bc,#bffe		;eLKJH
	call keyboard_row
	ld bc,#7ffe		;_sMNB
	call keyboard_row
	pop ix
	ret