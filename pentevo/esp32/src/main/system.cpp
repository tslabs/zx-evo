
#include <stdio.h>
#include <stdlib.h>
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
#if __has_include("esp_flash_chips/spi_flash_chip_driver.h")
#define HAVE_ESP_FLASH_CHIP_DRIVER_H 1
#include "esp_flash_chips/spi_flash_chip_driver.h"
#elif __has_include("spi_flash_chip_driver.h")
#define HAVE_ESP_FLASH_CHIP_DRIVER_H 1
#include "spi_flash_chip_driver.h"
#else
#define HAVE_ESP_FLASH_CHIP_DRIVER_H 0
#endif
#include "esp_partition.h"
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
#include "nvs_params.h"

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


const char *flash_vendor_str(uint8_t id)
{
  switch (id)
  {
    case 0x01: return "Spansion / Cypress / Infineon";
    case 0x1C: return "EON";
    case 0x20: return "ST / Numonyx / Micron family";
    case 0x31: return "Catalyst / Onsemi";
    case 0x62: return "Sanyo";
    case 0x68: return "Boya";
    case 0x85: return "Puya";
    case 0x8C: return "ESMT";
    case 0x9D: return "ISSI / PMC";
    case 0xAD: return "Bright / Hyundai";
    case 0xBF: return "SST / Microchip";
    case 0xC2: return "Macronix";
    case 0xC8: return "GigaDevice";
    case 0xD5: return "ISSI";
    case 0xEF: return "Winbond";
    default:   return "Unknown";
  }
}

const char *flash_mode_str()
{
#if defined(CONFIG_ESPTOOLPY_FLASHMODE_OPI)
  return "OPI";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_QIO)
  return "QIO";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_QOUT)
  return "QOUT";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_DIO)
  return "DIO";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_DOUT)
  return "DOUT";
#elif defined(CONFIG_ESPTOOLPY_FLASHMODE_SLOW_READ)
  return "SLOW_READ";
#else
  return "Unknown";
#endif
}

const char *flash_freq_str()
{
#if defined(CONFIG_ESPTOOLPY_FLASHFREQ_120M)
  return "120 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_80M)
  return "80 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_40M)
  return "40 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_26M)
  return "26.7 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_20M)
  return "20 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_16M)
  return "16 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_15M)
  return "15 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_12M)
  return "12 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_10M)
  return "10 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_5M)
  return "5 MHz";
#elif defined(CONFIG_ESPTOOLPY_FLASHFREQ_2M)
  return "2 MHz";
#else
  return "Unknown";
#endif
}

#define FLASH_MAX_PROTECT_REGIONS 64

typedef struct
{
  esp_flash_region_t region;
  esp_err_t state_err;
  bool protected_region;
} flash_protect_region_info_t;

typedef struct
{
  esp_flash_t *chip;
  esp_err_t id_err;
  esp_err_t size_hdr_err;
  esp_err_t size_phy_err;
  esp_err_t uid_err;
  esp_err_t chip_wp_err;
  esp_err_t regions_err;
  uint32_t id;
  uint32_t size_hdr;
  uint32_t size_phy;
  uint64_t uid;
  bool quad;
  bool chip_wp;
  bool unique_id_supported;
  bool chip_suspend_known;
  bool chip_suspend_supported;
  uint32_t regions_total;
  uint32_t regions_count;
  flash_protect_region_info_t regions[FLASH_MAX_PROTECT_REGIONS];
} flash_info_data_t;

bool flash_chip_suspend_cap(esp_flash_t *chip, bool *supported)
{
  *supported = false;

#if HAVE_ESP_FLASH_CHIP_DRIVER_H
  if (!chip || !chip->chip_drv || !chip->chip_drv->get_chip_caps) return false;

  spi_flash_caps_t caps = chip->chip_drv->get_chip_caps(chip);
  *supported = (caps & SPI_FLASH_CHIP_CAP_SUSPEND) != 0;
  return true;
#else
  return false;
#endif
}

