#include <evo.h>
#include "startup.h"	//этот файл генерируетс€ автоматически при компил€ции startup.asm



void memset(void* m,u8 b,u16 len) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld a,(hl)
	inc hl
	ld c,(hl)
	inc hl
	ld b,(hl)

	ex de,hl
	ld d,h
	ld e,l
	inc de
	dec bc
	ld (hl),a
	jp _FAST_LDIR
_endasm;
}



void memcpy(void* d,void* s,u16 len) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld e,(hl)
	inc hl
	ld d,(hl)
	push de
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	inc hl
	ld c,(hl)
	inc hl
	ld b,(hl)
	ex de,hl
	pop de
	jp _FAST_LDIR
_endasm;
}



void border(u8 n) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	ld (_BORDERCOL),a
	ld c,a
	and #7
	bit 3,c
	jr nz,1$
	out (0xfe),a
	ret
1$:
	out (0xf6),a
	ret
_endasm;
}



void vsync(void) _naked
{
_asm
	halt
	ret
_endasm;
}



u8 joystick(void) _naked
{
_asm
	jp _JOYSTICK
_endasm;
}



void keyboard(u8* keys) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld e,(hl)
	inc hl
	ld d,(hl)
	jp _KEYBOARD
_endasm;
}



u8 mouse_pos(u8* x,u8* y) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld a,(_MOUSE_X)
	ld (bc),a
	ld a,(_MOUSE_Y)
	ld (de),a
	ld a,(_MOUSE_BTN)
	ld l,a
	ret
_endasm;
}



void mouse_set(u8 x,u8 y) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	ld (_MOUSE_X),a
	inc hl
	ld a,(hl)
	ld (_MOUSE_Y),a
	jp _MOUSE_APPLY_CLIP
_endasm;
}



void mouse_clip(u8 xmin,u8 ymin,u8 xmax,u8 ymax) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	ld (_MOUSE_CX1),a
	inc hl
	ld a,(hl)
	ld (_MOUSE_CY1),a
	inc hl
	ld a,(hl)
	ld (_MOUSE_CX2),a
	inc hl
	ld a,(hl)
	ld (_MOUSE_CY2),a
	jp _MOUSE_APPLY_CLIP
_endasm;
}



u8 mouse_delta(i8* x,i8* y) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	ld a,(_MOUSE_DX)
	ld (bc),a
	ld a,(_MOUSE_DY)
	ld (bc),a
	ld a,(_MOUSE_BTN)
	ld l,a
	ret
_endasm;
}



void sfx_play(u8 sfx,i8 vol) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld b,(hl)
	inc hl
	ld c,(hl)
	jp _SFX_PLAY
_endasm;
}



void sfx_stop(void) _naked
{
_asm
	jp _SFX_STOP
_endasm;
}



void music_play(u8 mus) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	jp _MUSIC_PLAY
_endasm;
}



void music_stop(void) _naked
{
_asm
	jp _MUSIC_STOP
_endasm;
}



void sample_play(u8 sample) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld l,(hl)
	jp _SAMPLE_PLAY
_endasm;
}



u16 rand16(void) _naked
{
_asm
	ld hl,(1$)
	push hl
	srl h
	rr l
	ex de,hl
	ld hl,(2$)
	add hl,de
	ld (2$),hl
	ld a,l
	xor #15
	ld l,a
	ex de,hl
	pop hl
	sbc hl,de
	ld (1$),hl
	ret

1$:	.dw 1
2$:	.dw 5

_endasm;
}



void pal_clear(void) _naked
{
_asm
	ld hl,#_PALETTE
	ld bc,#0x1000
1$:
	ld (hl),c
	inc l
	djnz 1$
	ld a,h
	ld (_PALCHANGE),a
	ret
_endasm;
}



void pal_select(u8 id) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	jp _PAL_SELECT
_endasm;
}



void pal_bright(u8 bright) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	jp _PAL_BRIGHT
_endasm;
}



void pal_col(u8 id,u8 col) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	inc hl
	ld c,(hl)
	ld hl,#_PALETTE
	add a,l
	ld l,a
	ld a,c
	and #63
	ld (hl),a
	ld a,h
	ld (_PALCHANGE),a
	ret
_endasm;
}



void pal_copy(u8 id,u8* pal) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	jp _PAL_COPY
_endasm;
}



void pal_custom(u8* pal) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	inc hl
	ld h,(hl)
	ld l,a
	ld de,#_PALETTE
	ld b,#16
1$:
	ld a,(hl)
	and #63
	ld (de),a
	inc hl
	inc e
	djnz 1$
	ld a,d
	ld (_PALCHANGE),a
	ret
_endasm;
}



void draw_tile(u8 x,u8 y,u16 tile) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	jp _DRAW_TILE
_endasm;
}



void draw_tile_key(u8 x,u8 y,u16 tile) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld e,(hl)
	inc hl
	ld d,(hl)
	jp _DRAW_TILE_KEY
_endasm;
}



void draw_image(u8 x,u8 y,u8 id) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	inc hl
	ld a,(hl)
	jp _DRAW_IMAGE
_endasm;
}



void clear_screen(u8 color) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)
	jp _CLEAR_SCREEN
_endasm;
}



void swap_screen(void) _naked
{
_asm
	jp _SWAP_SCREEN
_endasm;
}



void select_image(u8 id) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld l,(hl)
	jp _SELECT_IMAGE
_endasm;
}



void color_key(u8 col) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	jp _COLOR_KEY
_endasm;
}



void set_sprite(u8 id,u8 x,u8 y,u16 spr) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld a,(hl)	;id
	inc hl
	ld c,(hl)	;x
	inc hl
	ld b,(hl)	;y
	inc hl
	ld e,(hl)	;sprl
	inc hl
	ld d,(hl)	;sprh
	
	add a,a
	add a,a
	ld l,a
	ld h,#_SPRQUEUE/256

	ld a,d		;пересчЄт номера спрайта
	cp #255
	jr z,1$
	add a,a
	add a,d
	ld d,a
1$:
	ld a,(_SCREENACTIVE)
	and #2
	jr nz,2$
	inc h
2$:
	ld (hl),d
	inc l
	ld (hl),e
	inc l
	ld (hl),b
	inc l
	ld (hl),c
	ret
_endasm;
}



void sprites_start(void) _naked
{
_asm
	jp _SPRITES_START
_endasm;
}



void sprites_stop(void) _naked
{
_asm
	jp _SPRITES_STOP
_endasm;
}



u32 time(void) _naked
{
_asm
	ld hl,#_TIME+3
	ld d,(hl)
	dec hl
	ld e,(hl)
	dec hl
	ld a,(hl)
	dec hl
	ld l,(hl)
	ld h,a
	ret
_endasm;
}



void delay(u16 time) _naked
{
_asm
	ld hl,#2
	add hl,sp
	ld c,(hl)
	inc hl
	ld b,(hl)
	ld a,b
	or c
	ret z
1$:
	halt
	dec bc
	ld a,b
	or c
	jr nz,1$
	ret
_endasm;
}