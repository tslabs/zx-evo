#pragma once

/* Supported layers of the ZiFi */
#define ZF_LAYERS       0x01

struct ZIFI
{
 HANDLE hPort;
 u8 open_port;
 u8 selected_api_layer;
 u8 result_code;
 u8 dummy;
 enum { BSIZE = 256 };
 unsigned rhead, rtail, whead, wtail;
 u8 rcbuf[BSIZE], wbuf[BSIZE];
 int open(int port);
 void close();
 void io();
 void write(u8 nreg, u8 value);
 u8 read(u8 nreg);
};
