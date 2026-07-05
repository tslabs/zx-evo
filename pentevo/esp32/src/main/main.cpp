
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
#include "nvs_params.h"

#if defined(CONFIG_ESP_WIFI_ENABLED) && CONFIG_ESP_WIFI_ENABLED
#include "wifi.h"
#include "http_client.h"
#include "gopher_client.h"
#include "stream_client.h"
#endif

#include "mem_obj.h"
#include "tracker.h"
#include "stats.h"
#include "ft8xx.h"
#include "elf.h"
#include "sdmmc.h"
#include "helper.h"
#include "depack.h"
#include "ps2_mouse.h"
#include "usb_mouse.h"

// #define ISR_PRINTF

EXT_RAM_BSS_ATTR tinfl_decompressor decomp_buf;
tinfl_decompressor *decomp = &decomp_buf;

const char TAG[] = "main";
int usb_mouse_start_on_boot = 0;

u32 task_ram_type_critical = MALLOC_CAP_INTERNAL;
u32 task_ram_type_non_critical = MALLOC_CAP_SPIRAM;

// ------------- Debug helpers

size_t sram_used_prev;

void log_sram_used(const char *name)
{
  size_t total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  size_t free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  size_t used = total - free;
  int delta = (int)used - (int)sram_used_prev;

  ESP_LOGI("SRAM", "%s, SRAM used: %d bytes", name, delta);
  sram_used_prev = used;
}

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
  esp_err_t err;

  // ----- Test GPIO init
  // gpio_set_direction((gpio_num_t)GPIO_TEST1, GPIO_MODE_OUTPUT);
  // gpio_set_direction((gpio_num_t)GPIO_TEST2, GPIO_MODE_OUTPUT);
  // gpio_set_direction((gpio_num_t)GPIO_TEST3, GPIO_MODE_OUTPUT);
  sram_used_prev = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) - heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  // ----- Runtime inits
  stats::init();
  bootloader_random_enable();

#ifdef ISR_PRINTF
  // ----- ISR printf init
  printQueue = xQueueCreate(PRINT_QUEUE_LENGTH, sizeof(char*));
  log_sram_used("QueueCreate printQueue");
  xTaskCreatePinnedToCoreWithCaps(print_task, "isr-printf", 2048, NULL, 1, NULL, 0, task_ram_type_non_critical);
  log_sram_used("TaskCreate print_task");
#endif


  // ----- NVS + params init
  initialize_nvs();
  ESP_ERROR_CHECK(app_params_load());

  usb_mouse_start_on_boot = app_params.usb_mode ? 1 : 0;

  ESP_LOGI("MAIN", "usb_mode=%u, usb_mouse_start_on_boot=%d", app_params.usb_mode, usb_mouse_start_on_boot);

  // ----- Wi-Fi init
#if defined(CONFIG_ESP_WIFI_ENABLED) && CONFIG_ESP_WIFI_ENABLED
  net.is_init = false;
  net.state = NETWORK_CLOSED;

  if (wifi_is_enabled())
  {
    initialize_wifi();
    wifi_start_autoconnect();
    net.is_init = true;
    log_sram_used("initialize_wifi");
  }
  else
    ESP_LOGI("MAIN", "WiFi disabled by config");
#endif

  // ----- Helper init
  helper_queue = xQueueCreateWithCaps(2, sizeof(int), task_ram_type_non_critical);
  log_sram_used("QueueCreate helper_queue");
  xTaskCreatePinnedToCoreWithCaps(helper_task, "helper", 6144, NULL, HELPER_TASK_PRIO, NULL, 0, task_ram_type_critical);
  log_sram_used("TaskCreate helper_task");

  // ----- LibXM init
  initialize_xm();

  // ----- Network client init
#if defined(CONFIG_ESP_WIFI_ENABLED) && CONFIG_ESP_WIFI_ENABLED
  http_init();
  log_sram_used("http_init");

  gopher_init();
  log_sram_used("gopher_init");

  stream_init();
  log_sram_used("stream_init");
#endif
  
  // ----- SDMMC init
#if defined(CONFIG_IDF_TARGET_ESP32P4)
  sd_ldo_init();
#endif

  // ----- SPI slave init
  init_slave_hd();

  // ----- FT812 init
  init_ft8xx();
  log_sram_used("init_ft8xx");

  // ----- PS/2 mouse init
  err = ps2_mouse_start();
  if (err != ESP_OK)
    ESP_LOGE("PS2", "ps2_mouse_start failed: %s", esp_err_to_name(err));
  log_sram_used("PS/2 mouse init");

  // ----- USB mouse init
  if (usb_mouse_start_on_boot)
  {
    err = usb_mouse_start();
    if (err != ESP_OK)
      ESP_LOGE("USBM", "usb_mouse_start failed: %s", esp_err_to_name(err));
  }
  log_sram_used("USB mouse init");

  // ----- Console init
  initialize_console();
  xTaskCreatePinnedToCoreWithCaps(console_task, "console", 6144, NULL, CONSOLE_TASK_PRIO, NULL, 0, task_ram_type_critical);
  log_sram_used("TaskCreate console_task");
}
