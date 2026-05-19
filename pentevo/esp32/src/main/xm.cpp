#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_console.h"
#include "esp_heap_caps.h"
#include <esp_crc.h>
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "sdkconfig.h"

#include "main.h"
#include "mem_obj.h"
#include "spi_slave.h"
#include "xm.h"
#include "xm_internal.h"
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
volatile bool xm_player_stopped = true;

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
int xm_wait_for_player_stop(int timeout_ms);
esp_err_t xm_host_stream_ensure_runtime();

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
  i2s_queue = xQueueCreateWithCaps(XM_BUF_NUM - 2, sizeof(int), task_ram_type_non_critical);
  player_queue = xQueueCreateWithCaps(2, sizeof(PLAYER_TASK), task_ram_type_non_critical);

  xTaskCreatePinnedToCoreWithCaps(xm_task, "xm-helper", 3072, NULL, XM_HELPER_TASK_PRIO, NULL, 0, task_ram_type_non_critical);     // XM helper tasks
  log_sram_used(__FILE_NAME__ ": TaskCreate xm_task");
  xTaskCreatePinnedToCoreWithCaps(i2s_task, "i2s-writer", 2048, NULL, I2S_TASK_PRIO, NULL, 0, task_ram_type_critical);         // I2S DAC writer
  log_sram_used(__FILE_NAME__ ": TaskCreate i2s_task");
  xTaskCreatePinnedToCoreWithCaps(player_task, "player", 2048, NULL, XM_PLAYER_TASK_PRIO, NULL, 1, task_ram_type_non_critical);    // XM renderer, libxm (should work on a separate core)
  log_sram_used(__FILE_NAME__ ": TaskCreate player_task");

  xm_host_stream_ensure_runtime();
  log_sram_used(__FILE_NAME__ ": xm_host_stream_ensure_runtime");
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
        xm_player_stopped = false;

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
        xm_player_stopped = true;
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
          xm_player_stopped = false;
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
        t.ctx = NULL;
        xm_player_stopped = false;
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

int xm_wait_for_player_stop(int timeout_ms)
{
  int waited_ms = 0;

  while (waited_ms < timeout_ms)
  {
    if (xm_player_stopped) return 1;

    vTaskDelay(pdMS_TO_TICKS(10));
    waited_ms += 10;
  }

  return xm_player_stopped ? 1 : 0;
}

int xm_stop_current_playback(bool quiet = false)
{
  int handle = xm_find_playing_handle();
  if (handle < 0) return 0;

  XM_TASK task = {};
  task.task = XM_TASK_STOP;
  task.handle = handle;
  xm_player_stopped = false;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, XM_OBJ_ST_STOPPED, 2000))
  {
    if (!quiet) printf("XM stop timeout, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  if (!xm_wait_for_player_stop(2000))
  {
    if (!quiet) printf("XM player stop timeout\r\n");
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

int xm_delete_all_modules(bool quiet)
{
  if (xm_stop_current_playback(quiet)) return 1;

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
  {
    if (!mem_obj[i].addr) continue;
    if (mem_obj[i].type != OBJ_TYPE_XM && mem_obj[i].type != OBJ_TYPE_XMC) continue;

    if (!delete_obj(i))
    {
      if (!quiet) printf("Delete failed for handle %02X\r\n", i);
      return 1;
    }

    if (curr_xm_handle == i) curr_xm_handle = -1;
    xm_clear_info(i);
  }

  return 0;
}

bool xm_path_is_sd_abs(const char *path)
{
  if (!path) return false;
  if (!strcmp(path, "/sd")) return true;
  return strncmp(path, "/sd/", 4) == 0;
}

bool xm_copy_path_with_ext_case(const char *path, char *out, size_t out_size, bool upper)
{
  const char *slash;
  const char *ext;
  size_t len;

  if (!path || !out || !out_size) return false;

  slash = strrchr(path, '/');
  ext = strrchr(path, '.');
  if (!ext || (slash && ext < slash) || ext[1] == 0) return false;

  len = strlen(path);
  if (len >= out_size) return false;

  memcpy(out, path, len + 1);

  for (char *p = out + (ext - path) + 1; *p; p++)
  {
    unsigned char c = (unsigned char)*p;
    *p = (char)(upper ? toupper(c) : tolower(c));
  }

  return strcmp(out, path) != 0;
}

esp_err_t xm_stat_file_size(const char *path, size_t *out_size)
{
  struct stat st;

  if (!path || !path[0] || !out_size) return ESP_ERR_INVALID_ARG;
  if (stat(path, &st) != 0) return ESP_FAIL;
  if (st.st_size <= 0 || st.st_size > INT_MAX) return ESP_ERR_INVALID_SIZE;

  *out_size = (size_t)st.st_size;
  return ESP_OK;
}

esp_err_t xm_stat_file_size_any_ext_case(const char *path, size_t *out_size)
{
  esp_err_t err;
  char alt[256];

  err = xm_stat_file_size(path, out_size);
  if (err == ESP_OK) return ESP_OK;
  if (err == ESP_ERR_INVALID_ARG || err == ESP_ERR_INVALID_SIZE) return err;

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), false))
  {
    err = xm_stat_file_size(alt, out_size);
    if (err == ESP_OK) return ESP_OK;
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), true))
  {
    err = xm_stat_file_size(alt, out_size);
    if (err == ESP_OK) return ESP_OK;
  }

  return err;
}

FILE *xm_fopen_any_ext_case(const char *path, const char *mode, char *opened_path, size_t opened_path_size)
{
  FILE *fp;
  char alt[256];

  if (opened_path && opened_path_size) opened_path[0] = 0;
  if (!path || !path[0] || !mode) return NULL;

  fp = fopen(path, mode);
  if (fp)
  {
    if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", path);
    return fp;
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), false))
  {
    fp = fopen(alt, mode);
    if (fp)
    {
      if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", alt);
      return fp;
    }
  }

  if (xm_copy_path_with_ext_case(path, alt, sizeof(alt), true))
  {
    fp = fopen(alt, mode);
    if (fp)
    {
      if (opened_path && opened_path_size) snprintf(opened_path, opened_path_size, "%s", alt);
      return fp;
    }
  }

  return NULL;
}

esp_err_t xm_get_file_size(const char *path, size_t *out_size)
{
  esp_err_t err;
  bool was_sd_mounted;
  char full[256];

  if (!path || !path[0] || !out_size) return ESP_ERR_INVALID_ARG;

  *out_size = 0;
  err = xm_stat_file_size_any_ext_case(path, out_size);
  if (err == ESP_OK) return ESP_OK;

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
    return ESP_ERR_INVALID_ARG;
  }

  err = xm_stat_file_size_any_ext_case(full, out_size);
  if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
  return err;
}

esp_err_t xm_read_file_data(const char *path, void *dst, size_t size)
{
  FILE *fp;
  size_t got;
  bool was_sd_mounted;
  char full[256];
  esp_err_t err = ESP_FAIL;

  if (!path || !path[0] || !dst) return ESP_ERR_INVALID_ARG;

  fp = xm_fopen_any_ext_case(path, "rb", NULL, 0);
  if (fp)
  {
    got = fread(dst, 1, size, fp);
    fclose(fp);
    if (got != size) return ESP_FAIL;
    return ESP_OK;
  }

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
    return ESP_ERR_INVALID_ARG;
  }

  fp = xm_fopen_any_ext_case(full, "rb", NULL, 0);
  if (fp)
  {
    got = fread(dst, 1, size, fp);
    fclose(fp);
    err = (got == size) ? ESP_OK : ESP_FAIL;
  }

  if (!was_sd_mounted) sd_fs_unmount("/sd", NULL);
  return err;
}

typedef enum
{
  XM_FILE_FORMAT_XM = 0,
  XM_FILE_FORMAT_XMZ
} xm_file_format_t;

const char *xm_file_format_str(xm_file_format_t format)
{
  switch (format)
  {
    case XM_FILE_FORMAT_XMZ: return "XMZ";
    case XM_FILE_FORMAT_XM:
    default: return "XM";
  }
}

int xm_init_handle(int handle, bool quiet)
{
  XM_TASK task = {};
  task.task = XM_TASK_INIT;
  task.handle = handle;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_init(handle, 3000))
  {
    if (!quiet) printf("XM init failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    delete_obj(handle);
    xm_clear_info(handle);
    return 1;
  }

  return 0;
}

#define XM_STREAM_PATH_MAX 256
#define XM_STREAM_BUFFER_SIZE 512
#define XM_STREAM_ARENA_RESERVE (32 * 1024)

