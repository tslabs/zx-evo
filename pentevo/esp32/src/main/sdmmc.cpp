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
#include <fcntl.h>
#include <unistd.h>
#include "esp_vfs_fat.h"
#include "diskio_impl.h"
#include "diskio_sdmmc.h"
#include "ff.h"
#include "esp_heap_caps.h"
#include "esp_console.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ft8xx.h"

sdmmc_host_t sd_host;
sdmmc_slot_config_t sd_slot;
sdmmc_card_t sd_card;
sdmmc_card_t *sd_card_ptr = NULL;
bool sd_initialized = false;
bool sd_fs_mounted = false;
char sd_fs_base_path[32] = { 0 };
sdmmc_card_t *sd_fs_card = NULL;
FATFS *sd_fs_fatfs = NULL;
BYTE sd_fs_pdrv = FF_DRV_NOT_USED;
bool sd_fs_vfs_registered = false;
bool sd_fs_diskio_registered = false;
bool sd_fs_fat_mounted = false;

#define SD_SAVE_CHUNK_SIZE (16U * 1024U)
#define SD_SAVE_PROGRESS_BAR_WIDTH 32U
#define SD_SAVE_FT_ADDR_LIMIT (4U * 1024U * 1024U)

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

esp_log_level_t sd_diskio_old_level = ESP_LOG_NONE;
esp_log_level_t sd_vfs_fat_old_level = ESP_LOG_NONE;
esp_log_level_t sd_vfs_fat_sdmmc_old_level = ESP_LOG_NONE;
esp_log_level_t sd_sdmmc_cmd_old_level = ESP_LOG_NONE;
int sd_host_log_suppress_depth = 0;

esp_log_level_t sd_host_log_suppress_begin()
{
  esp_log_level_t old_sd_host_level = esp_log_level_get("SD_HOST");

  if (sd_host_log_suppress_depth == 0)
  {
    sd_diskio_old_level = esp_log_level_get("diskio_sdmmc");
    sd_vfs_fat_old_level = esp_log_level_get("vfs_fat");
    sd_vfs_fat_sdmmc_old_level = esp_log_level_get("vfs_fat_sdmmc");
    sd_sdmmc_cmd_old_level = esp_log_level_get("sdmmc_cmd");

    esp_log_level_set("SD_HOST", ESP_LOG_NONE);
    esp_log_level_set("diskio_sdmmc", ESP_LOG_NONE);
    esp_log_level_set("vfs_fat", ESP_LOG_NONE);
    esp_log_level_set("vfs_fat_sdmmc", ESP_LOG_NONE);
    esp_log_level_set("sdmmc_cmd", ESP_LOG_NONE);
  }

  sd_host_log_suppress_depth++;
  return old_sd_host_level;
}