void flash_collect_info(esp_flash_t *chip, flash_info_data_t *info)
{
  memset(info, 0, sizeof(*info));
  info->chip = chip;
  info->uid_err = ESP_ERR_NOT_SUPPORTED;
  info->chip_wp_err = ESP_ERR_INVALID_STATE;
  info->regions_err = ESP_ERR_INVALID_STATE;

  info->id_err = esp_flash_read_id(chip, &info->id);
  info->size_hdr_err = esp_flash_get_size(chip, &info->size_hdr);
  info->size_phy_err = esp_flash_get_physical_size(chip, &info->size_phy);
  info->uid_err = esp_flash_read_unique_chip_id(chip, &info->uid);
  info->unique_id_supported = (info->uid_err == ESP_OK);
  info->quad = esp_flash_is_quad_mode(chip);
  info->chip_suspend_known = flash_chip_suspend_cap(chip, &info->chip_suspend_supported);
  info->chip_wp_err = esp_flash_get_chip_write_protect(chip, &info->chip_wp);

  const esp_flash_region_t *regions = NULL;
  info->regions_err = esp_flash_get_protectable_regions(chip, &regions, &info->regions_total);
  if (info->regions_err != ESP_OK) return;

  info->regions_count = info->regions_total;
  if (info->regions_count > FLASH_MAX_PROTECT_REGIONS) info->regions_count = FLASH_MAX_PROTECT_REGIONS;

  for (uint32_t i = 0; i < info->regions_count; i++)
  {
    info->regions[i].region = regions[i];
    info->regions[i].state_err = esp_flash_get_protected_region(chip, &regions[i], &info->regions[i].protected_region);
  }
}

void flash_print_protect_regions(const flash_info_data_t *info)
{
  if (info->regions_err != ESP_OK)
  {
    printf("Protection map: n/a (%s)\r\n", esp_err_to_name(info->regions_err));
    return;
  }

  printf("Protection map:\r\n");

  for (uint32_t i = 0; i < info->regions_count; i++)
  {
    const flash_protect_region_info_t *r = &info->regions[i];

    printf
    (
      "  %2" PRIu32 ": %08" PRIX32 "..%08" PRIX32 "  %8" PRIu32 " KB  ",
      i,
      r->region.offset,
      r->region.offset + r->region.size - 1,
      r->region.size / 1024
    );

    if (r->state_err == ESP_OK) printf("%s", r->protected_region ? "protected" : "open");
    else printf("state n/a (%s)", esp_err_to_name(r->state_err));

    printf("\r\n");
  }

  if (info->regions_total > info->regions_count)
    printf("  ... clipped: %" PRIu32 " regions not printed\r\n", info->regions_total - info->regions_count);
}

const char *flash_part_type_str(uint8_t type)
{
  switch (type)
  {
    case ESP_PARTITION_TYPE_APP:             return "app";
    case ESP_PARTITION_TYPE_DATA:            return "data";
    case ESP_PARTITION_TYPE_BOOTLOADER:      return "bootldr";
    case ESP_PARTITION_TYPE_PARTITION_TABLE: return "ptable";
    default:                                 return "custom";
  }
}

const char *flash_part_subtype_str(uint8_t type, uint8_t subtype)
{
  if (type == ESP_PARTITION_TYPE_BOOTLOADER)
  {
    switch (subtype)
    {
      case ESP_PARTITION_SUBTYPE_BOOTLOADER_PRIMARY:  return "primary";
      case ESP_PARTITION_SUBTYPE_BOOTLOADER_OTA:      return "ota";
      case ESP_PARTITION_SUBTYPE_BOOTLOADER_RECOVERY: return "recovery";
      default:                                        return "unknown";
    }
  }

  if (type == ESP_PARTITION_TYPE_PARTITION_TABLE)
  {
    switch (subtype)
    {
      case ESP_PARTITION_SUBTYPE_PARTITION_TABLE_PRIMARY: return "primary";
      case ESP_PARTITION_SUBTYPE_PARTITION_TABLE_OTA:     return "ota";
      default:                                            return "unknown";
    }
  }

  if (type == ESP_PARTITION_TYPE_APP)
  {
    if (subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) return "factory";
    if (subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN && subtype < ESP_PARTITION_SUBTYPE_APP_OTA_MAX) return "ota_x";
    if (subtype >= ESP_PARTITION_SUBTYPE_APP_TEE_MIN && subtype <= ESP_PARTITION_SUBTYPE_APP_TEE_MAX) return "tee_x";

    switch (subtype)
    {
      case ESP_PARTITION_SUBTYPE_APP_TEST: return "test";
      default:                             return "unknown";
    }
  }

  if (type == ESP_PARTITION_TYPE_DATA)
  {
    switch (subtype)
    {
      case ESP_PARTITION_SUBTYPE_DATA_OTA:       return "ota";
      case ESP_PARTITION_SUBTYPE_DATA_PHY:       return "phy";
      case ESP_PARTITION_SUBTYPE_DATA_NVS:       return "nvs";
      case ESP_PARTITION_SUBTYPE_DATA_COREDUMP:  return "coredump";
      case ESP_PARTITION_SUBTYPE_DATA_NVS_KEYS:  return "nvs_keys";
      case ESP_PARTITION_SUBTYPE_DATA_EFUSE_EM:  return "efuse_em";
      case ESP_PARTITION_SUBTYPE_DATA_UNDEFINED: return "undef";
      case ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD:  return "esphttpd";
      case ESP_PARTITION_SUBTYPE_DATA_FAT:       return "fat";
      case ESP_PARTITION_SUBTYPE_DATA_SPIFFS:    return "spiffs";
      case ESP_PARTITION_SUBTYPE_DATA_LITTLEFS:  return "littlefs";
      case 0x84:                                 return "tsf";
      case ESP_PARTITION_SUBTYPE_DATA_TEE_OTA:   return "tee_ota";
      default:                                   return "unknown";
    }
  }

  return "unknown";
}

