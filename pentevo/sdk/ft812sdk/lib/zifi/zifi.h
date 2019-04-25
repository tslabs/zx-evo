
#pragma once

#define ZF_LOW    0xEF

enum
{
  ZF_DR     = 0x00,
  ZF_ZIFR   = 0xC0,
  ZF_ZOFR   = 0xC1,
  ZF_RIFR   = 0xC2,
  ZF_ROFR   = 0xC3,
  ZF_IMR    = 0xC4,
  ZF_ISR    = 0xC4,
  ZF_ZIBTR  = 0xC5,
  ZF_ZITOR  = 0xC6,
  ZF_CR     = 0xC7,
  ZF_ER     = 0xC7,
  ZF_RIBTR  = 0xC8,
  ZF_RITOR  = 0xC9
};

enum
{
  ZF_CMD_CLRFIFO_IN   = 0x01,
  ZF_CMD_CLRFIFO_OUT  = 0x02,
  RS_CMD_CLRFIFO_IN   = 0x05,
  RS_CMD_CLRFIFO_OUT  = 0x06,
  ZF_CMD_SETAPI_OFF   = 0xF0,
  ZF_CMD_SETAPI_1     = 0xF1,
  ZF_CMD_GETVER       = 0xFF
};

enum
{
  ZF_RES_OK = 0x00
};

enum
{
  ZF_IMR_IBT  = 0x01,
  ZF_IMR_ITO  = 0x02,
  RS_IMR_IBT  = 0x04,
  RS_IMR_ITO  = 0x08
};

enum
{
  ZF_ERR_OK,
  ZF_ERR_OVF
};

void zifi_wait(u32);
// void zifi_rd_fifo_dma(void*, u8);

u8 zifi_rd(u8);                                   // args: register number
                                                  // ret:  register read value

void zifi_wr(u8, u8);                             // args: register number
                                                  //       register write value

bool zifi_init();                                 // ret:  command result (0 - error, 1 - success)

void zifi_set_recv_threshold(u8);                 // args: threshold value in bytes

void zifi_set_recv_timeout(u8);                   // args: timeout value in milliseconds

u8 zifi_input_buffer_used();                      // ret:  number of bytes available for the first read burst, actual avalaible data size in the input buffer may be larger

u8 zifi_output_buffer_free();                     // ret:  number of bytes available for the first write burst, actual available space size in the output buffer may be larger

void zifi_rd_fifo(void*, u8);                     // args: pointer to store received data
                                                  //       max number of bytes to receive

u16 zifi_receive(void*, u16);                     // args: pointer to store received data
                                                  //       max number of bytes to receive
                                                  // ret:  number of bytes received

void zifi_send(void*, u16);                       // args: pointer to store received data
                                                  //       number of bytes to send

bool zifi_connect_ap(const char*, const char*);   // args: AP name (ASCII)
                                                  //       AP password (ASCII)
                                                  // ret:  command result (0 - error, 1 - success)

bool zifi_disconnect_ap();                        // ret:  command result (0 - error, 1 - success)

bool zifi_is_connected_ap();

bool zifi_connect_server(const char*);            // args: server name of IP (ASCII)
                                                  // ret:  command result (0 - error, 1 - success)

bool zifi_disconnect_server();                    // ret:  command result (0 - error, 1 - success)

bool zifi_is_connected_server();

bool zifi_get_fw_version(char*);                  // args: buffer for received data
                                                  // ret:  command result (0 - error, 1 - success)

bool zifi_get_ap_list();                          // TBD
