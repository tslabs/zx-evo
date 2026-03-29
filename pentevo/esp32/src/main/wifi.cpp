
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "main.h"
#include "mem_obj.h"
#include "esp_spi_defs.h"
#include "spi_slave.h"
#include "wifi.h"
#include "http_client.h"

const char TAG[] = "wifi.cpp";

EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

EXT_RAM_BSS_ATTR wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
esp_netif_ip_info_t ip;

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

      bool autoconnect = false;

      if (autoconnect)
      {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
      }
    }

    else if (event_id == WIFI_EVENT_WIFI_READY)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_WIFI_READY");
    }

    else if (event_id == WIFI_EVENT_SCAN_DONE)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");
    }

    else if (event_id == WIFI_EVENT_STA_START)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
    }

    else if (event_id == WIFI_EVENT_STA_STOP)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");
    }

    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
    }

    else if (event_id == WIFI_EVENT_STA_BSS_RSSI_LOW)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_BSS_RSSI_LOW");
    }
  }

  else if (event_base == IP_EVENT)
  {
    if (event_id == IP_EVENT_STA_GOT_IP)
    {
      ip_event_got_ip_t *event = (ip_event_got_ip_t*)event_data;
      ip = event->ip_info;
      xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

      ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
  }
}

void get_ip(u8 *i, u8 *m, u8 *g)
{
  i[0] = ((u8*)&ip.ip)[0];
  i[1] = ((u8*)&ip.ip)[1];
  i[2] = ((u8*)&ip.ip)[2];
  i[3] = ((u8*)&ip.ip)[3];
  m[0] = ((u8*)&ip.netmask)[0];
  m[1] = ((u8*)&ip.netmask)[1];
  m[2] = ((u8*)&ip.netmask)[2];
  m[3] = ((u8*)&ip.netmask)[3];
  g[0] = ((u8*)&ip.gw)[0];
  g[1] = ((u8*)&ip.gw)[1];
  g[2] = ((u8*)&ip.gw)[2];
  g[3] = ((u8*)&ip.gw)[3];
};

void initialize_wifi()
{
  esp_log_level_set("wifi", ESP_LOG_WARN);

  static bool initialized = false;

  if (initialized) return;

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);
  (void)sta_netif;

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  initialized = true;
}

bool wifi_connect(const char *ssid, const char *pass, int timeout_ms)
{
  initialize_wifi();

  wifi_config_t wifi_config = { 0 };
  strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (pass) strlcpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_wifi_connect();

  int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);

  return (bits & CONNECTED_BIT) != 0;
}

int wf_scan(int timeout)
{
  initialize_wifi();

  memset(ap_info, 0, sizeof(ap_info));
  wifi_scan_config_t cfg = {};
  cfg.show_hidden = false;
  cfg.scan_type = WIFI_SCAN_TYPE_PASSIVE;
  cfg.scan_time.passive = timeout;
  // cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;
  // cfg.scan_time.active.min = 0;
  // cfg.scan_time.active.max = 300;

  return esp_wifi_scan_start(&cfg, true);
}

uint16_t wf_get_ap_num()
{
  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  uint16_t ap_count = 0;
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

  return ap_count;
}

void wf_get_ap(int idx, u8 &auth, i8 &rssi, u8 &chan, u8 *&ssid)
{
  auth = ap_info[idx].authmode;
  rssi = ap_info[idx].rssi;
  chan = ap_info[idx].primary;
  ssid = ap_info[idx].ssid;
}

// ------------- Console ---------------

