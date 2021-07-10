#include <stdio.h>
#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "rs232.h"
#include "zx.h"
#include "joystick.h"

//if want Log than comment next string
#undef LOGENABLE

static volatile u8 cur_state = 0;

void joystick_task(void)
{
	static u8 joy_state = 0;
	u8 temp = cur_state;

	if (joy_state ^ temp)
	{
		//change state of joystick pins
		joy_state = temp;

		//send to port
        zx_spi_send(SPI_KEMPSTON_JOYSTICK, joy_state);

#ifdef LOGENABLE
	char log_joystick[] = "JS..\r\n";
	log_joystick[2] = ((temp >> 4) <= 9)?'0'+(temp >> 4):'A'+(temp >> 4)-10;
	log_joystick[3] = ((temp & 0x0F) <= 9)?'0'+(temp & 0x0F):'A'+(temp & 0x0F)-10;
	to_log(log_joystick);
#endif
	}
}

void joystick_poll(void)
{
	u8 ext_port = JOYSTICK_EXT_PORT;
	cur_state = (ext_port & (1 << JOYSTICK_EXT_SEL))
		? (cur_state & ~(JOYSTICK_MASK | 0x20))
			| ((~JOYSTICK_PIN) & JOYSTICK_MASK)
			| ((~JOYSTICK_EXT_PIN << (5 - JOYSTICK_EXT_START_C)) & 0x20)
		: (cur_state & (JOYSTICK_MASK | 0x20))
			| ((~JOYSTICK_PIN << (6 - JOYSTICK_FIRE)) & 0x40)
			| ((~JOYSTICK_EXT_PIN << (7 - JOYSTICK_EXT_START_C)) & 0x80);
	JOYSTICK_EXT_PORT = ext_port ^ (1 << JOYSTICK_EXT_SEL);
}
