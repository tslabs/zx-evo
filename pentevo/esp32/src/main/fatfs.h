#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "wear_levelling.h"

#define FS_PART_LABEL  "fat"
#define FS_BASE_PATH   "/fs"

extern bool fs_initialized;
extern bool fs_fs_mounted;
extern char fs_fs_base_path[32];
extern char fs_part_label[16];
extern wl_handle_t fs_wl_handle;

esp_err_t fs_mount();
esp_err_t fs_init();
esp_err_t fs_ensure_ready();
void fs_deinit();
esp_err_t fs_list_dir(const char *base_path, const char *path);
int fs_build_full_path(const char *base_path, const char *path, char *full, size_t full_size);

int fs_info(int argc, char **argv);
int fs_init_cmd(int argc, char **argv);
int fs_ls(int argc, char **argv);
int fs_cmd(int argc, char **argv);

void fat_console_register_system_commands();
