#pragma once
// A fake HTTP client: the headers and the body of the current test case
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
typedef enum { HTTP_EVENT_ON_HEADER = 5 } esp_http_client_event_id_t;
typedef struct { esp_http_client_event_id_t event_id; char *header_key; char *header_value; } esp_http_client_event_t;
typedef esp_err_t (*http_event_handle_cb)(esp_http_client_event_t *);
typedef struct {
  const char *url; int timeout_ms; int buffer_size; int buffer_size_tx; const char *user_agent;
  http_event_handle_cb event_handler; esp_err_t (*crt_bundle_attach)(void *);
} esp_http_client_config_t;
typedef struct fake_client *esp_http_client_handle_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *);
esp_err_t esp_http_client_set_header(esp_http_client_handle_t, const char *, const char *);
esp_err_t esp_http_client_open(esp_http_client_handle_t, int);
long long esp_http_client_fetch_headers(esp_http_client_handle_t);
int esp_http_client_get_status_code(esp_http_client_handle_t);
esp_err_t esp_http_client_set_redirection(esp_http_client_handle_t);
int esp_http_client_read(esp_http_client_handle_t, char *, int);
esp_err_t esp_http_client_close(esp_http_client_handle_t);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t);
