#ifndef __CONFIG_INTERFACE_H__
#define __CONFIG_INTERFACE_H__

#include "mytypes.h"

#define FPGA_BASE 0
#define FPGA_TS   1
#define FPGA_EGG  2

#define ADDR_FPGA_CFG 0x0fff  // address in EEPROM

// Configuration Interface registers
enum
{
    CFGIF_REG_EXTSW         = 0x00,     // W
    CFGIF_REG_MODES_VIDEO   = 0x01,     // RW
    CFGIF_REG_MODES_MISC    = 0x02,     // RW
    CFGIF_REG_HOTKEYS       = 0x03,     // RW
    CFGIF_REG_PAD_MODE      = 0x04,     // RW
    CFGIF_REG_PAD_KEYMAP0   = 0x05,     // RW
    CFGIF_REG_PAD_KEYMAP1   = 0x06,     // RW
    CFGIF_REG_PAD_AUTOFIRE0 = 0x07,     // RW
    CFGIF_REG_PAD_AUTOFIRE1 = 0x08,     // RW
    CFGIF_REG_PROTECT       = 0x0E,     // RW
    CFGIF_REG_COMMAND       = 0x0F,     // W
    CFGIF_REG_STATUS        = 0x0F,     // R
};

// CFGIF_REG_MODES_VIDEO flags
enum {
    CFGIF_MODES_VSYNC_POSITIVE  = (0 << 7),
    CFGIF_MODES_VSYNC_NEGATIVE  = (1 << 7),

    CFGIF_MODES_HSYNC_POSITIVE  = (0 << 6),
    CFGIF_MODES_HSYNC_NEGATIVE  = (1 << 6),

    CFGIF_MODES_ULA_PENTAGON    = (0 << 4),
    CFGIF_MODES_ULA_60HZ        = (1 << 4),
    CFGIF_MODES_ULA_48K         = (2 << 4),
    CFGIF_MODES_ULA_128K        = (3 << 4),

    CFGIF_MODES_TV              = (0 << 0),
    CFGIF_MODES_VGA             = (1 << 0),
};

// CFGIF_REG_MODES_MISC flags
enum {
    CFGIF_MODES_MUX_BEEPER      = (0 << 1),
    CFGIF_MODES_MUX_TAPEOUT     = (1 << 1),
    CFGIF_MODES_MUX_TAPEIN      = (2 << 1),

    CFGIF_MODES_FLOPPY_NOSWAP   = (0 << 0),
    CFGIF_MODES_FLOPPY_SWAP     = (1 << 0),
};

// CFGIF_REG_PAD_MODE flags
enum {
    CFGIF_PAD_MODE_KEMPSTON         = (0 << 0),
    CFGIF_PAD_MODE_NONE             = (0 << 0),
    CFGIF_PAD_MODE_NES              = (1 << 0),
    CFGIF_PAD_MODE_SEGA             = (2 << 0),
    CFGIF_PAD_MODE_SEGA6BUTTON      = (3 << 0),
    CFGIF_PAD_MODE_LAST             = CFGIF_PAD_MODE_SEGA,

    CFGIF_PAD_MAPPING_KEMPSTON_KEYS = (0 << 0),
    CFGIF_PAD_MAPPING_KEYS_KEMPSTON = (1 << 0),
    CFGIF_PAD_MAPPING_KEYS_KEYS     = (2 << 0),
};

enum {
    CFGIF_PAD_MAPPING_KEY_MASK      = (1 << 6) - 1,

    CFGIF_PAD_MAPPING_MOD_CS        = (1 << 6),
    CFGIF_PAD_MAPPING_MOD_SS        = (1 << 7),

    CFGIF_PAD_MAPPING_NO_KEY        = 0xFF,
};