void print_auth_mode(int authmode)
{
  switch (authmode)
  {
    case WIFI_AUTH_OPEN:          printf("OPEN\t"); break;
    case WIFI_AUTH_OWE:           printf("OWE\t"); break;
    case WIFI_AUTH_WEP:           printf("WEP\t"); break;
    case WIFI_AUTH_WPA_PSK:       printf("WPA_PSK"); break;
    case WIFI_AUTH_WPA2_PSK:      printf("WPA2_PSK"); break;
    case WIFI_AUTH_WPA_WPA2_PSK:  printf("WPA_WPA2_PSK"); break;
    case WIFI_AUTH_ENTERPRISE:    printf("ENTERPRISE"); break;
    case WIFI_AUTH_WPA3_PSK:      printf("WPA3_PSK"); break;
    case WIFI_AUTH_WPA2_WPA3_PSK: printf("WPA2_WPA3_PSK"); break;
    case WIFI_AUTH_WPA3_ENT_192:  printf("WPA3_ENT_192"); break;
    default:                      printf("UNKNOWN"); break;
  }
}

void print_cipher_type(int cipher)
{
  switch (cipher)
  {
    case WIFI_CIPHER_TYPE_NONE:         printf("NONE\t"); break;
    case WIFI_CIPHER_TYPE_WEP40:        printf("WEP40\t"); break;
    case WIFI_CIPHER_TYPE_WEP104:       printf("WEP104"); break;
    case WIFI_CIPHER_TYPE_TKIP:         printf("TKIP\t"); break;
    case WIFI_CIPHER_TYPE_CCMP:         printf("CCMP\t"); break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:    printf("TKIP_CCMP"); break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:  printf("AES_CMAC128"); break;
    case WIFI_CIPHER_TYPE_SMS4:         printf("SMS4\t"); break;
    case WIFI_CIPHER_TYPE_GCMP:         printf("GCMP\t"); break;
    case WIFI_CIPHER_TYPE_GCMP256:      printf("GCMP256"); break;
    default:                            printf("UNKNOWN"); break;
  }
}

int parse_response(char *buffer, size_t length, size_t &content_index)
{
  const uint8_t delimiter[] = {0x0D, 0x0A, 0x0D, 0x0A};
  const char *content_length_field = "content-length: ";
  size_t content_length = 0;
  size_t eoh = sizeof(delimiter);
  size_t i;
  content_index = 0;

  // Search for the end of headers (double CRLF)
  for (i = 0; i <= length - eoh; i++)
    if (memcmp(&buffer[i], delimiter, eoh) == 0)
      break;

  if (i > length - eoh)
  {
    printf("End of headers not found.\n");
    return -1;
  }

  // Print the ASCII headers
  for (size_t j = 0; j < i; j++)
    putchar(buffer[j]);

  // Convert to lowercase
  for (size_t j = 0; j < i; j++)
    buffer[j] = tolower((unsigned char)buffer[j]);

  content_index = i + eoh;
  printf("\n\nFirst content byte index: %zu\n", content_index);

  // Search for the "Content-Length" field and extract its value
  for (size_t i = 0; i < content_index - strlen(content_length_field); i++)
    if (strncmp((const char *)&buffer[i], content_length_field, strlen(content_length_field)) == 0)
    {
      // Skip the field and extract the value
      sscanf((const char *)&buffer[i + strlen(content_length_field)], "%zu", &content_length);
      printf("Content length: %zu\n", content_length);
      break;
    }

  return content_length;
}

struct
{
  struct arg_int *timeout;
  struct arg_end *end;
} wscan_args;

int wifi_scan(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&wscan_args);
  if (nerrors != 0)
  {
    arg_print_errors(stderr, wscan_args.end, argv[0]);
    return 1;
  }

  int timeout = 300;

  if (argc > 1)
    timeout = wscan_args.timeout->ival[0];

  printf("Max AP number ap_info can hold = %u\r\n", DEFAULT_SCAN_LIST_SIZE);
  printf("Timeout = %u\r\n", timeout);

  wf_scan(timeout);
  auto ap_count = wf_get_ap_num();

  printf("Total APs scanned = %u\r\n\r\n", ap_count);
  printf("Index\tAuth mode\tPairwise cypher\tGroup cypher\tRSSI\tChannel\tSSID\r\n");

  for (int i = 0; i < ap_count; i++)
  {
    printf("%u\t", i);
    print_auth_mode(ap_info[i].authmode);

    if (ap_info[i].authmode != WIFI_AUTH_WEP)
    {
      printf("\t");
      print_cipher_type(ap_info[i].pairwise_cipher);
      printf("\t");
      print_cipher_type(ap_info[i].group_cipher);
    }
    else
      printf("\t\t\t\t");

    printf("\t%d\t%d\t%s\r\n", ap_info[i].rssi, ap_info[i].primary, ap_info[i].ssid);
  }

  return 0;
}

