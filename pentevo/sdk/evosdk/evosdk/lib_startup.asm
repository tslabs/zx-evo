	device pentagon1024

	include "target.asm"

	ifdef EVO

	display "EVO version"

MEM_SLOT0=#37f7
MEM_SLOT1=#77f7
MEM_SLOT2=#b7f7
MEM_SLOT3=#f7f7

INVMASK=#ff

	else

	display "ATM version"

MEM_SLOT0=#3ff7
MEM_SLOT1=#7ff7
MEM_SLOT2=#bff7
MEM_SLOT3=#fff7

INVMASK=#7f

	endif


SCR_PAGE1=(1^INVMASK)
SCR_PAGE3=(3^INVMASK)
SCR_PAGE5=(5^INVMASK)
SCR_PAGE7=(7^INVMASK)

SPBUF_PAGE0=(8^INVMASK)
SPBUF_PAGE1=(9^INVMASK)
SPBUF_PAGE2=(10^INVMASK)
SPBUF_PAGE3=(11^INVMASK)

SPTBL_PAGE=(6^INVMASK)

CC_PAGE0=(12^INVMASK)
CC_PAGE1=(13^INVMASK)
CC_PAGE2=(14^INVMASK)

SND_PAGE=(0^INVMASK)
PAL_PAGE=(4^INVMASK)
GFX_PAGE=(16^INVMASK)


IMG_LIST =#1000

;смещени€ в SND_PAGE

AFX_INIT =#4000
AFX_PLAY =#4003
AFX_FRAME=#4006
PT3_INIT =#4009
PT3_FRAME=#400c

MUS_COUNT=#49fe
SMP_COUNT=#49ff
SFX_COUNT=#5000