typedef struct XmStreamReader XmStreamReader;
typedef esp_err_t (*XmStreamReadFn)(XmStreamReader *reader, void *dst, size_t size, size_t *out_size);
typedef void (*XmStreamCloseFn)(XmStreamReader *reader);

struct XmStreamReader
{
  FILE *fp;
  size_t file_size;
  size_t pos;
  bool sd_mounted;
  esp_err_t err;
  char opened_path[XM_STREAM_PATH_MAX];
  uint8_t header[60];
  uint32_t module_header_size;
  uint16_t module_flags;
  XmStreamReadFn read;
  XmStreamCloseFn close;
};

esp_err_t xm_stream_file_read(XmStreamReader *reader, void *dst, size_t size, size_t *out_size)
{
  size_t got;

  if (out_size) *out_size = 0;
  if (!reader || !reader->fp || (!dst && size)) return ESP_ERR_INVALID_ARG;
  if (!size) return ESP_OK;

  got = fread(dst, 1, size, reader->fp);
  reader->pos += got;
  if (out_size) *out_size = got;

  if (got != size && ferror(reader->fp))
  {
    reader->err = ESP_FAIL;
    return ESP_FAIL;
  }

  return ESP_OK;
}

void xm_stream_file_close(XmStreamReader *reader)
{
  if (!reader) return;

  if (reader->fp)
  {
    fclose(reader->fp);
    reader->fp = NULL;
  }

  if (reader->sd_mounted)
  {
    sd_fs_unmount("/sd", NULL);
    reader->sd_mounted = false;
  }
}

