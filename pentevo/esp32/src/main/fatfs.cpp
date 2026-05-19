#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "esp_console.h"
#include "esp_partition.h"
#include "esp_vfs_fat.h"
#include "ff.h"
#include "wear_levelling.h"
#include "fatfs.h"

bool fs_initialized = false;
bool fs_fs_mounted = false;
char fs_fs_base_path[32] = { 0 };
char fs_part_label[16] = { 0 };
wl_handle_t fs_wl_handle = WL_INVALID_HANDLE;

#define FS_VFS_MAX_FILES 4

int fs_build_full_path(const char *base_path, const char *path, char *full, size_t full_size)
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

const esp_partition_t *fs_find_partition()
{
  return esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    ESP_PARTITION_SUBTYPE_DATA_FAT,
    FS_PART_LABEL);
}

size_t fs_get_allocation_unit_size(size_t part_size)
{
  if (part_size >= (16u * 1024u * 1024u)) return 16u * 1024u;
  if (part_size >= (4u * 1024u * 1024u)) return 4u * 1024u;
  return 4096;
}

void fs_fill_mount_cfg(esp_vfs_fat_mount_config_t *cfg, bool format_if_mount_failed)
{
  const esp_partition_t *part = fs_find_partition();
  size_t part_size = part ? part->size : 0;

  memset(cfg, 0, sizeof(*cfg));
  cfg->format_if_mount_failed = format_if_mount_failed;
  cfg->max_files = FS_VFS_MAX_FILES;
  cfg->allocation_unit_size = fs_get_allocation_unit_size(part_size);
  cfg->disk_status_check_enable = false;
  cfg->use_one_fat = true;
}


const char *fs_fat_type_name(BYTE fs_type)
{
  switch (fs_type)
  {
    case FS_FAT12: return "FAT12";
    case FS_FAT16: return "FAT16";
    case FS_FAT32: return "FAT32";
#if defined(FF_FS_EXFAT) && FF_FS_EXFAT
    case FS_EXFAT: return "exFAT";
#endif
    default: return "unknown";
  }
}

uint32_t fs_fat_sector_size(FATFS *fat)
{
  if (!fat) return 0;

#if FF_MAX_SS != FF_MIN_SS
  return fat->ssize;
#else
  return FF_MIN_SS;
#endif
}

FRESULT fs_get_mounted_fatfs(FATFS **out_fat, DWORD *out_free_clusters)
{
  if (!out_fat || !out_free_clusters) return FR_INVALID_PARAMETER;

  *out_fat = NULL;
  *out_free_clusters = 0;

  FRESULT res = f_getfree("", out_free_clusters, out_fat);
  if (res == FR_OK) return res;

  return f_getfree("0:", out_free_clusters, out_fat);
}

void fs_print_fatfs_details(uint64_t total_bytes, uint64_t free_bytes, esp_err_t usage_err)
{
  FATFS *fat = NULL;
  DWORD free_clusters_fatfs = 0;
  FRESULT res = fs_get_mounted_fatfs(&fat, &free_clusters_fatfs);

  if (res != FR_OK || !fat)
  {
    printf("  fatfs      : unavailable (FRESULT=%u)\r\n", (unsigned)res);
    return;
  }

  uint32_t sector_size = fs_fat_sector_size(fat);
  uint32_t cluster_size = (uint32_t)fat->csize * sector_size;
  uint32_t total_clusters = fat->n_fatent >= 2 ? fat->n_fatent - 2 : 0;
  uint32_t free_clusters = (uint32_t)free_clusters_fatfs;

  if (free_clusters > total_clusters)
    free_clusters = total_clusters;

  uint32_t used_clusters = total_clusters >= free_clusters ? total_clusters - free_clusters : 0;
  uint64_t data_bytes = (uint64_t)total_clusters * cluster_size;
  uint64_t fat_bytes = (uint64_t)fat->fsize * sector_size;

  printf("  fat type   : %s (%u)\r\n", fs_fat_type_name(fat->fs_type), (unsigned)fat->fs_type);
  printf("  fat copies : %u\r\n", (unsigned)fat->n_fats);
  printf("  sector     : %u bytes\r\n", (unsigned)sector_size);
  printf("  cluster    : %u sectors, %u bytes\r\n", (unsigned)fat->csize, (unsigned)cluster_size);
  printf("  clusters   : total=%u used=%u free=%u\r\n", (unsigned)total_clusters, (unsigned)used_clusters, (unsigned)free_clusters);
  printf("  data area  : %llu bytes\r\n", (unsigned long long)data_bytes);
  printf("  fat size   : %u sectors, %llu bytes each\r\n", (unsigned)fat->fsize, (unsigned long long)fat_bytes);
  printf("  root ents  : %u\r\n", (unsigned)fat->n_rootdir);
  printf("  vol base   : %llu\r\n", (unsigned long long)fat->volbase);
  printf("  fat base   : %llu\r\n", (unsigned long long)fat->fatbase);
  printf("  dir base   : %llu\r\n", (unsigned long long)fat->dirbase);
  printf("  data base  : %llu\r\n", (unsigned long long)fat->database);

  if (usage_err == ESP_OK && total_bytes != data_bytes)
    printf("  vfs total  : %llu bytes\r\n", (unsigned long long)total_bytes);

  if (usage_err == ESP_OK && cluster_size > 0 && free_bytes != (uint64_t)free_clusters * cluster_size)
    printf("  vfs free   : %llu bytes\r\n", (unsigned long long)free_bytes);
}

