
#include "stdafx.h"
#include <iostream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "spiffs/spiffs.h"
#include "getopt/getopt.h"

#define SPI_SIZE (2 * 1024 * 1024)
#define LOG_PAGE_SIZE 256

u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
u8_t spiffs_fds[32 * 4];
u8_t spiffs_cache_buf[(LOG_PAGE_SIZE + 32) * 4];

spiffs fs;
spiffs_config cfg;
u32 spi_size;
std::wstring img_name;
std::wstring add_name;
u8 *mem = 0;

std::string narrow(std::wstring const& text)
{
  std::locale const loc("");
  wchar_t const* from = text.c_str();
  std::size_t const len = text.size();
  std::vector<char> buffer(len + 1);
  std::use_facet<std::ctype<wchar_t> >(loc).narrow(from, from + len, '_', &buffer[0]);
  return std::string(&buffer[0], &buffer[len]);
}

s32_t spi_read(u32_t addr, u32_t size, u8_t *buf)
{
  // printf("spi_read: %08X, %u\n", addr, size);
  memcpy(buf, &mem[addr], size);
  return SPIFFS_OK;
}

s32_t spi_write(u32_t addr, u32_t size, u8_t *buf)
{
  // printf("spi_write: %08X, %u\n", addr, size);
  memcpy(&mem[addr], buf, size);
  return SPIFFS_OK;
}

s32_t spi_erase(u32_t addr, u32_t size)
{
  // printf("spi_erase: %08X, %u\n", addr, size);
  memset(&mem[addr], 255, size);
  return SPIFFS_OK;
}

void img_read()
{
  FILE *fp = _tfopen((_TCHAR*)img_name.c_str(), _T("rb"));

  if (fp)
  {
    fread(mem, 1, spi_size, fp);
    fclose(fp);
  }
}

void img_write()
{
  FILE *fp = _tfopen((_TCHAR*)img_name.c_str(), _T("wb"));

  if (fp)
  {
    fwrite(mem, 1, spi_size, fp);
    fclose(fp);
  }
}

void img_config()
{
  cfg.phys_size = spi_size; // use all spi flash
  cfg.phys_addr = 0; // start spiffs at start of spi flash
  cfg.phys_erase_block = 65536; // according to datasheet
  cfg.log_block_size = 65536; // let us not complicate things
  cfg.log_page_size = LOG_PAGE_SIZE; // as we said

  cfg.hal_read_f = spi_read;
  cfg.hal_write_f = spi_write;
  cfg.hal_erase_f = spi_erase;
}

void img_create()
{
  mem = (u8*)malloc(spi_size);
  memset(mem, 255, spi_size);

  SPIFFS_mount(
    &fs,
    &cfg,
    spiffs_work_buf,
    spiffs_fds,
    sizeof(spiffs_fds),
    spiffs_cache_buf,
    sizeof(spiffs_cache_buf),
    0);
  SPIFFS_unmount(&fs);
  SPIFFS_format(&fs);

  img_write();
  free(mem);
}

void img_add()
{
  mem = (u8*)malloc(spi_size);
  img_read();

  FILE *fp = _tfopen((_TCHAR*)add_name.c_str(), _T("rb"));
  
  if(!fp) return;
  
  fseek(fp, 0 , SEEK_END);
  int fsize = ftell(fp);
  fseek(fp, 0 , SEEK_SET);
  
  u8 *buf = (u8*)malloc(fsize);
  fread(buf, 1, fsize, fp);
  fclose(fp);

  SPIFFS_mount(
    &fs,
    &cfg,
    spiffs_work_buf,
    spiffs_fds,
    sizeof(spiffs_fds),
    spiffs_cache_buf,
    sizeof(spiffs_cache_buf),
    0);

  spiffs_file fd = SPIFFS_open(&fs, narrow(add_name).c_str(), SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs, fd, buf, fsize) < 0) printf("errno %i\n", SPIFFS_errno(&fs));
  SPIFFS_close(&fs, fd);
  free(buf);

  img_write();
  free(mem);
}

void img_list()
{
}

void img_free()
{
}

void img_delete()
{
}

int _tmain(int argc, _TCHAR* argv[])
{
  if (argc == 1)
  {
    printf("SPIFFS utility\n");
    printf("\nUsage:\n");
    printf("spiffs.exe <options> <operations>\n");
    printf("\nOptions:\n");
    printf(" i <filename> - new or existing image filename\n");
    printf(" s <size> - image size in kB\n");
    printf("\nOperations (sequence of a few is allowed):\n");
    printf(" c - create new image\n");
    printf(" a <filename> - add a file to image\n");
    printf(" l - list files\n");
    printf(" f - get free space in bytes\n");
    printf("\nExample:\n");
    printf("spiffs.exe -i spif.img -s 4096 -c -a conf.rbf -a ts.mlz\n");
    exit(1);
  }

  spi_size = SPI_SIZE;
  img_name = _T("new.img");
  img_config();

  int c = 0;
  while ((c = getopt (argc, argv, _T("i:cs:a:l"))) != -1)
    switch(c)
    {
      // image name
      case _T('i'):
        img_name = optarg;
      break;

      // image size
      case _T('s'):
        spi_size = _wtoi(optarg) * 1024;
        img_config();
      break;

      // create new image
      case _T('c'):
        img_create();
      break;

      // list files
      case _T('l'):
        img_list();
      break;

      // get free space
      case _T('f'):
        img_free();
      break;

      // add a file
      case _T('a'):
        add_name = optarg;
        img_add();
      break;

      default:
        exit(2);
    }

  return 0;
}
