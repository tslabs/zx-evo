
#include "stdafx.h"
#include <commctrl.h>

#define TRD_SZ  256*16*255
#define BAUD    115200
#define TOKEN1  0xAA
#define TOKEN2  0x55
#define ANS1    0xCC
#define ANS2    0xEE

enum OPCODE : U8
{
  OP_RD = 5,
  OP_WR = 6
};

enum STATE
{
  ST_IDLE = 0,
  ST_RECEIVE_START_TOKEN2,
  ST_RECEIVE_HEADER,
  ST_RECEIVE_DATA
};

#pragma pack (push, 1)
typedef struct
{
  U8      drv;
  OPCODE  op;
  U8      trk;
  U8      sec;
  U8      crc;
} REQ;

typedef struct
{
  U8      ack[2];
  U8      data[256];
  U8      crc;
} SECT;
#pragma pack (pop)

U8 img[4][TRD_SZ];
int baud = BAUD;
bool log = true;
_TCHAR* cport = TEXT("COM1");
_TCHAR* trd[4];
U8 drvs = 0;
U8 fifo_in_buf[512];
U8 uart_in_buf[512];

void print_help()
{
  printf("RS-232 VDOS Mounter,  (c) 2013 TS-Labs inc.\n\r\n\r");
  printf("Command line parameters (any is optional):\n\r");
  printf("-a|b|c|d <filename.trd>\n\r\tTRD image to be mounted on drive A-D (up to 4 images)\n\r");
  printf("-com\n\r\tSerial port name (default = COM1)\n\r");
  printf("-baud\n\r\tUART Baudrate (default = %d)\n\r", BAUD);
  printf("-log\n\r\tPrint log for disk operations\n\r\n\r", BAUD);
}

int parse_arg(int argc, _TCHAR* argv[], _TCHAR* arg, int n)
{
  for (int i=1; i<argc; i++)
    if (!wcscmp(argv[i], arg) && (argc-1) >= (i+n))
      return i+1;
  return 0;
}

int parse_args(int argc, _TCHAR* argv[])
{
  int i;

  if (i = parse_arg(argc, argv, L"-baud", 1))
    baud = _wtoi(argv[i]);

  if (i = parse_arg(argc, argv, L"-com", 1))
    cport = argv[i];

  if (i = parse_arg(argc, argv, L"-log", 0))
    log = 1;

  if (i = parse_arg(argc, argv, L"-a", 1))
    { trd[0] = argv[i]; drvs++; }

  if (i = parse_arg(argc, argv, L"-b", 1))
    { trd[1] = argv[i]; drvs++; }

  if (i = parse_arg(argc, argv, L"-c", 1))
    { trd[2] = argv[i]; drvs++; }

  if (i = parse_arg(argc, argv, L"-d", 1))
    { trd[3] = argv[i]; drvs++; }

  return drvs;
}

U8 update_xor(U8 xor, U8 *ptr, int num)
{
  while (num--) xor ^= *ptr++;

  return xor;
}