esp_err_t xm_stream_reader_open(XmStreamReader *reader, const char *path, size_t file_size)
{
  bool was_sd_mounted;
  char full[XM_STREAM_PATH_MAX];

  if (!reader || !path || !path[0]) return ESP_ERR_INVALID_ARG;

  memset(reader, 0, sizeof(*reader));
  reader->err = ESP_OK;
  reader->read = xm_stream_file_read;
  reader->close = xm_stream_file_close;
  reader->file_size = file_size;

  reader->fp = xm_fopen_any_ext_case(path, "rb", reader->opened_path, sizeof(reader->opened_path));
  if (reader->fp) return ESP_OK;

  was_sd_mounted = sd_fs_mounted;
  if (sd_fs_mount("/sd", NULL) != ESP_OK) return ESP_FAIL;
  reader->sd_mounted = !was_sd_mounted;

  if (xm_path_is_sd_abs(path))
  {
    snprintf(full, sizeof(full), "%s", path);
  }
  else if (!sd_fs_build_full_path("/sd", path, full, sizeof(full)))
  {
    reader->close(reader);
    return ESP_ERR_INVALID_ARG;
  }

  reader->fp = xm_fopen_any_ext_case(full, "rb", reader->opened_path, sizeof(reader->opened_path));
  if (!reader->fp)
  {
    reader->close(reader);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t xm_stream_reader_read_exact(XmStreamReader *reader, void *dst, size_t size)
{
  size_t got = 0;

  if (!reader || (!dst && size)) return ESP_ERR_INVALID_ARG;
  if (reader->err != ESP_OK) return reader->err;
  if (!size) return ESP_OK;

  if (reader->pos > reader->file_size || size > reader->file_size - reader->pos)
  {
    reader->err = ESP_ERR_INVALID_SIZE;
    return reader->err;
  }

  esp_err_t err = reader->read(reader, dst, size, &got);
  if (err != ESP_OK)
  {
    reader->err = err;
    return err;
  }

  if (got != size)
  {
    reader->err = ESP_FAIL;
    return reader->err;
  }

  return ESP_OK;
}

esp_err_t xm_stream_reader_skip(XmStreamReader *reader, size_t size)
{
  uint8_t buffer[XM_STREAM_BUFFER_SIZE];
  size_t done = 0;

  while (done < size)
  {
    size_t todo = size - done;
    if (todo > sizeof(buffer)) todo = sizeof(buffer);

    esp_err_t err = xm_stream_reader_read_exact(reader, buffer, todo);
    if (err != ESP_OK) return err;
    done += todo;
  }

  return ESP_OK;
}

esp_err_t xm_stream_reader_read_record(XmStreamReader *reader, uint8_t *dst, size_t dst_size, uint32_t rec_size, size_t already_read)
{
  size_t copy_size;

  if (!reader || (!dst && dst_size)) return ESP_ERR_INVALID_ARG;
  if (rec_size < already_read) return ESP_ERR_INVALID_SIZE;

  copy_size = rec_size - already_read;
  if (already_read >= dst_size)
    copy_size = 0;
  else if (copy_size > dst_size - already_read)
    copy_size = dst_size - already_read;

  if (copy_size)
  {
    esp_err_t err = xm_stream_reader_read_exact(reader, dst + already_read, copy_size);
    if (err != ESP_OK) return err;
  }

  if (rec_size > already_read + copy_size)
    return xm_stream_reader_skip(reader, rec_size - already_read - copy_size);

  return ESP_OK;
}

uint8_t xm_stream_read_u8(XmStreamReader *reader)
{
  uint8_t v = 0;
  xm_stream_reader_read_exact(reader, &v, sizeof(v));
  return v;
}

void *xm_stream_mempool_alloc(char **mempool, char *mempool_end, size_t size, XmStreamReader *reader, bool clear)
{
  void *ptr;

  if (!mempool || !*mempool || !mempool_end || !reader) return NULL;
  if (*mempool > mempool_end)
  {
    reader->err = ESP_ERR_NO_MEM;
    return NULL;
  }

  if (size > (size_t)(mempool_end - *mempool))
  {
    reader->err = ESP_ERR_NO_MEM;
    return NULL;
  }

  ptr = *mempool;
  *mempool += size;
  if (clear && size) memset(ptr, 0, size);
  return ptr;
}

void xm_load_pattern_stream(XmStreamReader *reader, xm_pattern_t *pat, uint16_t packed_size)
{
  for (uint16_t j = 0, k = 0; j < packed_size; k++)
  {
    uint8_t note = xm_stream_read_u8(reader);
    xm_pattern_slot_t *slot = pat->slots + k;
    j++;

    if (note & (1 << 7))
    {
      slot->note = (note & (1 << 0)) ? xm_stream_read_u8(reader) : 0;
      if (note & (1 << 0)) j++;
      slot->instrument = (note & (1 << 1)) ? xm_stream_read_u8(reader) : 0;
      if (note & (1 << 1)) j++;
      slot->volume_column = (note & (1 << 2)) ? xm_stream_read_u8(reader) : 0;
      if (note & (1 << 2)) j++;
      slot->effect_type = (note & (1 << 3)) ? xm_stream_read_u8(reader) : 0;
      if (note & (1 << 3)) j++;
      slot->effect_param = (note & (1 << 4)) ? xm_stream_read_u8(reader) : 0;
      if (note & (1 << 4)) j++;
    }
    else
    {
      slot->note = note;
      slot->instrument = xm_stream_read_u8(reader);
      slot->volume_column = xm_stream_read_u8(reader);
      slot->effect_type = xm_stream_read_u8(reader);
      slot->effect_param = xm_stream_read_u8(reader);
      j += 4;
    }
  }
}

void xm_load_sample_data_stream(XmStreamReader *reader, xm_sample_t *sample)
{
  uint8_t buffer[XM_STREAM_BUFFER_SIZE];

  if (!reader || !sample || !sample->data8) return;

  if (sample->bits == 16)
  {
    int16_t prev = 0;
    size_t pos = 0;

    while (pos < sample->length)
    {
      size_t todo = sample->length - pos;
      if (todo > XM_STREAM_BUFFER_SIZE / 2) todo = XM_STREAM_BUFFER_SIZE / 2;
      if (xm_stream_reader_read_exact(reader, buffer, todo * 2) != ESP_OK) return;

      for (size_t i = 0; i < todo; i++)
      {
        int16_t delta = (int16_t)xm_rd_le16(buffer + i * 2);
        prev = (int16_t)(prev + delta);
        sample->data16[pos + i] = prev;
      }

      pos += todo;
    }
  }
  else
  {
    int8_t prev = 0;
    size_t pos = 0;

    while (pos < sample->length)
    {
      size_t todo = sample->length - pos;
      if (todo > XM_STREAM_BUFFER_SIZE) todo = XM_STREAM_BUFFER_SIZE;
      if (xm_stream_reader_read_exact(reader, buffer, todo) != ESP_OK) return;

      for (size_t i = 0; i < todo; i++)
      {
        prev = (int8_t)(prev + (int8_t)buffer[i]);
        sample->data8[pos + i] = prev;
      }

      pos += todo;
    }
  }
}

char *xm_load_module_stream(xm_context_t *ctx, XmStreamReader *reader, const uint8_t *xm_header, char *mempool, char *mempool_end)
{
  uint8_t module_header[276] = {};
  xm_module_t *mod = &ctx->module;

  if (!ctx || !reader || !xm_header || !mempool) return mempool;

#if XM_STRINGS
  memcpy(mod->name, xm_header + 17, MODULE_NAME_LENGTH);
  memcpy(mod->trackername, xm_header + 38, TRACKER_NAME_LENGTH);
#endif

  esp_err_t err = xm_stream_reader_read_exact(reader, module_header, 4);
  if (err != ESP_OK) return mempool;

  uint32_t header_size = xm_rd_le32(module_header);
  if (header_size < 4)
  {
    reader->err = ESP_ERR_INVALID_SIZE;
    return mempool;
  }

  err = xm_stream_reader_read_record(reader, module_header, sizeof(module_header), header_size, 4);
  if (err != ESP_OK)
  {
    reader->err = err;
    return mempool;
  }

  mod->length = xm_rd_le16(module_header + 4);
  mod->restart_position = xm_rd_le16(module_header + 6);
  mod->num_channels = xm_rd_le16(module_header + 8);
  mod->num_patterns = xm_rd_le16(module_header + 10);
  mod->num_instruments = xm_rd_le16(module_header + 12);

  mod->patterns = (xm_pattern_t*)xm_stream_mempool_alloc(&mempool, mempool_end, mod->num_patterns * sizeof(xm_pattern_t), reader, true);
  if (!mod->patterns) return mempool;

  mod->instruments = (xm_instrument_t*)xm_stream_mempool_alloc(&mempool, mempool_end, mod->num_instruments * sizeof(xm_instrument_t), reader, true);
  if (!mod->instruments) return mempool;

  uint16_t flags = xm_rd_le16(module_header + 14);
  reader->module_header_size = header_size;
  reader->module_flags = flags;
  mod->frequency_type = (flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;
  ctx->tempo = xm_rd_le16(module_header + 16);
  ctx->bpm = xm_rd_le16(module_header + 18);
  memcpy(mod->pattern_table, module_header + 20, PATTERN_ORDER_TABLE_LENGTH);

  for (uint16_t i = 0; i < mod->num_patterns; i++)
  {
    uint8_t pattern_header[9] = {};
    err = xm_stream_reader_read_exact(reader, pattern_header, 4);
    if (err != ESP_OK) return mempool;

    uint32_t pattern_header_length = xm_rd_le32(pattern_header);
    if (pattern_header_length < 4)
    {
      reader->err = ESP_ERR_INVALID_SIZE;
      return mempool;
    }

    err = xm_stream_reader_read_record(reader, pattern_header, sizeof(pattern_header), pattern_header_length, 4);
    if (err != ESP_OK)
    {
      reader->err = err;
      return mempool;
    }

    xm_pattern_t *pat = mod->patterns + i;
    pat->num_rows = xm_rd_le16(pattern_header + 5);
    uint16_t packed_patterndata_size = xm_rd_le16(pattern_header + 7);
    pat->slots = (xm_pattern_slot_t*)xm_stream_mempool_alloc(&mempool, mempool_end, mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t), reader, true);
    if (!pat->slots) return mempool;

    if (packed_patterndata_size)
      xm_load_pattern_stream(reader, pat, packed_patterndata_size);
  }

  for (uint16_t i = 0; i < mod->num_instruments; i++)
  {
    uint8_t instrument_header[243] = {};
    uint32_t sample_header_size = 0;
    xm_instrument_t *instr = mod->instruments + i;

    err = xm_stream_reader_read_exact(reader, instrument_header, 4);
    if (err != ESP_OK) return mempool;

    uint32_t instrument_header_size = xm_rd_le32(instrument_header);
    if (instrument_header_size < 4)
    {
      reader->err = ESP_ERR_INVALID_SIZE;
      return mempool;
    }

    err = xm_stream_reader_read_record(reader, instrument_header, sizeof(instrument_header), instrument_header_size, 4);
    if (err != ESP_OK)
    {
      reader->err = err;
      return mempool;
    }

#if XM_STRINGS
    memcpy(instr->name, instrument_header + 4, INSTRUMENT_NAME_LENGTH);
#endif
    instr->num_samples = xm_rd_le16(instrument_header + 27);

    if (instr->num_samples > 0)
    {
      sample_header_size = xm_rd_le32(instrument_header + 29);
      memcpy(instr->sample_of_notes, instrument_header + 33, NUM_NOTES);
      instr->volume_envelope.num_points = instrument_header[225];
      instr->panning_envelope.num_points = instrument_header[226];

      for (uint8_t j = 0; j < instr->volume_envelope.num_points; j++)
      {
        instr->volume_envelope.points[j].frame = xm_rd_le16(instrument_header + 129 + 4 * j);
        instr->volume_envelope.points[j].value = xm_rd_le16(instrument_header + 129 + 4 * j + 2);
      }

      for (uint8_t j = 0; j < instr->panning_envelope.num_points; j++)
      {
        instr->panning_envelope.points[j].frame = xm_rd_le16(instrument_header + 177 + 4 * j);
        instr->panning_envelope.points[j].value = xm_rd_le16(instrument_header + 177 + 4 * j + 2);
      }

      instr->volume_envelope.sustain_point = instrument_header[227];
      instr->volume_envelope.loop_start_point = instrument_header[228];
      instr->volume_envelope.loop_end_point = instrument_header[229];
      instr->panning_envelope.sustain_point = instrument_header[230];
      instr->panning_envelope.loop_start_point = instrument_header[231];
      instr->panning_envelope.loop_end_point = instrument_header[232];

      uint8_t env_flags = instrument_header[233];
      instr->volume_envelope.enabled = env_flags & (1 << 0);
      instr->volume_envelope.sustain_enabled = env_flags & (1 << 1);
      instr->volume_envelope.loop_enabled = env_flags & (1 << 2);
      env_flags = instrument_header[234];
      instr->panning_envelope.enabled = env_flags & (1 << 0);
      instr->panning_envelope.sustain_enabled = env_flags & (1 << 1);
      instr->panning_envelope.loop_enabled = env_flags & (1 << 2);
      instr->vibrato_type = (xm_waveform_type_t)instrument_header[235];
      if (instr->vibrato_type == 2)
        instr->vibrato_type = XM_RAMP_DOWN_WAVEFORM;
      else if (instr->vibrato_type == 1)
        instr->vibrato_type = XM_SQUARE_WAVEFORM;
      instr->vibrato_sweep = instrument_header[236];
      instr->vibrato_depth = instrument_header[237];
      instr->vibrato_rate = instrument_header[238];
      instr->volume_fadeout = xm_rd_le16(instrument_header + 239);
      instr->samples = (xm_sample_t*)xm_stream_mempool_alloc(&mempool, mempool_end, instr->num_samples * sizeof(xm_sample_t), reader, true);
      if (!instr->samples) return mempool;
    }
    else
      instr->samples = NULL;

    for (uint16_t j = 0; j < instr->num_samples; j++)
    {
      uint8_t sample_header[40] = {};
      xm_sample_t *sample = instr->samples + j;

      if (sample_header_size < 4)
      {
        reader->err = ESP_ERR_INVALID_SIZE;
        return mempool;
      }

      err = xm_stream_reader_read_exact(reader, sample_header, sample_header_size < sizeof(sample_header) ? sample_header_size : sizeof(sample_header));
      if (err != ESP_OK) return mempool;
      if (sample_header_size > sizeof(sample_header))
      {
        err = xm_stream_reader_skip(reader, sample_header_size - sizeof(sample_header));
        if (err != ESP_OK) return mempool;
      }

      sample->length = xm_rd_le32(sample_header);
      sample->loop_start = xm_rd_le32(sample_header + 4);
      sample->loop_length = xm_rd_le32(sample_header + 8);
      sample->loop_end = sample->loop_start + sample->loop_length;
      sample->volume = (float)sample_header[12] / (float)0x40;
      sample->finetune = (int8_t)sample_header[13];

      uint8_t sample_flags = sample_header[14];
      if ((sample_flags & 3) == 0)
        sample->loop_type = XM_NO_LOOP;
      else if ((sample_flags & 3) == 1)
        sample->loop_type = XM_FORWARD_LOOP;
      else
        sample->loop_type = XM_PING_PONG_LOOP;

      sample->bits = (sample_flags & (1 << 4)) ? 16 : 8;
      sample->panning = (float)sample_header[15] / (float)0xFF;
      sample->relative_note = (int8_t)sample_header[16];
#if XM_STRINGS
      memcpy(sample->name, sample_header + 18, SAMPLE_NAME_LENGTH);
#endif
      sample->data8 = (int8_t*)xm_stream_mempool_alloc(&mempool, mempool_end, sample->length, reader, false);
      if (!sample->data8) return mempool;

      if (sample->bits == 16)
      {
        sample->loop_start >>= 1;
        sample->loop_length >>= 1;
        sample->loop_end >>= 1;
        sample->length >>= 1;
      }
    }

    for (uint16_t j = 0; j < instr->num_samples; j++)
      xm_load_sample_data_stream(reader, instr->samples + j);
  }

  return mempool;
}

int xm_create_context_safe_stream(xm_context_t **ctxp, XmStreamReader *reader, uint32_t rate, size_t *out_bytes_needed)
{
  size_t largest;
  size_t arena_size;
  size_t used_size;
  char *arena;
  char *mempool;
  char *mempool_end;
  xm_context_t *ctx;

  if (out_bytes_needed) *out_bytes_needed = 0;
  if (ctxp) *ctxp = NULL;
  if (!ctxp || !reader) return -1;

  esp_err_t err = xm_stream_reader_read_exact(reader, reader->header, sizeof(reader->header));
  if (err != ESP_OK) return -3;

#if XM_DEFENSIVE
  int ret = xm_check_sanity_preload((const char*)reader->header, sizeof(reader->header));
  if (ret) return -1;
#endif
  if (reader->err != ESP_OK) return -3;

  largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (largest <= XM_STREAM_ARENA_RESERVE) return -2;

  arena_size = largest - XM_STREAM_ARENA_RESERVE;
  if (arena_size < sizeof(xm_context_t)) return -2;
  if (arena_size > INT_MAX) arena_size = INT_MAX;

  arena = (char*)heap_caps_malloc(arena_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!arena) return -2;

  mempool = arena;
  mempool_end = arena + arena_size;
  ctx = (xm_context_t*)xm_stream_mempool_alloc(&mempool, mempool_end, sizeof(xm_context_t), reader, true);
  if (!ctx)
  {
    heap_caps_free(arena);
    return -2;
  }

  *ctxp = ctx;
  ctx->ctx_size = arena_size;
  ctx->rate = rate;

  mempool = xm_load_module_stream(ctx, reader, reader->header, mempool, mempool_end);
  if (reader->err != ESP_OK)
  {
    heap_caps_free(ctx);
    *ctxp = NULL;
    return reader->err == ESP_ERR_NO_MEM ? -2 : -3;
  }

  ctx->channels = (xm_channel_context_t*)xm_stream_mempool_alloc(&mempool, mempool_end, ctx->module.num_channels * sizeof(xm_channel_context_t), reader, true);
  if (!ctx->channels)
  {
    heap_caps_free(ctx);
    *ctxp = NULL;
    return -2;
  }

  ctx->global_volume = 1.f;
  ctx->amplification = .25f;

#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  for (uint8_t i = 0; i < ctx->module.num_channels; i++)
  {
    xm_channel_context_t *ch = ctx->channels + i;
    ch->ping = true;
    ch->vibrato_waveform = XM_SINE_WAVEFORM;
    ch->vibrato_waveform_retrigger = true;
    ch->tremolo_waveform = XM_SINE_WAVEFORM;
    ch->tremolo_waveform_retrigger = true;
    ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
    ch->panning = ch->panning_envelope_panning = .5f;
    ch->actual_volume = .0f;
    ch->actual_panning = .5f;
  }

  ctx->row_loop_count = (uint8_t*)xm_stream_mempool_alloc(&mempool, mempool_end, ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t), reader, true);
  if (!ctx->row_loop_count)
  {
    heap_caps_free(ctx);
    *ctxp = NULL;
    return -2;
  }

  used_size = (size_t)(mempool - (char*)ctx);
  ctx->ctx_size = used_size;
  if (out_bytes_needed) *out_bytes_needed = used_size;

#if XM_DEFENSIVE
  ret = xm_check_sanity_postload(ctx);
  if (ret)
  {
    heap_caps_free(ctx);
    *ctxp = NULL;
    return -1;
  }
#endif

  if (used_size < arena_size)
  {
    void *shrunk = heap_caps_realloc(ctx, used_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!shrunk)
    {
      heap_caps_free(ctx);
      *ctxp = NULL;
      return -2;
    }

    if (shrunk != ctx)
    {
      heap_caps_free(shrunk);
      *ctxp = NULL;
      return -4;
    }
  }

  *ctxp = ctx;
  return (int)used_size;
}

void xm_set_stream_info(int handle, const char *path, size_t file_size, xm_context_t *ctx, const XmStreamReader *reader)
{
  XM_INFO *info;

  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !ctx || !reader) return;

  info = &xm_info[handle];
  memset(info, 0, sizeof(*info));
  info->valid = 1;
  info->version = xm_rd_le16(reader->header + 58);
  info->header_size = reader->module_header_size;
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->flags = reader->module_flags;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;

  if (path)
  {
    strncpy(info->path, path, sizeof(info->path) - 1);
    info->path[sizeof(info->path) - 1] = 0;
  }

#if XM_STRINGS
  xm_copy_trimmed(info->module_name, sizeof(info->module_name), (const u8*)ctx->module.name, MODULE_NAME_LENGTH);
  xm_copy_trimmed(info->tracker_name, sizeof(info->tracker_name), (const u8*)ctx->module.trackername, TRACKER_NAME_LENGTH);
#endif
}

