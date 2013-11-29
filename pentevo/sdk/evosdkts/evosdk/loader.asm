	device pentagon1024

colorTop =#47
colorLoad=#46
colorLeft=#04

	org #5d3b-17

;заголовок hobeta, savehob не используется так как нужно задать автостарт

hobeta
	db "boot    B"
	dw line_end-begin
	dw line_end-begin
	db 0,high ((end-begin)+4)
	dw 0	;контрольная сумма

	;org #5d3b

begin
	db 0,1			;номер строки
	dw line_end-line_start	;длина строки в байтах
line_start
    db #fd,#30,#0e,#00,#00,#ff,#6f,#00,#3a
	db #f9,#c0,#30	;randomize usr 0
	db #0e,#00,#00	;число
	dw code_start
	db #00,#3a,#ea	;:rem


code_start
;	ld sp,#6fff

	;включение режима 14 МГц

	ld bc,#20af 
	ld a,2      ; 14 MHz
	out (c),a

	;очистка экрана

	halt
	xor a
	out (#fe),a
	ld hl,23295
	ld de,23294
	ld bc,6911
	ld (hl),a
	lddr

	ld bc,#12af
	ld (#8000),a
	out (c),a
	dec a
	ld (#8000),a
	ld a,2
	out (c),a
	ld a,(#8000)
	and a
	jp nz,notsc

	;рисование полоски

	ld a,#ee
	ld hl,20480+4*32+6*256
	call fill32bytes
	inc h
	call fill32bytes

	ld hl,20480+5*32
	ld b,4
.l1
	call fill32bytes
	inc h
	djnz .l1

	;вывод сообщения

	ld de,20480+6*32
	ld ix,messageStr
	call prstr
.l4
	ld hl,22528+22*32
	ld a,7+64
	call fill32bytes

	call displayBar

	;установка обработчика ошибок

	ld hl,checkError
	ld (23747),hl
	ld a,#c3
	ld (23746),a

	;загрузка блоков по списку

	ld hl,fileList
loadLoop
	ld a,(hl)
	inc hl
	ld c,(hl)
	inc hl

	push af
	push hl
					;включаем страницу C в верхнем окне
	ld a,c
	ld bc,#13af
	out (c),a	
	pop hl
	pop af

	or a
	jr z,loadDone

	push hl

	ld de,(23796)	;упакованный файл загружается в #7000
	ld b,a			;на случай если он больше распакованного
	ld c,#05
	ld hl,#7000
	call 15635

	call advanceBar

	di
	ld hl,#7000
	ld de,#c000
	call DEC40
	ei

	call advanceBar

	pop hl
	jr loadLoop

loadDone
	ld a,(hl)	;читаем адрес запуска
	inc hl
	ld h,(hl)
	ld l,a

	ld bc,32*256
	ld (progressNow),bc
	call displayBar

	jp (hl)

notsc
	ld de,#4800+3*32+7
	ld ix,errmsg1
	call prstr
	ld de,#4800+4*32+5
	ld ix,errmsg2
	call prstr
	ld hl,#4823
	ld bc,25
	call fillff
	ld hl,#4923
	call fillff
	ld hl,#4e00+6*32+3
	call fillff
	ld hl,#4f00+6*32+3
	call fillff
	ld hl,#4a23
	ld b,44
.l1	ld (hl),#c0
	ld d,l
	ld a,25
	add a,l
	ld l,a
	ld (hl),3
	ld l,d
    inc h
    ld a,h
    and 7
    jr nz,.l2
    ld a,l
    add a,32
    ld l,a
    jr c,.l2
    ld a,h
    sub 8
    ld h,a
.l2 djnz .l1   
	ei
	halt
	ld hl,#5800
	ld de,#5801
	ld bc,#2ff
	ld (hl),#42
	ldir
	jr $

fillff
	push bc
	ld d,h
	ld e,l
	inc de
	ld (hl),#ff
	ldir
	pop bc
	ret

prstr
	ld a,(ix)
	or a
	ret z

	push de

	ld l,a
	ld h,0
	add hl,hl
	add hl,hl
	add hl,hl
	ld bc,15616-32*8
	add hl,bc
	ld b,8
.l3
	ld a,(hl)
	sla a
	or (hl)
	ld (de),a
	inc l
	inc d
	djnz .l3

	pop de
	inc e
	inc ix
	jr prstr



fill32bytes
	push bc
	push hl
	ld b,32
.l1
	ld (hl),a
	inc l
	djnz .l1
	pop hl
	pop bc
	ret



displayBar
	ld a,(progressNow+1)
	cp 32
	jr c,$+4
	ld a,32

	ld ix,22528+20*32
	ld bc,32*256
.l1
	cp c
	ld de,colorLeft|(colorLeft<<8)
	jr c,.l2
	jr z,.l2
	ld de,colorTop|(colorLoad<<8)
.l2
	ld (ix),e
	ld (ix+32),d
	inc ixl
	inc c
	djnz .l1

	ret



advanceBar
	call displayBar
	ld hl,(progressNow)
	ld bc,(progressStep)
	add hl,bc
	ld (progressNow),hl
	ret



checkError
	ld a,(23823)
	or a
	ret z

showError
	xor a
.l1
	halt
	push af
	and 7
	out (#fe),a
	pop af
	ld hl,22528
	ld de,22529
	ld bc,767
	ld (hl),a
	ldir
	xor #12
	ld b,25
	halt
	djnz $-1
	jr .l1



	include "unmegalz.asm"

errmsg1  db "System meditation:",0
errmsg2  db "! ZXEvo TS not found !",0

progressNow	dw 0	;текущий прогресс загрузки 8:8, старший байт 0-31

;параметры загрузки

;messageStr
;	db " SOMETHING IS LOADING",0

;progressStep
;	dw 10	;шаг для прогрессбара

;fileList	;в списке длина в секторах и номер страницы (без инверсии)
;	db 100,0
;	db 0,0	;последняя запись 0 секторов, включается нужная страница
;	dw 0	;адрес запуска

	include "filelist.asm"

line_end

	display "Basic length ",/d,$-begin," bytes"

	db #80,#aa,#01,#00	;номер строки для автостарта

	ds (($-1-begin)&255)^255,0
end

	display "File size ",/d,end-begin," bytes"

	savebin "boot.$b",hobeta,end-hobeta