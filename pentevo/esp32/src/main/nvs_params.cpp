#include <string.h>
#include <stdlib.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "main.h"
#include "nvs_params.h"

app_params_t app_params;

char NVS_PARAMS_NS[] = "appcfg";
char NVS_KEY_USB_MODE[]  = "usb_mode";
char NVS_KEY_WIFI_MODE[] = "wifi_mode";
char NVS_KEY_WIFI_AP[]   = "wifi_ap";
char NVS_KEY_WIFI_PSW[]  = "wifi_psw";

void initialize_nvs()
{
  esp_err_t err = nvs_flash_init();
  log_sram_used(__FILE_NAME__ ": nvs_flash_init");

  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  log_sram_used(__FILE_NAME__ ": initialize_nvs end");
}

void app_params_set_defaults()
{
  memset(&app_params, 0, sizeof(app_params));
}

esp_err_t app_params_load_u8(nvs_handle_t h, const char *key, uint8_t *value)
{
  esp_err_t err = nvs_get_u8(h, key, value);
  if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
  return err;
}

esp_err_t app_params_load_str(nvs_handle_t h, const char *key, char *buf, size_t buf_size)
{
  size_t size = buf_size;
  esp_err_t err = nvs_get_str(h, key, buf, &size);
  if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
  if (err == ESP_ERR_NVS_INVALID_LENGTH)
  {
    buf[0] = 0;
    return ESP_OK;
  }
  return err;
}

esp_err_t app_params_load()
{
  app_params_set_defaults();

  nvs_handle_t h = 0;
  esp_err_t err = nvs_open(NVS_PARAMS_NS, NVS_READONLY, &h);
  if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
  if (err != ESP_OK) return err;

  err = app_params_load_u8(h, NVS_KEY_USB_MODE, &app_params.usb_mode);
  if (err != ESP_OK) goto end;

  err = app_params_load_u8(h, NVS_KEY_WIFI_MODE, &app_params.wifi_mode);
  if (err != ESP_OK) goto end;

  err = app_params_load_str(h, NVS_KEY_WIFI_AP, app_params.wifi_ap, sizeof(app_params.wifi_ap));
  if (err != ESP_OK) goto end;

  err = app_params_load_str(h, NVS_KEY_WIFI_PSW, app_params.wifi_psw, sizeof(app_params.wifi_psw));
  if (err != ESP_OK) goto end;

end:
  nvs_close(h);
  return err;
}

esp_err_t app_params_save()
{
  nvs_handle_t h = 0;
  esp_err_t err = nvs_open(NVS_PARAMS_NS, NVS_READWRITE, &h);
  if (err != ESP_OK) return err;

  err = nvs_set_u8(h, NVS_KEY_USB_MODE, app_params.usb_mode);
  if (err != ESP_OK) goto end;

  err = nvs_set_u8(h, NVS_KEY_WIFI_MODE, app_params.wifi_mode);
  if (err != ESP_OK) goto end;

  err = nvs_set_str(h, NVS_KEY_WIFI_AP, app_params.wifi_ap);
  if (err != ESP_OK) goto end;

  err = nvs_set_str(h, NVS_KEY_WIFI_PSW, app_params.wifi_psw);
  if (err != ESP_OK) goto end;

  err = nvs_commit(h);

end:
  nvs_close(h);
  return err;
}

esp_err_t app_params_reset()
{
  app_params_set_defaults();
  return app_params_save();
}

bool app_params_set_by_name(const char *name, const char *value)
{
  char *endp = NULL;
  unsigned long v = 0;

  if (!strcmp(name, "usb_mode"))
  {
    v = strtoul(value, &endp, 0);
    if (!endp || *endp || v > 255) return false;
    app_params.usb_mode = (uint8_t)v;
    return true;
  }

  if (!strcmp(name, "wifi_mode"))
  {
    v = strtoul(value, &endp, 0);
    if (!endp || *endp || v > 255) return false;
    app_params.wifi_mode = (uint8_t)v;
    return true;
  }

  if (!strcmp(name, "wifi_ap"))
  {
    strlcpy(app_params.wifi_ap, value, sizeof(app_params.wifi_ap));
    return true;
  }

  if (!strcmp(name, "wifi_psw"))
  {
    strlcpy(app_params.wifi_psw, value, sizeof(app_params.wifi_psw));
    return true;
  }

  return false;
}
