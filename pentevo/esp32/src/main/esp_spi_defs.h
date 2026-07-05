#pragma once

#define ZIFI_STR_HELPER(x) #x
#define ZIFI_STR(x) ZIFI_STR_HELPER(x)

#define PROD_VER0 0
#define PROD_VER1 6
#define PROD_VER2 4
#define PROD_VER_STRING ZIFI_STR(PROD_VER0) "." ZIFI_STR(PROD_VER1) "." ZIFI_STR(PROD_VER2)
#define CP_STRING "ESP32 SPI WiFi Module, ver." PROD_VER_STRING ", (c) TS-Labs"

#define API_VER 1
#define FEAT_VER 10
#define API_VER_STRING ZIFI_STR(API_VER)
#define FEAT_VER_STRING ZIFI_STR(FEAT_VER)
#define API_STRING "API version: " API_VER_STRING ", feature version: " FEAT_VER_STRING

// Registers
enum
{
  ESP_REG_COMMAND     = 0x00,  // 1 byte
  ESP_REG_STATUS      = 0x01,  // 1 byte

  // String commands
  ESP_REG_STRING_TYPE = 0x02,  // 1 byte
  ESP_REG_STRING_SIZE = 0x02,  // 1 byte
  ESP_REG_STRING_DATA = 0x03,  // byte array
  
  // Version commands
  ESP_REG_API         = 0x02,  // 1 byte
  ESP_REG_FEAT        = 0x03,  // 2 bytes
  ESP_REG_VER0        = 0x05,  // 1 byte
  ESP_REG_VER1        = 0x06,  // 1 byte
  ESP_REG_VER2        = 0x07,  // 1 byte

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

  // SFX commands
  ESP_REG_SFX_CHANNEL        = 0x02, // 1 byte
  ESP_REG_SFX_GROUP          = 0x03, // 1 byte
  ESP_REG_SFX_VOLUME         = 0x04, // 1 byte
  ESP_REG_SFX_PAN            = 0x05, // 1 byte
  ESP_REG_SFX_PITCH          = 0x06, // 2 bytes, Q4.8 little-endian

  ESP_REG_SFX_STATE_ACTIVE   = 0x10, // 1 byte
  ESP_REG_SFX_STATE_HANDLE   = 0x11, // 1 byte
  ESP_REG_SFX_STATE_GROUP    = 0x12, // 1 byte
  ESP_REG_SFX_STATE_ORDER    = 0x13, // 1 byte
  ESP_REG_SFX_STATE_VOLUME   = 0x14, // 1 byte
  ESP_REG_SFX_STATE_PAN      = 0x15, // 1 byte
  ESP_REG_SFX_STATE_PITCH    = 0x16, // 2 bytes, Q4.8 little-endian
  ESP_REG_SFX_STATE_RATE     = 0x18, // 4 bytes
  ESP_REG_SFX_STATE_FRAMES   = 0x1C, // 4 bytes
  ESP_REG_SFX_STATE_POSITION = 0x20, // 4 bytes
  ESP_REG_SFX_STATE_BITS     = 0x24, // 2 bytes
  ESP_REG_SFX_STATE_CHANNELS = 0x26, // 1 byte
  ESP_REG_SFX_STATE_SIGNED   = 0x27, // 1 byte
};

// Commands
enum
{
  ESP_CMD_NOP                   = 0x00, // (no parameters)
  ESP_CMD_GET_INFO_STR          = 0x01, // i: ESP_REG_STRING_TYPE
                                        // o: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA
  ESP_CMD_GET_VER               = 0x02, // o: ESP_REG_API, ESP_REG_FEAT, ESP_REG_VER0..2
  ESP_CMD_GET_CHIP_INFO         = 0x03, // +++

