
#pragma once

#include "esp_err.h"

extern "C" esp_err_t usb_mouse_start();
int usbmouse_cmd(int argc, char **argv);
