#ifndef __ATX_H__
#define __ATX_H__

/**
 * @file
 * @brief ATX power support.
 * @author http://www.nedopc.com
 *
 * ATX power switching:
 * - switch on ATX power on start by press "soft reset" button;
 * - switch off ATX power by press "soft reset" button on 5 seconds.
 */

/** Counter for atx power off delay. */
extern volatile UWORD atx_counter;

/** Wait for press "soft reset" to ATX power on. */
void wait_for_atx_power(void);

/**
 * Check for atx power off switch.
 */
void atx_power_task(void);

#endif //__ATX_H__
