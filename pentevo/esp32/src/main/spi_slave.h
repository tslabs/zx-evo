
#pragma once

#define DMA_BUF_SIZE  16384

enum
{
  DREQ_INFO,
  DREQ_WSCAN,
  DREQ_DATA,
  DREQ_RND,
  DREQ_TEST2,
  DREQ_TEST3,
};

u8 rd_reg8(u8 reg);
u32 rd_reg32(u8 reg);
void rd_regs(u8 reg, const void *data, int size);
void wr_reg8(u8 reg, u8 val);
void wr_reg32(u8 reg, u32 val);
void wr_regs(u8 reg, const void *data, int size);

void init_slave_hd(void);
void command();
void sender_task(void *arg);
void receiver_task(void *arg);

void put_rxq(int type);
void put_txq(int type);

void set_ok_ready();
void set_status(u8 err);
