#ifndef ZX_H
#define ZX_H

/**
 * @file
 * @brief Interchange with FPGA registers (ZX ports) via SPI.
 * @author http://www.nedopc.com
 *
 * Interchange with ZX ports:
 * - keyboard port (nowait);
 * - mouse port (nowait);
 * - kempstone joystick (nowait);
 * - gluk clock (wait).
 *
 * Configure internal FPGA registers (set modes):
 * - vga/tv mode;
 * - reset CPU.
 */

// key code is 7 bits, 8th bit is press/release (1=press,0=release)
//
// ACHTUNG!!!! DO NOT CHANGE THESE DEFINES, OTHERWISE MUCH OF CODE WILL BREAK!!!!
//
#define PRESS_BIT  7
#define PRESS_MASK 128
#define KEY_MASK   127


/** ZX keyboard data. */
#define SPI_KBD_DAT   0x10
/** ZX keyboard stop bit. */
#define SPI_KBD_STB   0x11

/** ZX mouse X coordinate register. */
#define SPI_MOUSE_X   0x20
/** ZX mouse Y coordinate register. */
#define SPI_MOUSE_Y   0x21
/** ZX mouse Y coordinate register. */
#define SPI_MOUSE_BTN 0x22

/** Kempston joystick register. */
#define SPI_KEMPSTON_JOYSTICK 0x23

/** ZX reset register. */
#define SPI_RST_REG   0x30

/** ZX configuration register. */
#define SPI_CONFIG_REG   0x50
/** ZX NMI bit flag of configuration register. */
#define SPI_CONFIG_NMI_FLAG 0x02
/** ZX $FE.D6 (tape in) bit flag of configuration register. */
#define SPI_TAPE_FLAG 0x04

/** ZX all data for wait registers. */
#define SPI_WAIT_DATA  0x40
/** ZX Gluk address register. */
#define SPI_GLUK_ADDR  0x41
/** ZX Kondratiev's rs232 address register. */
#define SPI_RS232_ADDR 0x42

/** Send/recv data for spi registers. */
UBYTE zx_spi_send(UBYTE addr, UBYTE data, UBYTE mask);


/** Pause between (CS|SS) and not(CS|SS). */
#define SHIFT_PAUSE 8
/** Pause between (CS|SS) and not(CS|SS) counter. */
extern volatile UBYTE shift_pause;

// real keys bitmap. send order: LSbit first, from [4] to [0]
// [5]..[9] - received data
// [10] - end scan flag
extern UBYTE zx_realkbd[11];

/*struct zx {
	UBYTE counters[40];
	UBYTE map[5]; // send order: LSbit first, from [4] to [0]
	UBYTE reset_type;
};*/

/** PS/2 keyboard CTRL key status. */
#define KB_CTRL_MASK   0x01
/** PS/2 keyboard ALT key status. */
#define KB_ALT_MASK    0x02
/** PS/2 keyboard LEFT SHIFT key status. */
#define KB_LSHIFT_MASK 0x04
/** PS/2 keyboard RIGHT SHIFT key status. */
#define KB_RSHIFT_MASK 0x08
/** PS/2 keyboard F12 key status. */
#define KB_F12_MASK    0x10
/** PS/2 keyboard CTRL,ALT,DEL mapped status (set = mapped all keys). */
#define KB_CTRL_ALT_DEL_MAPPED_MASK 0x80
/** PS/2 keyboard control keys status (for additional functons). */
extern volatile UBYTE kb_status;


#define ZX_TASK_INIT 0
#define ZX_TASK_WORK 1

/**
 * Interchange via SPI.
 * @param operation [in] - operation type.
 */
void zx_task(UBYTE operation);

void zx_init(void);

void to_zx(UBYTE scancode, UBYTE was_E0, UBYTE was_release);

void update_keys(UBYTE zxcode, UBYTE was_release);

/** Clear zx keyboard buffers. */
void zx_clr_kb(void);


void  zx_fifo_put(UBYTE input);
UBYTE zx_fifo_isfull(void);
UBYTE zx_fifo_isempty(void);
UBYTE zx_fifo_get(void);
UBYTE zx_fifo_copy(void);

/**
 * ZX mouse button register.
 * Bits description:
 * 7..4 - wheel code (if present) or 1111 if wheel not present;
 * 3	- always 1;
 * 2	- middle button (0, if pressed);
 * 1	- right button (0, if pressed);
 * 0	- left button (0, if pressed).
 */
extern volatile UBYTE zx_mouse_button;

/** ZX mouse X coordinate register. */
extern volatile UBYTE zx_mouse_x;

/** ZX mouse Y coordinate register. */
extern volatile UBYTE zx_mouse_y;

/**
 * Reset ZX mouse registers to default value.
 * @param enable [in] - 0: values like no mouse connected, other: values like mouse connected
 */
void zx_mouse_reset(UBYTE enable);

/** Send values of ZX mouse registers to fpga. */
void zx_mouse_task(void);


/** Gluk clock ZX port out. */
#define ZXW_GLUK_CLOCK  0x01

/** Kondratiev's modem ZX port out. */
#define ZXW_KONDR_RS232 0x02

/**
 * Work with WAIT ports.
 * @param status [in] - bit 7 - CPU is 0 -write, 1-read wait port
 *		                bits 6..0 is index of port
 */
void zx_wait_task(UBYTE status);

/** Switch vga mode on ZX */
void zx_vga_switcher(void);

/**
 * Set configuration register on zx.
 * @param flags [in] - bit 0: not used (depend from MODE_VGA on modes_register)
 *		               bit 1: NMI
 */
void zx_set_config(UBYTE flags);

#endif

