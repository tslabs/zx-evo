
#pragma once

#define OLED_ADDRESS_0 0x78
#define OLED_ADDRESS_1 0x7A

/* to do: define SSD1306 registers */

void oled_init();
void oled_clear();
void oled_set_xy(u8 x, u8 y);
void oled_print(const char *txt);
void oled_write_cmd(u8 cmd);
void oled_write_cmd(u8 cmd, u8 par1);
void oled_write_data(u8 data);
void oled_write_data(u8 *data, u8 len);
u8 oled_i2c_start();
