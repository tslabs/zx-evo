
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

#define DBUF ((u8*)0x8000);

struct
{
  u8 len;
  char name[64];
} ap;

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
  TS_GYOFFSL = 0;
  TS_GYOFFSH = 0;
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
    ft_load_ram_dma(0x0000, 0xF2, FT_RAM_DL, 6144);
  }
}

void test_cobra()
{
  TS_GYOFFSL = 0;
  TS_GYOFFSH = 0;
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
    esp_recv_dma(0x0000, 0xF2, 256, 24);
    ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
    while (!(ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP));
    ft_load_ram_dma(0x0000, 0xF2, FT_RAM_DL, 6144);
  }
}

void test_noise()
{
  TS_GYOFFSL = 0;
  TS_GYOFFSH = 0;
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

void print_status()
{
  u8 status = esp_rd_reg(ESP_REG_STATUS);
  esp_cmd(ESP_CMD_GET_ERROR);
  u8 error = esp_rd_reg(ESP_REG_ERROR);
  esp_cmd(ESP_CMD_GET_NETSTATE);
  u8 netstate = esp_rd_reg(ESP_REG_NETSTATE);

  printf("\a7Status: \aC%02X", status);
  printf("\t\a7Error: \aC%02X", error);
  printf("\t\a7Netstate: \aC%02X\r\n", netstate);
}

void test_reset()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  printf("\aFReset test\r\n");

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

  print_status();
  wait_enter();
}

void test_info()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  printf("\aFInfo test\r\n");

  print_status();

  esp_cmd(ESP_CMD_GET_INFO);

  u8 len;
  esp_rd_regs(ESP_REG_DATA_SIZE, (u8*)&len, 1);
  printf("\a7GET_INFO sent. Data \aC%d \a7bytes\r\n", len);
  len = min(len, 61);

  char b[62];
  esp_rd_regs(ESP_REG_DATA, b, len);
  b[len] = 0;
  printf("\a7Info string: \aC%s\r\n", b);
}

void test_wscan()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  ap.len = 0;

  printf("\aFWiFi AP scan test\r\n");

  esp_cmd(ESP_CMD_WSCAN);
  printf("\a7WSCAN sent. Waiting for data...");

  u8 rc = esp_wait_busy(65000);
  printf(" %s", rc ? "\a4OK" : "\aATimeout\r\n");

  if (rc)
  {
    u8 *dbuf = DBUF;
    u16 len;
    esp_rd_regs(ESP_REG_DATA_SIZE, (u8*)&len, 2);
    printf(", \aC%d \a7bytes\r\n", len);

    esp_recv(dbuf, len);
    u8 num = dbuf[0];
    printf("\a7Number of APs: \aC%d\r\n\r\n", num);
    int ptr = 1;

    for (u8 i = 0; i < num; i++)
    {
      if ((i & 15) == 0)
        printf("\aFAuth\tRSSI, dB\tChannel\tSSID\r\n");

      u8 auth = dbuf[ptr++];
      i8 rssi = dbuf[ptr++];
      u8 chan = dbuf[ptr++];
      u8 ssid_len = dbuf[ptr++];
      char ssid[64];
      memcpy(ssid, &dbuf[ptr], ssid_len);
      ssid[ssid_len] = 0;
      ptr += ssid_len;

      printf("%s\a7 (%d)\t\aC%d\t%d\t\a7%s\r\n", auth ? "\aApswd" : "\aCopen", auth, rssi, chan, ssid);

      if ((i & 15) == 15)
        wait_enter();
    }
  }
}

void test_conn()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  printf("\aFWiFi connect test\r\n");

  const u8 ap_ssid[] = "tsl";
  const u8 ap_pwd[] = "tslabs777";

  esp_cmdd(ESP_CMD_SET_AP_NAME, ap_ssid, sizeof(ap_ssid) - 1);
  esp_cmdd(ESP_CMD_SET_AP_PWD, ap_pwd, sizeof(ap_pwd) - 1);
  
  esp_cmd(ESP_CMD_AP_CONNECT);
  esp_wait_busy(65000);

  esp_cmd(ESP_CMD_GET_NETSTATE);
  u8 netstate = esp_rd_reg(ESP_REG_NETSTATE);
  printf("Network %sconnected\r\n", (netstate == NETWORK_OPEN) ? "\aC" : "\aAdis");
}

void test_ip()
{
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  struct
  {
    u8 ip[4];
    u8 own_ip[4];
    u8 mask[4];
    u8 gate[4];
  } net;

  esp_cmd(ESP_CMD_GET_IP);
  esp_rd_regs(ESP_REG_IP, &net, sizeof(net));
  printf("\aFIP: \aC%d.%d.%d.%d\aF, Mask: \aC%d.%d.%d.%d\aF, Gate: \aC%d.%d.%d.%d\r\n", net.own_ip[0], net.own_ip[1], net.own_ip[2], net.own_ip[3], net.mask[0], net.mask[1], net.mask[2], net.mask[3], net.gate[0], net.gate[1], net.gate[2], net.gate[3]);
}

void main()
{
  TS_BORDER = 0;
  init_print();

  // TS_FMADDR = 0x10 + 0xC;
  // *(u16*)(0xC1F0) = 32768 + 4096 + 128 + 4;
  // TS_FMADDR = 0;
  ft_init(FT_MODE_1024_768_67);
  printf("\aDTest of ESP32 SPI\r\n\r\n");

  esp_cmd(ESP_CMD_DATA_END);

  while (1)
  {

    // test_reset();
    test_info();
    // wait_enter();

    printf("\r\n");
    test_wscan();
    test_conn();
    test_ip();
    wait_enter();

    test_noise();
    test_tunnel();
    test_cobra();
  }
}
