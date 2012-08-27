#pragma once

extern u32 vbuf[];

void __fastcall render_1x(unsigned char *dst, unsigned pitch);
void __fastcall render_2x(unsigned char *dst, unsigned pitch);
void __fastcall render_3x(unsigned char *dst, unsigned pitch);
void __fastcall render_4x(unsigned char *dst, unsigned pitch);

void __fastcall render_tv(unsigned char *dst, unsigned pitch);
void __fastcall render_bil(unsigned char *dst, unsigned pitch);
void __fastcall render_scale(unsigned char *dst, unsigned pitch);