typedef struct
{
  uint32_t address;
  uint32_t size;
} flash_region_info_t;

void flash_region_insert_sorted(flash_region_info_t *regions, int *count, int max_count, uint32_t address, uint32_t size)
{
  if (*count >= max_count) return;

  int pos = *count;
  while (pos > 0 && regions[pos - 1].address > address)
  {
    regions[pos] = regions[pos - 1];
    pos--;
  }

  regions[pos].address = address;
  regions[pos].size = size;
  (*count)++;
}

void flash_region_add_system(flash_region_info_t *regions, int *count, int max_count)
{
  flash_region_insert_sorted(regions, count, max_count, 0x00000000, CONFIG_PARTITION_TABLE_OFFSET);
  flash_region_insert_sorted(regions, count, max_count, CONFIG_PARTITION_TABLE_OFFSET, 0x1000);
}

int flash_collect_regions(flash_region_info_t *regions, int max_count)
{
  int count = 0;
  esp_partition_iterator_t it;

  flash_region_add_system(regions, &count, max_count);

  it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it)
  {
    const esp_partition_t *p = esp_partition_get(it);
    if (p) flash_region_insert_sorted(regions, &count, max_count, p->address, p->size);
    it = esp_partition_next(it);
  }

  esp_partition_iterator_release(it);
  return count;
}

void flash_print_partition_line(int idx, uint32_t address, uint32_t size, uint32_t erase_size, const char *flags, const char *type_s, const char *sub_s, const char *label)
{
  printf
  (
    "%3d  %08" PRIX32 "  %08" PRIX32 "  %7" PRIu32 "  %5" PRIu32 "  %-5s  %-7s/%-8s  %s\r\n",
    idx,
    address,
    address + size - 1,
    size / 1024,
    erase_size,
    flags,
    type_s,
    sub_s,
    label
  );
}

void flash_print_partitions(uint32_t flash_size)
{
  printf("\r\n");
  printf("Partitions:\r\n");
  printf("Idx  Address   End       Size KB  Erase  Flags  Type/SubType      Label\r\n");

  int idx = 0;
  esp_partition_iterator_t it;

  flash_print_partition_line(idx++, 0x00000000, CONFIG_PARTITION_TABLE_OFFSET, 0x1000, "-", "bootldr", "primary", "bootloader");
  flash_print_partition_line(idx++, CONFIG_PARTITION_TABLE_OFFSET, 0x1000, 0x1000, "-", "ptable", "primary", "partition_table");

  it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it)
  {
    const esp_partition_t *p = esp_partition_get(it);
    const char *type_s = flash_part_type_str(p->type);
    const char *sub_s = flash_part_subtype_str(p->type, p->subtype);
    char flags[8];
    int f = 0;

    if (p->encrypted) flags[f++] = 'E';
    if (p->readonly) flags[f++] = 'R';
    if (!f) flags[f++] = '-';
    flags[f] = 0;

    flash_print_partition_line(idx++, p->address, p->size, p->erase_size, flags, type_s, sub_s, p->label[0] ? p->label : "-");
    it = esp_partition_next(it);
  }

  esp_partition_iterator_release(it);

  flash_region_info_t regions[128];
  int region_count = flash_collect_regions(regions, 128);
  uint32_t used = 0;

  for (int i = 0; i < region_count; i++)
    used += regions[i].size;

  printf("\r\n");
  printf("Coverage:\r\n");
  printf("  Used by regions    : %" PRIu32 " bytes (%" PRIu32 " KB)\r\n", used, used / 1024);
  if (flash_size >= used) printf("  Free in flash      : %" PRIu32 " bytes (%" PRIu32 " KB)\r\n", flash_size - used, (flash_size - used) / 1024);
  else printf("  Free in flash      : overflow (%" PRIu32 " bytes over)\r\n", used - flash_size);

  printf("\r\n");
  printf("Free gaps:\r\n");

  if (region_count <= 0)
  {
    printf("  none\r\n");
    return;
  }

  uint32_t prev_end = 0;
  bool found_gap = false;

  for (int i = 0; i < region_count; i++)
  {
    uint32_t start = regions[i].address;
    uint32_t end = regions[i].address + regions[i].size;

    if (start > prev_end)
    {
      printf("  %08" PRIX32 "..%08" PRIX32 "  %" PRIu32 " bytes (%" PRIu32 " KB)\r\n", prev_end, start - 1, start - prev_end, (start - prev_end) / 1024);
      found_gap = true;
    }

    if (end > prev_end) prev_end = end;
  }

  if (flash_size > prev_end)
  {
    printf("  %08" PRIX32 "..%08" PRIX32 "  %" PRIu32 " bytes (%" PRIu32 " KB)\r\n", prev_end, flash_size - 1, flash_size - prev_end, (flash_size - prev_end) / 1024);
    found_gap = true;
  }

  if (!found_gap) printf("  none\r\n");
}

