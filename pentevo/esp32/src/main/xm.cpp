#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_console.h"
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
#include "sdmmc.h"

using namespace stats;

QueueHandle_t xm_queue;
QueueHandle_t i2s_queue;
QueueHandle_t player_queue;

int master_volume = 50000;
int curr_xm_handle = -1;

#define BCLK_IO        GPIO_NUM_15      // I2S bit clock io number
#define WS_IO          GPIO_NUM_16      // I2S word select io number
#define DOUT_IO        GPIO_NUM_17      // I2S data out io number

#define XM_SAMPLE_RATE        44100
#ifdef CONFIG_SPIRAM
  #define XM_FRAME_MS           100
#else
  #define XM_FRAME_MS           10
#endif
#define XM_SAMPLES_PER_BUFFER (XM_SAMPLE_RATE * XM_FRAME_MS / 1000)
#define XM_BUF_SIZE           (XM_SAMPLES_PER_BUFFER * sizeof(i16) * 2)
#define XM_BUF_NUM            3
#define XM_INFO_PATH_MAX      128

EXT_RAM_BSS_ATTR i16 xm_buf[XM_BUF_NUM][XM_BUF_SIZE];

void xm_task(void *arg);
void i2s_task(void *arg);
void player_task(void *arg);

const char XM_TAG[] = "xm";

enum
{
  PLAYER_PLAY,
  PLAYER_STOP
};

void *xm_malloc(size_t size)
{
  void *ctx_mem = malloc_spiram(size);

  if (!ctx_mem)
    ESP_LOGE("xm_malloc", "Cannot allocate memory for XM context (%u bytes)!", size);

#ifdef VERBOSE
  else
    printf("Memory for XM allocated: 0x%08X, %u bytes\r\n", (unsigned int)ctx_mem, size);
#endif

  return ctx_mem;
}

i2s_chan_handle_t tx_chan;

