#ifndef __RTC_H__
#define __RTC_H__
/**
 * @file
 * @brief RTC support.
 * @author http://www.nedopc.com
 *
 * RTC PCF8583 support for ZX Evolution.
 *
 * ZX Evolution emulate Gluk clock standard:
 * - full read/write time emulate;
 * - full read/write nvram emulate;
 * - registers A,B,C,D read only;
 * - alarm functions not emulated.
 *
 * Save modes of ZX Evolution to RTC NVRAM.
 */

/** Address of PCF8583 RTC chip.*/
#define RTC_ADDRESS  0xA0

/** Register for year additional data. */
#define RTC_YEAR_ADD_REG     0xFF
/** Register for common modes. */
#define RTC_COMMON_MODE_REG  0xFE
/** Register for ps2mouse resolution. */
#define RTC_PS2MOUSE_RES_REG 0xFD

/** Init RTC.*/
void rtc_init(void);

/**
 * Write byte to RTC.
 * @param addr [in] - address of internal register on RTC
 * @param data [in] - data to write
 */
void rtc_write(u8 addr, u8 data);

/**
 * Read byte from RTC.
 * @return data
 * @param addr [in] - address of internal register on RTC
 */
u8 rtc_read(u8 addr);


/** Seconds register index. */
#define GLUK_REG_SEC        0x00
/** Seconds alarm register index. */
#define GLUK_REG_SEC_ALARM  0x01
/** Minutes register index. */
#define GLUK_REG_MIN        0x02
/** Minutes alarm register index. */
#define GLUK_REG_MIN_ALARM  0x03
/** Hours register index. */
#define GLUK_REG_HOUR       0x04
/** Hours alarm register index. */
#define GLUK_REG_HOUR_ALARM 0x05
/** Day of week register index. */
#define GLUK_REG_DAY_WEEK   0x06
/** Day of month register index. */
#define GLUK_REG_DAY_MONTH  0x07
/** Month register index. */
#define GLUK_REG_MONTH      0x08
/** Year register index. */
#define GLUK_REG_YEAR       0x09
/** A register index. */
#define GLUK_REG_A          0x0A
/** B register index. */
#define GLUK_REG_B          0x0B
/** C register index. */
#define GLUK_REG_C          0x0C
/** D register index. */
#define GLUK_REG_D          0x0D
/** E register index. (non-standard) */
#define GLUK_REG_E          0x0E

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
      /** PS/2 keyboard F12 key status. */
      u8 f12                 : 1;
    };
    u8 byte;
  };
} KB_CTRL_RTC_0_t;

typedef struct
{
  union
  {
    struct
    {
      /** PS/2 keyboard LWIN key status. */
      u8 lwin                : 1;
      /** PS/2 keyboard RWIN key status. */
      u8 rwin                : 1;
      /** PS/2 keyboard MENU key status. */
      u8 menu                : 1;
    };
    u8 byte;
  };
} KB_CTRL_RTC_1_t;

/** B register 2 bit - data mode (A 1 in DM signifies binary data while a 0 in DM specifies BCD data). */
#define GLUK_B_DATA_MODE      0x04
/** B register 1 bit - 24/12 mode (A 1 indicates the 24-hour mode and a 0 indicates the 12-hour mode). */
#define GLUK_B_24_12_MODE     0x02
/** C register 4 bit - Update-ended interrupt flag [UF] (Bit is set after each update cycle, UF is cleared by reading Register C or a RESET). */
#define GLUK_C_UPDATE_FLAG    0x10
/** C register 0 bit - unused in original, but in ZXEVO clear PS2 keyboard log on write. */
/** C register 0 bit - unused in original, but in ZXEVO NUM LED status of PS2 keyboard on read. */
#define GLUK_C_CLEAR_LOG_FLAG 0x01
#define GLUK_C_NUM_LED_FLAG   0x01
/** C register 1 bit - unused in original, but in ZXEVO switch CAPS LED mode on PS2 keyboard. */
#define GLUK_C_CAPS_LED_FLAG  0x02
/** C register 2 bit - unused in original, but in ZXEVO switch EEPROM mode on extra bytes (>0xF0). */
#define GLUK_C_EEPROM_FLAG    0x80

/** Initial value for Gluk A register. */
#define GLUK_A_INIT_VALUE   0x00
/** Initial value for Gluk B register. */
#define GLUK_B_INIT_VALUE   0x02
/** Initial value for Gluk C register. */
#define GLUK_C_INIT_VALUE   0x00
/** Initial value for Gluk D register. */
#define GLUK_D_INIT_VALUE   0x80

/** Read values from RTC and setup Gluk clock registers. */
void gluk_init(void);

/** Increment Gluk clock registers on one second. */
void gluk_inc(void);

/**
 * Get Gluk clock registers data.
 * @return registers data
 * @param index [in] - index of Gluck clock register
 */
u8 gluk_get_reg(u8 index);

/**
 * Set Gluk clock registers data.
 * @param index [in] - index of Gluck clock register
 * @param data [in] - data
 */
void gluk_set_reg(u8 index, u8 data);


#endif //__RTC_H__
