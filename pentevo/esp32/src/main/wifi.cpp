
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

#define CONNECT_TIMEOUT_MS        (10000)
#define DEFAULT_SCAN_LIST_SIZE    32

wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];

EventGroupHandle_t wifi_event_group;
const int          CONNECTED_BIT = BIT0;

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
}

void initialize_wifi()
{
  esp_log_level_set("wifi", ESP_LOG_WARN);
  static bool initialized = false;

  if (initialized)
    return;

  ESP_ERROR_CHECK(esp_netif_init());
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t        *ap_netif = esp_netif_create_default_wifi_ap();
  assert(ap_netif);
  esp_netif_t        *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  initialized = true;
}

bool wifi_connect(const char *ssid, const char *pass, int timeout_ms)
{
  initialize_wifi();
  wifi_config_t wifi_config = { 0 };
  strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
  if (pass)
  {
    strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  esp_wifi_connect();

  int bits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                                 pdFALSE, pdTRUE, timeout_ms / portTICK_PERIOD_MS);
  return (bits & CONNECTED_BIT) != 0;
}

/** Arguments used by 'connect' function */
struct
{
  struct arg_int *timeout;
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_end *end;
} connect_args;

int connect(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **)&connect_args);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, connect_args.end, argv[0]);
    return 1;
  }
  ESP_LOGI(__func__, "Connecting to '%s'",
           connect_args.ssid->sval[0]);

  /* set default value*/
  if (connect_args.timeout->count == 0)
  {
    connect_args.timeout->ival[0] = CONNECT_TIMEOUT_MS;
  }

  bool connected = wifi_connect(connect_args.ssid->sval[0],
                                connect_args.password->sval[0],
                                connect_args.timeout->ival[0]);
  if (!connected)
  {
    ESP_LOGW(__func__, "Connection timed out");
    return 1;
  }

  ESP_LOGI(__func__, "Connected");
  return 0;
}

static void print_auth_mode(int authmode)
{
  printf("Authmode \t\t");

  switch (authmode)
  {
    case WIFI_AUTH_OPEN:          printf("OPEN"); break;
    case WIFI_AUTH_OWE:           printf("OWE"); break;
    case WIFI_AUTH_WEP:           printf("WEP"); break;
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

static void print_cipher_type(int pairwise_cipher, int group_cipher)
{
  printf("Pairwise Cipher \t");

  switch (pairwise_cipher)
  {
    case WIFI_CIPHER_TYPE_NONE:         printf("NONE"); break;
    case WIFI_CIPHER_TYPE_WEP40:        printf("WEP40"); break;
    case WIFI_CIPHER_TYPE_WEP104:       printf("WEP104"); break;
    case WIFI_CIPHER_TYPE_TKIP:         printf("TKIP"); break;
    case WIFI_CIPHER_TYPE_CCMP:         printf("CCMP"); break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:    printf("TKIP_CCMP"); break;
    case WIFI_CIPHER_TYPE_AES_CMAC128:  printf("AES_CMAC128"); break;
    case WIFI_CIPHER_TYPE_SMS4:         printf("SMS4"); break;
    case WIFI_CIPHER_TYPE_GCMP:         printf("GCMP"); break;
    case WIFI_CIPHER_TYPE_GCMP256:      printf("GCMP256"); break;
    default:                            printf("UNKNOWN"); break;
  }

  printf("Group Cipher \t");

  switch (group_cipher)
  {
    case WIFI_CIPHER_TYPE_NONE:       printf("NONE"); break;
    case WIFI_CIPHER_TYPE_WEP40:      printf("WEP40"); break;
    case WIFI_CIPHER_TYPE_WEP104:     printf("WEP104"); break;
    case WIFI_CIPHER_TYPE_TKIP:       printf("TKIP"); break;
    case WIFI_CIPHER_TYPE_CCMP:       printf("CCMP"); break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:  printf("TKIP_CCMP"); break;
    case WIFI_CIPHER_TYPE_SMS4:       printf("SMS4"); break;
    case WIFI_CIPHER_TYPE_GCMP:       printf("GCMP"); break;
    case WIFI_CIPHER_TYPE_GCMP256:    printf("GCMP256"); break;
    default:                          printf("UNKNOWN"); break;
  }
}

/* Initialize Wi-Fi as sta and set scan method */
int wifi_scan(int argc, char **argv)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  uint16_t number = DEFAULT_SCAN_LIST_SIZE;
  wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
  uint16_t         ap_count = 0;
  memset(ap_info, 0, sizeof(ap_info));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());
  esp_wifi_scan_start(NULL, true);
  printf("Max AP number ap_info can hold = %u", number);
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
  printf("Total APs scanned = %u, actual AP number ap_info holds = %u\n", ap_count, number);
  
  for (int i = 0; i < number; i++) {
  printf("SSID \t\t%s", ap_info[i].ssid);
  printf("RSSI \t\t%d", ap_info[i].rssi);
  print_auth_mode(ap_info[i].authmode);
  if (ap_info[i].authmode != WIFI_AUTH_WEP)
    print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
  printf("Channel \t\t%d\n", ap_info[i].primary);
  }

  return 0;
}

void esp_console_register_wifi_commands()
{
  connect_args.timeout  = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
  connect_args.ssid     = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
  connect_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
  connect_args.end      = arg_end(2);

  const esp_console_cmd_t connect_cmd =
  {
    .command  = "connect",
    .help     = "Connect to a WiFi AP as a station",
    .hint     = NULL,
    .func     = &connect,
    .argtable = &connect_args
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&connect_cmd));

  const esp_console_cmd_t wscan_cmd =
  {
    .command = "wscan",
    .help    = "Scan WiFi APs",
    .hint    = NULL,
    .func    = &wifi_scan,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&wscan_cmd));
}
