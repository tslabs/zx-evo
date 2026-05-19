#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "sdmmc.h"

#if defined(CONFIG_IDF_TARGET_ESP32P4)
  #define SD_SLOT      SDMMC_HOST_SLOT_1
  #define SD_CLK       GPIO_NUM_43
  #define SD_CMD       GPIO_NUM_44
  #define SD_D0        GPIO_NUM_39
  #define SD_D1        GPIO_NUM_40
  #define SD_D2        GPIO_NUM_41
  #define SD_D3        GPIO_NUM_42
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define SD_SLOT      SDMMC_HOST_SLOT_1
  #define SD_CLK       GPIO_NUM_40
  #define SD_CMD       GPIO_NUM_39
  #define SD_D0        GPIO_NUM_38
  #define SD_D1        GPIO_NUM_2
  #define SD_D2        GPIO_NUM_41
  #define SD_D3        GPIO_NUM_42
#endif

#ifndef SDMMC_CMD13_SEND_STATUS
#define SDMMC_CMD13_SEND_STATUS 13U
#endif

#ifndef SDMMC_ARG_RCA
#define SDMMC_ARG_RCA(rca_) ((uint32_t)(rca_) << 16)
#endif

#ifndef SDMMC_R1
#define SDMMC_R1(resp_) ((uint32_t)((resp_)[0]))
#endif

#ifndef SDMMC_CMD7_SELECT_CARD
#define SDMMC_CMD7_SELECT_CARD 7U
#endif

#define SD_ST_OUT_OF_RANGE        (1U << 31)
#define SD_ST_ADDRESS_ERROR       (1U << 30)
#define SD_ST_BLOCK_LEN_ERROR     (1U << 29)
#define SD_ST_ERASE_SEQ_ERROR     (1U << 28)
#define SD_ST_ERASE_PARAM         (1U << 27)
#define SD_ST_WP_VIOLATION        (1U << 26)
#define SD_ST_CARD_IS_LOCKED      (1U << 25)
#define SD_ST_LOCK_UNLOCK_FAILED  (1U << 24)
#define SD_ST_COM_CRC_ERROR       (1U << 23)
#define SD_ST_ILLEGAL_COMMAND     (1U << 22)
#define SD_ST_CARD_ECC_FAILED     (1U << 21)
#define SD_ST_CC_ERROR            (1U << 20)
#define SD_ST_ERROR               (1U << 19)
#define SD_ST_CSD_OVERWRITE       (1U << 16)
#define SD_ST_WP_ERASE_SKIP       (1U << 15)
#define SD_ST_ERASE_RESET         (1U << 13)

extern sdmmc_host_t sd_host;
extern sdmmc_slot_config_t sd_slot;
extern sdmmc_card_t sd_card;
extern sdmmc_card_t *sd_card_ptr;
extern bool sd_initialized;
extern bool sd_fs_mounted;
extern char sd_fs_base_path[32];
extern sdmmc_card_t *sd_fs_card;

esp_err_t sd_init();
esp_err_t sd_reinit();
esp_err_t sd_ensure_ready();
esp_err_t sd_probe_card();
esp_err_t sd_fs_sense(const char *base_path);
esp_log_level_t sd_host_log_suppress_begin();
void sd_host_log_suppress_end(esp_log_level_t old_sd_host_level);
void sd_fs_unmount_force();
void sd_deinit();
esp_err_t sd_read_sectors(uint32_t sec, uint32_t num);
esp_err_t sd_card_erase();
esp_err_t sd_fs_mount(const char *base_path, sdmmc_card_t **out_card);
esp_err_t sd_fs_mount_quiet(const char *base_path, sdmmc_card_t **out_card);
void sd_fs_unmount(const char *base_path, sdmmc_card_t *card);
esp_err_t sd_fs_list_dir(const char *base_path, const char *path);
esp_err_t sd_fs_read_file(const char *base_path, const char *path, void *dst, size_t dst_size, size_t *out_size);
int sd_fs_build_full_path(const char *base_path, const char *path, char *full, size_t full_size);
bool sd_has_retryable_errno(int err);
bool sd_error_needs_reinit(esp_err_t err);
void sd_log_cid(const sdmmc_cid_t *cid);
void sd_log_ocr(const sdmmc_card_t *card);
void sd_log_scr(const sdmmc_card_t *card);

void sdmmc_console_register_system_commands();

#if defined(CONFIG_IDF_TARGET_ESP32P4)
void sd_ldo_init();
#endif