int flash_info()
{
  esp_flash_t *chip = esp_flash_default_chip;
  flash_info_data_t info;

  if (!chip)
  {
    printf("Main flash chip is not initialized\r\n");
    return 1;
  }

  flash_collect_info(chip, &info);

  printf("Flash info:\r\n");

  if (info.id_err == ESP_OK)
  {
    const uint8_t mf_id = (info.id >> 16) & 0xFF;
    const uint8_t mem_type = (info.id >> 8) & 0xFF;
    const uint8_t cap_code = info.id & 0xFF;

    printf("  JEDEC ID        : %06" PRIX32 "\r\n", info.id & 0xFFFFFF);
    printf("  Manufacturer    : %02X (%s)\r\n", mf_id, flash_vendor_str(mf_id));
    printf("  Memory type     : %02X\r\n", mem_type);
    printf("  Capacity code   : %02X\r\n", cap_code);
  }
  else
  {
    printf("  JEDEC ID        : n/a (%s)\r\n", esp_err_to_name(info.id_err));
  }

  if (info.size_hdr_err == ESP_OK) printf("  Header size     : %" PRIu32 " bytes (%" PRIu32 " MB)\r\n", info.size_hdr, info.size_hdr / (1024 * 1024));
  else printf("  Header size     : n/a (%s)\r\n", esp_err_to_name(info.size_hdr_err));

  if (info.size_phy_err == ESP_OK) printf("  Physical size   : %" PRIu32 " bytes (%" PRIu32 " MB)\r\n", info.size_phy, info.size_phy / (1024 * 1024));
  else printf("  Physical size   : n/a (%s)\r\n", esp_err_to_name(info.size_phy_err));

  if (info.uid_err == ESP_OK) printf("  Unique ID       : %016" PRIX64 "\r\n", info.uid);
  else if (info.uid_err == ESP_ERR_NOT_SUPPORTED) printf("  Unique ID       : not supported\r\n");
  else printf("  Unique ID       : n/a (%s)\r\n", esp_err_to_name(info.uid_err));

  printf("  Active quad     : %s\r\n", info.quad ? "yes" : "no");
  printf("  Boot mode       : %s\r\n", flash_mode_str());
  printf("  Boot freq       : %s\r\n", flash_freq_str());

#if defined(CONFIG_SPI_FLASH_AUTO_SUSPEND)
  printf("  Auto suspend cfg: enabled\r\n");
#else
  printf("  Auto suspend cfg: disabled\r\n");
#endif

  if (info.chip_suspend_known) printf("  Chip suspend cap: %s\r\n", info.chip_suspend_supported ? "yes" : "no");
  else printf("  Chip suspend cap: n/a\r\n");

  if (info.size_phy_err == ESP_OK)
    printf("  32-bit addr     : %s\r\n", info.size_phy > (16 * 1024 * 1024) ? "required" : "not required");

  if (info.chip_wp_err == ESP_OK) printf("  Chip WProtect   : %s\r\n", info.chip_wp ? "enabled" : "disabled");
  else printf("  Chip WProtect   : n/a (%s)\r\n", esp_err_to_name(info.chip_wp_err));

  printf("\r\n");
  printf("Capabilities:\r\n");
  printf("  Read ID           : %s\r\n", info.id_err == ESP_OK ? "yes" : "no / unknown");
  printf("  Read/Write/Erase  : yes\r\n");
  printf("  Unique ID         : %s\r\n", info.unique_id_supported ? "yes" : "no / unknown");
  printf("  Quad read mode    : %s\r\n", info.quad ? "active" : "inactive");
  if (info.chip_suspend_known) printf("  Suspend/Resume    : %s\r\n", info.chip_suspend_supported ? "supported by chip" : "not supported by chip");
  else printf("  Suspend/Resume    : n/a\r\n");
  if (info.regions_err == ESP_OK) printf("  Region protect    : yes (%" PRIu32 " regions)\r\n", info.regions_total);
  else printf("  Region protect    : unsupported (%s)\r\n", esp_err_to_name(info.regions_err));

  printf("  Part table offset : %08X\r\n", CONFIG_PARTITION_TABLE_OFFSET);
#if defined(CONFIG_PARTITION_TABLE_FILENAME)
  printf("  Part table file   : %s\r\n", CONFIG_PARTITION_TABLE_FILENAME);
#endif

  printf("\r\n");
  flash_print_protect_regions(&info);
  flash_print_partitions((info.size_phy_err == ESP_OK && info.size_phy) ? info.size_phy : info.size_hdr);

  return 0;
}

