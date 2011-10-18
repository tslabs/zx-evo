#ifndef PS2_H
#define PS2_H

/**
 * @file
 * @brief PS/2 mouse and keyboard support.
 * @author http://www.nedopc.com
 *
 * PS/2 keyboard support (read only).
 *
 * PS/2 mouse support (read/write).
 * ZX Kempston mouse interface emulation.
 *
 * Support PS/2 mouses:
 * - "microsoft" mouses with wheel (4bytes data);
 * - classic mouses (3bytes data).
 */

/**
 * Decode received data.
 * @return decoded data.
 * @param count - counter.
 * @param shifter - received bits.
 */
UBYTE ps2_decode(UBYTE count, UWORD shifter);

/**
 * Encode (prepare) sended data.
 * @return encoded data.
 * @param data - data to send.
 */
UWORD ps2_encode(UBYTE data);

/** Timeout value for PS/2 keyboard. */
#define PS2KEYBOARD_TIMEOUT 20

/** Command to reset PS2 keyboard. */
#define PS2KEYBOARD_CMD_RESET 0xFF
/** Command to enable PS2 keyboard. */
#define PS2KEYBOARD_CMD_ENABLE 0xF4
/** Command to set leds on PS2 keyboard. */
#define PS2KEYBOARD_CMD_SETLED 0xED

/** "Caps Lock" led bit in set leds command on PS2 keyboard. */
#define PS2KEYBOARD_LED_CAPSLOCK 0x04
/** "Num Lock" led bit in set leds command on PS2 keyboard. */
#define PS2KEYBOARD_LED_NUMLOCK 0x02
/** "Scroll Lock" led bit in set leds command on PS2 keyboard. */
#define PS2KEYBOARD_LED_SCROLLOCK 0x01

/** Received PS/2 keyboard data register. */
extern volatile UWORD ps2keyboard_shifter;
/** Counter of current PS/2 keyboard data bit. */
extern volatile UBYTE ps2keyboard_count;
/** Timeout register for detecting PS/2 keyboard timeouts. */
extern volatile UBYTE ps2keyboard_timeout;
/** Counter of stages PS/2 keyboard command. */
extern volatile UBYTE ps2keyboard_cmd_count;
/** Current PS/2 keyboard command (0 - none). */
extern volatile UBYTE ps2keyboard_cmd;

/** PS2 keyboards log. */
extern volatile UBYTE  ps2keyboard_log[15];
/** PS2 keyboards log length (0xFF - overload). */
extern volatile UBYTE  ps2keyboard_log_len;

/** Reset PS2 keyboard log. */
void ps2keyboard_reset_log(void);

/**
 * Get data from PS2 keyboard log.
 * @return data byte (0 - log empty, 0xFF - log overload).
 */
UBYTE ps2keyboard_from_log(void);

/**
 * Send command to PS/2 keboard.
 * @param cmd [in] - command.
 */
void ps2keyboard_send_cmd(UBYTE cmd);

/** PS/2 keyboard task. */
void ps2keyboard_task(void);

/**
 * Parsing PS/2 keboard recived bytes .
 * @param recbyte [in] - received byte.
 */
void ps2keyboard_parse(UBYTE recbyte);

/** Timeout for waiting response from mouse. */
#define PS2MOUSE_TIMEOUT 20
/** Received/sended PS/2 mouse data register. */
extern volatile UWORD ps2mouse_shifter;
/** Counter of current PS/2 mouse data bit. */
extern volatile UBYTE ps2mouse_count;
/** Timeout register for detecting PS/2 mouse timeouts. */
extern volatile UBYTE ps2mouse_timeout;
/** Index of PS/2 mouse initialization step (@see ps2mouse_init_sequence). */
extern volatile UBYTE ps2mouse_initstep;
/** Counter of PS/2 mouse response bytes. */
extern volatile UBYTE ps2mouse_resp_count;
/** Current PS/2 keyboard command (0 - none). */
extern volatile UBYTE ps2mouse_cmd;

/** Command to reset PS2 mouse. */
#define PS2MOUSE_CMD_RESET          0xFF
/** Command get type of PS2 mouse. */
#define PS2MOUSE_CMD_GET_TYPE       0xF2
/** Command to set resolution PS2 mouse. */
#define PS2MOUSE_CMD_SET_RESOLUTION 0xE8

/** PS/2 mouse task. */
void ps2mouse_task(void);

/**
 * Set PS/2 mouse resolution.
 * If left and right mouse buttons and some keyboard button pressed then resolution set.
 * @param code [in] - control codes:
 *        <B>0x7C</B> (keypad '*') - set default resolution;
 *        <B>0x79</B> (keypad '+') - inc resolution;
 *        <B>0x7B</B> (keypad '-') - dec resolution.
 */
void ps2mouse_set_resolution(UBYTE code);

#endif //PS2_H

