
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

#pragma once
#include "sysdefs.h"



constexpr auto rovbsize = 512;
constexpr auto wovbsize = 1024;

// *BSIZE should be power of 2  and  above double OVBSIZE
constexpr auto RS_RXBSIZE = 1024;
#define RS_RXBMASK      (RS_RXBSIZE-1)
constexpr auto RS_TXBSIZE = 1024;
#define RS_TXBMASK      (RS_TXBSIZE-1)
constexpr auto ZF_RXBSIZE = 1024;
#define ZF_RXBMASK      (ZF_RXBSIZE-1)
constexpr auto ZF_TXBSIZE = 1024;
#define ZF_TXBMASK      (ZF_TXBSIZE-1)


constexpr u8 zf_dr_reg_lim	= 0xBF; // DR registers limit.
constexpr u8 zf_ifr_reg		= 0xC0; // ZF_IFR register.
constexpr u8 zf_ofr_reg		= 0xC1; // ZF_OFR register.
constexpr u8 rs_ifr_reg		= 0xC2; // RS_IFR register.
constexpr u8 rs_ofr_reg		= 0xC3; // RS_OFR register.
constexpr u8 zf_cr_er_reg	= 0xC7; // CR/ER register.



// Supported layers of the ZiFi
constexpr auto zf_layers = 0x01;

struct zf232_t
{
	HANDLE rs_h_port;
	HANDLE zf_h_port;
	OVERLAPPED rs_ov_w;
	OVERLAPPED rs_ov_r;
	OVERLAPPED zf_ov_w;
	OVERLAPPED zf_ov_r;
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
