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


/** ZX all data for wait registers. */
#define SPI_WAIT_DATA  0x40
/** ZX Gluk address register. */
#define SPI_GLUK_ADDR  0x41
/** ZX Kondratiev's rs232 address register. */
#define SPI_RS232_ADDR 0x42

/** Send/recv data for spi registers. */
u8 zx_spi_send(u8 addr, u8 data, u8 mask);


/** Pause between (CS|SS) and not(CS|SS). */
#define SHIFT_PAUSE 8
/** Pause between (CS|SS) and not(CS|SS) counter. */
extern volatile u8 shift_pause;

// real keys bitmap. send order: LSbit first, from [4] to [0]
// [5]..[9] - received data
// [10] - end scan flag
extern u8 zx_realkbd[11];

/*struct zx {
	u8 counters[40];
	u8 map[5]; // send order: LSbit first, from [4] to [0]
	u8 reset_type;
};*/

/** PS/2 keyboard LCTRL key status. */
#define KB_LCTRL_MASK   0x01
/** PS/2 keyboard LCTRL key status. */
#define KB_RCTRL_MASK   0x02
/** PS/2 keyboard LALT key status. */
#define KB_LALT_MASK    0x04
/** PS/2 keyboard LALT key status. */
#define KB_RALT_MASK    0x08
/** PS/2 keyboard LEFT SHIFT key status. */
#define KB_LSHIFT_MASK  0x10
/** PS/2 keyboard RIGHT SHIFT key status. */
#define KB_RSHIFT_MASK  0x20
/** PS/2 keyboard F12 key status. */
#define KB_F12_MASK     0x40
/** PS/2 keyboard CTRL,ALT,DEL mapped status (set = mapped all keys). */
#define KB_CTRL_ALT_DEL_MAPPED_MASK 0x80
/** The _1 suffix in these defines indicate that they are in kb_ctrl_status[1] */
/** PS/2 keyboard WIN key status. */
#define KB_LWIN_MASK_1  0x01
/** PS/2 keyboard WIN key status. */
#define KB_RWIN_MASK_1  0x02
/** PS/2 keyboard MENU key status. */
#define KB_MENU_MASK_1  0x04
/** PS/2 keyboard control keys status (for additional functons). */
extern volatile u8 kb_ctrl_status[2];
/** PS/2 keyboard control keys mapped to zx keyboard (mapped keys not used in additional functions). */
extern volatile u8 kb_ctrl_mapped[2];


#define ZX_TASK_INIT 0
#define ZX_TASK_WORK 1

/**
 * Interchange via SPI.
 * @param operation [in] - operation type.
 */
void zx_task(u8 operation);

void zx_init(void);

void to_zx(u8 scancode, u8 was_E0, u8 was_release);

void update_keys(u8 zxcode, u8 was_release);

/** Clear zx keyboard buffers. */
void zx_clr_kb(void);


void  zx_fifo_put(u8 input);
u8 zx_fifo_isfull(void);
u8 zx_fifo_isempty(void);
u8 zx_fifo_get(void);
u8 zx_fifo_copy(void);

/**
 * ZX mouse button register.
 * Bits description:
 * 7..4 - wheel code (if present) or 1111 if wheel not present;
 * 3	- always 1;
 * 2	- middle button (0, if pressed);
 * 1	- right button (0, if pressed);
 * 0	- left button (0, if pressed).
 */
extern u8 zx_mouse_button;

/** ZX mouse X coordinate register. */
extern u8 zx_mouse_x;

/** ZX mouse Y coordinate register. */
extern u8 zx_mouse_y;

/**
 * Reset ZX mouse registers to default value.
 * @param enable [in] - 0: values like no mouse connected, other: values like mouse connected
 */
void zx_mouse_reset(u8 enable);

/** Send values of ZX mouse registers to fpga. */
void zx_mouse_task(void);


/** Wait port access mode R/nW */
#define ZXW_MODE 0x80

/** Kondratiev's modem ZX port out. */
#define ZXW_KONDR_RS232 0x02

/** Gluk clock ZX port out. */
#define ZXW_GLUK_CLOCK  0x01

/** Wait port sources mask */
#define ZXW_MASK (ZXW_GLUK_CLOCK | ZXW_KONDR_RS232)

/**
 * Work with WAIT ports.
 * @param status [in] - bit 7 - CPU is 0 -write, 1-read wait port
 *		                bits 6..0 is index of port
 */
void zx_wait_task(u8 status);

/**
 * Switch mode on ZX.
 * @param mode - mode flag
 */
void zx_mode_switcher(u8 mode);

/**
 * Set configuration register on zx.
 */
void zx_set_config();

#endif