enum {
    CFGIF_PAD_KEY_RIGHT     = 0,
    CFGIF_PAD_KEY_LEFT,
    CFGIF_PAD_KEY_UP,
    CFGIF_PAD_KEY_DOWN,
    CFGIF_PAD_KEY_B,
    CFGIF_PAD_KEY_C,
    CFGIF_PAD_KEY_A,
    CFGIF_PAD_KEY_START,
    CFGIF_PAD_KEY_MODE,
    CFGIF_PAD_KEY_X,
    CFGIF_PAD_KEY_Y,
    CFGIF_PAD_KEY_Z,
};

// CFGIF_REG_PROTECT flags
enum {
    CFGIF_PROTECT_ENABLE    = (1 << 7),
};

// CFGIF_REG_COMMAND commands
enum {
    CFGIF_CMD_REBOOT        = 0xF7,
    CFGIF_CMD_REBOOT_FLASH  = 0xFE,
};

u8 config_interface_read(u8 index);
void config_interface_write(u8 index, u8 data);


// Unified EEPROM TLV config block
#define EEPROM_ADDR_BOOT_CFG ((u8*)0x0200)
#define EEPROM_SIZE_BOOT_CFG 0x0400
#define BOOT_CFG_SIZE 0x0400
#define CFG_BOOT_NAME_SIZE 65

// TLV config tags
enum
{
  CFG_TAG_END     = 0x00,
  CFG_TAG_SIG     = 0x78,
  CFG_TAG_VER     = 0x79,
  CFG_TAG_BSTREAM = 0x01,
  CFG_TAG_ROM     = 0x02,
  CFG_TAG_ISBASE  = 0x03,
  CFG_TAG_JOYSTICK = 0x10,

  CFG_TAG_REP_COMMAND     = 0x80,
  CFG_TAG_REP_PROGRESS    = 0x81,
  CFG_TAG_REP_ADDRESS     = 0x82,
  CFG_TAG_REP_BLOCKS      = 0x83,
  CFG_TAG_REP_BLOCKS_GOOD = 0x84,
};

enum
{
  CFG_SIG0 = 'Z',
  CFG_SIG1 = 'X',
  CFG_SIG2 = 'E',
  CFG_SIG3 = 'C',
  CFG_VER  = 1,
};

// Joystick TLV payload offsets. Payload keeps old 40-byte JOY format.
enum
{
  CFG_JOY_SIZE = 40,
  CFG_JOY_SIG0 = 0,
  CFG_JOY_SIG1 = 1,
  CFG_JOY_SIG2 = 2,
  CFG_JOY_MODE = 3,
  CFG_JOY_PAD0_MAP = 4,
  CFG_JOY_PAD1_MAP = 20,
  CFG_JOY_PAD0_AUTOFIRE = 36,
  CFG_JOY_PAD1_AUTOFIRE = 38,
};

typedef struct
{
  u8 *addr;
  u16 size;
  u16 max_size;
} cfg_builder_t;

u8 cfg_config_valid();
u8 cfg_get_field_eeprom(u8 tag_req, void *addr);
u8 cfg_get_field_eeprom(u8 tag_req, void *addr, u8 maxlen);
u8 cfg_get_field(u8 tag_req, void *conf, void *addr);
u8 cfg_get_field(u8 tag_req, void *conf, void *addr, u8 maxlen);
u8 cfg_get_eeprom_record_count();
u16 cfg_get_eeprom_used_size();
u8 cfg_get_boot_bitstream(char *name, u8 maxlen);
u8 cfg_set_boot_bitstream(const char *name);
u8 cfg_get_boot_rom(char *name, u8 maxlen);
u8 cfg_get_isbase(u8 *isbase);
u8 cfg_set_isbase(u8 isbase);

void cfg_builder_start(cfg_builder_t *builder, void *addr, u16 max_size);
u8 cfg_builder_add(cfg_builder_t *builder, u8 tag, const void *addr_src, u8 len);
u16 cfg_builder_end(cfg_builder_t *builder);

u8 cfg_joy_available();
u8 cfg_joy_read(u8 offset);
void cfg_joy_write(u8 offset, u8 data);
void cfg_joy_init(u8 mode);

#endif