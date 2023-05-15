#pragma once
#include "draw.h"

extern VCTR vid;
CACHE_ALIGNED extern u32 vbuf[2][sizeof_vbuf];

void render_1x(u8 *dst, u32 pitch);
void render_2x(u8 *dst, u32 pitch);
void render_2xs(u8 *dst, u32 pitch);
void render_3x(u8 *dst, u32 pitch);
void render_4x(u8 *dst, u32 pitch);

static void flip_gdi(void);
static void flip_blt(void);
static void flip_d3d(void);
