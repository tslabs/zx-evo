#pragma once
#include "draw.h"

extern VCTR vid;
CACHE_ALIGNED extern u32 vbuf[2][sizeof_vbuf];

void __fastcall render_1x(u8 *dst, u32 pitch);
void __fastcall render_2x(u8 *dst, u32 pitch);
void __fastcall render_3x(u8 *dst, u32 pitch);
void __fastcall render_4x(u8 *dst, u32 pitch);

void __fastcall render_tv(u8 *dst, u32 pitch);
void __fastcall render_bil(u8 *dst, u32 pitch);
void __fastcall render_scale(u8 *dst, u32 pitch);
