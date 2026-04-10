
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "helper.h"
#include "stream_client.h"

static const char * TAG = "stream";

#define RING_BUF_SIZE (64 * 1024)

static uint8_t * ring_buf = NULL;
static volatile uint32_t ring_head = 0;
static volatile uint32_t ring_tail = 0;
static volatile bool ring_eof = false;
static SemaphoreHandle_t ring_mutex = NULL;

static TaskHandle_t reader_task_handle = NULL;
static int stream_sock = -1;
static esp_http_client_handle_t stream_http = NULL;
static uint32_t stream_total_size = 0;
static uint32_t stream_read_size = 0;
static bool stream_is_gopher = false;

static void reader_task(void *arg);

static uint32_t ring_available(void)
{
  uint32_t h = ring_head;
  uint32_t t = ring_tail;
  if (h >= t) return h - t;
  return RING_BUF_SIZE - t + h;
}

static uint32_t ring_free(void)
{
  return RING_BUF_SIZE - ring_available() - 1;
}

static esp_err_t stream_ring_alloc(void)
{
  if (ring_buf) return ESP_OK;

  ring_buf = (uint8_t*)heap_caps_malloc(RING_BUF_SIZE, MALLOC_CAP_SPIRAM);
  if (!ring_buf)
  {
    ESP_LOGE(TAG, "ring buffer alloc failed");
    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "ring buffer allocated: %p", ring_buf);
  return ESP_OK;
}

static void stream_ring_free(void)
{
  if (!ring_buf) return;

  heap_caps_free(ring_buf);
  ring_buf = NULL;
  ESP_LOGI(TAG, "ring buffer freed");
}

static void stream_ring_reset(void)
{
  xSemaphoreTake(ring_mutex, portMAX_DELAY);
  ring_head = 0;
  ring_tail = 0;
  ring_eof = false;
  xSemaphoreGive(ring_mutex);
}

static void stream_transport_close(void)
{
  if (stream_http)
  {
    esp_http_client_close(stream_http);
    esp_http_client_cleanup(stream_http);
    stream_http = NULL;
  }

  if (stream_sock >= 0)
  {
    close(stream_sock);
    stream_sock = -1;
  }
}

static esp_err_t stream_reader_start(void)
{
  esp_err_t err = stream_ring_alloc();
  if (err != ESP_OK) return err;

  stream_ring_reset();
  stream_read_size = 0;

  if (xTaskCreatePinnedToCoreWithCaps(reader_task, "stream_reader", 4096, NULL, STREAMER_TASK_PRIO, &reader_task_handle, 0, MALLOC_CAP_INTERNAL) == pdPASS)
    return ESP_OK;

  stream_ring_free();
  return ESP_FAIL;
}

static void stream_state_reset(void)
{
  stream_total_size = 0;
  stream_read_size = 0;
  stream_is_gopher = false;
  stream_ring_reset();
}

static void reader_task(void *arg)
{
  uint8_t temp_buf[512];

  while (true)
  {
    int len = 0;

    if (stream_http)
    {
      len = esp_http_client_read(stream_http, (char*)temp_buf, sizeof(temp_buf));
    }
    else if (stream_sock >= 0)
    {
      len = read(stream_sock, temp_buf, sizeof(temp_buf));
    }

    if (len <= 0)
    {
      xSemaphoreTake(ring_mutex, portMAX_DELAY);
      ring_eof = true;
      xSemaphoreGive(ring_mutex);
      ESP_LOGI(TAG, "reader: EOF, total=%u", stream_read_size);
      break;
    }

    stream_read_size += len;

    // Wait for space in ring buffer
    while (true)
    {
      xSemaphoreTake(ring_mutex, portMAX_DELAY);
      uint32_t free = ring_free();
      xSemaphoreGive(ring_mutex);

      if (free >= (uint32_t) len) break;
      vTaskDelay(1);
    }

    // Write to ring buffer
    xSemaphoreTake(ring_mutex, portMAX_DELAY);
    uint32_t h = ring_head;
    for(int i = 0; i < len; i++)
    {
      ring_buf[h] = temp_buf[i];
      h = (h + 1) % RING_BUF_SIZE;
    }
    ring_head = h;
    xSemaphoreGive(ring_mutex);
  }

  reader_task_handle = NULL;
  vTaskDelete(NULL);
}

