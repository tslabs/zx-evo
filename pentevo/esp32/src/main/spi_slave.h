#pragma once

#include "freertos/FreeRTOS.h"

#define DMA_BUF_SIZE  16384

enum
{
  DREQ_WSCAN,
  DREQ_DATA,
  DREQ_ZIP,
  DREQ_RND,
  DREQ_URL,
  DREQ_STREAM,
};

u8 rd_reg8(u8 reg);
u32 rd_reg32(u8 reg);
void rd_regs(u8 reg, const void *data, int size);
void wr_reg8(u8 reg, u8 val);
void wr_reg32(u8 reg, u32 val);
void wr_regs(u8 reg, const void *data, int size);

void init_spi_configs();
void init_slave_hd();
esp_err_t spi_switch_to_master();
esp_err_t spi_switch_to_slave();
void spi_master_set_clock_hz(u32 hz);
u32 spi_master_get_actual_freq_hz();
esp_err_t spi_master_set_data_lines(u8 lines);
esp_err_t spi_master_write_buf(u8 cmd, u16 addr, const void *tx_data, size_t size);
esp_err_t spi_master_read_buf(u8 cmd, u16 addr, void *rx_data, size_t size);
esp_err_t spi_master_xfer(void *tx_data, void *rx_data, size_t size);
bool spi_master_bg_is_busy();
bool spi_master_bg_is_done();
esp_err_t spi_master_bg_get_result();
esp_err_t spi_master_bg_wait_done(TickType_t ticks_to_wait);
esp_err_t spi_master_write_buf_bg(u8 cmd, u16 addr, const void *tx_data, size_t size);
esp_err_t spi_master_read_buf_bg(u8 cmd, u16 addr, void *rx_data, size_t size);
void spi_master_bg_task(void *arg);

void command();
void sender_task(void *arg);
void receiver_task(void *arg);

void put_rxq(int type);
void put_txq(int type);
u32 prepare_tx_data(u8 type, size_t size);
void process_rx_data(u8 type, size_t size);
void set_status(u8 err);
u8* get_dma_buf(void);
void spi_slave_dma_send_direct(size_t len);
void wait_status(u8 status);
void wait_not_status(u8 status);
