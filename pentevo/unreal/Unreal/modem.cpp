#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

void ISA_MODEM::open(int port)
{
   if (open_port == port) return;
   whead = wtail = rhead = rtail = 0;
   if (hPort && hPort != INVALID_HANDLE_VALUE)
       CloseHandle(hPort);
   open_port = port;
   if (!port) return;

   char portName[11];
   _snprintf(portName, _countof(portName), "\\\\.\\COM%d", port);

   hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
   if (hPort == INVALID_HANDLE_VALUE)
   {
      errmsg("can't open modem on %s", portName); err_win32();
      conf.modem_port = open_port = 0; return;
   }

   COMMTIMEOUTS times;
   times.ReadIntervalTimeout = MAXDWORD;
   times.ReadTotalTimeoutMultiplier = 0;
   times.ReadTotalTimeoutConstant = 0;
   times.WriteTotalTimeoutMultiplier = 0;
   times.WriteTotalTimeoutConstant = 0;
   SetCommTimeouts(hPort, &times);

#if 0
   DCB dcb;
   if (GetCommState(hPort, &dcb)) {
      printf(
       "modem state:\n"
       "rate=%d\n"
       "parity=%d, OutxCtsFlow=%d, OutxDsrFlow=%d, DtrControl=%d, DsrSensitivity=%d\n"
       "TXContinueOnXoff=%d, OutX=%d, InX=%d, ErrorChar=%d\n"
       "Null=%d, RtsControl=%d, AbortOnError=%d, XonLim=%d, XoffLim=%d\n"
       "ByteSize=%d, Parity=%d, StopBits=%d\n"
       "XonChar=#%02X, XoffChar=#%02X, ErrorChar=#%02X, EofChar=#%02X, EvtChar=#%02X\n\n",
       dcb.BaudRate,
       dcb.fParity, dcb.fOutxCtsFlow, dcb.fOutxDsrFlow, dcb.fDtrControl, dcb.fDsrSensitivity,
       dcb.fTXContinueOnXoff, dcb.fOutX, dcb.fInX, dcb.fErrorChar,
       dcb.fNull, dcb.fRtsControl, dcb.fAbortOnError, dcb.XonLim, dcb.XoffLim,
       dcb.ByteSize, dcb.Parity, dcb.StopBits,
       (BYTE)dcb.XonChar, (BYTE)dcb.XoffChar, (BYTE)dcb.ErrorChar, (BYTE)dcb.EofChar, (BYTE)dcb.EvtChar);
   }
#endif
}

void ISA_MODEM::close()
{
   if (!hPort || hPort == INVALID_HANDLE_VALUE)
       return;
   CloseHandle(hPort);
   hPort = INVALID_HANDLE_VALUE;
   open_port = 0;
}

void ISA_MODEM::io()
{
   if (!hPort || hPort == INVALID_HANDLE_VALUE)
       return;
   unsigned char temp[BSIZE];

   int needwrite = whead - wtail;
   if (needwrite < 0)
       needwrite += BSIZE;
   if (needwrite)
   {
      if (whead > wtail)
          memcpy(temp, wbuf+wtail, needwrite);
      else
      {
          memcpy(temp, wbuf+wtail, BSIZE-wtail);
          memcpy(temp+BSIZE-wtail, wbuf, whead);
      }

      DWORD written = 0;
      if (WriteFile(hPort, temp, needwrite, &written, 0))
      {
//printf("\nsend: "); dump1(temp, written);
         wtail = (wtail+written) & (BSIZE-1);
      }
   }
   if (((whead+1) & (BSIZE-1)) != wtail) reg[5] |= 0x60;

   int canread = rtail - rhead - 1;
   if (canread < 0)
       canread += BSIZE;
   if (canread)
   {
      DWORD read = 0;
      if (ReadFile(hPort, temp, canread, &read, 0) && read)
      {
         for (unsigned i = 0; i < read; i++)
             rcbuf[rhead++] = temp[i], rhead &= (BSIZE-1);
//printf("\nrecv: "); dump1(temp, read);
      }
   }
   if (rhead != rtail)
       reg[5] |= 1;

   setup_int();
}

