#pragma once
#include "sysdefs.h"

void gdi_frame();
void _render_black(u8 *dst, unsigned pitch);

void rend_frame8(u8 *dst, unsigned pitch);
void rend_frame16(u8 *dst, unsigned pitch);
void rend_frame32(u8 *dst, unsigned pitch);

void rend_frame_8d1(u8 *dst, unsigned pitch);
void rend_frame_8d(u8 *dst, unsigned pitch);
void rend_frame_16d1(u8 *dst, unsigned pitch);
void rend_frame_16d(u8 *dst, unsigned pitch);
void rend_frame_32d1(u8 *dst, unsigned pitch);
void rend_frame_32d(u8 *dst, unsigned pitch);
