
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <unistd.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_chip_info.h"
#include "esp_sleep.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_private/esp_clk.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#ifdef CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS
#define WITH_TASKS_INFO    1
#endif

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

int free_mem(int argc, char **argv)
{
  printf("%" PRIu32 "\n", esp_get_free_heap_size());
  return 0;
}

void register_free()
{
  const esp_console_cmd_t cmd =
  {
    .command = "free",
    .help    = "Get the current size of free heap memory",
    .hint    = NULL,
    .func    = &free_mem,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}

int heap_size(int argc, char **argv)
{
  uint32_t heap_size = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);

  printf("min heap size: %" PRIu32 "\n", heap_size);
  return 0;
}

void register_heap()
{
  const esp_console_cmd_t heap_cmd =
  {
    .command = "heap",
    .help    = "Get minimum size of free heap memory that was available during program execution",
    .hint    = NULL,
    .func    = &heap_size,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&heap_cmd));
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

void esp_console_register_system_commands()
{
  register_free();
  register_heap();
  register_info();
  register_restart();
#if WITH_TASKS_INFO
  register_tasks();
#endif
}
