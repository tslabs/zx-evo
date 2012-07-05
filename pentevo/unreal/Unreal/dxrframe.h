#pragma once

void gdi_frame();
void _render_black(unsigned char *dst, unsigned pitch);

void rend_frame8(unsigned char *dst, unsigned pitch);
void rend_frame16(unsigned char *dst, unsigned pitch);
void rend_frame32(unsigned char *dst, unsigned pitch);

void rend_frame_8d1(unsigned char *dst, unsigned pitch);
void rend_frame_8d(unsigned char *dst, unsigned pitch);
void rend_frame_16d1(unsigned char *dst, unsigned pitch);
void rend_frame_16d(unsigned char *dst, unsigned pitch);
void rend_frame_32d1(unsigned char *dst, unsigned pitch);
void rend_frame_32d(unsigned char *dst, unsigned pitch);
