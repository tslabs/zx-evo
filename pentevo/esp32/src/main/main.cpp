
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_slave_hd.h"
#include "driver/gpio.h"
#include "soc/rtc_cntl_reg.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "console.h"

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#include "spi_slave.h"
#include "ft812.h"

// Pin setting
#define GPIO_MOSI     13
#define GPIO_MISO     11
#define GPIO_SCLK     12
#define GPIO_CS       10
#define GPIO_TEST     (gpio_num_t)3

#define SLAVE_HOST    SPI2_HOST
#define DMA_CHAN      SPI_DMA_CH_AUTO
#define QUEUE_SIZE    4

#define DMA_BUF_SIZE  32768

const char TAG[] = "SPI_SLAVE";
const char cp_string[] = "ESP32-S3 SPI WiFi Module, ver.0.1, (c)2024, TS-Labs";

enum
{
  REQ_INFO,
  REQ_RND,
  REQ_TEST2,
  REQ_TEST3,
};

volatile struct
{
  bool is_req;
  u8 type;
} data_req;

#define DEGS 360
#define PI 3.14159265358979323846
float sin_tab[DEGS];

void calc_sin()
{
  for (int i = 0; i < DEGS; i++)
  {
    float s = sin((float)i * PI / (DEGS / 2));
    sin_tab[i] = s;
  }
}

#include "cobra.cpp"
#include "tonnel.cpp"
#include "system.cpp"
#include "wifi.cpp"
#include "console.cpp"

void IRAM_ATTR splash(int n)
{
  while (n--)
  {
    // gpio_set_level(GPIO_TEST, 1);
    // gpio_set_level(GPIO_TEST, 0);
  }
}

// Regs have been read
bool IRAM_ATTR cb_regs_read(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(3);
  return true;
}

bool IRAM_ATTR cb_recv_dma_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(4);
  return true;
}

bool IRAM_ATTR cb_cmd7(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(7);
  return true;
}

// DMA sending done
static IRAM_ATTR bool cb_cmd8(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(8);

  return true;
}

bool IRAM_ATTR cb_cmd9(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(9);
  return true;
}

bool IRAM_ATTR cb_cmdA(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  // splash(10);
  return true;
}

void IRAM_ATTR set_status(u8 status)
{
  spi_slave_hd_write_buffer(SLAVE_HOST, ESP_REG_STATUS, (u8*)&status, 1);
}

void IRAM_ATTR set_error(u8 error)
{
  spi_slave_hd_write_buffer(SLAVE_HOST, ESP_REG_EXT_STAT, (u8*)&error, 1);
}

void IRAM_ATTR set_data_size(u32 size)
{
  spi_slave_hd_write_buffer(SLAVE_HOST, ESP_REG_DATA_SIZE, (u8*)&size, 4);
}

extern "C"
{
void spi_slave_hd_intr_segment2();
}

// Called after Master has finished writing shared regs
bool IRAM_ATTR cb_cmd(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  u8 cmd;
  u8 zero = 0;
  spi_slave_hd_read_buffer(SLAVE_HOST, ESP_REG_COMMAND, &cmd, 1);
  spi_slave_hd_write_buffer(SLAVE_HOST, ESP_REG_COMMAND, &zero, 1);

  if (cmd)
  {
    set_status(ESP_ST_BUSY);

    switch (cmd)
    {
      case ESP_CMD_DMA_END:
        *(u32*)0x60024044 = 1 << 5; // trigger CMD8 intr
      break;

      case ESP_CMD_GET_INFO:
        data_req.is_req = true;
        data_req.type = REQ_INFO;
      break;

      case ESP_CMD_GET_RND:
        data_req.is_req = true;
        data_req.type = REQ_RND;
      break;

      case ESP_CMD_TEST2:
        data_req.is_req = true;
        data_req.type = REQ_TEST2;
      break;

      case ESP_CMD_TEST3:
        data_req.is_req = true;
        data_req.type = REQ_TEST3;
      break;

      case ESP_CMD_RESET:
        REG_WRITE(RTC_CNTL_OPTIONS0_REG, RTC_CNTL_SW_SYS_RST);  // SoC reset
        while (1);
    }
  }

  return true;
}

