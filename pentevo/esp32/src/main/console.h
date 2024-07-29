
#pragma once

void initialize_console();
void console_task(void *arg);
void initialize_nvs();
void esp_console_register_system_commands();
void esp_console_register_wifi_commands();
