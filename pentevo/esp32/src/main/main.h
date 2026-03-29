
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

#define GPIO_TEST1    4
#define GPIO_TEST2    5
#define GPIO_TEST3    6

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define CP_STRING "ESP32 SPI WiFi Module, ver.0.5.9, (c) TS-Labs"

extern tinfl_decompressor *decomp;

// #define VERBOSE

#define _delay_ms(a) vTaskDelay(pdMS_TO_TICKS(a));

#ifdef CONFIG_SPIRAM
  #define malloc_spiram(a) heap_caps_malloc(a, MALLOC_CAP_SPIRAM)
#else
  #define malloc_spiram(a) malloc(a)
#endif
