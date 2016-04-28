
/*
"Kondratyev's" modem/RS-232
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Реализован обычный "Кондратьевский" модем/RS-232 и
его расширение: FIFO на портах ZiFi

ZiFi API layer 1
~~~~~~~~~~~~~~~~
Реализован API уровень 1  -  мост  ZiFi<==>COM-port

--------------------------------------------------------------------------------
About ZiFi's API:
АПИ использует 3 уровня абстракции:
- уровень 1: прямая работа через уарт с модулем вифи
             - программер берет на себя все тяготы обслуживания ESP8266,
             а именно конфигурация вифи, контроль подключений,
             формирование http запросов при помощи АТ-команд и т.д.
- уровень 2: TCP - программеру предоставляется возможность подключения
             к айпи/урлу и обмен сырым TCP трафиком с сервером.
             Возможна реализация клиентов типа ИРЦ.
- уровень 3: HTTP - программер формирует GET/PUT запросы,
             получает/передает данные хттп.

--------------------------------------------------------------------------------

Address     Mode    Name    Description

0x00EF..    R       DR      Data register (ZIFI or RS232).
..0xBFEF                    Get byte from input FIFO.
                            Input FIFO must not be empty (xx_IFR > 0).
0x00EF..    W       DR      Data register (ZIFI or RS232).
..0xBFEF                    Put byte into output FIFO.
                            Output FIFO must not be full (xx_OFR > 0).

0xC0EF      R       ZF_IFR  ZIFI Input FIFO Used Register.
                            Switch DR to ZIFI FIFO.
                            0 - input FIFO is empty,
                            191 - input FIFO contain 191 or more bytes.
0xC1EF      R       ZF_OFR  ZIFI Output FIFO Free Register.
                            Switch DR to ZIFI FIFO.
                            0 - output FIFO is full,
                            191 - output FIFO free 191 or more bytes.
0xC2EF      R       RS_IFR  RS232 Input FIFO Used Register.
                            Switch DR to RS232 FIFO.
                            0 - input FIFO is empty,
                            191 - input FIFO contain 191 or more bytes.
0xC3EF      R       RS_OFR  RS232 Output FIFO Free Register.
                            Switch DR to RS232 FIFO.
                            0 - output FIFO is full,
                            191 - output FIFO free 191 or more bytes.

0xC7EF      W       CR      Command register.
                            Command set depends on API mode selected.
                            All mode commands:
                              Code      Description
                              000000oi  Clear ZIFI FIFOs
                                        i: 1 - clear input ZIFI FIFO,
                                        o: 1 - clear output ZIFI FIFO.
                              000001oi  Clear RS232 FIFOs
                                        i: 1 - clear input RS232 FIFO,
                                        o: 1 - clear output RS232 FIFO.
                              11110mmm  Set API mode or disable API:
                                         0     API disabled.
                                         1     transparent: all data is
                                               sent/received to/from external
                                               UART directly.
                                         2..7  reserved.
                              11111111  Returns highest supported API layer.
                                        ER=0xFF - no API available.

0xC7EF      R       ER      Error register - command execution result code.
                            Depends on command issued.
                            All mode responses:
                              Code  Description
                              0x00  OK - no error.
                              0xFF  REJ - command rejected.

--------------------------------------------------------------------------------
*/

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"
#include "zf232.h"

/* DR registers limit. */
#define ZF_DR_REG_LIM   0xBF

/* ZF_IFR register. */
#define ZF_IFR_REG      0xC0

/* ZF_OFR register. */
#define ZF_OFR_REG      0xC1

/* RS_IFR register. */
#define RS_IFR_REG      0xC2

/* RS_OFR register. */
#define RS_OFR_REG      0xC3

/* CR/ER register. */
#define ZF_CR_ER_REG    0xC7

/* OK result. */
#define ZF_OK_RES       0x00

/* CLRFIFO command. */
#define ZF_CLRFIFO      0x00
#define ZF_CLRFIFO_MASK 0xFC
#define ZF_CLRFIFO_IN   0x01
#define ZF_CLRFIFO_OUT  0x02
#define RS_CLRFIFO      0x04
#define RS_CLRFIFO_MASK 0xFC
#define RS_CLRFIFO_IN   0x01
#define RS_CLRFIFO_OUT  0x02

/* SETAPI command. */
#define ZF_SETAPI       0xF0
#define ZF_SETAPI_MASK  0xF8