int fl_cmd(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage:\r\n");
    printf("  fl info\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "info"))
    return flash_info();

  printf("Unknown subcommand: %s\r\n", argv[1]);
  printf("Usage:\r\n");
  printf("  fl info\r\n");
  return 1;
}

int restart(int argc, char **argv)
{
  ESP_LOGI(TAG, "Restarting");
  esp_restart();
}

#define TESTHEAP_SLOTS              256
#define TESTHEAP_DEFAULT_MIN_ALLOC  1
#define TESTHEAP_DEFAULT_MAX_ALLOC  (4 * 1024 * 1024)
#define TESTHEAP_PROBE_STEP         4096

typedef struct
{
  void *ptr;
  size_t size;
} testheap_slot_t;

uint32_t testheap_rnd(uint32_t *state)
{
  uint32_t x = *state;

  if (!x) x = 0xA5A55A5A;

  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;

  *state = x;
  return x;
}

bool testheap_parse_size(const char *s, size_t *out)
{
  if (!s || !s[0] || !out) return false;

  char *end = NULL;
  unsigned long long value = strtoull(s, &end, 0);
  if (end == s) return false;

  unsigned long long mul = 1;

  if (end[0])
  {
    if (!end[1] && (end[0] == 'k' || end[0] == 'K')) mul = 1024ULL;
    else if (!end[1] && (end[0] == 'm' || end[0] == 'M')) mul = 1024ULL * 1024ULL;
    else return false;
  }

  if (value > (unsigned long long)SIZE_MAX / mul) return false;

  *out = (size_t)(value * mul);
  return true;
}

size_t testheap_random_size(uint32_t *state, size_t min_size, size_t max_size)
{
  uint64_t range = (uint64_t)max_size - (uint64_t)min_size + 1;
  uint64_t rnd = ((uint64_t)testheap_rnd(state) << 32) ^ testheap_rnd(state);

  if (range <= 1) return min_size;

  return min_size + (size_t)(rnd % range);
}

int testheap_first_empty_slot(testheap_slot_t *slots, int count)
{
  for (int i = 0; i < count; i++)
    if (!slots[i].ptr)
      return i;

  return -1;
}

int testheap_random_used_slot(testheap_slot_t *slots, int count, uint32_t *state)
{
  int used = 0;

  for (int i = 0; i < count; i++)
    if (slots[i].ptr)
      used++;

  if (!used) return -1;

  int target = testheap_rnd(state) % used;

  for (int i = 0; i < count; i++)
  {
    if (!slots[i].ptr) continue;
    if (!target) return i;
    target--;
  }

  return -1;
}

void testheap_free_slot(testheap_slot_t *slots, int idx, int *active_count, size_t *held_bytes)
{
  if (!slots[idx].ptr) return;

  heap_caps_free(slots[idx].ptr);

  if (*held_bytes >= slots[idx].size) *held_bytes -= slots[idx].size;
  else *held_bytes = 0;

  slots[idx].ptr = NULL;
  slots[idx].size = 0;

  if (*active_count > 0) (*active_count)--;
}

void testheap_touch(void *ptr, size_t size, uint32_t value)
{
  uint8_t *p = (uint8_t *)ptr;

  if (!p || !size) return;

  p[0] = (uint8_t)value;
  p[size - 1] = (uint8_t)(value >> 8);
}

void testheap_print_status(const char *prefix, uint32_t iter, uint32_t ok, uint32_t fail, uint32_t frees, uint32_t slow, int active, size_t held, uint64_t sum_us, uint64_t max_us)
{
  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  uint64_t avg_us = ok ? sum_us / ok : 0;

  printf
  (
    "%s iter=%" PRIu32 " ok=%" PRIu32 " fail=%" PRIu32 " free=%u largest=%u active=%d held=%u avg_us=%" PRIu64 " max_us=%" PRIu64 " frees=%" PRIu32 " slow=%" PRIu32 "\r\n",
    prefix,
    iter,
    ok,
    fail,
    (unsigned)free_size,
    (unsigned)largest,
    active,
    (unsigned)held,
    avg_us,
    max_us,
    frees,
    slow
  );
}

size_t testheap_probe_largest(size_t *free_before, size_t *idf_largest, uint32_t *tries, uint64_t *probe_us)
{
  size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  size_t idf_size = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  size_t first_ok = 0;
  size_t prev_fail = 0;
  size_t result = 0;
  uint32_t cnt = 0;
  int64_t t0 = esp_timer_get_time();

  if (free_before) *free_before = free_size;
  if (idf_largest) *idf_largest = idf_size;

  for (size_t n = free_size; n > 0;)
  {
    cnt++;
    void *ptr = heap_caps_malloc(n, MALLOC_CAP_SPIRAM);

    if (ptr)
    {
      heap_caps_free(ptr);
      first_ok = n;
      break;
    }

    prev_fail = n;

    if (n == 1) break;
    if (n > TESTHEAP_PROBE_STEP) n -= TESTHEAP_PROBE_STEP;
    else n = 1;
  }

  result = first_ok;

  if (first_ok && prev_fail > first_ok)
  {
    size_t hi = prev_fail - 1;

    for (size_t n = hi; n > first_ok; n--)
    {
      cnt++;
      void *ptr = heap_caps_malloc(n, MALLOC_CAP_SPIRAM);

      if (ptr)
      {
        heap_caps_free(ptr);
        result = n;
        break;
      }
    }
  }

  int64_t t1 = esp_timer_get_time();

  if (tries) *tries = cnt;
  if (probe_us) *probe_us = (uint64_t)(t1 - t0);

  return result;
}

void testheap_print_largest_probe()
{
  size_t free_before = 0;
  size_t idf_largest = 0;
  uint32_t tries = 0;
  uint64_t probe_us = 0;
  size_t brute_largest = testheap_probe_largest(&free_before, &idf_largest, &tries, &probe_us);
  int64_t diff = (int64_t)brute_largest - (int64_t)idf_largest;

  printf("\r\n");
  printf("Largest PSRAM allocation probe:\r\n");
  printf("  free before       : %u bytes\r\n", (unsigned)free_before);
  printf("  IDF largest block : %u bytes\r\n", (unsigned)idf_largest);
  printf("  brute largest     : %u bytes\r\n", (unsigned)brute_largest);
  printf("  diff brute-IDF    : %" PRId64 " bytes\r\n", diff);
  printf("  brute tries       : %" PRIu32 "\r\n", tries);
  printf("  brute time        : %" PRIu64 " us\r\n", probe_us);
}

int testheap_cmd(int argc, char **argv)
{
  testheap_slot_t slots[TESTHEAP_SLOTS];
  memset(slots, 0, sizeof(slots));

  size_t min_alloc = TESTHEAP_DEFAULT_MIN_ALLOC;
  size_t max_alloc = TESTHEAP_DEFAULT_MAX_ALLOC;

  if (argc > 3 || (argc >= 2 && !testheap_parse_size(argv[1], &min_alloc)) || (argc >= 3 && !testheap_parse_size(argv[2], &max_alloc)) || min_alloc < 1 || max_alloc < min_alloc)
  {
    printf("Usage:\r\n");
    printf("  testheap [min_size] [max_size]\r\n");
    printf("\r\n");
    printf("Sizes are bytes by default. Suffixes k/K and m/M are accepted.\r\n");
    printf("Default: testheap %u %u\r\n", (unsigned)TESTHEAP_DEFAULT_MIN_ALLOC, (unsigned)TESTHEAP_DEFAULT_MAX_ALLOC);
    return 1;
  }

  uint32_t rng = (uint32_t)esp_timer_get_time() ^ (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
  if (!rng) rng = 0x12345678;

  uint32_t iter = 0;
  uint32_t ok = 0;
  uint32_t fail = 0;
  uint32_t frees = 0;
  uint32_t slow = 0;
  uint32_t slow_prints = 0;
  uint64_t sum_us = 0;
  uint64_t max_us = 0;
  uint64_t avg_us_x1024 = 0;
  size_t held = 0;
  int active = 0;
  bool fragmented = false;
  bool user_stop = false;
  bool no_victim = false;

  printf("\r\n");
  printf("PSRAM heap fragmentation test\r\n");
  printf("  slots       : %d\r\n", TESTHEAP_SLOTS);
  printf("  alloc range : %u..%u bytes\r\n", (unsigned)min_alloc, (unsigned)max_alloc);
  printf("  stop        : fragmentation detected or any key\r\n");
  testheap_print_status("start", iter, ok, fail, frees, slow, active, held, sum_us, max_us);

  while (1)
  {
    char c;
    if (uart_read_bytes(UART_NUM_0, &c, 1, 0) > 0)
    {
      user_stop = true;
      break;
    }

    iter++;

    int slot = testheap_first_empty_slot(slots, TESTHEAP_SLOTS);
    if (slot < 0)
    {
      slot = testheap_random_used_slot(slots, TESTHEAP_SLOTS, &rng);
      if (slot < 0)
      {
        no_victim = true;
        break;
      }

      testheap_free_slot(slots, slot, &active, &held);
      frees++;
    }

    size_t req = testheap_random_size(&rng, min_alloc, max_alloc);
    void *ptr = NULL;
    uint64_t alloc_us = 0;

    while (1)
    {
      int64_t t0 = esp_timer_get_time();
      ptr = heap_caps_malloc(req, MALLOC_CAP_SPIRAM);
      int64_t t1 = esp_timer_get_time();

      alloc_us = (uint64_t)(t1 - t0);

      if (ptr) break;

      fail++;

      size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
      size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

      if (free_size >= req)
      {
        printf
        (
          "\r\nFRAGMENTATION DETECTED: request=%u free=%u largest=%u active=%d held=%u\r\n",
          (unsigned)req,
          (unsigned)free_size,
          (unsigned)largest,
          active,
          (unsigned)held
        );

        fragmented = true;
        break;
      }

      int victim = testheap_random_used_slot(slots, TESTHEAP_SLOTS, &rng);
      if (victim < 0)
      {
        printf
        (
          "\r\nALLOC FAILED: request=%u free=%u largest=%u, no buffers to free\r\n",
          (unsigned)req,
          (unsigned)free_size,
          (unsigned)largest
        );

        no_victim = true;
        break;
      }

      testheap_free_slot(slots, victim, &active, &held);
      frees++;
    }

    if (fragmented || no_victim) break;

    slots[slot].ptr = ptr;
    slots[slot].size = req;
    active++;
    held += req;
    testheap_touch(ptr, req, iter);

    ok++;
    sum_us += alloc_us;
    if (alloc_us > max_us) max_us = alloc_us;

    uint64_t prev_avg_us_x1024 = avg_us_x1024 ? avg_us_x1024 : alloc_us * 1024;
    bool slow_alloc = ok > 32 && alloc_us > 1000 && alloc_us * 1024 > prev_avg_us_x1024 * 8;

    if (!avg_us_x1024) avg_us_x1024 = alloc_us * 1024;
    else avg_us_x1024 = (avg_us_x1024 * 63 + alloc_us * 1024) / 64;

    if (slow_alloc)
    {
      slow++;

      if (slow_prints < 16)
      {
        size_t free_size = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);

        printf
        (
          "SLOW ALLOC: iter=%" PRIu32 " request=%u alloc_us=%" PRIu64 " avg_us=%" PRIu64 " free=%u largest=%u\r\n",
          iter,
          (unsigned)req,
          alloc_us,
          prev_avg_us_x1024 / 1024,
          (unsigned)free_size,
          (unsigned)largest
        );

        slow_prints++;
      }
    }

    if ((ok & 0xFF) == 0)
      testheap_print_status("run  ", iter, ok, fail, frees, slow, active, held, sum_us, max_us);
  }

  testheap_print_status("stop ", iter, ok, fail, frees, slow, active, held, sum_us, max_us);
  testheap_print_largest_probe();

  for (int i = 0; i < TESTHEAP_SLOTS; i++)
    testheap_free_slot(slots, i, &active, &held);

  if (fragmented) printf("Result: stopped on fragmentation condition\r\n");
  else if (user_stop) printf("Result: stopped by user\r\n");
  else if (no_victim) printf("Result: stopped, no buffer can be freed\r\n");
  else printf("Result: stopped\r\n");

  testheap_print_status("final", iter, ok, fail, frees, slow, active, held, sum_us, max_us);

  return fragmented ? 1 : 0;
}

void print_heap_region_info(const char *name, uint32_t caps)
{
  multi_heap_info_t info = {};
  heap_caps_get_info(&info, caps);

  size_t total = heap_caps_get_total_size(caps);
  size_t fragmented = 0;
  unsigned fragmented_pct = 0;

  if (info.total_free_bytes > info.largest_free_block)
    fragmented = info.total_free_bytes - info.largest_free_block;

  if (info.total_free_bytes)
    fragmented_pct = (unsigned)((fragmented * 100) / info.total_free_bytes);

  printf(" %s:\r\n", name);
  printf("   total          : %u\r\n", (unsigned)total);
  printf("   free           : %u\r\n", (unsigned)info.total_free_bytes);
  printf("   allocated      : %u\r\n", (unsigned)info.total_allocated_bytes);
  printf("   largest free   : %u\r\n", (unsigned)info.largest_free_block);
  printf("   minimum free   : %u\r\n", (unsigned)info.minimum_free_bytes);
  printf("   fragmented     : %u (%u%%)\r\n", (unsigned)fragmented, fragmented_pct);
  printf("   free blocks    : %u\r\n", (unsigned)info.free_blocks);
  printf("   alloc blocks   : %u\r\n", (unsigned)info.allocated_blocks);
  printf("   total blocks   : %u\r\n", (unsigned)info.total_blocks);
}

int mem_info(int argc, char **argv)
{
  printf("Heap free/total:\r\n");
  printf(" Default:\t%u/%u\r\n",    heap_caps_get_free_size(MALLOC_CAP_DEFAULT), heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
  printf(" 32-bit:\t%u/%u\r\n",     heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_total_size(MALLOC_CAP_32BIT));
  printf(" 8-bit:\t\t%u/%u\r\n",    heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_total_size(MALLOC_CAP_8BIT));
  printf(" SPI RAM:\t%u/%u\r\n",    heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
  printf(" SRAM:\t\t%u/%u\r\n",     heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_total_size(MALLOC_CAP_INTERNAL));
  printf(" DMA:\t\t%u/%u\r\n",      heap_caps_get_free_size(MALLOC_CAP_DMA), heap_caps_get_total_size(MALLOC_CAP_DMA));
  printf(" RTC fast:\t%u/%u\r\n",   heap_caps_get_free_size(MALLOC_CAP_RTCRAM), heap_caps_get_total_size(MALLOC_CAP_RTCRAM));
  printf(" Retention:\t%u/%u\r\n",  heap_caps_get_free_size(MALLOC_CAP_RETENTION), heap_caps_get_total_size(MALLOC_CAP_RETENTION));
  printf(" IRAM 8-bit:\t%u/%u\r\n", heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT), heap_caps_get_total_size(MALLOC_CAP_IRAM_8BIT));
  printf(" Exec:\t\t%u/%u\r\n",     heap_caps_get_free_size(MALLOC_CAP_EXEC), heap_caps_get_total_size(MALLOC_CAP_EXEC));

  printf("\r\nHeap details:\r\n");
  print_heap_region_info("SRAM", MALLOC_CAP_INTERNAL);
  print_heap_region_info("SPI RAM", MALLOC_CAP_SPIRAM);

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

int nvs_show()
{
  esp_err_t err = app_params_load();
  if (err != ESP_OK)
  {
    printf("app_params_load failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("NVS params:\r\n");
  printf("  usb_mode : %u\r\n", app_params.usb_mode);
  printf("  wifi_mode: %u\r\n", app_params.wifi_mode);
  printf("  wifi_ap  : %s\r\n", app_params.wifi_ap[0] ? app_params.wifi_ap : "");
  printf("  wifi_psw : %s\r\n", app_params.wifi_psw[0] ? app_params.wifi_psw : "");

  return 0;
}

int nvs_cmd(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage:\r\n");
    printf("  nvs show\r\n");
    printf("  nvs set <name> <value>\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "show"))
    return nvs_show();

  if (!strcmp(argv[1], "set"))
  {
    if (argc < 4)
    {
      printf("Usage: nvs set <name> <value>\r\n");
      return 1;
    }

    esp_err_t err = app_params_load();
    if (err != ESP_OK)
    {
      printf("app_params_load failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    if (!app_params_set_by_name(argv[2], argv[3]))
    {
      printf("Bad name/value: %s = %s\r\n", argv[2], argv[3]);
      return 1;
    }

    err = app_params_save();
    if (err != ESP_OK)
    {
      printf("app_params_save failed: %s\r\n", esp_err_to_name(err));
      return 1;
    }

    printf("Saved: %s = %s\r\n", argv[2], argv[3]);
    return 0;
  }

  printf("Unknown subcommand: %s\r\n", argv[1]);
  printf("Usage:\r\n");
  printf("  nvs show\r\n");
  printf("  nvs set <name> <value>\r\n");
  return 1;
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
      .command = "testheap",
      .help    = "PSRAM heap fragmentation test",
      .hint    = NULL,
      .func    = &testheap_cmd,
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
      .command  = "nvs",
      .help     = "NVS commands: show/set",
      .hint     = NULL,
      .func     = &nvs_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command  = "fl",
      .help     = "Flash commands: info",
      .hint     = NULL,
      .func     = &fl_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }

  {
    const esp_console_cmd_t cmd =
    {
      .command  = "usb",
      .help     = "USB commands: en",
      .hint     = NULL,
      .func     = &usb_cmd,
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
