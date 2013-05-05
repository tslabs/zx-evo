// rs232mnt.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <commctrl.h>

typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned long u32;
typedef signed long i32;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned char u8;
typedef signed char i8;

#define TRD_SZ	256*16*255
#define	BAUD	115200
#define	ACK		0x55AA
#define	ANS1	0xCC
#define	ANS2	0xEE

struct REQ {
	u16	ack;
	u8	drv;
	u8	op;
	u8	trk;
	u8	sec;
	u8	crc;
};

HANDLE			hPort;
DCB				PortDCB; 
COMMTIMEOUTS	CommTimeouts; 
_TCHAR*			trd[4];
u8				buf[256+1];
u8				img[4][TRD_SZ];
u8				drvs = 0;
REQ				req;
int				baud = BAUD;
int				log = 0;
_TCHAR*			cport = TEXT("COM1");
u8				ans[2];

#define OP_RD	5
#define OP_WR	6

void configure()
{
	PortDCB.fBinary = TRUE;						// Binary mode; no EOF check
	PortDCB.fParity = FALSE;					// No parity checking 
	PortDCB.fDsrSensitivity = FALSE;			// DSR sensitivity 
	PortDCB.fErrorChar = FALSE;					// Disable error replacement 
	PortDCB.fOutxDsrFlow = FALSE;				// No DSR output flow control 
	PortDCB.fAbortOnError = FALSE;				// Do not abort reads/writes on error
	PortDCB.fNull = FALSE;						// Disable null stripping 
	PortDCB.fTXContinueOnXoff = FALSE;			// XOFF continues Tx 

    PortDCB.BaudRate = baud;            
	PortDCB.ByteSize = 8;            
    PortDCB.Parity = NOPARITY;                   
	PortDCB.StopBits =  ONESTOPBIT;          
	PortDCB.fOutxCtsFlow = FALSE;				// No CTS output flow control 
    PortDCB.fDtrControl = DTR_CONTROL_DISABLE;	// DTR flow control type 
    PortDCB.fOutX = FALSE;						// No XON/XOFF out flow control 
    PortDCB.fInX = FALSE;						// No XON/XOFF in flow control 
    PortDCB.fRtsControl = RTS_CONTROL_DISABLE;	// RTS flow control 
}

int configuretimeout()
{
	//memset(&CommTimeouts, 0x00, sizeof(CommTimeouts)); 
	CommTimeouts.ReadIntervalTimeout = 50; 
	CommTimeouts.ReadTotalTimeoutConstant = 50; 
	CommTimeouts.ReadTotalTimeoutMultiplier = 10;
	CommTimeouts.WriteTotalTimeoutMultiplier = 10;
	CommTimeouts.WriteTotalTimeoutConstant = 50; 
   return 1;
}

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

//-------------------------------------------------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	DWORD dwRead, dwWrite;
	FILE *f;

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

	hPort = CreateFile (cport,
						GENERIC_READ | GENERIC_WRITE,
						0,                                  
						NULL,                             
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,                     
			            NULL); 

	if (hPort == INVALID_HANDLE_VALUE)
	{
		wprintf(L"Can't open %s\n\r", cport);
		return 3;
	}
	else
		wprintf(L"%s opened successfully\n\r\n\r", cport);
	
    PortDCB.DCBlength = sizeof(DCB); 
    GetCommState(hPort, &PortDCB);
	configure();
	GetCommTimeouts (hPort, &CommTimeouts);
	configuretimeout();
	SetCommState (hPort, &PortDCB);

	ans[0] = ANS1;
	ans[1] = ANS2;
	while (1)
	{
		OVERLAPPED osReader = {0};

		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ReadFile(hPort, &req, 7, &dwRead, &osReader);
		if (!dwRead)
		{
			SleepEx(10, NULL);
			continue;
		}
		if (req.op != OP_RD) continue;
		
		req.drv &= 3;
		req.sec &= 15;
		if (log)
			printf("Op: %d\tDrv: %d\tTrk: %d\tSec: %d\n\r", req.op, req.drv, req.trk, req.sec);
		DWORD offs = (req.trk * 16 + req.sec) * 256;
		u8 *t = img[req.drv] + offs;
		u8 crc = ANS1 ^ ANS2;
		WriteFile(hPort, ans, 2, &dwWrite, NULL);
		WriteFile(hPort, t, 256, &dwWrite, NULL);
		for (int i=0; i<256; i++)
			crc ^= *t++;
		WriteFile(hPort, &crc, 1, &dwWrite, NULL);
	}

	return 0;
}

