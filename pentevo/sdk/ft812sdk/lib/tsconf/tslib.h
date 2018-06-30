
#pragma once

void ts_wreg(u8, u8);
u8 ts_rreg(u8);
void ts_set_dma_saddr(u32);
void ts_set_dma_daddr(u32);
void ts_set_dma_saddr_p(u16, u8);
void ts_set_dma_daddr_p(u16, u8);
void ts_set_dma_size(u16, u16);
void ts_dma_start(u8);
void ts_dma_wait();
