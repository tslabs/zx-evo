
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <esp_crc.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "main.h"
#include "mem_obj.h"
#include "spi_slave.h"
#include "xm.h"
#include "xm_cpp.h"
#include "stats.h"
#include "esp_spi_defs.h"

using namespace stats;

QueueHandle_t xm_queue;
QueueHandle_t i2s_queue;
QueueHandle_t player_queue;

#define MCLK_IO        GPIO_NUM_14      // I2S bit clock io number
#define BCLK_IO        GPIO_NUM_15      // I2S bit clock io number
#define WS_IO          GPIO_NUM_16      // I2S word select io number
#define DOUT_IO        GPIO_NUM_17      // I2S data out io number

#define XM_SAMPLE_RATE        44100
#define XM_FRAME_MS           100
#define XM_SAMPLES_PER_BUFFER (XM_SAMPLE_RATE * XM_FRAME_MS / 1000)
#define XM_BUF_SIZE           (XM_SAMPLES_PER_BUFFER * sizeof(i16) * 2)
#define XM_BUF_NUM            3
i16 *xm_buf[XM_BUF_NUM];

void xm_task(void *arg);
void i2s_task(void *arg);
void player_task(void *arg);

int curr_xm_handle;

enum
{
  PLAYER_PLAY,
  PLAYER_STOP
};

void *xm_malloc(size_t size)
{
  void *ctx_mem = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);;

  if (!ctx_mem)
    ESP_LOGE("xm_malloc", "Cannot allocate memory for XM context (%u bytes)!", size);

#ifdef VERBOSE
  else
    printf("xm_malloc", "Memory for XM allocated: 0x%08X, %u bytes\r\n", (unsigned int)ctx_mem, size);
#endif

  return ctx_mem;
}

i2s_chan_handle_t tx_chan;

void initialize_xm()
{
  for (int i = 0; i < XM_BUF_NUM; i++)
  {
    xm_buf[i] = (i16*)heap_caps_malloc(XM_BUF_SIZE, MALLOC_CAP_SPIRAM);
    assert(xm_buf[i]);
  }

  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));

  i2s_std_config_t tx_std_cfg =
  {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(XM_SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg =
    {
      .mclk         = MCLK_IO,
      .bclk         = BCLK_IO,
      .ws           = WS_IO,
      .dout         = DOUT_IO,
      .invert_flags =
      {
        .mclk_inv = false,
        .bclk_inv = false,
        .ws_inv   = false,
      },
    },
  };

  tx_std_cfg.slot_cfg.bit_shift = true;   // Phillips format support

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

  xm_queue = xQueueCreate(XM_BUF_NUM + 1, sizeof(XM_TASK));
  i2s_queue = xQueueCreate(XM_BUF_NUM - 2, sizeof(int));
  player_queue = xQueueCreate(2, sizeof(PLAYER_TASK));

  xTaskCreatePinnedToCore(xm_task, "xm-helper", 4096, NULL, 20, NULL, 0);     // XM helper tasks
  xTaskCreatePinnedToCore(i2s_task, "i2s-writer", 2048, NULL, 22, NULL, 0);   // I2S DAC writer
  xTaskCreatePinnedToCore(player_task, "player", 4096, NULL, 24, NULL, 1);    // XM renderer, libxm (should work on a separate core)
}