esp_err_t fs_mount_impl(bool quiet)
{
  esp_err_t err;

  if (fs_fs_mounted)
  {
    fs_initialized = true;
    return ESP_OK;
  }

  esp_vfs_fat_mount_config_t mount_cfg;
  fs_fill_mount_cfg(&mount_cfg, false);

  err = esp_vfs_fat_spiflash_mount_rw_wl(
    FS_BASE_PATH,
    FS_PART_LABEL,
    &mount_cfg,
    &fs_wl_handle);

  if (err != ESP_OK)
  {
    if (!quiet)
      printf("E: esp_vfs_fat_spiflash_mount_rw_wl failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  fs_initialized = true;
  fs_fs_mounted = true;
  snprintf(fs_fs_base_path, sizeof(fs_fs_base_path), "%s", FS_BASE_PATH);
  snprintf(fs_part_label, sizeof(fs_part_label), "%s", FS_PART_LABEL);

  return ESP_OK;
}

esp_err_t fs_mount()
{
  return fs_mount_impl(false);
}

esp_err_t fs_mount_quiet()
{
  return fs_mount_impl(true);
}

esp_err_t fs_format()
{
  const esp_partition_t *part = fs_find_partition();
  if (!part) return ESP_ERR_NOT_FOUND;

  if (fs_fs_mounted)
    fs_deinit();

  esp_vfs_fat_mount_config_t format_cfg;
  fs_fill_mount_cfg(&format_cfg, true);

  esp_err_t err = esp_vfs_fat_spiflash_format_cfg_rw_wl(
    FS_BASE_PATH,
    FS_PART_LABEL,
    &format_cfg);
  if (err != ESP_OK)
  {
    printf("E: esp_vfs_fat_spiflash_format_cfg_rw_wl failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  return fs_mount();
}

void fs_deinit()
{
  if (fs_fs_mounted && fs_wl_handle != WL_INVALID_HANDLE)
    esp_vfs_fat_spiflash_unmount_rw_wl(fs_fs_base_path[0] ? fs_fs_base_path : FS_BASE_PATH, fs_wl_handle);

  fs_initialized = false;
  fs_fs_mounted = false;
  fs_fs_base_path[0] = 0;
  fs_part_label[0] = 0;
  fs_wl_handle = WL_INVALID_HANDLE;
}

esp_err_t fs_ensure_ready()
{
  return fs_mount();
}

esp_err_t fs_ensure_ready_quiet()
{
  return fs_mount_quiet();
}

esp_err_t fs_list_dir(const char *base_path, const char *path)
{
  char full[256];

  if (!fs_build_full_path(base_path, path, full, sizeof(full)))
    return ESP_ERR_INVALID_ARG;

  errno = 0;
  DIR *d = opendir(full);
  if (!d)
  {
    int saved_errno = errno;
    printf("E: opendir('%s') failed, errno=%d\r\n", full, saved_errno);
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

    struct stat st = {};
    if (stat(p, &st) == 0)
    {
      const char *t = S_ISDIR(st.st_mode) ? "DIR " : "FILE";
      printf("  %s  %10" PRIu32 "  %s\r\n", t, (uint32_t)st.st_size, e->d_name);
    }
    else
    {
      printf("  ?    ?          %s\r\n", e->d_name);
    }
  }

  closedir(d);
  return ESP_OK;
}

int fs_info(int argc, char **argv)
{
  if (argc < 0 || !argv) return 1;

  esp_err_t err = fs_ensure_ready();
  if (err != ESP_OK) return 1;

  const esp_partition_t *part = fs_find_partition();
  uint64_t total = 0;
  uint64_t free = 0;
  uint64_t used = 0;

  err = esp_vfs_fat_info(FS_BASE_PATH, &total, &free);
  if (err == ESP_OK)
    used = total - free;

  printf("FAT info\r\n");
  printf("  mounted    : %s\r\n", fs_fs_mounted ? "yes" : "no");
  printf("  label      : %s\r\n", FS_PART_LABEL);
  printf("  base path  : %s\r\n", FS_BASE_PATH);

  esp_vfs_fat_mount_config_t cfg;
  fs_fill_mount_cfg(&cfg, false);
  printf("  cfg alloc  : %u bytes\r\n", (unsigned)cfg.allocation_unit_size);
  printf("  cfg fats   : %u on next format\r\n", cfg.use_one_fat ? 1u : 2u);
  printf("  max files  : %d\r\n", cfg.max_files);

  if (err == ESP_OK)
  {
    printf("  total      : %llu bytes\r\n", (unsigned long long)total);
    printf("  used       : %llu bytes\r\n", (unsigned long long)used);
    printf("  free       : %llu bytes\r\n", (unsigned long long)free);
  }
  else
  {
    printf("  usage      : unavailable (%s)\r\n", esp_err_to_name(err));
  }

  fs_print_fatfs_details(total, free, err);

  if (part)
  {
    printf("  address    : 0x%08" PRIX32 "\r\n", part->address);
    printf("  size       : 0x%08lx (%lu bytes)\r\n", (unsigned long)part->size, (unsigned long)part->size);
  }
  else
  {
    printf("  partition  : not found\r\n");
  }

  if (fs_wl_handle != WL_INVALID_HANDLE)
  {
    size_t wl_bytes = wl_size(fs_wl_handle);
    size_t wl_sector = wl_sector_size(fs_wl_handle);
    printf("  wl size    : %u bytes\r\n", (unsigned)wl_bytes);
    printf("  wl sector  : %u bytes\r\n", (unsigned)wl_sector);
    printf("  wl handle  : %d\r\n", (int)fs_wl_handle);
  }

  return 0;
}

esp_err_t fs_init()
{
  return fs_format();
}

int fs_format_cmd(int argc, char **argv)
{
  if (argc < 0 || !argv) return 1;

  const esp_partition_t *part = fs_find_partition();
  if (!part)
  {
    printf("E: partition '%s' not found\r\n", FS_PART_LABEL);
    return 1;
  }

  esp_vfs_fat_mount_config_t format_cfg;
  fs_fill_mount_cfg(&format_cfg, true);

  esp_err_t err = fs_format();
  if (err != ESP_OK) return 1;

  printf("FAT formatted and mounted: %s -> %s\r\n", FS_PART_LABEL, FS_BASE_PATH);
  printf("  address    : 0x%08" PRIX32 "\r\n", part->address);
  printf("  size       : 0x%08lx (%lu bytes)\r\n", (unsigned long)part->size, (unsigned long)part->size);
  printf("  alloc unit : %u bytes\r\n", (unsigned)format_cfg.allocation_unit_size);
  printf("  fat copies : %u\r\n", format_cfg.use_one_fat ? 1u : 2u);
  return 0;
}

int fs_ls(int argc, char **argv)
{
  const char *path = "/";

  if (argc >= 3)
    path = argv[2];

  esp_err_t err = fs_ensure_ready();
  if (err != ESP_OK) return 1;

  err = fs_list_dir(FS_BASE_PATH, path);
  return (err == ESP_OK) ? 0 : 1;
}

int fs_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  fat format\r\n");
    printf("  fat info\r\n");
    printf("  fat ls [path]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "format"))
    return fs_format_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return fs_info(argc, argv);

  if (!strcmp(op, "ls"))
    return fs_ls(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void fat_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "fat",
    .help = "FATFS commands: format/info/ls",
    .hint     = NULL,
    .func     = &fs_cmd,
    .argtable = NULL
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
