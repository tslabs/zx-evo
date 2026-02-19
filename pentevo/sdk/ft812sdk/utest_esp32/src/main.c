
#include <stdio.h>
#include <string.h>
#include <defs.h>
#include <sdklib.h>

#include "tsconf.h"
#include "../../../../esp32/src/main/esp_spi_defs.h"
#include <font.h>

#include <esp32.c>
#include "print.c"

__sfr __banked __at 0x00FE KBD;
#define DBUF ((u8*)0x8000);

// -------------------- helpers --------------------

void wait_key()
{
  while (!(KBD & 1));
  while (KBD & 1);
}

void suite_halt_and_restart()
{
  printf("\r\n\aEPress Enter to restart...\r\n");
  while (1)
  {
    wait_key();
    return;
  }
}

void esp_zero_regs()
{
  for (u8 a = 0; a < 0x40; a++)
    esp_wr_reg8(a, 0);
}

void dump_regs()
{
  u8 b[64];
  u8 v;

  esp_rd_regs(0x00, b, 64);

  printf("\r\n\a7Reg dump:\r\n");
  printf("\aF    0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\r\n");

  for (u8 row = 0; row < 4; row++)
  {
    printf("\aF%02X ", (row << 4));

    for (u8 col = 0; col < 16; col++)
    {
      v = b[(row << 4) + col];
      printf("\a4%02X ", v);
    }

    printf("\r\n");
  }

  printf("\r\n");
}

void fail_stop(const char *msg, u8 st)
{
  u32 t = esp_rd_reg32(ESP_EXEC_TIME);
  printf("\aAFAIL\a7: %s  status=\aA%02X\a7  exec=\aF%lu\a7us\r\n", msg, st, t / 1000);
  dump_regs();
  suite_halt_and_restart();
}

void step_ok(const char *msg)
{
  u32 t = esp_rd_reg32(ESP_EXEC_TIME);
  printf("\aCOK\a7: %s  exec=\aF%lu\a7us\r\n", msg, t / 1000);
}

void step_info(const char *msg)
{
  printf("\a7%s\r\n", msg);
}

u8 wait_status_eq(u8 want, u32 timeout_ms)
{
  u32 rc = esp_wait_status(want, timeout_ms * 25);  // roughly ms
  return rc ? 1 : 0;
}

// -------------------- tests --------------------

void test_reset_and_status()
{
  printf("\r\n\aFReset + expect ESP_ST_RESET\r\n");

  step_info("Sending ESP_CMD_RESET");
  esp_cmd(ESP_CMD_RESET);

  step_info("Waiting for ESP_ST_RESET (timeout 1s)");
  if (!wait_status_eq(ESP_ST_RESET, 1000))
  {
    u8 st = esp_rd_reg8(ESP_REG_STATUS);
    fail_stop("reset timeout (no ESP_ST_RESET)", st);
  }

  {
    u8 st = esp_rd_reg8(ESP_REG_STATUS);
    if (st != ESP_ST_RESET)
      fail_stop("status mismatch after reset", st);
  }

  step_ok("Device reset and ESP_ST_RESET observed");
}

