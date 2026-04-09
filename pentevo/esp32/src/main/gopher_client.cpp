#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "mem_obj.h"
#include "helper.h"
#include "gopher_client.h"

static const char *TAG = "gopher";

#define GOPHER_BUF_SIZE  (2 * 1024 * 1024)
static uint8_t *gopher_buf = NULL;
static int gopher_sock = -1;

void gopher_init()
{
  gopher_buf = (uint8_t*)heap_caps_malloc(GOPHER_BUF_SIZE, MALLOC_CAP_SPIRAM);

  if (!gopher_buf)
    gopher_buf = (uint8_t*)malloc(16 * 1024);

  ESP_LOGI(TAG, "init done, buf=%p", gopher_buf);
}

void gopher_do_get()
{
  ESP_LOGI(TAG, "GOPHER_GET");

  if (!gopher_buf)
  {
    gopher_buf = (uint8_t*)heap_caps_malloc(GOPHER_BUF_SIZE, MALLOC_CAP_SPIRAM);

    if (!gopher_buf)
    {
      wr_reg8(ESP_REG_STATUS, 0xA0);
      return;
    }
  }

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

  // Parse gopher://host[:port]/selector
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
      selector[sizeof(selector) - 1] = 0;
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
    selector[sizeof(selector) - 1] = 0;
  }
  else
  {
    strncpy(host, p, sizeof(host) - 1);
    host[sizeof(host) - 1] = 0;
  }

  ESP_LOGI(TAG, "host=%s port=%d selector=%s", host, port, selector);

  // DNS lookup
  struct hostent * server = gethostbyname(host);

  if (!server)
  {
    ESP_LOGW(TAG, "DNS lookup failed");
    wr_reg8(ESP_REG_STATUS, 0xB1);
    return;
  }

  // Create socket
  gopher_sock = socket(AF_INET, SOCK_STREAM, 0);

  if (gopher_sock < 0)
  {
    ESP_LOGW(TAG, "socket creation failed");
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  // Connect
  struct sockaddr_in serv_addr;
  memset( & serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  memcpy( & serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

  if (connect(gopher_sock, (struct sockaddr*)& serv_addr, sizeof(serv_addr)) < 0)
  {
    ESP_LOGW(TAG, "connection failed");
    close(gopher_sock);
    gopher_sock = -1;
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  ESP_LOGI(TAG, "connected");

  // Send request
  char request[512];
  memset(request, 0, sizeof(request));
  snprintf(request, sizeof(request), "/%s\r\n", selector);

  int sent = write(gopher_sock, request, strlen(request));

  if (sent < 0)
  {
    ESP_LOGW(TAG, "send failed");
    close(gopher_sock);
    gopher_sock = -1;
    wr_reg8(ESP_REG_STATUS, 0xB2);
    return;
  }

  ESP_LOGI(TAG, "sent %d bytes", sent);

  // Receive response
  int total = 0;
  int len;
  int last_progress = 0;

  while((len = read(gopher_sock, (char*)gopher_buf + total, GOPHER_BUF_SIZE - total)) > 0)
  {
    total += len;
    if (total >= GOPHER_BUF_SIZE) break;

    // Update progress every 64KB
    if (total - last_progress >= 65536)
    {
      wr_reg32(ESP_REG_DATA_SIZE, total);
      last_progress = total;
    }

    vTaskDelay(1);
  }

  close(gopher_sock);
  gopher_sock = -1;

  ESP_LOGI(TAG, "recv %d bytes total", total);

  if (total == 0)
  {
    ESP_LOGW(TAG, "empty response");
    wr_reg8(ESP_REG_STATUS, 0xB4);
    return;
  }

  int resp = make_obj(total, OBJ_TYPE_DATA);
  if (resp < 0)
  {
    ESP_LOGW(TAG, "make_obj failed");
    wr_reg8(ESP_REG_STATUS, 0xA0);
    return;
  }

  memcpy(mem_obj[resp].addr, gopher_buf, total);

  // Return result
  wr_reg8(ESP_REG_OBJ_HANDLE, resp);
  wr_reg32(ESP_REG_DATA_SIZE, total);
  wr_reg8(ESP_REG_STATUS, ESP_ST_READY);

  ESP_LOGI(TAG, "done: handle=%d size=%d", resp, total);
}