typedef struct
{
  SemaphoreHandle_t start_sem;
  SemaphoreHandle_t chunk_ready_sem;
  SemaphoreHandle_t chunk_done_sem;
  TaskHandle_t task;
  bool active;
  bool chunk_active;
  bool waiting_chunk;
  size_t total_size;
  size_t pos;
  size_t requested_size;
  const u8 *chunk_data;
  size_t chunk_size;
  size_t chunk_pos;
  int handle;
  u8 status;
  esp_err_t err;
  uint8_t header[60];
} XmHostStreamState;

XmHostStreamState g_xm_host_stream = {};

void xm_host_stream_task(void *arg);

void xm_host_stream_clear_semaphore(SemaphoreHandle_t sem)
{
  if (!sem) return;
  while (xSemaphoreTake(sem, 0) == pdTRUE) {}
}

esp_err_t xm_host_stream_ensure_runtime()
{
  if (!g_xm_host_stream.start_sem)
    g_xm_host_stream.start_sem = xSemaphoreCreateBinaryWithCaps(task_ram_type_non_critical);

  if (!g_xm_host_stream.chunk_ready_sem)
    g_xm_host_stream.chunk_ready_sem = xSemaphoreCreateBinaryWithCaps(task_ram_type_non_critical);

  if (!g_xm_host_stream.chunk_done_sem)
    g_xm_host_stream.chunk_done_sem = xSemaphoreCreateBinaryWithCaps(task_ram_type_non_critical);

  if (!g_xm_host_stream.start_sem || !g_xm_host_stream.chunk_ready_sem || !g_xm_host_stream.chunk_done_sem)
    return ESP_ERR_NO_MEM;

  if (!g_xm_host_stream.task)
  {
    BaseType_t ok = xTaskCreatePinnedToCoreWithCaps(
      xm_host_stream_task,
      "xm-stream",
      6144,
      NULL,
      XM_HELPER_TASK_PRIO,
      &g_xm_host_stream.task,
      0,
      task_ram_type_non_critical);

    if (ok != pdPASS)
    {
      g_xm_host_stream.task = NULL;
      return ESP_ERR_NO_MEM;
    }
  }

  return ESP_OK;
}

void xm_host_stream_release_chunk()
{
  if (!g_xm_host_stream.chunk_active) return;

  g_xm_host_stream.chunk_active = false;
  g_xm_host_stream.chunk_data = NULL;
  g_xm_host_stream.chunk_size = 0;
  g_xm_host_stream.chunk_pos = 0;
  xSemaphoreGive(g_xm_host_stream.chunk_done_sem);
}

