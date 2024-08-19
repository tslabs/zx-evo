
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_slave_hd.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_log.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "helper.h"
#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "wifi.h"
#include "stats.h"
#include "spi_slave.h"

// Pin setting
#define GPIO_MOSI     13
#define GPIO_MISO     11
#define GPIO_SCLK     12
#define GPIO_CS       10

#define SLAVE_HOST    SPI2_HOST
#define DMA_CHAN      SPI_DMA_CH_AUTO

using namespace stats;

const char TAG[] = "spi_slave";

u8 *dma_buf;
u8 *curr_obj_addr;
size_t current_chunk_size;
size_t next_chunk_size;
u8 is_busy;

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
static IRAM_ATTR bool cb_cmd8(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
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
static IRAM_ATTR bool cb_tx_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  wr_reg8(ESP_REG_STATUS, ESP_ST_DATA_S2M);
  return true;
}

void set_ok_ready()
{
  is_busy = false;
  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
  stats::set_end();
  wr_reg32(ESP_EXEC_TIME, stats::get_time());
}

void set_status(u8 err)
{
  is_busy = false;
  wr_reg8(ESP_REG_STATUS, err);
  stats::set_end();
  wr_reg32(ESP_EXEC_TIME, stats::get_time());
}

// ------------- SPI device


void init_slave_hd(void)
{
  spi_bus_config_t bus_cfg = {};
  // memset(&bus_cfg, 0, sizeof(spi_bus_config_t));

  bus_cfg.mosi_io_num     = GPIO_MOSI;
  bus_cfg.miso_io_num     = GPIO_MISO;
  bus_cfg.sclk_io_num     = GPIO_SCLK;
  bus_cfg.quadwp_io_num   = -1;
  bus_cfg.quadhd_io_num   = -1;
  bus_cfg.max_transfer_sz = DMA_BUF_SIZE;
  bus_cfg.flags           = 0;
  bus_cfg.intr_flags      = 0;

  spi_slave_hd_slot_config_t slave_hd_cfg = {};
  // memset(&slave_hd_cfg, 0, sizeof(spi_slave_hd_slot_config_t));

  slave_hd_cfg.spics_io_num = GPIO_CS;
  slave_hd_cfg.flags        = 0;
  slave_hd_cfg.mode         = 0;
  slave_hd_cfg.command_bits = 8;
  slave_hd_cfg.address_bits = 8;
  slave_hd_cfg.dummy_bits   = 8;
  slave_hd_cfg.queue_size   = 1;
  slave_hd_cfg.dma_chan     = DMA_CHAN;
  slave_hd_cfg.cb_config    = (spi_slave_hd_callback_config_t)
  {
    .cb_buffer_tx      = cb_regs_read,        // Callback when master reads from shared buffer (after CMD2)
    .cb_buffer_rx      = cb_cmd,              // Callback when master writes to shared buffer (after CMD1)
    .cb_send_dma_ready = cb_tx_ready,         // Callback when TX data buffer is loaded to the hardware (after DMA transaction is pushed by sender task)
    .cb_sent           = cb_cmd8,             // Callback when data are sent (after CMD8)
    .cb_recv_dma_ready = cb_rx_ready,         // Callback when RX data buffer is loaded to the hardware
    .cb_recv           = cb_cmd7,             // Callback when data are received (after CMD7)
    .cb_cmd9           = cb_cmd9,             // Callback when CMD9 received
    .cb_cmdA           = cb_cmdA,             // Callback when CMDA received
    .arg               = NULL                 // Argument indicating this SPI Slave HD peripheral instance
  };

  spi_slave_hd_init(SLAVE_HOST, &bus_cfg, &slave_hd_cfg);
  
  set_status(ESP_ST_RESET);

  tx_queue = xQueueCreate(2, sizeof(int));
  rx_queue = xQueueCreate(2, sizeof(int));

  dma_buf = (u8*)heap_caps_malloc(DMA_BUF_SIZE, MALLOC_CAP_DMA);

  if (!dma_buf)
    ESP_LOGE(TAG, "Cannot allocate memory for SPI DMA buf!");
  else
  {
    xTaskCreatePinnedToCore(sender_task, "sender", 4096, NULL, 23, NULL, 0);
    xTaskCreatePinnedToCore(receiver_task, "receiver", 4096, NULL, 23, NULL, 0);
  }

  is_busy = false;
}

// ------------- Commands