//-------------------------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
  FIFO fifo_in;
  REQ req;
  SECT sect;

  DWORD dwRead, dwWrite;
  FILE *f;
  STATE state = ST_IDLE;

  printf("\n\r");

  if (!parse_args(argc, argv))
  {
    print_help();
    return 1;
  }

  for (int i=0; i<4; i++)
  {
    if (trd[i])
    {
      if (!(f = _wfopen(trd[i], L"r")))
      {
        wprintf(L"Can't open: %s\n\r", trd[i]);
        return 2;
      }
      else
      {
        wprintf(L"%s opened successfully\n\r", trd[i]);
        fread(img[i], 1, TRD_SZ, f);
        fclose(f);
      }
    }
  }

  HANDLE hPort = CreateFile (cport, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (hPort == INVALID_HANDLE_VALUE)
  {
    wprintf(L"Can't open %s\n\r", cport);
    return 3;
  }
  else
    wprintf(L"%s opened successfully\n\r\n\r", cport);

  COMMTIMEOUTS times;
  times.ReadIntervalTimeout = MAXDWORD;
  times.ReadTotalTimeoutMultiplier = 0;
  times.ReadTotalTimeoutConstant = 1;
  times.WriteTotalTimeoutMultiplier = 0;
  times.WriteTotalTimeoutConstant = 1;
  SetCommTimeouts(hPort, &times);

  DCB PortDCB;
  PortDCB.DCBlength = sizeof(DCB);
  PortDCB.fBinary = TRUE;            // Binary mode; no EOF check
  PortDCB.fParity = FALSE;          // No parity checking
  PortDCB.fDsrSensitivity = FALSE;      // DSR sensitivity
  PortDCB.fErrorChar = FALSE;          // Disable error replacement
  PortDCB.fOutxDsrFlow = FALSE;        // No DSR output flow control
  PortDCB.fAbortOnError = FALSE;        // Do not abort reads/writes on error
  PortDCB.fNull = FALSE;            // Disable null stripping
  PortDCB.fTXContinueOnXoff = FALSE;      // XOFF continues Tx
  PortDCB.BaudRate = baud;
  PortDCB.ByteSize = 8;
  PortDCB.Parity = NOPARITY;
  PortDCB.StopBits =  ONESTOPBIT;
  PortDCB.fOutxCtsFlow = FALSE;        // No CTS output flow control
  PortDCB.fDtrControl = DTR_CONTROL_DISABLE;  // DTR flow control type
  PortDCB.fOutX = FALSE;            // No XON/XOFF out flow control
  PortDCB.fInX = FALSE;            // No XON/XOFF in flow control
  PortDCB.fRtsControl = RTS_CONTROL_DISABLE;  // RTS flow control
  SetCommState (hPort, &PortDCB);

  fifo_init(&fifo_in, fifo_in_buf, sizeof(fifo_in_buf));

  U8 *disk_ptr;
  while (1)
  {
    ReadFile(hPort, uart_in_buf, 1, &dwRead, NULL);
    fifo_put(&fifo_in, uart_in_buf, dwRead);

    while (fifo_used(fifo_in))
    {
      switch (state)
      {
        case ST_IDLE:
          if (fifo_get_byte(&fifo_in) == TOKEN1)
            state = ST_RECEIVE_START_TOKEN2;
        break;

        case ST_RECEIVE_START_TOKEN2:
          if (fifo_get_byte(&fifo_in) == TOKEN2)
            state = ST_RECEIVE_HEADER;
          else
            state = ST_IDLE;
        break;

        case ST_RECEIVE_HEADER:
          if (!fifo_get(&fifo_in, (U8*)&req, sizeof(req)))
            goto cont1;
          else
          {
            if (log) printf("Op: %d\tDrv: %d\tTrk: %d\tSec: %d\n\r", req.op, req.drv, req.trk, req.sec);

            if (req.drv > 3)
              if (log) printf("Wrong drive!\n\r");

            if (req.sec > 15)
              if (log) printf("Wrong sector!\n\r");

            disk_ptr = img[req.drv] + ((req.trk * 16 + req.sec) * sizeof(sect.data));

            switch (req.op)
            {
              case OP_RD:
                sect.ack[0] = ANS1;
                sect.ack[1] = ANS2;
                memcpy(sect.data, disk_ptr, sizeof(sect.data));
                sect.crc = update_xor(ANS1 ^ ANS2, disk_ptr, sizeof(sect.data));
                WriteFile(hPort, &sect, sizeof(sect), &dwWrite, NULL);
                state = ST_IDLE;
              break;

              case OP_WR:
                state = ST_RECEIVE_DATA;
              break;

              default:
                if (log) printf("Wrong operation!\n\r");
                state = ST_IDLE;
            }
          }
        break;

        case ST_RECEIVE_DATA:
          if (!fifo_get(&fifo_in, (U8*)&sect, sizeof(sect)))
            goto cont1;
          else
          {
            memcpy(disk_ptr, sect.data, sizeof(sect.data));
            state = ST_IDLE;
          }
        break;
      }
    }
  cont1: ;
  }

  return 0;
}