  ESP_CMD_GET_NETSTATE          = 0x11, // o: ESP_REG_NETSTATE
  ESP_CMD_WSCAN                 = 0x12, // (no parameters)
  ESP_CMD_SET_AP_NAME           = 0x13, // i: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA
  ESP_CMD_SET_AP_PWD            = 0x14, // i: ESP_REG_STRING_SIZE, ESP_REG_STRING_DATA
  ESP_CMD_AP_CONNECT            = 0x16, // (no parameters)
  ESP_CMD_AP_DISCONNECT         = 0x17, // (no parameters)
  ESP_CMD_SET_URL               = 0x18, // (no parameters)
  ESP_CMD_GET_IP                = 0x19, // o: ESP_REG_IP
  ESP_CMD_HTTP_GET              = 0x20, // +++
  ESP_CMD_HTTPS_GET             = 0x21, // (no parameters)
                                        // o: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE
  ESP_CMD_GOPHER_GET            = 0x22, // (no parameters)
                                        // o: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE
  ESP_CMD_HTTP_STREAM_START     = 0x23, // (no parameters)
                                        // o: ESP_REG_DATA_SIZE (total size)
  ESP_CMD_HTTPS_STREAM_START    = 0x24, // (no parameters)
                                        // o: ESP_REG_DATA_SIZE (total size)
  ESP_CMD_GOPHER_STREAM_START   = 0x25, // (no parameters)
                                        // o: ESP_REG_DATA_SIZE (total size)
  ESP_CMD_STREAM_READ           = 0x26, // (no parameters)
                                        // o: ESP_REG_DATA_SIZE (chunk size)
                                        // (DMA read like ESP_CMD_READ_OBJECT)
  ESP_CMD_STREAM_CLOSE          = 0x27, // (no parameters)

  ESP_CMD_TRACK_INIT            = 0xA0, // i: ESP_REG_OBJ_HANDLE
  ESP_CMD_TRACK_PLAY            = 0xA1, // i: ESP_REG_OBJ_HANDLE
  ESP_CMD_TRACK_STOP            = 0xA2, // (no parameters)
  ESP_CMD_TRACK_RESET           = 0xA3, // i: ESP_REG_OBJ_HANDLE, stop playback and reset XMC context to start
  ESP_CMD_TRACK_SET_VOLUME      = 0xA4, // +++
  ESP_CMD_TRACK_SET_S_RATE      = 0xA5, // +++
  ESP_CMD_TRACK_SET_PARAMS      = 0xA6, // +++
  ESP_CMD_TRACK_SET_POS         = 0xA7, // +++
  ESP_CMD_TRACK_GET_INFO        = 0xA8, // +++
  ESP_CMD_TRACK_GET_STATE       = 0xA9, // +++
  ESP_CMD_TRACK_GET_STATS       = 0xAA, // +++

  ESP_CMD_XM_STREAM_LOAD        = 0xAB, // i: ESP_REG_DATA_SIZE (tx chunk, aligned), ESP_REG_DATA_OFFSET (total on first chunk, 0 on next chunks)
                                        // o: ESP_REG_DATA_OFFSET (chunk offset), ESP_REG_DATA_SIZE (chunk size), ESP_REG_OBJ_HANDLE
  ESP_CMD_MOD_STREAM_LOAD       = 0xAC, // i: ESP_REG_DATA_SIZE (tx chunk, aligned), ESP_REG_DATA_OFFSET (total on first chunk, 0 on next chunks)
                                        // o: ESP_REG_DATA_OFFSET (chunk offset), ESP_REG_DATA_SIZE (chunk size), ESP_REG_OBJ_HANDLE
  ESP_CMD_S3M_STREAM_LOAD       = 0xAD, // i: ESP_REG_DATA_SIZE (tx chunk, aligned), ESP_REG_DATA_OFFSET (total on first chunk, 0 on next chunks)
                                        // o: ESP_REG_DATA_OFFSET (chunk offset), ESP_REG_DATA_SIZE (chunk size), ESP_REG_OBJ_HANDLE

  ESP_CMD_SFX_PLAY              = 0xB0, // i: ESP_REG_OBJ_HANDLE, ESP_REG_SFX_GROUP; o: ESP_REG_SFX_CHANNEL
  ESP_CMD_SFX_PLAY_EX           = 0xB1, // i: ESP_REG_OBJ_HANDLE, ESP_REG_SFX_GROUP, ESP_REG_SFX_VOLUME, ESP_REG_SFX_PAN, ESP_REG_SFX_PITCH; o: ESP_REG_SFX_CHANNEL
  ESP_CMD_SFX_STOP              = 0xB2, // i: ESP_REG_SFX_CHANNEL
  ESP_CMD_SFX_SET_PARAMS        = 0xB3, // i: ESP_REG_SFX_CHANNEL, ESP_REG_SFX_VOLUME, ESP_REG_SFX_PAN, ESP_REG_SFX_PITCH
  ESP_CMD_SFX_SET_VOLUME        = 0xB4, // i: ESP_REG_SFX_VOLUME
  ESP_CMD_SFX_GET_STATE         = 0xB5, // i: ESP_REG_SFX_CHANNEL; o: ESP_REG_SFX_STATE_*
  ESP_CMD_SFX_STOP_GROUP        = 0xB6, // i: ESP_REG_SFX_GROUP