esp_err_t xm_host_stream_request_chunk()
{
  size_t left;
  size_t req;

  if (!g_xm_host_stream.active) return ESP_ERR_INVALID_STATE;
  if (g_xm_host_stream.pos >= g_xm_host_stream.total_size) return ESP_ERR_INVALID_SIZE;

  left = g_xm_host_stream.total_size - g_xm_host_stream.pos;
  req = left > DMA_BUF_SIZE ? DMA_BUF_SIZE : left;
  if (!req) return ESP_ERR_INVALID_SIZE;

  g_xm_host_stream.requested_size = req;
  g_xm_host_stream.waiting_chunk = true;
  wr_reg32(ESP_REG_DATA_OFFSET, g_xm_host_stream.pos);
  wr_reg32(ESP_REG_DATA_SIZE, req);
  put_rxq(DREQ_XM_STREAM);

  if (xSemaphoreTake(g_xm_host_stream.chunk_ready_sem, portMAX_DELAY) != pdTRUE)
  {
    g_xm_host_stream.waiting_chunk = false;
    return ESP_ERR_TIMEOUT;
  }

  g_xm_host_stream.waiting_chunk = false;
  if (!g_xm_host_stream.chunk_active) return ESP_ERR_INVALID_STATE;
  if (g_xm_host_stream.chunk_size != req) return ESP_ERR_INVALID_SIZE;
  return ESP_OK;
}

esp_err_t xm_host_stream_need_chunk()
{
  if (g_xm_host_stream.chunk_active && g_xm_host_stream.chunk_pos < g_xm_host_stream.chunk_size)
    return ESP_OK;

  xm_host_stream_release_chunk();
  return xm_host_stream_request_chunk();
}

esp_err_t xm_host_stream_read_exact(void *dst, size_t size)
{
  uint8_t *out = (uint8_t*)dst;
  size_t done = 0;

  if (!dst && size) return ESP_ERR_INVALID_ARG;

  while (done < size)
  {
    esp_err_t err = xm_host_stream_need_chunk();
    if (err != ESP_OK) return err;

    size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
    size_t todo = size - done;
    if (todo > avail) todo = avail;

    if (todo && out)
      memcpy(out + done, g_xm_host_stream.chunk_data + g_xm_host_stream.chunk_pos, todo);

    g_xm_host_stream.chunk_pos += todo;
    g_xm_host_stream.pos += todo;
    done += todo;

    if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      xm_host_stream_release_chunk();
  }

  return ESP_OK;
}

esp_err_t xm_host_stream_skip(size_t size)
{
  size_t done = 0;

  while (done < size)
  {
    esp_err_t err = xm_host_stream_need_chunk();
    if (err != ESP_OK) return err;

    size_t avail = g_xm_host_stream.chunk_size - g_xm_host_stream.chunk_pos;
    size_t todo = size - done;
    if (todo > avail) todo = avail;

    g_xm_host_stream.chunk_pos += todo;
    g_xm_host_stream.pos += todo;
    done += todo;

    if (g_xm_host_stream.chunk_pos >= g_xm_host_stream.chunk_size)
      xm_host_stream_release_chunk();
  }

  return ESP_OK;
}

uint8_t xm_host_stream_read_u8()
{
  uint8_t v = 0;
  if (xm_host_stream_read_exact(&v, sizeof(v)) != ESP_OK)
    g_xm_host_stream.err = ESP_FAIL;
  return v;
}

esp_err_t xm_host_stream_read_record(uint8_t *dst, size_t dst_size, uint32_t rec_size, size_t already_read)
{
  size_t copy_size;

  if (!dst && dst_size) return ESP_ERR_INVALID_ARG;
  if (rec_size < already_read) return ESP_ERR_INVALID_SIZE;

  copy_size = rec_size - already_read;
  if (copy_size > dst_size - already_read)
    copy_size = dst_size - already_read;

  if (copy_size)
  {
    esp_err_t err = xm_host_stream_read_exact(dst + already_read, copy_size);
    if (err != ESP_OK) return err;
  }

  if (rec_size > already_read + copy_size)
    return xm_host_stream_skip(rec_size - already_read - copy_size);

  return ESP_OK;
}

