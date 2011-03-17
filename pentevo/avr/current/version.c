#include <avr/pgmspace.h>

#include "mytypes.h"
#include "main.h"

//base configuration version string pointer [far address of PROGMEM]
const ULONG baseVersionAddr = 0x1DFF0;

//bootloader version string pointer [far address of PROGMEM]
const ULONG bootVersionAddr = 0x1FFF0;

UBYTE GetVersionByte(UBYTE index)
{
	if ( index < 0x10 )
	{
		if ( flags_register & FLAG_VERSION_TYPE )
		{
			//bootloader version
			return (UBYTE)pgm_read_byte_far(bootVersionAddr+(ULONG)index);
		}
		else
		{
			//base configuration version
			return (UBYTE)pgm_read_byte_far(baseVersionAddr+(ULONG)index);
		}
	}
	return (UBYTE)0xFF;
}

void SetVersionType(UBYTE type)
{
	switch(type)
	{
		case 0:
			//base configuration
			flags_register &= ~FLAG_VERSION_TYPE;
			break;

		case 1:
			//bootloader
			flags_register |= FLAG_VERSION_TYPE;
			break;
	}
}
