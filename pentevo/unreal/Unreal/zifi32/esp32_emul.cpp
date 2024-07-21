
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
int error = ESP_ERR_RESET;
int netstate = NETWORK_CLOSED;
char ap_name[32];
char ap_pwd[61];
int ap_name_len = 0;
int ap_pwd_len = 0;

#pragma pack(1)
struct
{
  u8 ip[4];
  u8 own_ip[4];
  u8 mask[4];
  u8 gate[4];
} net;
#pragma pack()

const char info_string[] = "ESP32 SPI Emulator Unreal Speccy";

const char* ap[] =
{
  "Unreal Speccy WiFi"
};

u32 get_wscan(u8 *buf)
{
  int num = sizeof(ap) / sizeof(ap[0]);
  buf[0] = num;
  int ptr = 1;

  for (int i = 0; i < num; i++)
  {
    buf[ptr++] = i;                             // Auth type
    buf[ptr++] = -i * 10;                       // RSSI
    buf[ptr++] = i;                             // Channel
    buf[ptr++] = strlen(ap[i]);                 // SSID length
    memcpy(&buf[ptr], ap[i], strlen(ap[i]));    // SSID
    ptr += strlen(ap[i]);
  }

  return ptr;
}

void esp32_emul_init()
{
  calc_sin();

  regs[ESP_REG_STATUS] = ESP_ST_READY;
}

u8 esp32_emul(int d)
{
  u8 r = 0xFF;

  if (d == -1)  // SS de-asserted
  {
    // Execute a command on ~SS de-assert
    if ((state == ST_WR_REGS) && (cmd == ESP_SPI_CMD_WR_REGS))
    {
      int scmd = regs[ESP_REG_COMMAND];
      data_len = 0;
      int status = ESP_ST_READY;

      // printf("ESP32 Slave CMD: %02X\n", scmd);

      switch (scmd)
      {
        case ESP_CMD_GET_INFO:
        {
          memcpy(&regs[ESP_REG_DATA], info_string, sizeof(info_string));
          regs[ESP_REG_DATA_SIZE] = sizeof(info_string);
          error = ESP_ERR_NONE;
        }
        break;

        case ESP_CMD_GET_ERROR:
          regs[ESP_REG_ERROR] = error;
          error = ESP_ERR_NONE;
          break;

        case ESP_CMD_GET_NETSTATE:
          regs[ESP_REG_NETSTATE] = netstate;
          error = ESP_ERR_NONE;
          break;

        case ESP_CMD_AP_CONNECT:
        {
          if ((ap_name_len == strlen(ap[0])) && (!strncmp(ap_name, ap[0], ap_name_len)))
          {
            net.ip[0] = 0;
            net.ip[1] = 0;
            net.ip[2] = 0;
            net.ip[3] = 0;
            
            net.own_ip[0] = 192;
            net.own_ip[1] = 168;
            net.own_ip[2] = 0;
            net.own_ip[3] = 1;
            
            net.mask[0] = 255;
            net.mask[1] = 255;
            net.mask[2] = 255;
            net.mask[3] = 0;

            net.gate[0] = 192;
            net.gate[1] = 168;
            net.gate[2] = 0;
            net.gate[3] = 254;

            netstate = NETWORK_OPEN;
            error = ESP_ERR_NONE;
          }
          else
          {
            memset(&net, 0, sizeof(net));
            netstate = NETWORK_CLOSED;
            status = ESP_ST_ERROR;
            error = ESP_ERR_AP_NOT_CONN;
          }
        }
        break;

        case ESP_CMD_AP_DISCONNECT:
        {
          memset(&net, 0, sizeof(net));
          netstate = NETWORK_CLOSED;
          error = ESP_ERR_NONE;
        }
        break;

        case ESP_CMD_GET_IP:
        {
          memcpy(&regs[ESP_REG_IP], &net, sizeof(net));
          error = ESP_ERR_NONE;
        }
        break;

        case ESP_CMD_SET_AP_NAME:
        {
          int len = regs[ESP_REG_DATA_SIZE];
          
          if (len > sizeof(ap_name))
          {
            status = ESP_ST_ERROR;
            error = ESP_ERR_WRONG_PARAMETER;
          }
          else
          {
            memcpy(ap_name, &regs[ESP_REG_NAME], len);
            ap_name_len = len;
            error = ESP_ERR_NONE;
          }
        }
        break;

        case ESP_CMD_SET_AP_PWD:
        {
          int len = regs[ESP_REG_DATA_SIZE];
          
          if (len > sizeof(ap_pwd))
          {
            status = ESP_ST_ERROR;
            error = ESP_ERR_WRONG_PARAMETER;
          }
          else
          {
            memcpy(ap_pwd, &regs[ESP_REG_NAME], len);
            ap_pwd_len = len;
            error = ESP_ERR_NONE;
          }
        }
        break;

        case ESP_CMD_WSCAN:
        {
          data = data_buf;
          data_len = get_wscan(data);
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

        case ESP_CMD_DATA_END:
          error = ESP_ERR_NONE;
        break;

        case ESP_CMD_RESET:
          error = ESP_ERR_RESET;
          regs[ESP_REG_ERROR] = error;
        break;

        default:
          status = ESP_ST_ERROR;
          error = ESP_ERR_INVALID_COMMAND;
      }

      regs[ESP_REG_STATUS] = status;
      
      if (status == ESP_ST_ERROR)
        regs[ESP_REG_ERROR] = error;

      if (status == ESP_ST_DATA_S2M)
      {
        *(u32*)&regs[ESP_REG_DATA_SIZE] = data_len;
        data_ptr = 0;
      }
    }

    state = ST_IDLE;
    next_state = ST_IDLE;
  }
  
  // Transaction in progress
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

          case ESP_SPI_CMD_WR_DATA:
            break;

          case ESP_SPI_CMD_RD_DATA:
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
        else
          r = 0;
        break;

      case ST_DUMMY:
        if (!--dummy_cnt)
          state = next_state;

        break;
    }
  }

  return r;
}
