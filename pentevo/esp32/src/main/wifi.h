
#pragma once

#include <stdint.h>
#include <stdbool.h>

#define CONNECT_TIMEOUT_MS        (10000)
#define DEFAULT_SCAN_LIST_SIZE    64

void initialize_wifi();
int wf_scan(int timeout);
uint16_t wf_get_ap_num();
void wf_get_ap(int idx, uint8_t &auth, int8_t &rssi, uint8_t &chan, uint8_t *&ssid);
void get_ip(uint8_t *i, uint8_t *m, uint8_t *g);
bool wifi_connect(const char *ssid, const char *pass, int timeout_ms);
bool wifi_connect_saved(int timeout_ms);
bool wifi_is_enabled();
bool wifi_has_saved_ap();
void wifi_start_autoconnect();
void wifi_disconnect_now();

void esp_console_register_wifi_commands();
