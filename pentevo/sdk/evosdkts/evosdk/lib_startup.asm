	device pentagon1024



MEM_SLOT0=#10AF
MEM_SLOT1=#11AF
MEM_SLOT2=#12AF
MEM_SLOT3=#13AF
;screen 0 - 8,9,10,11
;screen 1 - 16,17,18,19
SPBUF_PAGE0=12 
SPBUF_PAGE1=13
SPBUF_PAGE2=14
SPBUF_PAGE3=15

SPTBL_PAGE=6

CC_PAGE0=20
CC_PAGE1=21
CC_PAGE2=22
SND_PAGE=0
PAL_PAGE=1
GFX_PAGE=24

FMAddr=#15AF
VPAGE =#01AF

AFX_INIT =#4000
AFX_PLAY =#4003
AFX_FRAME=#4006
PT3_INIT =#4009
PT3_FRAME=#400c

IMG_LIST =#1000

;смещени€ в SND_PAGE

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
	inc a
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

	macro MROMEnable

	ld bc,MEM_SLOT0
	ld a,#ff
	ld (_romActive),a
	inc a
	out (c),a
	ld bc,#21AF
	ld a,4
	out (c),a
	
	endm

	macro MROMDisable

	ld bc,MEM_SLOT0
	xor a
	ld (_romActive),a
	ld a,CC_PAGE0
	out (c),a
	ld a,%00001111 ; 
	ld bc,#21AF
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

	ld a,8
	ld (_screenActive),a
	call poll_mouse_delta

	;установка обработчика прерываний

	ld a,im2vector/256
	ld i,a
	im 2

	;гашение экрана
	ei
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

	ld a,8
	call clearPage
	ld a,9
	call clearPage
	ld a,10
	call clearPage
	ld a,11
	call clearPage
	ld a,16
	call clearPage
	ld a,17
	call clearPage
	ld a,18
	call clearPage
	ld a,19
	call clearPage

	ld bc,VPAGE
	ld a,#10
	out (c),a

	LD A,%01000001 ; 320 x 200   16c
	ld bc,#00AF
	out (c),a
	inc b
	ld a,#10
	out (c),a ; VPAGE
	inc b
	xor a
	out (c),a ; scroll offset 
	inc b
	out (c),a
	inc b
	out (c),a
	inc b
	out (c),a
	inc b
	out (c),a ; TSConfig
	inc b
	ld a,15
	out (c),a ; PalSel
	call _swap_screen

	;установка нормальной €ркости

	ld hl,3<<7
	ld (_palBright),hl
	ld a,1
	ld (_palChange),a
	halt

	;установка карты пам€ти дл€ выполнени€ кода
	;страница в верхнем окне не мен€етс€, всегда 11
	MROMDisable
;	ld a,CC_PAGE0
;	call setSlot0
	ld a,CC_PAGE1
	call setSlot1
	ld a,CC_PAGE2
	call setSlot2

	jp 0



;setActiveScreen
;	ld a,(_screenActive)
;	xor 2
;
;	ld bc,MEM_SLOT1
;	ld (_memSlot1),a
;	out (c),a
;
;	ld b,high MEM_SLOT2
;	sub 4
;	ld (_memSlot2),a
;	out (c),a
;
;	ret
;
;setShadowScreen
;	MSetShadowScreen
;	ret



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
	ld a,(hl)
	ld (.clcol),a
	MROMEnable
	ld a,(_screenActive)
	call setSlot1
	ld bc,#1CAF
	out (c),a
	ld b,#1F
	out (c),a
	ld hl,0
	ld b,#1A
	out (c),l
	inc b
	out (c),h
	inc h
	ld b,#1D
	out (c),l
	inc b
	out (c),h
	ld b,#26
	ld a,79
	out (c),a
	ld b,#28
	ld a,198
	out (c),a
	ld hl,#4000
	ld de,#4001
	ld bc,159
	ld (hl),0
.clcol equ $-1
	call _fast_ldir
	ld a,%00110001
	ld bc,#27AF
	out (c),a
	rst 8 ; DMA
	MROMDisable
	MRestoreMemMap12
	ret

_swap_screen
	push ix
	push iy

	ld a,(spritesActive)
	or a
	push af
	jr z,.noSpr0
	call updateTilesToBuffer
	call prspr
.noSpr0

	halt
	ld a,(_screenActive)
	ld bc,VPAGE
	out (c),a
	xor #18
	ld (_screenActive),a

	pop af
	jr z,.noSpr1

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

	ld bc,FMAddr
	ld a,#10
	out (c),a
	ld bc,#21AF    ; memconfig
	ld a,4 ; 
	out (c),a
	ld de,(_palBright)
	ld a,d
	add a,high palBrightTable
	ld b,a

	ld hl,_palette
	ld ix,#1e0 ; palette F0..FF
	dup 16
	ld a,(hl)
	add a,a
	add a,e
	ld c,a
	ld a,(bc)
	ld (ix),a
	inc bc
	inc ix
	ld a,(bc)
	ld (ix),a
	inc ix
	inc l
	edup
	ld bc,FMAddr
	xor a
	out (c),a
	ld a,(_romActive)
	and a
	jr nz,.rom
	ld bc,#21AF    ; memconfig
	ld a,%00001111 ; 
	out (c),a
