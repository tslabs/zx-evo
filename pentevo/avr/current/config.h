#ifndef CONFIG_H
#define CONFIG_H

#define FPGA_BASE 0
#define FPGA_TS   1
#define FPGA_EGG  2

#define EEPROM_ADDR_USER_KBMAP    ((u8*)0x0000)   // User map offset in EEPROM
#define EEPROM_ADDR_USER_KBMAP_E0 ((u8*)0x0100)   // User map (extent E0) offset in EEPROM
#define EEPROM_ADDR_BOOT_CFG      ((u8*)0x0200)   // Boot config offset in EEPROM
#define EEPROM_SIZE_BOOT_CFG      0x100           // Boot config size in EEPROM
#define EEPROM_ADDR_FPGA_CFG      ((u8*)0x0FFF)   // FPGA bitstream selector offset in EEPROM
#define BOOT_CFG_SIZE             0x100           // Boot config size

typedef enum
{
  CFG_TAG_START   = 0xFF,   // command to start new config
  CFG_TAG_END     = 0x00,   // 1. end of config marker, 2. command to finish config

  CFG_TAG_SIG     = 0x78,   // config signature
  CFG_TAG_VER     = 0x79,   // config version

  CFG_TAG_BSTREAM = 0x01,   // boot bitstream filename
  CFG_TAG_ROM     = 0x02,   // boot ROM filename
  CFG_TAG_ISBASE  = 0x03,   // is Baseconf
} CFG_TAG;

enum
{
  CFG_SIG         = 0x4345585A, // 'ZXEC' signature
  CFG_SIG_LEN     = 4,
  CFG_VER         = 1,
};

u8 cfg_get_field_eeprom(u8, void*);
u8 cfg_get_field(u8, void*, void*);
u8 cfg_get_field(u8, void*, void*, u8);

#endif
