#pragma once

extern CACHE_ALIGNED u32 vbuf[];


// small
void rend_ts_small(unsigned char *dst, unsigned pitch);

// double
void rend_ts(unsigned char *dst, unsigned pitch);