.rom	
	;восстановление цвета бордюра
	ld a,(_borderCol)
	or #f0
	ld bc,#0FAF
	out (c),a
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
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00 ; bright 0
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00,#00
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#00,#00,#00,#00,#00,#00,#20 ; bright 1
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#01,#00,#01,#00,#01,#00,#21
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#00,#00,#00,#00,#00,#00,#20
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#01,#00,#01,#00,#01,#00,#21
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#00,#00,#00,#00,#00,#00,#20
	db #00,#00,#00,#00,#00,#00,#00,#20,#00,#01,#00,#01,#00,#01,#00,#21
	db #08,#00,#08,#00,#08,#00,#08,#20,#08,#00,#08,#00,#08,#00,#08,#20
	db #08,#00,#08,#00,#08,#00,#08,#20,#08,#01,#08,#01,#08,#01,#08,#21
	db #00,#00,#00,#00,#00,#20,#00,#40,#00,#00,#00,#00,#00,#20,#00,#40 ; bright 2
	db #00,#01,#00,#01,#00,#21,#00,#41,#00,#02,#00,#02,#00,#22,#00,#42
	db #00,#00,#00,#00,#00,#20,#00,#40,#00,#00,#00,#00,#00,#20,#00,#40
	db #00,#01,#00,#01,#00,#21,#00,#41,#00,#02,#00,#02,#00,#22,#00,#42
	db #08,#00,#08,#00,#08,#20,#08,#40,#08,#00,#08,#00,#08,#20,#08,#40
	db #08,#01,#08,#01,#08,#21,#08,#41,#08,#02,#08,#02,#08,#22,#08,#42
	db #10,#00,#10,#00,#10,#20,#10,#40,#10,#00,#10,#00,#10,#20,#10,#40
	db #10,#01,#10,#01,#10,#21,#10,#41,#10,#02,#10,#02,#10,#22,#10,#42
	db #00,#00,#00,#20,#00,#40,#00,#60,#00,#01,#00,#21,#00,#41,#00,#61 ; bright 3
	db #00,#02,#00,#22,#00,#42,#00,#62,#00,#03,#00,#23,#00,#43,#00,#63
	db #08,#00,#08,#20,#08,#40,#08,#60,#08,#01,#08,#21,#08,#41,#08,#61
	db #08,#02,#08,#22,#08,#42,#08,#62,#08,#03,#08,#23,#08,#43,#08,#63
	db #10,#00,#10,#20,#10,#40,#10,#60,#10,#01,#10,#21,#10,#41,#10,#61
	db #10,#02,#10,#22,#10,#42,#10,#62,#10,#03,#10,#23,#10,#43,#10,#63
	db #18,#00,#18,#20,#18,#40,#18,#60,#18,#01,#18,#21,#18,#41,#18,#61
	db #18,#02,#18,#22,#18,#42,#18,#62,#18,#03,#18,#23,#18,#43,#18,#63
	db #08,#21,#08,#41,#08,#61,#08,#61,#08,#22,#08,#42,#08,#62,#08,#62 ; bright 4
	db #08,#23,#08,#43,#08,#63,#08,#63,#08,#23,#08,#43,#08,#63,#08,#63
	db #10,#21,#10,#41,#10,#61,#10,#61,#10,#22,#10,#42,#10,#62,#10,#62
	db #10,#23,#10,#43,#10,#63,#10,#63,#10,#23,#10,#43,#10,#63,#10,#63
	db #18,#21,#18,#41,#18,#61,#18,#61,#18,#22,#18,#42,#18,#62,#18,#62
	db #18,#23,#18,#43,#18,#63,#18,#63,#18,#23,#18,#43,#18,#63,#18,#63
	db #18,#21,#18,#41,#18,#61,#18,#61,#18,#22,#18,#42,#18,#62,#18,#62
	db #18,#23,#18,#43,#18,#63,#18,#63,#18,#23,#18,#43,#18,#63,#18,#63
	db #10,#42,#10,#62,#10,#62,#10,#62,#10,#43,#10,#63,#10,#63,#10,#63 ; bright 5
	db #10,#43,#10,#63,#10,#63,#10,#63,#10,#43,#10,#63,#10,#63,#10,#63
	db #18,#42,#18,#62,#18,#62,#18,#62,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#43,#18,#63,#18,#63,#18,#63,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#42,#18,#62,#18,#62,#18,#62,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#43,#18,#63,#18,#63,#18,#63,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#42,#18,#62,#18,#62,#18,#62,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#43,#18,#63,#18,#63,#18,#63,#18,#43,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63 ; bright 6
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63
	db #18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63,#18,#63

	align 256	;#nn00
scrTable
adr=#0000
	dup 25
	db (high adr) & #3F
adr=adr+(256*8)
	edup
	align 32	;#nn20
adr=#0000
	dup 25
	db (high adr) >> 6
adr=adr+(256*8)
	edup



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
	db #00,#11,#22,#33,#44,#55,#66,#77	;дл€ двух пикселей
	db #88,#99,#aa,#bb,#cc,#dd,#ee,#ff
	db #00,#10,#20,#30,#40,#50,#60,#70	;дл€ ink
	db #80,#90,#a0,#b0,#c0,#d0,#e0,#f0
	ds 16,0
	db #00,#01,#02,#03,#04,#05,#06,#07	;дл€ paper
	db #08,#09,#0a,#0b,#0c,#0d,#0e,#0f


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
im2ad equ $-2
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
_romActive  db 0
_borderCol	db 0
_palBright	dw 3<<7
_palChange	db 1
_screenActive	db 0	;8 или 16
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
	export _romActive
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