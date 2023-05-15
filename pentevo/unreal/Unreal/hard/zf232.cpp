

#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"
#include "zf232.h"


/* OK result. */
enum
{
	ZF_OK_RES = 0x00
};

/* CLRFIFO command. */
enum
{
	ZF_CLRFIFO = 0x00,
	ZF_CLRFIFO_MASK = 0xFC,
	ZF_CLRFIFO_IN = 0x01,
	ZF_CLRFIFO_OUT = 0x02,
	RS_CLRFIFO = 0x04,
	RS_CLRFIFO_MASK = 0xFC,
	RS_CLRFIFO_IN = 0x01,
	RS_CLRFIFO_OUT = 0x02
};

/* SETAPI command. */
enum
{
	ZF_SETAPI = 0xF0,
	ZF_SETAPI_MASK = 0xF8
};

/* GETVER command. */
enum
{
	ZF_GETVER = 0xFF,
	ZF_GETVER_MASK = 0xFF
};


//------------------------------------------------------------------------------

void zf232_t::rs_open(const int port)
{
	if (rs_open_port == port)
		return;
	rs_whead = rs_wtail = rs_rhead = rs_rtail = 0;
	if (rs_h_port && rs_h_port != INVALID_HANDLE_VALUE)
	{
		CloseHandle(rs_h_port);
		CloseHandle(rs_ov_w.hEvent);
		CloseHandle(rs_ov_r.hEvent);
	}
	if (port < 1 || port > 255)
		return;
	rs_open_port = port;
	open_port = rs_open_port | zf_open_port;

	char port_name[11];
	_snprintf(port_name, _countof(port_name), R"(\\.\COM%d)", port);

	rs_h_port = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (rs_h_port == INVALID_HANDLE_VALUE)
	{
		errmsg("can't open modem on %s", port_name);
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
	SetCommTimeouts(rs_h_port, &times);

	memset(&rs_ov_w, 0, sizeof(rs_ov_w));
	memset(&rs_ov_r, 0, sizeof(rs_ov_r));
	rs_ov_w.hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
	rs_ov_r.hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);

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
			(BYTE)dcb.XonChar, (BYTE)dcb.XoffChar, (BYTE)dcb.ErrorChar, (BYTE)dcb.EofChar, (BYTE)dcb.EvtChar);
	}
#endif
}

//------------------------------------------------------------------------------

