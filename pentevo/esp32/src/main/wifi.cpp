#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
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
#include "nvs_params.h"

const char TAG[] = "wifi.cpp";

EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

EXT_RAM_BSS_ATTR wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
esp_netif_ip_info_t ip;
esp_netif_t *wifi_sta_netif = NULL;
bool wifi_initialized = false;
bool wifi_started = false;
TaskHandle_t wifi_autoconn_task = NULL;

bool wifi_is_enabled()
{
  return app_params.wifi_mode != 0;
}

bool wifi_has_saved_ap()
{
  return app_params.wifi_ap[0] != 0;
}

bool wifi_should_autoconnect()
{
  return wifi_is_enabled() && wifi_has_saved_ap();
}

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_base == WIFI_EVENT)
  {
    if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
      ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");

      bool autoconnect = wifi_started && wifi_should_autoconnect();
      xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

      if (autoconnect)
        esp_wifi_connect();
    }

    else if (event_id == WIFI_EVENT_WIFI_READY)
      ESP_LOGI(TAG, "WIFI_EVENT_WIFI_READY");

    else if (event_id == WIFI_EVENT_SCAN_DONE)
      ESP_LOGI(TAG, "WIFI_EVENT_SCAN_DONE");

    else if (event_id == WIFI_EVENT_STA_START)
      ESP_LOGI(TAG, "WIFI_EVENT_STA_START");

    else if (event_id == WIFI_EVENT_STA_STOP)
      ESP_LOGI(TAG, "WIFI_EVENT_STA_STOP");

    else if (event_id == WIFI_EVENT_STA_CONNECTED)
      ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");

    else if (event_id == WIFI_EVENT_STA_BSS_RSSI_LOW)
      ESP_LOGI(TAG, "WIFI_EVENT_STA_BSS_RSSI_LOW");
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

void get_ip(uint8_t *i, uint8_t *m, uint8_t *g)
{
  i[0] = ((uint8_t*)&ip.ip)[0];
  i[1] = ((uint8_t*)&ip.ip)[1];
  i[2] = ((uint8_t*)&ip.ip)[2];
  i[3] = ((uint8_t*)&ip.ip)[3];
  m[0] = ((uint8_t*)&ip.netmask)[0];
  m[1] = ((uint8_t*)&ip.netmask)[1];
  m[2] = ((uint8_t*)&ip.netmask)[2];
  m[3] = ((uint8_t*)&ip.netmask)[3];
  g[0] = ((uint8_t*)&ip.gw)[0];
  g[1] = ((uint8_t*)&ip.gw)[1];
  g[2] = ((uint8_t*)&ip.gw)[2];
  g[3] = ((uint8_t*)&ip.gw)[3];
};

bool wifi_is_connected()
{
  if (!wifi_event_group) return false;
  return (xEventGroupGetBits(wifi_event_group) & CONNECTED_BIT) != 0;
}

void wifi_autoconnect_task(void *arg)
{
  bool connected = false;

  if (!wifi_is_enabled())
    ESP_LOGI(TAG, "WiFi disabled by config");
  else if (!wifi_has_saved_ap())
    ESP_LOGI(TAG, "WiFi enabled, AP is not configured");
  else
  {
    connected = wifi_connect(app_params.wifi_ap, app_params.wifi_psw, CONNECT_TIMEOUT_MS);
    ESP_LOGI(TAG, "WiFi auto connect: %s", connected ? "OK" : "FAIL");
  }

  wifi_autoconn_task = NULL;
  vTaskDelete(NULL);
}

void wifi_start_autoconnect()
{
  initialize_wifi();
  if (wifi_autoconn_task) return;
  xTaskCreatePinnedToCore(wifi_autoconnect_task, "wifi", 4096, NULL, 21, &wifi_autoconn_task, 0);
}

void initialize_wifi()
{
  esp_log_level_set("wifi", ESP_LOG_WARN);

  if (!wifi_initialized)
  {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_sta_netif = esp_netif_create_default_wifi_sta();
    assert(wifi_sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_initialized = true;
  }

  if (wifi_started) return;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  if (wifi_event_group) xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  wifi_started = true;
}

void wifi_disconnect_now()
{
  if (!wifi_initialized || !wifi_started) return;

  xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  esp_wifi_disconnect();
  esp_wifi_stop();
  memset(&ip, 0, sizeof(ip));
  wifi_started = false;
}

bool wifi_connect(const char *ssid, const char *pass, int timeout_ms)
{
  initialize_wifi();

  wifi_config_t wifi_config = { 0 };
  strlcpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (pass) strlcpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

  xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_wifi_connect();

  int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);

  return (bits & CONNECTED_BIT) != 0;
}

bool wifi_connect_saved(int timeout_ms)
{
  if (!wifi_should_autoconnect()) return false;
  return wifi_connect(app_params.wifi_ap, app_params.wifi_psw, timeout_ms);
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

void wf_get_ap(int idx, uint8_t &auth, int8_t &rssi, uint8_t &chan, uint8_t *&ssid)
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
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_end *end;
} set_ap_args;

