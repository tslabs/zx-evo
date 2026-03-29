#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "mem_obj.h"
#include "http_client.h"

const char *TAG = "http";

#define HTTP_BUF_SIZE  (3 * 1024 * 1024)


void http_init()
{
}

void http_do_get()
{
  uint8_t *http_buf = NULL;
  uint8_t url_handle = rd_reg8(ESP_REG_OBJ_HANDLE);
  
  char *url;
  // size_t url_size;
  
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
  
  if (!mem_obj[url_handle].addr) 
  {
    ESP_LOGI(TAG, "bad handle %d", url_handle);
    set_status(0x80);
    goto cleanup;
  }
  
  url = (char*)mem_obj[url_handle].addr;
  // url_size = mem_obj[url_handle].size;
  
  ESP_LOGI(TAG, "GET %s", url);
  
  cfg.url = url;
  cfg.timeout_ms = 1000;
  cfg.buffer_size = 4096;
  cfg.user_agent = "ZiFi/1.0";
  
  client = esp_http_client_init(&cfg);
  if (!client) 
  {
    ESP_LOGI(TAG, "init fail");
    set_status(0xB2);
    goto cleanup;
  }
  
  err = esp_http_client_open(client, 0);
  if (err != ESP_OK) 
  {
    ESP_LOGI(TAG, "open fail");
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
  }
  
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  
  ESP_LOGI(TAG, "recv %d bytes", total);
  
  if (total == 0) 
  {
    set_status(0xB4);
    goto cleanup;
  }
  
  resp = make_obj(total, OBJ_TYPE_DATA);
  if (resp < 0) 
  {
    ESP_LOGI(TAG, "make_obj failed");
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

void http_stream_read_task() {}
void http_stream_delete(uint8_t handle) {}
bool http_is_stream_handle(uint8_t handle) { return false; }
void http_stream_read(uint8_t handle, uint32_t offset, uint32_t size) {}
