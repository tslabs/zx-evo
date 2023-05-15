#pragma once
#include "sysdefs.h"

#define line16 line8d
#define line16d line32

void rend_copy8(u8 *dst, unsigned pitch);
void rend_copy8_nf(u8 *dst, unsigned pitch);
void rend_copy8d(u8 *dst, unsigned pitch);
void rend_copy8d1(u8 *dst, unsigned pitch);
void rend_copy8d1_nf(u8 *dst, unsigned pitch);
void rend_copy8d_nf(u8 *dst, unsigned pitch);
void rend_copy8t(u8 *dst, unsigned pitch);
void rend_copy8t_nf(u8 *dst, unsigned pitch);
void rend_copy8q(u8 *dst, unsigned pitch);
void rend_copy8q_nf(u8 *dst, unsigned pitch);

void rend_copy16(u8 *dst, unsigned pitch);
void rend_copy16d(u8 *dst, unsigned pitch);
void rend_copy16d1(u8 *dst, unsigned pitch);
void rend_copy16_nf(u8 *dst, unsigned pitch);
void rend_copy16d1_nf(u8 *dst, unsigned pitch);
void rend_copy16d_nf(u8 *dst, unsigned pitch);
void rend_copy16t(u8 *dst, unsigned pitch);
void rend_copy16t_nf(u8 *dst, unsigned pitch);
void rend_copy16q(u8 *dst, unsigned pitch);
void rend_copy16q_nf(u8 *dst, unsigned pitch);

void rend_copy32(u8 *dst, unsigned pitch);
void rend_copy32d(u8 *dst, unsigned pitch);
void rend_copy32d1(u8 *dst, unsigned pitch);
void rend_copy32_nf(u8 *dst, unsigned pitch);
void rend_copy32d1_nf(u8 *dst, unsigned pitch);
void rend_copy32d_nf(u8 *dst, unsigned pitch);
void rend_copy32t(u8 *dst, unsigned pitch);
void rend_copy32t_nf(u8 *dst, unsigned pitch);
void rend_copy32q(u8 *dst, unsigned pitch);
void rend_copy32q_nf(u8 *dst, unsigned pitch);

void line8(u8 *dst, u8 *src, unsigned *tab);
void line8d(u8 *dst, u8 *src, unsigned *tab);
void line8t(u8 *dst, u8 *src, unsigned *tab);
void line8_nf(u8 *dst, u8 *src, unsigned *tab);
void line16_nf(u8 *dst, u8 *src, unsigned *tab);
void line32(u8 *dst, u8 *src, unsigned *tab);
void line32d(u8 *dst, u8 *src, unsigned *tab);
void line32_nf(u8 *dst, u8 *src, unsigned *tab);