void *xm_host_mempool_alloc(char **mempool, char *mempool_end, size_t size, bool clear)
{
  void *ptr;

  if (!mempool || !*mempool || !mempool_end) return NULL;
  if (*mempool > mempool_end)
  {
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  if (size > (size_t)(mempool_end - *mempool))
  {
    g_xm_host_stream.err = ESP_ERR_NO_MEM;
    return NULL;
  }

  ptr = *mempool;
  *mempool += size;
  if (clear && size) memset(ptr, 0, size);
  return ptr;
}

void xm_host_load_pattern_stream(xm_pattern_t *pat, uint16_t packed_size)
{
  for (uint16_t j = 0, k = 0; j < packed_size; k++)
  {
    uint8_t note = xm_host_stream_read_u8();
    xm_pattern_slot_t *slot = pat->slots + k;
    j++;

    if (note & (1 << 7))
    {
      slot->note = (note & (1 << 0)) ? xm_host_stream_read_u8() : 0;
      if (note & (1 << 0)) j++;
      slot->instrument = (note & (1 << 1)) ? xm_host_stream_read_u8() : 0;
      if (note & (1 << 1)) j++;
      slot->volume_column = (note & (1 << 2)) ? xm_host_stream_read_u8() : 0;
      if (note & (1 << 2)) j++;
      slot->effect_type = (note & (1 << 3)) ? xm_host_stream_read_u8() : 0;
      if (note & (1 << 3)) j++;
      slot->effect_param = (note & (1 << 4)) ? xm_host_stream_read_u8() : 0;
      if (note & (1 << 4)) j++;
    }
    else
    {
      slot->note = note;
      slot->instrument = xm_host_stream_read_u8();
      slot->volume_column = xm_host_stream_read_u8();
      slot->effect_type = xm_host_stream_read_u8();
      slot->effect_param = xm_host_stream_read_u8();
      j += 4;
    }
  }
}

void xm_host_load_sample_data_stream(xm_sample_t *sample)
{
  uint8_t buffer[XM_STREAM_BUFFER_SIZE];

  if (!sample || !sample->data8) return;

  if (sample->bits == 16)
  {
    int16_t prev = 0;
    size_t pos = 0;

    while (pos < sample->length)
    {
      size_t todo = sample->length - pos;
      if (todo > XM_STREAM_BUFFER_SIZE / 2) todo = XM_STREAM_BUFFER_SIZE / 2;
      if (xm_host_stream_read_exact(buffer, todo * 2) != ESP_OK) return;

      for (size_t i = 0; i < todo; i++)
      {
        int16_t delta = (int16_t)xm_rd_le16(buffer + i * 2);
        prev = (int16_t)(prev + delta);
        sample->data16[pos + i] = prev;
      }

      pos += todo;
    }
  }
  else
  {
    int8_t prev = 0;
    size_t pos = 0;

    while (pos < sample->length)
    {
      size_t todo = sample->length - pos;
      if (todo > XM_STREAM_BUFFER_SIZE) todo = XM_STREAM_BUFFER_SIZE;
      if (xm_host_stream_read_exact(buffer, todo) != ESP_OK) return;

      for (size_t i = 0; i < todo; i++)
      {
        prev = (int8_t)(prev + (int8_t)buffer[i]);
        sample->data8[pos + i] = prev;
      }

      pos += todo;
    }
  }
}

char *xm_host_load_module_stream(xm_context_t *ctx, const uint8_t *xm_header, char *mempool, char *mempool_end)
{
  uint8_t module_header[276] = {};
  xm_module_t *mod = &ctx->module;

#if XM_STRINGS
  memcpy(mod->name, xm_header + 17, MODULE_NAME_LENGTH);
  memcpy(mod->trackername, xm_header + 38, TRACKER_NAME_LENGTH);
#endif

  esp_err_t err = xm_host_stream_read_exact(module_header, 4);
  if (err != ESP_OK) return mempool;

  uint32_t header_size = xm_rd_le32(module_header);
  if (header_size < 4)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    return mempool;
  }

  err = xm_host_stream_read_record(module_header, sizeof(module_header), header_size, 4);
  if (err != ESP_OK)
  {
    g_xm_host_stream.err = err;
    return mempool;
  }

  mod->length = xm_rd_le16(module_header + 4);
  mod->restart_position = xm_rd_le16(module_header + 6);
  mod->num_channels = xm_rd_le16(module_header + 8);
  mod->num_patterns = xm_rd_le16(module_header + 10);
  mod->num_instruments = xm_rd_le16(module_header + 12);

  mod->patterns = (xm_pattern_t*)xm_host_mempool_alloc(&mempool, mempool_end, mod->num_patterns * sizeof(xm_pattern_t), true);
  if (!mod->patterns) return mempool;

  mod->instruments = (xm_instrument_t*)xm_host_mempool_alloc(&mempool, mempool_end, mod->num_instruments * sizeof(xm_instrument_t), true);
  if (!mod->instruments) return mempool;

  uint16_t flags = xm_rd_le16(module_header + 14);
  mod->frequency_type = (flags & (1 << 0)) ? XM_LINEAR_FREQUENCIES : XM_AMIGA_FREQUENCIES;
  ctx->tempo = xm_rd_le16(module_header + 16);
  ctx->bpm = xm_rd_le16(module_header + 18);
  memcpy(mod->pattern_table, module_header + 20, PATTERN_ORDER_TABLE_LENGTH);

  for (uint16_t i = 0; i < mod->num_patterns; i++)
  {
    uint8_t pattern_header[9] = {};
    err = xm_host_stream_read_exact(pattern_header, 4);
    if (err != ESP_OK) return mempool;

    uint32_t pattern_header_length = xm_rd_le32(pattern_header);
    if (pattern_header_length < 4)
    {
      g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
      return mempool;
    }

    err = xm_host_stream_read_record(pattern_header, sizeof(pattern_header), pattern_header_length, 4);
    if (err != ESP_OK)
    {
      g_xm_host_stream.err = err;
      return mempool;
    }

    xm_pattern_t *pat = mod->patterns + i;
    pat->num_rows = xm_rd_le16(pattern_header + 5);
    uint16_t packed_patterndata_size = xm_rd_le16(pattern_header + 7);
    pat->slots = (xm_pattern_slot_t*)xm_host_mempool_alloc(&mempool, mempool_end, mod->num_channels * pat->num_rows * sizeof(xm_pattern_slot_t), true);
    if (!pat->slots) return mempool;

    if (packed_patterndata_size)
      xm_host_load_pattern_stream(pat, packed_patterndata_size);
  }

  for (uint16_t i = 0; i < ctx->module.num_instruments; i++)
  {
    uint8_t instrument_header[243] = {};
    uint32_t sample_header_size = 0;
    xm_instrument_t *instr = mod->instruments + i;

    err = xm_host_stream_read_exact(instrument_header, 4);
    if (err != ESP_OK) return mempool;

    uint32_t instrument_header_size = xm_rd_le32(instrument_header);
    if (instrument_header_size < 4)
    {
      g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
      return mempool;
    }

    err = xm_host_stream_read_record(instrument_header, sizeof(instrument_header), instrument_header_size, 4);
    if (err != ESP_OK)
    {
      g_xm_host_stream.err = err;
      return mempool;
    }

#if XM_STRINGS
    memcpy(instr->name, instrument_header + 4, INSTRUMENT_NAME_LENGTH);
#endif
    instr->num_samples = xm_rd_le16(instrument_header + 27);

    if (instr->num_samples > 0)
    {
      sample_header_size = xm_rd_le32(instrument_header + 29);
      memcpy(instr->sample_of_notes, instrument_header + 33, NUM_NOTES);
      instr->volume_envelope.num_points = instrument_header[225];
      instr->panning_envelope.num_points = instrument_header[226];

      for (uint8_t j = 0; j < instr->volume_envelope.num_points; j++)
      {
        instr->volume_envelope.points[j].frame = xm_rd_le16(instrument_header + 129 + 4 * j);
        instr->volume_envelope.points[j].value = xm_rd_le16(instrument_header + 129 + 4 * j + 2);
      }

      for (uint8_t j = 0; j < instr->panning_envelope.num_points; j++)
      {
        instr->panning_envelope.points[j].frame = xm_rd_le16(instrument_header + 177 + 4 * j);
        instr->panning_envelope.points[j].value = xm_rd_le16(instrument_header + 177 + 4 * j + 2);
      }

      instr->volume_envelope.sustain_point = instrument_header[227];
      instr->volume_envelope.loop_start_point = instrument_header[228];
      instr->volume_envelope.loop_end_point = instrument_header[229];
      instr->panning_envelope.sustain_point = instrument_header[230];
      instr->panning_envelope.loop_start_point = instrument_header[231];
      instr->panning_envelope.loop_end_point = instrument_header[232];

      uint8_t env_flags = instrument_header[233];
      instr->volume_envelope.enabled = env_flags & (1 << 0);
      instr->volume_envelope.sustain_enabled = env_flags & (1 << 1);
      instr->volume_envelope.loop_enabled = env_flags & (1 << 2);
      env_flags = instrument_header[234];
      instr->panning_envelope.enabled = env_flags & (1 << 0);
      instr->panning_envelope.sustain_enabled = env_flags & (1 << 1);
      instr->panning_envelope.loop_enabled = env_flags & (1 << 2);
      instr->vibrato_type = (xm_waveform_type_t)instrument_header[235];
      if (instr->vibrato_type == 2)
        instr->vibrato_type = XM_RAMP_DOWN_WAVEFORM;
      else if (instr->vibrato_type == 1)
        instr->vibrato_type = XM_SQUARE_WAVEFORM;
      instr->vibrato_sweep = instrument_header[236];
      instr->vibrato_depth = instrument_header[237];
      instr->vibrato_rate = instrument_header[238];
      instr->volume_fadeout = xm_rd_le16(instrument_header + 239);
      instr->samples = (xm_sample_t*)xm_host_mempool_alloc(&mempool, mempool_end, instr->num_samples * sizeof(xm_sample_t), true);
      if (!instr->samples) return mempool;
    }
    else
      instr->samples = NULL;

    for (uint16_t j = 0; j < instr->num_samples; j++)
    {
      uint8_t sample_header[40] = {};
      xm_sample_t *sample = instr->samples + j;

      if (sample_header_size < 4)
      {
        g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
        return mempool;
      }

      err = xm_host_stream_read_exact(sample_header, sample_header_size < sizeof(sample_header) ? sample_header_size : sizeof(sample_header));
      if (err != ESP_OK) return mempool;
      if (sample_header_size > sizeof(sample_header))
      {
        err = xm_host_stream_skip(sample_header_size - sizeof(sample_header));
        if (err != ESP_OK) return mempool;
      }

      sample->length = xm_rd_le32(sample_header);
      sample->loop_start = xm_rd_le32(sample_header + 4);
      sample->loop_length = xm_rd_le32(sample_header + 8);
      sample->loop_end = sample->loop_start + sample->loop_length;
      sample->volume = (float)sample_header[12] / (float)0x40;
      sample->finetune = (int8_t)sample_header[13];

      uint8_t sample_flags = sample_header[14];
      if ((sample_flags & 3) == 0)
        sample->loop_type = XM_NO_LOOP;
      else if ((sample_flags & 3) == 1)
        sample->loop_type = XM_FORWARD_LOOP;
      else
        sample->loop_type = XM_PING_PONG_LOOP;

      sample->bits = (sample_flags & (1 << 4)) ? 16 : 8;
      sample->panning = (float)sample_header[15] / (float)0xFF;
      sample->relative_note = (int8_t)sample_header[16];
#if XM_STRINGS
      memcpy(sample->name, sample_header + 18, SAMPLE_NAME_LENGTH);
#endif
      sample->data8 = (int8_t*)xm_host_mempool_alloc(&mempool, mempool_end, sample->length, false);
      if (!sample->data8) return mempool;

      if (sample->bits == 16)
      {
        sample->loop_start >>= 1;
        sample->loop_length >>= 1;
        sample->loop_end >>= 1;
        sample->length >>= 1;
      }
    }

    for (uint16_t j = 0; j < instr->num_samples; j++)
      xm_host_load_sample_data_stream(instr->samples + j);
  }

  return mempool;
}

void xm_host_stream_set_info(int handle, xm_context_t *ctx, size_t file_size)
{
  XM_INFO *info;

  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !ctx) return;

  info = &xm_info[handle];
  memset(info, 0, sizeof(*info));
  info->valid = 1;
  info->version = xm_rd_le16(g_xm_host_stream.header + 58);
  info->song_length = ctx->module.length;
  info->restart_position = ctx->module.restart_position;
  info->num_channels = ctx->module.num_channels;
  info->num_patterns = ctx->module.num_patterns;
  info->num_instruments = ctx->module.num_instruments;
  info->tempo = ctx->tempo;
  info->bpm = ctx->bpm;
  info->file_size = (u32)file_size;
  strncpy(info->path, "host-stream", sizeof(info->path) - 1);