MUS_LIST =#4a00
SMP_LIST =#4d00
SFX_DATA =#5100



	macro MDebug color

	push af
	ld a,color
	out (#fe),a
	pop af

	endm

	macro MSetShadowScreen

	ld a,(_screenActive)

	ld bc,MEM_SLOT1
	ld (_memSlot1),a
	out (c),a

	ld b,high MEM_SLOT2
	sub 4
	ld (_memSlot2),a
	out (c),a

	endm

	macro MRestoreMemMap012

	ld bc,MEM_SLOT0
	ld a,CC_PAGE0
	out (c),a

	ld b,high MEM_SLOT1
	ld a,CC_PAGE1
	ld (_memSlot1),a
	out (c),a

	ld b,high MEM_SLOT2
	ld a,CC_PAGE2
	ld (_memSlot2),a
	out (c),a

	endm

	macro MRestoreMemMap12

	ld bc,MEM_SLOT1
	ld a,CC_PAGE1
	ld (_memSlot1),a
	out (c),a

	ld b,high MEM_SLOT2
	ld a,CC_PAGE2
	ld (_memSlot2),a
	out (c),a

	endm



	org #e000

begin
	di
	ld sp,begin-1

	;определение TS

	ld bc,#fffd	;чип 0
	out (c),b
	xor a		;регистр 0
	out (c),a
	ld b,#bf	;значение #bf
	out (c),b
	ld b,#ff	;чип 1
	ld a,#fe
	out (c),a
	xor a		;регистр 0
	out (c),a
	ld b,#bf	;значение 0
	out (c),a
	ld b,#ff	;чип 0
	out (c),b
	xor a		;регистр 0
	out (c),a
	in a,(c)
	ld (turboSound),a

	;инициализаци€ звуковых эффектов, если они есть

	ld a,SND_PAGE
	call setSlot1
	ld a,(SFX_COUNT)
	or a
	jr z,.noSfx
	ld hl,SFX_DATA
	call AFX_INIT
.noSfx
	xor a
	call reset_ay
	inc a
	call reset_ay

	;установка переменных

	ld a,SCR_PAGE1
	ld (_screenActive),a

	call poll_mouse_delta

	;установка обработчика прерываний

	ld a,im2vector/256
	ld i,a
	im 2
	ei

	;гашение экрана

	halt
	LD A,%10101011	;разрешение палитры
	ld bc,#bd77
	out (c),a

	ld a,2
.fade0
	halt
	halt
	push af
	call _pal_bright
	pop af
	dec a
	cp 255
	jr nz,.fade0

	;очистка экрана

	ld a,SCR_PAGE1
	call clearPage
	ld a,SCR_PAGE3
	call clearPage
	ld a,SCR_PAGE5
	call clearPage
	ld a,SCR_PAGE7
	call clearPage

	;режим EGA с палитрой

	halt
	LD A,%10101000
	ld bc,#bd77
	out (c),a

	call _swap_screen

	;установка нормальной €ркости

	ld hl,3<<6
	ld (_palBright),hl
	ld a,1
	ld (_palChange),a
	halt

	;установка карты пам€ти дл€ выполнени€ кода
	;страница в верхнем окне не мен€етс€, всегда 11

	ld a,CC_PAGE0
	call setSlot0
	ld a,CC_PAGE1
	call setSlot1
	ld a,CC_PAGE2
	call setSlot2

	jp 0



setActiveScreen
	ld a,(_screenActive)
	xor 2

	ld bc,MEM_SLOT1
	ld (_memSlot1),a
	out (c),a

	ld b,high MEM_SLOT2
	sub 4
	ld (_memSlot2),a
	out (c),a

	ret



setShadowScreen
	MSetShadowScreen
	ret



;более быстра€ верси€ ldir, эффективна при bc>12
;из статьи на MSX Assembly Page
;в отличие от нормального ldir портит A и флаги

_fast_ldir
	xor a
	sub c
	and 63
	add a,a
	ld (.jump),a
.jump=$+1
	jr nz,.loop
.loop
	dup 64
	ldi
	edup
	jp pe,.loop
	ret



_clear_screen
	and 15
	ld l,a
	ld h,high colorMaskTable
	ld e,(hl)
	call setShadowScreen
	ld hl,#4000
	ld (hl),e
	ld de,#4001
	ld bc,#7fff
	call _fast_ldir
	MRestoreMemMap12
	ret



_swap_screen
	push ix
	push iy

	ld a,(spritesActive)
	or a
	push af
	jr z,.noSpr0
	call setShadowScreen
	call updateTilesToBuffer
	call prspr
.noSpr0

	halt

	ld a,(_screenActive)
	xor 2
	ld (_screenActive),a
	ld e,a

	ld a,#10
	bit 1,e
	jr z,$+4
	or #08
	ld bc,#7ffd
	out (c),a

	pop af
	jr z,.noSpr1

	call setShadowScreen
	call respr
	call updateTilesFromBuffer
	MRestoreMemMap012
.noSpr1
	pop iy
	pop ix
	ret



pal_get_address
	ld h,0
	ld l,a
	add hl,hl
	add hl,hl
	add hl,hl
	add hl,hl

	ld bc,MEM_SLOT0
	ld a,PAL_PAGE
	out (c),a
	ret



_pal_select
	call pal_get_address

	ld de,_palette
	ld bc,16
	ldir

	ld a,d
	ld (_palChange),a

	ld bc,MEM_SLOT0
	ld a,CC_PAGE0
	out (c),a
	ret



_pal_bright
	cp 7
	jr c,.l1
	ld a,6
.l1
	ld h,a
	ld l,0
	srl h
	rr l
	srl h
	rr l
	ld (_palBright),hl
	ld a,1
	ld (_palChange),a
	ret



_pal_copy
	push de
	call pal_get_address

	ld de,palTemp
	ld bc,16
	ldir

	ld bc,MEM_SLOT0
	ld a,CC_PAGE0
	out (c),a

	pop de
	ld hl,palTemp
	ld bc,16
	ldir

	ret



	include "lib_input.asm"
	include "lib_sound.asm"
	include "lib_tiles.asm"
	include "lib_sprites.asm"



im2handler
	push af
	push bc
	push de
	push hl
	push ix
	push iy
	exa
	exx
	push af
	push bc
	push de
	push hl


	ld a,(_palChange)
	or a
	jp z,.noPalette

	;изменение палитры

	ld de,(_palBright)
	ld a,d
	add a,high palBrightTable
	ld b,a

	ld hl,_palette

.colId=0
	dup 8
	ld a,.colId
	out (#fe),a
	ld a,(hl)
	add a,e
	ld c,a
	ld a,(bc)
	out (#ff),a
	inc l
.colId=.colId+1
	edup
.colId=0
	dup 8
	ld a,.colId
	out (#f6),a
	ld a,(hl)
	add a,e
	ld c,a
	ld a,(bc)
	out (#ff),a
	inc l
.colId=.colId+1
	edup

	;восстановление цвета бордюра

	ld a,(_borderCol)
	ld c,a
	and 7
	bit 3,c
	jr nz,.bright
	out (#fe),a
	jr .palSet
.bright
	out (#f6),a
.palSet
	xor a
	ld (_palChange),a

.noPalette


	ld a,SND_PAGE
	ld bc,MEM_SLOT1
	out (c),a

	ld a,(musicPage)
	or a
	jr z,.noMusic
	ld bc,MEM_SLOT2
	out (c),a
	ld bc,#fffd		;второй чип Turbo Sound
	ld a,#fe		;если Turbo Sound нет, звуки и музыка
	out (c),a		;играют на одном чипе, иначе на разных
	call PT3_FRAME
	ld a,(turboSound)
	or a
	jr z,.sfx
.noMusic
	ld a,1
	call reset_ay
.sfx
	ld bc,#fffd		;первый чип
	out (c),b
	call AFX_FRAME

	poll_mouse

	ld a,(_memSlot1)
	ld bc,MEM_SLOT1
	out (c),a
	ld a,(_memSlot2)
	ld bc,MEM_SLOT2
	out (c),a

	;счЄтчик кадров

	ld hl,_time
	ld b,4
.time1
	inc (hl)
	jr nz,.time2
	inc hl
	djnz .time1
.time2

	pop hl
	pop de
	pop bc
	pop	af
	exx
	exa
	pop iy
	pop ix
	pop hl
	pop de
	pop bc
	pop af
	ei
	ret



;таблицы

	align 256
palBrightTable
	db #ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff	;bright 0
	db #ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff
	db #ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff
	db #ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff,#ff
	db #ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#7f,#7f,#7f,#3f	;bright 1
	db #ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#7f,#7f,#7f,#3f
	db #ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#ff,#ff,#ff,#bf,#7f,#7f,#7f,#3f
	db #df,#df,#df,#9f,#df,#df,#df,#9f,#df,#df,#df,#9f,#5f,#5f,#5f,#1f
	db #ff,#ff,#bf,#fd,#ff,#ff,#bf,#fd,#7f,#7f,#3f,#7d,#ef,#ef,#af,#ed	;bright 2
	db #ff,#ff,#bf,#fd,#ff,#ff,#bf,#fd,#7f,#7f,#3f,#7d,#ef,#ef,#af,#ed
	db #df,#df,#9f,#dd,#df,#df,#9f,#dd,#5f,#5f,#1f,#5d,#cf,#cf,#8f,#cd
	db #fe,#fe,#be,#fc,#fe,#fe,#be,#fc,#7e,#7e,#3e,#7c,#ee,#ee,#ae,#ec
	db #ff,#bf,#fd,#bd,#7f,#3f,#7d,#3d,#ef,#af,#ed,#ad,#6f,#2f,#6d,#2d	;bright 3
	db #df,#9f,#dd,#9d,#5f,#1f,#5d,#1d,#cf,#8f,#cd,#8d,#4f,#0f,#4d,#0d
	db #fe,#be,#fc,#bc,#7e,#3e,#7c,#3c,#ee,#ae,#ec,#ac,#6e,#2e,#6c,#2c
	db #de,#9e,#dc,#9c,#5e,#1e,#5c,#1c,#ce,#8e,#cc,#8c,#4e,#0e,#4c,#0c
	db #1f,#5d,#1d,#1d,#8f,#cd,#8d,#8d,#0f,#4d,#0d,#0d,#0f,#4d,#0d,#0d	;bright 4
	db #3e,#7c,#3c,#3c,#ae,#ec,#ac,#ac,#2e,#6c,#2c,#2c,#2e,#6c,#2c,#2c
	db #1e,#5c,#1c,#1c,#8e,#cc,#8c,#8c,#0e,#4c,#0c,#0c,#0e,#4c,#0c,#0c
	db #1e,#5c,#1c,#1c,#8e,#cc,#8c,#8c,#0e,#4c,#0c,#0c,#0e,#4c,#0c,#0c
	db #ec,#ac,#ac,#ac,#6c,#2c,#2c,#2c,#6c,#2c,#2c,#2c,#6c,#2c,#2c,#2c	;bright 5
	db #cc,#8c,#8c,#8c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c
	db #cc,#8c,#8c,#8c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c
	db #cc,#8c,#8c,#8c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c,#4c,#0c,#0c,#0c
	db #0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c	;bright 6
	db #0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c
	db #0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c
	db #0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c,#0c

	align 256	;#nn00
scrTable
adr=#4000
	dup 25
	db low adr
adr=adr+(40*8)
	edup
	align 32	;#nn20
adr=#4000
	dup 25
	db high adr
adr=adr+(40*8)
	edup



;номер страницы инвертирован

setSlot0
	ld (_memSlot0),a
	ld bc,MEM_SLOT0
	out (c),a
	ret

setSlot1
	ld (_memSlot1),a
	ld bc,MEM_SLOT1
	out (c),a
	ret

setSlot2
	ld (_memSlot2),a
	ld bc,MEM_SLOT2
	out (c),a
	ret



clearPage
	call setSlot1
	ld hl,#4000
	ld de,#4001
	ld bc,#3fff
	ld (hl),l
	jp _fast_ldir



	align 256	;#nn00
tileUpdateXTable
	dup 8
	db #01,#02,#04,#08,#10,#20,#40,#80
	edup
.x=0
	dup 64
	db .x>>3
.x=.x+1
	edup


	align 256	;#nn00
colorMaskTable
	db #00,#09,#12,#1b,#24,#2d,#36,#3f	;дл€ двух пикселей
	db #c0,#c9,#d2,#db,#e4,#ed,#f6,#ff
	db #00,#01,#02,#03,#04,#05,#06,#07	;дл€ ink
	db #40,#41,#42,#43,#44,#45,#46,#47
	ds 16,0
	db #00,#08,#10,#18,#20,#28,#30,#38	;дл€ paper
	db #80,#88,#90,#98,#a0,#a8,#b0,#b8


	align 256
_sprqueue
_sprqueue0	;формат 4 байта на спрайт, idh,idl,y,x (idh=255 конец списка)
	ds 256,255
_sprqueue1
	ds 256,255


	display "Top ",/h,$," (should be <=0xFD00)"


	org #fd00
tileUpdateMap	;битова€ карта обновившихс€ знакомест, 64x25 бит
	ds 8*25,0

	org #fdfd
	jp im2handler

	org #fe00
im2vector
	ds 257,#fd

;переменные

musicPage		db 0
tileOffset		dw 0
spritesActive	db 0	;1 если вывод спрайтов разрешЄн
tileUpdate		db 0	;1 если выводились тайлы, дл€ системы обновлени€ фона под спрайтами
palTemp			ds 16,0
keysPrevState	ds 40,0
turboSound		db 0	;1 если есть TS

;экспортируемые переменные

	macro rgb222 b2,g2,r2
	db (((r2&3)<<4)|((g2&3)<<2)|(b2&3))
	endm

	align 16
_palette
	rgb222(0,0,0)
	rgb222(0,0,2)
	rgb222(2,0,0)
	rgb222(2,0,2)
	rgb222(0,2,0)
	rgb222(0,2,2)
	rgb222(2,2,0)
	rgb222(2,2,2)
	rgb222(0,0,0)
	rgb222(0,0,3)
	rgb222(3,0,0)
	rgb222(3,0,3)
	rgb222(0,3,0)
	rgb222(0,3,3)
	rgb222(3,3,0)
	rgb222(3,3,3)

_memSlot0	db 0
_memSlot1	db 0
_memSlot2	db 0
_borderCol	db 0
_palBright	dw 3<<6
_palChange	db 1
_screenActive	db 0	;~1 или ~3
_mouse_dx	db 0
_mouse_dy	db 0
_mouse_x	db 80
_mouse_y	db 100
_mouse_cx1	db 0
_mouse_cx2	db 160
_mouse_cy1	db 0
_mouse_cy2	db 200
_mouse_btn	db 0
_mouse_prev_dx	db 0
_mouse_prev_dy	db 0
_time		dd 0

	export _palette
	export _memSlot0
	export _memSlot1
	export _memSlot2
	export _borderCol
	export _palBright
	export _palChange
	export _sprqueue
	export _screenActive
	export _mouse_dx
	export _mouse_dy
	export _mouse_x
	export _mouse_y
	export _mouse_cx1
	export _mouse_cx2
	export _mouse_cy1
	export _mouse_cy2
	export _mouse_btn
	export _time

;экспортируемые функции

	export _pal_select
	export _pal_copy
	export _pal_bright
	export _swap_screen
	export _clear_screen
	export _fast_ldir

end

	display "Size ",/d,end-begin," bytes"

	savebin "startup.bin",begin,end-begin