/* GETVER command. */
#define ZF_GETVER       0xFF
#define ZF_GETVER_MASK  0xFF


//------------------------------------------------------------------------------

void ZF232::rs_open(int port)
{
 if (rs_open_port == port)
  return;
 rs_whead = rs_wtail = rs_rhead = rs_rtail = 0;
 if (rs_hPort && rs_hPort != INVALID_HANDLE_VALUE)
 {
  CloseHandle(rs_hPort);
  CloseHandle(rs_OvW.hEvent);
  CloseHandle(rs_OvR.hEvent);
 }
 if (port < 1 || port > 255)
  return;
 rs_open_port = port;
 open_port = rs_open_port | zf_open_port;

 char portName[11];
 _snprintf(portName, _countof(portName), "\\\\.\\COM%d", port);

 rs_hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
 if (rs_hPort == INVALID_HANDLE_VALUE)
 {
  errmsg("can't open modem on %s", portName);
  err_win32();
  conf.modem_port = rs_open_port = 0;
  open_port = zf_open_port;
  return;
 }

 COMMTIMEOUTS times;
 times.ReadIntervalTimeout = MAXDWORD;
 times.ReadTotalTimeoutMultiplier = 0;
 times.ReadTotalTimeoutConstant = 0;
 times.WriteTotalTimeoutMultiplier = 0;
 times.WriteTotalTimeoutConstant = 0;
 SetCommTimeouts(rs_hPort, &times);

 memset(&rs_OvW, 0, sizeof(rs_OvW));
 memset(&rs_OvR, 0, sizeof(rs_OvR));
 rs_OvW.hEvent = CreateEvent(0, TRUE, TRUE, 0);
 rs_OvR.hEvent = CreateEvent(0, TRUE, TRUE, 0);

#if 0
 DCB dcb;
 if (GetCommState(rs_hPort, &dcb))
 {
  printf("modem state:\n"
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
         (BYTE) dcb.XonChar, (BYTE) dcb.XoffChar, (BYTE) dcb.ErrorChar, (BYTE) dcb.EofChar, (BYTE) dcb.EvtChar);
 }
#endif
}

//------------------------------------------------------------------------------

void ZF232::zf_open(int port)
{
 if (zf_open_port == port)
  return;
 selected_api_layer = zf_whead = zf_wtail = zf_rhead = zf_rtail = 0;
 result_code = 0xFF;
 if (zf_hPort && zf_hPort != INVALID_HANDLE_VALUE)
 {
  CloseHandle(zf_hPort);
  CloseHandle(zf_OvW.hEvent);
  CloseHandle(zf_OvR.hEvent);
 }
 if (port < 1 || port > 255)
  return;
 conf.zifi_port = zf_open_port = port;
 open_port = rs_open_port | zf_open_port;

 char portName[11];
 _snprintf(portName, _countof(portName), "\\\\.\\COM%d", port);

 zf_hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
 if (zf_hPort == INVALID_HANDLE_VALUE)
 {
  errmsg("can't open %s", portName);
  err_win32();
  zf_open_port = 0;
  open_port = rs_open_port;
  return;
 }

 DCB dcb;
 if (GetCommState(zf_hPort, &dcb) == 0)
 {
  errmsg("GetCommState() on %s", portName);
  err_win32();
  CloseHandle(zf_hPort);
  zf_open_port = 0;
  open_port = rs_open_port;
  return;
 }
 dcb.fBinary = TRUE; // Binary mode; no EOF check
 dcb.fParity = FALSE; // No parity checking
 dcb.fDsrSensitivity = FALSE; // DSR sensitivity
 dcb.fErrorChar = FALSE; // Disable error replacement
 dcb.fOutxDsrFlow = FALSE; // No DSR output flow control
 dcb.fAbortOnError = FALSE; // Do not abort reads/writes on error
 dcb.fNull = FALSE; // Disable null stripping
 dcb.fTXContinueOnXoff = FALSE; // XOFF continues Tx
 dcb.BaudRate = 115200;
 dcb.ByteSize = 8;
 dcb.Parity = NOPARITY;
 dcb.StopBits = ONESTOPBIT;
 dcb.fOutxCtsFlow = FALSE; // No CTS output flow control
 dcb.fDtrControl = DTR_CONTROL_DISABLE; // DTR flow control type
 dcb.fOutX = FALSE; // No XON/XOFF out flow control
 dcb.fInX = FALSE; // No XON/XOFF in flow control
 dcb.fRtsControl = RTS_CONTROL_DISABLE; // RTS flow control
 if (SetCommState(zf_hPort, &dcb) == 0)
 {
  errmsg("SetCommState() on %s", portName);
  err_win32();
  CloseHandle(zf_hPort);
  zf_open_port = 0;
  open_port = rs_open_port;
  return;
 }

 COMMTIMEOUTS times;
 times.ReadIntervalTimeout = MAXDWORD;
 times.ReadTotalTimeoutMultiplier = 0;
 times.ReadTotalTimeoutConstant = 0;
 times.WriteTotalTimeoutMultiplier = 0;
 times.WriteTotalTimeoutConstant = 0;
 SetCommTimeouts(zf_hPort, &times);

 memset(&zf_OvW, 0, sizeof(zf_OvW));
 memset(&zf_OvR, 0, sizeof(zf_OvR));
 zf_OvW.hEvent = CreateEvent(0, TRUE, TRUE, 0);
 zf_OvR.hEvent = CreateEvent(0, TRUE, TRUE, 0);

 return;
}