#if XM_STRINGS
  strncpy(info->module_name, ctx->module.name, sizeof(info->module_name) - 1);
  strncpy(info->tracker_name, ctx->module.trackername, sizeof(info->tracker_name) - 1);
#endif
}

int xm_host_stream_build_context(xm_context_t **out_ctx, size_t *out_used_size)
{
  size_t largest;
  size_t arena_size;
  size_t used_size;
  char *arena;
  char *mempool;
  char *mempool_end;
  xm_context_t *ctx;

  if (out_ctx) *out_ctx = NULL;
  if (out_used_size) *out_used_size = 0;
  if (!out_ctx || !out_used_size) return -1;

  esp_err_t err = xm_host_stream_read_exact(g_xm_host_stream.header, sizeof(g_xm_host_stream.header));
  if (err != ESP_OK) return -3;

#if XM_DEFENSIVE
  int ret = xm_check_sanity_preload((const char*)g_xm_host_stream.header, sizeof(g_xm_host_stream.header));
  if (ret) return -1;
#endif

  largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (largest <= XM_STREAM_ARENA_RESERVE) return -2;

  arena_size = largest - XM_STREAM_ARENA_RESERVE;
  if (arena_size < sizeof(xm_context_t)) return -2;
  if (arena_size > INT_MAX) arena_size = INT_MAX;

  arena = (char*)heap_caps_malloc(arena_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!arena) return -2;

  mempool = arena;
  mempool_end = arena + arena_size;
  ctx = (xm_context_t*)xm_host_mempool_alloc(&mempool, mempool_end, sizeof(xm_context_t), true);
  if (!ctx)
  {
    heap_caps_free(arena);
    return -2;
  }

  ctx->ctx_size = arena_size;
  ctx->rate = XM_SAMPLE_RATE;

  mempool = xm_host_load_module_stream(ctx, g_xm_host_stream.header, mempool, mempool_end);
  if (g_xm_host_stream.err != ESP_OK)
  {
    heap_caps_free(ctx);
    return g_xm_host_stream.err == ESP_ERR_NO_MEM ? -2 : -3;
  }

  ctx->channels = (xm_channel_context_t*)xm_host_mempool_alloc(&mempool, mempool_end, ctx->module.num_channels * sizeof(xm_channel_context_t), true);
  if (!ctx->channels)
  {
    heap_caps_free(ctx);
    return -2;
  }

  ctx->global_volume = 1.f;
  ctx->amplification = .25f;

#if XM_RAMPING
  ctx->volume_ramp = (1.f / 128.f);
  ctx->panning_ramp = (1.f / 128.f);
#endif

  for (uint8_t i = 0; i < ctx->module.num_channels; i++)
  {
    xm_channel_context_t *ch = ctx->channels + i;
    ch->ping = true;
    ch->vibrato_waveform = XM_SINE_WAVEFORM;
    ch->vibrato_waveform_retrigger = true;
    ch->tremolo_waveform = XM_SINE_WAVEFORM;
    ch->tremolo_waveform_retrigger = true;
    ch->volume = ch->volume_envelope_volume = ch->fadeout_volume = 1.0f;
    ch->panning = ch->panning_envelope_panning = .5f;
    ch->actual_volume = .0f;
    ch->actual_panning = .5f;
  }

  ctx->row_loop_count = (uint8_t*)xm_host_mempool_alloc(&mempool, mempool_end, ctx->module.length * MAX_NUM_ROWS * sizeof(uint8_t), true);
  if (!ctx->row_loop_count)
  {
    heap_caps_free(ctx);
    return -2;
  }

  used_size = (size_t)(mempool - (char*)ctx);
  ctx->ctx_size = used_size;

#if XM_DEFENSIVE
  ret = xm_check_sanity_postload(ctx);
  if (ret)
  {
    heap_caps_free(ctx);
    return -1;
  }
#endif

  if (used_size < arena_size)
  {
    void *shrunk = heap_caps_realloc(ctx, used_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!shrunk)
    {
      heap_caps_free(ctx);
      return -2;
    }

    if (shrunk != ctx)
    {
      heap_caps_free(shrunk);
      return -4;
    }
  }

  *out_ctx = ctx;
  *out_used_size = used_size;
  return (int)used_size;
}

void xm_host_stream_task(void *arg)
{
  while (1)
  {
    xSemaphoreTake(g_xm_host_stream.start_sem, portMAX_DELAY);

    xm_context_t *ctx = NULL;
    size_t used_size = 0;
    int rc = xm_host_stream_build_context(&ctx, &used_size);

    if (rc < 0)
    {
      xm_host_stream_release_chunk();
      g_xm_host_stream.active = false;
      if (rc == -2)
        set_status(ESP_ERR_OUT_OF_MEMORY);
      else if (rc == -4)
        set_status(ESP_ERR_INV_STATE);
      else
        set_status(ESP_ERR_INV_XM);
      continue;
    }

    int handle = attach_obj(ctx, (int)used_size, OBJ_TYPE_XMC);
    if (handle < 0)
    {
      xm_free_context(ctx);
      xm_host_stream_release_chunk();
      g_xm_host_stream.active = false;
      continue;
    }

    mem_obj[handle].state = XM_OBJ_ST_STOPPED;
    xm_set_max_loop_count(ctx, 0);
    xm_host_stream_set_info(handle, ctx, g_xm_host_stream.total_size);
    wr_reg8(ESP_REG_OBJ_HANDLE, handle);
    wr_reg32(ESP_REG_DATA_SIZE, used_size);
    g_xm_host_stream.handle = handle;
    g_xm_host_stream.active = false;
    set_status(ESP_ST_READY);
  }
}

esp_err_t xm_host_stream_start(size_t module_size)
{
  if (!module_size || module_size > INT_MAX) return ESP_ERR_INVALID_SIZE;

  esp_err_t err = xm_host_stream_ensure_runtime();
  if (err != ESP_OK) return err;
  if (g_xm_host_stream.active) return ESP_ERR_INVALID_STATE;

  xm_host_stream_clear_semaphore(g_xm_host_stream.start_sem);
  xm_host_stream_clear_semaphore(g_xm_host_stream.chunk_ready_sem);
  xm_host_stream_clear_semaphore(g_xm_host_stream.chunk_done_sem);

  g_xm_host_stream.active = true;
  g_xm_host_stream.chunk_active = false;
  g_xm_host_stream.waiting_chunk = false;
  g_xm_host_stream.total_size = module_size;
  g_xm_host_stream.pos = 0;
  g_xm_host_stream.requested_size = 0;
  g_xm_host_stream.chunk_data = NULL;
  g_xm_host_stream.chunk_size = 0;
  g_xm_host_stream.chunk_pos = 0;
  g_xm_host_stream.handle = -1;
  g_xm_host_stream.status = ESP_ST_BUSY;
  g_xm_host_stream.err = ESP_OK;
  memset(g_xm_host_stream.header, 0, sizeof(g_xm_host_stream.header));

  xSemaphoreGive(g_xm_host_stream.start_sem);
  return ESP_OK;
}

void xm_host_stream_process_rx_data(const u8 *data, size_t size)
{
  if (!g_xm_host_stream.active || !g_xm_host_stream.waiting_chunk || !data || !size)
  {
    set_status(ESP_ERR_INV_STATE);
    return;
  }

  if (size != g_xm_host_stream.requested_size)
  {
    g_xm_host_stream.err = ESP_ERR_INVALID_SIZE;
    g_xm_host_stream.waiting_chunk = false;
    g_xm_host_stream.chunk_active = false;
    xSemaphoreGive(g_xm_host_stream.chunk_ready_sem);
    set_status(ESP_ERR_INV_SIZE);
    return;
  }

  g_xm_host_stream.chunk_data = data;
  g_xm_host_stream.chunk_size = size;
  g_xm_host_stream.chunk_pos = 0;
  g_xm_host_stream.chunk_active = true;

  xSemaphoreGive(g_xm_host_stream.chunk_ready_sem);
  xSemaphoreTake(g_xm_host_stream.chunk_done_sem, portMAX_DELAY);
}

