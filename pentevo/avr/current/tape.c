#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "tape.h"

void tape_task(void)
{
	UBYTE temp = ( TAPEIN_PIN & (1<<TAPEIN) )? FLAG_LAST_TAPE_VALUE:0;
	if ( (flags_register&FLAG_LAST_TAPE_VALUE)^temp )
	{
		zx_set_config( (temp)?SPI_TAPE_FLAG:0 );
		if ( temp )
		{
			flags_register |= FLAG_LAST_TAPE_VALUE;
		}
		else
		{
			flags_register &= ~FLAG_LAST_TAPE_VALUE;
		}
	}
}
