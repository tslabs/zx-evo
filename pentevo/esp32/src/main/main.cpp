
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/spi_slave_hd.h"
#include "driver/gpio.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "types.h"
#include "spi_slave.h"
#include "console.h"
#include "ft812.h"
#include "wifi.h"
#include "cobra.h"
#include "tunnel.h"
#include "xm_cpp.h"
#include "stats.h"

// Pin setting
#define GPIO_MOSI     13
#define GPIO_MISO     11
#define GPIO_SCLK     12
#define GPIO_CS       10

#define SLAVE_HOST    SPI2_HOST
#define DMA_CHAN      SPI_DMA_CH_AUTO
#define QUEUE_SIZE    2

using namespace stats;

const char TAG[] = "zf32_main";
const char cp_string[] = "ESP32-S3 SPI WiFi Module, ver.0.2, (c)2024, TS-Labs";

enum
{
  DRQ_INFO,
  DRQ_WSCAN,
  DRQ_RND,
  DRQ_TEST2,
  DRQ_TEST3,
};

enum
{
  TASK_WSCAN,
  TASK_CONN,
};

QueueHandle_t drqueue;
TaskHandle_t sender_task_h = NULL;

u8 error;

#pragma pack(1)
struct
{
  bool is_init = false;
  bool is_busy = false;
  u8 state;
  QueueHandle_t queue;

  u8 ssid[33];
  u8 pwd[62];

  struct
  {
    u8 ip[4];
    u8 own_ip[4];
    u8 mask[4];
    u8 gate[4];
  } ip;
} net;
#pragma pack()

#define DMA_BUF_SIZE 8192
u8 *send_buf;
u8 *recv_buf[QUEUE_SIZE];

void IRAM_ATTR splash(int n)
{
  while (n--)
  {
    // gpio_set_level((gpio_num_t)GPIO_TEST1, 1); gpio_set_level((gpio_num_t)GPIO_TEST1, 0); // !!!
  }
}

// ------------- Command helpers

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

void IRAM_ATTR put_drq(int type)
{
  xQueueSend(drqueue, &type, 0);
}

void IRAM_ATTR put_drq_isr(int type)
{
  // gpio_set_level((gpio_num_t)GPIO_TEST1, 1); gpio_set_level((gpio_num_t)GPIO_TEST1, 0); // !!!
  _st.drq_data_t = esp_timer_get_time();
  xQueueSendFromISR(drqueue, &type, 0);
  portYIELD_FROM_ISR();
}

// ------------- Interrupt callbacks

// Regs have been read
bool IRAM_ATTR cb_regs_read(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  return true;
}

bool IRAM_ATTR cb_recv_dma_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
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

// ------------- Net helper task

void net_task(void *arg)
{
  while (1)
  {
    int task;

    if (xQueueReceive(net.queue, &task, portMAX_DELAY))
    {
      switch (task)
      {
        case TASK_WSCAN:
        {
          wf_scan();
          net.is_busy = false;
          put_drq(DRQ_WSCAN);
        }
        break;

        case TASK_CONN:
        {
          net.state = NETWORK_OPENING;
          auto rc = wifi_connect((const char*)net.ssid, (const char*)net.pwd, 10000);
          net.is_busy = false;
          net.state = rc ? NETWORK_OPEN : NETWORK_CLOSED;

          if (rc)
          {
            error = ESP_ERR_NONE;
            wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
            get_ip(net.ip.own_ip, net.ip.mask, net.ip.gate);
          }
          else
          {
            error = ESP_ERR_AP_NOT_CONN;
            wr_reg8(ESP_REG_ERROR, error);
            wr_reg8(ESP_REG_STATUS, ESP_ST_ERROR);
          }
        }
        break;

        default:
          net.is_busy = false;
      }
    }
  }
}

// ------------- SPI device

