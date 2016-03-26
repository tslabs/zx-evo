
// Implemented API layer 1  -  the bridge ZiFi<==>COM-port
// Реализован API уровень 1  -  мост ZiFi<==>COM-port

/*
--------------------------------------------------------------------------------

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

Address         Mode Name Description
0x00EF..0xBFEF  R    DR   Data register. Get byte from input FIFO. Input FIFO must not be empty (IFR > 0).
0x00EF..0xBFEF  W    DR   Data register. Put byte into output FIFO. Output FIFO must not be full (OFR > 0).

Address Mode Name Description
0xC0EF  R    IFR  Input FIFO Used Register. 0 - input FIFO is empty, 255 - input FIFO is full.
0xC1EF  R    OFR  Output FIFO Free Register. 0 - output FIFO is full, 255 - output FIFO is empty.

Address Mode Name Description
0xC7EF  W    CR   Command register. Command set depends on API mode selected.

  All mode commands:
    Code     Command      Description
    000000oi Clear FIFOs  i: 1 - clear input FIFO, o: 1 - clear output FIFO.
    11110mmm Set API mode or disable API:
              0     API disabled.
              1     transparent: all data is sent/received to/from external UART directly.
              2..7  reserved.
    11111111 Get Version  Returns highest supported API version. ER=0xFF - no API available.

Address Mode Name Description
0xC7EF  R    ER   Error register - command execution result code. Depends on command issued.

  All mode responses:
    Code Description
    0x00 OK - no error.
    0xFF REJ - command rejected.

--------------------------------------------------------------------------------
*/

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"


/* DR registers limit. */
#define ZF_DR_REG_LIM   0xBF

/* IFR register. */
#define ZF_IFR_REG      0xC0

/* OFR register. */
#define ZF_OFR_REG      0xC1

/* CR/ER register. */
#define ZF_CR_ER_REG    0xC7

/* OK result. */
#define ZF_OK_RES       0x00

/* CLRFIFO command. */
#define ZF_CLRFIFO      0x00
#define ZF_CLRFIFO_MASK 0xFC
#define ZF_CLRFIFO_IN   0x01
#define ZF_CLRFIFO_OUT  0x02

/* SETAPI command. */
#define ZF_SETAPI       0xF0
#define ZF_SETAPI_MASK  0xF8

/* GETVER command. */
#define ZF_GETVER       0xFF
#define ZF_GETVER_MASK  0xFF


int ZIFI::open(int port)
{
 if (open_port == port) return 1;
 selected_api_layer = whead = wtail = rhead = rtail = 0;
 result_code = 0xff;
 if (hPort && hPort != INVALID_HANDLE_VALUE)  CloseHandle(hPort);
 open_port = port;
 if (!port) return 0;

 char portName[11];
 _snprintf(portName, _countof(portName), "\\\\.\\COM%d", port);

 hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
 if (hPort == INVALID_HANDLE_VALUE)
 {
  errmsg("can't open serial-port on %s", portName);
  err_win32();
  open_port = 0;
  return 0;
 }

 COMMTIMEOUTS times;
 times.ReadIntervalTimeout = MAXDWORD;
 times.ReadTotalTimeoutMultiplier = 0;
 times.ReadTotalTimeoutConstant = 0;
 times.WriteTotalTimeoutMultiplier = 0;
 times.WriteTotalTimeoutConstant = 0;
 SetCommTimeouts(hPort, &times);

 DCB dcb;
 if (GetCommState(hPort, &dcb))
 {
  dcb.fBinary = TRUE;                     // Binary mode; no EOF check
  dcb.fParity = FALSE;                    // No parity checking
  dcb.fDsrSensitivity = FALSE;            // DSR sensitivity
  dcb.fErrorChar = FALSE;                 // Disable error replacement
  dcb.fOutxDsrFlow = FALSE;               // No DSR output flow control
  dcb.fAbortOnError = FALSE;              // Do not abort reads/writes on error
  dcb.fNull = FALSE;                      // Disable null stripping
  dcb.fTXContinueOnXoff = FALSE;          // XOFF continues Tx
  dcb.BaudRate = 115200;
  dcb.ByteSize = 8;
  dcb.Parity = NOPARITY;
  dcb.StopBits =  ONESTOPBIT;
  dcb.fOutxCtsFlow = FALSE;               // No CTS output flow control
  dcb.fDtrControl = DTR_CONTROL_DISABLE;  // DTR flow control type
  dcb.fOutX = FALSE;                      // No XON/XOFF out flow control
  dcb.fInX = FALSE;                       // No XON/XOFF in flow control
  dcb.fRtsControl = RTS_CONTROL_DISABLE;  // RTS flow control
  SetCommState(hPort, &dcb);
 }
 return 1;
}


void ZIFI::close()
{
 if (!hPort || hPort == INVALID_HANDLE_VALUE)  return;
 CloseHandle(hPort);
 hPort = INVALID_HANDLE_VALUE;
 open_port = 0;
}


void ZIFI::io()
{
 if (!hPort || hPort == INVALID_HANDLE_VALUE)  return;
 u8 temp[BSIZE];

 int needwrite = (whead - wtail) & (BSIZE-1);
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
   //printf("\nzifi send: "); dump1(temp, written);
   wtail = (wtail+written) & (BSIZE-1);
  }
 }

 int canread = (rtail - rhead - 1) & (BSIZE-1);
 if (canread)
 {
  DWORD read = 0;
  if (ReadFile(hPort, temp, canread, &read, 0) && read)
  {
   for (unsigned i = 0; i < read; i++)
    rcbuf[rhead++] = temp[i], rhead &= (BSIZE-1);
   //printf("\nzifi recv: "); dump1(temp, read);
  }
 }
}


void ZIFI::write(u8 nreg, u8 value)
{
 if (nreg == ZF_CR_ER_REG)
 {
  // set API layer
  if ((value & ZF_SETAPI_MASK) == ZF_SETAPI)
  {
   selected_api_layer = value & ~ZF_SETAPI_MASK;
   if (selected_api_layer > ZF_LAYERS)  selected_api_layer = 0;
   result_code = ZF_OK_RES;
  }
  else if (selected_api_layer)
  {
   // get supported API layers
   if ((value & ZF_GETVER_MASK) == ZF_GETVER)
    result_code = 1;
   // clear FIFOs
   else if ((value & ZF_CLRFIFO_MASK) == ZF_CLRFIFO)
   {
    if (value & ZF_CLRFIFO_IN)   rhead = rtail = 0;
    if (value & ZF_CLRFIFO_OUT)  whead = wtail = 0;
   }
  }
 }
 // data write
 else if ( (nreg <= ZF_DR_REG_LIM) && (selected_api_layer == 1) )
 {
  if (((whead+1) & (BSIZE-1)) != wtail)
  {
   wbuf[whead++] = value;
   whead &= (BSIZE-1);
  }
 }
}


u8 ZIFI::read(u8 nreg)
{
 u8 result = 0xff;
 if (selected_api_layer)
 {
  // data read
  if (nreg <= ZF_DR_REG_LIM)
  {
   if (rhead != rtail)
   {
    result = rcbuf[rtail++];
    rtail &= (BSIZE-1);
   }
  }
  // FIFO in used read
  else if (nreg == ZF_IFR_REG)
  {
   result = rhead - rtail;
  }
  // FIFO out free read
  else if (nreg == ZF_OFR_REG)
  {
   result = wtail - whead - 1;
  }
  // error/result code read
  else if (nreg == ZF_CR_ER_REG)
  {
   result = result_code;
  }
 }
 return result;
}