void stream_init(void)
{
  if (!ring_mutex)
    ring_mutex = xSemaphoreCreateMutex();

  stream_sock = -1;
  stream_http = NULL;
  reader_task_handle = NULL;
  stream_state_reset();

  ESP_LOGI(TAG, "init done");
}

static void stream_http_common(bool use_https)
{
  ESP_LOGI(TAG, "STREAM_%s_START", use_https ? "HTTPS" : "HTTP");

  stream_state_reset();

  if (!net.url[0])
  {
    ESP_LOGW(TAG, "URL not set");
    wr_reg8(ESP_REG_STATUS, 0x81);
    return;
  }

  char * url = (char*)net.url;
  const char * prefix = use_https ? "https://" : "http://";
  int prefix_len = use_https ? 8 : 7;

  if (strncmp(url, prefix, prefix_len) != 0)
  {
    ESP_LOGW(TAG, "invalid URL format");
    wr_reg8(ESP_REG_STATUS, 0x81);
    return;
  }

  ESP_LOGI(TAG, "GET %s", url);

  esp_http_client_config_t cfg = {};
  cfg.url = url;
  cfg.timeout_ms = 30000;
  cfg.buffer_size = 512;
  cfg.user_agent = "ZiFi/1.0";

  if (use_https)
    cfg.crt_bundle_attach = esp_crt_bundle_attach;

  stream_http = esp_http_client_init( & cfg);
  
  if (!stream_http)
  {
    ESP_LOGW(TAG, "init fail");
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  esp_err_t err = esp_http_client_open(stream_http, 0);
  
  if (err != ESP_OK)
  {
    ESP_LOGW(TAG, "open fail");
    esp_http_client_cleanup(stream_http);
    stream_http = NULL;
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  esp_http_client_fetch_headers(stream_http);
  int code = esp_http_client_get_status_code(stream_http);
  ESP_LOGI(TAG, "status %d", code);

  if (code < 200 || code >= 300)
  {
    esp_http_client_close(stream_http);
    esp_http_client_cleanup(stream_http);
    stream_http = NULL;
    wr_reg8(ESP_REG_STATUS, 0xB3);
    return;
  }

  int content_length = esp_http_client_get_content_length(stream_http);
  if (content_length < 0)
    stream_total_size = 0xFFFFFFFF;
  else
    stream_total_size = (uint32_t) content_length;

  ESP_LOGI(TAG, "connected, size=%u", stream_total_size);

  if (stream_reader_start() != ESP_OK)
  {
    stream_transport_close();
    wr_reg8(ESP_REG_STATUS, 0xB0);
    return;
  }

  wr_reg32(ESP_REG_DATA_SIZE, stream_total_size);
  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
}

void stream_http_start(void)
{
  stream_http_common(false);
}

void stream_https_start(void)
{
  stream_http_common(true);
}

void stream_gopher_start(void)
{
  ESP_LOGI(TAG, "STREAM_GOPHER_START");

  stream_state_reset();
  stream_is_gopher = true;

  if (!net.url[0])
  {
    ESP_LOGW(TAG, "URL not set");
    wr_reg8(ESP_REG_STATUS, 0x81);
    return;
  }

  char * url = (char*)net.url;

  if (strncmp(url, "gopher://", 9) != 0)
  {
    ESP_LOGW(TAG, "invalid URL format");
    wr_reg8(ESP_REG_STATUS, 0x81);
    return;
  }

  ESP_LOGI(TAG, "GET %s", url);

  char host[256];
  int port = 70;
  char selector[256];

  memset(host, 0, sizeof(host));
  memset(selector, 0, sizeof(selector));
  //strcpy(selector, "/");
  selector[0] = '\0';

  const char * p = url + 9;
  const char * slash = strchr(p, '/');
  const char * colon = strchr(p, ':');

  if (colon && (!slash || colon < slash))
  {
    int host_len = colon - p;
    strncpy(host, p, host_len);
    host[host_len] = 0;
    port = atoi(colon + 1);
    
    if (slash)
    {
      const char * sel_start = slash + 1;
      // Skip Gopher type character and slash if present (e.g., "9/" -> "")
      if (sel_start[0] && sel_start[1] == '/' && strchr("0123456789+gIihs", sel_start[0]))
        sel_start += 2;
      
      strncpy(selector, sel_start, sizeof(selector) - 1);
    }
  }
  else if (slash)
  {
    int host_len = slash - p;
    strncpy(host, p, host_len);
    host[host_len] = 0;
    const char * sel_start = slash + 1;
    
    // Skip Gopher type character and slash if present
    if (sel_start[0] && sel_start[1] == '/' && strchr("0123456789+gIihs", sel_start[0]))
      sel_start += 2;

    strncpy(selector, sel_start, sizeof(selector) - 1);
  }
  else
    strncpy(host, p, sizeof(host) - 1);

  struct hostent * server = gethostbyname(host);
  
  if (!server)
  {
    ESP_LOGW(TAG, "DNS lookup failed");
    wr_reg8(ESP_REG_STATUS, 0xB1);
    return;
  }

  stream_sock = socket(AF_INET, SOCK_STREAM, 0);
  
  if (stream_sock < 0)
  {
    ESP_LOGW(TAG, "socket creation failed");
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(stream_sock, (struct sockaddr*)& serv_addr, sizeof(serv_addr)) < 0)
  {
    ESP_LOGW(TAG, "connection failed");
    close(stream_sock);
    stream_sock = -1;
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  // Set socket timeout to 30 seconds
  struct timeval timeout;
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  setsockopt(stream_sock, SOL_SOCKET, SO_RCVTIMEO, & timeout, sizeof(timeout));

  char request[512];
  snprintf(request, sizeof(request), "/%s\r\n", selector);
  write(stream_sock, request, strlen(request));

  ESP_LOGI(TAG, "sent selector: [%s]", selector);

  // Size unknown from Gopher
  stream_total_size = 0xFFFFFFFF;

  ESP_LOGI(TAG, "connected");

  if (stream_reader_start() != ESP_OK)
  {
    stream_transport_close();
    wr_reg8(ESP_REG_STATUS, 0xB0);
    return;
  }

  wr_reg32(ESP_REG_DATA_SIZE, stream_total_size);
  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
}

void stream_read(void)
{
  if (stream_sock < 0 && !stream_http)
  {
    ESP_LOGW(TAG, "stream not opened");
    wr_reg8(ESP_REG_STATUS, 0x82);
    return;
  }

  // Wait for data in ring buffer or EOF
  int timeout = 0;
  
  while (timeout < 30000)
  {
    xSemaphoreTake(ring_mutex, portMAX_DELAY);
    uint32_t avail = ring_available();
    bool eof = ring_eof;
    xSemaphoreGive(ring_mutex);

    if (avail > 0 || eof) break;
    vTaskDelay(1);
    timeout++;
  }

  xSemaphoreTake(ring_mutex, portMAX_DELAY);
  uint32_t avail = ring_available();
  bool eof = ring_eof;

  if (avail == 0 && eof)
  {
    xSemaphoreGive(ring_mutex);
    ESP_LOGI(TAG, "stream end, total=%u", stream_read_size);
    wr_reg32(ESP_REG_DATA_SIZE, 0);
    wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
    return;
  }

  if (avail == 0)
  {
    xSemaphoreGive(ring_mutex);
    ESP_LOGW(TAG, "timeout waiting for data");
    wr_reg32(ESP_REG_DATA_SIZE, 0);
    wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
    return;
  }

  // Read up to 512 bytes from ring buffer
  uint32_t to_read = avail > 512 ? 512 : avail;
  uint8_t * dma = get_dma_buf();
  uint32_t t = ring_tail;

  for (uint32_t i = 0; i < to_read; i++)
  {
    dma[i] = ring_buf[t];
    t = (t + 1) % RING_BUF_SIZE;
  }
  ring_tail = t;

  xSemaphoreGive(ring_mutex);

  spi_slave_dma_send_direct(to_read);
}

void stream_close(void)
{
  if (reader_task_handle)
  {
    vTaskDelete(reader_task_handle);
    reader_task_handle = NULL;
  }

  stream_transport_close();

  ESP_LOGI(TAG, "stream closed, total read=%u", stream_read_size);

  stream_state_reset();
  stream_ring_free();

  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);
}
