#include <windows.h>
#include "std.h"
#include "ftd2xx.h"
#include "libMPSSE_spi.h"
#include "ft232h.h"

namespace spi
{
	typedef void (__cdecl *INIT_LIBMPSSE)();
	typedef FT_STATUS (__cdecl *SPI_GET_NUM_CHANNELS)(uint32 *numChannels);
	typedef FT_STATUS (__cdecl *SPI_OPEN_CHANNEL)(uint32 index, FT_HANDLE *handle);
	typedef FT_STATUS (__cdecl *SPI_INIT_CHANNEL)(FT_HANDLE handle, ChannelConfig *config);
	typedef FT_STATUS (__cdecl *SPI_CLOSE_CHANNEL)(FT_HANDLE handle);
	typedef FT_STATUS (__cdecl *SPI_READ_WRITE)(FT_HANDLE handle, uint8 *inBuffer, uint8 *outBuffer, uint32 sizeToTransfer, uint32 *sizeTransferred, uint32 transferOptions);
	typedef void (__cdecl *CLEANUP_LIBMPSSE)();

	HMODULE mpsse_dll;
	INIT_LIBMPSSE init_libmpsse;
	SPI_GET_NUM_CHANNELS spi_get_num_channels;
	SPI_OPEN_CHANNEL spi_open_channel;
	SPI_INIT_CHANNEL spi_init_channel;
	SPI_CLOSE_CHANNEL spi_close_channel;
	SPI_READ_WRITE spi_read_write;
	CLEANUP_LIBMPSSE cleanup_libmpsse;

	FT_HANDLE ftHandle;
	FT_STATUS status = FT_OK;

	ChannelConfig channelConf = { 0 };
	uint32 channels = 0;
	uint32 sizeTransferred;

	bool load_mpsse()
	{
		if (mpsse_dll)
			return true;

		HMODULE dll = LoadLibrary("libmpsse.dll");
		if (!dll)
			return false;

		init_libmpsse = (INIT_LIBMPSSE)GetProcAddress(dll, "Init_libMPSSE");
		spi_get_num_channels = (SPI_GET_NUM_CHANNELS)GetProcAddress(dll, "SPI_GetNumChannels");
		spi_open_channel = (SPI_OPEN_CHANNEL)GetProcAddress(dll, "SPI_OpenChannel");
		spi_init_channel = (SPI_INIT_CHANNEL)GetProcAddress(dll, "SPI_InitChannel");
		spi_close_channel = (SPI_CLOSE_CHANNEL)GetProcAddress(dll, "SPI_CloseChannel");
		spi_read_write = (SPI_READ_WRITE)GetProcAddress(dll, "SPI_ReadWrite");
		cleanup_libmpsse = (CLEANUP_LIBMPSSE)GetProcAddress(dll, "Cleanup_libMPSSE");

		if (!init_libmpsse || !spi_get_num_channels || !spi_open_channel || !spi_init_channel ||
			!spi_close_channel || !spi_read_write || !cleanup_libmpsse)
		{
			FreeLibrary(dll);
			init_libmpsse = 0;
			spi_get_num_channels = 0;
			spi_open_channel = 0;
			spi_init_channel = 0;
			spi_close_channel = 0;
			spi_read_write = 0;
			cleanup_libmpsse = 0;
			return false;
		}

		mpsse_dll = dll;
		return true;
	}

	int open()
  {
		if (!load_mpsse())
			return -2;

		ftHandle = 0;
		channelConf.ClockRate = 14000000;
		channelConf.LatencyTimer = 1;
		channelConf.configOptions = SPI_CONFIG_OPTION_MODE0 | SPI_CONFIG_OPTION_CS_DBUS3 | SPI_CONFIG_OPTION_CS_ACTIVELOW;
		channelConf.Pin = 0xF0B0F0B0;		/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/
		channelConf.reserved = 0;
		
		init_libmpsse();

		status = spi_get_num_channels(&channels);
		if (status) return status;
		if (!channels) return -1;

		status = spi_open_channel(0, &ftHandle);
		if (status) return status;

		status = spi_init_channel(ftHandle, &channelConf);
		if (status)
		{
			spi_close_channel(ftHandle);
			ftHandle = 0;
		}
		return status;
  }

	void close()
	{
		if (ftHandle && spi_close_channel)
			spi_close_channel(ftHandle);
		ftHandle = 0;

		if (mpsse_dll)
		{
			cleanup_libmpsse();
			FreeLibrary(mpsse_dll);
			mpsse_dll = 0;
		}
	}

	int xfer(uint8_t *wr_buf, uint8_t *rd_buf, int len)
	{
		if (!ftHandle || !spi_read_write)
			return -1;

		status = spi_read_write(ftHandle, rd_buf, wr_buf, len, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
		return status;
	}

	int xfer_byte(uint8_t &wr, uint8_t &rd)
	{
		if (!ftHandle || !spi_read_write)
			return -1;

		status = spi_read_write(ftHandle, &rd, &wr, 1, &sizeTransferred, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES);
		return status;
	}

	int set_ss(bool is_ss)
	{
		if (!ftHandle || !spi_read_write)
			return -1;

		uint8_t d = 255;
		status = spi_read_write(ftHandle, &d, &d, 0, &sizeTransferred, is_ss ? SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE : SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
		return status;
	}
};
