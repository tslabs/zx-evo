
#pragma once

#include "esp_err.h"

esp_err_t ps2_mouse_start();
esp_err_t ps2_mouse_send_movement(int dx, int dy, unsigned buttons);
bool ps2_mouse_is_active();
