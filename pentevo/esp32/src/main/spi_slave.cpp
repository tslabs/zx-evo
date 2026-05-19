
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/spi_slave_hd.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_crc.h"
#include "esp_memory_utils.h"
#include "esp_async_memcpy.h"
#include "miniz.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "helper.h"
#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "stats.h"
#include "spi_slave.h"

#ifdef CONFIG_ESP32_WIFI_ENABLED
#include "wifi.h"
#include "http_client.h"
#endif

// Pin setting
#if defined(CONFIG_IDF_TARGET_ESP32P4)
#define GPIO_CS       23
#define GPIO_SCLK     21
#define GPIO_MOSI     20
#define GPIO_MISO     22
#else
#define GPIO_CS       10
#define GPIO_SCLK     12
#define GPIO_MOSI     13
#define GPIO_MISO     11
#define GPIO_FT_IO2   14
#define GPIO_FT_IO3   9
#endif

#define FT_CS         1

#define SLAVE_HOST    SPI2_HOST
#define DMA_CHAN      SPI_DMA_CH_AUTO

enum spi_role_t
{
  SPI_ROLE_SLAVE_HD = 0,
  SPI_ROLE_MASTER   = 1,
  SPI_ROLE_SWITCHING = 2
};

volatile spi_role_t spi_role = SPI_ROLE_SLAVE_HD;

spi_bus_config_t g_bus_cfg = {};
spi_slave_hd_slot_config_t g_slave_hd_cfg = {};
spi_device_interface_config_t g_master_dev_cfg = {};

spi_device_handle_t g_master_dev = NULL;
u8 g_master_data_lines = 1;
SemaphoreHandle_t g_spi_mode_mtx = NULL;

TaskHandle_t g_sender_task = NULL;
TaskHandle_t g_receiver_task = NULL;

using namespace stats;

const char TAG[] = "spi_slave";

// DMA transmitters
u8 *dma_buf;

u8 is_busy;

uint32_t seed = 12345678;

QueueHandle_t tx_queue;
QueueHandle_t rx_queue;

enum spi_master_bg_op_t
{
  SPI_MASTER_BG_OP_NONE = 0,
  SPI_MASTER_BG_OP_WRITE,
  SPI_MASTER_BG_OP_READ,
};

typedef struct
{
  spi_master_bg_op_t op;
  u8 cmd;
  u16 addr;
  const u8 *tx_data;
  u8 *rx_data;
  size_t size;
} spi_master_bg_req_t;

QueueHandle_t g_master_bg_queue = NULL;
SemaphoreHandle_t g_master_bg_done_sem = NULL;
SemaphoreHandle_t g_master_bg_copy_sem = NULL;
TaskHandle_t g_master_bg_task = NULL;
async_memcpy_handle_t g_async_memcpy = NULL;
volatile bool g_master_bg_busy = false;
volatile bool g_master_bg_done = true;
esp_err_t g_master_bg_result = ESP_OK;

#define SPI_SLAVE_REG_SHADOW_SIZE 64

u8 g_slave_reg_shadow[SPI_SLAVE_REG_SHADOW_SIZE] = {};

// ------------- Command functions

int IRAM_ATTR spi_slave_reg_clip_size(u8 reg, int size)
{
  if (size <= 0) return 0;
  if (reg >= SPI_SLAVE_REG_SHADOW_SIZE) return 0;

  int max_size = SPI_SLAVE_REG_SHADOW_SIZE - reg;
  if (size > max_size) return max_size;
  return size;
}

void IRAM_ATTR spi_slave_reg_shadow_read(u8 reg, void *data, int size)
{
  size = spi_slave_reg_clip_size(reg, size);
  if (!size) return;

  memcpy(data, &g_slave_reg_shadow[reg], size);
}

void IRAM_ATTR spi_slave_reg_shadow_write(u8 reg, const void *data, int size)
{
  size = spi_slave_reg_clip_size(reg, size);
  if (!size) return;

  memcpy(&g_slave_reg_shadow[reg], data, size);
}

void spi_slave_reg_shadow_load_hw()
{
  spi_slave_hd_read_buffer(SLAVE_HOST, 0, g_slave_reg_shadow, SPI_SLAVE_REG_SHADOW_SIZE);
}

void spi_slave_reg_shadow_flush_hw()
{
  spi_slave_hd_write_buffer(SLAVE_HOST, 0, g_slave_reg_shadow, SPI_SLAVE_REG_SHADOW_SIZE);
}

u8 IRAM_ATTR rd_reg8(u8 reg)
{
  u8 val = 0;
  rd_regs(reg, &val, sizeof(val));
  return val;
}

u32 IRAM_ATTR rd_reg32(u8 reg)
{
  u32 val = 0;
  rd_regs(reg, &val, sizeof(val));
  return val;
}

void IRAM_ATTR rd_regs(u8 reg, const void *data, int size)
{
  size = spi_slave_reg_clip_size(reg, size);
  if (!size) return;

  if (spi_role == SPI_ROLE_SLAVE_HD)
  {
    spi_slave_hd_read_buffer(SLAVE_HOST, reg, (u8*)data, size);
    spi_slave_reg_shadow_write(reg, data, size);
    return;
  }

  spi_slave_reg_shadow_read(reg, (void*)data, size);
}

void IRAM_ATTR wr_reg8(u8 reg, u8 val)
{
  wr_regs(reg, &val, sizeof(val));
}

void IRAM_ATTR wr_reg32(u8 reg, u32 val)
{
  wr_regs(reg, &val, sizeof(val));
}

void IRAM_ATTR wr_regs(u8 reg, const void *data, int size)
{
  size = spi_slave_reg_clip_size(reg, size);
  if (!size) return;

  spi_slave_reg_shadow_write(reg, data, size);

  if (spi_role == SPI_ROLE_SLAVE_HD)
    spi_slave_hd_write_buffer(SLAVE_HOST, reg, (u8*)data, size);
}

void IRAM_ATTR put_txq(int type)
{
  xQueueSend(tx_queue, &type, 0);
}

void IRAM_ATTR put_txq_isr(int type)
{
  _st.drq_data_t = esp_timer_get_time();
  xQueueSendFromISR(tx_queue, &type, 0);
  portYIELD_FROM_ISR();
}

void IRAM_ATTR put_rxq(int type)
{
  xQueueSend(rx_queue, &type, 0);
}

void IRAM_ATTR put_rxq_isr(int type)
{
  xQueueSendFromISR(rx_queue, &type, 0);
  portYIELD_FROM_ISR();
}

