#pragma once

// small
void rend_atmframe8(unsigned char *dst, unsigned pitch);
void rend_atmframe16(unsigned char *dst, unsigned pitch);
void rend_atmframe32(unsigned char *dst, unsigned pitch);

// double
void rend_atmframe_8d1(unsigned char *dst, unsigned pitch);
void rend_atmframe_8d(unsigned char *dst, unsigned pitch);
void rend_atmframe_16d1(unsigned char *dst, unsigned pitch);
void rend_atmframe_16d(unsigned char *dst, unsigned pitch);
void rend_atmframe_32d1(unsigned char *dst, unsigned pitch);
void rend_atmframe_32d(unsigned char *dst, unsigned pitch);
