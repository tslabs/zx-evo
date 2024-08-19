
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>
#include "tsconf.h"
#include <tslib.h>
#include <ft812.h>
#include <ft812lib.h>
#include <font.h>
#include <xm.h>
#include "../../../../esp32/src/main/esp_spi_defs.h"

#include <esp32.c>
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

int lib_hdl;
u8 dat_hdl;

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
  printf("\aE\r\nPress Enter");
  wait_key();
  printf("\r\n");
}

void delay(u32 d)
{
  while (d--);
}

void check_error(const char *s)
{
  u8 e = esp_rd_reg8(ESP_REG_STATUS);
  u32 time = esp_rd_reg32(ESP_EXEC_TIME);
  printf("\a7%s: %s \a7(%02X)  \aF%ld \a7us\r\n", s, e == ESP_ST_READY ? "\aCOK" : "\aAERROR", e, time);
}

void print_status()
{
  u8 status = esp_rd_reg8(ESP_REG_STATUS);
  esp_cmd(ESP_CMD_GET_NETSTATE);
  u8 netstate = esp_rd_reg8(ESP_REG_NETSTATE);

  printf("\a7Status: \aC%02X", status);
  printf("\t\a7Netstate: \aC%02X\r\n", netstate);
}

#define LIB_SZ 13600

void load_lib()
{
  cls();
  printf("\r\n\aFLIB load\r\n\r\n");

  // Clear memory
  esp_cmd(ESP_CMD_KILL_OBJECTS);
  esp_wait_busy(1000);
  check_error("ESP_CMD_KILL_OBJECTS");

  // Create DAT objects for output arrays
  u8 dat_hdl = esp_create_obj(8192, OBJ_TYPE_DATA);
  check_error("ESP_CMD_MAKE_OBJECT");

  // Create archive object
  // u8 hh = esp_create_obj(LIB_SZ, OBJ_TYPE_HST);
  u8 hh = esp_create_obj(LIB_SZ, OBJ_TYPE_ZIP);
  check_error("ESP_CMD_MAKE_OBJECT");

  // Load archive binary
  esp_cmd(ESP_CMD_WRITE_OBJECT);
  esp_wait_status(ESP_ST_DATA_M2S, 2000);
  esp_send_dma(0xC000, 8, 512, 32);   // Page 8, Addr 0xC000, 16k
  check_error("ESP_CMD_WRITE_OBJECT");

  // Unpack archive
  esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_ELF);
  esp_wr_reg32(ESP_REG_DATA_SIZE, LIB_SZ);
  // esp_cmd(ESP_CMD_DEHST);
  esp_cmd(ESP_CMD_UNZIP);
  esp_wait_busy(10000);
  check_error("ESP_CMD_UNZIP");

  // Initialize ELF
  esp_cmd(ESP_CMD_LOAD_ELF);
  esp_wait_busy(1000);
  check_error("ESP_CMD_LOAD_ELF");
  lib_hdl = esp_rd_reg8(ESP_REG_LIB_HANDLE);

  // Call library function 0
  esp_wr_reg8(ESP_REG_FUNC, 0);
  esp_wr_reg32(ESP_REG_ARG, 12345);
  esp_wr_reg8(ESP_REG_ARR1_HANDLE, dat_hdl);
  esp_cmd(ESP_CMD_RUN_FUNC1);
  esp_wait_busy(1000);
  check_error("ESP_CMD_RUN_FUNC");
  printf("retval: %lu\r\n", esp_rd_reg32(ESP_REG_RETVAL));

  // Call library function 2
  esp_wr_reg8(ESP_REG_FUNC, 2);
  esp_wr_reg32(ESP_REG_ARG, 10);
  esp_wr_reg8(ESP_REG_ARR1_HANDLE, dat_hdl);
  esp_cmd(ESP_CMD_RUN_FUNC1);
  esp_wait_busy(1000);
  check_error("ESP_CMD_RUN_FUNC");
  printf("retval: %lu\r\n", esp_rd_reg32(ESP_REG_RETVAL));

  wait_enter();
}