// ------------- Interrupt callbacks

// Regs have been read
bool IRAM_ATTR cb_regs_read(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

bool IRAM_ATTR cb_rx_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  wr_reg8(ESP_REG_STATUS, ESP_ST_DATA_M2S);
  return true;
}

bool IRAM_ATTR cb_cmd7(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

// DMA sending done
bool IRAM_ATTR cb_cmd8(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

bool IRAM_ATTR cb_cmd9(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

bool IRAM_ATTR cb_cmdA(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

// Called after Master has finished writing shared regs
void command();

bool IRAM_ATTR cb_cmd(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  command();
  return true;
}

// DMA to send is ready (the transaction is pushed by sender task)
bool IRAM_ATTR cb_tx_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  wr_reg8(ESP_REG_STATUS, ESP_ST_DATA_S2M);
  return true;
}

void IRAM_ATTR set_status(u8 err)
{
  is_busy = false;
  wr_reg8(ESP_REG_STATUS, err);
  stats::set_end();
  wr_reg32(ESP_EXEC_TIME, stats::get_time());
}

u8* get_dma_buf(void)
{
  return dma_buf;
}

void spi_slave_dma_send_direct(size_t len)
{
  wr_reg32(ESP_REG_DATA_SIZE, len);
  put_txq(DREQ_STREAM);
}

void IRAM_ATTR wait_status(u8 status)
{
  while (rd_reg8(ESP_REG_STATUS) != status)
    vTaskDelay(1);
}

void IRAM_ATTR wait_not_status(u8 status)
{
  while (rd_reg8(ESP_REG_STATUS) == status)
    vTaskDelay(1);
}

// ------------- SPI device

bool spi_master_buf_is_dma_ok(const void *buf, size_t size)
{
  if (!buf) return false;
  if (!esp_ptr_word_aligned(buf)) return false;
  if (size & 3) return false;
  if (esp_ptr_dma_capable(buf)) return true;
  if (esp_ptr_dma_ext_capable(buf)) return true;
  return false;
}

esp_err_t spi_master_prep_write_trans(spi_transaction_ext_t *t, u8 cmd, u16 addr, const void *tx_data, size_t size)
{
  if (!t) return ESP_ERR_INVALID_ARG;

  memset(t, 0, sizeof(*t));

  if (g_master_data_lines == 1)
  {
    t->base.flags = SPI_TRANS_VARIABLE_CMD
                   | SPI_TRANS_VARIABLE_ADDR;
  }
  else if (g_master_data_lines == 2 || g_master_data_lines == 4)
  {
    u32 mode_flag = (g_master_data_lines == 4) ? SPI_TRANS_MODE_QIO : SPI_TRANS_MODE_DIO;

    t->base.flags = mode_flag
                   | SPI_TRANS_MULTILINE_CMD
                   | SPI_TRANS_MULTILINE_ADDR
                   | SPI_TRANS_VARIABLE_CMD
                   | SPI_TRANS_VARIABLE_ADDR;
  }
  else
    return ESP_ERR_NOT_SUPPORTED;

  t->command_bits = 8;
  t->address_bits = 16;
  t->base.cmd = cmd;
  t->base.addr = addr;
  t->base.length = size * 8;
  t->base.tx_buffer = tx_data;

  return ESP_OK;
}

esp_err_t spi_master_prep_read_trans(spi_transaction_ext_t *t, u8 cmd, u16 addr, void *rx_data, size_t size)
{
  if (!t) return ESP_ERR_INVALID_ARG;

  memset(t, 0, sizeof(*t));

  if (g_master_data_lines == 1)
  {
    t->base.flags = SPI_TRANS_VARIABLE_CMD
                   | SPI_TRANS_VARIABLE_ADDR
                   | SPI_TRANS_VARIABLE_DUMMY;
    t->dummy_bits = 8;
  }
  else if (g_master_data_lines == 2 || g_master_data_lines == 4)
  {
    u32 mode_flag = (g_master_data_lines == 4) ? SPI_TRANS_MODE_QIO : SPI_TRANS_MODE_DIO;

    t->base.flags = mode_flag
                   | SPI_TRANS_MULTILINE_CMD
                   | SPI_TRANS_MULTILINE_ADDR
                   | SPI_TRANS_VARIABLE_CMD
                   | SPI_TRANS_VARIABLE_ADDR
                   | SPI_TRANS_VARIABLE_DUMMY;
    t->dummy_bits = 4;
  }
  else
    return ESP_ERR_NOT_SUPPORTED;

  t->command_bits = 8;
  t->address_bits = 16;
  t->base.cmd = cmd;
  t->base.addr = addr;
  t->base.rxlength = size * 8;
  t->base.rx_buffer = rx_data;

  return ESP_OK;
}

esp_err_t spi_master_queue_write_buf(u8 cmd, u16 addr, const void *tx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (!tx_data && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  spi_transaction_ext_t t = {};
  esp_err_t err = spi_master_prep_write_trans(&t, cmd, addr, tx_data, size);
  if (err != ESP_OK) return err;

  err = spi_device_queue_trans(g_master_dev, &t.base, portMAX_DELAY);
  if (err != ESP_OK) return err;

  spi_transaction_t *ret = NULL;
  return spi_device_get_trans_result(g_master_dev, &ret, portMAX_DELAY);
}

esp_err_t spi_master_queue_read_buf(u8 cmd, u16 addr, void *rx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (!rx_data && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  spi_transaction_ext_t t = {};
  esp_err_t err = spi_master_prep_read_trans(&t, cmd, addr, rx_data, size);
  if (err != ESP_OK) return err;

  err = spi_device_queue_trans(g_master_dev, &t.base, portMAX_DELAY);
  if (err != ESP_OK) return err;

  spi_transaction_t *ret = NULL;
  return spi_device_get_trans_result(g_master_dev, &ret, portMAX_DELAY);
}

esp_err_t spi_master_xfer(void *tx_data, void *rx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (spi_master_bg_is_busy()) return ESP_ERR_INVALID_STATE;
  if (!tx_data && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  u8 *tx = (u8*)tx_data;

  if (g_master_data_lines == 1)
  {
    if (!rx_data)
    {
      spi_transaction_t t = {};
      t.length = size * 8;
      t.tx_buffer = tx_data;
      return spi_device_polling_transmit(g_master_dev, &t);
    }

    if (size < 4) return ESP_ERR_INVALID_ARG;

    u8 *rx = (u8*)rx_data;
    spi_transaction_ext_t t = {};
    u16 addr = ((u32)tx[1] << 8) | (u32)tx[2];

    memset(rx, 0, 4);

    esp_err_t err = spi_master_prep_read_trans(&t, tx[0], addr, rx + 4, size - 4);
    if (err != ESP_OK) return err;

    return spi_device_polling_transmit(g_master_dev, &t.base);
  }

  if (g_master_data_lines != 2 && g_master_data_lines != 4)
    return ESP_ERR_NOT_SUPPORTED;


  if (rx_data)
  {
    if (size < 4) return ESP_ERR_INVALID_ARG;

    u8 *rx = (u8*)rx_data;
    spi_transaction_ext_t t = {};
    u16 addr = ((u32)tx[1] << 8) | (u32)tx[2];

    memset(rx, 0, 4);

    esp_err_t err = spi_master_prep_read_trans(&t, tx[0], addr, rx + 4, size - 4);
    if (err != ESP_OK) return err;

    return spi_device_polling_transmit(g_master_dev, &t.base);
  }

  if (size < 3) return ESP_ERR_INVALID_ARG;

  spi_transaction_ext_t t = {};
  u16 addr = ((u32)tx[1] << 8) | (u32)tx[2];
  esp_err_t err = spi_master_prep_write_trans(&t, tx[0], addr, tx + 3, size - 3);
  if (err != ESP_OK) return err;

  return spi_device_polling_transmit(g_master_dev, &t.base);
}

bool IRAM_ATTR spi_master_async_memcpy_cb(async_memcpy_handle_t mcp_hdl, async_memcpy_event_t *event, void *cb_args)
{
  BaseType_t awoken = pdFALSE;
  xSemaphoreGiveFromISR((SemaphoreHandle_t)cb_args, &awoken);
  return awoken == pdTRUE;
}

esp_err_t spi_master_copy_async(void *dst, const void *src, size_t size)
{
  if (!size) return ESP_OK;
  if (!dst || !src) return ESP_ERR_INVALID_ARG;

  if (g_async_memcpy && g_master_bg_copy_sem)
  {
    while (xSemaphoreTake(g_master_bg_copy_sem, 0) == pdTRUE);

    esp_err_t err = esp_async_memcpy(g_async_memcpy, dst, (void*)src, size, spi_master_async_memcpy_cb, g_master_bg_copy_sem);
    if (err == ESP_OK)
    {
      if (xSemaphoreTake(g_master_bg_copy_sem, portMAX_DELAY) == pdTRUE)
        return ESP_OK;

      return ESP_FAIL;
    }
  }

  memcpy(dst, src, size);
  return ESP_OK;
}

bool spi_master_bg_is_busy()
{
  if (g_master_bg_busy) return true;
  if (g_master_bg_queue && uxQueueMessagesWaiting(g_master_bg_queue)) return true;
  return false;
}

bool spi_master_bg_is_done()
{
  return g_master_bg_done;
}

esp_err_t spi_master_bg_wait_done(TickType_t ticks_to_wait)
{
  if (!g_master_bg_done_sem) return ESP_ERR_INVALID_STATE;
  if (g_master_bg_done) return g_master_bg_result;
  if (xSemaphoreTake(g_master_bg_done_sem, ticks_to_wait) != pdTRUE) return ESP_ERR_TIMEOUT;
  xSemaphoreGive(g_master_bg_done_sem);
  return g_master_bg_result;
}

esp_err_t spi_master_bg_get_result()
{
  return g_master_bg_result;
}

esp_err_t spi_master_bg_submit(spi_master_bg_op_t op, u8 cmd, u16 addr, const void *tx_data, void *rx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (!g_master_bg_queue || !g_master_bg_done_sem || !dma_buf) return ESP_ERR_INVALID_STATE;
  if (spi_master_bg_is_busy()) return ESP_ERR_INVALID_STATE;
  if ((op == SPI_MASTER_BG_OP_WRITE) && !tx_data && size) return ESP_ERR_INVALID_ARG;
  if ((op == SPI_MASTER_BG_OP_READ) && !rx_data && size) return ESP_ERR_INVALID_ARG;

  spi_master_bg_req_t req = {};
  req.op = op;
  req.cmd = cmd;
  req.addr = addr;
  req.tx_data = (const u8*)tx_data;
  req.rx_data = (u8*)rx_data;
  req.size = size;

  while (xSemaphoreTake(g_master_bg_done_sem, 0) == pdTRUE);

  g_master_bg_done = false;
  g_master_bg_result = ESP_OK;

  if (!size)
  {
    g_master_bg_done = true;
    xSemaphoreGive(g_master_bg_done_sem);
    return ESP_OK;
  }

  if (xQueueSend(g_master_bg_queue, &req, 0) != pdTRUE)
  {
    g_master_bg_done = true;
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}

void spi_master_bg_task(void *arg)
{
  spi_master_bg_req_t req;

  while (1)
  {
    xQueueReceive(g_master_bg_queue, &req, portMAX_DELAY);

    g_master_bg_busy = true;
    g_master_bg_done = false;
    g_master_bg_result = ESP_OK;
    size_t offs = 0;

    while (offs < req.size)
    {
      size_t chunk = min((size_t)DMA_BUF_SIZE, req.size - offs);
      u32 addr24 = (((u32)req.cmd << 16) | (u32)req.addr) + offs;
      u8 chunk_cmd = (u8)(addr24 >> 16);
      u16 chunk_addr = (u16)addr24;

      esp_err_t err = ESP_OK;

      if (req.op == SPI_MASTER_BG_OP_WRITE)
      {
        const u8 *tx_ptr = &req.tx_data[offs];

        if (!spi_master_buf_is_dma_ok(tx_ptr, chunk))
        {
          err = spi_master_copy_async(dma_buf, tx_ptr, chunk);
          if (err != ESP_OK)
          {
            g_master_bg_result = err;
            break;
          }

          tx_ptr = dma_buf;
        }

        err = spi_master_queue_write_buf(chunk_cmd, chunk_addr, tx_ptr, chunk);
      }
      else if (req.op == SPI_MASTER_BG_OP_READ)
      {
        u8 *rx_ptr = &req.rx_data[offs];
        bool copy_back = false;

        if (!spi_master_buf_is_dma_ok(rx_ptr, chunk))
        {
          rx_ptr = dma_buf;
          copy_back = true;
        }

        err = spi_master_queue_read_buf(chunk_cmd, chunk_addr, rx_ptr, chunk);

        if ((err == ESP_OK) && copy_back)
          err = spi_master_copy_async(&req.rx_data[offs], dma_buf, chunk);
      }
      else
        err = ESP_ERR_INVALID_STATE;

      if (err != ESP_OK)
      {
        g_master_bg_result = err;
        break;
      }

      offs += chunk;
    }

    g_master_bg_busy = false;
    g_master_bg_done = true;
    xSemaphoreGive(g_master_bg_done_sem);
  }
}

esp_err_t spi_master_write_buf_bg(u8 cmd, u16 addr, const void *tx_data, size_t size)
{
  return spi_master_bg_submit(SPI_MASTER_BG_OP_WRITE, cmd, addr, tx_data, NULL, size);
}

esp_err_t spi_master_read_buf_bg(u8 cmd, u16 addr, void *rx_data, size_t size)
{
  return spi_master_bg_submit(SPI_MASTER_BG_OP_READ, cmd, addr, NULL, rx_data, size);
}

esp_err_t spi_master_write_buf(u8 cmd, u16 addr, const void *tx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (spi_master_bg_is_busy()) return ESP_ERR_INVALID_STATE;
  if (!tx_data && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;
  if (!dma_buf) return ESP_ERR_INVALID_STATE;
  if (size > DMA_BUF_SIZE) return ESP_ERR_INVALID_ARG;

  const void *data = tx_data;

  if (!spi_master_buf_is_dma_ok(tx_data, size))
  {
    memcpy(dma_buf, tx_data, size);
    data = dma_buf;
  }

  spi_transaction_ext_t t = {};
  esp_err_t err = spi_master_prep_write_trans(&t, cmd, addr, data, size);
  if (err != ESP_OK) return err;

  return spi_device_polling_transmit(g_master_dev, &t.base);
}

esp_err_t spi_master_read_buf(u8 cmd, u16 addr, void *rx_data, size_t size)
{
  if (!g_master_dev) return ESP_ERR_INVALID_STATE;
  if (spi_master_bg_is_busy()) return ESP_ERR_INVALID_STATE;
  if (!rx_data && size) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;
  if (!dma_buf) return ESP_ERR_INVALID_STATE;
  if (size > DMA_BUF_SIZE) return ESP_ERR_INVALID_ARG;

  void *data = rx_data;
  bool copy_back = false;
  spi_transaction_ext_t t = {};

  if (!spi_master_buf_is_dma_ok(rx_data, size))
  {
    data = dma_buf;
    copy_back = true;
  }

  esp_err_t err = spi_master_prep_read_trans(&t, cmd, addr, data, size);
  if (err != ESP_OK) return err;

  err = spi_device_polling_transmit(g_master_dev, &t.base);
  if (err != ESP_OK) return err;

  if (copy_back)
    memcpy(rx_data, data, size);

  return ESP_OK;
}

void init_spi_configs()
{
  memset(&g_bus_cfg, 0, sizeof(g_bus_cfg));
  g_bus_cfg.sclk_io_num     = GPIO_SCLK;
  g_bus_cfg.mosi_io_num     = GPIO_MOSI;
  g_bus_cfg.miso_io_num     = GPIO_MISO;
  g_bus_cfg.data2_io_num    = GPIO_FT_IO2;
  g_bus_cfg.data3_io_num    = GPIO_FT_IO3;
  g_bus_cfg.quadwp_io_num   = GPIO_FT_IO2;
  g_bus_cfg.quadhd_io_num   = GPIO_FT_IO3;
  g_bus_cfg.data4_io_num    = -1;
  g_bus_cfg.data5_io_num    = -1;
  g_bus_cfg.data6_io_num    = -1;
  g_bus_cfg.data7_io_num    = -1;
  g_bus_cfg.max_transfer_sz = DMA_BUF_SIZE;
  g_bus_cfg.flags           = SPICOMMON_BUSFLAG_DUAL | SPICOMMON_BUSFLAG_QUAD;
  g_bus_cfg.intr_flags      = 0;

  memset(&g_slave_hd_cfg, 0, sizeof(g_slave_hd_cfg));
  g_slave_hd_cfg.spics_io_num = GPIO_CS;
  g_slave_hd_cfg.flags        = 0;
  g_slave_hd_cfg.mode         = 0;
  g_slave_hd_cfg.command_bits = 8;
  g_slave_hd_cfg.address_bits = 8;
  g_slave_hd_cfg.dummy_bits   = 8;
  g_slave_hd_cfg.queue_size   = 1;
  g_slave_hd_cfg.dma_chan     = DMA_CHAN;
  g_slave_hd_cfg.cb_config    = (spi_slave_hd_callback_config_t)
  {
    .cb_buffer_tx      = cb_regs_read,
    .cb_buffer_rx      = cb_cmd,
    .cb_send_dma_ready = cb_tx_ready,
    .cb_sent           = cb_cmd8,
    .cb_recv_dma_ready = cb_rx_ready,
    .cb_recv           = cb_cmd7,
    .cb_cmd9           = cb_cmd9,
    .cb_cmdA           = cb_cmdA,
    .arg               = NULL
  };

  memset(&g_master_dev_cfg, 0, sizeof(g_master_dev_cfg));
  g_master_dev_cfg.command_bits     = 0;
  g_master_dev_cfg.address_bits     = 0;
  g_master_dev_cfg.dummy_bits       = 0;
  g_master_dev_cfg.mode             = 0;
  g_master_dev_cfg.clock_source     = SPI_CLK_SRC_DEFAULT;
  g_master_dev_cfg.clock_speed_hz   = 30000000U;
  g_master_dev_cfg.spics_io_num     = FT_CS;
  g_master_dev_cfg.queue_size       = 1;
  g_master_dev_cfg.flags            = SPI_DEVICE_NO_DUMMY | SPI_DEVICE_HALFDUPLEX;
  g_master_dev_cfg.pre_cb           = NULL;
  g_master_dev_cfg.post_cb          = NULL;
}

void init_slave_hd()
{
  if (!g_spi_mode_mtx)
    g_spi_mode_mtx = xSemaphoreCreateMutex();

  init_spi_configs();
  log_sram_used(__FILE_NAME__ ": init_spi_configs");

  ESP_ERROR_CHECK(spi_slave_hd_init(SLAVE_HOST, &g_bus_cfg, &g_slave_hd_cfg));
  log_sram_used(__FILE_NAME__ ": spi_slave_hd_init");

  memset(g_slave_reg_shadow, 0, sizeof(g_slave_reg_shadow));
  spi_role = SPI_ROLE_SLAVE_HD;
  set_status(ESP_ST_RESET);

  if (!tx_queue)
    tx_queue = xQueueCreate(2, sizeof(int));

  if (!rx_queue)
    rx_queue = xQueueCreate(2, sizeof(int));

  if (!dma_buf)
  {
    dma_buf = (u8*)heap_caps_malloc(DMA_BUF_SIZE, MALLOC_CAP_DMA);
    log_sram_used(__FILE_NAME__ ": dma_buf");
  }

  if (!dma_buf)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for SPI DMA buf!");
    return;
  }

  if (!g_sender_task)
  {
    xTaskCreatePinnedToCoreWithCaps(sender_task, "sender", 2048, NULL, SLAVE_TASK_PRIO, &g_sender_task, 0, task_ram_type_critical);
    log_sram_used(__FILE_NAME__ ": TaskCreate sender");
  }

  if (!g_receiver_task)
  {
    xTaskCreatePinnedToCoreWithCaps(receiver_task, "receiver", 4096, NULL, SLAVE_TASK_PRIO, &g_receiver_task, 0, task_ram_type_critical);
    log_sram_used(__FILE_NAME__ ": TaskCreate receiver");
  }

  if (!g_master_bg_queue)
    g_master_bg_queue = xQueueCreate(1, sizeof(spi_master_bg_req_t));

  if (!g_master_bg_done_sem)
  {
    g_master_bg_done_sem = xSemaphoreCreateBinary();
    if (g_master_bg_done_sem)
      xSemaphoreGive(g_master_bg_done_sem);
  }

  if (!g_master_bg_copy_sem)
    g_master_bg_copy_sem = xSemaphoreCreateBinary();

  if (!g_async_memcpy)
  {
    async_memcpy_config_t cfg = ASYNC_MEMCPY_DEFAULT_CONFIG();
    cfg.backlog = 1;
    cfg.dma_burst_size = 16;
    esp_err_t err = esp_async_memcpy_install(&cfg, &g_async_memcpy);
    if (err != ESP_OK)
      ESP_LOGW(TAG, "esp_async_memcpy_install failed: %s", esp_err_to_name(err));
  }

  if (!g_master_bg_task)
  {
    xTaskCreatePinnedToCoreWithCaps(spi_master_bg_task, "spi_bg", 4096, NULL, SPI_BG_TASK_PRIO, &g_master_bg_task, 0, task_ram_type_critical);
    log_sram_used(__FILE_NAME__ ": TaskCreate spi_bg");
  }

  seed = esp_timer_get_time();
  is_busy = false;
  spi_role = SPI_ROLE_SLAVE_HD;

  log_sram_used(__FILE_NAME__ ": init_slave_hd end");
}

esp_err_t spi_switch_to_master()
{
  if (!g_spi_mode_mtx)
    return ESP_ERR_INVALID_STATE;

  xSemaphoreTake(g_spi_mode_mtx, portMAX_DELAY);

  if (spi_role == SPI_ROLE_MASTER)
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return ESP_OK;
  }

  if (is_busy)
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return ESP_ERR_INVALID_STATE;
  }

  if (uxQueueMessagesWaiting(tx_queue) || uxQueueMessagesWaiting(rx_queue))
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return ESP_ERR_INVALID_STATE;
  }

  if (g_sender_task)
    vTaskSuspend(g_sender_task);

  if (g_receiver_task)
    vTaskSuspend(g_receiver_task);

  spi_slave_reg_shadow_load_hw();
  spi_role = SPI_ROLE_SWITCHING;

  esp_err_t err = spi_slave_hd_disable(SLAVE_HOST);
  if (err != ESP_OK)
  {
    spi_role = SPI_ROLE_SLAVE_HD;
    goto fail_resume;
  }

  err = spi_slave_hd_deinit(SLAVE_HOST);
  if (err != ESP_OK)
    goto fail_resume;

  err = spi_bus_initialize(SLAVE_HOST, &g_bus_cfg, DMA_CHAN);
  if (err != ESP_OK)
    goto fail_restore_slave;

  err = spi_bus_add_device(SLAVE_HOST, &g_master_dev_cfg, &g_master_dev);
  if (err != ESP_OK)
  {
    spi_bus_free(SLAVE_HOST);
    goto fail_restore_slave;
  }

  gpio_set_pull_mode((gpio_num_t)GPIO_FT_IO2, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)GPIO_FT_IO3, GPIO_PULLUP_ONLY);

  g_master_data_lines = 1;
  spi_role = SPI_ROLE_MASTER;

  xSemaphoreGive(g_spi_mode_mtx);
  return ESP_OK;

fail_restore_slave:
  if (spi_slave_hd_init(SLAVE_HOST, &g_bus_cfg, &g_slave_hd_cfg) == ESP_OK)
  {
    spi_slave_reg_shadow_flush_hw();
    spi_role = SPI_ROLE_SLAVE_HD;
  }

fail_resume:
  if (g_sender_task)
    vTaskResume(g_sender_task);

  if (g_receiver_task)
    vTaskResume(g_receiver_task);

  xSemaphoreGive(g_spi_mode_mtx);
  return err;
}

esp_err_t spi_switch_to_slave()
{
  if (!g_spi_mode_mtx)
    return ESP_ERR_INVALID_STATE;

  xSemaphoreTake(g_spi_mode_mtx, portMAX_DELAY);

  if (spi_role == SPI_ROLE_SLAVE_HD)
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return ESP_OK;
  }

  if (spi_master_bg_is_busy())
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return ESP_ERR_INVALID_STATE;
  }

  esp_err_t err = ESP_OK;

  if (g_master_dev)
  {
    err = spi_bus_remove_device(g_master_dev);
    if (err != ESP_OK)
    {
      xSemaphoreGive(g_spi_mode_mtx);
      return err;
    }
    g_master_dev = NULL;
  }

  err = spi_bus_free(SLAVE_HOST);
  if (err != ESP_OK)
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return err;
  }

  err = spi_slave_hd_init(SLAVE_HOST, &g_bus_cfg, &g_slave_hd_cfg);
  if (err != ESP_OK)
  {
    xSemaphoreGive(g_spi_mode_mtx);
    return err;
  }

  spi_slave_reg_shadow_flush_hw();

  spi_role = SPI_ROLE_SLAVE_HD;

  if (g_sender_task)
    vTaskResume(g_sender_task);

  if (g_receiver_task)
    vTaskResume(g_receiver_task);

  g_master_data_lines = 1;
  set_status(ESP_ST_READY);

  xSemaphoreGive(g_spi_mode_mtx);
  return ESP_OK;
}

