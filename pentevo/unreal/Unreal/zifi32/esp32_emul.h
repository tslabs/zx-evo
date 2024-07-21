
#pragma once

/// <spi_slave.h>

// Registers
enum
{
  ESP_REG_COMMAND = 0,
  ESP_REG_STATUS = 1,
  ESP_REG_ERROR = 2,
  ESP_REG_PARAMS = 2,

  ESP_REG_DATA_SIZE = ESP_REG_PARAMS,
  ESP_REG_DATA = ESP_REG_PARAMS + 1,
  ESP_REG_NAME = ESP_REG_PARAMS + 1,

  ESP_REG_NETSTATE = ESP_REG_PARAMS,

  ESP_REG_IP = ESP_REG_PARAMS,
  ESP_REG_OWN_IP = ESP_REG_PARAMS + 4,
  ESP_REG_MASK = ESP_REG_PARAMS + 8,
  ESP_REG_GATE = ESP_REG_PARAMS + 12,
};

// Commands
enum
{
  ESP_CMD_NOP           = 0x00,
  ESP_CMD_GET_ERROR     = 0x01,
  ESP_CMD_GET_INFO      = 0x02,
                        
  ESP_CMD_GET_NETSTATE  = 0x11,
  ESP_CMD_WSCAN         = 0x12,
  ESP_CMD_SET_AP_NAME   = 0x13,
  ESP_CMD_SET_AP_PWD    = 0x14,
  ESP_CMD_AP_CONNECT    = 0x16,
  ESP_CMD_AP_DISCONNECT = 0x17,
  ESP_CMD_GET_IP        = 0x19,

  ESP_CMD_DATA_END      = 0xD0,
  ESP_CMD_BREAK         = 0xE0,
  ESP_CMD_RESET         = 0xEE,
                        
  ESP_CMD_GET_RND       = 0xF0,
  ESP_CMD_TEST2         = 0xF1,
  ESP_CMD_TEST3         = 0xF2,
};

// Status codes
enum
{
  ESP_ST_IDLE     = 0x00, // Success/Idle
  ESP_ST_BUSY     = 0x01, // Busy executing command or initializing
  ESP_ST_READY    = 0x02, // Slave is ready
  ESP_ST_ERROR    = 0x03, // Error (see. Get last error)
  ESP_ST_DATA_M2S = 0x04, // Ready to receive data from Master to Slave
  ESP_ST_DATA_S2M = 0x05  // Ready to send data from Slave to Master
};

// Errors
enum
{
  ESP_ERR_NONE             = 0x00,
  ESP_ERR_WRONG_PARAMETER  = 0x01,
  ESP_ERR_INVALID_COMMAND  = 0x02,
  ESP_ERR_WRONG_STATE      = 0x03,
  ESP_ERR_AP_NOT_CONN      = 0x10,
  ESP_ERR_RESET            = 0xFE
};

// Network state
enum
{
  NETWORK_CLOSED  = 0x00,
  NETWORK_OPENING = 0x01,
  NETWORK_OPEN    = 0x02,
  NETWORK_CLOSING = 0x03,
  NETWORK_UNKNOWN = 0xFF
};

// SPI Commands
enum
{
  ESP_SPI_CMD_WR_REGS = 0x01,
  ESP_SPI_CMD_RD_REGS = 0x02,
  ESP_SPI_CMD_WR_DATA = 0x03,
  ESP_SPI_CMD_RD_DATA = 0x04
};

// Functions
void esp32_emul_init();
u8 esp32_emul(int d);