void test_dl(u8 f)
{
  if (lib_hdl == -1)
    load_lib();

  cls();
  TS_VCONFIG = 0x87;
  printf("\r\n\aFLIB load\r\n\r\n");

  ft_wreg8(FT_REG_INT_MASK, FT_INT_SWAP);
  ft_wreg8(FT_REG_INT_EN, 1);

  esp_wr_reg32(ESP_REG_LIB_HANDLE, lib_hdl);
  esp_wr_reg8(ESP_REG_ARR1_HANDLE, dat_hdl);
  esp_wr_reg8(ESP_REG_FUNC, f);

  u8 was_key_released = 0;

  while (1)
  {
    if ((!was_key_released) && (KBD & 1))
      was_key_released = 1;

    if ((was_key_released) && !(KBD & 1))
      break;

    esp_cmd(ESP_CMD_RUN_FUNC1);
    esp_wait_busy(5000);
    check_error("ESP_CMD_RUN_FUNC");

    esp_wr_reg8(ESP_REG_OBJ_HANDLE, dat_hdl);
    esp_wr_reg32(ESP_REG_DATA_SIZE, 6144);
    esp_wr_reg32(ESP_REG_DATA_OFFSET, 0);
    esp_cmd(ESP_CMD_READ_OBJECT);
    esp_wait_status(ESP_ST_DATA_S2M, 5000);
    esp_recv_dma(0x0000, 0xF2, 32, 192);

    ft_wreg8(FT_REG_DLSWAP, FT_DLSWAP_FRAME);
    while (!(ft_rreg8(FT_REG_INT_FLAGS) & FT_INT_SWAP));

    ft_load_ram_dma(0x0000, 0xF2, FT_RAM_DL, 6144);
  }
}

void test_noise()
{
  TS_PAGE3 = 0xF2;
  memset((void*)0xC000, 0, 6144);
  memset((void*)0xD800, 0x47, 768);
  TS_GYOFFSL = 0;
  TS_GYOFFSH = 0;
  TS_VCONFIG = 0;
  TS_VPAGE = 0xF2;
  TS_VSINTL = 32 + 48;

  asm(ei); asm(halt);

  u8 was_key_released = 0;

  while (1)
  {
    if ((!was_key_released) && (KBD & 1))
      was_key_released = 1;

    if ((was_key_released) && !(KBD & 1))
      break;

    esp_wr_reg32(ESP_REG_DATA_SIZE, 6144);
    TS_BORDER = 13;
    esp_cmd(ESP_CMD_GET_RND);
    esp_wait_status(ESP_ST_DATA_S2M, 5000);
    TS_BORDER = 0;

    asm(ei); asm(halt);

    TS_BORDER = 17;
    esp_recv_dma(0x0000, 0xF2, 32, 192);
    TS_BORDER = 0;
  }
}

void test_reset()
{
  cls();
  printf("\aFReset test\r\n\r\n");

  printf("\a7Status: \aC%02X\r\n", esp_rd_reg8(ESP_REG_STATUS));
  esp_cmd(ESP_CMD_RESET);
  printf("\a7RESET sent. Waiting for ready...");

  u32 i = esp_wait_status(ESP_ST_RESET, 20000);

  if (!i)
    printf("\a2Timeout!\r\n");
  else
  {
    // u32 t = 20000 - i;
    // t = t * (469 * 1000L / 3.5f) / 1000;
    // u16 ti = t / 1000;
    // printf("\aCOK (%d ms)\r\n", ti);
    printf("\aCOK\r\n");
  }

  print_status();
}

void test_info()
{
  u8 len;
  char b[62];
  printf("\aF\r\nInfo strings:\r\n");

  // Checking for all available strings
  for (u16 i = 0; i < 256; i++)
  {
    esp_cmdp1(ESP_CMD_GET_INFO_STR, i);
    len = esp_rd_reg8(ESP_REG_STRING_SIZE);

    if (len)
    {
      len = min(len, 61);
      esp_rd_regs(ESP_REG_STRING_DATA, b, len);
      b[len] = 0;
      printf("\aF%02X: \aC%s\r\n", i, b);
    }
  }
}

