#pragma once
#include "draw.h"

extern VCTR vid;
CACHE_ALIGNED extern u32 vbuf[2][sizeof_vbuf];

void render_1x(u8 *dst, u32 pitch);
void render_2x(u8 *dst, u32 pitch);
void render_2xs(u8 *dst, u32 pitch);
void render_3x(u8 *dst, u32 pitch);
void render_4x(u8 *dst, u32 pitch);

void FlipGdi(void);
void FlipBlt(void);
void FlipD3d(void);
