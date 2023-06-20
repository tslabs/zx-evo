#ifndef __CONFIG_INTERFACE_H__
#define __CONFIG_INTERFACE_H__

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

#endif