int set_ap(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&set_ap_args);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, set_ap_args.end, argv[0]);
    return 1;
  }

  strlcpy(app_params.wifi_ap, set_ap_args.ssid->sval[0], sizeof(app_params.wifi_ap));

  if (set_ap_args.password->count)
    strlcpy(app_params.wifi_psw, set_ap_args.password->sval[0], sizeof(app_params.wifi_psw));
  else
    app_params.wifi_psw[0] = 0;

  esp_err_t err = app_params_save();
  if (err != ESP_OK)
  {
    printf("app_params_save failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("Saved AP: %s\r\n", app_params.wifi_ap);

  if (wifi_is_enabled())
  {
    initialize_wifi();
    wifi_start_autoconnect();
  }

  return 0;
}

int wifi_enable_cmd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Usage: wf en <0|1>\r\n");
    return 1;
  }

  char *endp = NULL;
  unsigned long en = strtoul(argv[2], &endp, 0);
  if (!endp || *endp || en > 1)
  {
    printf("Bad <0|1>: %s\r\n", argv[2]);
    return 1;
  }

  app_params.wifi_mode = (uint8_t)en;

  esp_err_t err = app_params_save();
  if (err != ESP_OK)
  {
    printf("app_params_save failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("WiFi enable: %u\r\n", app_params.wifi_mode);

  if (app_params.wifi_mode)
  {
    initialize_wifi();
    wifi_start_autoconnect();
  }
  else
    wifi_disconnect_now();

  return 0;
}

int wf_info(int argc, char **argv)
{
  esp_err_t err = app_params_load();
  if (err != ESP_OK)
  {
    printf("app_params_load failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  wifi_mode_t mode = WIFI_MODE_NULL;
  uint8_t mac[6] = {0};
  wifi_config_t cfg = {};
  wifi_ap_record_t ap = {};
  const char *hostname = NULL;
  bool connected = wifi_is_connected();
  bool have_cfg = false;
  bool have_ap = false;

  if (wifi_initialized)
  {
    if (esp_wifi_get_mode(&mode) != ESP_OK) mode = WIFI_MODE_NULL;
    if (esp_wifi_get_config(WIFI_IF_STA, &cfg) == ESP_OK) have_cfg = true;
    esp_wifi_get_mac(WIFI_IF_STA, mac);
  }

  if (connected && esp_wifi_sta_get_ap_info(&ap) == ESP_OK) have_ap = true;

  if (wifi_sta_netif) esp_netif_get_hostname(wifi_sta_netif, &hostname);

  printf("WiFi:\r\n");
  printf("  enabled     : %u\r\n", app_params.wifi_mode);
  printf("  initialized : %u\r\n", wifi_initialized ? 1 : 0);
  printf("  started     : %u\r\n", wifi_started ? 1 : 0);
  printf("  connected   : %u\r\n", connected ? 1 : 0);
  printf("  saved_ap    : %s\r\n", app_params.wifi_ap[0] ? app_params.wifi_ap : "");
  printf("  pass_set    : %u\r\n", app_params.wifi_psw[0] ? 1 : 0);
  printf("  hostname    : %s\r\n", hostname ? hostname : "");
  printf("  mac         : %02X:%02X:%02X:%02X:%02X:%02X\r\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  printf("  mode        : ");
  switch (mode)
  {
    case WIFI_MODE_NULL: printf("NULL\r\n"); break;
    case WIFI_MODE_STA:  printf("STA\r\n"); break;
    case WIFI_MODE_AP:   printf("AP\r\n"); break;
    case WIFI_MODE_APSTA:printf("APSTA\r\n"); break;
    default:             printf("%d\r\n", (int)mode); break;
  }

  if (have_cfg)
    printf("  cfg_ssid    : %s\r\n", cfg.sta.ssid[0] ? (char*)cfg.sta.ssid : "");
  else
    printf("  cfg_ssid    : \r\n");

  printf("  ip          : " IPSTR "\r\n", IP2STR(&ip.ip));
  printf("  netmask     : " IPSTR "\r\n", IP2STR(&ip.netmask));
  printf("  gateway     : " IPSTR "\r\n", IP2STR(&ip.gw));

  if (have_ap)
  {
    printf("  ap_ssid     : %s\r\n", ap.ssid);
    printf("  bssid       : %02X:%02X:%02X:%02X:%02X:%02X\r\n",
      ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);
    printf("  channel     : %u\r\n", ap.primary);
    printf("  rssi        : %d\r\n", ap.rssi);
    printf("  auth        : ");
    print_auth_mode(ap.authmode);
    printf("\r\n");
    printf("  pairwise    : ");
    print_cipher_type(ap.pairwise_cipher);
    printf("\r\n");
    printf("  group       : ");
    print_cipher_type(ap.group_cipher);
    printf("\r\n");
  }
  else
  {
    printf("  ap_ssid     : \r\n");
    printf("  bssid       : \r\n");
    printf("  channel     : 0\r\n");
    printf("  rssi        : 0\r\n");
    printf("  auth        : \r\n");
    printf("  pairwise    : \r\n");
    printf("  group       : \r\n");
  }

  return 0;
}

int wf_cmd(int argc, char **argv)
{
  if (argc < 2)
  {
    printf("Usage:\r\n");
    printf("  wf conn\r\n");
    printf("  wf conn <ssid> [pass] [timeout_ms]\r\n");
    printf("  wf ap <ssid> [pass]\r\n");
    printf("  wf en <0|1>\r\n");
    printf("  wf dis\r\n");
    printf("  wf scan [timeout]\r\n");
    printf("  wf info\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "scan"))
  {
    int timeout = 300;
    char *scan_argv[2];
    char timeout_buf[16];

    if (argc >= 3)
    {
      char *endp = NULL;
      unsigned long v = strtoul(argv[2], &endp, 0);
      if (!endp || *endp || v > 1000)
      {
        printf("Bad <timeout>: %s\r\n", argv[2]);
        return 1;
      }
      timeout = (int)v;
    }

    scan_argv[0] = argv[0];

    if (argc >= 3)
    {
      snprintf(timeout_buf, sizeof(timeout_buf), "%d", timeout);
      scan_argv[1] = timeout_buf;
      return wifi_scan(2, scan_argv);
    }

    return wifi_scan(1, scan_argv);
  }

  if (!strcmp(argv[1], "ap"))
  {
    if (argc < 3)
    {
      printf("Usage: wf ap <ssid> [pass]\r\n");
      return 1;
    }

    char *ap_argv[3];
    ap_argv[0] = argv[0];
    ap_argv[1] = argv[2];
    ap_argv[2] = argc >= 4 ? argv[3] : NULL;
    return set_ap(argc - 1, ap_argv);
  }

  if (!strcmp(argv[1], "en"))
    return wifi_enable_cmd(argc, argv);

  if (!strcmp(argv[1], "info"))
    return wf_info(argc, argv);

  if (!strcmp(argv[1], "dis"))
  {
    wifi_disconnect_now();
    printf("WiFi stopped\r\n");
    return 0;
  }

  if (!strcmp(argv[1], "conn"))
  {
    if (argc == 2)
    {
      if (!wifi_has_saved_ap())
      {
        printf("Saved AP is not configured\r\n");
        return 1;
      }

      bool connected = wifi_connect_saved(CONNECT_TIMEOUT_MS);
      if (!connected)
      {
        printf("Connection timed out\r\n");
        return 1;
      }

      printf("Connected\r\n");
      return 0;
    }

    if (argc < 3 || argc > 5)
    {
      printf("Usage: wf conn [<ssid> [<pass> [timeout_ms]]]\r\n");
      return 1;
    }

    const char *ssid = argv[2];
    const char *pass = argc >= 4 ? argv[3] : NULL;
    int timeout_ms = CONNECT_TIMEOUT_MS;

    if (argc >= 5)
    {
      char *endp = NULL;
      unsigned long v = strtoul(argv[4], &endp, 0);
      if (!endp || *endp)
      {
        printf("Bad <timeout_ms>: %s\r\n", argv[4]);
        return 1;
      }
      timeout_ms = (int)v;
    }

    bool connected = wifi_connect(ssid, pass, timeout_ms);
    if (!connected)
    {
      printf("Connection timed out\r\n");
      return 1;
    }

    printf("Connected\r\n");
    return 0;
  }

  printf("Unknown subcommand: %s\r\n", argv[1]);
  printf("Usage:\r\n");
  printf("  wf conn\r\n");
  printf("  wf conn <ssid> [pass] [timeout_ms]\r\n");
  printf("  wf ap <ssid> [pass]\r\n");
  printf("  wf en <0|1>\r\n");
  printf("  wf dis\r\n");
  printf("  wf scan [timeout]\r\n");
  printf("  wf info\r\n");
  return 1;
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
    wscan_args.timeout    = arg_int0(NULL, NULL, "<timeout>", "Timeout (0-1000)");
    wscan_args.end        = arg_end(1);
    connect_args.timeout  = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
    connect_args.ssid     = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    connect_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    connect_args.end      = arg_end(2);
    set_ap_args.ssid      = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
    set_ap_args.password  = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
    set_ap_args.end       = arg_end(2);

    const esp_console_cmd_t wf_cmd_desc =
    {
      .command  = "wf",
      .help     = "WiFi commands: conn/ap/en/dis/scan/info",
      .hint     = NULL,
      .func     = &wf_cmd,
      .argtable = NULL
    };

    ESP_ERROR_CHECK(esp_console_cmd_register(&wf_cmd_desc));
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