void initialize_xm()
{
  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  ESP_ERROR_CHECK(i2s_new_channel(&tx_chan_cfg, &tx_chan, NULL));
  log_sram_used(__FILE_NAME__ ": i2s_new_channel");

  i2s_std_config_t tx_std_cfg =
  {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(XM_SAMPLE_RATE),
    .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg =
    {
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
  log_sram_used(__FILE_NAME__ ": tx_std_cfg");

  tx_std_cfg.slot_cfg.bit_shift = true;   // Phillips format support

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &tx_std_cfg));
  log_sram_used(__FILE_NAME__ ": i2s_channel_init_std_mode");
  ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
  log_sram_used(__FILE_NAME__ ": i2s_channel_enable");

  xm_queue = xQueueCreate(XM_BUF_NUM + 1, sizeof(XM_TASK));
  i2s_queue = xQueueCreate(XM_BUF_NUM - 2, sizeof(int));
  player_queue = xQueueCreate(2, sizeof(PLAYER_TASK));

  xTaskCreatePinnedToCoreWithCaps(xm_task, "xm-helper", 3072, NULL, XM_HELPER_TASK_PRIO, NULL, 0, MALLOC_CAP_SPIRAM);     // XM helper tasks
  log_sram_used(__FILE_NAME__ ": TaskCreate xm_task");
  xTaskCreatePinnedToCoreWithCaps(i2s_task, "i2s-writer", 2048, NULL, I2S_TASK_PRIO, NULL, 0, MALLOC_CAP_SPIRAM);         // I2S DAC writer
  log_sram_used(__FILE_NAME__ ": TaskCreate i2s_task");
  xTaskCreatePinnedToCoreWithCaps(player_task, "player", 2048, NULL, XM_PLAYER_TASK_PRIO, NULL, 1, MALLOC_CAP_SPIRAM);    // XM renderer, libxm (should work on a separate core)
  log_sram_used(__FILE_NAME__ ": TaskCreate player_task");
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

          int l = left * master_volume;
          l = max(l, -32768);
          l = min(l, 32767);
          xm_buf[xm_buf_idx][2 * i] = l;

          int r = right * master_volume;
          r = max(r, -32768);
          r = min(r, 32767);
          xm_buf[xm_buf_idx][2 * i + 1] = r;
        }

        auto t2  = esp_timer_get_time();

        xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
        xm_buf_idx++;
        xm_buf_idx %= XM_BUF_NUM;

        int t_us = (int)(t2 - t1);
        _st.xm_render_last_us = t_us;
        _st.xm_render_min_us = min(_st.xm_render_min_us, t_us);
        _st.xm_render_max_us = max(_st.xm_render_max_us, t_us);

        int cpu = t_us * XM_SAMPLE_RATE / XM_SAMPLES_PER_BUFFER / 10000;
        _st.xm_render_last_cpu = cpu;
        _st.xm_render_min_cpu = min(_st.xm_render_min_cpu, cpu);
        _st.xm_render_max_cpu = max(_st.xm_render_max_cpu, cpu);
      }
      break;

      case PLAYER_STOP:
      {
        memset(xm_buf[xm_buf_idx], 0, XM_BUF_SIZE);
        xQueueSend(i2s_queue, &xm_buf_idx, portMAX_DELAY);
        xm_buf_idx++;
        xm_buf_idx %= XM_BUF_NUM;
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

        if (obj->type == OBJ_TYPE_XM)
        {
          xm_context_s *ctx = NULL;

          int rc = xm_create_context_safe(&ctx, obj->addr, obj->size, XM_SAMPLE_RATE, xm_malloc);

          if (rc == -1)
          {
            ESP_LOGE("xm_task", "XM module error!");
            set_status(ESP_ERR_INV_XM);
            break;
          }

          else if (rc == -2)
          {
            ESP_LOGE("xm_task", "XM context memory allocation error!");
            set_status(ESP_ERR_OUT_OF_MEMORY);
            break;
          }

#ifdef VERBOSE
          printf("XM context created\r\n");
#endif

          free(obj->addr);
          obj->type = OBJ_TYPE_XMC;
          obj->addr = (void*)ctx;
          obj->size = rc;
        }

        else if (obj->type == OBJ_TYPE_XMC)
        {
          u32 ctx_size = *(u32*)obj->addr;
          u8 *p = (u8*)obj->addr;
          int rel_sz = *(u16*)&p[ctx_size];
          u32 *rel = (u32*)&p[ctx_size + sizeof(u16)];

#ifdef VERBOSE
          printf("XM context relocating entries %d\r\n", rel_sz);
#endif

          for (int i = 0; i < rel_sz; i++)
          {
            u32 *r = (u32*)(rel[i] + (u32)obj->addr);

#ifdef VERBOSE
            printf("Entry %d: &%08lX %08lX\r\n", i, (u32)rel[i], *r);
#endif

            *r += (u32)obj->addr;
          }
        }

        obj->state = XM_OBJ_ST_STOPPED;
        xm_set_max_loop_count((xm_context_s*)obj->addr, 0);

        // Clear stats
        _st.xm_render_min_us = INT_MAX;
        _st.xm_render_min_cpu = INT_MAX;
        _st.xm_render_max_us = 0;
        _st.xm_render_max_cpu = 0;

#ifdef VERBOSE
        printf("XM init success\r\n");
#endif
        set_status(ESP_ST_READY);
      }
      break;

      case XM_TASK_PLAY:
      {
        MEM_OBJ *obj = &mem_obj[task.handle];

        if (!obj->addr || (obj->state != XM_OBJ_ST_STOPPED))
        {
          ESP_LOGE("xm_task", "Attempt to play not initialized module!");
          set_status(ESP_ERR_INV_STATE);
        }
        else
        {
          PLAYER_TASK t;
          t.task = PLAYER_PLAY;
          t.ctx = (xm_context_s*)obj->addr;
          xQueueSend(player_queue, &t, portMAX_DELAY);

          curr_xm_handle = task.handle;
          obj->state = XM_OBJ_ST_PLAYING;
          set_status(ESP_ST_READY);
        }
      }
      break;

      case XM_TASK_STOP:
      {
        PLAYER_TASK t;
        t.task = PLAYER_STOP;
        xQueueSend(player_queue, &t, portMAX_DELAY);

        int handle = task.handle;
        if (handle < 0 || handle >= OBJ_HANDLES_MAX)
          handle = curr_xm_handle;

        if (handle >= 0 && handle < OBJ_HANDLES_MAX && mem_obj[handle].addr)
          mem_obj[handle].state = XM_OBJ_ST_STOPPED;

        set_status(ESP_ST_READY);
      }
      break;
    }
  }
}

// ------------- CLI commands ----------------

