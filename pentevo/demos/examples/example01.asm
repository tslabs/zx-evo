
; This example creates 3 pixel planes with pre-selected parameters.
; Two upper pixel planes are created of tiles and have 15 colors palette with trasparent color 0.
; Lower plane is graphics video memory and can be of 16 or 256 colors palette.
; Note that total CRAM size is 256 colors and 16/15c modes share its part with 256c mode.
;
; В этом примере создаются 3 пиксельные плоскости с заданными параметрами.
; Две верхние состоят из тайлов и имеют 15-цветную палитру, в которой цвет 0 прозрачный.
; Нижняя плоскость - это графическая видеопамять, для которой можно задать режим 256/16 цветов на точку.
; Обратите внимание на тот факт, что общий объем палитрового ОЗУ - 256 ячеек, поэтому режимы 16/15 цветов на точку
; используют часть палитры от режима 256 цветов на точку.

	include "tsconfig.asm"
	; include "tsfuncs.asm"

PLANE0_PAGE		equ $20			; the first RAM page for bottom pixel plane, it can be either 16 or 256 colors per pixel
PLANE1_PAGE		equ $30			; the first RAM page for middle pixel plane, it can be 15 colors per pixel only
PLANE2_PAGE		equ $38			; the first RAM page for top pixel plane, it can be 15 colors per pixel only

TILEMAP_PAGE	equ $1F			; this page is used for TileMap

PLANE0_MODE		equ VID_16C
; PLANE0_MODE		equ VID_256C	; uncomment either

; RASTER_SIZE		equ VID_256X192
; RASTER_SIZE		equ VID_320X200
RASTER_SIZE		equ VID_320X240
; RASTER_SIZE		equ VID_360X288		; uncomment either

PLANE0_PALETTE	equ 13			; palettes select which 16 colors part of CRAM is used for current plane, where 0 is colors 0..15, and 15 is colors 240-255
PLANE1_PALETTE	equ 14			; for 256c mode the whole CRAM is used, so this definition will be ignored
PLANE2_PALETTE	equ 15

	org $8000
	; this address is arbitrary, you are free to use any
	; note that example uses window $C000 for banks access, so org addr and stack should be set to lower values

	; turn off graphics to prevent noise on video while configuring
	ld bc, VCONFIG
	ld a, VID_NOGFX
	out (c), a
	ld b, high TSCONFIG
	xor a
	out (c), a

	; fill TileMap with incrementing values
	; the TileMap elements are interleaved as 128 + 128 bytes of tile planes 0 and 1
	; a TileMap element is a 16 bit word of the following structure:
	;	bits 0..11 - tile bitmap selector
	;	bits 12..13 - 2 LSBs of tile plane palette selector
	;	bit 14 - horizontal flip
	;	bit 15 - vertical flip
	ld b, high PAGE3
	ld a, TILEMAP_PAGE
	out (c), a

	ld hl, $C000
	ld de, 0

EX01_03:
	ld b, 64
	ld a, d
	or (PLANE1_PALETTE << TL_PAL_BS) & TL_PAL_MASK
	ld c, a
EX01_01:
	ld (hl), e
	inc l
	ld (hl), c
	inc l
	inc e
	djnz EX01_01

	ld a, e
	sub 64
	ld e, a

	ld b, 64
	ld a, d
	or (PLANE2_PALETTE << TL_PAL_BS) & TL_PAL_MASK
	ld c, a
EX01_02:
	ld (hl), e
	inc l
	ld (hl), c
	inc l
	inc de
	djnz EX01_02

	inc h
	jr nz, EX01_03

	; set TileMap page
	ld bc, TMPAGE
	ld a, TILEMAP_PAGE
	out (c), a

	; set RAM page for graphics
	ld b, high VPAGE
	ld a, PLANE0_PAGE
	out (c), a

	; set RAM pages for tile planes
	ld b, high T0GPAGE
	ld a, PLANE1_PAGE
	out (c), a

	ld b, high T1GPAGE
	ld a, PLANE2_PAGE
	out (c), a

	; set palette selector for all planes
	ld b, high PALSEL
	ld a, PLANE0_PALETTE | (((PLANE1_PALETTE >> 2) << PAL_T0PAL_BS) & PAL_T0PAL_MASK) | (((PLANE2_PALETTE >> 2) << PAL_T1PAL_BS) & PAL_T1PAL_MASK)
	out (c), a

	; zeroing scrollers to avoid surprises from previous usage
	xor a
	ld b, high GXOFFSL : out (c), a
	ld b, high GXOFFSH : out (c), a
	ld b, high GYOFFSL : out (c), a
	ld b, high GYOFFSH : out (c), a
	ld b, high T0XOFFSL : out (c), a
	ld b, high T0XOFFSH : out (c), a
	ld b, high T0YOFFSL : out (c), a
	ld b, high T0YOFFSH : out (c), a
	ld b, high T1XOFFSL : out (c), a
	ld b, high T1XOFFSH : out (c), a
	ld b, high T1YOFFSL : out (c), a
	ld b, high T1YOFFSH : out (c), a

	; finally turn on graphics
	ld b, high VCONFIG
	ld a, PLANE0_MODE | RASTER_SIZE
	out (c), a

	; ... and tile planes
	ld b, high TSCONFIG
	ld a, TSU_T0EN | TSU_T1EN | TSU_T0ZEN | TSU_T1ZEN
	out (c), a

	ret

	output "example01.bin"
