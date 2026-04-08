
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "sdmmc.h"
#include "sdmmc_cmd.h"
#include <dirent.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "esp_heap_caps.h"
#include "esp_console.h"
#include "esp_log.h"

sdmmc_host_t sd_host;
sdmmc_slot_config_t sd_slot;
sdmmc_card_t sd_card;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
#include "sd_pwr_ctrl.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "sd_pwr_ctrl_interface.h"
#include "esp_ldo_regulator.h"

typedef struct
{
  esp_ldo_channel_handle_t ldo_chan;
  int voltage_mv;
} sd_pwr_ctrl_ldo_ctx_t;

sd_pwr_ctrl_handle_t sd_pwr = NULL;

void sd_ldo_init()
{
  if (!sd_pwr)
  {
    sd_pwr_ctrl_ldo_config_t ldo_cfg = {};
    ldo_cfg.ldo_chan_id = 4,

    esp_err_t err = sd_pwr_ctrl_new_on_chip_ldo(&ldo_cfg, &sd_pwr);
    if (err != ESP_OK)
      printf("E: sd_pwr_ctrl_new_on_chip_ldo failed: %s\r\n", esp_err_to_name(err));
  }

  sd_host.pwr_ctrl_handle = sd_pwr;
#endif

void sd_setup()
{
  sd_host = SDMMC_HOST_DEFAULT();
  sd_host.slot = SD_SLOT;
  sd_host.max_freq_khz = 40000;

  sd_slot = SDMMC_SLOT_CONFIG_DEFAULT();
  sd_slot.width = 4;
  sd_slot.clk = SD_CLK;
  sd_slot.cmd = SD_CMD;
  sd_slot.d0  = SD_D0;
  sd_slot.d1  = SD_D1;
  sd_slot.d2  = SD_D2;
  sd_slot.d3  = SD_D3;
  sd_slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
}

esp_err_t sd_init()
{
  esp_err_t err;
  esp_log_level_t old_sd_host_level;

  sd_setup();

  old_sd_host_level = esp_log_level_get("SD_HOST");
  esp_log_level_set("SD_HOST", ESP_LOG_ERROR);

  err = sdmmc_host_init();
  if (err != ESP_OK)
  {
    esp_log_level_set("SD_HOST", old_sd_host_level);
    printf("E: sdmmc_host_init failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  err = sdmmc_host_init_slot(SD_SLOT, &sd_slot);
  if (err != ESP_OK)
  {
    esp_log_level_set("SD_HOST", old_sd_host_level);
    printf("E: sdmmc_host_init_slot failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    return err;
  }

  err = sdmmc_card_init(&sd_host, &sd_card);
  esp_log_level_set("SD_HOST", old_sd_host_level);

  if (err != ESP_OK)
  {
    printf("E: sdmmc_card_init failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    return err;
  }

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  printf("SD voltage = %dmv\r\n", ((sd_pwr_ctrl_ldo_ctx_t *)sd_pwr->ctx)->voltage_mv);
#endif

  return ESP_OK;
}

void sd_deinit()
{
  sdmmc_host_deinit();
}

esp_err_t sd_card_erase()
{
  esp_err_t err;
  {
    uint64_t sector_count = (uint64_t)sd_card.csd.capacity;
    uint32_t sector_size = (uint32_t)sd_card.csd.sector_size;
    uint64_t size_mib = (sector_count * (uint64_t)sector_size) / (1024ull * 1024ull);

    printf("Erasing SD card: %llu MiB, sectors=%llu, sector_size=%u\r\n",
      (unsigned long long)size_mib,
      (unsigned long long)sector_count,
      (unsigned)sector_size);
  }

  err = sdmmc_full_erase(&sd_card);
  if (err != ESP_OK)
  {
    printf("E: sdmmc_full_erase failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  printf("Erase done\r\n");
  return ESP_OK;
}

int sd_read_sectors(uint32_t sec, uint32_t num)
{
  esp_err_t err;
  if (num == 0)
  {
    printf("num must be > 0\r\n");
    return 1;
  }

  uint64_t sector_count = (uint64_t)sd_card.csd.capacity;
  uint32_t sector_size  = (uint32_t)sd_card.csd.sector_size;

  if ((uint64_t)sec >= sector_count || (uint64_t)sec + (uint64_t)num > sector_count)
  {
    printf("Out of range: sec=%u num=%u (capacity=%llu)\r\n",
      (unsigned)sec, (unsigned)num, (unsigned long long)sector_count);
    return 1;
  }

  size_t total_bytes = (size_t)((uint64_t)num * (uint64_t)sector_size);
  uint8_t *buf = (uint8_t *)malloc(total_bytes);
  if (!buf)
  {
    printf("malloc(%u) failed\r\n", (unsigned)total_bytes);
    return 1;
  }

  printf("Reading: sec=%u num=%u (%u bytes/sector, total=%u)\r\n",
         (unsigned)sec,
         (unsigned)num,
         (unsigned)sector_size,
         (unsigned)total_bytes);

  err = sdmmc_read_sectors(&sd_card, buf, sec, num);
  if (err != ESP_OK)
  {
    printf("E: sdmmc_read_sectors failed: %s\r\n", esp_err_to_name(err));
    free(buf);
    return (int)err;
  }

  // void hexdump(const void *data, size_t len, uint64_t base_off);
  // hexdump(buf, total_bytes, (uint64_t)sec * (uint64_t)sector_size);

  free(buf);

  return 0;
}

esp_err_t sd_fs_mount(const char *base_path, sdmmc_card_t **out_card)
{
  esp_err_t err;

  sd_setup();

  esp_vfs_fat_mount_config_t mount_cfg =
  {
    .format_if_mount_failed = false,
    .max_files = 4,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = true,
  };

  err = esp_vfs_fat_sdmmc_mount(base_path, &sd_host, &sd_slot, &mount_cfg, out_card);
  if (err != ESP_OK)
  {
    printf("E: esp_vfs_fat_sdmmc_mount failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

void sd_fs_unmount(const char *base_path, sdmmc_card_t *card)
{
  esp_vfs_fat_sdcard_unmount(base_path, card);
}

int sd_fs_list_dir(const char *base_path, const char *path)
{
  char full[256];

  if (!path || path[0] == 0 || strcmp(path, "/") == 0)
  {
    snprintf(full, sizeof(full), "%s", base_path);
  }
  else if (path[0] == '/')
  {
    snprintf(full, sizeof(full), "%s%s", base_path, path);
  }
  else
  {
    snprintf(full, sizeof(full), "%s/%s", base_path, path);
  }

  DIR *d = opendir(full);
  if (!d)
  {
    printf("E: opendir('%s') failed\r\n", full);
    return 1;
  }

  printf("Listing: %s\r\n", full);

  for (;;)
  {
    struct dirent *e = readdir(d);
    if (!e) break;

    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
      continue;

    char p[256];
    int n = snprintf(p, sizeof(p), "%s/%s", full, e->d_name);
    if (n < 0 || (size_t)n >= sizeof(p))
    {
      printf("  ?    ?          %s (path too long)\r\n", e->d_name);
      continue;
    }

    struct stat st;
    if (stat(p, &st) == 0)
    {
      const char *t = S_ISDIR(st.st_mode) ? "DIR " : "FILE";
      printf("  %s  %10" PRIu32 "  %s\r\n", t, (uint32_t)st.st_size, e->d_name);
    }
    else
      printf("  ?    ?          %s\r\n", e->d_name);
  }

  closedir(d);
  return 0;
}

const char *sd_mid_to_name(uint8_t mid)
{
  switch (mid)
  {
    case 0x00: return "Generic";
    case 0x01: return "Panasonic";
    case 0x02: return "Toshiba / Kioxia";
    case 0x03: return "SanDisk / WD";
    case 0x05: return "Lenovo";
    case 0x06: return "SanDisk Extreme Pro / Sabrent";
    case 0x09: return "ATP";
    case 0x12: return "Patriot";
    case 0x1B: return "Samsung";
    case 0x1D: return "ADATA";
    case 0x27: return "Phison OEM (Delkin/HP/Integral/Kingston/Lexar/PNY/...)";
    case 0x28: return "Lexar (Longsys)";
    case 0x31: return "Silicon Power";
    case 0x41: return "Kingston";
    case 0x45: return "TEAMGROUP";
    case 0x56: return "SanDian / various";
    case 0x6F: return "Hiksemi / HP / Kodak / Lenovo / Netac";
    case 0x74: return "Transcend / Gigastone";
    case 0x76: return "PNY / Patriot";
    case 0x82: return "Sony";
    case 0x89: return "Netac / Intel";
    case 0x90: return "Strontium";
    case 0x92: return "Verbatim";
    case 0x9B: return "Patriot";
    case 0x9C: return "Angelbird / Hoodman";
    case 0xB6: return "Delkin Devices";
    default:   return "Unknown";
  }
}

void sd_log_cid(const sdmmc_cid_t *cid)
{
  char pnm[sizeof(cid->name) + 1];
  memcpy(pnm, cid->name, sizeof(cid->name));
  pnm[sizeof(cid->name)] = 0;

  char oid[3];
  oid[0] = (char)((cid->oem_id >> 8) & 0xFF);
  oid[1] = (char)(cid->oem_id & 0xFF);
  oid[2] = 0;

  bool oid_printable = isprint((unsigned char)oid[0]) && isprint((unsigned char)oid[1]);

  uint8_t prv_major = (cid->revision >> 4) & 0x0F;
  uint8_t prv_minor = cid->revision & 0x0F;

  uint16_t mdt = cid->date;
  uint16_t year = (uint16_t)(2000u + (mdt >> 4));
  uint8_t month = (uint8_t)(mdt & 0x0F);

  printf("CID decode:\r\n");
  printf("  MID (manufacturer) : 0x%02X (%s)\r\n", cid->mfg_id, sd_mid_to_name(cid->mfg_id));

  if (oid_printable)
  {
    printf("  OID (OEM/app)      : \"%s\" (raw 0x%04X)\r\n", oid, (unsigned)cid->oem_id);
  }
  else
  {
    printf("  OID (OEM/app)      : raw 0x%04X (bytes %02X %02X)\r\n",
           (unsigned)cid->oem_id,
           (unsigned)((cid->oem_id >> 8) & 0xFF),
           (unsigned)(cid->oem_id & 0xFF));
  }

  printf("  PNM (product)      : \"%s\"\r\n", pnm);
  printf("  PRV (revision)     : %u.%u (raw 0x%02X)\r\n",
         (unsigned)prv_major,
         (unsigned)prv_minor,
         (unsigned)cid->revision);
  printf("  PSN (serial)       : 0x%08" PRIX32 "\r\n", (uint32_t)cid->serial);

  if (month >= 1 && month <= 12)
  {
    printf("  MDT (date)         : %04u-%02u (raw 0x%03X)\r\n",
           (unsigned)year,
           (unsigned)month,
           (unsigned)(mdt & 0x0FFF));
  }
  else
  {
    printf("  MDT (date)         : %04u-?? (raw 0x%03X)\r\n",
           (unsigned)year,
           (unsigned)(mdt & 0x0FFF));
  }
}

void sd_log_ocr(const sdmmc_card_t *card)
{
  uint32_t ocr = card->ocr;

  unsigned busy = (ocr >> 31) & 1;   // power-up status (1=ready)
  unsigned ccs  = (ocr >> 30) & 1;   // SDHC/SDXC
  unsigned bit29 = (ocr >> 29) & 1;  // UHS-II / (в некоторых доках) FastBoot/reserved
  unsigned xpc  = (ocr >> 28) & 1;   // XPC
  // unsigned s18a = (ocr >> 24) & 1;   // 1.8V switching accepted

  printf("OCR decode:\r\n");
  printf("  raw   : 0x%08" PRIX32 "\r\n", ocr);
  printf("  BUSY  : %u (%s)\r\n", busy, busy ? "ready" : "busy");
  printf("  CCS   : %u (%s)\r\n", ccs, ccs ? "SDHC/SDXC (block addressing)" : "SDSC (byte addressing)");
  printf("  BIT29 : %u (UHS-II / reserved / fast-boot, depends on spec)\r\n", bit29);
  printf("  XPC   : %u\r\n", xpc);
  // printf("  S18A  : %u (1.8V switch %s)\r\n", s18a, s18a ? "accepted" : "not accepted");

  if (!busy)
  {
    printf("  note  : CCS/S18A/BIT29 are valid when BUSY=1\r\n");
  }

  // Voltage window (OCR[23:0]). Most SD cards use high-voltage 2.7–3.6V (bits 15..23).
  // Table mapping is commonly: 15=2.7-2.8 ... 23=3.5-3.6. :contentReference[oaicite:2]{index=2}
  static const struct
  {
    uint8_t bit;
    const char *range;
  } vtbl[] =
  {
    { 4,  "1.6-1.7" }, { 5,  "1.7-1.8" }, { 6,  "1.8-1.9" }, { 7,  "1.9-2.0" },
    { 8,  "2.0-2.1" }, { 9,  "2.1-2.2" }, { 10, "2.2-2.3" }, { 11, "2.3-2.4" },
    { 12, "2.4-2.5" }, { 13, "2.5-2.6" }, { 14, "2.6-2.7" },
    { 15, "2.7-2.8" }, { 16, "2.8-2.9" }, { 17, "2.9-3.0" }, { 18, "3.0-3.1" },
    { 19, "3.1-3.2" }, { 20, "3.2-3.3" }, { 21, "3.3-3.4" }, { 22, "3.4-3.5" },
    { 23, "3.5-3.6" },
  };

  bool any = false;
  printf("  VDD window bits set:\r\n");
  for (size_t i = 0; i < sizeof(vtbl) / sizeof(vtbl[0]); i++)
  {
    if (ocr & (1u << vtbl[i].bit))
    {
      printf("    OCR[%u] = 1 -> %s V\r\n", (unsigned)vtbl[i].bit, vtbl[i].range);
      any = true;
    }
  }
  if (!any)
  {
    printf("    (none)\r\n");
  }
}

void sd_log_scr(const sdmmc_card_t *sd_card)
{
  const sdmmc_scr_t *s = &sd_card->scr;

  printf("SCR decode:\r\n");
  printf("  sd_spec          = %u\r\n", (unsigned)s->sd_spec);
  printf("  erase_mem_state  = %u\r\n", (unsigned)s->erase_mem_state);
  printf("  bus_width bitmap = 0x%X\r\n", (unsigned)s->bus_width);
  printf("  rsvd_mnf         = 0x%08" PRIx32 "\r\n", (uint32_t)s->rsvd_mnf);
}

// ------------- Console ---------------

int sd_info(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  if (sd_init() != ESP_OK) return 1;

  sdmmc_card_print_info(stdout, &sd_card);
  sd_log_cid(&sd_card.cid);
  sd_log_ocr(&sd_card);
  sd_log_scr(&sd_card);

  sd_deinit();
  return 0;
}

int sd_read(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Usage: sd read <sec> <num>\r\n");
    return 1;
  }

  char *endp = NULL;
  uint64_t sec = strtoull(argv[2], &endp, 0);
  if (!endp || *endp)
  {
    printf("Bad <sec>: %s\r\n", argv[2]);
    return 1;
  }

  endp = NULL;
  uint64_t num = strtoull(argv[3], &endp, 0);
  if (!endp || *endp || num == 0)
  {
    printf("Bad <num>: %s\r\n", argv[3]);
    return 1;
  }

  if (sd_init() != ESP_OK) return 1;

  int rc = sd_read_sectors((uint32_t)sec, (uint32_t)num);
  sd_deinit();
  return rc;
}

int sd_erase(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  if (sd_init() != ESP_OK) return 1;

  esp_err_t err = sd_card_erase();
  sd_deinit();
  return (err == ESP_OK) ? 0 : 1;
}

int sd_ls(int argc, char **argv)
{
  const char *base = "/sd";
  const char *path = "/";

  if (argc >= 3)
    path = argv[2];

  sdmmc_card_t *card = NULL;
  esp_err_t err = sd_fs_mount(base, &card);
  if (err != ESP_OK) return 1;

  int rc = sd_fs_list_dir(base, path);

  sd_fs_unmount(base, card);
  return rc;
}

int sd_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  sd info\r\n");
    printf("  sd erase\r\n");
    printf("  sd read <sec> <num>\r\n");
    printf("  sd ls [path]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "info"))
    return sd_info(argc, argv);

  if (!strcmp(op, "erase"))
    return sd_erase(argc, argv);

  if (!strcmp(op, "read"))
    return sd_read(argc, argv);

  if (!strcmp(argv[1], "ls"))
    return sd_ls(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void sdmmc_console_register_system_commands()
{
  {
    const esp_console_cmd_t cmd =
    {
      .command  = "sd",
      .help = "SD card commands: info/erase/read/diag'",
      .hint     = NULL,
      .func     = &sd_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
  }
}
