
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
#include "nvs_flash.h"
#include <esp_crc.h>

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "console.h"
#include "ft812.h"
#include "wifi.h"
#include "mem_obj.h"
#include "xm.h"
#include "xm_cpp.h"
#include "stats.h"
#include "elf.cpp.h"
#include "helper.h"
#include "depack.h"

// #define ISR_PRINTF

const char cp_string[] = "ESP32-S3 SPI WiFi Module, ver.0.3, (c)2024, TS-Labs";
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

  // ----- Runtime inits
  stats::init();
  bootloader_random_enable();

#ifdef ISR_PRINTF
  // ----- ISR printf init
  printQueue = xQueueCreate(PRINT_QUEUE_LENGTH, sizeof(char*));
  xTaskCreatePinnedToCore(print_task, "isr-printf", 2048, NULL, 1, NULL, 0);
#endif

  // ----- Wi-Fi init
  initialize_nvs();
  initialize_wifi();
  net.is_init = true;
  net.state = NETWORK_CLOSED;

  helper_queue = xQueueCreate(2, sizeof(int));
  xTaskCreatePinnedToCore(helper_task, "helper", 6144, NULL, 23, NULL, 0);

  // ----- LibXM init
  initialize_xm();

  // ----- SPI slave init
  init_slave_hd();

  // ----- Console init
  initialize_console();
  xTaskCreatePinnedToCore(console_task, "console", 4096, NULL, 1, NULL, 0);

  TaskHandle_t wifiHandle = xTaskGetHandle("wifi");
  vTaskPrioritySet(wifiHandle, 21);
}