void spi_master_set_clock_hz(u32 hz)
{
  g_master_dev_cfg.clock_speed_hz = hz;
}

u32 spi_master_get_actual_freq_hz()
{
  int freq_khz = 0;

  if (!g_master_dev)
    return 0;

  if (spi_device_get_actual_freq(g_master_dev, &freq_khz) != ESP_OK)
    return 0;

  return (u32)freq_khz * 1000UL;
}

esp_err_t spi_master_set_data_lines(u8 lines)
{
  if (!g_master_dev)
    return ESP_ERR_INVALID_STATE;

  g_master_data_lines = lines;
  return ESP_OK;
}


// ------------- Commands

void IRAM_ATTR command()
{
  u8 cmd = rd_reg8(ESP_REG_COMMAND);

  if (cmd)
  {
    // Clear command register
    wr_reg8(ESP_REG_COMMAND, 0);

    if (is_busy)
    {
      if (cmd == ESP_CMD_BREAK)
      {} // +++ break ESP_CMD_STREAM_UNZIP
    }
    else
    {
      stats::set_start();

      // The BUSY status is set by default, it must be updated in each command
      // set_status(ESP_ST_READY) for successful completion, or
      // set_status(ESP_ERR_xx) for an error.
      is_busy = true;
      set_status(ESP_ST_BUSY);

      switch (cmd)
      {
        case ESP_CMD_GET_INFO_STR:
        {
          const char *ptr = "";
          int size = 0;

          switch (rd_reg8(ESP_REG_STRING_TYPE))
          {
            case GET_INFO_COPYRIGHT:
              ptr = CP_STRING;
              size = sizeof(CP_STRING) - 1;
            break;

            case GET_INFO_BUILD:
              ptr = __DATE__ " " __TIME__;
              size = sizeof(__DATE__ " " __TIME__) - 1;
            break;

            case GET_INFO_IDF:
              // ptr = esp_get_idf_version();
              ptr = IDF_VER;
              size = sizeof(IDF_VER) - 1;
            break;

            // case 0xEE:
              // ptr = "Easter egg placeholder";
              // size = sizeof("Easter egg placeholder") - 1;
            // break;
          }

          wr_reg8(ESP_REG_STRING_SIZE, size);
          wr_regs(ESP_REG_STRING_DATA, ptr, size);
          set_status(ESP_ST_READY);
        }
        break;

        case ESP_CMD_GET_NETSTATE:
          wr_reg8(ESP_REG_NETSTATE, net.state);
          set_status(ESP_ST_READY);
        break;

        case ESP_CMD_GET_IP:
          wr_regs(ESP_REG_IP, &net.ip, sizeof(net.ip));
          set_status(ESP_ST_READY);
        break;

	    case ESP_CMD_SET_URL:
        {
          size_t size = rd_reg32(ESP_REG_DATA_SIZE);

          if (!size || (size > DMA_BUF_SIZE))
          {
            set_status(ESP_ERR_INV_SIZE);
            break;
          }

          if (!net.url)
          {
            set_status(ESP_ERR_INV_STATE);
            break;
          }

          put_rxq_isr(DREQ_URL);
        }
        break;

        case ESP_CMD_SET_AP_NAME:
        {
          int len = rd_reg8(ESP_REG_STRING_SIZE);

          if (!len || (len > 32))
          {
            net.ssid[0] = 0;
            set_status(ESP_ERR_INV_STR_LEN);
            break;
          }

          rd_regs(ESP_REG_STRING_DATA, net.ssid, len);
          net.ssid[len] = 0;
          set_status(ESP_ST_READY);

        }
        break;

        case ESP_CMD_SET_AP_PWD:
        {
          int len = rd_reg8(ESP_REG_STRING_SIZE);
          net.pwd[0] = 0;

          if (len > 61)
          {
            set_status(ESP_ERR_INV_STR_LEN);
            break;
          }

          rd_regs(ESP_REG_STRING_DATA, net.pwd, len);
          net.pwd[len] = 0;
          set_status(ESP_ST_READY);
        }
        break;

        case ESP_CMD_AP_CONNECT:
          put_helper_isr(TASK_CONN);
        break;

        case ESP_CMD_AP_DISCONNECT:
          put_helper_isr(TASK_DISCONN);
        break;

        case ESP_CMD_WSCAN:
          put_helper_isr(TASK_WSCAN);
        break;

        case ESP_CMD_XM_STREAM_LOAD:
        {
          size_t size = rd_reg32(ESP_REG_DATA_SIZE);

          if (!size)
          {
            set_status(ESP_ERR_INV_SIZE);
            break;
          }

          esp_err_t err = xm_host_stream_start(size);
          if (err != ESP_OK)
          {
            if (err == ESP_ERR_NO_MEM)
              set_status(ESP_ERR_OUT_OF_MEMORY);
            else if (err == ESP_ERR_INVALID_SIZE)
              set_status(ESP_ERR_INV_SIZE);
            else
              set_status(ESP_ERR_INV_STATE);
            break;
          }
        }
        break;

        case ESP_CMD_XM_INIT:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);

          if (!check_handle(handle) || !((mem_obj[handle].type == OBJ_TYPE_XM) || (mem_obj[handle].type == OBJ_TYPE_XMC)))
          {
            set_status(ESP_ERR_INV_XM_HANDLE);
            break;
          }

          XM_TASK t;
          t.handle = handle;
          t.task = XM_TASK_INIT;
          xQueueSendFromISR(xm_queue, &t, 0);
          portYIELD_FROM_ISR();
        }
        break;

        case ESP_CMD_XM_PLAY:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);

          if (!check_handle(handle) || (mem_obj[handle].type != OBJ_TYPE_XMC))
          {
            set_status(ESP_ERR_INV_XM_HANDLE);
            break;
          }

          XM_TASK t;
          t.handle = handle;
          t.task = XM_TASK_PLAY;
          xQueueSendFromISR(xm_queue, &t, 0);
          portYIELD_FROM_ISR();
        }
        break;

        case ESP_CMD_XM_STOP:
        {
          XM_TASK t;
          t.task = XM_TASK_STOP;
          xQueueSendFromISR(xm_queue, &t, 0);
          portYIELD_FROM_ISR();
        }
        break;

        case ESP_CMD_LOAD_ELF:
          put_helper_isr(TASK_LOAD_ELF);
        break;

        case ESP_CMD_LOAD_ELF_OPT:
          put_helper_isr(TASK_LOAD_ELF_OPT);
        break;

        case ESP_CMD_RUN_FUNC0:
          put_helper_isr(TASK_RUN_FUNC0);
        break;

        case ESP_CMD_RUN_FUNC1:
          put_helper_isr(TASK_RUN_FUNC1);
        break;

        case ESP_CMD_RUN_FUNC2:
          put_helper_isr(TASK_RUN_FUNC2);
        break;

        case ESP_CMD_RUN_FUNC3:
          put_helper_isr(TASK_RUN_FUNC3);
        break;

        case ESP_CMD_MAKE_OBJECT:
          put_helper_isr(TASK_MAKE_OBJ);
        break;

        case ESP_CMD_WRITE_OBJECT:
        case ESP_CMD_READ_OBJECT:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);

          if (!check_handle(handle))
          {
            set_status(ESP_ERR_INV_HANDLE);
            break;
          }

          size_t offs = rd_reg32(ESP_REG_DATA_OFFSET);
          size_t size = rd_reg32(ESP_REG_DATA_SIZE);

          if (!size || (size > DMA_BUF_SIZE) || ((offs + size) > mem_obj[handle].size))
          {
            set_status(ESP_ERR_INV_SIZE);
            break;
          }

          (cmd == ESP_CMD_READ_OBJECT) ? put_txq_isr(DREQ_DATA) : put_rxq_isr(DREQ_DATA);
        }
        break;

        case ESP_CMD_STREAM_UNZIP:
        {
          size_t size = rd_reg32(ESP_REG_DATA_SIZE);

          if ((size > DMA_BUF_SIZE))
            set_status(ESP_ERR_INV_SIZE);

          else if (!size)   // Reset stream unzip
            put_helper_isr(TASK_RESET_STREAM_UNZIP);

          else
            put_rxq_isr(DREQ_ZIP);
        }
        break;

        case ESP_CMD_DELETE_OBJECT:
          put_helper_isr(TASK_DEL_OBJ);
        break;

        case ESP_CMD_KILL_OBJECTS:
          put_helper_isr(TASK_KILL_OBJ);
        break;

        case ESP_CMD_DEHST:
          put_helper_isr(TASK_DEHST);
        break;

        case ESP_CMD_UNZIP:
          put_helper_isr(TASK_UNZIP);
        break;

        case ESP_CMD_HTTP_GET:
          put_helper_isr(TASK_HTTP_GET);
        break;

        case ESP_CMD_HTTPS_GET:
          put_helper_isr(TASK_HTTPS_GET);
        break;

        case ESP_CMD_GOPHER_GET:
          put_helper_isr(TASK_GOPHER_GET);
        break;

        case ESP_CMD_HTTP_STREAM_START:
          put_helper_isr(TASK_HTTP_STREAM_START);
        break;

        case ESP_CMD_HTTPS_STREAM_START:
          put_helper_isr(TASK_HTTPS_STREAM_START);
        break;

        case ESP_CMD_GOPHER_STREAM_START:
          put_helper_isr(TASK_GOPHER_STREAM_START);
        break;

        case ESP_CMD_STREAM_READ:
          put_helper_isr(TASK_STREAM_READ);
        break;

        case ESP_CMD_STREAM_CLOSE:
          put_helper_isr(TASK_STREAM_CLOSE);
        break;

        case ESP_CMD_GET_RND:
          put_txq_isr(DREQ_RND);
        break;

        case ESP_CMD_REBOOT:
          put_helper_isr(TASK_REBOOT);
        break;

        case ESP_CMD_RESET:
          put_helper_isr(TASK_RESET);
        break;

        default:
          set_status(ESP_ERR_INV_COMMAND);
      }
    }
  }
}

