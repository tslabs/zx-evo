
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "types.h"
#include "xm.h"
#include "stats.h"
#include "xm_cpp.h"

using namespace stats;

QueueHandle_t xm_queue;
xm_context_t *xm_ctx;
const void *xm_mem = 0;

QueueHandle_t i2s_queue;

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

// #include "xm/Human_Essence_32ch.h"
// #include "xm/sakic_rainy_day_8bit_opt_32ch.h"
// #include "xm/dwiizle_bmp_no_voice_32ch.h"
#include "xm/tunar2_32ch.h"
// #include "xm/11my_8_32ch.h"
// #include "xm/ufo_religion_26ch.h"
// #include "xm/Deadly_Shadows_20ch.h"
// #include "xm/cracked-zone_rmx_20ch.h"
// #include "xm/3DMARK99_test2_16ch.h"
// #include "xm/FR-EDIT_last_16ch.h"
// #include "xm/trackbal_16ch.h"
// #include "xm/AT4RE_Challenge02_12ch.h"
// #include "xm/thunderstruck_remix_10ch.h"
// #include "xm/zap-t-balls_intro_8ch.h"
// #include "xm/aerosol_4ch.h"
// #include "xm/stellar_valley_32ch.h"

void *xm_malloc(size_t size)
{
  void *ctx_mem = malloc(size);

  if (!ctx_mem)
    ESP_LOGE("xm_malloc", "Cannot allocate memory for XM context (%u bytes)!", size);
  else
    ESP_LOGD("xm_malloc", "Memory for XM allocated: 0x%08X, %u bytes", (unsigned int)ctx_mem, size);

  return ctx_mem;
}

bool is_xm_init = false;

i2s_chan_handle_t tx_chan;

void initialize_xm()
{
  for (int i = 0; i < XM_BUF_NUM; i++)
  {
    xm_buf[i] = (i16*)malloc(XM_BUF_SIZE);
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

  xTaskCreatePinnedToCore(xm_task, "libxm", 4096, NULL, 23, NULL, 1);
  xTaskCreatePinnedToCore(i2s_task, "i2s-writer", 4096, NULL, 22, NULL, 0);
}

void player_task(void *arg)
{
  while (!is_xm_init)
    vTaskDelay(pdMS_TO_TICKS(20));

  XM_TASK t;
  // t.id = XM_TASK_SILENCE;
  // xQueueSend(xm_queue, &t, portMAX_DELAY);
  // xQueueSend(xm_queue, &t, portMAX_DELAY);

  while (1)
  {
    t.id = XM_TASK_RENDER;
    xQueueSend(xm_queue, &t, portMAX_DELAY);
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
  static int xm_buf_idx = 0;

  // !!!
  _delay_ms(200);
  task.id = XM_TASK_INIT;
  xQueueSend(xm_queue, &task, 0);

  while (1)
  {
    xQueueReceive(xm_queue, &task, portMAX_DELAY);

    switch (task.id)
    {
      // Initialize XM module
      case XM_TASK_INIT:
      {
        auto rc = xm_create_context_safe(&xm_ctx, (void*)xm_module, sizeof(xm_module), XM_SAMPLE_RATE, xm_malloc);
        xm_set_max_loop_count(xm_ctx, 0);
        is_xm_init = true;

        xTaskCreatePinnedToCore(player_task, "player", 4096, NULL, 20, NULL, 0);
        ESP_LOGD("xm_task", "XM context created: rc=%u", rc);
      }
      break;

      case XM_TASK_SILENCE:
      {
        memset(xm_buf[xm_buf_idx], 0, XM_BUF_SIZE);
        xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
        xm_buf_idx++; xm_buf_idx %= XM_BUF_NUM;
      }
      break;

      // Render sound buffer from module
      case XM_TASK_RENDER:
      {

        // gpio_set_level((gpio_num_t)GPIO_TEST3, 1);  // !!!
        auto t1 = esp_timer_get_time();

        for (int i = 0; i < XM_SAMPLES_PER_BUFFER; i++)
        {
          float left, right;
          xm_sample(xm_ctx, &left, &right);

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

        // gpio_set_level((gpio_num_t)GPIO_TEST3, 0); // !!!
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

      default:
        ;
    }
  }
}