  ESP_CMD_CONFIG_SPI            = 0xC0, // +++

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
  ESP_CMD_REBOOT                = 0xED, // (no parameters)
  ESP_CMD_RESET                 = 0xEE, // (no parameters)
  ESP_CMD_BREAK                 = 0xEF, // abort active host tracker stream state, READY if idle

  ESP_CMD_GET_RND               = 0xF0, // i: ESP_REG_DATA_SIZE
  ESP_CMD_DEHST                 = 0xF1, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE, ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
  ESP_CMD_UNZIP                 = 0xF2, // i: ESP_REG_OBJ_HANDLE, ESP_REG_DATA_SIZE, ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_DATA_OFFSET, ESP_REG_DATA_SIZE
  ESP_CMD_STREAM_UNZIP          = 0xF3, // i: ESP_REG_OBJ_TYPE
                                        // o: ESP_REG_OBJ_HANDLE
};

enum
{
  ESP_OPT_DATA_SRAM     = 0x01,
  ESP_OPT_RODATA_SRAM   = 0x02,
  ESP_OPT_BSS_SRAM      = 0x04,

  ESP_OPT_SPI_1BIT      = 0x00,
  ESP_OPT_SPI_2BIT      = 0x40,
  ESP_OPT_SPI_4BIT      = 0x80,
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
  ESP_ERR_INV_SIZE         , // 84
  ESP_ERR_INV_OBJ_TYPE     ,
  ESP_ERR_INV_ARR_NUM      ,
  ESP_ERR_INV_HANDLE       ,
  ESP_ERR_INV_XM_HANDLE    , // 88
  ESP_ERR_INV_LIB_HANDLE   ,
  ESP_ERR_INV_ELF_HANDLE   ,
  ESP_ERR_INV_HST_HANDLE   ,
  ESP_ERR_INV_BSS_HANDLE   , // 8C
  ESP_ERR_INV_ARG_HANDLE   ,
  ESP_ERR_INV_SRC_HANDLE   ,
  ESP_ERR_INV_DST_HANDLE   ,
  ESP_ERR_INV_XM           , // 90,
  ESP_ERR_INV_LIB          ,
  ESP_ERR_INV_ELF          ,
  ESP_ERR_INV_HST          ,
  ESP_ERR_INV_ZIP          , // 94
  ESP_ERR_INV_MOD          ,
  ESP_ERR_INV_MOD_HANDLE   ,
  ESP_ERR_INV_S3M          ,
  ESP_ERR_INV_S3M_HANDLE   , // 98
  ESP_ERR_OUT_OF_MEMORY    ,
  ESP_ERR_OUT_OF_HANDLES   ,
  ESP_ERR_OBJ_NOT_DELETED  ,

  ESP_ERR_AP_NOT_CONNECTED = 0xB0,
  ESP_ERR_NET_BUSY         ,
};

// Info type (ESP_CMD_GET_INFO_STR)
enum
{
  GET_INFO_COPYRIGHT = 0x00,
  GET_INFO_API       = 0x01,
  GET_INFO_BUILD     = 0x02,
  GET_INFO_IDF       = 0x03,
};

// Memory object types (add new types to mem_obj.cpp)
enum
{
  OBJ_TYPE_NONE  = 0,
  OBJ_TYPE_DATA  = 1,  // SPIRAM
  OBJ_TYPE_DATAF = 2,  // SRAM
  OBJ_TYPE_ELF   = 3,  // SPIRAM
  OBJ_TYPE_LIB   = 4,  // Virtual
  OBJ_TYPE_XM    = 5,  // SPIRAM
  OBJ_TYPE_WAV   = 6,  // SPIRAM
  OBJ_TYPE_HST   = 7,  // SPIRAM
  OBJ_TYPE_ZIP   = 8,  // SPIRAM
  OBJ_TYPE_XMC   = 9,  // SPIRAM
  OBJ_TYPE_MOD   = 10, // SPIRAM
  OBJ_TYPE_MDC   = 11, // SPIRAM
  OBJ_TYPE_S3M   = 12, // SPIRAM
  OBJ_TYPE_S3C   = 13, // SPIRAM
  OBJ_TYPE_MBC   = 14, // SPIRAM
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

  TRACK_OBJ_ST_STOPPED = 0x20,   // Tracker object context created, idling
  TRACK_OBJ_ST_PLAYING = 0x21,   // Tracker object playing

  WAV_OBJ_ST_PLAYING = 0x30,  // WAV object is used by SFX renderer

  OBJ_ST_DELETING   = 0xFE,   // Object is being deleted, new users must not attach
  OBJ_ST_ERROR      = 0xFF    // Reserved
};