// DMA to send is ready (the transaction is pushed by sender task)
static IRAM_ATTR bool cb_tx_ready(void *arg, spi_slave_hd_event_t *event, BaseType_t *awoken)
{
  set_status(ESP_ST_DATA_S2M);

  return true;
}

u32 prepare_tx_data(u8 *data, u8 type, u32 size)
{
  switch (type)
  {
    case REQ_INFO:
      size = snprintf((char*)data, size, cp_string);
    break;

    case REQ_RND:
    {
      bootloader_random_enable();
      u32 *p = (u32*)data;
      size = 6144;

      for (int i = 0; i < size / 4; i++)
      { 
        u32 a = rand();
        p[i] = a ^ (a << 1);
      }
    }
    break;

    case REQ_TEST2:
      size = tunnel(data);
    break;

    case REQ_TEST3:
      size = cobra(data);
    break;
  }

  return size;
}

void sender(void *arg)
{
  esp_err_t err = ESP_OK;
  spi_slave_hd_data_t slave_trans;          //Transaction descriptor
  spi_slave_hd_data_t *ret_trans;           //Pointer to the descriptor of return result

  u8 *send_buf;
  send_buf = (u8*)heap_caps_calloc(1, DMA_BUF_SIZE, MALLOC_CAP_DMA);
  if (!send_buf)
  {
    ESP_LOGE(TAG, "Not enough memory!");
    abort();
  }

  // printf("Sender task started\r\n");
  // printf("Send buf: 0x%08X\r\n", send_buf);

  while (1)
  {
    while(!data_req.is_req);
    data_req.is_req = false;

    slave_trans.data = send_buf;
    slave_trans.len = prepare_tx_data(send_buf, data_req.type, DMA_BUF_SIZE);
    set_data_size(slave_trans.len);
    err = spi_slave_hd_queue_trans(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &slave_trans, portMAX_DELAY);
    err = spi_slave_hd_get_trans_res(SLAVE_HOST, SPI_SLAVE_CHAN_TX, &ret_trans, portMAX_DELAY);
    
    if (err != ESP_OK)  {}

    u32 cmd;
    spi_slave_hd_read_buffer(SLAVE_HOST, 0, (u8*)&cmd, 4);
  }
}

void receiver(void *arg)
{
  spi_slave_hd_data_t *ret_trans;
  u32 recv_buf_size = DMA_BUF_SIZE;
  u8 *recv_buf[QUEUE_SIZE];
  spi_slave_hd_data_t slave_trans[QUEUE_SIZE];

  for (int i = 0; i < QUEUE_SIZE; i++)
  {
    recv_buf[i] = (u8*)heap_caps_calloc(1, recv_buf_size, MALLOC_CAP_DMA);

    if (!recv_buf[i])
    {
      ESP_LOGE(TAG, "Not enough memory!");
      abort();
    }
  }

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

  // printf("Receiver task started\r\n");

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

  // printf("Slave init done\r\n");
}

extern "C" void app_main()
{
  // printf("ESP32 SPI\r\n");
  
  calc_sin();
  initialize_nvs();

  // gpio_set_direction(GPIO_TEST, GPIO_MODE_OUTPUT);
  
  initialize_console();
  xTaskCreate(console, "consoleTask", 8192, NULL, 1, NULL);
  
  data_req.is_req = false;
  init_slave_hd();
  set_status(ESP_ST_READY);
  set_error(ESP_ERR_RESET);
  xTaskCreate(receiver, "recvTask", 4096, NULL, 1, NULL);
  xTaskCreate(sender, "sendTask", 4096, NULL, 1, NULL);
}
