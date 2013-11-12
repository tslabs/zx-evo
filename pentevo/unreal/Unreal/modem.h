#pragma once

struct ISA_MODEM
{
   HANDLE hPort;
   u8 reg[8];
   union {
      u8 div[2];
      u16 divfq;
   };
   u8 open_port;
   u8 align;

   enum { BSIZE = 1024 }; // should be power of 2
   unsigned rhead, rtail, whead, wtail;
   u8 rcbuf[BSIZE], wbuf[BSIZE];

   void open(int port);
   void close();
   void io();

   void write(unsigned nreg, u8 value);
   u8 read(unsigned nreg);
   void setup_int();
};
