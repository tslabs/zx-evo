
// ESP32 VDAC3 emulator

#include "std.h"
#include "vars.h"
#include "util.h"
#include "esp32_emul.h"

#include "ft812.h"
#include "tonnel.cpp"
#include "cobra.cpp"

enum
{
  ST_IDLE = 0,
  ST_DUMMY,
  ST_REG,
  ST_WR_REGS,
  ST_RD_REGS,
  ST_WR_DMA,
  ST_RD_DMA
};

int state = ST_IDLE;
int next_state = ST_IDLE;
int dummy_cnt = 0;
int cmd = 0;
int reg = 0;
u8 regs[64];
u8* data = NULL;
int data_ptr = 0;
int data_len;
u8 data_buf[65536];
bool is_init = false;

const char info_string[] = "ESP32 SPI Emulator Unreal Speccy";

u8 esp32_emul(int d)
{
  if (!is_init)
  {
    calc_sin();
    is_init = true;
  }
  
  u8 r = 0xFF;

  if (d == -1)
  {
    // Execute a command on ~SS fall
    if ((state == ST_WR_REGS) && (cmd == ESP_SPI_CMD_WR_REGS))
    {
      int scmd = regs[ESP_REG_COMMAND];
      data_len = 0;
      int status = ESP_ST_IDLE;

      // printf("ESP32 Slave CMD: %02X\n", scmd);

      switch (scmd)
      {
        case ESP_CMD_GET_INFO:
        {
          data = (u8*)info_string;
          data_len = sizeof(info_string);
          status = ESP_ST_DATA_S2M;
        }
        break;

        case ESP_CMD_GET_RND:
        {
          for (int i = 0; i < 6912; i++)
            data_buf[i] = rand() >> 7;

          data = data_buf;
          data_len = 6912;
          status = ESP_ST_DATA_S2M;
        }
        break;

        case ESP_CMD_TEST2:
        {
          data = data_buf;
          data_len = tunnel(data);
          status = ESP_ST_DATA_S2M;
        }
        break;
  
        case ESP_CMD_TEST3:
        {
          data = data_buf;
          data_len = cobra(data);
          status = ESP_ST_DATA_S2M;
        }
        break;

        case ESP_CMD_DMA_END:
          break;
      }

      regs[ESP_REG_STATUS] = status;
      *(u32*)&regs[ESP_REG_DATA_SIZE] = data_len;
    }

    state = ST_IDLE;
    next_state = ST_IDLE;
  }
  else
  {
    switch (state)
    {
      case ST_IDLE:
        // Parse SPI command
        cmd = d;

        switch (cmd)
        {
          case ESP_SPI_CMD_WR_REGS:
          case ESP_SPI_CMD_RD_REGS:
            state = ST_REG;
            break;

          case ESP_SPI_CMD_WR_DMA:
            break;

          case ESP_SPI_CMD_RD_DMA:
            data_ptr = 0;
            state = ST_DUMMY;
            dummy_cnt = 2;
            next_state = ST_RD_DMA;
            break;
        }
        break;

      case ST_REG:
        reg = d;

        switch (cmd)
        {
          case ESP_SPI_CMD_WR_REGS:
            state = ST_DUMMY;
            dummy_cnt = 1;
            next_state = ST_WR_REGS;
            break;

          case ESP_SPI_CMD_RD_REGS:
            state = ST_DUMMY;
            dummy_cnt = 1;
            next_state = ST_RD_REGS;
            break;
        }
        break;

      case ST_WR_REGS:
        if (reg < sizeof(regs))
          regs[reg++] = d;
        break;

      case ST_RD_REGS:
        if (reg < sizeof(regs))
          r = regs[reg++];
        break;

      case ST_RD_DMA:
        if (data_ptr < data_len)
          r = data[data_ptr++];
        break;

      case ST_DUMMY:
        if (!--dummy_cnt)
          state = next_state;

        break;
    }
  }

  return r;
}