void IRAM_ATTR command()
{
  u8 cmd = rd_reg8(ESP_REG_COMMAND);

  if (cmd)
  {
    // Clear command register
    wr_reg8(ESP_REG_COMMAND, 0);

    if (!is_busy)
    {
      stats::set_start();
      
      // The BUSY status is set by default, it must be updated in each command
      // set_ok_ready() for successful completion, or
      // set_status(ESP_ERR_xx) for an error.
      is_busy = true;
      set_status(ESP_ST_BUSY);

      switch (cmd)
      {
        case ESP_CMD_GET_INFO_STR:
        {
          const char *ptr = "";

          switch (rd_reg8(ESP_REG_STRING_TYPE))
          {
            case GET_INFO_COPYRIGHT:
              ptr = cp_string;
            break;

            case GET_INFO_BUILD:
              ptr = __DATE__ " " __TIME__;
            break;

            // case 0xEE:
              // ptr = "Easter egg placeholder";
            // break;
          }

          int size = strlen(ptr);
          wr_reg8(ESP_REG_STRING_SIZE, size);
          wr_regs(ESP_REG_STRING_DATA, ptr, size);
          set_ok_ready();
        }
        break;

        case ESP_CMD_GET_NETSTATE:
          wr_reg8(ESP_REG_NETSTATE, net.state);
          set_ok_ready();
        break;

        case ESP_CMD_GET_IP:
          wr_regs(ESP_REG_IP, &net.ip, sizeof(net.ip));
          set_ok_ready();
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
          set_ok_ready();

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
          set_ok_ready();
        }
        break;

        case ESP_CMD_AP_CONNECT:
          put_helper_isr(TASK_CONN);
        break;

        case ESP_CMD_WSCAN:
          put_helper_isr(TASK_WSCAN);
        break;

        case ESP_CMD_XM_INIT:
        case ESP_CMD_XM_PLAY:
        {
          int handle = rd_reg8(ESP_REG_OBJ_HANDLE);

          if (!check_handle(handle) || (mem_obj[handle].type != OBJ_TYPE_XM))
          {
            set_status(ESP_ERR_INV_XM_HANDLE);
            break;
          }

          XM_TASK t;
          t.handle = handle;

          switch (cmd)
          {
            case ESP_CMD_XM_INIT:   t.task = XM_TASK_INIT; break;
            case ESP_CMD_XM_PLAY:   t.task = XM_TASK_PLAY; break;
          }

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

          curr_obj_addr = &((u8*)mem_obj[handle].addr)[offs];
          current_chunk_size = size;
          next_chunk_size = min(mem_obj[handle].size - offs - size, DMA_BUF_SIZE);
          wr_reg32(ESP_REG_DATA_OFFSET, offs + size);

          (cmd == ESP_CMD_READ_OBJECT) ? put_txq_isr(DREQ_DATA) : put_rxq_isr(DREQ_DATA);
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

        case ESP_CMD_GET_RND:
          put_txq_isr(DREQ_RND);
        break;

        case ESP_CMD_RESET:
          REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);  // SoC reset
          while (1);

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
    next_chunk_size = 0;

    xQueueReceive(tx_queue, &req_data_type, portMAX_DELAY);

    _st.drq_data_start_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_start_min_us = min(_st.drq_data_start_min_us, _st.drq_data_start_last_us);
    _st.drq_data_start_max_us = max(_st.drq_data_start_max_us, _st.drq_data_start_last_us);

    slave_trans.data = dma_buf;
    size_t max_size = rd_reg32(ESP_REG_DATA_SIZE);
    slave_trans.len = prepare_tx_data(req_data_type, max_size);

    _st.drq_data_end_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_end_min_us = min(_st.drq_data_end_min_us, _st.drq_data_end_last_us);
    _st.drq_data_end_max_us = max(_st.drq_data_end_max_us, _st.drq_data_end_last_us);

    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &slave_trans, portMAX_DELAY);
    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &ret_trans, portMAX_DELAY);
    wr_reg32(ESP_REG_DATA_SIZE, next_chunk_size);
    set_ok_ready();

    // printf("Sent %d bytes\r\n", ret_trans->len);
  }
}

void IRAM_ATTR receiver_task(void *arg)
{
  spi_slave_hd_data_t slave_trans;
  spi_slave_hd_data_t *ret_trans;
  int req_data_type;

  while (1)
  {
    next_chunk_size = 0;

    xQueueReceive(rx_queue, &req_data_type, portMAX_DELAY);
    // printf("R_DMA request %d\r\n", req_data_type);

    slave_trans.data = dma_buf;
    slave_trans.len  = DMA_BUF_SIZE;

    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &slave_trans, portMAX_DELAY);
    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &ret_trans, portMAX_DELAY);
    int len = ret_trans->trans_len;

    // printf("Received %d bytes, %08lX\r\n", ret_trans->trans_len, esp_crc32_le(0, dma_buf, len));

    process_rx_data(req_data_type, len);
    wr_reg32(ESP_REG_DATA_SIZE, next_chunk_size);
    set_ok_ready();
  }
}

u32 IRAM_ATTR prepare_tx_data(u8 type, size_t size)
{
  u8 *data = dma_buf;

  switch (type)
  {
    case DREQ_WSCAN:
    {
      auto num = wf_get_ap_num();
      int ptr = 0;
      data[ptr++] = num;    // number of APs

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

      size = ptr;
    }
    break;

    case DREQ_DATA:
    {
      size = current_chunk_size;
      memcpy(dma_buf, curr_obj_addr, size);
    }
    break;

    case DREQ_RND:
    {
      u32 *p = (u32*)data;

      for (int i = 0; i < size / 4; i++)
      {
        u32 a = rand();
        p[i] = a ^ (a << 1);
      }
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
      size = min(size, current_chunk_size);
      memcpy(curr_obj_addr, dma_buf, size);
    }
    break;
  }
}
