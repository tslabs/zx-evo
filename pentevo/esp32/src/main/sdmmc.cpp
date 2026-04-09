#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
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
sdmmc_card_t *sd_card_ptr = NULL;
bool sd_initialized = false;
bool sd_fs_mounted = false;
char sd_fs_base_path[32] = { 0 };
sdmmc_card_t *sd_fs_card = NULL;

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
    ldo_cfg.ldo_chan_id = 4;

    esp_err_t err = sd_pwr_ctrl_new_on_chip_ldo(&ldo_cfg, &sd_pwr);
    if (err != ESP_OK)
      printf("E: sd_pwr_ctrl_new_on_chip_ldo failed: %s\r\n", esp_err_to_name(err));
  }

  sd_host.pwr_ctrl_handle = sd_pwr;
}
#endif

esp_log_level_t sd_host_log_suppress_begin()
{
  esp_log_level_t old_sd_host_level = esp_log_level_get("SD_HOST");
  esp_log_level_set("SD_HOST", ESP_LOG_ERROR);
  return old_sd_host_level;
}

void sd_host_log_suppress_end(esp_log_level_t old_sd_host_level)
{
  esp_log_level_set("SD_HOST", old_sd_host_level);
}

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

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  sd_ldo_init();
#endif
}

bool sd_has_retryable_errno(int err)
{
  return err == EIO || err == ENODEV || err == ENXIO || err == EBADF;
}

bool sd_error_needs_reinit(esp_err_t err)
{
  if (err == ESP_OK) return false;
  if (err == ESP_ERR_INVALID_ARG) return false;
  if (err == ESP_ERR_INVALID_SIZE) return false;
  if (err == ESP_ERR_NO_MEM) return false;
  if (err == ESP_ERR_NOT_FOUND) return false;
  return true;
}

esp_err_t sd_probe_card()
{
  if (!sd_initialized || !sd_card_ptr) return ESP_ERR_INVALID_STATE;
  return sdmmc_get_status(sd_card_ptr);
}

void sd_deinit()
{
  bool was_fs_mounted = sd_fs_mounted;

  if (sd_fs_mounted)
  {
    esp_vfs_fat_sdcard_unmount(sd_fs_base_path, sd_fs_card);
    sd_fs_mounted = false;
    sd_fs_card = NULL;
    sd_fs_base_path[0] = 0;
  }

  if (sd_initialized && !was_fs_mounted)
    sdmmc_host_deinit();

  memset(&sd_card, 0, sizeof(sd_card));
  sd_card_ptr = NULL;
  sd_initialized = false;
}

