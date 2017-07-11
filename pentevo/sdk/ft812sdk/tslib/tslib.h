
#pragma once

void ts_wreg(u8 a, u8 v);
void ts_set_dma_saddr(u32 a);
void ts_set_dma_daddr(u32 a);
void ts_set_dma_saddr_p(u16 a, u8 p);
void ts_set_dma_daddr_p(u16 a, u8 p);
void ts_set_dma_size(u16 l, u16 n);
void ts_dma_wait();