// ------------- DMA send / receive

void IRAM_ATTR sender_task(void *arg)
{
  spi_slave_hd_data_t slave_trans;
  spi_slave_hd_data_t *ret_trans;
  int req_data_type;

  while (1)
  {
    xQueueReceive(tx_queue, &req_data_type, portMAX_DELAY);

    _st.drq_data_start_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_start_min_us = min(_st.drq_data_start_min_us, _st.drq_data_start_last_us);
    _st.drq_data_start_max_us = max(_st.drq_data_start_max_us, _st.drq_data_start_last_us);

    slave_trans.data = dma_buf;
    size_t max_size = rd_reg32(ESP_REG_DATA_SIZE);
    slave_trans.len = prepare_tx_data(req_data_type, max_size);
    wr_reg32(ESP_REG_DATA_SIZE, slave_trans.len);

    _st.drq_data_end_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_end_min_us = min(_st.drq_data_end_min_us, _st.drq_data_end_last_us);
    _st.drq_data_end_max_us = max(_st.drq_data_end_max_us, _st.drq_data_end_last_us);

    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &slave_trans, portMAX_DELAY);
    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &ret_trans, portMAX_DELAY);
    set_status(ESP_ST_READY);
  }
}

void IRAM_ATTR receiver_task(void *arg)
{
  spi_slave_hd_data_t slave_trans;
  spi_slave_hd_data_t *ret_trans;
  int req_data_type;

  while (1)
  {
    xQueueReceive(rx_queue, &req_data_type, portMAX_DELAY);
    // printf("R_DMA request, type %d\r\n", req_data_type);

    slave_trans.data = dma_buf;
    slave_trans.len  = DMA_BUF_SIZE;
    size_t size = rd_reg32(ESP_REG_DATA_SIZE);

    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &slave_trans, portMAX_DELAY);
    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &ret_trans, portMAX_DELAY);

