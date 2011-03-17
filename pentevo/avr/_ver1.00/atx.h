#ifndef __ATX_H__
#define __ATX_H__

/** Counter for atx power off delay. */
extern volatile UWORD atx_counter;

/** Wait for ATX power button on. */
void wait_for_atx_power(void);

/**
 * Check for atx power off switch.
 * @return 0 - if atx power off
 *         >0 - if atx power on
 */
UBYTE atx_power_task(void);

#endif //__ATX_H__
