#pragma once

void __fastcall render_small(unsigned char *dst, unsigned pitch);
void __fastcall render_dbl(unsigned char *dst, unsigned pitch);
void __fastcall render_3x(unsigned char *dst, unsigned pitch);
void __fastcall render_quad(unsigned char *dst, unsigned pitch);

void __fastcall render_tv(unsigned char *dst, unsigned pitch);
void __fastcall render_bil(unsigned char *dst, unsigned pitch);
void __fastcall render_scale(unsigned char *dst, unsigned pitch);

void rend_dbl(unsigned char *dst, unsigned pitch);
void rend_small(unsigned char *dst, unsigned pitch);
