
#include <avr/pgmspace.h>
#include "mytypes.h"
#include "main.h"
#include "ps2.h"
#include "spiflash.h"

//base configuration version string pointer [far address of PROGMEM]
const u32 baseVersionAddr = 0x1DFF0;

//bootloader version string pointer [far address of PROGMEM]
const u32 bootVersionAddr = 0x1FFF0;

u8 GetVersionByte(u8 index)
{
	index &= 0x0F;

	switch (ext_type_gluk)
	{
		case EXT_TYPE_BASECONF_VERSION:
			//base configuration version
			return (u8)pgm_read_byte_far(baseVersionAddr+(u32)index);

		case EXT_TYPE_BOOTLOADER_VERSION:
			//bootloader version
			return (u8)pgm_read_byte_far(bootVersionAddr+(u32)index);

		case EXT_TYPE_PS2KEYBOARDS_LOG:
			//PS2 keyboards log
			return ps2keyboard_from_log();

		case EXT_TYPE_RDCFG:
			// read config byte
			return (index == 0) ? modes_register : 0xFF;

    case EXT_TYPE_SPIFL:
      // read from SPI Flash interface
      return spi_flash_read(index);
	}
	return 0xFF;
}

void SetVersionType(u8 index, u8 type)
{
	index &= 0x0F;

    switch (ext_type_gluk)
    {
      case EXT_TYPE_SPIFL:
        // write to SPI Flash interface
        spi_flash_write(index, type);
      break;

      default:
        ext_type_gluk = type;
    }
}