esp_err_t sd_init()
{
  esp_err_t err;
  esp_log_level_t old_sd_host_level;

  if (sd_initialized) return ESP_OK;

  sd_setup();

  old_sd_host_level = sd_host_log_suppress_begin();

  err = sdmmc_host_init();
  if (err != ESP_OK)
  {
    sd_host_log_suppress_end(old_sd_host_level);
    printf("E: sdmmc_host_init failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  err = sdmmc_host_init_slot(SD_SLOT, &sd_slot);
  if (err != ESP_OK)
  {
    sd_host_log_suppress_end(old_sd_host_level);
    printf("E: sdmmc_host_init_slot failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    return err;
  }

  err = sdmmc_card_init(&sd_host, &sd_card);
  sd_host_log_suppress_end(old_sd_host_level);

  if (err != ESP_OK)
  {
    printf("E: sdmmc_card_init failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    return err;
  }

  sd_card_ptr = &sd_card;
  sd_initialized = true;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  printf("SD voltage = %dmv\r\n", ((sd_pwr_ctrl_ldo_ctx_t *)sd_pwr->ctx)->voltage_mv);
#endif

  return ESP_OK;
}

esp_err_t sd_reinit()
{
  sd_deinit();
  return sd_init();
}

esp_err_t sd_ensure_ready()
{
  esp_err_t err = sd_init();
  if (err != ESP_OK) return err;

  err = sd_probe_card();
  if (err == ESP_OK) return ESP_OK;

  printf("W: SD probe failed: %s, reinit\r\n", esp_err_to_name(err));
  return sd_reinit();
}

esp_err_t sd_card_erase()
{
  esp_err_t err;
  sdmmc_card_t *card = sd_card_ptr;

  if (!card) return ESP_ERR_INVALID_STATE;

  {
    uint64_t sector_count = (uint64_t)card->csd.capacity;
    uint32_t sector_size = (uint32_t)card->csd.sector_size;
    uint64_t size_mib = (sector_count * (uint64_t)sector_size) / (1024ull * 1024ull);

    printf("Erasing SD card: %llu MiB, sectors=%llu, sector_size=%u\r\n",
      (unsigned long long)size_mib,
      (unsigned long long)sector_count,
      (unsigned)sector_size);
  }

  err = sdmmc_full_erase(card);
  if (err != ESP_OK)
  {
    printf("E: sdmmc_full_erase failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  printf("Erase done\r\n");
  return ESP_OK;
}

esp_err_t sd_read_sectors(uint32_t sec, uint32_t num)
{
  esp_err_t err;
  sdmmc_card_t *card = sd_card_ptr;

  if (!card) return ESP_ERR_INVALID_STATE;

  if (num == 0)
  {
    printf("num must be > 0\r\n");
    return ESP_ERR_INVALID_ARG;
  }

  uint64_t sector_count = (uint64_t)card->csd.capacity;
  uint32_t sector_size  = (uint32_t)card->csd.sector_size;

  if ((uint64_t)sec >= sector_count || (uint64_t)sec + (uint64_t)num > sector_count)
  {
    printf("Out of range: sec=%u num=%u (capacity=%llu)\r\n",
      (unsigned)sec, (unsigned)num, (unsigned long long)sector_count);
    return ESP_ERR_INVALID_SIZE;
  }

  size_t total_bytes = (size_t)((uint64_t)num * (uint64_t)sector_size);
  uint8_t *buf = (uint8_t *)malloc(total_bytes);
  if (!buf)
  {
    printf("malloc(%u) failed\r\n", (unsigned)total_bytes);
    return ESP_ERR_NO_MEM;
  }

  printf("Reading: sec=%u num=%u (%u bytes/sector, total=%u)\r\n",
         (unsigned)sec,
         (unsigned)num,
         (unsigned)sector_size,
         (unsigned)total_bytes);

  err = sdmmc_read_sectors(card, buf, sec, num);
  if (err != ESP_OK)
  {
    printf("E: sdmmc_read_sectors failed: %s\r\n", esp_err_to_name(err));
    free(buf);
    return err;
  }

  free(buf);
  return ESP_OK;
}

esp_err_t sd_fs_mount(const char *base_path, sdmmc_card_t **out_card)
{
  esp_err_t err;
  esp_log_level_t old_sd_host_level;

  if (sd_fs_mounted)
  {
    if (!base_path || strcmp(sd_fs_base_path, base_path) == 0)
    {
      if (out_card)
        *out_card = sd_fs_card;
      return ESP_OK;
    }

    sd_deinit();
  }

  if (sd_initialized)
    sd_deinit();

  sd_setup();

  esp_vfs_fat_mount_config_t mount_cfg =
  {
    .format_if_mount_failed = false,
    .max_files = 4,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = true,
  };

  old_sd_host_level = sd_host_log_suppress_begin();
  err = esp_vfs_fat_sdmmc_mount(base_path, &sd_host, &sd_slot, &mount_cfg, &sd_fs_card);
  sd_host_log_suppress_end(old_sd_host_level);

  if (err != ESP_OK)
  {
    printf("E: esp_vfs_fat_sdmmc_mount failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    sd_fs_card = NULL;
    sd_card_ptr = NULL;
    sd_initialized = false;
    return err;
  }

  sd_initialized = true;
  sd_fs_mounted = true;
  sd_card_ptr = sd_fs_card;
  snprintf(sd_fs_base_path, sizeof(sd_fs_base_path), "%s", base_path ? base_path : "");

  if (out_card)
    *out_card = sd_fs_card;

  return ESP_OK;
}

void sd_fs_unmount(const char *base_path, sdmmc_card_t *card)
{
  (void)base_path;
  (void)card;
  sd_deinit();
}

int sd_fs_build_full_path(const char *base_path, const char *path, char *full, size_t full_size)
{
  int n;

  if (!base_path || !base_path[0] || !full || full_size < 2) return 0;

  if (!path || !path[0] || strcmp(path, "/") == 0)
  {
    n = snprintf(full, full_size, "%s", base_path);
    return n >= 0 && (size_t)n < full_size;
  }

  if (path[0] == '/')
  {
    n = snprintf(full, full_size, "%s%s", base_path, path);
    return n >= 0 && (size_t)n < full_size;
  }

  n = snprintf(full, full_size, "%s/%s", base_path, path);
  return n >= 0 && (size_t)n < full_size;
}

esp_err_t sd_fs_read_file_once(const char *base_path, const char *path, void *dst, size_t dst_size, size_t *out_size)
{
  char full[256];
  sdmmc_card_t *card = NULL;

  if (!sd_fs_build_full_path(base_path, path, full, sizeof(full)))
    return ESP_ERR_INVALID_ARG;

  esp_err_t err = sd_fs_mount(base_path, &card);
  if (err != ESP_OK) return err;

  struct stat st = {};
  if (stat(full, &st) != 0)
  {
    int saved_errno = errno;
    printf("E: stat('%s') failed, errno=%d\r\n", full, saved_errno);
    sd_fs_unmount(base_path, card);
    if (sd_has_retryable_errno(saved_errno))
      return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
  }

  if (st.st_size <= 0)
  {
    printf("E: bad file size for '%s': %ld\r\n", full, (long)st.st_size);
    sd_fs_unmount(base_path, card);
    return ESP_ERR_INVALID_SIZE;
  }

  if (out_size)
    *out_size = (size_t)st.st_size;

  if (!dst)
  {
    sd_fs_unmount(base_path, card);
    return ESP_OK;
  }

  if (dst_size < (size_t)st.st_size)
  {
    printf("E: buffer too small for '%s': have=%u need=%u\r\n",
      full,
      (unsigned)dst_size,
      (unsigned)st.st_size);
    sd_fs_unmount(base_path, card);
    return ESP_ERR_INVALID_SIZE;
  }

  FILE *f = fopen(full, "rb");
  if (!f)
  {
    int saved_errno = errno;
    printf("E: fopen('%s') failed, errno=%d\r\n", full, saved_errno);
    sd_fs_unmount(base_path, card);
    if (sd_has_retryable_errno(saved_errno))
      return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
  }

  size_t rd = fread(dst, 1, (size_t)st.st_size, f);
  fclose(f);
  sd_fs_unmount(base_path, card);

  if (rd != (size_t)st.st_size)
  {
    printf("E: fread('%s') failed, got %u of %u bytes\r\n",
      full,
      (unsigned)rd,
      (unsigned)st.st_size);
    return ESP_FAIL;
  }

  return ESP_OK;
}

esp_err_t sd_fs_read_file(const char *base_path, const char *path, void *dst, size_t dst_size, size_t *out_size)
{
  esp_err_t err = sd_fs_read_file_once(base_path, path, dst, dst_size, out_size);

  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD file read failed: %s, retry\r\n", esp_err_to_name(err));
    sd_deinit();
    err = sd_fs_read_file_once(base_path, path, dst, dst_size, out_size);
  }

  return err;
}

esp_err_t sd_fs_list_dir(const char *base_path, const char *path)
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

  errno = 0;
  DIR *d = opendir(full);
  if (!d)
  {
    int saved_errno = errno;
    printf("E: opendir('%s') failed, errno=%d\r\n", full, saved_errno);
    if (sd_has_retryable_errno(saved_errno))
      return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
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
  return ESP_OK;
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

  unsigned busy = (ocr >> 31) & 1;
  unsigned ccs  = (ocr >> 30) & 1;
  unsigned bit29 = (ocr >> 29) & 1;
  unsigned xpc  = (ocr >> 28) & 1;

  printf("OCR decode:\r\n");
  printf("  raw   : 0x%08" PRIX32 "\r\n", ocr);
  printf("  BUSY  : %u (%s)\r\n", busy, busy ? "ready" : "busy");
  printf("  CCS   : %u (%s)\r\n", ccs, ccs ? "SDHC/SDXC (block addressing)" : "SDSC (byte addressing)");
  printf("  BIT29 : %u (UHS-II / reserved / fast-boot, depends on spec)\r\n", bit29);
  printf("  XPC   : %u\r\n", xpc);

  if (!busy)
  {
    printf("  note  : CCS/S18A/BIT29 are valid when BUSY=1\r\n");
  }

  const struct
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

void sd_log_scr(const sdmmc_card_t *card)
{
  const sdmmc_scr_t *s = &card->scr;

  printf("SCR decode:\r\n");
  printf("  sd_spec          = %u\r\n", (unsigned)s->sd_spec);
  printf("  erase_mem_state  = %u\r\n", (unsigned)s->erase_mem_state);
  printf("  bus_width bitmap = 0x%X\r\n", (unsigned)s->bus_width);
  printf("  rsvd_mnf         = 0x%08" PRIx32 "\r\n", (uint32_t)s->rsvd_mnf);
}

int sd_info(int, char **)
{
  if (sd_ensure_ready() != ESP_OK) return 1;

  sdmmc_card_print_info(stdout, sd_card_ptr);
  sd_log_cid(&sd_card_ptr->cid);
  sd_log_ocr(sd_card_ptr);
  sd_log_scr(sd_card_ptr);

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

  if (sd_fs_mounted)
    sd_deinit();

  esp_err_t err = sd_ensure_ready();
  if (err != ESP_OK) return 1;

  err = sd_read_sectors((uint32_t)sec, (uint32_t)num);
  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD read failed: %s, reinit\r\n", esp_err_to_name(err));
    if (sd_reinit() == ESP_OK)
      err = sd_read_sectors((uint32_t)sec, (uint32_t)num);
  }

  return (err == ESP_OK) ? 0 : 1;
}

int sd_erase(int, char **)
{
  if (sd_fs_mounted)
    sd_deinit();

  esp_err_t err = sd_ensure_ready();
  if (err != ESP_OK) return 1;

  err = sd_card_erase();
  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD erase failed: %s, reinit\r\n", esp_err_to_name(err));
    if (sd_reinit() == ESP_OK)
      err = sd_card_erase();
  }

  if (err == ESP_OK)
    sd_deinit();

  return (err == ESP_OK) ? 0 : 1;
}

int sd_ls(int argc, char **argv)
{
  const char *base = "/sd";
  const char *path = "/";

  if (argc >= 3)
    path = argv[2];

  esp_err_t err = sd_fs_mount(base, NULL);
  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD mount failed: %s, retry\r\n", esp_err_to_name(err));
    sd_deinit();
    err = sd_fs_mount(base, NULL);
  }
  if (err != ESP_OK) return 1;

  err = sd_fs_list_dir(base, path);
  if (err != ESP_OK && sd_error_needs_reinit(err))
  {
    printf("W: SD list failed: %s, remount\r\n", esp_err_to_name(err));
    sd_deinit();
    err = sd_fs_mount(base, NULL);
    if (err == ESP_OK)
      err = sd_fs_list_dir(base, path);
  }

  return (err == ESP_OK) ? 0 : 1;
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

  if (!strcmp(op, "ls"))
    return sd_ls(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void sdmmc_console_register_system_commands()
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