void IRAM_ATTR command()
{
  u8 zero = 0;
  u8 cmd = rd_reg8(ESP_REG_COMMAND);
  wr_reg8(ESP_REG_COMMAND, zero);

  if (cmd)
  {
    wr_reg8(ESP_REG_STATUS, ESP_ST_BUSY);

    switch (cmd)
    {
      case ESP_CMD_DATA_END:
        *(u32*)0x60024044 = 1 << 5;   // trigger CMD8 interrupt (CMD8 does not trigger it)
        wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
      break;

      case ESP_CMD_GET_INFO:
        wr_regs(ESP_REG_DATA, cp_string, sizeof(cp_string) - 1);
        wr_reg8(ESP_REG_DATA_SIZE, sizeof(cp_string) - 1);
        error = ESP_ERR_NONE;
        wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
      break;

      case ESP_CMD_GET_ERROR:
        wr_reg8(ESP_REG_ERROR, error);
        error = ESP_ERR_NONE;
        wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
      break;

      case ESP_CMD_GET_NETSTATE:
        wr_reg8(ESP_REG_NETSTATE, net.state);
        error = ESP_ERR_NONE;
        wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
      break;

      case ESP_CMD_GET_IP:
        wr_regs(ESP_REG_IP, &net.ip, sizeof(net.ip));
        error = ESP_ERR_NONE;
        wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
      break;

      case ESP_CMD_SET_AP_NAME:
      {
        int len = rd_reg8(ESP_REG_DATA_SIZE);

        if (!len || (len > 32))
        {
          net.ssid[0] = 0;
          error = ESP_ERR_WRONG_PARAMETER;
          wr_reg8(ESP_REG_ERROR, error);
          wr_reg8(ESP_REG_STATUS, ESP_ST_ERROR);
        }
        else
        {
          rd_regs(ESP_REG_DATA, net.ssid, len);
          net.ssid[len] = 0;
          error = ESP_ERR_NONE;
          wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
        }
      }
      break;

      case ESP_CMD_SET_AP_PWD:
      {
        int len = rd_reg8(ESP_REG_DATA_SIZE);
        net.pwd[0] = 0;

        if (len > 61)
        {
          error = ESP_ERR_WRONG_PARAMETER;
          wr_reg8(ESP_REG_ERROR, error);
          wr_reg8(ESP_REG_STATUS, ESP_ST_ERROR);
        }
        else
        {
          rd_regs(ESP_REG_DATA, net.pwd, len);
          net.pwd[len] = 0;
          error = ESP_ERR_NONE;
          wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
        }
      }
      break;

      case ESP_CMD_AP_CONNECT:
        if (!net.is_busy)
        {
          net.is_busy = true;
          int task = TASK_CONN;
          xQueueSendFromISR(net.queue, &task, 0);
        }
      break;

      case ESP_CMD_WSCAN:
        if (!net.is_busy)
        {
          net.is_busy = true;
          int task = TASK_WSCAN;
          xQueueSendFromISR(net.queue, &task, 0);
        }
      break;

      case ESP_CMD_XM_INIT:
      {
        XM_TASK t;
        t.id = XM_TASK_INIT;
        // t.arg = (void*)xm_module;
        xQueueSendFromISR(xm_queue, &t, 0);
      }
      break;

      case ESP_CMD_GET_RND:
        put_drq_isr(DRQ_RND);
      break;

      case ESP_CMD_TEST2:
        put_drq_isr(DRQ_TEST2);
      break;

      case ESP_CMD_TEST3:
        put_drq_isr(DRQ_TEST3);
      break;

      case ESP_CMD_RESET:
        REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);  // SoC reset
        while (1);

      default:
        wr_reg8(ESP_REG_STATUS, ESP_ST_ERROR);
        wr_reg8(ESP_REG_ERROR, ESP_ERR_INVALID_COMMAND);
    }
  }
}

u32 IRAM_ATTR prepare_tx_data(u8 *data, u8 type, u32 max_size)
{
  int size = 0;

  switch (type)
  {
    case DRQ_WSCAN:
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

    case DRQ_RND:
    {
      u32 *p = (u32*)data;
      size = 6144;

      for (int i = 0; i < size / 4; i++)
      {
        u32 a = rand();
        p[i] = a ^ (a << 1);
      }
    }
    break;

    case DRQ_TEST2:
      size = tunnel(data);
    break;

    case DRQ_TEST3:
      size = cobra(data);
    break;
  }

  return size;
}