typedef struct
{
  u8 valid;
  u16 version;
  u32 header_size;
  u16 song_length;
  u16 restart_position;
  u16 num_channels;
  u16 num_patterns;
  u16 num_instruments;
  u16 flags;
  u16 tempo;
  u16 bpm;
  u32 file_size;
  char path[XM_INFO_PATH_MAX];
  char module_name[21];
  char tracker_name[21];
} XM_INFO;

EXT_RAM_BSS_ATTR XM_INFO xm_info[OBJ_HANDLES_MAX] = {};

u16 xm_rd_le16(const u8 *p)
{
  return (u16)p[0] | ((u16)p[1] << 8);
}

u32 xm_rd_le32(const u8 *p)
{
  return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

void xm_copy_trimmed(char *dst, size_t dst_size, const u8 *src, size_t src_size)
{
  if (!dst || !dst_size) return;

  size_t n = src_size;
  while (n && (src[n - 1] == 0 || src[n - 1] == ' ')) n--;
  if (n >= dst_size) n = dst_size - 1;

  if (n) memcpy(dst, src, n);
  dst[n] = 0;
}

void xm_clear_info(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return;
  memset(&xm_info[handle], 0, sizeof(xm_info[handle]));
}

int xm_parse_info(int handle, const char *path, const u8 *data, size_t size)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return 0;
  if (!data || size < 80) return 0;
  if (memcmp(data, "Extended Module: ", 17)) return 0;
  if (data[37] != 0x1A) return 0;

  XM_INFO *info = &xm_info[handle];
  memset(info, 0, sizeof(*info));

  info->valid = 1;
  info->version = xm_rd_le16(data + 58);
  info->header_size = xm_rd_le32(data + 60);
  info->song_length = xm_rd_le16(data + 64);
  info->restart_position = xm_rd_le16(data + 66);
  info->num_channels = xm_rd_le16(data + 68);
  info->num_patterns = xm_rd_le16(data + 70);
  info->num_instruments = xm_rd_le16(data + 72);
  info->flags = xm_rd_le16(data + 74);
  info->tempo = xm_rd_le16(data + 76);
  info->bpm = xm_rd_le16(data + 78);
  info->file_size = (u32)size;

  xm_copy_trimmed(info->module_name, sizeof(info->module_name), data + 17, 20);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), data + 38, 20);

  if (path)
  {
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = 0;
  }

  return 1;
}

const char *xm_obj_state_str(u8 st)
{
  switch (st)
  {
    case XM_OBJ_ST_STOPPED: return "Stopped";
    case XM_OBJ_ST_PLAYING: return "Playing";
    case OBJ_ST_NONE:       return "None";
    case OBJ_ST_ERROR:      return "Error";
    default:                return "Unknown";
  }
}

const char *xm_obj_type_str(u8 type)
{
  switch (type)
  {
    case OBJ_TYPE_XM:  return "XM";
    case OBJ_TYPE_XMC: return "XMC";
    default:           return "Other";
  }
}

int xm_find_playing_handle()
{
  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (mem_obj[i].addr && mem_obj[i].state == XM_OBJ_ST_PLAYING)
      return i;

  return -1;
}

int xm_find_first_handle()
{
  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (mem_obj[i].addr && (mem_obj[i].type == OBJ_TYPE_XM || mem_obj[i].type == OBJ_TYPE_XMC))
      return i;

  return -1;
}

int xm_wait_for_state(int handle, u8 state, int timeout_ms)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return 0;

  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    if (mem_obj[handle].addr && mem_obj[handle].state == state)
      return 1;

    u8 st = rd_reg8(ESP_REG_STATUS);
    if (st >= 0x80) return 0;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return 0;
}

int xm_wait_for_init(int handle, int timeout_ms)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX) return 0;

  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    if (mem_obj[handle].addr && mem_obj[handle].type == OBJ_TYPE_XMC && mem_obj[handle].state == XM_OBJ_ST_STOPPED)
      return 1;

    u8 st = rd_reg8(ESP_REG_STATUS);
    if (st >= 0x80) return 0;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return 0;
}