void ISA_MODEM::setup_int()
{
   reg[6] &= ~0x10;

   unsigned char mask = reg[5] & 1;
   if (reg[5] & 0x20) mask |= 2, reg[6] |= 0x10;
   if (reg[5] & 0x1E) mask |= 4;
   // if (mask & reg[1]) cpu.nmi()

   if (mask & 4) reg[2] = 6;
   else if (mask & 1) reg[2] = 4;
   else if (mask & 2) reg[2] = 2;
   else if (mask & 8) reg[2] = 0;
   else reg[2] = 1;
}

void ISA_MODEM::write(unsigned nreg, unsigned char value)
{
   DCB dcb;

   if ((1<<nreg) & ((1<<2)|(1<<5)|(1<<6))) return; // R/O registers

   if (nreg < 2 && (reg[3] & 0x80))
   {
     div[nreg] = value;
     if (GetCommState(hPort, &dcb))
     {
       if (!divfq) divfq = 1;
       dcb.BaudRate = 115200 / divfq;
       SetCommState(hPort, &dcb);
     }
     return;
   }

   if (nreg == 0)
   { // write char to output buffer
      reg[5] &= ~0x60;
      if (((whead+1) & (BSIZE-1)) == wtail)
      {
         reg[5] |= 2;
      }
      else
      {
         wbuf[whead++] = value, whead &= (BSIZE-1);
         if (((whead+1) & (BSIZE-1)) != wtail) reg[5] |= 0x60;
      }
      setup_int();
      return;
   }

   reg[nreg] = value;

   // Thu 28 Jul 2005. transfer mode control (code by Alex/AT)

   if (nreg == 3)
   {
      // LCR set, renew modem config
      if (!GetCommState(hPort, &dcb)) return;

      dcb.fBinary = TRUE;
      dcb.fParity = (reg[3] & 8)? TRUE : FALSE;
//		dcb.fParity = FALSE; // fix suggested by lvd
      dcb.fOutxCtsFlow = FALSE;
      dcb.fOutxDsrFlow = FALSE;
      dcb.fDtrControl = DTR_CONTROL_DISABLE;
      dcb.fDsrSensitivity = FALSE;
      dcb.fTXContinueOnXoff = FALSE;
      dcb.fOutX = FALSE;
      dcb.fInX = FALSE;
      dcb.fErrorChar = FALSE;
      dcb.fNull = FALSE;
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
      dcb.fAbortOnError = FALSE;
//      dcb.ByteSize = 6 + (reg[3] & 2) - (reg[3] & 1);
//		dcb.ByteSize = 8; // fix suggested by lvd
      dcb.ByteSize = 5 + (reg[3] & 3); // fix by Deathsoft

      static const BYTE parity[] = { ODDPARITY, EVENPARITY, MARKPARITY, SPACEPARITY };
      dcb.Parity = (reg[3] & 8)? parity[(reg[3]>>4) & 3] : NOPARITY;

      if (!(reg[3] & 4)) dcb.StopBits = ONESTOPBIT;
      else dcb.StopBits = ((reg[3] & 3) == 1)? ONE5STOPBITS : TWOSTOPBITS;

      SetCommState(hPort, &dcb);
      return;
   }

   if (nreg == 4)
   {
      // MCR set, renew DTR/RTS
      EscapeCommFunction(hPort, (reg[4] & 1)? SETDTR : CLRDTR);
      EscapeCommFunction(hPort, (reg[4] & 2)? SETRTS : CLRRTS);
   }
}

unsigned char ISA_MODEM::read(unsigned nreg)
{
   if (nreg < 2 && (reg[3] & 0x80)) return div[nreg];

   unsigned char result = reg[nreg];

   if (nreg == 0)
   { // read char from buffer
      if (conf.mem_model == MM_ATM3)
          result = 0;
      
      if (rhead != rtail)
      {
           result = reg[0] = rcbuf[rtail++];
           rtail &= (BSIZE-1);
      }

      if (rhead != rtail)
          reg[5] |= 1;
      else
          reg[5] &= ~1;

      setup_int();
   }

   if (nreg == 5)
   {
       reg[5] &= ~0x0E;
       setup_int();
   }

   if (nreg == 6)
   {
       DWORD ModemStatus;
       GetCommModemStatus(hPort, &ModemStatus);
       u8 r6 = reg[6];
       reg[6] &= ~(1 << 4);
       reg[6] |= (ModemStatus & MS_CTS_ON) ? (1 << 4): 0;
       reg[6] &= ~1;
       reg[6] |= ((r6 ^ reg[6]) & (1 << 4)) >> 4;
       result = reg[6];
   }
   return result;
}
