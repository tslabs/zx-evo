
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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

#include "main.h"
#include "esp_spi_defs.h"
#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "stats.h"

using namespace stats;

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO    1
#endif

const char TAG[] = "zf32_system";

int get_info(int argc, char **argv)
{
  const char      *model;
  esp_chip_info_t info;
  uint32_t        flash_size;
  uint32_t        psram_size;

  esp_chip_info(&info);

  switch (info.model)
  {
    case CHIP_ESP32:
      model = "ESP32";
      break;

    case CHIP_ESP32S2:
      model = "ESP32-S2";
      break;

    case CHIP_ESP32S3:
      model = "ESP32-S3";
      break;

    case CHIP_ESP32C3:
      model = "ESP32-C3";
      break;

    case CHIP_ESP32H2:
      model = "ESP32-H2";
      break;

    case CHIP_ESP32C2:
      model = "ESP32-C2";
      break;

    default:
      model = "Unknown";
      break;
  }

  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    printf("Get flash size failed");
    flash_size = 0;
  }

  // if (esp_psram_get_size(NULL, &flash_size) != ESP_OK)
  {
    // printf("Get flash size failed");
    psram_size = 0;
  }

  printf("\r\n%s\r\n", cp_string);
  printf("Build: " __DATE__ ", " __TIME__ "\r\n");
  printf("\r\n");

  printf("Chip info:\r\n");
  printf("\tModel: %s\r\n", model);
  printf("\tSilicon revision: %d\r\n", info.revision);
  printf("\tFrequency: %dMHz\r\n", esp_clk_cpu_freq() / 1000000);
  printf("\tCores: %d\r\n", info.cores);
  printf("\tFeature: %s%s%s\r\n",
         info.features & CHIP_FEATURE_WIFI_BGN ? "WiFi 802.11bgn" : "",
         info.features & CHIP_FEATURE_BLE ? " / BLE" : "",
         info.features & CHIP_FEATURE_BT ? " / BT" : "");
  printf("\t%s %luMB\r\n",
         info.features & CHIP_FEATURE_EMB_FLASH ? "Embedded-Flash: " : "External-Flash: ",
         flash_size / (1024 * 1024));
  // printf("\t%s, %luMB\r\n",
         // info.features & CHIP_FEATURE_EMB_PSRAM ? "Embedded-PSRAM: " : "External-PSRAM: ",
         // psram_size / (1024 * 1024));
  printf("\r\n");

  printf("IDF Version: %s\r\n", esp_get_idf_version());
  printf("\r\n");

  return 0;
}

void register_info()
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

int restart(int argc, char **argv)
{
  ESP_LOGI(TAG, "Restarting");
  esp_restart();
}

void register_restart()
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

void register_mem()
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

int obj_info(int argc, char **argv)
{
  printf("\r\nMemory objects\r\n");

  for (int i = 0; i < OBJ_HANDLES_MAX; i++)
    if (mem_obj[i].addr)
    {
      printf("Hdl %02X  Typ %02X  St %02X  Addr %08X  Size %u  ", i, mem_obj[i].type, mem_obj[i].state, (unsigned int)mem_obj[i].addr, mem_obj[i].size);

      switch (mem_obj[i].type)
      {
        case OBJ_TYPE_LIB:
          printf("Ent %08X  TX %08X  DT %08X  RO %08X  BS %08X\r\n", (unsigned int)mem_obj[i].entry, (unsigned int)mem_obj[i].text, (unsigned int)mem_obj[i].data, (unsigned int)mem_obj[i].rodata, (unsigned int)mem_obj[i].bss);
        break;
        
        default:
          printf("\r\n");
        break;
      }
    }

  printf("\r\n");

  return 0;
}

void register_objects()
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

#if WITH_TASKS_INFO

int tasks_info(int argc, char **argv)
{
  const size_t bytes_per_task    = 40; /* see vTaskList description */
  char         *task_list_buffer = (char*)malloc(uxTaskGetNumberOfTasks() * bytes_per_task);

  if (task_list_buffer == NULL)
  {
    ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
    return 1;
  }

  fputs("Task Name\tStatus\tPrio\tHWM\tTask#", stdout);
#ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
  fputs("\tAffinity", stdout);
#endif
  fputs("\n", stdout);
  vTaskList(task_list_buffer);
  fputs(task_list_buffer, stdout);
  free(task_list_buffer);

  return 0;
}

void register_tasks()
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

int stats_info(int argc, char **argv)
{
#if CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER
  vTaskGetRunTimeStats(_st.runtime_stats_buffer);
  printf("\r\n");
  printf("Task runtime statistics:\n%s", _st.runtime_stats_buffer);
#endif

  printf("\r\n");

  while (1)
  {
    printf("drq_data_start_last_us: %d  \r\n", _st.drq_data_start_last_us);
    printf("drq_data_start_min_us: %d  \r\n", _st.drq_data_start_min_us);
    printf("drq_data_start_max_us: %d  \r\n", _st.drq_data_start_max_us);
    printf("\r\n");
    printf("drq_data_end_last_us: %d  \r\n", _st.drq_data_end_last_us);
    printf("drq_data_end_min_us: %d  \r\n", _st.drq_data_end_min_us);
    printf("drq_data_end_max_us: %d  \r\n", _st.drq_data_end_max_us);
    printf("\r\n");
    printf("xm_render_last_us: %d  \r\n", _st.xm_render_last_us);
    printf("xm_render_min_us: %d  \r\n", _st.xm_render_min_us);
    printf("xm_render_max_us: %d  \r\n", _st.xm_render_max_us);
    printf("\r\n");
    printf("xm_render_last_cpu: %d  \r\n", _st.xm_render_last_cpu);
    printf("xm_render_min_cpu: %d  \r\n", _st.xm_render_min_cpu);
    printf("xm_render_max_cpu: %d  \r\n", _st.xm_render_max_cpu);
    printf("\r\n");
    printf("xm_samp_min: %f   \r\n", _st.xm_samp_min);
    printf("xm_samp_max: %f   \r\n", _st.xm_samp_max);

    char c; if (uart_read_bytes(UART_NUM_0, &c, 1, 0)) break;

    printf("\033[18A");   // go up
  }

  printf("\r\n");

  return 0;
}

void register_stats()
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

void register_x()
{
  // const esp_console_cmd_t cmd =
  // {
    // .command = "x",
    // .help    = "Test",
    // .hint    = NULL,
    // .func    = &tst_func,
  // };

  // ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

void esp_console_register_system_commands()
{
  register_mem();
  register_objects();
  register_info();
  register_restart();
#if WITH_TASKS_INFO
  register_tasks();
  register_stats();
  register_x();
#endif
}