int xm_stop_current_playback()
{
  int handle = xm_find_playing_handle();
  if (handle < 0) return 0;

  XM_TASK task = {};
  task.task = XM_TASK_STOP;
  task.handle = handle;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, XM_OBJ_ST_STOPPED, 2000))
  {
    printf("XM stop timeout, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  return 0;
}

int xm_parse_handle_arg(const char *s, int *out_handle)
{
  if (!s || !out_handle) return 0;

  char *endp = NULL;
  unsigned long v = strtoul(s, &endp, 0);
  if (!endp || *endp || v >= OBJ_HANDLES_MAX) return 0;

  *out_handle = (int)v;
  return 1;
}

int xm_load_file(const char *path)
{
  if (!path || !path[0])
  {
    printf("Usage: xm load <path>\r\n");
    return 1;
  }

  size_t size = 0;
  esp_err_t err = sd_fs_read_file("/sd", path, NULL, 0, &size);
  if (err != ESP_OK) return 1;
  if (size > INT_MAX)
  {
    printf("E: XM file too large: %u bytes\r\n", (unsigned)size);
    return 1;
  }

  int handle = make_obj((int)size, OBJ_TYPE_XM);
  if (handle < 0)
    return 1;

  err = sd_fs_read_file("/sd", path, mem_obj[handle].addr, size, NULL);
  if (err != ESP_OK)
  {
    delete_obj(handle);
    return 1;
  }

  if (!xm_parse_info(handle, path, (const u8*)mem_obj[handle].addr, size))
  {
    printf("E: invalid XM header\r\n");
    delete_obj(handle);
    return 1;
  }

  XM_TASK task = {};
  task.task = XM_TASK_INIT;
  task.handle = handle;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_init(handle, 3000))
  {
    printf("XM init failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    delete_obj(handle);
    xm_clear_info(handle);
    return 1;
  }

  printf("XM loaded: handle=%02X  size=%u\r\n", handle, (unsigned)mem_obj[handle].size);
  if (xm_info[handle].module_name[0])
    printf("  Module : %s\r\n", xm_info[handle].module_name);
  if (xm_info[handle].tracker_name[0])
    printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);

  return 0;
}

int xm_list_cmd()
{
  int found = 0;

  printf("\r\nXM objects\r\n");
  printf("HH  Type  State     Size      Name\r\n");

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
  {
    if (!mem_obj[i].addr) continue;
    if (mem_obj[i].type != OBJ_TYPE_XM && mem_obj[i].type != OBJ_TYPE_XMC) continue;

    const char *name = xm_info[i].module_name[0] ? xm_info[i].module_name : xm_info[i].path;
    printf("%02X  %-4s  %-8s  %-8u  %s\r\n",
      i,
      xm_obj_type_str(mem_obj[i].type),
      xm_obj_state_str(mem_obj[i].state),
      (unsigned)mem_obj[i].size,
      name && name[0] ? name : "-");
    found++;
  }

  if (!found)
    printf("(none)\r\n");

  printf("\r\n");
  return 0;
}

int xm_info_cmd(int argc, char **argv)
{
  int handle = -1;

  if (argc >= 3)
  {
    if (!xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Bad <handle>: %s\r\n", argv[2]);
      return 1;
    }
  }
  else if (curr_xm_handle >= 0 && curr_xm_handle < OBJ_HANDLES_MAX && mem_obj[curr_xm_handle].addr)
    handle = curr_xm_handle;
  else
    handle = xm_find_first_handle();

  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !mem_obj[handle].addr)
  {
    printf("No XM object selected\r\n");
    return 1;
  }

  XM_INFO *info = &xm_info[handle];

  printf("\r\nXM object %02X\r\n", handle);
  printf("Type            : %s\r\n", xm_obj_type_str(mem_obj[handle].type));
  printf("State           : %s\r\n", xm_obj_state_str(mem_obj[handle].state));
  printf("Object size     : %u\r\n", (unsigned)mem_obj[handle].size);

  if (info->valid)
  {
    printf("Path            : %s\r\n", info->path[0] ? info->path : "-");
    printf("Module name     : %s\r\n", info->module_name[0] ? info->module_name : "-");
    printf("Tracker name    : %s\r\n", info->tracker_name[0] ? info->tracker_name : "-");
    printf("Version         : %u.%02u\r\n", (unsigned)(info->version >> 8), (unsigned)(info->version & 0xFF));
    printf("File size       : %u\r\n", (unsigned)info->file_size);
    printf("Header size     : %u\r\n", (unsigned)info->header_size);
    printf("Song length     : %u\r\n", (unsigned)info->song_length);
    printf("Restart pos     : %u\r\n", (unsigned)info->restart_position);
    printf("Channels        : %u\r\n", (unsigned)info->num_channels);
    printf("Patterns        : %u\r\n", (unsigned)info->num_patterns);
    printf("Instruments     : %u\r\n", (unsigned)info->num_instruments);
    printf("Flags           : 0x%04X\r\n", (unsigned)info->flags);
    printf("Tempo           : %u\r\n", (unsigned)info->tempo);
    printf("BPM             : %u\r\n", (unsigned)info->bpm);
  }
  else
    printf("Header info     : not available\r\n");

  printf("\r\n");
  return 0;
}

