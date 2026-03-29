
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
#define GPIO_MOSI     20
#define GPIO_MISO     22
#define GPIO_SCLK     21
#define GPIO_CS       23
#else
#define GPIO_MOSI     13
#define GPIO_MISO     11
#define GPIO_SCLK     12
#define GPIO_CS       10
#endif

#define FT_CS         1

#define SLAVE_HOST    SPI2_HOST
#define DMA_CHAN      SPI_DMA_CH_AUTO

enum spi_role_t
{
  SPI_ROLE_SLAVE_HD = 0,
  SPI_ROLE_MASTER   = 1
};

spi_role_t spi_role = SPI_ROLE_SLAVE_HD;

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

// ------------- Command functions

u8 IRAM_ATTR rd_reg8(u8 reg)
{
  u8 val;
  spi_slave_hd_read_buffer(SLAVE_HOST, reg, (u8*)&val, sizeof(val));
  return val;
}

u32 IRAM_ATTR rd_reg32(u8 reg)
{
  u32 val;
  spi_slave_hd_read_buffer(SLAVE_HOST, reg, (u8*)&val, sizeof(val));
  return val;
}

void IRAM_ATTR rd_regs(u8 reg, const void *data, int size)
{
  spi_slave_hd_read_buffer(SLAVE_HOST, reg, (u8*)data, size);
}

void IRAM_ATTR wr_reg8(u8 reg, u8 val)
{
  spi_slave_hd_write_buffer(SLAVE_HOST, reg, (u8*)&val, sizeof(val));
}

void IRAM_ATTR wr_reg32(u8 reg, u32 val)
{
  spi_slave_hd_write_buffer(SLAVE_HOST, reg, (u8*)&val, sizeof(val));
}

void IRAM_ATTR wr_regs(u8 reg, const void *data, int size)
{
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

// ------------- SPI device


void init_spi_configs()
{
  memset(&g_bus_cfg, 0, sizeof(g_bus_cfg));
  g_bus_cfg.sclk_io_num     = GPIO_SCLK;
  g_bus_cfg.mosi_io_num     = GPIO_MOSI;
  g_bus_cfg.miso_io_num     = GPIO_MISO;
  g_bus_cfg.data2_io_num    = -1;
  g_bus_cfg.data3_io_num    = -1;
  g_bus_cfg.quadwp_io_num   = -1;
  g_bus_cfg.quadhd_io_num   = -1;
  g_bus_cfg.data4_io_num    = -1;
  g_bus_cfg.data5_io_num    = -1;
  g_bus_cfg.data6_io_num    = -1;
  g_bus_cfg.data7_io_num    = -1;
  g_bus_cfg.max_transfer_sz = DMA_BUF_SIZE;
  g_bus_cfg.flags           = SPICOMMON_BUSFLAG_DUAL;
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

  ESP_ERROR_CHECK(spi_slave_hd_init(SLAVE_HOST, &g_bus_cfg, &g_slave_hd_cfg));

  set_status(ESP_ST_RESET);

  if (!tx_queue)
    tx_queue = xQueueCreate(2, sizeof(int));

  if (!rx_queue)
    rx_queue = xQueueCreate(2, sizeof(int));

  if (!dma_buf)
    dma_buf = (u8*)heap_caps_malloc(DMA_BUF_SIZE, MALLOC_CAP_DMA);

  if (!dma_buf)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for SPI DMA buf!");
    return;
  }

  if (!g_sender_task)
    xTaskCreatePinnedToCore(sender_task, "sender", 2048, NULL, 23, &g_sender_task, 0);

  if (!g_receiver_task)
    xTaskCreatePinnedToCore(receiver_task, "receiver", 4096, NULL, 23, &g_receiver_task, 0);

  seed = esp_timer_get_time();
  is_busy = false;
  spi_role = SPI_ROLE_SLAVE_HD;
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

  esp_err_t err = spi_slave_hd_disable(SLAVE_HOST);
  if (err != ESP_OK)
    goto fail_resume;

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

  g_master_data_lines = 1;
  spi_role = SPI_ROLE_MASTER;

  xSemaphoreGive(g_spi_mode_mtx);
  return ESP_OK;

fail_restore_slave:
  spi_slave_hd_init(SLAVE_HOST, &g_bus_cfg, &g_slave_hd_cfg);
  spi_role = SPI_ROLE_SLAVE_HD;

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

  if (g_sender_task)
    vTaskResume(g_sender_task);

  if (g_receiver_task)
    vTaskResume(g_receiver_task);

  g_master_data_lines = 1;
  spi_role = SPI_ROLE_SLAVE_HD;
  set_status(ESP_ST_READY);

  xSemaphoreGive(g_spi_mode_mtx);
  return ESP_OK;
}