int xm_load_xm_stream_file_to_handle(const char *path, size_t size, int *out_handle, bool quiet)
{
  XmStreamReader reader;
  xm_context_t *ctx = NULL;
  size_t bytes_needed = 0;
  int rc;
  int handle;

  if (out_handle) *out_handle = -1;
  if (!path || !path[0]) return 1;

  esp_err_t err = xm_stream_reader_open(&reader, path, size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: XM stream open failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  rc = xm_create_context_safe_stream(&ctx, &reader, XM_SAMPLE_RATE, &bytes_needed);
  if (rc < 0)
  {
    if (!quiet)
    {
      if (rc == -1)
        printf("E: invalid XM stream\r\n");
      else if (rc == -2)
      {
        size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        printf("E: XM stream context alloc failed: need=%u free=%u largest=%u\r\n",
          (unsigned)bytes_needed,
          (unsigned)free_size,
          (unsigned)largest);
      }
      else if (rc == -4)
        printf("E: XM stream shrink moved context pointer\r\n");
      else
        printf("E: XM stream read failed: %s\r\n", esp_err_to_name(reader.err));
    }
    if (ctx) xm_free_context(ctx);
    reader.close(&reader);
    return 1;
  }

  handle = attach_obj(ctx, rc, OBJ_TYPE_XMC);
  if (handle < 0)
  {
    xm_free_context(ctx);
    reader.close(&reader);
    return 1;
  }

  mem_obj[handle].state = XM_OBJ_ST_STOPPED;
  xm_set_max_loop_count(ctx, 0);
  xm_set_stream_info(handle, path, size, ctx, &reader);
  reader.close(&reader);

  if (!quiet)
  {
    printf("XM stream loaded: handle=%02X size=%u ctx=%u\r\n",
      handle,
      (unsigned)size,
      (unsigned)mem_obj[handle].size);
    if (xm_info[handle].module_name[0])
      printf("  Module : %s\r\n", xm_info[handle].module_name);
    if (xm_info[handle].tracker_name[0])
      printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);
  }

  if (out_handle) *out_handle = handle;
  return 0;
}

int xm_load_compressed_file_to_handle(const char *path, size_t size, xm_file_format_t format, int *out_handle, bool quiet)
{
  esp_err_t err;
  u8 *zip_data;
  u8 *out_data;
  u32 unpacked_size;
  size_t inbytes;
  size_t outbytes;
  tinfl_status decomp_status;
  int handle;

  if (size <= sizeof(u32) || size > INT_MAX)
  {
    if (!quiet) printf("E: invalid %s size: %u bytes\r\n", xm_file_format_str(format), (unsigned)size);
    return 1;
  }

  zip_data = (u8*)malloc_spiram(size);
  if (!zip_data)
  {
    if (!quiet) printf("E: %s alloc failed: %u bytes\r\n", xm_file_format_str(format), (unsigned)size);
    return 1;
  }

  err = xm_read_file_data(path, zip_data, size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s read failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    free(zip_data);
    return 1;
  }

  unpacked_size = xm_rd_le32(zip_data);
  if (unpacked_size == 0 || unpacked_size > INT_MAX)
  {
    if (!quiet) printf("E: invalid %s unpacked size: %u bytes\r\n", xm_file_format_str(format), (unsigned)unpacked_size);
    free(zip_data);
    return 1;
  }

  handle = make_obj((int)unpacked_size, OBJ_TYPE_XM);
  if (handle < 0)
  {
    free(zip_data);
    return 1;
  }

  out_data = (u8*)mem_obj[handle].addr;
  tinfl_init(decomp);

  inbytes = size - sizeof(u32);
  outbytes = unpacked_size;
  decomp_status = tinfl_decompress(decomp,
    zip_data + sizeof(u32),
    &inbytes,
    out_data,
    out_data,
    &outbytes,
    TINFL_FLAG_PARSE_ZLIB_HEADER | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

  free(zip_data);

  if (decomp_status != TINFL_STATUS_DONE || outbytes != unpacked_size)
  {
    if (!quiet)
    {
      printf("E: %s unzip failed: status=%d out=%u/%u\r\n",
        xm_file_format_str(format),
        (int)decomp_status,
        (unsigned)outbytes,
        (unsigned)unpacked_size);
    }
    delete_obj(handle);
    return 1;
  }

  if (!xm_parse_info(handle, path, (const u8*)mem_obj[handle].addr, unpacked_size))
  {
    if (!quiet) printf("E: invalid XM header after unzip\r\n");
    delete_obj(handle);
    return 1;
  }

  if (xm_init_handle(handle, quiet)) return 1;

  if (out_handle) *out_handle = handle;
  return 0;
}

int xm_load_file_to_handle(const char *path, int *out_handle, bool quiet)
{
  size_t size = 0;
  esp_err_t err;
  xm_file_format_t format;
  int handle = -1;

  if (out_handle) *out_handle = -1;

  if (!path || !path[0])
  {
    if (!quiet) printf("Usage: xm load <path>\r\n");
    return 1;
  }

  const char *ext = strrchr(path, '.');
  format = (ext && !strcasecmp(ext, ".xmz")) ? XM_FILE_FORMAT_XMZ : XM_FILE_FORMAT_XM;

  err = xm_get_file_size(path, &size);
  if (err != ESP_OK)
  {
    if (!quiet) printf("E: %s size failed: %s\r\n", xm_file_format_str(format), esp_err_to_name(err));
    return 1;
  }

  if (size == 0 || size > INT_MAX)
  {
    if (!quiet) printf("E: %s file too large: %u bytes\r\n", xm_file_format_str(format), (unsigned)size);
    return 1;
  }

  if (format == XM_FILE_FORMAT_XM)
  {
    if (xm_load_xm_stream_file_to_handle(path, size, &handle, quiet)) return 1;
    if (out_handle) *out_handle = handle;
    return 0;
  }

  if (xm_load_compressed_file_to_handle(path, size, format, &handle, quiet)) return 1;

  if (!quiet)
  {
    printf("%s loaded: handle=%02X  size=%u\r\n",
      xm_file_format_str(format),
      handle,
      (unsigned)mem_obj[handle].size);
    if (xm_info[handle].module_name[0])
      printf("  Module : %s\r\n", xm_info[handle].module_name);
    if (xm_info[handle].tracker_name[0])
      printf("  Tracker: %s\r\n", xm_info[handle].tracker_name);
  }

  if (out_handle) *out_handle = handle;
  return 0;
}

int xm_load_file(const char *path)
{
  int handle = -1;
  return xm_load_file_to_handle(path, &handle, false);
}

int xm_load_play_file(const char *path, bool quiet)
{
  int handle = -1;

  if (xm_delete_all_modules(quiet)) return 1;
  if (xm_load_file_to_handle(path, &handle, quiet)) return 1;
  return xm_play_cmd(handle, quiet);
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
  {
    for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    {
      if (mem_obj[i].addr && (mem_obj[i].type == OBJ_TYPE_XM || mem_obj[i].type == OBJ_TYPE_XMC))
      {
        handle = i;
        break;
      }
    }
  }

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

int xm_play_cmd(int handle, bool quiet)
{
  if (handle < 0 || handle >= OBJ_HANDLES_MAX || !mem_obj[handle].addr)
  {
    if (!quiet) printf("Bad <handle>: %d\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].type != OBJ_TYPE_XMC)
  {
    if (!quiet) printf("Handle %02X is not an initialized XMC object\r\n", handle);
    return 1;
  }

  if (mem_obj[handle].state == XM_OBJ_ST_PLAYING)
  {
    if (xm_stop_current_playback(quiet)) return 1;
  }
  else
  {
    int playing = xm_find_playing_handle();
    if (playing >= 0 && playing != handle)
      if (xm_stop_current_playback(quiet)) return 1;
  }

  XM_TASK task = {};
  task.task = XM_TASK_PLAY;
  task.handle = handle;
  xQueueSend(xm_queue, &task, portMAX_DELAY);

  if (!xm_wait_for_state(handle, XM_OBJ_ST_PLAYING, 2000))
  {
    if (!quiet) printf("XM play failed, status=%02X\r\n", rd_reg8(ESP_REG_STATUS));
    return 1;
  }

  if (!quiet) printf("XM playing: %02X\r\n", handle);
  return 0;
}

int xm_stop_cmd(bool quiet)
{
  int handle = xm_find_playing_handle();
  if (handle < 0)
  {
    if (!quiet) printf("XM is not playing\r\n");
    return 0;
  }

  if (xm_stop_current_playback(quiet)) return 1;

  if (!quiet) printf("XM stopped: %02X\r\n", handle);
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
    if (xm_stop_current_playback(false)) return 1;

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
    printf("  xm load <file.xm|xmz>\r\n");
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
