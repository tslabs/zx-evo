#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "rs232.h"
#include "zx.h"
#include "joystick.h"

//if want Log than comment next string
#undef LOGENABLE

void joystick_task(void)
{
	static UBYTE joy_state = 0;
	UBYTE temp = (~JOYSTICK_PIN) & JOYSTICK_MASK;

	if ( joy_state ^ temp )
	{
		//change state of joystick pins
		joy_state = temp;

		//send to port
		zx_spi_send(SPI_KEMPSTON_JOYSTICK, joy_state, 0x7F);

#ifdef LOGENABLE
	char log_joystick[] = "JS..\r\n";
	log_joystick[2] = ((temp >> 4) <= 9 )?'0'+(temp >> 4):'A'+(temp >> 4)-10;
	log_joystick[3] = ((temp & 0x0F) <= 9 )?'0'+(temp & 0x0F):'A'+(temp & 0x0F)-10;
	to_log(log_joystick);
#endif
	}
}