void IRAM_ATTR sender_task(void *arg)
{
  spi_slave_hd_data_t slave_trans;          //Transaction descriptor
  spi_slave_hd_data_t *ret_trans;           //Pointer to the descriptor of return result
  int req_data_type;

  while (1)
  {
    xQueueReceive(drqueue, &req_data_type, portMAX_DELAY);
    // gpio_set_level((gpio_num_t)GPIO_TEST2, 1); gpio_set_level((gpio_num_t)GPIO_TEST2, 0); // !!!
    _st.drq_data_start_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_start_min_us = min(_st.drq_data_start_min_us, _st.drq_data_start_last_us);
    _st.drq_data_start_max_us = max(_st.drq_data_start_max_us, _st.drq_data_start_last_us);

    slave_trans.data = send_buf;
    slave_trans.len = prepare_tx_data(send_buf, req_data_type, DMA_BUF_SIZE);
    wr_reg32(ESP_REG_DATA_SIZE, slave_trans.len);
    _st.drq_data_end_last_us = esp_timer_get_time() - _st.drq_data_t;
    _st.drq_data_end_min_us = min(_st.drq_data_end_min_us, _st.drq_data_end_last_us);
    _st.drq_data_end_max_us = max(_st.drq_data_end_max_us, _st.drq_data_end_last_us);
    
    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &slave_trans, portMAX_DELAY);
    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &ret_trans, portMAX_DELAY);
  }
}

void IRAM_ATTR receiver_task(void *arg)
{
  spi_slave_hd_data_t *ret_trans;
  u32 recv_buf_size = DMA_BUF_SIZE;
  spi_slave_hd_data_t slave_trans[QUEUE_SIZE];

  /**
   * - For SPI Slave, you can use this way (making use of the internal queue) to pre-load transactions to driver. Thus if
   *   Master's speed is slower than Slave, Slave won't need to wait until Master finishes.
   */
  u32 descriptor_id = 0;

  for (int i = 0; i < QUEUE_SIZE; i++)
  {
    slave_trans[descriptor_id].data = recv_buf[descriptor_id];
    slave_trans[descriptor_id].len  = recv_buf_size;
    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &slave_trans[descriptor_id], portMAX_DELAY);
    descriptor_id = (descriptor_id + 1) % QUEUE_SIZE;
  }

  while (1)
  {
    /**
     * Get the RX transaction result
     *
     * The actual used (by master) buffer size could be derived by ``ret_trans->trans_len``:
     * For example, when Master sends 4bytes, whereas slave prepares 512 bytes buffer. When master finish sending its
     * 4 bytes, it will send CMD7, which will force stopping the transaction. Slave will get the actual received length
     * from `ret_trans->trans_len`` member (here 4 bytes). The ``ret_trans`` will exactly point to the transaction descriptor
     * passed to the driver before (here ``slave_trans``). If the master sends longer than slave recv buffer,
     * slave will lose the extra bits.
     */

    spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &ret_trans, portMAX_DELAY);

    //Process the received data in your own code. Here we just print it out.
    memset(ret_trans->data, 0x0, recv_buf_size);

    /**
     * Prepared data for new transaction
     */
    slave_trans[descriptor_id].data = recv_buf[descriptor_id];
    slave_trans[descriptor_id].len  = recv_buf_size;

    //Start new transaction
    spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_RX, &slave_trans[descriptor_id], portMAX_DELAY);
    descriptor_id = (descriptor_id + 1) % QUEUE_SIZE;   //descriptor_id will be: 0, 1, 2, ..., QUEUE_SIZE, 0, 1, ....
  }
}

