
#pragma once

#include "esp_err.h"

int ps2_mouse_cmd(int argc, char **argv);
esp_err_t ps2_mouse_send_movement(int dx, int dy, unsigned buttons);
bool ps2_mouse_is_active();
