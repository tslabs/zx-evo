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

/** ZX data for wait registers. */
#define SPI_WAIT_DATA  0x40
/** ZX address for wait registers. */
#define SPI_WAIT_ADDR  0x41

/** Send/recv data for spi registers. */
u8 zx_spi_send(u8 addr, u8 data);

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

typedef struct
{
  union
  {
    struct
    {
      /** PS/2 keyboard LCTRL key status. */
      u8 lctrl               : 1;
      /** PS/2 keyboard RCTRL key status. */
      u8 rctrl               : 1;
      /** PS/2 keyboard LALT key status. */
      u8 lalt                : 1;
      /** PS/2 keyboard RALT key status. */
      u8 ralt                : 1;
      /** PS/2 keyboard LSHIFT key status. */
      u8 lshift              : 1;
      /** PS/2 keyboard RSHIFT key status. */
      u8 rshift              : 1;
      /** PS/2 keyboard LWIN key status. */
      u8 lwin                : 1;
      /** PS/2 keyboard RWIN key status. */
      u8 rwin                : 1;
      /** PS/2 keyboard MENU key status. */
      u8 menu                : 1;
      /** PS/2 keyboard F12 key status. */
      u8 f12                 : 1;
      /** PS/2 keyboard CTRL,ALT,DEL mapped status (set = mapped all keys). */
      u8 ctrl_alt_del_mapped : 1;
    };
    u16 all;
  };
} KB_CTRL_STATUS_t;

// callbacks for functional keys
extern void (*cb_ctrl_alt_del)();
extern void (*cb_ctrl_alt_f12)();
extern void (*cb_prt_scr)();
extern void (*cb_num_lock)();
extern void (*cb_scr_lock)();
extern void (*cb_menu_f1)();
extern void (*cb_menu_f2)();
extern void (*cb_menu_f3)();
extern void (*cb_menu_f4)();
extern void (*cb_menu_f5)();
extern void (*cb_menu_f6)();
extern void (*cb_menu_f7)();
extern void (*cb_menu_f8)();
extern void (*cb_menu_f9)();
extern void (*cb_menu_f10)();
extern void (*cb_menu_f11)();

// platform dependent config register setter
extern void (*cb_zx_set_config)();

/** PS/2 keyboard control keys status (for additional functons). */
extern volatile KB_CTRL_STATUS_t kb_ctrl_status;
/** PS/2 keyboard control keys mapped to zx keyboard (mapped keys not used in additional functions). */
extern volatile KB_CTRL_STATUS_t kb_ctrl_mapped;


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
void zx_wait_task();
void zx_wait_task_old();

/**
 * Switch mode on ZX.
 * @param mode - mode flag
 */
void zx_mode_switcher(u8 mode);

/**
 * Set configuration register on zx.
 */
void zx_set_config_base();
void zx_set_config_ts();
void zx_set_config(u8);

/**
 * Assert reser/reboot form SD card
 */
void func_reset();
void func_flash();

#endif
