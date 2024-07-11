#pragma once

// Registers
enum
{
  ESP_REG_EXT_STAT  = 0,
  ESP_REG_DATA_SIZE = 4,
  
  ESP_REG_COMMAND   = 8,
  ESP_REG_STATUS    = 9,
  ESP_REG_PARAMS    = 10
};

// Status codes
enum
{
  ESP_ST_IDLE     = 0x00, // Success/Idle
  ESP_ST_ERROR    = 0x01, // Error (see. Get last error)
  ESP_ST_READY    = 0x02, // Slave is ready
  ESP_ST_BUSY     = 0x03, // Busy executing command or initializing
  ESP_ST_DATA_M2S = 0x04, // Ready to receive data from Master to Slave
  ESP_ST_DATA_S2M = 0x05  // Ready to send data from Slave to Master
};

// SPI Commands
enum
{
  ESP_SPI_CMD_WR_REGS = 0x01,
  ESP_SPI_CMD_RD_REGS = 0x02,
  ESP_SPI_CMD_WR_DMA  = 0x03,
  ESP_SPI_CMD_RD_DMA  = 0x04
};

// Commands
enum
{
  ESP_CMD_NOP       = 0x00,
  ESP_CMD_GET_INFO  = 0x01,
  ESP_CMD_BREAK     = 0x02,
  ESP_CMD_DMA_END   = 0xD0,
  ESP_CMD_RESET     = 0xEE,
  ESP_CMD_GET_RND   = 0xF0,
  ESP_CMD_TEST2     = 0xF1,
  ESP_CMD_TEST3     = 0xF2
};

// Extended statuses / errors
enum
{
  ESP_ERR_RESET = 0xFE
};

#define _delay_ms(a) vTaskDelay(pdMS_TO_TICKS(a));