static void init_slave_hd(void)
{
  spi_bus_config_t bus_cfg;
  memset(&bus_cfg, 0, sizeof(spi_bus_config_t));
  bus_cfg.mosi_io_num     = GPIO_MOSI;
  bus_cfg.miso_io_num     = GPIO_MISO;
  bus_cfg.sclk_io_num     = GPIO_SCLK;
  bus_cfg.quadwp_io_num   = -1;
  bus_cfg.quadhd_io_num   = -1;
  bus_cfg.max_transfer_sz = DMA_BUF_SIZE;
  bus_cfg.flags           = 0;
  bus_cfg.intr_flags      = 0;

  spi_slave_hd_slot_config_t slave_hd_cfg = {};
  memset(&slave_hd_cfg, 0, sizeof(spi_slave_hd_slot_config_t));
  slave_hd_cfg.spics_io_num = GPIO_CS;
  slave_hd_cfg.flags        = 0;
  slave_hd_cfg.mode         = 0;
  slave_hd_cfg.command_bits = 8;
  slave_hd_cfg.address_bits = 8;
  slave_hd_cfg.dummy_bits   = 8;
  slave_hd_cfg.queue_size   = QUEUE_SIZE;
  slave_hd_cfg.dma_chan     = DMA_CHAN;
  slave_hd_cfg.cb_config    = (spi_slave_hd_callback_config_t)
  {
    .cb_buffer_tx      = cb_regs_read,        // Callback when master reads from shared buffer (after CMD2)
    .cb_buffer_rx      = cb_cmd,              // Callback when master writes to shared buffer (after CMD1)
    .cb_send_dma_ready = cb_tx_ready,         // Callback when TX data buffer is loaded to the hardware (after DMA transaction is pushed by sender task)
    .cb_sent           = cb_cmd8,             // Callback when data are sent (after CMD8)
    .cb_recv_dma_ready = cb_recv_dma_ready,   // Callback when RX data buffer is loaded to the hardware
    .cb_recv           = cb_cmd7,             // Callback when data are received (after CMD7)
    .cb_cmd9           = cb_cmd9,             // Callback when CMD9 received
    .cb_cmdA           = cb_cmdA,             // Callback when CMDA received
    .arg               = NULL                 // Argument indicating this SPI Slave HD peripheral instance
  };

  spi_slave_hd_init(SLAVE_HOST, &bus_cfg, &slave_hd_cfg);
}

extern "C" void app_main()
{
  // ----- Runtime inits
  stats::init();
  calc_sin();
  bootloader_random_enable();

  // ----- Wi-Fi init
  initialize_nvs();
  initialize_wifi();
  net.is_init = true;
  net.queue = xQueueCreate(2, sizeof(int));
  xTaskCreatePinnedToCore(net_task, "net helper", 4096, NULL, 5, NULL, 0);

  // ----- Test GPIO init

  gpio_set_direction((gpio_num_t)GPIO_TEST1, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)GPIO_TEST2, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)GPIO_TEST3, GPIO_MODE_OUTPUT);

  // ----- DMA send/receive init
  send_buf = (u8*)heap_caps_calloc(1, DMA_BUF_SIZE, MALLOC_CAP_DMA);

  if (!send_buf)
  {
    ESP_LOGE(TAG, "Cannot allocate memory for send buf!");
    abort();
  }

  for (int i = 0; i < QUEUE_SIZE; i++)
  {
    recv_buf[i] = (u8*)heap_caps_calloc(1, DMA_BUF_SIZE, MALLOC_CAP_DMA);

    if (!recv_buf[i])
    {
      ESP_LOGE(TAG, "Cannot allocate memory for receive buf!");
      abort();
    }
  }

  drqueue = xQueueCreate(2, sizeof(int));
  // xTaskCreatePinnedToCore(receiver_task, "receiver", 4096, NULL, -1, NULL, 0);
  xTaskCreatePinnedToCore(sender_task, "sender", 4096, NULL, 23, &sender_task_h, 0);

  // ----- LibXM init
  initialize_xm();

  // ----- SPI slave init
  init_slave_hd();
  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
  error = ESP_ERR_RESET;
  net.state = NETWORK_CLOSED;

  // ----- Console init
  initialize_console();
  xTaskCreatePinnedToCore(console_task, "console", 4096, NULL, 1, NULL, 0);
  
  TaskHandle_t wifiHandle = xTaskGetHandle("wifi");
  vTaskPrioritySet(wifiHandle, 21);
}
