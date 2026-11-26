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
#include "net_util.h"
#include "gopher_client.h"

static const char *TAG = "gopher";

#define GOPHER_BUF_SIZE  (2 * 1024 * 1024)

static uint8_t *gopher_buf = NULL;
static size_t gopher_buf_size = 0;
static int gopher_sock = -1;

void gopher_buf_free()
{
  if (!gopher_buf) return;

  heap_caps_free(gopher_buf);
  gopher_buf = NULL;
  gopher_buf_size = 0;
  ESP_LOGI(TAG, "buffer freed");
}

bool gopher_buf_alloc()
{
  if (gopher_buf) return true;

  gopher_buf = (uint8_t*)heap_caps_malloc(GOPHER_BUF_SIZE, MALLOC_CAP_SPIRAM);
  if (!gopher_buf)
  {
    ESP_LOGW(TAG, "SPIRAM buffer alloc failed, size=%u", (unsigned)GOPHER_BUF_SIZE);
    return false;
  }

  gopher_buf_size = GOPHER_BUF_SIZE;
  ESP_LOGI(TAG, "buffer allocated in SPIRAM: %p, size=%u", gopher_buf, (unsigned)gopher_buf_size);
  return true;
}

void gopher_close_socket()
{
  if (gopher_sock < 0) return;

  close(gopher_sock);
  gopher_sock = -1;
}

void gopher_cleanup()
{
  gopher_close_socket();
  gopher_buf_free();
}

void gopher_init()
{
  gopher_buf = NULL;
  gopher_buf_size = 0;
  gopher_sock = -1;

  ESP_LOGI(TAG, "init done");
}

void gopher_do_get()
{
  ESP_LOGI(TAG, "GOPHER_GET");

  if (!gopher_buf_alloc())
  {
    set_status(0xA0);
    return;
  }

  if (!net.url[0])
  {
    ESP_LOGW(TAG, "URL not set");
    gopher_cleanup();
    set_status(0x81);
    return;
  }

  char * url = (char*)net.url;

  if (strncmp(url, "gopher://", 9) != 0)
  {
    ESP_LOGW(TAG, "invalid URL format");
    gopher_cleanup();
    set_status(0x81);
    return;
  }

  ESP_LOGI(TAG, "GET %s", url);

  char host[256];
  int port = 70;
  char selector[256];

  memset(host, 0, sizeof(host));
  memset(selector, 0, sizeof(selector));
  selector[0] = '\0';

  const char * p = url + 9;
  const char * slash = strchr(p, '/');
  const char * colon = strchr(p, ':');

  if (colon && (!slash || colon < slash))
  {
    int host_len = colon - p;
    if (host_len > (int)sizeof(host) - 1) host_len = sizeof(host) - 1;
    strncpy(host, p, host_len);
    host[host_len] = 0;
    port = atoi(colon + 1);

    if (slash)
    {
      const char * sel_start = slash + 1;

      if (sel_start[0] && sel_start[1] == '/' && strchr("0123456789+gIihs", sel_start[0]))
        sel_start += 2;

      strncpy(selector, sel_start, sizeof(selector) - 1);
      selector[sizeof(selector) - 1] = 0;
    }
  }
  else if (slash)
  {
    int host_len = slash - p;
    if (host_len > (int)sizeof(host) - 1) host_len = sizeof(host) - 1;
    strncpy(host, p, host_len);
    host[host_len] = 0;
    const char * sel_start = slash + 1;

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

  struct hostent * server = gethostbyname(host);
  if (!server)
  {
    ESP_LOGW(TAG, "DNS lookup failed");
    gopher_cleanup();
    set_status(0xB1);
    return;
  }

  uint32_t addr;
  memcpy(&addr, server->h_addr, sizeof(addr));

  // 10 s to connect, 30 s per read / write (gopher had none: a silent server
  // blocked the helper task for good)
  gopher_sock = net_connect_timeout(addr, port, 10000, 30000);
  if (gopher_sock < 0)
  {
    ESP_LOGW(TAG, "connection failed");
    gopher_cleanup();
    set_status(0xB2);
    return;
  }

  ESP_LOGI(TAG, "connected");

  char request[512];
  memset(request, 0, sizeof(request));
  snprintf(request, sizeof(request), "/%s\r\n", selector);

  int sent = write(gopher_sock, request, strlen(request));
  if (sent < 0)
  {
    ESP_LOGW(TAG, "send failed");
    gopher_cleanup();
    set_status(0xB2);
    return;
  }

  ESP_LOGI(TAG, "sent %d bytes", sent);

  int total = 0;
  int len;
  int last_progress = 0;

  while ((len = read(gopher_sock, (char*)gopher_buf + total, gopher_buf_size - total)) > 0)
  {
    total += len;
    if ((size_t)total >= gopher_buf_size) break;

    if (total - last_progress >= 65536)
    {
      wr_reg32(ESP_REG_DATA_SIZE, total);
      last_progress = total;
    }
  }

  gopher_close_socket();

  ESP_LOGI(TAG, "recv %d bytes total", total);

  if (total == 0)
  {
    ESP_LOGW(TAG, "empty response");
    gopher_buf_free();
    set_status(0xB4);
    return;
  }

  int resp = make_obj(total, OBJ_TYPE_DATA);
  if (resp < 0)
  {
    ESP_LOGW(TAG, "make_obj failed");
    gopher_buf_free();
    set_status(0xA0);
    return;
  }

  memcpy(mem_obj[resp].addr, gopher_buf, total);
  gopher_buf_free();

  wr_reg8(ESP_REG_OBJ_HANDLE, resp);
  wr_reg32(ESP_REG_DATA_SIZE, total);
  set_status(ESP_ST_READY);

  ESP_LOGI(TAG, "done: handle=%d size=%d", resp, total);
}
