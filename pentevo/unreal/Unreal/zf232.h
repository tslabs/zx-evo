#pragma once
#include "sysdefs.h"

#define ROVBSIZE        512
#define WOVBSIZE        1024

// *BSIZE should be power of 2  and  above double OVBSIZE
#define RS_RXBSIZE      1024
#define RS_RXBMASK      (RS_RXBSIZE-1)
#define RS_TXBSIZE      1024
#define RS_TXBMASK      (RS_TXBSIZE-1)
#define ZF_RXBSIZE      1024
#define ZF_RXBMASK      (ZF_RXBSIZE-1)
#define ZF_TXBSIZE      1024
#define ZF_TXBMASK      (ZF_TXBSIZE-1)

// Supported layers of the ZiFi
#define ZF_LAYERS       0x01

struct ZF232
{
 HANDLE rs_hPort;
 HANDLE zf_hPort;
 OVERLAPPED rs_OvW;
 OVERLAPPED rs_OvR;
 OVERLAPPED zf_OvW;
 OVERLAPPED zf_OvR;
 u8 rs_reg[8];
 union
 {
  u8 rs_div[2];
  u16 rs_divfq;
 };
 u8 rs_open_port;
 u8 zf_open_port;
 u8 open_port;
 u8 selected_api_layer;
 u8 result_code;
 u8 select_zf;

 unsigned rs_rhead, rs_rtail, rs_whead, rs_wtail;
 unsigned zf_rhead, zf_rtail, zf_whead, zf_wtail;
 u8 rs_rcbuf[RS_RXBSIZE], rs_wbuf[RS_TXBSIZE];
 u8 zf_rcbuf[ZF_RXBSIZE], zf_wbuf[ZF_TXBSIZE];

 void rs_open(int port);
 void zf_open(int port);
 void rs_close();
 void zf_close();

 void io();

 void write(u8 nreg, u8 value);
 u8 read(u8 nreg);

 void setup_int();
};
