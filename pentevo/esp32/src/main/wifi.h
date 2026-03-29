
#pragma once

#define CONNECT_TIMEOUT_MS        (10000)
#define DEFAULT_SCAN_LIST_SIZE    64

void initialize_wifi();
int wf_scan(int timeout);
uint16_t wf_get_ap_num();
void wf_get_ap(int idx, u8 &auth, i8 &rssi, u8 &chan, u8 *&ssid);
void get_ip(u8 *i, u8 *m, u8 *g);
bool wifi_connect(const char *ssid, const char *pass, int timeout_ms);

void esp_console_register_wifi_commands();
