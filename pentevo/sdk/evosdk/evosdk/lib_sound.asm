	export _sfx_play
	export _sfx_stop
	export _music_play
	export _music_stop
	export _sample_play



;выключение звука на указанном чипе
;a=0 или 1

reset_ay
	push af
	or #fe
	di
	ld bc,#fffd
	out (c),a

	xor a
	ld l,a
.l0
	ld b,#ff
	out (c),a
	ld b,#bf
	out (c),l
	inc a
	cp 14
	jr nz,.l0
	ei
	pop af
	ret



;запуск звукового эффекта

_sfx_play
	push bc
	ld a,SND_PAGE
	call setSlot1
	pop bc
	ld a,b
	call AFX_PLAY
	ld a,CC_PAGE1
	jp setSlot1



;останов звуковых эффектов

_sfx_stop
	xor a
	jp reset_ay



;запуск музыки

_music_play
	push ix
	push iy
	push af
	ld a,SND_PAGE
	call setSlot1

	ld a,(MUS_COUNT)
	ld l,a
	pop af

	cp l
	jr nc,.skip

	ld h,high MUS_LIST
	ld l,a

	ld e,(hl)
	inc h
	ld d,(hl)
	inc h
	ld a,(hl)
	ex de,hl
	
	ifdef EVO
	cpl
	else
	xor 127
	endif
	
	call setSlot2
	di
	ld (musicPage),a
	ld bc,#fffd
	ld a,#fe
	out(c),a
	call PT3_INIT
	ei
	ld a,CC_PAGE2
	call setSlot2

.skip
	pop iy
	pop ix

	ld a,CC_PAGE1
	jp setSlot1



;выключение музыки

_music_stop
	xor a
	ld (musicPage),a
	jp reset_ay


;проигрывание сэмпла
;l=номер сэмпла

_sample_play
	ld bc,MEM_SLOT0
	ld a,SND_PAGE
	out (c),a
	ld a,(SMP_COUNT&#3fff)
	ld e,a
	ld a,l
	cp e
	jr nc,.skip

	ld h,high (SMP_LIST&#3f00)

	ld e,(hl)	;lsb
	inc h
	ld d,(hl)	;msb
	inc h
	ld a,(hl)	;page
	inc h
	ld h,(hl)	;delay
	ex de,hl

	ifdef ATM
	xor 128
	endif
	out (c),a
	ld e,a
	di
	ld a,%10100000 ;режим EGA без турбо, так как в 14 ћ√ц скорость нестабильна
	ld bc,#bd77
	out (c),a
	ld bc,MEM_SLOT0
.l0
	ld a,(hl)	;7
	out (#fb),a	;11
	or a		;4
	jr z,.done	;7/12
	inc hl		;6
	bit 6,h		;8
	jr nz,.page	;7/12
.delay
	ld a,d		;4
	dec a		;4
	jp nz,$-1	;10
	jp .l0		;10=78t при d=1, шаг задержки 14 тактов
.page
	ld h,0
	dec e
	out (c),e
	jp .delay
.done
	ld a,%10101000 ;режим EGA с турбо
	ld bc,#bd77
	out (c),a
	ei

.skip
	ld bc,MEM_SLOT0
	ld a,CC_PAGE0
	out (c),a
	ret