#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "esp_crt_bundle.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "mem_obj.h"
#include "helper.h"
#include "http_client.h"

const char *TAG = "http";

#define HTTP_BUF_SIZE (2 * 1024 * 1024)

void http_init() {}

void http_do_get()
{
  uint8_t *http_buf = NULL;
  uint8_t url_handle = rd_reg8(ESP_REG_OBJ_HANDLE);

  char *url;
  int total = 0;
  int len;
  int code;

  esp_http_client_handle_t client;
  esp_http_client_config_t cfg = {};

  esp_err_t err;
  int resp;

  ESP_LOGI(TAG, "HTTP_GET handle=%d", url_handle);

  http_buf = (uint8_t*)heap_caps_malloc(HTTP_BUF_SIZE, MALLOC_CAP_SPIRAM);

  if (!http_buf)
  {
    set_status(0xA0);
    ESP_LOGE(TAG, "HTTP buf malloc error");
    return;
  }

  if (!net.url[0])
  {
    ESP_LOGW(TAG, "URL not set");
    set_status(0x81);
    return;
  }

  url = (char*)net.url;

  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0)
  {
    ESP_LOGW(TAG, "invalid URL format");
    set_status(0x81);
    return;
  }

  ESP_LOGI(TAG, "GET %s", url);

  cfg.url = url;
  cfg.timeout_ms = 30000;
  cfg.buffer_size = 4096;
  cfg.user_agent = "ZiFi/1.0";

  // Enable TLS for HTTPS
  if (strncmp(url, "https://", 8) == 0)
    cfg.crt_bundle_attach = esp_crt_bundle_attach;

  client = esp_http_client_init(&cfg);

  if (!client)
  {
    ESP_LOGW(TAG, "init fail");
    set_status(0xB2);
    return;
  }

  err = esp_http_client_open(client, 0);
  if (err != ESP_OK)
  {
    ESP_LOGW(TAG, "open fail");
    esp_http_client_cleanup(client);
    set_status(0xB2);
    goto cleanup;
  }

  esp_http_client_fetch_headers(client);
  code = esp_http_client_get_status_code(client);
  ESP_LOGI(TAG, "status %d", code);

  if (code < 200 || code >= 300)
  {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    set_status(0xB3);
    goto cleanup;
  }

  for (;;)
  {
    len = esp_http_client_read(client, (char*)http_buf + total, HTTP_BUF_SIZE - total);

    if (len > 0)
    {
      total += len;

      wr_reg32(ESP_REG_DATA_SIZE, total);
      ESP_LOGI(TAG, "got: %d bytes, progress: %d bytes", len, total);

      if (total >= HTTP_BUF_SIZE) break;
    }

    else if (len == -ESP_ERR_HTTP_EAGAIN)
      continue;

    else if (len == 0)
    {
      ESP_LOGI(TAG, "empty response received");
      break;
    }

    else
    {
      ESP_LOGE(TAG, "error: %d", -len);
      break;
    }

    vTaskDelay(1);
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  ESP_LOGI(TAG, "recv %d bytes", total);

  if (total == 0)
  {
    ESP_LOGW(TAG, "empty response");
    set_status(0xB4);
    goto cleanup;
  }

  resp = make_obj(total, OBJ_TYPE_DATA);
  if (resp < 0)
  {
    ESP_LOGE(TAG, "make_obj failed");
    set_status(0xA0);
    goto cleanup;
  }

  memcpy(mem_obj[resp].addr, http_buf, total);

  wr_reg8(ESP_REG_OBJ_HANDLE, resp);
  wr_reg32(ESP_REG_DATA_SIZE, total);
  set_status(ESP_ST_READY);

  ESP_LOGI(TAG, "done: handle=%d size=%d", resp, total);

cleanup:
  if (http_buf) free(http_buf);
}

void https_do_get()
{
  // HTTPS uses same logic as HTTP - esp_http_client handles both
  // Just need to ensure URL starts with https://
  http_do_get();
}

void http_stream_read_task() {}
void http_stream_delete(uint8_t handle) {}
bool http_is_stream_handle(uint8_t handle) { return false; }
void http_stream_read(uint8_t handle, uint32_t offset, uint32_t size) {}
