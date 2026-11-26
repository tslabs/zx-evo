// Internet radio of the audio stream player (ESP_CMD_AUDIO_URL): an HTTP / HTTPS
// stream (Icecast, SHOUTcast 2) read by its own task into the input ring of
// the player, the ICY metadata (StreamTitle) cut out of it, reconnects when
// the connection drops. The format comes from Content-Type (the player can not
// sniff a stream that starts in the middle of a frame).

#include "audio_radio.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_wifi.h"

#include "audio_stream.h"
#include "esp_spi_defs.h"

#define TAG "radio"

#define RADIO_STACK    8192
#define RADIO_PRIO     5
#define RADIO_BUF      2048
#define RADIO_TIMEOUT  3000       // ms: a read that waits longer gives up (and sees a stop)
#define RADIO_REDIRECT 5
#define RADIO_START_TRIES 3       // before the first byte: then the URL is taken as dead

bool wifi_is_connected();

static struct
{
  TaskHandle_t task;
  volatile u32 session;           // 0: none; a new one ends the old
  char url[1024];
  // of the response being read
  int metaint;
  char name[AUDIO_META_LEN];
  char ctype[48];
  u8 buf[RADIO_BUF];
  char meta[16 * 255 + 1];
} R;

static esp_err_t on_event(esp_http_client_event_t *e)
{
  if (e->event_id != HTTP_EVENT_ON_HEADER || !e->header_key || !e->header_value) return ESP_OK;
  if (!strcasecmp(e->header_key, "icy-metaint"))
  {
    // a number of audio bytes, 1 .. 1 GB; anything else (negative, garbage,
    // too large) is no metadata. A negative value made the length of the
    // audio piece negative: audio_radio_write read past R.buf
    char *end;
    long v = strtol(e->header_value, &end, 10);
    bool num = end != e->header_value;
    while (*end == ' ' || *end == '\t') end++;
    R.metaint = (num && !*end && v > 0 && v <= (1L << 30)) ? (int)v : 0;
  }
  else if (!strcasecmp(e->header_key, "icy-name"))
    strlcpy(R.name, e->header_value, sizeof(R.name));
  else if (!strcasecmp(e->header_key, "content-type"))
    strlcpy(R.ctype, e->header_value, sizeof(R.ctype));
  return ESP_OK;
}

static u8 fmt_of(const char *ct)
{
  if (!strncasecmp(ct, "audio/mpeg", 10) || !strncasecmp(ct, "audio/mp3", 9)) return AUDIO_FMT_MP3;
  if (!strncasecmp(ct, "audio/aac", 9) || !strncasecmp(ct, "audio/aacp", 10) || !strncasecmp(ct, "audio/x-aac", 11))
    return AUDIO_FMT_AAC;
  if (!strncasecmp(ct, "audio/ogg", 9) || !strncasecmp(ct, "application/ogg", 15)) return AUDIO_FMT_OGG;
  if (!strncasecmp(ct, "audio/flac", 10)) return AUDIO_FMT_FLAC;
  return AUDIO_FMT_AUTO;
}

// "StreamTitle='Artist - Song';StreamUrl='';" -> the title
static void meta_title(const char *m)
{
  const char *p = strstr(m, "StreamTitle='");
  if (!p) return;
  p += 13;
  const char *e = strstr(p, "';");
  size_t n = e ? (size_t)(e - p) : strlen(p);
  char t[AUDIO_META_LEN];
  if (n >= sizeof(t)) n = sizeof(t) - 1;
  memcpy(t, p, n);
  t[n] = 0;
  audio_radio_meta(R.session, NULL, t);
}

