#pragma once

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#define SF_PART_LABEL  "spiffs"
#define SF_BASE_PATH   "/sf"

extern bool sf_initialized;
extern bool sf_fs_mounted;
extern char sf_fs_base_path[32];
extern char sf_part_label[16];

esp_err_t sf_mount();
esp_err_t sf_init();
esp_err_t sf_ensure_ready();
void sf_deinit();
esp_err_t sf_list_dir(const char *base_path, const char *path);
int sf_build_full_path(const char *base_path, const char *path, char *full, size_t full_size);

int sf_info(int argc, char **argv);
int sf_init_cmd(int argc, char **argv);
int sf_ls(int argc, char **argv);
int sf_cmd(int argc, char **argv);

void spiffs_console_register_system_commands();
