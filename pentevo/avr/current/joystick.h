#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

/**
 * @file
 * @brief Kempstone joystick support.
 * @author http://www.nedopc.com
 *
 * Kempston joystick support for ZX Evolution.
 *
 * Kempston joystick port bits (if bit set - button pressed):
 * - 0: right;
 * - 1: left;
 * - 2: down;
 * - 3: up;
 * - 4: fire;
 * - 5-7: 0.
 */

/** Kempstone joystick task. */
void joystick_task(void);

#endif //__JOYSTICK_H__