//------------------------------------------------------------------------------

void ZF232::rs_close()
{
 if (!rs_hPort || rs_hPort == INVALID_HANDLE_VALUE)
  return;
 CloseHandle(rs_hPort);
 rs_hPort = INVALID_HANDLE_VALUE;
 rs_open_port = 0;
 open_port = zf_open_port;
 CloseHandle(rs_OvW.hEvent);
 CloseHandle(rs_OvR.hEvent);
}

//------------------------------------------------------------------------------

void ZF232::zf_close()
{
 if (!zf_hPort || zf_hPort == INVALID_HANDLE_VALUE)
  return;
 CloseHandle(zf_hPort);
 zf_hPort = INVALID_HANDLE_VALUE;
 zf_open_port = 0;
 open_port = rs_open_port;
 CloseHandle(zf_OvW.hEvent);
 CloseHandle(zf_OvR.hEvent);
}

//------------------------------------------------------------------------------

void ZF232::io()
{
 //- - -
 if (rs_hPort && rs_hPort != INVALID_HANDLE_VALUE)
 {
  {
   static u8 tempwr[WOVBSIZE];
   static DWORD written;
   if (WaitForSingleObject(rs_OvW.hEvent, 0) == WAIT_OBJECT_0)
   {
    written = rs_OvW.InternalHigh;
    rs_OvW.InternalHigh = 0;
    rs_wtail = (rs_wtail + written) & RS_TXBMASK;
    //if (written) printf("rs write %u bytes\n", written);
    DWORD needwrite = (rs_whead - rs_wtail) & RS_TXBMASK;
    if (needwrite)
    {
     if (needwrite > WOVBSIZE)
      needwrite = WOVBSIZE;
     unsigned j = rs_wtail;
     for (unsigned i = 0; i < needwrite; i++)
     {
      tempwr[i] = rs_wbuf[j++];
      j &= RS_TXBMASK;
     }
     WriteFile(rs_hPort, tempwr, needwrite, &written, &rs_OvW);
    }
   }
  }
  if (((rs_whead + 1) & RS_RXBMASK) != rs_wtail)
   rs_reg[5] |= 0x60;
  {
   static u8 temprd[ROVBSIZE];
   static DWORD readed;
   if (WaitForSingleObject(rs_OvR.hEvent, 0) == WAIT_OBJECT_0)
   {
    readed = rs_OvR.InternalHigh;
    rs_OvR.InternalHigh = 0;
    if (readed)
    {
     //printf("rs read %u byted\n", readed);
     for (unsigned i = 0; i < readed; i++)
     {
      rs_rcbuf[rs_rhead++] = temprd[i];
      rs_rhead &= RS_RXBMASK;
     }
    }
    DWORD canread = (rs_rtail - rs_rhead - 1) & RS_RXBMASK;
    if (canread)
    {
     if (canread > ROVBSIZE)
      canread = ROVBSIZE;
     ReadFile(rs_hPort, temprd, canread, &readed, &rs_OvR);
    }
   }
  }
  if (rs_rhead != rs_rtail)
   rs_reg[5] |= 1;
  setup_int();
 }
 //- - -
 if (zf_hPort && zf_hPort != INVALID_HANDLE_VALUE)
 {
  {
   static u8 tempwr[WOVBSIZE];
   static DWORD written;
   if (WaitForSingleObject(zf_OvW.hEvent, 0) == WAIT_OBJECT_0)
   {
    written = zf_OvW.InternalHigh;
    zf_OvW.InternalHigh = 0;
    zf_wtail = (zf_wtail + written) & ZF_TXBMASK;
    //if (written) printf("zf write %u bytes\n", written);
    DWORD needwrite = (zf_whead - zf_wtail) & ZF_TXBMASK;
    if (needwrite)
    {
     if (needwrite > WOVBSIZE)
      needwrite = WOVBSIZE;
     unsigned j = zf_wtail;
     for (unsigned i = 0; i < needwrite; i++)
     {
      tempwr[i] = zf_wbuf[j++];
      j &= ZF_TXBMASK;
     }
     WriteFile(zf_hPort, tempwr, needwrite, &written, &zf_OvW);
    }
   }
  }
  {
   static u8 temprd[ROVBSIZE];
   static DWORD readed;
   if (WaitForSingleObject(zf_OvR.hEvent, 0) == WAIT_OBJECT_0)
   {
    readed = zf_OvR.InternalHigh;
    zf_OvR.InternalHigh = 0;
    if (readed)
    {
     //printf("zf read %u byted\n", readed);
     for (unsigned i = 0; i < readed; i++)
     {
      zf_rcbuf[zf_rhead++] = temprd[i];
      zf_rhead &= ZF_RXBMASK;
     }
    }
    int canread = (zf_rtail - zf_rhead - 1) & ZF_RXBMASK;
    if (canread)
    {
     if (canread > ROVBSIZE)
      canread = ROVBSIZE;
     ReadFile(zf_hPort, temprd, canread, &readed, &zf_OvR);
    }
   }
  }

 }
 //- - -
}

