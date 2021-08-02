#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

/**
 * @file
 * @brief Kempston joystick support.
 * @author http://www.nedopc.com
 *
 * Kempston joystick support for ZX Evolution.
 *
 * Kempston joystick port bits (if bit set - button pressed):
 * - 0: right;
 * - 1: left;
 * - 2: down;
 * - 3: up;
 * - 4: fire (B on Sega gamepad);
 * - 5: add. fire (C on Sega gamepad);
 * - 6: (A on Sega gamepad);
 * - 7: (Start on Sega gamepad);
 */

typedef union {
    struct {
        u8  pad0        : 3;
        u8  pad1        : 3;
        u8  pad_mapping : 2;
    } b;
    u8 raw;
} joystick_mode_t;

enum {
    JOYSTICK_EEPROM_POS         = 512,
    JOYSTICK_EEPROM_MODE        = JOYSTICK_EEPROM_POS + 3,
    JOYSTICK_EEPROM_PAD0_MAP    = JOYSTICK_EEPROM_POS + 4,
    JOYSTICK_EEPROM_PAD1_MAP    = JOYSTICK_EEPROM_POS + 4 + 16*2,
};

// flags for joystick_flags
enum {
    JOYSTICK_FLAG_ACTIVE     = (1 << 0),
    JOYSTICK_FLAG_EEPROM_MAP = (1 << 1),
};

/** Joystick init */
void joystick_init(void);

/** Joystick set mode */
void joystick_set_mode(u8 mode);
u8   joystick_get_mode();

/** Joystick keymap functions */
u8 joystick_keymap_read(u8 joystick, u8 index);
void joystick_keymap_write(u8 joystick, u8 index, u8 data);

/** Kempston joystick task. */
void joystick_task(void);
void joystick_poll(void);

#endif //__JOYSTICK_H__
