#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

struct app_params_t
{
  uint8_t usb_mode;
  uint8_t wifi_mode;
  char wifi_ap[64];
  char wifi_psw[64];
};

extern app_params_t app_params;

void initialize_nvs();
void app_params_set_defaults();
esp_err_t app_params_load();
esp_err_t app_params_save();
esp_err_t app_params_reset();
bool app_params_set_by_name(const char *name, const char *value);
