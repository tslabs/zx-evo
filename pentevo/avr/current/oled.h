
#pragma once

#define OLED_ADDRESS_0 0x78
#define OLED_ADDRESS_1 0x7A

/* to do: define SSD1306 registers */

void oled_init();
u8 oled_start();
void oled_write_cmd(u8 cmd);
void oled_write_data(u8 data);
void oled_write_data(u8 *data, u8 len);
