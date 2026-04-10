
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
#include "esp_err.h"
#include "esp_intr_alloc.h"
#include "freertos/queue.h"
#include "usb/usb_host.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_mouse.h"

#include "main.h"
#include "usb_mouse.h"
#include "ps2_mouse.h"
#include "nvs_params.h"

enum usb_mouse_evt_group_t
{
  USB_MOUSE_EVT_QUIT = 0,
  USB_MOUSE_EVT_HID,
};

typedef struct
{
  usb_mouse_evt_group_t event_group;
  struct
  {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t event;
    void *arg;
  } hid;
} usb_mouse_evt_t;

QueueHandle_t usb_mouse_evt_queue = NULL;
TaskHandle_t usb_mouse_task_handle = NULL;
TaskHandle_t usb_mouse_lib_task_handle = NULL;
hid_host_device_handle_t usb_mouse_dev = NULL;
volatile bool usb_mouse_mode_active = false;
int usb_mouse_abs_x = 0;
int usb_mouse_abs_y = 0;

void usb_mouse_report_callback(const uint8_t *data, int length)
{
  if (length < (int)sizeof(hid_mouse_input_report_boot_t))
    return;

  const hid_mouse_input_report_boot_t *r = (const hid_mouse_input_report_boot_t *)data;

  usb_mouse_abs_x += r->x_displacement;
  usb_mouse_abs_y += r->y_displacement;

  ps2_mouse_send_movement((int)r->x_displacement,
                          -(int)r->y_displacement,
                          (unsigned)r->buttons.val);
}

void usb_mouse_interface_callback(hid_host_device_handle_t hid_device_handle, hid_host_interface_event_t event, void *arg)
{
  (void)arg;

  uint8_t data[64];
  size_t data_length = 0;
  hid_host_dev_params_t dev_params;

  memset(data, 0, sizeof(data));

  if (hid_host_device_get_params(hid_device_handle, &dev_params) != ESP_OK)
    return;

  switch (event)
  {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
      if (hid_host_device_get_raw_input_report_data(hid_device_handle, data, sizeof(data), &data_length) != ESP_OK)
        return;

      if (dev_params.sub_class == HID_SUBCLASS_BOOT_INTERFACE && dev_params.proto == HID_PROTOCOL_MOUSE)
        usb_mouse_report_callback(data, (int)data_length);
    break;

    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
      printf("usb mouse disconnected\r\n");
      if (usb_mouse_dev == hid_device_handle)
        usb_mouse_dev = NULL;
      usb_mouse_abs_x = 0;
      usb_mouse_abs_y = 0;
      hid_host_device_close(hid_device_handle);
    break;

    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
      printf("usb mouse transfer error\r\n");
    break;

    default:
    break;
  }
}

void usb_mouse_process_device_event(hid_host_device_handle_t hid_device_handle, hid_host_driver_event_t event, void *arg)
{
  (void)arg;

  hid_host_dev_params_t dev_params;
  if (hid_host_device_get_params(hid_device_handle, &dev_params) != ESP_OK)
    return;

  if (event != HID_HOST_DRIVER_EVENT_CONNECTED)
    return;

  if (dev_params.proto != HID_PROTOCOL_MOUSE)
  {
    printf("usb hid connected, but not a mouse (proto=%d)\r\n", (int)dev_params.proto);
    return;
  }

  if (dev_params.sub_class != HID_SUBCLASS_BOOT_INTERFACE)
  {
    printf("usb mouse connected, but not boot protocol\r\n");
    return;
  }

  const hid_host_device_config_t dev_config =
  {
    .callback = usb_mouse_interface_callback,
    .callback_arg = NULL,
  };

  if (hid_host_device_open(hid_device_handle, &dev_config) != ESP_OK)
  {
    printf("hid_host_device_open failed\r\n");
    return;
  }

  if (hid_class_request_set_protocol(hid_device_handle, HID_REPORT_PROTOCOL_BOOT) != ESP_OK)
  {
    printf("hid_class_request_set_protocol failed\r\n");
    hid_host_device_close(hid_device_handle);
    return;
  }

  if (hid_host_device_start(hid_device_handle) != ESP_OK)
  {
    printf("hid_host_device_start failed\r\n");
    hid_host_device_close(hid_device_handle);
    return;
  }

  usb_mouse_dev = hid_device_handle;
  usb_mouse_abs_x = 0;
  usb_mouse_abs_y = 0;

  printf("usb mouse connected\r\n");
}

void usb_mouse_device_callback(hid_host_device_handle_t hid_device_handle, hid_host_driver_event_t event, void *arg)
{
  usb_mouse_evt_t evt = {};
  evt.event_group = USB_MOUSE_EVT_HID;
  evt.hid.handle = hid_device_handle;
  evt.hid.event = event;
  evt.hid.arg = arg;

  if (usb_mouse_evt_queue)
    xQueueSend(usb_mouse_evt_queue, &evt, 0);
}

void usb_mouse_lib_task(void *arg)
{
  const usb_host_config_t host_config =
  {
    .skip_phy_setup = false,
    .intr_flags = ESP_INTR_FLAG_LOWMED,
  };

  esp_err_t err = usb_host_install(&host_config);
  if (err != ESP_OK)
  {
    printf("usb_host_install failed: %s\r\n", esp_err_to_name(err));
    usb_mouse_lib_task_handle = NULL;
    xTaskNotifyGive((TaskHandle_t)arg);
    vTaskDelete(NULL);
    return;
  }

  xTaskNotifyGive((TaskHandle_t)arg);

  while (1)
  {
    uint32_t event_flags = 0;
    err = usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    if (err != ESP_OK)
    {
      printf("usb_host_lib_handle_events failed: %s\r\n", esp_err_to_name(err));
      break;
    }

    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
    {
      usb_host_device_free_all();
      break;
    }
  }

  vTaskDelay(pdMS_TO_TICKS(10));
  usb_host_uninstall();
  usb_mouse_lib_task_handle = NULL;
  vTaskDelete(NULL);
}