void test_wscan()
{
  ap.len = 0;

  cls();
  printf("\r\n\aFWiFi AP scan test\r\n\r\n");

  esp_cmd(ESP_CMD_WSCAN);
  printf("\a7WSCAN sent. Waiting for data...");

  u8 rc = esp_wait_busy(65000);
  printf(" %s", rc ? "\a4OK" : "\aATimeout\r\n");

  if (rc)
  {
    u8 *dbuf = DBUF;
    u16 len = esp_rd_reg32(ESP_REG_DATA_SIZE);
    printf(", \aC%d \a7bytes\r\n", len);

    esp_recv(dbuf, len);
    u8 num = dbuf[0];
    printf("\a7Number of APs: \aC%d\r\n", num);
    int ptr = 1;

    for (u8 i = 0; i < num; i++)
    {
      if ((i & 15) == 0)
        printf("\aF\r\nAuth\tRSSI, dB\tChannel\tSSID\r\n");

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
  cls();
  printf("\r\n\aFWiFi connect test\r\n");

  const u8 ap_ssid[] = "tsl";
  const u8 ap_pwd[] = "tslabs777";

  esp_cmdd(ESP_CMD_SET_AP_NAME, ap_ssid, sizeof(ap_ssid) - 1);
  esp_cmdd(ESP_CMD_SET_AP_PWD, ap_pwd, sizeof(ap_pwd) - 1);

  esp_cmd(ESP_CMD_AP_CONNECT);
  esp_wait_busy(65000);

  esp_cmd(ESP_CMD_GET_NETSTATE);
  u8 netstate = esp_rd_reg8(ESP_REG_NETSTATE);
  printf("Network %sconnected\r\n", (netstate == NETWORK_OPEN) ? "\aC" : "\aAdis");
}

void test_ip()
{
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

void test_xm()
{
  cls();
  printf("\r\n\aFXM test\r\n\r\n");

  TS_PAGE3 = 0xF0;
  memcpy((void*)0xF800, xm, sizeof(xm));

  esp_cmd(ESP_CMD_XM_STOP);
  esp_wait_busy(1000);
  check_error("ESP_CMD_XM_STOP");

  esp_cmd(ESP_CMD_KILL_OBJECTS);
  esp_wait_busy(1000);
  check_error("ESP_CMD_KILL_OBJECTS");

  // Create HST object
  u8 hh = esp_create_obj(sizeof(xm), OBJ_TYPE_HST);
  check_error("ESP_CMD_MAKE_OBJECT");

  // Load HST binary
  esp_cmd(ESP_CMD_WRITE_OBJECT);
  esp_wait_status(ESP_ST_DATA_M2S, 2000);
  esp_send_dma(0xF800, 0xF0, 512, 2); // 1024 bytes
  check_error("ESP_CMD_WRITE_OBJECT");

  // Unpack HST to XM
  esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_XM);
  esp_wr_reg32(ESP_REG_DATA_SIZE, sizeof(xm));
  esp_cmd(ESP_CMD_DEHST);
  esp_wait_busy(10000);
  check_error("ESP_CMD_DEHST");

  // Initialize
  esp_cmd(ESP_CMD_XM_INIT);
  esp_wait_busy(1000);
  check_error("ESP_CMD_XM_INIT");

  esp_cmd(ESP_CMD_XM_PLAY);
  esp_wait_busy(1000);
  check_error("ESP_CMD_XM_PLAY");

  wait_enter();

  esp_cmd(ESP_CMD_XM_STOP);
  esp_wait_busy(1000);
  check_error("ESP_CMD_XM_STOP");
  esp_cmd(ESP_CMD_DELETE_OBJECT);
  esp_wait_busy(1000);
  check_error("ESP_CMD_DELETE_OBJECT");
}

void main()
{
  TS_BORDER = 0;
  TS_VCONFIG = 0x83;
  TS_VPAGE = 0xF0;
  TS_PAGE3 = 0xF0;

  lib_hdl = -1;

  ft_init(FT_MODE_1024_768_67);

  esp_rd_end();
  esp_wr_end();

start:
  cls();
  printf("\aDESP32 SPI test\r\n\r\n");
  printf("\aF1. \aCReset and info\r\n");
  printf("\aF2. \aCWiFi connect\r\n");
  printf("\aF3. \aCWiFi scan\r\n");
  printf("\aF4. \aCXM player\r\n");
  printf("\aF5. \aCLibrary load\r\n");
  printf("\aF6. \aCRandom number generation\r\n");
  printf("\aF7. \aCTunnel demo\r\n");
  printf("\aF8. \aCCobra demo\r\n");

  while (1)
  {
    switch (getkey())
    {
      case KEY_1:
        test_reset();
        test_info();
        wait_enter();
        goto start;
      break;

      case KEY_2:
        test_conn();
        test_ip();
        wait_enter();
        goto start;
      break;

      case KEY_3:
        test_wscan();
        wait_enter();
        goto start;
      break;

      case KEY_4:
        test_xm();
        wait_enter();
        goto start;
      break;

      case KEY_5:
        load_lib();
        goto start;
      break;

      case KEY_6:
        test_noise();
        goto start;
      break;

      case KEY_7:
        test_dl(10);
        goto start;
      break;

      case KEY_8:
        test_dl(11);
        goto start;
      break;
    }
  }
}
