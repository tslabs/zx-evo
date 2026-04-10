
#pragma once

#include <stdint.h>
#include "miniz.h"
#include "xm.h"

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

enum
{
  // Core 0
  CONSOLE_TASK_PRIO           = 2,
  WIFI_AUTOCONNECT_TASK_PRIO  = 3,
  STREAMER_TASK_PRIO          = 4,
  TCPIP_TASK_PRIO             = 5,
  WIFI_TASK_PRIO              = 6,
  XM_HELPER_TASK_PRIO         = 7,
  HELPER_TASK_PRIO            = 8,
  USB_MOUSE_TASK_PRIO         = 10,
  USB_MOUSE_LIB_TASK_PRIO     = 10,
  USB_MOUSE_HID_TASK_PRIO     = 11,
  I2S_TASK_PRIO               = 12,
  SLAVE_TASK_PRIO             = 14,
  
  // Core 1
  XM_PLAYER_TASK_PRIO         = 10,
};

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define CP_STRING "ESP32 SPI WiFi Module, ver.0.5.299, (c) TS-Labs"

extern tinfl_decompressor *decomp;

// #define VERBOSE

#define _delay_ms(a) vTaskDelay(pdMS_TO_TICKS(a));

#ifdef CONFIG_SPIRAM
  #define malloc_spiram(a) heap_caps_malloc(a, MALLOC_CAP_SPIRAM)
#else
  #define malloc_spiram(a) malloc(a)
#endif

void log_sram_used(const char *name);