#ifdef VERBOSE
    printf("Received %d, expected %d, CRC %08lX\r\n", ret_trans->trans_len, size, esp_crc32_le(0, dma_buf, size));

#if 0
    int sz = size;
    int of = 0;

    while (sz)
    {
      int s = min(1024, sz);
      printf("%08X: %08lX\r\n", of, esp_crc32_le(0, &dma_buf[of], s));
      of += s;
      sz -= s;
    }
#endif
#endif

    process_rx_data(req_data_type, size);
  }
}

// For some data types <size> is input parameter - number of bytes requested.
// For other it is ignored and returned in result of operation.
u32 IRAM_ATTR prepare_tx_data(u8 type, size_t size)
{
  u8 *data = dma_buf;

  switch (type)
  {
    case DREQ_WSCAN:
    {
      int ptr = 0;

#ifdef CONFIG_ESP32_WIFI_ENABLED
      auto num = wf_get_ap_num();
      data[ptr++] = num;    // number of APs

      // printf("AP num %d\r\n", num);

      for (int i = 0; i < num; i++)
      {
        u8 auth;
        i8 rssi;
        u8 chan;
        u8 *ssid;

        wf_get_ap(i, auth, rssi, chan, ssid);
        int len = strlen((char*)ssid);

        data[ptr++] = auth;               // Auth type
        data[ptr++] = rssi;               // RSSI
        data[ptr++] = chan;               // Channel
        data[ptr++] = len;                // SSID length
        memcpy(&data[ptr], ssid, len);    // SSID
        ptr += len;
      }
#endif

      size = ptr;
      // printf("Data size %d\r\n", size);
    }
    break;

    case DREQ_DATA:
    {
      int handle = rd_reg8(ESP_REG_OBJ_HANDLE);
      u32 offset = rd_reg32(ESP_REG_DATA_OFFSET);
      u8 *addr = &((u8*)mem_obj[handle].addr)[offset];
      memcpy(dma_buf, addr, size);
      wr_reg32(ESP_REG_DATA_OFFSET, offset + size);
    }
    break;

    case DREQ_RND:
    {
      u32 *p = (u32*)data;

      uint32_t x = seed;

      for (size_t i = 0; i < size / 4; i++)
      {
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        p[i] = x;
      }

      seed = x;
    }
    break;

    case DREQ_STREAM:
      // Data already in dma_buf, just return size
    break;
  }

  return size;
}