struct
{
  struct arg_int *timeout;
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_end *end;
} connect_args;

int connect_ap(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&connect_args);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, connect_args.end, argv[0]);
    return 1;
  }

  ESP_LOGI(__func__, "Connecting to '%s'", connect_args.ssid->sval[0]);

  /* set default value*/
  if (connect_args.timeout->count == 0)
    connect_args.timeout->ival[0] = CONNECT_TIMEOUT_MS;

  bool connected = wifi_connect(connect_args.ssid->sval[0], connect_args.password->sval[0], connect_args.timeout->ival[0]);

  if (!connected)
  {
    ESP_LOGW(__func__, "Connection timed out");
    return 1;
  }

  ESP_LOGI(__func__, "Connected");
  return 0;
}

struct
{
  struct arg_str *url;
  struct arg_end *end;
} http_get_args;

int http_get(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&http_get_args);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, http_get_args.end, argv[0]);
    return 1;
  }

  auto url = http_get_args.url->sval[0];
  ESP_LOGI(__func__, "Downloading '%s'", url);
  int l = strlen(url) + 1;
  int h = make_obj(l, OBJ_TYPE_DATA);
  write_obj(h, url, l);
  wr_reg8(ESP_REG_OBJ_HANDLE, h);
  http_do_get();
  delete_obj(h);
  delete_obj(rd_reg8(ESP_REG_OBJ_HANDLE));
  ESP_LOGI(__func__, "Downloading finished");

  return 0;
}