void zf232_t::zf_open(const int port)
{
	if (zf_open_port == port)
		return;
	selected_api_layer = zf_whead = zf_wtail = zf_rhead = zf_rtail = 0;
	result_code = 0xFF;
	if (zf_h_port && zf_h_port != INVALID_HANDLE_VALUE)
	{
		CloseHandle(zf_h_port);
		CloseHandle(zf_ov_w.hEvent);
		CloseHandle(zf_ov_r.hEvent);
	}
	if (port < 1 || port > 255)
		return;
	conf.zifi_port = zf_open_port = port;
	open_port = rs_open_port | zf_open_port;

	char portName[11];
	_snprintf(portName, _countof(portName), R"(\\.\COM%d)", port);

	zf_h_port = CreateFile(portName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
	if (zf_h_port == INVALID_HANDLE_VALUE)
	{
		errmsg("can't open %s", portName);
		err_win32();
		zf_open_port = 0;
		open_port = rs_open_port;
		return;
	}

	DCB dcb;
	if (GetCommState(zf_h_port, &dcb) == 0)
	{
		errmsg("GetCommState() on %s", portName);
		err_win32();
		CloseHandle(zf_h_port);
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
	if (SetCommState(zf_h_port, &dcb) == 0)
	{
		errmsg("SetCommState() on %s", portName);
		err_win32();
		CloseHandle(zf_h_port);
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
	SetCommTimeouts(zf_h_port, &times);

	memset(&zf_ov_w, 0, sizeof(zf_ov_w));
	memset(&zf_ov_r, 0, sizeof(zf_ov_r));
	zf_ov_w.hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);
	zf_ov_r.hEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);

	return;
}

//------------------------------------------------------------------------------

void zf232_t::rs_close()
{
	if (!rs_h_port || rs_h_port == INVALID_HANDLE_VALUE)
		return;
	CloseHandle(rs_h_port);
	rs_h_port = INVALID_HANDLE_VALUE;
	rs_open_port = 0;
	open_port = zf_open_port;
	CloseHandle(rs_ov_w.hEvent);
	CloseHandle(rs_ov_r.hEvent);
}

//------------------------------------------------------------------------------

void zf232_t::zf_close()
{
	if (!zf_h_port || zf_h_port == INVALID_HANDLE_VALUE)
		return;
	CloseHandle(zf_h_port);
	zf_h_port = INVALID_HANDLE_VALUE;
	zf_open_port = 0;
	open_port = rs_open_port;
	CloseHandle(zf_ov_w.hEvent);
	CloseHandle(zf_ov_r.hEvent);
}

//------------------------------------------------------------------------------

void zf232_t::io()
{
	//- - -
	if (rs_h_port && rs_h_port != INVALID_HANDLE_VALUE)
	{
		{
			static DWORD written;
			if (WaitForSingleObject(rs_ov_w.hEvent, 0) == WAIT_OBJECT_0)
			{
				written = rs_ov_w.InternalHigh;
				rs_ov_w.InternalHigh = 0;
				rs_wtail = (rs_wtail + written) & RS_TXBMASK;
				//if (written) printf("rs write %u bytes\n", written);
				DWORD needwrite = (rs_whead - rs_wtail) & RS_TXBMASK;
				if (needwrite)
				{
					static u8 tempwr[wovbsize];
					if (needwrite > wovbsize)
						needwrite = wovbsize;
					unsigned j = rs_wtail;
					for (unsigned i = 0; i < needwrite; i++)
					{
						tempwr[i] = rs_wbuf[j++];
						j &= RS_TXBMASK;
					}
					WriteFile(rs_h_port, tempwr, needwrite, &written, &rs_ov_w);
				}
			}
		}
		if (((rs_whead + 1) & RS_RXBMASK) != rs_wtail)
			rs_reg[5] |= 0x60;
		{
			static DWORD readed;
			if (WaitForSingleObject(rs_ov_r.hEvent, 0) == WAIT_OBJECT_0)
			{
				static u8 temprd[rovbsize];
				readed = rs_ov_r.InternalHigh;
				rs_ov_r.InternalHigh = 0;
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
					if (canread > rovbsize)
						canread = rovbsize;
					ReadFile(rs_h_port, temprd, canread, &readed, &rs_ov_r);
				}
			}
		}
		if (rs_rhead != rs_rtail)
			rs_reg[5] |= 1;
		setup_int();
	}
	//- - -
	if (zf_h_port && zf_h_port != INVALID_HANDLE_VALUE)
	{
		{
			static DWORD written;
			if (WaitForSingleObject(zf_ov_w.hEvent, 0) == WAIT_OBJECT_0)
			{
				written = zf_ov_w.InternalHigh;
				zf_ov_w.InternalHigh = 0;
				zf_wtail = (zf_wtail + written) & ZF_TXBMASK;
				//if (written) printf("zf write %u bytes\n", written);
				DWORD needwrite = (zf_whead - zf_wtail) & ZF_TXBMASK;
				if (needwrite)
				{
					static u8 tempwr[wovbsize];
					if (needwrite > wovbsize)
						needwrite = wovbsize;
					unsigned j = zf_wtail;
					for (unsigned i = 0; i < needwrite; i++)
					{
						tempwr[i] = zf_wbuf[j++];
						j &= ZF_TXBMASK;
					}
					WriteFile(zf_h_port, tempwr, needwrite, &written, &zf_ov_w);
				}
			}
		}
		{
			static DWORD readed;
			if (WaitForSingleObject(zf_ov_r.hEvent, 0) == WAIT_OBJECT_0)
			{
				static u8 temprd[rovbsize];
				readed = zf_ov_r.InternalHigh;
				zf_ov_r.InternalHigh = 0;
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
					if (canread > rovbsize)
						canread = rovbsize;
					ReadFile(zf_h_port, temprd, canread, &readed, &zf_ov_r);
				}
			}
		}

	}
	//- - -
}

//------------------------------------------------------------------------------