void player_task(void *arg)
{
  PLAYER_TASK t;
  t.task = PLAYER_STOP;
  int xm_buf_idx = 0;

  while (1)
  {
    xQueueReceive(player_queue, &t, 0);

    switch (t.task)
    {
      case PLAYER_PLAY:
      {
        auto t1 = esp_timer_get_time();

        for (int i = 0; i < XM_SAMPLES_PER_BUFFER; i++)
        {
          float left, right;
          xm_sample(t.ctx, &left, &right);

          _st.xm_samp_min = min(_st.xm_samp_min, left);
          _st.xm_samp_min = min(_st.xm_samp_min, right);
          _st.xm_samp_max = max(_st.xm_samp_max, left);
          _st.xm_samp_max = max(_st.xm_samp_max, right);

          left = max(left, -1);
          left = min(left, 1);
          right = max(right, -1);
          right = min(right, 1);

          int mvol = 32767;
          xm_buf[xm_buf_idx][2 * i]     = (i16)(left * mvol);
          xm_buf[xm_buf_idx][2 * i + 1] = (i16)(right * mvol);
        }

        auto t2  = esp_timer_get_time();

        xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
        xm_buf_idx++; xm_buf_idx %= XM_BUF_NUM;

        int t = (t2 - t1);
        _st.xm_render_last_us = t;
        _st.xm_render_min_us = min(_st.xm_render_min_us, t);
        _st.xm_render_max_us = max(_st.xm_render_max_us, t);

        int cpu = t * XM_SAMPLE_RATE / XM_SAMPLES_PER_BUFFER / 10000;
        _st.xm_render_last_cpu = cpu;
        _st.xm_render_min_cpu = min(_st.xm_render_min_cpu, cpu);
        _st.xm_render_max_cpu = max(_st.xm_render_max_cpu, cpu);
      }
      break;

      case PLAYER_STOP:
      {
        memset(xm_buf[xm_buf_idx], 0, XM_BUF_SIZE);
        xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
        xm_buf_idx++; xm_buf_idx %= XM_BUF_NUM;
      }
      break;
    }
  }
}

void IRAM_ATTR i2s_task(void *arg)
{
  int idx;

  while (1)
  {
    xQueueReceive(i2s_queue, &idx, portMAX_DELAY);
    // gpio_set_level((gpio_num_t)GPIO_TEST2, 1); gpio_set_level((gpio_num_t)GPIO_TEST2, 0); // !!!

    // gpio_set_level((gpio_num_t)GPIO_TEST1, 1); // !!!
    size_t w_bytes = 0;
    i2s_channel_write(tx_chan, xm_buf[idx], XM_BUF_SIZE, &w_bytes, 1000);
    // gpio_set_level((gpio_num_t)GPIO_TEST1, 0); // !!!
  }
}

void IRAM_ATTR xm_task(void *arg)
{
  XM_TASK task;

  while (1)
  {
    xQueueReceive(xm_queue, &task, portMAX_DELAY);

    switch (task.task)
    {
      // Initialize XM module
      case XM_TASK_INIT:
      {
        MEM_OBJ *obj = &mem_obj[task.handle];
        xm_context_s *ctx = NULL;
        
        int rc = xm_create_context_safe(&ctx, obj->addr, obj->size, XM_SAMPLE_RATE, xm_malloc);

        switch (rc)
        {
          case -1:
            set_status(ESP_ERR_INV_XM);

            ESP_LOGE("xm_task", "XM module is not sane!");
          break;

          case -2:
            set_status(ESP_ERR_OUT_OF_MEMORY);

            ESP_LOGE("xm_task", "XM context memory allocation error!");
          break;

          default:
          {
            free(obj->addr);

            obj->addr = (void*)ctx;
            obj->size = rc;
            obj->state = XM_OBJ_ST_STOPPED;

            xm_set_max_loop_count((xm_context_s*)obj->addr, 0);

            set_ok_ready();

#ifdef VERBOSE
            printf("xm_task", "XM context created\r\n");
#endif
          }
          break;

        }
      }
      break;

      case XM_TASK_PLAY:
      {
        MEM_OBJ *obj = &mem_obj[task.handle];

        if (!obj->addr || (obj->state != XM_OBJ_ST_STOPPED))
        {
          set_status(ESP_ERR_INV_STATE);

          ESP_LOGE("xm_task", "Attempt to play not initialized module!");
        }
        else
        {
          PLAYER_TASK t;
          t.task = PLAYER_PLAY;
          t.ctx = (xm_context_s*)obj->addr;
          xQueueSend(player_queue, &t, portMAX_DELAY);

          curr_xm_handle = task.handle;
          obj->state = XM_OBJ_ST_PLAYING;
          set_ok_ready();
        }
      }
      break;

      case XM_TASK_STOP:
      {
        PLAYER_TASK t;
        t.task = PLAYER_STOP;
        xQueueSend(player_queue, &t, portMAX_DELAY);

        MEM_OBJ *obj = &mem_obj[curr_xm_handle];
        if (obj) obj->state = XM_OBJ_ST_STOPPED;
        set_ok_ready();
      }
      break;
    }
  }
}