//------------------------------------------------------------------------------

void ZF232::setup_int()
{
 rs_reg[6] &= ~0x10;

 u8 mask = rs_reg[5] & 1;
 if (rs_reg[5] & 0x20)
  mask |= 2, rs_reg[6] |= 0x10;
 if (rs_reg[5] & 0x1E)
  mask |= 4;
 // if (mask & rs_reg[1]) cpu.nmi()

 if (mask & 4)
  rs_reg[2] = 6;
 else if (mask & 1)
  rs_reg[2] = 4;
 else if (mask & 2)
  rs_reg[2] = 2;
 else if (mask & 8)
  rs_reg[2] = 0;
 else
  rs_reg[2] = 1;
}

//------------------------------------------------------------------------------

void ZF232::write(u8 nreg, u8 value)
{
 if (nreg >= 0xF8)
 {
  nreg &= 0x07;

  DCB dcb;

  if ((nreg == 5) || (nreg == 6))
   return; // R/O registers

  if (nreg < 2 && (rs_reg[3] & 0x80))
  {
   rs_div[nreg] = value;
   if (rs_hPort && rs_hPort != INVALID_HANDLE_VALUE && GetCommState(rs_hPort, &dcb))
   {
    if (!rs_divfq)
     dcb.BaudRate = 230400; // non standard baurrate
    else
     dcb.BaudRate = 115200 / rs_divfq;
    SetCommState(rs_hPort, &dcb);
   }
   return;
  }

  if (nreg == 0)
  { // THR, write char to output buffer
   rs_reg[5] &= ~0x60;
   if (((rs_whead + 1) & RS_TXBMASK) != rs_wtail)
   {
    rs_wbuf[rs_whead++] = value;
    rs_whead &= RS_TXBMASK;
    if (((rs_whead + 1) & RS_TXBMASK) != rs_wtail)
     rs_reg[5] |= 0x60; // Transmitter holding register empty | transmitter empty
   }
   else
   {
    // printf("write to full FIFO\n");
    // rs_reg[5] |= 2; Overrun error  (Ошибка, этот бит только на прием, а не на передачу)
   }
   setup_int();
   return;
  }

  if (nreg == 2) // FCR
  {
   ULONG Flags = 0;
   if (value & 2) // RX FIFO reset
   {
    rs_rhead = rs_rtail = 0;
    Flags |= PURGE_RXCLEAR | PURGE_RXABORT;
   }
   if (value & 4) // TX FIFO reset
   {
    rs_whead = rs_wtail = 0;
    Flags |= PURGE_TXCLEAR | PURGE_TXABORT;
   }
   if (Flags)
    PurgeComm(rs_hPort, Flags);
   return;
  }

  u8 old = rs_reg[nreg];
  rs_reg[nreg] = value;

  if (nreg == 3 && rs_hPort && rs_hPort != INVALID_HANDLE_VALUE)
  {
   // LCR set, renew modem config
   if (!GetCommState(rs_hPort, &dcb))
    return;
   dcb.fBinary = TRUE;
   dcb.fParity = (rs_reg[3] & 8) ? TRUE : FALSE;
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
   dcb.ByteSize = 5 + (rs_reg[3] & 3); // fix by Deathsoft

   static const BYTE parity[] = { ODDPARITY, EVENPARITY, MARKPARITY, SPACEPARITY };
   dcb.Parity = (rs_reg[3] & 8) ? parity[(rs_reg[3] >> 4) & 3] : NOPARITY;

   if (!(rs_reg[3] & 4))
    dcb.StopBits = ONESTOPBIT;
   else
    dcb.StopBits = ((rs_reg[3] & 3) == 1) ? ONE5STOPBITS : TWOSTOPBITS;

   SetCommState(rs_hPort, &dcb);
   return;
  }

  if (nreg == 4 && rs_hPort && rs_hPort != INVALID_HANDLE_VALUE)
  {
   // MCR set, renew DTR/RTS
   if ((old ^ rs_reg[4]) & 0x20) // auto rts/cts toggled
   {
    if (!GetCommState(rs_hPort, &dcb))
     return;
    if (rs_reg[4] & 0x20) // auto rts/cts enabled
    {
     dcb.fOutxCtsFlow = TRUE;
     dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    }
    else // auto rts/cts disabled
    {
     dcb.fOutxCtsFlow = FALSE;
     dcb.fRtsControl = RTS_CONTROL_DISABLE;
    }
    SetCommState(rs_hPort, &dcb);
   }
   if (!(rs_reg[4] & 0x20)) // auto rts/cts disabled
   {
    if ((old ^ rs_reg[4]) & 1)
    {
     EscapeCommFunction(rs_hPort, (rs_reg[4] & 1) ? SETDTR : CLRDTR);
    }
    if ((old ^ rs_reg[4]) & 2)
    {
     EscapeCommFunction(rs_hPort, (rs_reg[4] & 2) ? SETRTS : CLRRTS);
    }
   }
  }
 }
 // ZiFi CR
 else if (nreg == ZF_CR_ER_REG)
 {
  // set API layer
  if ((value & ZF_SETAPI_MASK) == ZF_SETAPI)
  {
   selected_api_layer = value & ~ZF_SETAPI_MASK;
   if (selected_api_layer > ZF_LAYERS)
    selected_api_layer = 0;
   result_code = ZF_OK_RES;
  }
  else if (selected_api_layer)
  {
   // get supported API layers
   if ((value & ZF_GETVER_MASK) == ZF_GETVER)
    result_code = ZF_LAYERS;
   // clear FIFOs
   else if ((value & ZF_CLRFIFO_MASK) == ZF_CLRFIFO)
   {
    if (value & ZF_CLRFIFO_IN)
     zf_rhead = zf_rtail = 0;
    if (value & ZF_CLRFIFO_OUT)
     zf_whead = zf_wtail = 0;
   }
   else if ((value & RS_CLRFIFO_MASK) == RS_CLRFIFO)
   {
    if (value & RS_CLRFIFO_IN)
     rs_rhead = rs_rtail = 0;
    if (value & RS_CLRFIFO_OUT)
     rs_whead = rs_wtail = 0;
   }
  }
 }
 // ZiFi DR
 else if (nreg <= ZF_DR_REG_LIM)
 {
  if (select_zf)
  { // ZiFi data write
   if (selected_api_layer == 1)
   {
    if (((zf_whead + 1) & ZF_TXBMASK) != zf_wtail)
    {
     zf_wbuf[zf_whead++] = value;
     zf_whead &= ZF_TXBMASK;
    }
   }
  }
  else
  { // enchanced RS-232 data write
   if (selected_api_layer)
   {
    rs_reg[5] &= ~0x60;
    if (((rs_whead + 1) & RS_TXBMASK) != rs_wtail)
    {
     rs_wbuf[rs_whead++] = value;
     rs_whead &= RS_TXBMASK;
     if (((rs_whead + 1) & RS_TXBMASK) != rs_wtail)
      rs_reg[5] |= 0x60; // Transmitter holding register empty | transmitter empty
    }
    else
    {
     // printf("write to full FIFO\n");
    }
   }
  }
 }
}

