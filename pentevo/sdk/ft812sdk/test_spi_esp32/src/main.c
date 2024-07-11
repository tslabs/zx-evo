
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include "tsconf.h"
#include <tslib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <font.h>
#include "../../../../esp32/src/main/spi_slave.h"
#include "esp32.c"
#include "print.c"

// Borders:
// 5 - dark mgt
// 10 - lgt mgt
// 13 - pink
// 17 - dirty blue
// 21 - dark grey
// 25 - rose
// 28 - orange
// 29 - lgt pink
// 32 - salad

__sfr __banked __at 0x00FE KBD;

void dump(u8 *b, u8 n)
{
  for (u8 i = 0; i < n; i++)
  {
    printf("%02X ", b[i]);
    pca ^= 8;
  }

  printf("\r\n");
}

void wait_key()
{
  while (!(KBD & 1));
  while (KBD & 1);
}

void wait_enter()
{
  printf("\aEPress Enter\r\n");
  wait_key();
  printf("\r\n");
}

void test_tunnel()
{
  TS_VCONFIG = 4;
  TS_VPAGE = 0xF2;
  TS_PAGE3 = 0xF2;

  u8 was_key_released = 0;

  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);

  while (1)
  {
    if ((!was_key_released) && (KBD & 1))
      was_key_released = 1;

    if ((was_key_released) && !(KBD & 1))
      break;

    esp_cmd(ESP_CMD_TEST2);
    esp_wait_status(ESP_ST_DATA_S2M, 800);
    esp_recv_dma(0x0000, 0xF2, 256, 24);
    ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
    while (!(ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP));
    ft_load_ram_dma(0x0000, 0xF2, FT_RAM_DL, 8192);
  }
}

void test_cobra()
{
  TS_VCONFIG = 4;
  TS_PAGE3 = 0xF2;

  u8 was_key_released = 0;

  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);

  while (1)
  {
    if ((!was_key_released) && (KBD & 1))
      was_key_released = 1;

    if ((was_key_released) && !(KBD & 1))
      break;

    esp_cmd(ESP_CMD_TEST3);
    esp_wait_status(ESP_ST_DATA_S2M, 800);
    esp_recv_dma(0x0000, 0xF2, 256, 32);
    ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
    while (!(ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP));
    ft_load_ram_dma(0x0000, 0xF2, FT_RAM_DL, 8192);
  }
}

void test_noise()
{
  TS_VCONFIG = 0;
  TS_VPAGE = 0xF2;
  TS_PAGE3 = 0xF2;
  memset((void*)0xC000, 0, 6144);
  memset((void*)0xD800, 0x47, 768);

  TS_VSINTL = 32 + 48;
  asm(ei); asm(halt);

  u8 was_key_released = 0;

  while (1)
  {
    if ((!was_key_released) && (KBD & 1))
      was_key_released = 1;

    if ((was_key_released) && !(KBD & 1))
      break;

    TS_BORDER = 13;
    esp_cmd(ESP_CMD_GET_RND);
    esp_wait_status(ESP_ST_DATA_S2M, 800);
    TS_BORDER = 0;

    asm(ei); asm(halt);

    TS_BORDER = 17;
    esp_recv_dma(0x0000, 0xF2, 32, 192);
    TS_BORDER = 0;
  }
}

void test_reset()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  printf("\aFReset Test\r\n");

  printf("\a7Status: \aC%02X\r\n", esp_rd_reg(ESP_REG_STATUS));
  esp_cmd(ESP_CMD_RESET);
  printf("\a7RESET sent. Waiting for ready...");

  u32 i = esp_wait_status(ESP_ST_READY, 20000);

  if (!i)
    printf("\a2Timeout!\r\n");
  else
  {
    u32 t = 20000 - i;
    t = t * (469 * 1000L / 3.5f) / 1000;
    u16 ti = t / 1000;
    printf("\aCOK (%d ms)\r\n", ti);
  }

  printf("\a7Status: \aC%02X\r\n", esp_rd_reg(ESP_REG_STATUS));
  printf("\a7Ext Status: \aC%02X\r\n", esp_rd_reg(ESP_REG_EXT_STAT));
  wait_enter();
}

void test_info()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  printf("\aFInfo Test\r\n");

  esp_cmd(ESP_CMD_GET_INFO);
  printf("\a7GET_INFO sent. Waiting for data...");

  u8 rc = esp_wait_status(ESP_ST_DATA_S2M, 2000);
  printf(" %s", rc ? "\a4OK" : "\aATimeout\r\n");

  if (rc)
  {
    u32 len;
    esp_rd_regs(ESP_REG_DATA_SIZE, (u8*)&len, 4);
    printf(", \aC%d \a7bytes\r\n", len);
    len = min(len, 63);

    char b[64];
    esp_recv(b, len);
    b[len] = 0;
    printf("\a7Info string: \aC%s\r\n", b);
  }

  wait_enter();
}

void main()
{
  TS_BORDER = 0;
  init_print();
  // TS_FMADDR = 0x10 + 0xC;
  // *(u16*)(0xC1F0) = 32768 + 4096 + 128 + 4;
  // TS_FMADDR = 0;
  ft_init(FT_MODE_1024_768_67);
  printf("\aDTest of ESP32 SPI Slave\r\n\r\n");

  esp_cmd(ESP_CMD_DMA_END);

  while (1)
  {
    // test_reset();
    test_info();
    test_noise();
    test_tunnel();
    test_cobra();
  }
}