// One connection. Returns the audio bytes it delivered (0: nothing came);
// it ends when the session changes or the connection drops.
static size_t radio_connect(u32 s)
{
  // the modem sleep of the default power save (MIN_MODEM) cut the stream rate
  // of a 128 kbit/s station to 5..8 KB/s for half a minute: off while radio plays
  esp_wifi_set_ps(WIFI_PS_NONE);

  R.metaint = 0;
  R.name[0] = 0;
  R.ctype[0] = 0;

  esp_http_client_config_t cfg = {};
  cfg.url = R.url;
  cfg.timeout_ms = RADIO_TIMEOUT;
  cfg.buffer_size = 2048;
  cfg.buffer_size_tx = 1024;
  cfg.user_agent = "ZiFi32";
  cfg.event_handler = on_event;
  cfg.crt_bundle_attach = esp_crt_bundle_attach;
  esp_http_client_handle_t c = esp_http_client_init(&cfg);
  if (!c) return 0;
  esp_http_client_set_header(c, "Icy-MetaData", "1");

  size_t got = 0;
  int code = 0;
  for (int i = 0; i <= RADIO_REDIRECT; i++)
  {
    if (esp_http_client_open(c, 0) != ESP_OK) goto out;
    esp_http_client_fetch_headers(c);
    code = esp_http_client_get_status_code(c);
    if (code < 300 || code >= 400) break;
    ESP_LOGI(TAG, "redirect %d", code);
    if (esp_http_client_set_redirection(c) != ESP_OK) goto out;
    esp_http_client_close(c);
  }
  if (code < 200 || code >= 300)
  {
    ESP_LOGW(TAG, "HTTP %d", code);
    goto out;
  }

  ESP_LOGI(TAG, "connected: %s, metaint %d, '%s'", R.ctype, R.metaint, R.name);
  audio_radio_meta(s, R.name, NULL);
  audio_radio_format(s, fmt_of(R.ctype));

  {
    int audio_left = R.metaint;   // audio bytes before the next metadata block
    int meta_len = -1, meta_pos = 0;
    while (s == R.session)
    {
      int n = esp_http_client_read(c, (char*)R.buf, RADIO_BUF);
      if (n <= 0) break;
      int p = 0;
      while (p < n)
      {
        if (!R.metaint || audio_left)
        {
          int k = n - p;
          if (R.metaint && k > audio_left) k = audio_left;
          if (!audio_radio_write(s, R.buf + p, k)) goto out;
          got += k;
          p += k;
          if (R.metaint) audio_left -= k;
          continue;
        }
        if (meta_len < 0)
        {
          meta_len = R.buf[p++] * 16;   // the length byte of a metadata block
          meta_pos = 0;
        }
        while (p < n && meta_pos < meta_len)
          R.meta[meta_pos++] = (char)R.buf[p++];
        if (meta_pos >= meta_len)
        {
          if (meta_len)
          {
            R.meta[meta_len] = 0;
            meta_title(R.meta);
          }
          meta_len = -1;
          audio_left = R.metaint;
        }
      }
    }
  }

out:
  esp_http_client_close(c);
  esp_http_client_cleanup(c);
  return got;
}

static void radio_task(void *arg)
{
  while (1)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    u32 s = R.session;
    int fails = 0;
    bool played = false;
    while (s && s == R.session)
    {
      if (!wifi_is_connected())
      {
        ESP_LOGW(TAG, "no Wi-Fi");
        audio_radio_error(s, ESP_ERR_AUDIO_NET);
        break;
      }
      audio_radio_connecting(s, true);
      size_t got = radio_connect(s);
      if (s != R.session) break;
      audio_radio_connecting(s, true);
      if (got) { played = true; fails = 0; }
      else fails++;
      if (!played && fails >= RADIO_START_TRIES)
      {
        audio_radio_error(s, ESP_ERR_AUDIO_NET);
        break;
      }
      // a dropped stream: again after 1, 2, 4 .. 16 s
      int wait = 1000 << (fails < 4 ? fails : 4);
      ESP_LOGI(TAG, "reconnect in %d ms", wait);
      for (int t = 0; t < wait && s == R.session; t += 100) vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
}

void radio_init()
{
  xTaskCreatePinnedToCore(radio_task, "radio", RADIO_STACK, NULL, RADIO_PRIO, &R.task, 0);
}

void radio_start(u32 session, const char *url)
{
  strlcpy(R.url, url, sizeof(R.url));
  R.session = session;
  if (R.task) xTaskNotifyGive(R.task);
}

void radio_stop()
{
  R.session = 0;
}
