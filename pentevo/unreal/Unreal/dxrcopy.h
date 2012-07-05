#pragma once

#define line16 line8d
#define line16d line32

void __fastcall rend_copy8(unsigned char *dst, unsigned pitch);
void __fastcall rend_copy8_nf(unsigned char *dst, unsigned pitch);
void rend_copy8d(unsigned char *dst, unsigned pitch);
void rend_copy8d1(unsigned char *dst, unsigned pitch);
void rend_copy8d1_nf(unsigned char *dst, unsigned pitch);
void rend_copy8d_nf(unsigned char *dst, unsigned pitch);
void rend_copy8t(unsigned char *dst, unsigned pitch);
void rend_copy8t_nf(unsigned char *dst, unsigned pitch);
void rend_copy8q(unsigned char *dst, unsigned pitch);
void rend_copy8q_nf(unsigned char *dst, unsigned pitch);

void rend_copy16(unsigned char *dst, unsigned pitch);
void rend_copy16d(unsigned char *dst, unsigned pitch);
void rend_copy16d1(unsigned char *dst, unsigned pitch);
void rend_copy16_nf(unsigned char *dst, unsigned pitch);
void rend_copy16d1_nf(unsigned char *dst, unsigned pitch);
void rend_copy16d_nf(unsigned char *dst, unsigned pitch);
void rend_copy16t(unsigned char *dst, unsigned pitch);
void rend_copy16t_nf(unsigned char *dst, unsigned pitch);
void rend_copy16q(unsigned char *dst, unsigned pitch);
void rend_copy16q_nf(unsigned char *dst, unsigned pitch);

void rend_copy32(unsigned char *dst, unsigned pitch);
void rend_copy32d(unsigned char *dst, unsigned pitch);
void rend_copy32d1(unsigned char *dst, unsigned pitch);
void rend_copy32_nf(unsigned char *dst, unsigned pitch);
void rend_copy32d1_nf(unsigned char *dst, unsigned pitch);
void rend_copy32d_nf(unsigned char *dst, unsigned pitch);
void rend_copy32t(unsigned char *dst, unsigned pitch);
void rend_copy32t_nf(unsigned char *dst, unsigned pitch);
void rend_copy32q(unsigned char *dst, unsigned pitch);
void rend_copy32q_nf(unsigned char *dst, unsigned pitch);

void line8(unsigned char *dst, unsigned char *src, unsigned *tab);
void line8d(unsigned char *dst, unsigned char *src, unsigned *tab);
void line8t(unsigned char *dst, unsigned char *src, unsigned *tab);
void line8_nf(unsigned char *dst, unsigned char *src, unsigned *tab);
void line16_nf(unsigned char *dst, unsigned char *src, unsigned *tab);
void line32(unsigned char *dst, unsigned char *src, unsigned *tab);
void line32d(unsigned char *dst, unsigned char *src, unsigned *tab);
void line32_nf(unsigned char *dst, unsigned char *src, unsigned *tab);
