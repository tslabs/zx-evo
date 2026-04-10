#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include "esp_console.h"
#include "esp_partition.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "spiffs.h"

bool sf_initialized = false;
bool sf_fs_mounted = false;
char sf_fs_base_path[32] = { 0 };
char sf_part_label[16] = { 0 };

int sf_build_full_path(const char *base_path, const char *path, char *full, size_t full_size)
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

esp_log_level_t sf_log_suppress_begin()
{
  esp_log_level_t old_level = esp_log_level_get("SPIFFS");
  esp_log_level_set("SPIFFS", ESP_LOG_NONE);
  return old_level;
}

void sf_log_suppress_end(esp_log_level_t old_level)
{
  esp_log_level_set("SPIFFS", old_level);
}

esp_err_t sf_register(bool format_if_mount_failed, bool quiet)
{
  esp_err_t err;
  esp_log_level_t old_level = ESP_LOG_INFO;

  if (sf_fs_mounted)
  {
    sf_initialized = true;
    return ESP_OK;
  }

  esp_vfs_spiffs_conf_t conf =
  {
    .base_path = SF_BASE_PATH,
    .partition_label = SF_PART_LABEL,
    .max_files = 8,
    .format_if_mount_failed = format_if_mount_failed,
  };

  if (quiet)
    old_level = sf_log_suppress_begin();

  err = esp_vfs_spiffs_register(&conf);

  if (quiet)
    sf_log_suppress_end(old_level);

  if (err != ESP_OK)
  {
    if (!quiet)
      printf("E: esp_vfs_spiffs_register failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  sf_initialized = true;
  sf_fs_mounted = true;
  snprintf(sf_fs_base_path, sizeof(sf_fs_base_path), "%s", SF_BASE_PATH);
  snprintf(sf_part_label, sizeof(sf_part_label), "%s", SF_PART_LABEL);

  return ESP_OK;
}

esp_err_t sf_mount()
{
  return sf_register(false, false);
}

esp_err_t sf_init()
{
  esp_err_t err;

  if (sf_fs_mounted)
    sf_deinit();

  err = sf_register(true, true);
  if (err != ESP_OK)
  {
    printf("E: sf init failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  return ESP_OK;
}

void sf_deinit()
{
  if (sf_fs_mounted)
    esp_vfs_spiffs_unregister(sf_part_label[0] ? sf_part_label : SF_PART_LABEL);

  sf_initialized = false;
  sf_fs_mounted = false;
  sf_fs_base_path[0] = 0;
  sf_part_label[0] = 0;
}

esp_err_t sf_ensure_ready()
{
  return sf_mount();
}

esp_err_t sf_list_dir(const char *base_path, const char *path)
{
  char full[256];

  if (!sf_build_full_path(base_path, path, full, sizeof(full)))
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

int sf_info(int argc, char **argv)
{
  if (argc < 0 || !argv) return 1;

  esp_err_t err = sf_ensure_ready();
  if (err != ESP_OK) return 1;

  size_t total = 0;
  size_t used = 0;
  err = esp_spiffs_info(SF_PART_LABEL, &total, &used);
  if (err != ESP_OK)
  {
    printf("E: esp_spiffs_info failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  const esp_partition_t *part = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
    SF_PART_LABEL);

  printf("SPIFFS info\r\n");
  printf("  mounted    : %s\r\n", sf_fs_mounted ? "yes" : "no");
  printf("  label      : %s\r\n", SF_PART_LABEL);
  printf("  base path  : %s\r\n", SF_BASE_PATH);
  printf("  total      : %u bytes\r\n", (unsigned)total);
  printf("  used       : %u bytes\r\n", (unsigned)used);
  printf("  free       : %u bytes\r\n", (unsigned)(total - used));

  if (part)
  {
    printf("  address    : 0x%08" PRIX32 "\r\n", part->address);
    printf("  size       : 0x%08lx (%lu bytes)\r\n", (unsigned long)part->size, (unsigned long)part->size);
  }
  else
  {
    printf("  partition  : not found\r\n");
  }

  return 0;
}

int sf_init_cmd(int argc, char **argv)
{
  if (argc < 0 || !argv) return 1;

  esp_err_t err = sf_init();
  if (err != ESP_OK) return 1;

  printf("SPIFFS formatted and mounted: %s -> %s\r\n", SF_PART_LABEL, SF_BASE_PATH);
  return 0;
}

int sf_ls(int argc, char **argv)
{
  const char *path = "/";

  if (argc >= 3)
    path = argv[2];

  esp_err_t err = sf_ensure_ready();
  if (err != ESP_OK) return 1;

  err = sf_list_dir(SF_BASE_PATH, path);
  return (err == ESP_OK) ? 0 : 1;
}

int sf_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  sf init\r\n");
    printf("  sf info\r\n");
    printf("  sf ls [path]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "init"))
    return sf_init_cmd(argc, argv);

  if (!strcmp(op, "info"))
    return sf_info(argc, argv);

  if (!strcmp(op, "ls"))
    return sf_ls(argc, argv);

  printf("Unknown subcommand: %s\r\n", op);
  return 1;
}

void spiffs_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "sf",
    .help = "SPIFFS commands: init/info/ls",
    .hint     = NULL,
    .func     = &sf_cmd,
    .argtable = NULL
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
