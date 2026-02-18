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

static const char *TAG = "http";

#define HTTP_BUF_SIZE  (700 * 1024)
static uint8_t *http_buf = NULL;

void http_init(void)
{
    http_buf = (uint8_t*)heap_caps_malloc(HTTP_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (!http_buf) {
        http_buf = (uint8_t*)malloc(16 * 1024);
    }
    ESP_LOGW(TAG, "init done, buf=%p", http_buf);
}

void http_do_get(void)
{
    uint8_t url_handle = rd_reg8(ESP_REG_OBJ_HANDLE);
    
    ESP_LOGW(TAG, "HTTP_GET handle=%d", url_handle);
    
    if (!http_buf) {
        http_buf = (uint8_t*)heap_caps_malloc(HTTP_BUF_SIZE, MALLOC_CAP_SPIRAM);
        if (!http_buf) {
            wr_reg8(ESP_REG_STATUS, 0xA0);
            return;
        }
    }
    
    if (!mem_obj[url_handle].addr) {
        ESP_LOGW(TAG, "bad handle %d", url_handle);
        wr_reg8(ESP_REG_STATUS, 0x80);
        return;
    }
    
    char *url = (char*)mem_obj[url_handle].addr;
    size_t url_size = mem_obj[url_handle].size;
    
    // Показываем первые байты как hex
    ESP_LOGW(TAG, "URL obj size=%d, hex: %02X %02X %02X %02X %02X %02X %02X %02X",
             url_size,
             (uint8_t)url[0], (uint8_t)url[1], (uint8_t)url[2], (uint8_t)url[3],
             (uint8_t)url[4], (uint8_t)url[5], (uint8_t)url[6], (uint8_t)url[7]);
    
    // Проверяем что URL начинается с http
    if (url_size < 7 || (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0)) {
        ESP_LOGW(TAG, "invalid URL format");
        wr_reg8(ESP_REG_STATUS, 0x81);
        return;
    }
    
    ESP_LOGW(TAG, "GET %s", url);
    
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.timeout_ms = 30000;
    cfg.buffer_size = 4096;
    cfg.user_agent = "ZiFi/1.0";
    
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGW(TAG, "init fail");
        wr_reg8(ESP_REG_STATUS, 0xB2);
        return;
    }
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "open fail");
        esp_http_client_cleanup(client);
        wr_reg8(ESP_REG_STATUS, 0xB2);
        return;
    }
    
    esp_http_client_fetch_headers(client);
    int code = esp_http_client_get_status_code(client);
    ESP_LOGW(TAG, "status %d", code);
    
    if (code < 200 || code >= 300) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        wr_reg8(ESP_REG_STATUS, 0xB3);
        return;
    }
    
    int total = 0;
    int len;
    int last_progress = 0;
    while ((len = esp_http_client_read(client, (char*)http_buf + total, HTTP_BUF_SIZE - total)) > 0) 
    {
        ESP_LOGW(TAG, "get: %d bytes", len);
        
        total += len;
        if (total >= HTTP_BUF_SIZE) break;
        
        // Обновляем прогресс каждые 64KB - Z80 может проверять что значение меняется
        if (total - last_progress >= 65536) 
        {
            wr_reg32(ESP_REG_DATA_SIZE, total);
            last_progress = total;
            ESP_LOGW(TAG, "progress: %d bytes", total);
        }
        
        vTaskDelay(1);
    }
    
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    
    ESP_LOGW(TAG, "recv %d bytes", total);
    
    if (total == 0) {
        ESP_LOGW(TAG, "empty response");
        wr_reg8(ESP_REG_STATUS, 0xB4);
        return;
    }
    
    int resp = make_obj(total, OBJ_TYPE_DATA);
    if (resp < 0) {
        ESP_LOGW(TAG, "make_obj failed");
        wr_reg8(ESP_REG_STATUS, 0xA0);
        return;
    }
    
    memcpy(mem_obj[resp].addr, http_buf, total);
    
    // Возвращаем результат
    wr_reg8(ESP_REG_OBJ_HANDLE, resp);
    wr_reg32(ESP_REG_DATA_SIZE, total);
    wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
    
    ESP_LOGW(TAG, "done: handle=%d size=%d", resp, total);
}

// Заглушки для streaming API (не используется пока)
void http_stream_read_task(void) {}
void http_stream_delete(uint8_t handle) {}
bool http_is_stream_handle(uint8_t handle) { return false; }
void http_stream_read(uint8_t handle, uint32_t offset, uint32_t size) {}
