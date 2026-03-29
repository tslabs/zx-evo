
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include <esp_crc.h>
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "sdkconfig.h"
#include <esp_heap_caps.h>
#include "esp_timer.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "spi_slave.h"
#include "stats.h"
#include "ft8xx.h"
#include "usb_mouse.h"
#include "ps2_mouse.h"

using namespace stats;

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO    1
#endif

const char TAG[] = "zf32_system";

int perf_test(int argc, char **argv)
{
#define _START(a) gettimeofday(&start, NULL); printf(a ": ");
#define _END      gettimeofday(&end, NULL); elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0; \
                  printf("%.2fk iter/s\n", iters / elapsed_ms);
#define _END1     gettimeofday(&end, NULL); elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0; \
                  printf("%.2f MB/s\n", TEST_BUF_SIZE / elapsed_ms / 1000);
#define _END2     gettimeofday(&end, NULL); elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0; \
                  printf("%.2f iter/s\n", iters * 1000 / elapsed_ms);

 struct timeval start, end;
 double elapsed_ms;
 float sumf = 0;
 int sum = 0;

#define TEST_BUF_SIZE   2 * 1024 * 1024

  size_t iters;
  auto *buffer = heap_caps_malloc(TEST_BUF_SIZE, MALLOC_CAP_SPIRAM);

  if (!buffer)
  {
    printf("PSRAM allocation failed\n");
    return 1;
  }

  printf("\n");
  printf("Aligned PSRAM access\n");

  _START("Fill 32-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(int); i++)
    ((int*)buffer)[i] = 0x01234567;
  _END1

  _START("Fill 16-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint16_t); i++)
    ((uint16_t*)buffer)[i] = 0x89AB;
  _END1

  _START("Fill  8-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint8_t); i++)
    ((uint8_t*)buffer)[i] = 0x55;
  _END1

  _START("Read 32-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(int); i++)
    ((volatile int*)buffer)[i];
  _END1

  _START("Read 16-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint16_t); i++)
    ((volatile uint16_t*)buffer)[i];
  _END1

  _START("Read  8-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint8_t); i++)
    ((volatile uint8_t*)buffer)[i];
  _END1

  printf("\n");
  printf("Unaligned PSRAM access\n");

  _START("Fill 32-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(int); i++)
    ((int*)buffer)[((i << 9) & ((TEST_BUF_SIZE / sizeof(int)) - 1)) | (i >> 8)] = 0x01234567;
  _END1

  _START("Fill 16-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint16_t); i++)
    ((uint16_t*)buffer)[((i << 10) & ((TEST_BUF_SIZE / sizeof(uint16_t)) - 1)) | (i >> 9)] = (uint16_t)0x01234567;
  _END1

  _START("Fill  8-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint8_t); i++)
    ((uint8_t*)buffer)[((i << 11) & (TEST_BUF_SIZE - 1)) | (i >> 10)] = 0x55;
  _END1

  _START("Read 32-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(int); i++)
    ((volatile int*)buffer)[((i << 9) & ((TEST_BUF_SIZE / sizeof(int)) - 1)) | (i >> 8)];
  _END1

  _START("Read 16-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint16_t); i++)
    ((volatile uint16_t*)buffer)[((i << 10) & ((TEST_BUF_SIZE / sizeof(uint16_t)) - 1)) | (i >> 9)];
  _END1

  _START("Read  8-bit")
  for (size_t i = 0; i < TEST_BUF_SIZE / sizeof(uint8_t); i++)
    ((volatile uint8_t*)buffer)[((i << 11) & (TEST_BUF_SIZE - 1)) | (i >> 10)];
  _END1

  printf("\n");
  printf("Math operations\n");

  iters = 100000;

  srand(esp_timer_get_time());
  _START("Rnd")
  for (size_t i = 0; i < iters; i++)
    sumf += rand();
  _END

  iters = 1000000;

  uint32_t rng_state = 123456789;
  _START("Fast rnd")
  uint32_t x = rng_state;
  for (size_t i = 0; i < iters; i++)
  {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    volatile auto a = x; a;
  }
  rng_state = x;
  _END

  iters = 100000;

  _START("Sqrt FP")
  for (size_t i = 0; i < iters; i++)
    sumf += sqrt((float)i);
  _END

  _START("Log FP")
  for (size_t i = 0; i < iters; i++)
    sumf += logf((float)i);
  _END

  _START("Sin FP")
  for (size_t i = 0; i < iters; i++)
    sumf += sinf((float)i);
  _END

#define WIDTH 800
#define HEIGHT 600
iters = 100;

  printf("\n");
  printf("Fractals %ux%u\n", WIDTH, HEIGHT);

  _START("Mandelbrot")
  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      float cr = -2.0f + (float)x * 3.0f / WIDTH;
      float ci = -1.5f + (float)y * 3.0f / HEIGHT;
      float zr = 0.0f, zi = 0.0f;
      int iter = 0;

      while (zr * zr + zi * zi <= 4.0f && iter < iters)
      {
        float zr_new = zr * zr - zi * zi + cr;
        zi = 2.0f * zr * zi + ci;
        zr = zr_new;
        iter++;
      }

      volatile float s = (float)iter / iters; s;
    }
  }
  _END2

  _START("Julia")
  float cr = -0.4f, ci = 0.6f;

  for (int y = 0; y < HEIGHT; y++)
  {
    for (int x = 0; x < WIDTH; x++)
    {
      float zr = -2.0f + (float)x * 3.0f / WIDTH;
      float zi = -1.5f + (float)y * 3.0f / HEIGHT;
      int iter = 0;

      while (zr * zr + zi * zi <= 4.0f && iter < iters)
      {
        float zr_new = zr * zr - zi * zi + cr;
        zi = 2.0f * zr * zi + ci;
        zr = zr_new;
        iter++;
      }

      volatile float s = (float)iter / iters; s;
    }
  }
  _END2

#undef WIDTH
#undef HEIGHT

  printf("\n");

  heap_caps_free(buffer);

  volatile auto s1 = sumf; s1;  // prevent optimization
  volatile auto s2 = sum; s2;   // prevent optimization

  return 0;

#undef _START
#undef _END
#undef _END1
#undef _END2
}

void hexdump(const void *data, size_t len, uint64_t base_off)
{
  const uint8_t *p = (const uint8_t *)data;

  for (size_t i = 0; i < len; i += 16)
  {
    printf("%08" PRIx64 "  ", (uint64_t)(base_off + i));

    for (size_t j = 0; j < 16; j++)
    {
      if (i + j < len) printf("%02X ", p[i + j]);
      else printf("   ");
    }

    printf(" ");

    for (size_t j = 0; j < 16; j++)
    {
      if (i + j < len)
      {
        unsigned char c = p[i + j];
        printf("%c", isprint(c) ? c : '.');
      }
    }

    printf("\r\n");
  }
}

const char *esp_status_str(uint8_t st)
{
  switch (st)
  {
    // Status codes
    case ESP_ST_IDLE:     return "Idle";
    case ESP_ST_READY:    return "Ready";
    case ESP_ST_BUSY:     return "Busy";
    case ESP_ST_DATA_M2S: return "Data Master->Slave";
    case ESP_ST_DATA_S2M: return "Data Slave->Master";
    case ESP_ST_RESET:    return "Reset performed";

    // Error codes
    case ESP_ERR_INV_COMMAND:      return "Invalid command";
    case ESP_ERR_INV_PARAM:        return "Invalid parameter";
    case ESP_ERR_INV_STATE:        return "Invalid state";
    case ESP_ERR_INV_STR_LEN:      return "Invalid string length";
    case ESP_ERR_INV_SIZE:         return "Invalid size";
    case ESP_ERR_INV_OBJ_TYPE:     return "Invalid object type";
    case ESP_ERR_INV_ARR_NUM:      return "Invalid array number";
    case ESP_ERR_INV_HANDLE:       return "Invalid handle";
    case ESP_ERR_INV_XM_HANDLE:    return "Invalid XM handle";
    case ESP_ERR_INV_LIB_HANDLE:   return "Invalid LIB handle";
    case ESP_ERR_INV_ELF_HANDLE:   return "Invalid ELF handle";
    case ESP_ERR_INV_HST_HANDLE:   return "Invalid HST handle";
    case ESP_ERR_INV_BSS_HANDLE:   return "Invalid BSS handle";
    case ESP_ERR_INV_ARG_HANDLE:   return "Invalid ARG handle";
    case ESP_ERR_INV_SRC_HANDLE:   return "Invalid source handle";
    case ESP_ERR_INV_DST_HANDLE:   return "Invalid destination handle";

    case ESP_ERR_INV_XM:           return "Invalid XM";
    case ESP_ERR_INV_LIB:          return "Invalid LIB";
    case ESP_ERR_INV_ELF:          return "Invalid ELF";
    case ESP_ERR_INV_HST:          return "Invalid HST";
    case ESP_ERR_INV_ZIP:          return "Invalid ZIP";

    case ESP_ERR_OUT_OF_MEMORY:    return "Out of memory";
    case ESP_ERR_OUT_OF_HANDLES:   return "Out of handles";
    case ESP_ERR_OBJ_NOT_DELETED:  return "Object not deleted";

    case ESP_ERR_AP_NOT_CONNECTED: return "AP not connected";
    case ESP_ERR_NET_BUSY:         return "Network busy";

    default:                       return "";
  }
}

int error_list(int argc, char **argv)
{
  u8 st = rd_reg8(ESP_REG_STATUS);

  for (int i = 0; i < 256; i++)
  {
    auto s = esp_status_str(i);
    if (s[0]) printf("%02X - %s\r\n", i, s);
  }

  printf("\r\n");
  printf("Current status: %02X (%s)\r\n", st, esp_status_str(st));

  return 0;
}

int get_info(int argc, char **argv)
{
  const char      *model;
  esp_chip_info_t info;
  uint32_t        flash_size;

  esp_chip_info(&info);
  model = "Unknown";

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  model = "ESP32-S3";
#endif

#if defined(CONFIG_IDF_TARGET_ESP32C6)
  model = "ESP32-C6";
#endif

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  model = "ESP32-P4";
#endif

  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    printf("Get flash size failed");
    flash_size = 0;
  }

  printf("\r\n%s\r\n", CP_STRING);
  printf("Build: " __DATE__ ", " __TIME__ "\r\n");
  printf("\r\n");

  printf("Chip info:\r\n");
  printf("\tModel: %s (model ID %d)\r\n", model, info.model);
  printf("\tSilicon revision: %d\r\n", info.revision);
  printf("\tFrequency: %dMHz\r\n", esp_clk_cpu_freq() / 1000000);
  printf("\tCores: %d\r\n", info.cores);

  printf("\tFeatures:\r\n");
  if (info.features & CHIP_FEATURE_WIFI_BGN) printf("\t  WiFi 802.11bgn\r\n");
  if (info.features & CHIP_FEATURE_BLE) printf("\t  BLE\r\n");
  if (info.features & CHIP_FEATURE_BT) printf("\t  BT\r\n");
  if (info.features & CHIP_FEATURE_IEEE802154) printf("\t  IEEE 802.15.4\r\n");

  printf("\t%s %luMB\r\n",
         info.features & CHIP_FEATURE_EMB_FLASH ? "Embedded-Flash: " : "External-Flash: ",
         flash_size / (1024 * 1024));
  printf("\r\n");

  printf("IDF Version: %s\r\n", esp_get_idf_version());
  printf("\r\n");

  return 0;
}

int restart(int argc, char **argv)
{
  ESP_LOGI(TAG, "Restarting");
  esp_restart();
}

int mem_info(int argc, char **argv)
{
  printf("Heap free/total:\n");
  printf(" Default:\t%u/%u\n",    heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
  printf(" 32-bit:\t%u/%u\n",     heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_total_size(MALLOC_CAP_32BIT));
  printf(" 8-bit:\t\t%u/%u\n",    heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_total_size(MALLOC_CAP_8BIT));
  printf(" SPI RAM:\t%u/%u\n",    heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
  printf(" SRAM:\t\t%u/%u\n",     heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
  printf(" DMA:\t\t%u/%u\n",      heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_total_size(MALLOC_CAP_DMA));
  printf(" RTC fast:\t%u/%u\n",   heap_caps_get_free_size(MALLOC_CAP_RTCRAM), heap_caps_get_total_size(MALLOC_CAP_RTCRAM));
  printf(" Retention:\t%u/%u\n",  heap_caps_get_free_size(MALLOC_CAP_RETENTION), heap_caps_get_total_size(MALLOC_CAP_RETENTION));
  printf(" IRAM 8-bit:\t%u/%u\n", heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT), heap_caps_get_total_size(MALLOC_CAP_IRAM_8BIT));
  printf(" Exec:\t\t%u/%u\n",     heap_caps_get_free_size(MALLOC_CAP_EXEC), heap_caps_get_total_size(MALLOC_CAP_EXEC));

  printf("\nMin heap:\t%u\n", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));

  return 0;
}

const char *obj_type_str(uint8_t t)
{
  switch (t)
  {
    case OBJ_TYPE_NONE:  return "None";
    case OBJ_TYPE_DATA:  return "Data";
    case OBJ_TYPE_DATAF: return "Data SRAM";
    case OBJ_TYPE_ELF:   return "ELF";
    case OBJ_TYPE_LIB:   return "LIB";
    case OBJ_TYPE_XM:    return "XM";
    case OBJ_TYPE_WAV:   return "WAV";
    case OBJ_TYPE_HST:   return "HST";
    case OBJ_TYPE_ZIP:   return "ZIP";
    case OBJ_TYPE_XMC:   return "XM ctx";
    default:             return "Unknown";
  }
}

const char *obj_state_str(uint8_t st)
{
  switch (st)
  {
    case OBJ_ST_NONE:       return "None";
    case OBJ_ST_ERROR:      return "Error";
    case LIB_OBJ_ST_READY:  return "Ready";
    case XM_OBJ_ST_STOPPED: return "Stopped";
    case XM_OBJ_ST_PLAYING: return "Playing";
    default:                return "Unknown";
  }
}

int obj_info(int argc, char **argv)
{
  printf("\r\nMemory objects\r\n");

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (mem_obj[i].addr)
    {
      const uint8_t t = mem_obj[i].type;
      const uint8_t s = mem_obj[i].state;

      switch (mem_obj[i].type)
      {
        case OBJ_TYPE_LIB:
          printf
          (
            "ID: %02X  Type: %02X (%s)  State: %02X (%s)  Size: %u  ",
            (unsigned)i,
            (unsigned)t, obj_type_str(t),
            (unsigned)s, obj_state_str(s),
            (unsigned)mem_obj[i].size
          );
          printf("Entry: %08X  text: %08X  data: %08X  ro: %08X  bss: %08X\r\n", (unsigned int)mem_obj[i].entry, (unsigned int)mem_obj[i].text, (unsigned int)mem_obj[i].data, (unsigned int)mem_obj[i].rodata, (unsigned int)mem_obj[i].bss);
        break;

        default:
          printf
          (
            "ID: %02X  Type: %02X (%s)  State: %02X (%s)  Addr: %08X  Size: %u\r\n",
            (unsigned)i,
            (unsigned)t, obj_type_str(t),
            (unsigned)s, obj_state_str(s),
            (unsigned int)mem_obj[i].addr,
            (unsigned)mem_obj[i].size
          );
        break;
      }
    }

  printf("\r\n");

  return 0;
}

#if WITH_TASKS_INFO
int tasks_info(int argc, char **argv)
{
  const size_t bytes_per_task = 40; /* see vTaskList description */
  char *task_list_buffer = (char*)malloc(uxTaskGetNumberOfTasks() * bytes_per_task);

  if (task_list_buffer == NULL)
  {
    ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
    return 1;
  }

  printf("Task Name\tStatus\tPrio\tHWM*\tTask#");
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
  printf("\tAffinity");
#endif
  printf("\r\n");
  vTaskList(task_list_buffer);
  printf(task_list_buffer);
  printf("*HWM = Free stack bytes\r\n\r\n");

  free(task_list_buffer);

  return 0;
}
#endif // WITH_TASKS_INFO

int stats_info(int argc, char **argv)
{
#if CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER
  vTaskGetRunTimeStats(_st.runtime_stats_buffer);
  printf("\r\n");
  printf("Task runtime statistics:\n%s", _st.runtime_stats_buffer);
#endif

  printf("\r\ndrq_data_start, us\tdrq_data_end, us\txm_render, us   \txm_render, %%cpu\r\n");
  printf("min\tlast\tmax\tmin\tlast\tmax\tmin\tcurr\tmax\tmin\tcurr\tmax\r\n");
  // printf("xm_samp_min: %f   \r\n", _st.xm_samp_min);
  // printf("xm_samp_max: %f   \r\n", _st.xm_samp_max);

  while (1)
  {
    printf("%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t%d \t\r",
      _st.drq_data_start_min_us == INT_MAX ? 0 : _st.drq_data_start_min_us,
      _st.drq_data_start_last_us,
      _st.drq_data_start_max_us,
      _st.drq_data_end_min_us == INT_MAX ? 0 : _st.drq_data_end_min_us,
      _st.drq_data_end_last_us,
      _st.drq_data_end_max_us,
      _st.xm_render_min_us == INT_MAX ? 0 : _st.xm_render_min_us,
      _st.xm_render_last_us,
      _st.xm_render_max_us,
      _st.xm_render_min_cpu == INT_MAX ? 0 : _st.xm_render_min_cpu,
      _st.xm_render_last_cpu,
      _st.xm_render_max_cpu);

    char c; if (uart_read_bytes(UART_NUM_0, &c, 1, 0)) break;

    // printf("\033[18A");   // go up
  }

  printf("\r\n");

  return 0;
}

int set_mvol(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage: volume <volume>\r\n");
    return 1;
  }

  char *endp = NULL;
  uint64_t vol = strtoull(argv[1], &endp, 0);
  if (!endp || *endp)
  {
    printf("Bad <volume>: %s\r\n", argv[1]);
    return 1;
  }

  if (vol > 255)
  {
    printf("Bad <volume>: %s (expected 0..255)\r\n", argv[1]);
    return 1;
  }

  master_volume = (int)(vol * 1000ULL);

  printf("Master volume: %d\r\n", master_volume / 1000);

  return 0;
}

// ---------- Command registration ----------
void esp_console_register_system_commands()
{
  {
    const esp_console_cmd_t cmd =
    {
      .command = "errors",
      .help    = "Current error and help",
      .hint    = NULL,
      .func    = &error_list,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "perf",
      .help    = "Performance test",
      .hint    = NULL,
      .func    = &perf_test,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "info",
      .help    = "Get chip info",
      .hint    = NULL,
      .func    = &get_info,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

#if WITH_TASKS_INFO
  {
    const esp_console_cmd_t cmd =
    {
      .command = "tasks",
      .help    = "Get information about running tasks",
      .hint    = NULL,
      .func    = &tasks_info,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
#endif // WITH_TASKS_INFO

  {
    const esp_console_cmd_t cmd =
    {
      .command = "restart",
      .help    = "Software chip reset",
      .hint    = NULL,
      .func    = &restart,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "memory",
      .help    = "Get the memory info",
      .hint    = NULL,
      .func    = &mem_info,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "objects",
      .help    = "Get the memory objects info",
      .hint    = NULL,
      .func    = &obj_info,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "stats",
      .help    = "Get CPU usage of running tasks",
      .hint    = NULL,
      .func    = &stats_info,
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command = "volume",
      .help    = "Set master volume",
      .hint    = NULL,
      .func    = &set_mvol,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command  = "usbmouse",
      .help     = "Switch USB to host mode, wait for mouse, print movement, reboot on mouse button press",
      .hint     = NULL,
      .func     = &usbmouse_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command  = "ps2mouse",
      .help     = "PS/2 mouse emulation on GPIO5(data), GPIO7(clk): start, stop, move <dx> <dy> [buttons]",
      .hint     = NULL,
      .func     = &ps2_mouse_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    // const esp_console_cmd_t cmd =
    // {
      // .command  = "btmouse",
      // .help     = "Start BLE mouse scan/connect mode, any key stops BT",
      // .hint     = NULL,
      // .func     = &bt_mouse_cmd,
      // .argtable = NULL
    // };

    // ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
}