int https_get(int argc, char **argv)
{
  // #define WEB_SERVER "prods.tslabs.info"
  // #define WEB_URL "https://prods.tslabs.info/files/StreetFighter2_1.1.zip"
  // #define WEB_SERVER "releases.ubuntu.com"
  // #define WEB_URL "https://releases.ubuntu.com/24.04.2/ubuntu-24.04.2-desktop-amd64.iso"
  #define WEB_SERVER "downloads.raspberrypi.com"
  #define WEB_URL "https://downloads.raspberrypi.com/raspios_armhf/images/raspios_armhf-2024-11-19/2024-11-19-raspios-bookworm-armhf.img.xz"
  #define WEB_PORT "443"

  static const char REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
                                "Host: " WEB_SERVER "\r\n"
                                "User-Agent: esp-idf/1.0 esp32\r\n"
                                "\r\n";

  int nerrors = arg_parse(argc, argv, (void **)&http_get_args);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, http_get_args.end, argv[0]);
    return 1;
  }

  size_t written_bytes = 0;
  bool is_resp = false;
  size_t content_index = 0;
  size_t content_length = 0;

  int ret;
  void *buf;

  // ESP_LOGI(__func__, "Downloading '%s'", http_get_args.url->sval[0]);
  ESP_LOGI(__func__, "Downloading '%s'", WEB_URL);

  esp_tls_cfg_t cfg =
  {
    .crt_bundle_attach = esp_crt_bundle_attach,
  };

  ESP_LOGI(TAG, "URL: %s", WEB_URL);

  esp_tls_t *tls = esp_tls_init();

  if (!tls)
  {
    ESP_LOGE(TAG, "Failed to allocate esp_tls handle!");
    goto exit;
  }

  if (esp_tls_conn_http_new_sync(WEB_URL, &cfg, tls) == 1)
  {
    ESP_LOGI(TAG, "Connection established");
  }
  else
  {
    ESP_LOGE(TAG, "Connection failed");
    int esp_tls_code = 0, esp_tls_flags = 0;
    esp_tls_error_handle_t tls_e = NULL;
    esp_tls_get_error_handle(tls, &tls_e);

    /* Try to get TLS stack level error and certificate failure flags, if any */
    ret = esp_tls_get_and_clear_last_error(tls_e, &esp_tls_code, &esp_tls_flags);

    if (ret == ESP_OK)
    {
      ESP_LOGE(TAG, "TLS error = -0x%x, TLS flags = -0x%x", esp_tls_code, esp_tls_flags);
    }

    goto cleanup;
  }

  ESP_LOGI(TAG, "Writing HTTP request");

  while (written_bytes < strlen(REQUEST))
  {
    ret = esp_tls_conn_write(tls, REQUEST + written_bytes, strlen(REQUEST) - written_bytes);

    if (ret >= 0)
    {
      ESP_LOGI(TAG, "%d bytes written", ret);
      written_bytes += ret;
    }

    else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE)
    {
      ESP_LOGE(TAG, "esp_tls_conn_write returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
      goto cleanup;
    }
  };

  ESP_LOGI(TAG, "Reading HTTP response");

  buf = malloc_spiram(16384);
  
  do
  {
    ret = esp_tls_conn_read(tls, (char*)buf, sizeof(buf));

    if (ret == 0)
    {
      ESP_LOGI(TAG, "Connection closed");
      break;
    }

    if (ret < 0)
      ESP_LOGE(TAG, "esp_tls_conn_read returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));

    if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ)
      continue;

    ESP_LOGD(TAG, "Received %u bytes", ret);

    if (!is_resp)
    {
      content_length = parse_response((char*)buf, ret, content_index);

      if (content_length <= 0)
        break;

      content_length += content_index;
      is_resp = true;
    }

    content_length -= ret;
    ESP_LOGD(TAG, "Left %u bytes", content_length);
  } while (content_length);
  
  if (buf) free(buf);

cleanup:
    esp_tls_conn_destroy(tls);

exit:

  return 0;
}

void esp_console_register_wifi_commands()
{
  {
    wscan_args.timeout  = arg_int0(NULL, NULL, "<timeout>", "Timeout (0-1000)");
    wscan_args.end = arg_end(1);

    const esp_console_cmd_t wscan_cmd =
    {
      .command = "wscan",
      .help    = "Scan WiFi APs",
      .hint    = NULL,
      .func    = &wifi_scan,
      .argtable = &wscan_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&wscan_cmd));
  }

  {
    connect_args.timeout  = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    connect_args.ssid     = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    connect_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    connect_args.end      = arg_end(2);

    const esp_console_cmd_t connect_cmd =
    {
      .command  = "connect_ap",
      .help     = "Connect to a WiFi AP as a station",
      .hint     = NULL,
      .func     = &connect_ap,
      .argtable = &connect_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&connect_cmd));
  }

  {
    http_get_args.url     = arg_str1(NULL, NULL, "<url>", "URL");
    http_get_args.end     = arg_end(1 /* num of errors to print */);

    const esp_console_cmd_t http_get_cmd =
    {
      .command  = "http_get",
      .help     = "Download an HTTP URL",
      .hint     = NULL,
      .func     = &http_get,
      .argtable = &http_get_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&http_get_cmd));
  }

  {
    http_get_args.url     = arg_str1(NULL, NULL, "<url>", "URL");
    http_get_args.end     = arg_end(1 /* num of errors to print */);

    const esp_console_cmd_t https_get_cmd =
    {
      .command  = "https_get",
      .help     = "Download an HTTPS URL",
      .hint     = NULL,
      .func     = &https_get,
      .argtable = &http_get_args
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&https_get_cmd));
  }
}
