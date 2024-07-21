
#include <windows.h>
#include "std.h"
#include "ftd2xx.h"
#include "libMPSSE_spi.h"
#include "ft232h.h"

#pragma comment(lib, "FTD2XX.lib")
#pragma comment(lib, "libMPSSE.lib")

namespace spi
{
	FT_HANDLE ftHandle;
	FT_STATUS status = FT_OK;

	ChannelConfig channelConf = { 0 };
	uint32 channels = 0;
	uint32 sizeTransferred;

	int open()
  {
		channelConf.ClockRate = 14000000;
		channelConf.LatencyTimer = 1;
		channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
		channelConf.Pin = 0xF0B0F0B0;		/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/
		channelConf.reserved = 0;
		
		Init_libMPSSE();

		status = SPI_GetNumChannels(&channels);
		if (status) return status;
		if (!channels) return -1;

		status = SPI_OpenChannel(0, &ftHandle);
		if (status) return status;

		status = SPI_InitChannel(ftHandle, &channelConf);
		return status;
  }

	void close()
	{
		Cleanup_libMPSSE();
	}

	int xfer(uint8_t *wr_buf, uint8_t *rd_buf, int len)
	{
		status = SPI_ReadWrite(ftHandle, rd_buf, wr_buf, len, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
		return status;
	}

	int xfer_byte(uint8_t &wr, uint8_t &rd)
	{
		status = SPI_ReadWrite(ftHandle, &rd, &wr, 1, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
		return status;
	}

	int set_ss(bool is_ss)
	{
		uint8_t d = 255;
		status = SPI_ReadWrite(ftHandle, &d, &d, 0, &sizeTransferred, is_ss ? SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE : SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
		return status;
	}
};
