
#include <avr/pgmspace.h>
#include <util/twi.h>
#include "mytypes.h"
#include "i2c.h"
#include "oled.h"
#include "font.h"

u8 oled_addr;

void oled_init()
{
  // Detect address
  oled_addr = OLED_ADDRESS_0;

  if (!oled_i2c_start())
  {
    tw_send_stop();
    oled_addr = OLED_ADDRESS_1;

    if (!oled_i2c_start())
    {
      tw_send_stop();
      oled_addr = 0;
      return;
    }
  }

  tw_send_stop();

	// Init display module
	oled_write_cmd(0xAE);       // Display off
  oled_write_cmd(0xA8, 0x3F); // Set multiplex ratio(1 to 64)
	oled_write_cmd(0xD3, 0x00); // Set display offset
	oled_write_cmd(0x40);       // Set start line address
	oled_write_cmd(0xA0);       // Set segment re-map off
	oled_write_cmd(0xC0);       // Set COM Output Scan Direction (0xC0/C8)
	oled_write_cmd(0xDA, 0x02); // Set COM pins hardware configuration (normal)
	oled_write_cmd(0x81, 0xFF); // Set contrast control register
	oled_write_cmd(0xA4);       // 0xA4 - Output follows RAM content; 0xA5 - Output ignores RAM content
	oled_write_cmd(0xA6);       // Set normal display
	oled_write_cmd(0xD5, 0xF0); // Set oscillator frequency/divider
	oled_write_cmd(0x8D, 0x14); // Set DC-DC enable
	oled_write_cmd(0xD9, 0x22); // Set pre-charge period
	oled_write_cmd(0xDB, 0x20); // Set Vcomh: 0x20 - 0.77xVcc
	oled_write_cmd(0x20, 0x00); // Set Memory Addressing Mode: 0 - Horizontal Addressing Mode; 1 - Vertical Addressing Mode; 2 - Page Addressing Mode (RESET); 3 - Invalid
	oled_write_cmd(0xAF);       // Display on
}

void oled_clear()
{
  if (!oled_addr) return;

  oled_write_cmd(0x00);       // Set low column address
  oled_write_cmd(0x10);       // Set high column address
  oled_write_cmd(0xB0);       // Set page address

  if (!oled_i2c_start()) goto stop;
  if (!(tw_send_data(0x40) == TW_MT_DATA_ACK)) goto stop;

  for (int i = 0; i < 1024; i++)
    if (!(tw_send_data(0) == TW_MT_DATA_ACK)) goto stop;

stop:
  tw_send_stop();
}

void oled_set_xy(u8 x, u8 y)
{
  if (!oled_addr) return;

  x = (x & 15) << 3;
  oled_write_cmd(0x00 | (x & 15));            // Set low column address
  oled_write_cmd(0x10 | ((x >> 4) & 15));    // Set high column address
  oled_write_cmd(0xB0 | (y & 15));            // Set page address
}

void oled_print(const char *txt)
{
  u8 c;

  if (!oled_addr) return;
  if (!oled_i2c_start()) goto stop;
  if (!(tw_send_data(0x40) == TW_MT_DATA_ACK)) goto stop;

  while (c = *txt++)
  {
    c -= 32;
    u32 p = (u32)&oled_font[c << 3];

    for (int i = 1; i < 8; i++) // only 7 columns for default font
      if (!(tw_send_data(pgm_read_byte_far(p++)) == TW_MT_DATA_ACK)) goto stop;
  }

stop:
  tw_send_stop();
}

void oled_write_cmd(u8 cmd, u8 par1)
{
  // Command sequence: S AW 00 cc dd P

  if (!oled_addr) return;
  if (!oled_i2c_start()) goto stop;
  if (!(tw_send_data(0x00) == TW_MT_DATA_ACK)) goto stop;
  if (!(tw_send_data(cmd) == TW_MT_DATA_ACK)) goto stop;
  tw_send_data(par1);

stop:
  tw_send_stop();
}

void oled_write_cmd(u8 cmd)
{
  // Command sequence: S AW 00 cc P

  if (!oled_addr) return;
  if (!oled_i2c_start()) goto stop;
  if (!(tw_send_data(0x00) == TW_MT_DATA_ACK)) goto stop;
  tw_send_data(cmd);

stop:
  tw_send_stop();
}

void oled_write_data(u8 data)
{
  oled_write_data(&data, 1);
}

void oled_write_data(u8 *data, u8 len)
{
  // Data sequence: S AW 40 dd .. P

  if (!oled_addr) return;
  if (!oled_i2c_start()) goto stop;
  if (!(tw_send_data(0x40) == TW_MT_DATA_ACK)) goto stop;

  while(len--)
    if (!(tw_send_data(*data++) == TW_MT_DATA_ACK)) goto stop;

stop:
  tw_send_stop();
}

u8 oled_i2c_start()
{
  if (!(tw_send_start() & (TW_START | TW_REP_START))) return 0;
  if (!(tw_send_addr(oled_addr | TW_WRITE) == TW_MT_SLA_ACK)) return 0;

  return 1;
}