int xm_play_cmd(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !mem_obj[handle].addr)
  {
    printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XMC)
  {
    printf("Handle %02X is not an initialized XMC object\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].state == XM_OBJ_ST_PLAYING)
  {
    if (xm_stop_current_playback()) return 1;
  }
  else
  {
    int playing = xm_find_playing_handle();
    if (playing >= 0 && playing != handle)
      if (xm_stop_current_playback()) return 1;
  }

  XM_TASK task = {};
  task.task = XM_TASK_PLAY;
  task.handle = handle;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, XM_OBJ_ST_PLAYING, 2000))
  {
    printf("XM play failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  printf("XM playing: %02X\r\n", handle);
  return 0;
}

int xm_stop_cmd()
{
  int handle = xm_find_playing_handle();
  if (handle < 0)
  {
    printf("XM is not playing\r\n");
    return 0;
  }

  if (xm_stop_current_playback()) return 1;

  printf("XM stopped: %02X\r\n", handle);
  return 0;
}

int xm_del_cmd(int handle)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !mem_obj[handle].addr)
  {
    printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XM && mem_obj[handle].type != OBJ_TYPE_XMC)
  {
    printf("Handle %02X is not an XM object\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].state == XM_OBJ_ST_PLAYING)
  {
    XM_TASK task = {};
    task.task = XM_TASK_STOP;
    task.handle = handle;
    xQueueSend(xm_queue, &task, portMAX_DELAY);

    if (!xm_wait_for_state(handle, XM_OBJ_ST_STOPPED, 2000))
    {
      printf("XM stop before delete failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
      return 1;
    }
  }

  if (!delete_obj(handle))
  {
    printf("Delete failed for handle %02X\r\n", handle);
    return 1;
  }

  if (curr_xm_handle == handle)
    curr_xm_handle = -1;

  xm_clear_info(handle);

  printf("XM deleted: %02X\r\n", handle);
  return 0;
}

int xm_vol_cmd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Master volume: %d\r\n", master_volume / 1000);
    return 0;
  }

  char *endp = NULL;
  unsigned long vol = strtoul(argv[2], &endp, 0);
  if (!endp || *endp || vol > 255)
  {
    printf("Bad <volume>: %s (expected 0..255)\r\n", argv[2]);
    return 1;
  }

  master_volume = (int)(vol * 1000UL);
  printf("Master volume: %d\r\n", master_volume / 1000);
  return 0;
}

int xm_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  xm load <path>\r\n");
    printf("  xm list\r\n");
    printf("  xm info [handle]\r\n");
    printf("  xm play <handle>\r\n");
    printf("  xm stop\r\n");
    printf("  xm del <handle>\r\n");
    printf("  xm vol [0..255]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "load"))
    return xm_load_file(argc >= 3 ? argv[2] : NULL);

  if (!strcmp(op, "list"))
    return xm_list_cmd();

  if (!strcmp(op, "info"))
    return xm_info_cmd(argc, argv);

  if (!strcmp(op, "stop"))
    return xm_stop_cmd();

  if (!strcmp(op, "vol"))
    return xm_vol_cmd(argc, argv);

  if (!strcmp(op, "play"))
  {
    int handle = -1;
    if (argc < 3 || !xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Usage: xm play <handle>\r\n");
      return 1;
    }
    return xm_play_cmd(handle);
  }

  if (!strcmp(op, "del"))
  {
    int handle = -1;
    if (argc < 3 || !xm_parse_handle_arg(argv[2], &handle))
    {
      printf("Usage: xm del <handle>\r\n");
      return 1;
    }
    return xm_del_cmd(handle);
  }

  printf("Unknown xm subcommand: %s\r\n", op);
  return 1;
}

void xm_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "xm",
    .help     = "XM commands: load/list/info/play/stop/del/vol",
    .hint     = NULL,
    .func     = &xm_cmd,
    .argtable = NULL,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