void test_make_object_data()
{
  u8 st;
  u8 h;

  printf("\r\n\aFMAKE_OBJECT: 65536 bytes, type DATA\r\n");

  step_info("Sending ESP_CMD_MAKE_OBJECT");
  esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_DATA);
  esp_wr_reg32(ESP_REG_DATA_SIZE, 65536UL);
  esp_cmd(ESP_CMD_MAKE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("MAKE_OBJECT returned non-READY status", st);

  h = esp_rd_reg8(ESP_REG_OBJ_HANDLE);
  printf("\a7Handle: \aC%u\r\n", h);

  step_ok("Object created");

  step_info("Sending ESP_CMD_DELETE_OBJECT");
  esp_wr_reg8(ESP_REG_OBJ_HANDLE, h);
  esp_cmd(ESP_CMD_DELETE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("DELETE_OBJECT returned non-READY status", st);

  step_ok("Object deleted");
}

void test_get_info_strings()
{
  u8 st;
  u8 len;
  char b[62];

  printf("\r\n\aFGET_INFO_STR: enumerate available strings\r\n");

  for (u16 i = 0; i < 256; i++)
  {
    esp_cmdp1(ESP_CMD_GET_INFO_STR, i);
    esp_wait_busy(1000);

    st = esp_rd_reg8(ESP_REG_STATUS);
    if (st != ESP_ST_READY)
      fail_stop("GET_INFO_STR returned non-READY status", st);

    len = esp_rd_reg8(ESP_REG_STRING_SIZE);
    if (!len)
      continue;

    if (len > 61)
      len = 61;

    esp_rd_regs(ESP_REG_STRING_DATA, b, len);
    b[len] = 0;

    printf("\aF%02X: \aC%s\r\n", i, b);
  }

  step_ok("GET_INFO_STR completed");
}

void test_data_transfer()
{
  u8 st;
  u8 h;
  u8 hi;
  u8 exp;
  u8 got;
  u8 *tx;
  u8 *rx;
  u32 abs_ofs;

  printf("\r\n\aFData transfer: 64k write/read/compare in 256-byte chunks (per-chunk command)\r\n");

  tx = DBUF;
  rx = DBUF + 256;

  step_info("Sending ESP_CMD_MAKE_OBJECT");
  esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_DATA);
  esp_wr_reg32(ESP_REG_DATA_SIZE, 65536UL);
  esp_cmd(ESP_CMD_MAKE_OBJECT);

  esp_wait_busy(5000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("MAKE_OBJECT returned non-READY status", st);

  h = esp_rd_reg8(ESP_REG_OBJ_HANDLE);
  printf("\a7Handle: \aC%u\r\n", h);

  esp_wr_reg32(ESP_REG_DATA_OFFSET, 0);
  esp_wr_reg32(ESP_REG_DATA_SIZE, 256);

  for (u16 blk = 0; blk < 256; blk++)
  {
    hi = (u8)blk;

    for (u16 i = 0; i < 256; i++)
      tx[i] = ((u8)i) ^ hi;

    // step_info("Sending ESP_CMD_WRITE_OBJECT");
    esp_cmd(ESP_CMD_WRITE_OBJECT);

    if (!esp_wait_status(ESP_ST_DATA_M2S, 2000))
    {
      st = esp_rd_reg8(ESP_REG_STATUS);
      fail_stop("WRITE_OBJECT timeout waiting for DATA_M2S", st);
    }

    esp_send(tx, 256);

    if (!esp_wait_not_status(ESP_ST_DATA_M2S, 2000))
    {
      st = esp_rd_reg8(ESP_REG_STATUS);
      fail_stop("WRITE_OBJECT timeout waiting end of DATA_M2S", st);
    }

    st = esp_rd_reg8(ESP_REG_STATUS);
    if (st != ESP_ST_READY)
      fail_stop("WRITE_OBJECT returned non-READY status", st);
  }

  step_ok("64k write completed");

  esp_wr_reg32(ESP_REG_DATA_OFFSET, 0);
  esp_wr_reg32(ESP_REG_DATA_SIZE, 256);

  for (u16 blk = 0; blk < 256; blk++)
  {
    hi = (u8)blk;


    // step_info("Sending ESP_CMD_READ_OBJECT");
    esp_cmd(ESP_CMD_READ_OBJECT);

    if (!esp_wait_status(ESP_ST_DATA_S2M, 2000))
    {
      st = esp_rd_reg8(ESP_REG_STATUS);
      fail_stop("READ_OBJECT timeout waiting for DATA_S2M", st);
    }

    esp_recv(rx, 256);

    if (!esp_wait_not_status(ESP_ST_DATA_S2M, 2000))
    {
      st = esp_rd_reg8(ESP_REG_STATUS);
      fail_stop("READ_OBJECT timeout waiting end of DATA_S2M", st);
    }

    st = esp_rd_reg8(ESP_REG_STATUS);
    if (st != ESP_ST_READY)
      fail_stop("READ_OBJECT returned non-READY status", st);

    for (u16 i = 0; i < 256; i++)
    {
      exp = ((u8)i) ^ hi;
      got = rx[i];

      if (got != exp)
      {
        abs_ofs = ((u32)blk << 8) + i;
        printf("\aAMismatch\a7 at \aC%lu\a7: exp=\aC%02X\a7 got=\aA%02X\r\n", abs_ofs, exp, got);
        st = esp_rd_reg8(ESP_REG_STATUS);
        fail_stop("data compare failed", st);
      }
    }
  }

  step_ok("64k read/compare OK");

  step_info("Sending ESP_CMD_DELETE_OBJECT");
  esp_cmd(ESP_CMD_DELETE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("DELETE_OBJECT returned non-READY status", st);

  step_ok("Object deleted");
}

void test_http_get()
{
  u8 st;
  u8 url_h;
  u8 resp_h;
  u32 resp_sz;

  const char url[] = "http://ipv4.download.thinkbroadband.com/5MB.zip";
  // const char url[] = "http://zxart.ee/file/id:589755/filename:589755.scr";
  // const char url[] = "http://zifi.vtrd.in/zifi_ver.php?w=0732";

  printf("\r\n\aFHTTP client: ESP_CMD_HTTP_GET\r\n");

  step_info("Sending ESP_CMD_MAKE_OBJECT (URL buffer)");
  esp_wr_reg8(ESP_REG_OBJ_TYPE, OBJ_TYPE_DATA);
  esp_wr_reg32(ESP_REG_DATA_SIZE, (u32)sizeof(url));
  esp_cmd(ESP_CMD_MAKE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("MAKE_OBJECT(URL) returned non-READY status", st);

  url_h = esp_rd_reg8(ESP_REG_OBJ_HANDLE);
  printf("\a7URL handle: \aC%u\r\n", url_h);

  step_info("Sending ESP_CMD_WRITE_OBJECT (URL string, 0-terminated)");
  esp_wr_reg8(ESP_REG_OBJ_HANDLE, url_h);
  esp_wr_reg32(ESP_REG_DATA_OFFSET, 0);
  esp_wr_reg32(ESP_REG_DATA_SIZE, (u32)sizeof(url));
  esp_cmd(ESP_CMD_WRITE_OBJECT);

  if (!esp_wait_status(ESP_ST_DATA_M2S, 2000))
  {
    st = esp_rd_reg8(ESP_REG_STATUS);
    fail_stop("WRITE_OBJECT(URL) timeout waiting for DATA_M2S", st);
  }

  esp_send((const u8*)url, (u32)sizeof(url));

  if (!esp_wait_not_status(ESP_ST_DATA_M2S, 2000))
  {
    st = esp_rd_reg8(ESP_REG_STATUS);
    fail_stop("WRITE_OBJECT(URL) timeout waiting end of DATA_M2S", st);
  }

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("WRITE_OBJECT(URL) returned non-READY status", st);

  step_ok("URL uploaded");

  step_info("Sending ESP_CMD_HTTP_GET");
  esp_wr_reg8(ESP_REG_OBJ_HANDLE, url_h);
  esp_cmd(ESP_CMD_HTTP_GET);

  esp_wait_busy(6500000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("HTTP_GET returned non-READY status", st);

  resp_h = esp_rd_reg8(ESP_REG_OBJ_HANDLE);
  resp_sz = esp_rd_reg32(ESP_REG_DATA_SIZE);

  printf("\a7Response handle: \aC%u\a7, size: \aC%lu\a7 bytes\r\n", resp_h, resp_sz);

  step_ok("HTTP_GET completed");

  step_info("Sending ESP_CMD_DELETE_OBJECT (URL)");
  esp_wr_reg8(ESP_REG_OBJ_HANDLE, url_h);
  esp_cmd(ESP_CMD_DELETE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("DELETE_OBJECT(URL) returned non-READY status", st);

  step_ok("URL object deleted");

  step_info("Sending ESP_CMD_DELETE_OBJECT (response)");
  esp_wr_reg8(ESP_REG_OBJ_HANDLE, resp_h);
  esp_cmd(ESP_CMD_DELETE_OBJECT);

  esp_wait_busy(2000);

  st = esp_rd_reg8(ESP_REG_STATUS);
  if (st != ESP_ST_READY)
    fail_stop("DELETE_OBJECT(response) returned non-READY status", st);

  step_ok("Response object deleted");
}

// -------------------- runner --------------------

void run_all_tests()
{
  printf("\r\n\aDESP32-SPI unit test suite\r\n");

  // Ensure SPI framing ends are in a known state (as in your main.c)
  esp_rd_end();
  esp_wr_end();

  // esp_zero_regs(); test_reset_and_status();
  esp_zero_regs(); test_get_info_strings();
  // esp_zero_regs(); test_make_object_data();
  // esp_zero_regs(); test_data_transfer();
  esp_zero_regs(); test_http_get();
  
  printf("\r\n\aCALL TESTS PASSED\a7\r\n");
}

void main()
{
  TS_BORDER = 0;

  while (1)
  {
    cls();
    run_all_tests();
    suite_halt_and_restart();
  }
}
