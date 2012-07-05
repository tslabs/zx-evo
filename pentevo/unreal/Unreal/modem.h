#pragma once

struct ISA_MODEM
{
   HANDLE hPort;
   unsigned char reg[8];
   union {
      unsigned char div[2];
      unsigned short divfq;
   };
   unsigned char open_port;
   unsigned char align;

   enum { BSIZE = 1024 }; // should be power of 2
   unsigned rhead, rtail, whead, wtail;
   unsigned char rcbuf[BSIZE], wbuf[BSIZE];

   void open(int port);
   void close();
   void io();

   void write(unsigned nreg, unsigned char value);
   unsigned char read(unsigned nreg);
   void setup_int();
};