//------------------------------------------------------------------------------

u8 ZF232::read(u8 nreg)
{
 u8 result = 0xFF;
 if (nreg >= 0xF8)
 {
  nreg &= 0x07;

  if (nreg < 2 && (rs_reg[3] & 0x80))
   return rs_div[nreg];

  result = rs_reg[nreg];

  if (nreg == 0)
  { // read char from buffer
   //if (conf.mem_model == MM_ATM3)  result = 0;
   if (rs_rhead != rs_rtail)
   {
    result = rs_reg[0] = rs_rcbuf[rs_rtail++];
    rs_rtail &= RS_RXBMASK;
   }

   if (rs_rhead != rs_rtail)
    rs_reg[5] |= 1;
   else
    rs_reg[5] &= ~1;

   setup_int();
  }
  else if (nreg == 5)
  {
   rs_reg[5] &= ~0x0E;
   setup_int();
  }
  else if (nreg == 6)
  {
   if (rs_hPort && rs_hPort != INVALID_HANDLE_VALUE)
   {
    DWORD ModemStatus;
    GetCommModemStatus(rs_hPort, &ModemStatus);
    u8 r6 = rs_reg[6];
    rs_reg[6] &= ~(1 << 4);
    rs_reg[6] |= (ModemStatus & MS_CTS_ON) ? (1 << 4) : 0;
    rs_reg[6] &= ~1;
    rs_reg[6] |= ((r6 ^ rs_reg[6]) & (1 << 4)) >> 4;
    result = rs_reg[6];
   }
  }
 }

 // ZiFi & enchanced RS-232
 else if (nreg <= ZF_DR_REG_LIM)
 {
  if (select_zf)
  { // ZiFi data read
   if (selected_api_layer == 1)
   {
    if (zf_rhead != zf_rtail)
    {
     result = zf_rcbuf[zf_rtail++];
     zf_rtail &= ZF_RXBMASK;
    }
   }
  }
  else
  { // enchanced RS-232 data read
   if (selected_api_layer)
   {
    if (rs_rhead != rs_rtail)
    {
     result = rs_rcbuf[rs_rtail++];
     rs_rtail &= RS_RXBMASK;
    }
   }
  }
 }
 // ZiFi FIFO in used read
 else if (nreg == ZF_IFR_REG)
 {
  if (selected_api_layer == 1)
  {
   unsigned tmp = (zf_rhead - zf_rtail) & ZF_RXBMASK;
   if (tmp > ZF_DR_REG_LIM)
    result = ZF_DR_REG_LIM;
   else
    result = (u8) tmp;
   select_zf = 1;
  }
 }
 // ZiFi FIFO out free read
 else if (nreg == ZF_OFR_REG)
 {
  if (selected_api_layer == 1)
  {
   unsigned tmp = (zf_wtail - zf_whead - 1) & ZF_TXBMASK;
   if (tmp > ZF_DR_REG_LIM)
    result = ZF_DR_REG_LIM;
   else
    result = (u8) tmp;
   select_zf = 1;
  }
 }
 // enchanced RS-232 FIFO in used read
 else if (nreg == RS_IFR_REG)
 {
  if (selected_api_layer)
  {
   unsigned tmp = (rs_rhead - rs_rtail) & RS_RXBMASK;
   if (tmp > ZF_DR_REG_LIM)
    result = ZF_DR_REG_LIM;
   else
    result = (u8) tmp;
   select_zf = 0;
  }
 }
 // enchanced RS-232 FIFO out free read
 else if (nreg == RS_OFR_REG)
 {
  if (selected_api_layer)
  {
   unsigned tmp = (rs_wtail - rs_whead - 1) & RS_TXBMASK;
   if (tmp > ZF_DR_REG_LIM)
    result = ZF_DR_REG_LIM;
   else
    result = (u8) tmp;
   select_zf = 0;
  }
 }
 // error/result code read
 else if (nreg == ZF_CR_ER_REG)
 {
  if (selected_api_layer)
   result = result_code;
 }

 return result;
}

//------------------------------------------------------------------------------
