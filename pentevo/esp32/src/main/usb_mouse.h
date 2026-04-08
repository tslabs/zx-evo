
#pragma once

#include "esp_err.h"

extern "C" esp_err_t usb_mouse_start();
int usb_cmd(int argc, char **argv);