void IRAM_ATTR process_rx_data(u8 type, size_t size)
{
  switch (type)
  {
    case DREQ_URL:
    {
      if (!net.url)
      {
        set_status(ESP_ERR_INV_STATE);
        break;
      }

      if (size > 1023) size = 1023;
      memcpy(net.url, dma_buf, size);
      net.url[size] = 0;  // Ensure null terminator
      set_status(ESP_ST_READY);
    }
    break;

    case DREQ_DATA:
    {
      int handle = rd_reg8(ESP_REG_OBJ_HANDLE);
      u32 offset = rd_reg32(ESP_REG_DATA_OFFSET);
      u8 *addr = &((u8*)mem_obj[handle].addr)[offset];
      memcpy(addr, dma_buf, size);
      wr_reg32(ESP_REG_DATA_OFFSET, offset + size);
      set_status(ESP_ST_READY);
    }
    break;

    case DREQ_XM_STREAM:
      xm_host_stream_process_rx_data(dma_buf, size);
    break;

    case DREQ_ZIP:
    {
      enum
      {
        ZIP_ST_INITIAL,
        ZIP_ST_NEXT
      };

      static int zip_state = ZIP_ST_INITIAL;
      static int zip_handle = -1;
      int inpos;
      static u8 *outbuf;
      static int outpos;
      static u32 uncomp_size;

      if (!size)   // Reset stream depacker
      {
#ifdef VERBOSE
        printf("Inflate stream reset, ZIP_ST %d\r\n", zip_state);
#endif

        if (zip_state == ZIP_ST_NEXT)
        {
          delete_obj(zip_handle);
          // if (decomp) free(decomp);
          zip_state = ZIP_ST_INITIAL;
        }

        set_status(ESP_ST_READY);
        break;
      }

      if (zip_state == ZIP_ST_INITIAL)
      {
#ifdef VERBOSE
        // printf("ZIP_ST_INITIAL\r\n");
#endif

        uncomp_size = *(u32*)dma_buf;

#ifdef VERBOSE
        printf("Uncompressed size %ld\r\n", uncomp_size);
#endif

        zip_handle = make_obj(uncomp_size, rd_reg8(ESP_REG_OBJ_TYPE));
        if (zip_handle < 0) break;  // error status is set in make_obj()

        tinfl_init(decomp);

        inpos = sizeof(u32);   // skip uncomp size
        outbuf = (u8*)mem_obj[zip_handle].addr;
        outpos = 0;

#ifdef VERBOSE
        printf("Inflate init success\r\n");
#endif
      }

      // zip_state == ZIP_ST_NEXT
      else
      {
#ifdef VERBOSE
        // printf("ZIP_ST_NEXT\r\n");
#endif

        inpos = 0;
      }

      size_t outbytes = uncomp_size - outpos;
      size_t inbytes = size - inpos;

      // int in = stream.avail_in;
      // int out = stream.total_out;

      // printf(".next_in   %08X\r\n", (unsigned int)stream.next_in);
      // printf(".avail_in  %d\r\n", stream.avail_in);
      // printf(".total_in  %ld\r\n", stream.total_in);
      // printf(".next_out  %08X\r\n", (unsigned int)stream.next_out);
      // printf(".avail_out %d\r\n", stream.avail_out);
      // printf(".total_out %ld\r\n", stream.total_out);

      tinfl_status decomp_status = tinfl_decompress(decomp, &dma_buf[inpos], &inbytes, outbuf, &outbuf[outpos], &outbytes, TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF | TINFL_FLAG_HAS_MORE_INPUT);

      outpos += outbytes;

#ifdef VERBOSE
      printf("Inflate status = %d, inbytes = %d, outbytes = %d, total_out = %d\r\n", decomp_status, inbytes, outbytes, outpos);
#endif

      if (decomp_status == TINFL_STATUS_DONE)
      {
        // free(decomp); decomp = NULL;

#ifdef VERBOSE
        printf("Inflate success\r\n");
#endif

        zip_state = ZIP_ST_INITIAL;
        wr_reg8(ESP_REG_OBJ_HANDLE, zip_handle);
        set_status(ESP_ST_READY);
        break;
      }

      else if (decomp_status > TINFL_STATUS_DONE)
      {
        zip_state = ZIP_ST_NEXT;
        set_status(ESP_ST_READY);
        break;
      }

      else
      {
#ifdef VERBOSE
        ESP_LOGE(TAG, "Inflate error!");
#endif

        // free(decomp); decomp = NULL;
        delete_obj(zip_handle);
        zip_state = ZIP_ST_INITIAL;
        set_status(ESP_ERR_INV_ZIP);
        break;
      }
    }
    break;
  }
}
