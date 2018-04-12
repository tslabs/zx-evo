#include <avr/io.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "tape.h"

void tape_task(void)
{
  u8 temp = (TAPEIN_PIN & _BV(TAPEIN)) ? FLAG_LAST_TAPE_VALUE : 0;

  if ((flags_register & FLAG_LAST_TAPE_VALUE) ^ temp)
  {
    flags_register &= ~FLAG_LAST_TAPE_VALUE;
    flags_register |= temp;
    zx_set_config();
  }
}
