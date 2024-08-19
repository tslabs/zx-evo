#pragma once

// Registers
enum
{
  ESP_REG_COMMAND     = 0x00,  // 1 byte
  ESP_REG_STATUS      = 0x01,  // 1 byte

  // String commands
  ESP_REG_STRING_TYPE = 0x02,  // 1 byte
  ESP_REG_STRING_SIZE = 0x02,  // 1 byte
  ESP_REG_STRING_DATA = 0x03,  // byte array

  // Object commands and data transmission commands
  ESP_REG_OBJ_TYPE    = 0x02,  // 1 byte
  ESP_REG_DATA_SIZE   = 0x03,  // 4 bytes
  ESP_REG_DATA_OFFSET = 0x07,  // 4 bytes
  ESP_REG_OBJ_HANDLE  = 0x0B,  // 1 byte

  // Lib commands
  ESP_REG_FUNC        = 0x02,  // 1 byte
  ESP_REG_OPT         = 0x03,  // 1 byte
  ESP_REG_ARG         = 0x03,  // 4 bytes
  ESP_REG_RETVAL      = 0x07,  // 4 bytes
  ESP_REG_ARR1_HANDLE = 0x0C,  // 1 byte
  ESP_REG_ARR2_HANDLE = 0x0D,  // 1 byte
  ESP_REG_ARR3_HANDLE = 0x0E,  // 1 byte
  ESP_REG_LIB_HANDLE  = 0x0F,  // 1 byte

  // Network commands
  ESP_REG_NETSTATE    = 0x02,
  ESP_REG_IP          = 0x2C, // 4 bytes
  ESP_REG_OWN_IP      = 0x30, // 4 bytes
  ESP_REG_MASK        = 0x34, // 4 bytes
  ESP_REG_GATE        = 0x38, // 4 bytes
  
  // Stats commands
  ESP_EXEC_TIME       = 0x3C, // 4 bytes
};

// Commands
enum
{
  ESP_CMD_NOP                   = 0x00, // (no parameters)

  ESP_CMD_GET_INFO_STR          = 0x01, // i: ESP_REG_STRING_TYPE
                                        // o: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA

  ESP_CMD_GET_API_VER           = 0x02, // +++
  
  ESP_CMD_GET_CHIP_INFO         = 0x03, // +++

  ESP_CMD_GET_NETSTATE          = 0x11, // o: ESP_REG_NETSTATE

  ESP_CMD_WSCAN                 = 0x12, // (no parameters)

  ESP_CMD_SET_AP_NAME           = 0x13, // i: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA

  ESP_CMD_SET_AP_PWD            = 0x14, // i: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA

  ESP_CMD_AP_CONNECT            = 0x16, // (no parameters)

  ESP_CMD_AP_DISCONNECT         = 0x17, // +++

  ESP_CMD_GET_IP                = 0x19, // o: ESP_REG_IP

  ESP_CMD_GET_XM_INFO           = 0xA0, // +++

  ESP_CMD_XM_INIT               = 0xA1, // i: ESP_REG_OBJ_HANDLE

  ESP_CMD_XM_SET_POS            = 0xA2, // +++

  ESP_CMD_XM_PLAY               = 0xA3, // i: ESP_REG_OBJ_HANDLE

  ESP_CMD_XM_STOP               = 0xA4, // (no parameters)

  ESP_CMD_XM_GET_STATS          = 0xA6, // +++

  ESP_CMD_XM_SET_VOLUME         = 0xA7, // +++

  ESP_CMD_XM_SET_S_RATE         = 0xA8, // +++

  ESP_CMD_XM_SET_PARAMS         = 0xA9, // +++

  ESP_CMD_GET_XM_STATE          = 0xAE, // +++

  ESP_CMD_GET_XM_STATE_CURRENT  = 0xAF, // +++

  ESP_CMD_LOAD_ELF              = 0xD0, // i: ESP_REG_OBJ_HANDLE
                                        // o: ESP_REG_LIB_HANDLE

  ESP_CMD_LOAD_ELF_OPT          = 0xD1, // i: ESP_REG_OBJ_HANDLE, ESP_REG_OPT
                                        // o: ESP_REG_LIB_HANDLE

  ESP_CMD_RUN_FUNC0             = 0xD2, // i: ESP_REG_LIB_HANDLE, ESP_REG_FUNC, ESP_REG_ARG,
                                        // o: ESP_REG_RETVAL

  ESP_CMD_RUN_FUNC1             = 0xD3, // i: ESP_REG_LIB_HANDLE, ESP_REG_FUNC, ESP_REG_ARG, ESP_REG_ARR1_HANDLE
                                        // o: ESP_REG_RETVAL

  ESP_CMD_RUN_FUNC2             = 0xD4, // i: ESP_REG_LIB_HANDLE, ESP_REG_FUNC, ESP_REG_ARG, ESP_REG_ARR1_HANDLE, ESP_REG_ARR2_HANDLE
                                        // o: ESP_REG_RETVAL