esp_err_t spi_master_set_data_lines(u8 lines)
{
  if (spi_role != SPI_ROLE_MASTER || !g_master_dev)
    return ESP_ERR_INVALID_STATE;

  if (lines != 1 && lines != 2)
    return ESP_ERR_INVALID_ARG;

  g_master_data_lines = lines;
  return ESP_OK;
}

esp_err_t spi_master_xfer(void *tx_data, void *rx_data, size_t size)
{
  if (spi_role != SPI_ROLE_MASTER || !g_master_dev) return ESP_ERR_INVALID_STATE;
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

    memset(rx, 0, 4);

    t.base.flags = SPI_TRANS_VARIABLE_CMD
                 | SPI_TRANS_VARIABLE_ADDR
                 | SPI_TRANS_VARIABLE_DUMMY;
    t.command_bits = 8;
    t.address_bits = 16;
    t.dummy_bits = 8;
    t.base.cmd = tx[0];
    t.base.addr = ((u32)tx[1] << 8) | (u32)tx[2];
    t.base.rxlength = (size - 4) * 8;
    t.base.rx_buffer = rx + 4;

    return spi_device_polling_transmit(g_master_dev, &t.base);
  }

  if (g_master_data_lines != 2) return ESP_ERR_NOT_SUPPORTED;

  if (rx_data)
  {
    if (size < 4) return ESP_ERR_INVALID_ARG;

    u8 *rx = (u8*)rx_data;
    spi_transaction_ext_t t = {};

    memset(rx, 0, 4);

    t.base.flags = SPI_TRANS_MODE_DIO
                 | SPI_TRANS_MULTILINE_CMD
                 | SPI_TRANS_MULTILINE_ADDR
                 | SPI_TRANS_VARIABLE_CMD
                 | SPI_TRANS_VARIABLE_ADDR
                 | SPI_TRANS_VARIABLE_DUMMY;
    t.command_bits = 8;
    t.address_bits = 16;
    t.dummy_bits = 4;
    t.base.cmd = tx[0];
    t.base.addr = ((u32)tx[1] << 8) | (u32)tx[2];
    t.base.rxlength = (size - 4) * 8;
    t.base.rx_buffer = rx + 4;

    return spi_device_polling_transmit(g_master_dev, &t.base);
  }

  if (size < 3) return ESP_ERR_INVALID_ARG;

  spi_transaction_ext_t t = {};
  t.base.flags = SPI_TRANS_MODE_DIO
               | SPI_TRANS_MULTILINE_CMD
               | SPI_TRANS_MULTILINE_ADDR
               | SPI_TRANS_VARIABLE_CMD
               | SPI_TRANS_VARIABLE_ADDR;
  t.command_bits = 8;
  t.address_bits = 16;
  t.base.cmd = tx[0];
  t.base.addr = ((u32)tx[1] << 8) | (u32)tx[2];
  t.base.length = (size - 3) * 8;
  t.base.tx_buffer = tx + 3;

  return spi_device_polling_transmit(g_master_dev, &t.base);
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

        case ESP_CMD_WSCAN:
          put_helper_isr(TASK_WSCAN);
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

        case ESP_CMD_HTTP_ABORT:
          set_status(ESP_ST_READY); // ??? Check for graceful HTTP exit
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
  }

  return size;
}

void IRAM_ATTR process_rx_data(u8 type, size_t size)
{
  switch (type)
  {
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
