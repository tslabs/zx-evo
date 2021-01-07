
#include <avr/pgmspace.h>
#include <util/twi.h>
#include "mytypes.h"
#include "i2c.h"
#include "oled.h"

// pgm_read_byte_far();

u8 oled_addr;

void oled_init()
{
  oled_addr = OLED_ADDRESS_0;

  if (!oled_start())
  {
    tw_send_stop();
    oled_addr = OLED_ADDRESS_1;

    if (!oled_start())
    {
      tw_send_stop();
      oled_addr = 0;
      return;
    }
  }

  tw_send_stop();

	oled_write_cmd(0xAE); // Display off
	oled_write_cmd(0x20); // Set Memory Addressing Mode
	oled_write_cmd(0x10); // 00 - Horizontal Addressing Mode; 01 - Vertical Addressing Mode; 10 - Page Addressing Mode (RESET); 11 - Invalid
	oled_write_cmd(0xC8); // Set COM Output Scan Direction
	oled_write_cmd(0x00); // Set low column address
	oled_write_cmd(0x10); // Set high column address
	oled_write_cmd(0x40); // Set start line address
	oled_write_cmd(0x81); // Set contrast control register
	oled_write_cmd(0xFF);
	oled_write_cmd(0xA1); // Set segment re-map 0 to 127
	oled_write_cmd(0xA6); // Set normal display
	oled_write_cmd(0xA8); // Set multiplex ratio(1 to 64)
	oled_write_cmd(0x3F); //
	oled_write_cmd(0xA4); // 0xA4 - Output follows RAM content; 0xA5 - Output ignores RAM content
	oled_write_cmd(0xD3); // Set display offset
	oled_write_cmd(0x00); // Not offset
	oled_write_cmd(0xD5); // Set display clock divide ratio/oscillator frequency
	oled_write_cmd(0xF0); // Set divide ratio
	oled_write_cmd(0xD9); // Set pre-charge period
	oled_write_cmd(0x22); //
	oled_write_cmd(0xDA); // Set com pins hardware configuration
	oled_write_cmd(0x12);
	oled_write_cmd(0xDB); // Set Vcomh
	oled_write_cmd(0x20); // 0x20 - 0.77xVcc
	oled_write_cmd(0x8D); // Set DC-DC enable
	oled_write_cmd(0x14); //
	oled_write_cmd(0xAF); // Display on


  for (int j = 0; j < 8; j++)
  {
    oled_write_cmd(0xB0 + j); // Set Page Start Address for Page Addressing Mode, 0-7

    for (int i = 0; i < 128; i++)
      oled_write_data(0);
  }

  oled_write_cmd(0xB4);

  for (int i = 0; i < 128; i++)
    oled_write_data(i);
}

u8 oled_start()
{
  if (!(tw_send_start() & (TW_START | TW_REP_START))) return 0;
  if (!(tw_send_addr(oled_addr | TW_WRITE) == TW_MT_SLA_ACK)) return 0;

  return 1;
}

void oled_write_cmd(u8 cmd)
{
  // Command sequence: S AW 00 cc P

  if (!oled_addr) return;
  if (!oled_start()) return;
  if (!(tw_send_data(0x00) == TW_MT_DATA_ACK)) return;
  tw_send_data(cmd);
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
  if (!oled_start()) return;
  if (!(tw_send_data(0x40) == TW_MT_DATA_ACK)) return;

  while(len--)
    if (!(tw_send_data(*data++) == TW_MT_DATA_ACK)) return;

  tw_send_stop();
}