void sd_host_log_suppress_end(esp_log_level_t old_sd_host_level)
{
  if (sd_host_log_suppress_depth <= 0) return;

  sd_host_log_suppress_depth--;
  if (sd_host_log_suppress_depth > 0) return;

  esp_log_level_set("SD_HOST", old_sd_host_level);
  esp_log_level_set("diskio_sdmmc", sd_diskio_old_level);
  esp_log_level_set("vfs_fat", sd_vfs_fat_old_level);
  esp_log_level_set("vfs_fat_sdmmc", sd_vfs_fat_sdmmc_old_level);
  esp_log_level_set("sdmmc_cmd", sd_sdmmc_cmd_old_level);
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

void sd_fs_clear_state()
{
  memset(&sd_card, 0, sizeof(sd_card));
  sd_card_ptr = NULL;
  sd_initialized = false;
  sd_fs_mounted = false;
  sd_fs_card = NULL;
  sd_fs_fatfs = NULL;
  sd_fs_pdrv = FF_DRV_NOT_USED;
  sd_fs_vfs_registered = false;
  sd_fs_diskio_registered = false;
  sd_fs_fat_mounted = false;
  sd_fs_base_path[0] = 0;
}

void sd_fs_unmount_force()
{
  char drv[3];

  if (sd_fs_pdrv != FF_DRV_NOT_USED)
  {
    drv[0] = (char)('0' + sd_fs_pdrv);
    drv[1] = ':';
    drv[2] = 0;

    if (sd_fs_fat_mounted)
      f_mount(NULL, drv, 0);
  }

  if (sd_fs_diskio_registered && sd_fs_pdrv != FF_DRV_NOT_USED)
    ff_diskio_unregister(sd_fs_pdrv);

  if (sd_fs_vfs_registered && sd_fs_base_path[0])
    esp_vfs_fat_unregister_path(sd_fs_base_path);

  if (sd_initialized)
    sdmmc_host_deinit();

  sd_fs_clear_state();
}

void sd_deinit()
{
  sd_fs_unmount_force();
}

esp_err_t sd_init_impl(bool quiet)
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
    if (!quiet)
      printf("E: sdmmc_host_init failed: %s\r\n", esp_err_to_name(err));
    sd_fs_clear_state();
    return err;
  }

  err = sdmmc_host_init_slot(SD_SLOT, &sd_slot);
  if (err != ESP_OK)
  {
    sd_host_log_suppress_end(old_sd_host_level);
    if (!quiet)
      printf("E: sdmmc_host_init_slot failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    sd_fs_clear_state();
    return err;
  }

  err = sdmmc_card_init(&sd_host, &sd_card);
  sd_host_log_suppress_end(old_sd_host_level);

  if (err != ESP_OK)
  {
    if (!quiet)
      printf("E: sdmmc_card_init failed: %s\r\n", esp_err_to_name(err));
    sdmmc_host_deinit();
    sd_fs_clear_state();
    return err;
  }

  sd_card_ptr = &sd_card;
  sd_initialized = true;

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  printf("SD voltage = %dmv\r\n", ((sd_pwr_ctrl_ldo_ctx_t *)sd_pwr->ctx)->voltage_mv);
#endif

  return ESP_OK;
}