  ESP_CMD_RUN_FUNC3             = 0xD5, // i: ESP_REG_LIB_HANDLE, ESP_REG_FUNC, ESP_REG_ARG, ESP_REG_ARR1_HANDLE, ESP_REG_ARR2_HANDLE, ESP_REG_ARR3_HANDLE
                                        // o: ESP_REG_RETVAL

  ESP_CMD_MAKE_OBJECT           = 0xE0, // i: ESP_REG_DATA_SIZE, ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE

  ESP_CMD_WRITE_OBJECT          = 0xE1, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE

  ESP_CMD_READ_OBJECT           = 0xE2, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE

  ESP_CMD_DELETE_OBJECT         = 0xE3, // i: ESP_REG_OBJ_HANDLE

  ESP_CMD_KILL_OBJECTS          = 0xE4, // (no parameters)

  ESP_CMD_FILL_OBJECT           = 0xE5, // +++

  ESP_CMD_COPY_OBJECT           = 0xE6, // +++

  ESP_CMD_GET_OBJECT_INFO       = 0xE7, // +++

  ESP_CMD_RESET                 = 0xEE, // (no parameters)

  ESP_CMD_BREAK                 = 0xEF, // +++

  ESP_CMD_GET_RND               = 0xF0, // i: ESP_REG_DATA_SIZE

  ESP_CMD_DEHST                 = 0xF1, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE, ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
                                        
  ESP_CMD_UNZIP                 = 0xF2, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE, ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
};

enum
{
  ESP_OPT_DATA_SRAM     = 0x01,
  ESP_OPT_RODATA_SRAM   = 0x02,
  ESP_OPT_BSS_SRAM      = 0x04,
};

enum
{
  // Status codes
  ESP_ST_IDLE     = 0x00, // Idle
  ESP_ST_READY    = 0x01, // Command completed
  ESP_ST_BUSY     = 0x02, // Busy executing command or initializing
  ESP_ST_DATA_M2S = 0x03, // Ready to receive data from Master to Slave
  ESP_ST_DATA_S2M = 0x04, // Ready to send data from Slave to Master
  ESP_ST_RESET    = 0x7E, // Reset performed, ready

  // Error codes
  ESP_ERR_INV_COMMAND      = 0x80,
  ESP_ERR_INV_PARAM        ,
  ESP_ERR_INV_STATE        ,
  ESP_ERR_INV_STR_LEN      ,
  ESP_ERR_INV_SIZE         ,
  ESP_ERR_INV_OBJ_TYPE     ,
  ESP_ERR_INV_ARR_NUM      ,
  ESP_ERR_INV_HANDLE       ,
  ESP_ERR_INV_XM_HANDLE    ,
  ESP_ERR_INV_LIB_HANDLE   ,
  ESP_ERR_INV_ELF_HANDLE   ,
  ESP_ERR_INV_HST_HANDLE   ,
  ESP_ERR_INV_BSS_HANDLE   ,
  ESP_ERR_INV_ARG_HANDLE   ,
  ESP_ERR_INV_SRC_HANDLE   ,
  ESP_ERR_INV_DST_HANDLE   ,

  ESP_ERR_INV_XM           = 0x90,
  ESP_ERR_INV_LIB          ,
  ESP_ERR_INV_ELF          ,
  ESP_ERR_INV_HST          ,
  ESP_ERR_INV_ZIP          ,

  ESP_ERR_OUT_OF_MEMORY    = 0xA0,
  ESP_ERR_OUT_OF_HANDLES   ,
  ESP_ERR_OBJ_NOT_DELETED  ,

  ESP_ERR_AP_NOT_CONNECTED = 0xB0,
  ESP_ERR_NET_BUSY         ,
};

// Info type (ESP_CMD_GET_INFO)
enum
{
  GET_INFO_COPYRIGHT = 0x00,
  GET_INFO_BUILD     = 0x01,
};

// Memory object types (add new types to mem_obj.cpp)
enum
{
  OBJ_TYPE_NONE = 0,
  OBJ_TYPE_DATA,      // SPIRAM
  OBJ_TYPE_DATAF,     // SRAM
  OBJ_TYPE_ELF,       // SPIRAM
  OBJ_TYPE_LIB,       // Virtual
  OBJ_TYPE_XM,        // SPIRAM
  OBJ_TYPE_WAV,       // SPIRAM
  OBJ_TYPE_HST,       // SPIRAM
  OBJ_TYPE_ZIP,       // SPIRAM
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

// SPI Slave Commands (ESP32)
enum
{
  ESP_SPI_CMD_WR_REGS = 0x01,
  ESP_SPI_CMD_RD_REGS = 0x02,
  ESP_SPI_CMD_WR_DATA = 0x03,
  ESP_SPI_CMD_RD_DATA = 0x04,
  ESP_SPI_CMD_W_END   = 0x07,
  ESP_SPI_CMD_R_END   = 0x08,
};

enum
{
  OBJ_ST_NONE       = 0,

  LIB_OBJ_ST_READY  = 0x10,   // Library loaded

  XM_OBJ_ST_STOPPED = 0x20,   // Object context created, idling
  XM_OBJ_ST_PLAYING,          // Object playing

  OBJ_ST_ERROR      = 0xFF    // Reserved
};