void usb_mouse_task(void *arg)
{
  (void)arg;

  usb_mouse_evt_t evt;
  esp_err_t err;

  usb_mouse_evt_queue = xQueueCreate(8, sizeof(usb_mouse_evt_t));
  if (!usb_mouse_evt_queue)
  {
    printf("usb mouse queue create failed\r\n");
    usb_mouse_mode_active = false;
    usb_mouse_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  if (xTaskCreatePinnedToCoreWithCaps(usb_mouse_lib_task, "usb_mouse_lib", 2048, xTaskGetCurrentTaskHandle(), USB_MOUSE_LIB_TASK_PRIO, &usb_mouse_lib_task_handle, 0, MALLOC_CAP_SPIRAM) != pdTRUE)
  {
    printf("usb mouse lib task create failed\r\n");
    vQueueDelete(usb_mouse_evt_queue);
    usb_mouse_evt_queue = NULL;
    usb_mouse_mode_active = false;
    usb_mouse_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));

  const hid_host_driver_config_t hid_cfg =
  {
    .create_background_task = true,
    .task_priority = USB_MOUSE_HID_TASK_PRIO,
    .stack_size = 4096,
    .core_id = 0,
    .callback = usb_mouse_device_callback,
    .callback_arg = NULL,
  };

  err = hid_host_install(&hid_cfg);
  if (err != ESP_OK)
  {
    printf("hid_host_install failed: %s\r\n", esp_err_to_name(err));
    vQueueDelete(usb_mouse_evt_queue);
    usb_mouse_evt_queue = NULL;
    usb_mouse_mode_active = false;
    usb_mouse_task_handle = NULL;
    vTaskDelete(NULL);
    return;
  }

  printf("USB switched to host mode, connect mouse\r\n");

  while (1)
  {
    if (!xQueueReceive(usb_mouse_evt_queue, &evt, portMAX_DELAY))
      continue;

    if (evt.event_group == USB_MOUSE_EVT_QUIT)
      break;

    if (evt.event_group == USB_MOUSE_EVT_HID)
      usb_mouse_process_device_event(evt.hid.handle, evt.hid.event, evt.hid.arg);
  }

  if (usb_mouse_dev)
  {
    hid_host_device_stop(usb_mouse_dev);
    hid_host_device_close(usb_mouse_dev);
    usb_mouse_dev = NULL;
  }

  hid_host_uninstall();

  for (int i = 0; i < 100; i++)
  {
    if (usb_mouse_lib_task_handle == NULL)
      break;
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  QueueHandle_t q = usb_mouse_evt_queue;
  usb_mouse_evt_queue = NULL;
  if (q) vQueueDelete(q);

  usb_mouse_mode_active = false;
  usb_mouse_task_handle = NULL;

  vTaskDelete(NULL);
}

extern "C" esp_err_t usb_mouse_start()
{
  if (usb_mouse_mode_active)
    return ESP_ERR_INVALID_STATE;

  usb_mouse_mode_active = true;

  if (xTaskCreatePinnedToCoreWithCaps(usb_mouse_task, "usb_mouse", 4096, NULL, USB_MOUSE_TASK_PRIO, &usb_mouse_task_handle, 0, MALLOC_CAP_SPIRAM) != pdTRUE)
  {
    usb_mouse_mode_active = false;
    usb_mouse_task_handle = NULL;
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}


int usb_cmd(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage:\r\n");
    printf("  usb en <0|1>\r\n");
    printf("  usb up\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "up"))
  {
    if (argc != 2)
    {
      printf("Usage: usb up\r\n");
      return 1;
    }

    esp_err_t err = usb_mouse_start();
    if (err == ESP_ERR_INVALID_STATE)
    {
      printf("usb mouse mode already active\r\n");
      return 0;
    }

    if (err != ESP_OK)
    {
      printf("usb mouse task create failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    printf("usb mouse host started until reset (NVS unchanged)\r\n");
    return 0;
  }

  if (strcmp(argv[1], "en"))
  {
    printf("Unknown subcommand: %s\r\n", argv[1]);
    printf("Usage:\r\n");
    printf("  usb en <0|1>\r\n");
    printf("  usb up\r\n");
    return 1;
  }

  if (argc < 3)
  {
    printf("Usage: usb en <0|1>\r\n");
    return 1;
  }

  char *endp = NULL;
  unsigned long en = strtoul(argv[2], &endp, 0);
  if (!endp || *endp || en > 1)
  {
    printf("Bad <0|1>: %s\r\n", argv[2]);
    return 1;
  }

  app_params.usb_mode = (uint8_t)en;

  esp_err_t err = app_params_save();
  if (err != ESP_OK)
  {
    printf("app_params_save failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("USB enable: %u\r\n", app_params.usb_mode);

  if (app_params.usb_mode)
  {
    err = usb_mouse_start();
    if (err == ESP_ERR_INVALID_STATE)
    {
      printf("usb mouse mode already active\r\n");
      return 0;
    }

    if (err != ESP_OK)
    {
      printf("usb mouse task create failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    printf("usb mouse host started\r\n");
    return 0;
  }

  if (usb_mouse_mode_active)
    printf("USB disable saved, active host mode will stay until reset\r\n");

  return 0;
}