esp_err_t sd_init()
{
  return sd_init_impl(false);
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

esp_err_t sd_fs_register_fat(const char *base_path, const esp_vfs_fat_mount_config_t *mount_cfg)
{
  esp_err_t err;
  FRESULT res;
  char drv[3];

  err = ff_diskio_get_drive(&sd_fs_pdrv);
  if (err != ESP_OK || sd_fs_pdrv == FF_DRV_NOT_USED)
    return ESP_ERR_NO_MEM;

  ff_diskio_register_sdmmc(sd_fs_pdrv, sd_card_ptr);
  ff_sdmmc_set_disk_status_check(sd_fs_pdrv, mount_cfg->disk_status_check_enable);
  sd_fs_diskio_registered = true;

  drv[0] = (char)('0' + sd_fs_pdrv);
  drv[1] = ':';
  drv[2] = 0;

  err = esp_vfs_fat_register(base_path, drv, (size_t)mount_cfg->max_files, &sd_fs_fatfs);
  if (err != ESP_OK)
    return err;
  sd_fs_vfs_registered = true;

  res = f_mount(sd_fs_fatfs, drv, 1);
  if (res != FR_OK)
    return ESP_FAIL;

  sd_fs_fat_mounted = true;
  return ESP_OK;
}

esp_err_t sd_fs_mount_new(const char *base_path, sdmmc_card_t **out_card, bool quiet)
{
  esp_err_t err;

  if (!base_path || !base_path[0]) return ESP_ERR_INVALID_ARG;

  err = sd_init_impl(quiet);
  if (err != ESP_OK) return err;

  err = sd_probe_card();
  if (err != ESP_OK)
  {
    sd_deinit();
    err = sd_init_impl(quiet);
    if (err != ESP_OK) return err;

    err = sd_probe_card();
    if (err != ESP_OK)
    {
      sd_deinit();
      return err;
    }
  }

  esp_vfs_fat_mount_config_t mount_cfg =
  {
    .format_if_mount_failed = false,
    .max_files = 4,
    .allocation_unit_size = 16 * 1024,
    .disk_status_check_enable = true,
  };

  err = sd_fs_register_fat(base_path, &mount_cfg);
  if (err != ESP_OK)
  {
    if (!quiet)
      printf("E: SD FAT mount failed: %s\r\n", esp_err_to_name(err));
    sd_fs_unmount_force();
    return err;
  }

  sd_fs_mounted = true;
  sd_fs_card = sd_card_ptr;
  snprintf(sd_fs_base_path, sizeof(sd_fs_base_path), "%s", base_path);

  if (out_card)
    *out_card = sd_fs_card;

  return ESP_OK;
}

esp_err_t sd_fs_sense_impl(const char *base_path, sdmmc_card_t **out_card, bool quiet)
{
  esp_err_t err;

  if (out_card)
    *out_card = NULL;

  if (!base_path || !base_path[0]) return ESP_ERR_INVALID_ARG;

  if (!sd_fs_mounted)
    return sd_fs_mount_new(base_path, out_card, quiet);

  if (strcmp(sd_fs_base_path, base_path) != 0)
    return ESP_ERR_INVALID_ARG;

  if (!sd_fs_card)
  {
    sd_fs_unmount_force();
    return sd_fs_mount_new(base_path, out_card, quiet);
  }

  if (quiet)
  {
    esp_log_level_t old_sd_host_level = sd_host_log_suppress_begin();
    err = sdmmc_get_status(sd_fs_card);
    sd_host_log_suppress_end(old_sd_host_level);
  }
  else
  {
    err = sdmmc_get_status(sd_fs_card);
  }

  if (err == ESP_OK)
  {
    if (out_card)
      *out_card = sd_fs_card;
    return ESP_OK;
  }

  if (!quiet)
    printf("W: SD card sense failed: %s, remounting %s\r\n", esp_err_to_name(err), sd_fs_base_path);

  sd_fs_unmount_force();

  err = sd_fs_mount_new(base_path, out_card, quiet);
  if (err != ESP_OK && !quiet)
    printf("E: SD card is not mounted: %s\r\n", esp_err_to_name(err));

  return err;
}

esp_err_t sd_fs_sense(const char *base_path)
{
  return sd_fs_sense_impl(base_path, NULL, false);
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

bool sd_parse_u32_arg(const char *s, const char *name, uint32_t *out)
{
  char *endp = NULL;
  uint64_t value;

  if (!s || !s[0] || !out)
  {
    printf("Missing <%s>\r\n", name ? name : "arg");
    return false;
  }

  errno = 0;
  value = strtoull(s, &endp, 0);
  if (errno || !endp || *endp || value > UINT32_MAX)
  {
    printf("Bad <%s>: %s\r\n", name ? name : "arg", s);
    return false;
  }

  *out = (uint32_t)value;
  return true;
}

void sd_save_print_progress(uint32_t done, uint32_t total, int64_t start_us)
{
  uint64_t percent = total ? ((uint64_t)done * 100ULL) / (uint64_t)total : 100ULL;
  uint32_t filled = total ? (uint32_t)(((uint64_t)done * SD_SAVE_PROGRESS_BAR_WIDTH) / (uint64_t)total) : SD_SAVE_PROGRESS_BAR_WIDTH;
  int64_t elapsed_us = esp_timer_get_time() - start_us;
  uint32_t kib_s = 0;

  if (percent > 100ULL) percent = 100ULL;
  if (filled > SD_SAVE_PROGRESS_BAR_WIDTH) filled = SD_SAVE_PROGRESS_BAR_WIDTH;

  if (elapsed_us > 0)
    kib_s = (uint32_t)(((uint64_t)done * 1000000ULL) / (uint64_t)elapsed_us / 1024ULL);

  printf("\rSD save ft: [");
  for (uint32_t i = 0; i < SD_SAVE_PROGRESS_BAR_WIDTH; i++)
    putchar((i < filled) ? '#' : '.');
  printf("] %3" PRIu64 "%% %" PRIu32 "/%" PRIu32 " KiB %" PRIu32 " KiB/s",
    percent,
    done / 1024U,
    total / 1024U,
    kib_s);
  fflush(stdout);
}

esp_err_t sd_fs_save_ft_dump_once(const char *base_path, uint32_t addr, uint32_t size, const char *path)
{
  char full[256];
  uint8_t *buf = NULL;
  int fd = -1;
  bool ft_open = false;
  bool progress_open = false;
  uint32_t done = 0;
  uint64_t last_percent = UINT64_MAX;
  int64_t start_us = 0;
  esp_err_t err;
  esp_err_t err2;

  if (!base_path || !base_path[0]) return ESP_ERR_INVALID_ARG;
  if (!path || !path[0]) return ESP_ERR_INVALID_ARG;
  if (size == 0) return ESP_ERR_INVALID_SIZE;
  if ((uint64_t)addr + (uint64_t)size > SD_SAVE_FT_ADDR_LIMIT) return ESP_ERR_INVALID_SIZE;

  size_t base_len = strlen(base_path);
  if (!strncmp(path, base_path, base_len) && (path[base_len] == 0 || path[base_len] == '/'))
  {
    int n = snprintf(full, sizeof(full), "%s", path);
    if (n < 0 || (size_t)n >= sizeof(full)) return ESP_ERR_INVALID_ARG;
  }
  else if (!sd_fs_build_full_path(base_path, path, full, sizeof(full)))
    return ESP_ERR_INVALID_ARG;

  err = sd_fs_mount(base_path, NULL);
  if (err != ESP_OK) return err;

  err = sd_fs_sense(base_path);
  if (err != ESP_OK) return err;

  buf = (uint8_t*)heap_caps_malloc(SD_SAVE_CHUNK_SIZE, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
  if (!buf)
    buf = (uint8_t*)heap_caps_malloc(SD_SAVE_CHUNK_SIZE, MALLOC_CAP_8BIT);
  if (!buf) return ESP_ERR_NO_MEM;

  fd = open(full, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
  {
    int saved_errno = errno;
    printf("E: open('%s') failed, errno=%d\r\n", full, saved_errno);
    free(buf);
    if (sd_has_retryable_errno(saved_errno)) return ESP_ERR_INVALID_STATE;
    return ESP_FAIL;
  }

  err = ft_open_session();
  if (err != ESP_OK)
  {
    printf("E: FT open failed: %s\r\n", esp_err_to_name(err));
    goto done;
  }
  ft_open = true;

  printf("Saving FT dump: addr=0x%08" PRIX32 " size=%" PRIu32 " -> %s\r\n", addr, size, full);
  start_us = esp_timer_get_time();
  sd_save_print_progress(0, size, start_us);
  progress_open = true;

  while (done < size)
  {
    uint32_t n = size - done;
    uint64_t percent;

    if (n > SD_SAVE_CHUNK_SIZE)
      n = SD_SAVE_CHUNK_SIZE;

    err = ft_read(buf, addr + done, n);
    if (err != ESP_OK)
    {
      printf("\r\nE: ft_read failed at 0x%08" PRIX32 ", size=%" PRIu32 ": %s\r\n",
        addr + done, n, esp_err_to_name(err));
      goto done;
    }

    size_t wr_done = 0;
    while (wr_done < n)
    {
      ssize_t wr = write(fd, buf + wr_done, n - wr_done);
      if (wr < 0)
      {
        int saved_errno = errno;
        printf("\r\nE: write('%s') failed, errno=%d\r\n", full, saved_errno);
        err = sd_has_retryable_errno(saved_errno) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
        goto done;
      }
      if (wr == 0)
      {
        printf("\r\nE: write('%s') returned 0\r\n", full);
        err = ESP_FAIL;
        goto done;
      }
      wr_done += (size_t)wr;
    }

    done += n;
    percent = ((uint64_t)done * 100ULL) / (uint64_t)size;
    if (percent != last_percent || done == size)
    {
      sd_save_print_progress(done, size, start_us);
      last_percent = percent;
    }
  }

  if (close(fd) != 0)
  {
    int saved_errno = errno;
    fd = -1;
    printf("\r\nE: close('%s') failed, errno=%d\r\n", full, saved_errno);
    err = sd_has_retryable_errno(saved_errno) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
    goto done;
  }
  fd = -1;

  printf("\r\nSave done: %s, %" PRIu32 " bytes\r\n", full, size);
  err = ESP_OK;

done:
  if (fd >= 0)
  {
    if (close(fd) != 0 && err == ESP_OK)
    {
      int saved_errno = errno;
      if (progress_open) printf("\r\n");
      printf("E: close('%s') failed, errno=%d\r\n", full, saved_errno);
      err = sd_has_retryable_errno(saved_errno) ? ESP_ERR_INVALID_STATE : ESP_FAIL;
    }
  }

  if (ft_open)
  {
    err2 = ft_close_session();
    if (err == ESP_OK && err2 != ESP_OK)
      err = err2;
  }

  free(buf);
  return err;
}

esp_err_t sd_fs_save_ft_dump(const char *base_path, uint32_t addr, uint32_t size, const char *path)
{
  return sd_fs_save_ft_dump_once(base_path, addr, size, path);
}

esp_err_t sd_fs_mount(const char *base_path, sdmmc_card_t **out_card)
{
  return sd_fs_sense_impl(base_path, out_card, false);
}

esp_err_t sd_fs_mount_quiet(const char *base_path, sdmmc_card_t **out_card)
{
  return sd_fs_sense_impl(base_path, out_card, true);
}

void sd_fs_unmount(const char *base_path, sdmmc_card_t *card)
{
  if (base_path && base_path[0] && sd_fs_mounted && strcmp(sd_fs_base_path, base_path) != 0) return;
  if (card && sd_fs_card && card != sd_fs_card) return;
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

  err = sd_fs_sense(base_path);
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
  return sd_fs_read_file_once(base_path, path, dst, dst_size, out_size);
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

  esp_err_t err = sd_fs_sense(base_path);
  if (err != ESP_OK) return err;

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
  if (err != ESP_OK) return 1;

  err = sd_fs_list_dir(base, path);
  return (err == ESP_OK) ? 0 : 1;
}

int sd_save(int argc, char **argv)
{
  uint32_t addr;
  uint32_t size;
  esp_err_t err;

  if (argc != 6)
  {
    printf("Usage: sd save ft <addr> <size> \"path/name\"\r\n");
    return 1;
  }

  if (strcmp(argv[2], "ft") != 0)
  {
    printf("Bad <src>: %s, only 'ft' is supported\r\n", argv[2]);
    return 1;
  }

  if (!sd_parse_u32_arg(argv[3], "addr", &addr)) return 1;
  if (!sd_parse_u32_arg(argv[4], "size", &size)) return 1;
  if (size == 0)
  {
    printf("Bad <size>: must be > 0\r\n");
    return 1;
  }
  if ((uint64_t)addr + (uint64_t)size > SD_SAVE_FT_ADDR_LIMIT)
  {
    printf("Bad range: addr=0x%08" PRIX32 " size=%" PRIu32 ", FT address limit is 0x%06X\r\n",
      addr,
      size,
      SD_SAVE_FT_ADDR_LIMIT);
    return 1;
  }

  err = sd_fs_save_ft_dump("/sd", addr, size, argv[5]);
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
    printf("  sd save ft <addr> <size> \"path/name\"\r\n");
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

  if (!strcmp(op, "save"))
    return sd_save(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void sdmmc_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "sd",
    .help = "SD card commands: info/erase/read/ls/save",
    .hint     = NULL,
    .func     = &sd_cmd,
    .argtable = NULL
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