void zf232_t::setup_int()
{
	rs_reg[6] &= ~0x10;

	u8 mask = rs_reg[5] & 1;

	if (rs_reg[5] & 0x20) {
		mask |= 2;
		rs_reg[6] |= 0x10;
	}
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

void zf232_t::write(u8 nreg, const u8 value)
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
			if (rs_h_port && rs_h_port != INVALID_HANDLE_VALUE && GetCommState(rs_h_port, &dcb))
			{
				if (!rs_divfq)
					dcb.BaudRate = 230400; // non standard baurrate
				else
					dcb.BaudRate = 115200 / rs_divfq;
				SetCommState(rs_h_port, &dcb);
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
			ULONG flags = 0;
			if (value & 2) // RX FIFO reset
			{
				rs_rhead = rs_rtail = 0;
				flags |= PURGE_RXCLEAR | PURGE_RXABORT;
			}
			if (value & 4) // TX FIFO reset
			{
				rs_whead = rs_wtail = 0;
				flags |= PURGE_TXCLEAR | PURGE_TXABORT;
			}
			if (flags)
				PurgeComm(rs_h_port, flags);
			return;
		}

		const u8 old = rs_reg[nreg];
		rs_reg[nreg] = value;

		if (nreg == 3 && rs_h_port && rs_h_port != INVALID_HANDLE_VALUE)
		{
			// LCR set, renew modem config
			if (!GetCommState(rs_h_port, &dcb))
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

			static constexpr BYTE parity[] = { ODDPARITY, EVENPARITY, MARKPARITY, SPACEPARITY };
			dcb.Parity = (rs_reg[3] & 8) ? parity[(rs_reg[3] >> 4) & 3] : NOPARITY;

			if (!(rs_reg[3] & 4))
				dcb.StopBits = ONESTOPBIT;
			else
				dcb.StopBits = ((rs_reg[3] & 3) == 1) ? ONE5STOPBITS : TWOSTOPBITS;

			SetCommState(rs_h_port, &dcb);
			return;
		}

		if (nreg == 4 && rs_h_port && rs_h_port != INVALID_HANDLE_VALUE)
		{
			// MCR set, renew DTR/RTS
			if ((old ^ rs_reg[4]) & 0x20) // auto rts/cts toggled
			{
				if (!GetCommState(rs_h_port, &dcb))
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
				SetCommState(rs_h_port, &dcb);
			}
			if (!(rs_reg[4] & 0x20)) // auto rts/cts disabled
			{
				if ((old ^ rs_reg[4]) & 1)
				{
					EscapeCommFunction(rs_h_port, (rs_reg[4] & 1) ? SETDTR : CLRDTR);
				}
				if ((old ^ rs_reg[4]) & 2)
				{
					EscapeCommFunction(rs_h_port, (rs_reg[4] & 2) ? SETRTS : CLRRTS);
				}
			}
		}
	}
	// ZiFi CR
	else if (nreg == zf_cr_er_reg)
	{
		// set API layer
		if ((value & ZF_SETAPI_MASK) == ZF_SETAPI)
		{
			selected_api_layer = value & ~ZF_SETAPI_MASK;
			if (selected_api_layer > zf_layers)
				selected_api_layer = 0;
			result_code = ZF_OK_RES;
		}
		else if (selected_api_layer)
		{
			// get supported API layers
			if ((value & ZF_GETVER_MASK) == ZF_GETVER)
				result_code = zf_layers;
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
	else if (nreg <= zf_dr_reg_lim)
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

u8 zf232_t::read(u8 nreg)
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
			if (rs_h_port && rs_h_port != INVALID_HANDLE_VALUE)
			{
				DWORD modem_status;
				GetCommModemStatus(rs_h_port, &modem_status);
				const u8 r6 = rs_reg[6];
				rs_reg[6] &= ~(1 << 4);
				rs_reg[6] |= (modem_status & MS_CTS_ON) ? (1 << 4) : 0;
				rs_reg[6] &= ~1;
				rs_reg[6] |= ((r6 ^ rs_reg[6]) & (1 << 4)) >> 4;
				result = rs_reg[6];
			}
		}
	}

	// ZiFi & enchanced RS-232
	else if (nreg <= zf_dr_reg_lim)
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
	else if (nreg == zf_ifr_reg)
	{
		if (selected_api_layer == 1)
		{
			const unsigned tmp = (zf_rhead - zf_rtail) & ZF_RXBMASK;
			if (tmp > zf_dr_reg_lim)
				result = zf_dr_reg_lim;
			else
				result = static_cast<u8>(tmp);
			select_zf = 1;
		}
	}
	// ZiFi FIFO out free read
	else if (nreg == zf_ofr_reg)
	{
		if (selected_api_layer == 1)
		{
			const unsigned tmp = (zf_wtail - zf_whead - 1) & ZF_TXBMASK;
			if (tmp > zf_dr_reg_lim)
				result = zf_dr_reg_lim;
			else
				result = static_cast<u8>(tmp);
			select_zf = 1;
		}
	}
	// enchanced RS-232 FIFO in used read
	else if (nreg == rs_ifr_reg)
	{
		if (selected_api_layer)
		{
			const unsigned tmp = (rs_rhead - rs_rtail) & RS_RXBMASK;
			if (tmp > zf_dr_reg_lim)
				result = zf_dr_reg_lim;
			else
				result = static_cast<u8>(tmp);
			select_zf = 0;
		}
	}
	// enchanced RS-232 FIFO out free read
	else if (nreg == rs_ofr_reg)
	{
		if (selected_api_layer)
		{
			const unsigned tmp = (rs_wtail - rs_whead - 1) & RS_TXBMASK;
			if (tmp > zf_dr_reg_lim)
				result = zf_dr_reg_lim;
			else
				result = static_cast<u8>(tmp);
			select_zf = 0;
		}
	}
	// error/result code read
	else if (nreg == zf_cr_er_reg)
	{
		if (selected_api_layer)
			result = result_code;
	}

	return result;
}

//------------------------------------------------------------------------------
