
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"
#include "bootloader_random.h"
#include "esp_random.h"
#include "nvs.h"
#include "miniz.h"
#include "nvs_flash.h"
#include <esp_crc.h>

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "console.h"

#ifdef CONFIG_ESP32_WIFI_ENABLED
#include "wifi.h"
#endif

#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "stats.h"
#include "ft8xx.h"
#include "elf.cpp.h"
#include "helper.h"
#include "http_client.h"
#include "depack.h"

// #define ISR_PRINTF

tinfl_decompressor *decomp = NULL;

const char TAG[] = "main";

// ------------- Debug helpers

#ifdef ISR_PRINTF

#define PRINT_QUEUE_LENGTH 100
static QueueHandle_t printQueue;

void print_task(void *pvParameters)
{
  char *message;

  while (1)
  {
    if (xQueueReceive(printQueue, &message, portMAX_DELAY))
      printf("%s", message);
  }
}

extern "C"
{
void print_isr(const char* msg)
{
  xQueueSendFromISR(printQueue, &msg, NULL);
}
}

#endif  // ISR_PRINTF

// void IRAM_ATTR splash(int n)
// {
  // while (n--)
  // {
    // gpio_set_level((gpio_num_t)GPIO_TEST1, 1); gpio_set_level((gpio_num_t)GPIO_TEST1, 0); // !!!
  // }
// }

// ------------- System start and initialization

extern "C" void app_main()
{
  // ----- Test GPIO init
  // gpio_set_direction((gpio_num_t)GPIO_TEST1, GPIO_MODE_OUTPUT);
  // gpio_set_direction((gpio_num_t)GPIO_TEST2, GPIO_MODE_OUTPUT);
  // gpio_set_direction((gpio_num_t)GPIO_TEST3, GPIO_MODE_OUTPUT);
  ESP_LOGI("SRAM", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // ----- Runtime inits
  stats::init();
  bootloader_random_enable();

#ifdef ISR_PRINTF
  // ----- ISR printf init
  printQueue = xQueueCreate(PRINT_QUEUE_LENGTH, sizeof(char*));
  ESP_LOGI("SRAM xQueueCreate printQueue", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  xTaskCreatePinnedToCore(print_task, "isr-printf", 2048, NULL, 1, NULL, 0);
  ESP_LOGI("SRAM xTaskCreatePinnedToCore print_task", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#endif

  // ----- Wi-Fi init
  initialize_nvs();
  ESP_LOGI("SRAM initialize_nvs", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#ifdef CONFIG_ESP32_WIFI_ENABLED
  initialize_wifi();
  ESP_LOGI("SRAM initialize_wifi", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  net.is_init = true;
  net.state = NETWORK_CLOSED;
#endif

  // ----- Helper init
  helper_queue = xQueueCreate(2, sizeof(int));
  ESP_LOGI("SRAM xQueueCreate helper_queue", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  xTaskCreatePinnedToCore(helper_task, "helper", 3072, NULL, 23, NULL, 0);
  ESP_LOGI("SRAM xTaskCreatePinnedToCore helper_task", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  decomp = (tinfl_decompressor*)malloc_spiram(sizeof(tinfl_decompressor)); // early init to avoid heap fragmentation at stream inflate

  // ----- LibXM init
  initialize_xm();
  ESP_LOGI("SRAM initialize_xm", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // ----- HTTP init
  http_init();
  ESP_LOGI("SRAM http_init", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  
  // ----- SDMMC init
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  sd_ldo_init();
#endif

  // ----- SPI slave init
  init_spi_configs();
  ESP_LOGI("SRAM init_spi_configs", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  init_slave_hd();
  ESP_LOGI("SRAM init_slave_hd", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

  // ----- FT812 init
  init_ft8xx();
  ESP_LOGI("SRAM init_ft8xx", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  
  // ----- Console init
  initialize_console();
  ESP_LOGI("SRAM initialize_console", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  xTaskCreatePinnedToCore(console_task, "console", 6144, NULL, 1, NULL, 0);
  ESP_LOGI("SRAM xTaskCreatePinnedToCore console_task", "%u", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));

#ifdef CONFIG_ESP32_WIFI_ENABLED
  TaskHandle_t wifiHandle = xTaskGetHandle("wifi");
  vTaskPrioritySet(wifiHandle, 21);
#endif
}
