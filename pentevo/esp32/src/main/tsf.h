#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "tsspiffs.h"

#define TSF_PART_LABEL          "tsf"
#define TSF_BASE_PATH           "/tsf"
#define TSF_PART_SUBTYPE_VALUE  0x84
#define TSF_PART_SUBTYPE_NAME   "tsf"
#define TSF_BLOCK_SIZE          4096
#define TSF_HAL_BUF_SIZE        256
#define TSF_VFS_MAX_FILES       4

extern bool tsf_storage_initialized;
extern bool tsf_storage_mounted;
extern char tsf_storage_base_path[32];
extern char tsf_storage_part_label[16];

esp_err_t tsf_storage_mount();
esp_err_t tsf_storage_mount_quiet();
esp_err_t tsf_storage_format();
esp_err_t tsf_storage_delete();
esp_err_t tsf_storage_init();
esp_err_t tsf_storage_ensure_ready();
esp_err_t tsf_storage_ensure_ready_quiet();
void tsf_storage_deinit();

TSF_VOLUME *tsf_storage_volume();
esp_err_t tsf_storage_space_info(uint64_t *total_bytes, uint64_t *free_bytes);
bool tsf_storage_path_has_base(const char *path);
esp_err_t tsf_storage_name_from_path(const char *path, char *out, size_t out_size);
bool tsf_storage_path_is_root(const char *path);
esp_err_t tsf_storage_open_read(const char *path, TSF_FILE *file, size_t *out_size);
esp_err_t tsf_storage_read_file(const char *path, void *dst, size_t size, size_t *out_size);
esp_err_t tsf_storage_list_dir(const char *path);
esp_err_t tsf_storage_result_to_err(TSF_RESULT res);
const char *tsf_storage_result_name(TSF_RESULT res);
const char *tsf_storage_subtype_name();
esp_err_t tsf_storage_register_vfs();

int tsf_storage_info_cmd(int argc, char **argv);
int tsf_storage_format_cmd(int argc, char **argv);
int tsf_storage_delete_cmd(int argc, char **argv);
int tsf_storage_ls_cmd(int argc, char **argv);
int tsf_storage_cmd(int argc, char **argv);

void tsf_console_register_system_commands();
