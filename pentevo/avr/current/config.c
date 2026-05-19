#include <stdio.h>
#include <avr/io.h>

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "joystick.h"
#include "config.h"

// current joystick mapping position
u8 joymap_pos = 0;
u8 prev_index = 0;

u8 cfg_protect = 0;

u8 cfg_joy_find(u16 *offset);
u8 cfg_ensure_boot_fields();

u8 cfg_min_u8(u8 a, u8 b)
{
  if (a < b) return a;
  return b;
}

u8 cfg_read_byte(u16 offset)
{
  return eeprom_read_byte(EEPROM_ADDR_BOOT_CFG + offset);
}

void cfg_write_byte(u16 offset, u8 data)
{
  eeprom_busy_wait();
  eeprom_update_byte(EEPROM_ADDR_BOOT_CFG + offset, data);
}

u8 cfg_find_eeprom_field(u8 tag_req, u16 *value_offset, u8 *value_len)
{
  u16 ptr = 0;

  while (ptr < EEPROM_SIZE_BOOT_CFG)
  {
    u8 tag = cfg_read_byte(ptr++);

    if (tag == CFG_TAG_END) return 0;
    if (ptr >= EEPROM_SIZE_BOOT_CFG) return 0;

    u8 len = cfg_read_byte(ptr++);
    if ((u16)(ptr + len) > EEPROM_SIZE_BOOT_CFG) return 0;

    if (tag == tag_req)
    {
      *value_offset = ptr;
      *value_len = len;
      return 1;
    }

    ptr += len;
  }

  return 0;
}

u8 cfg_find_eeprom_end(u16 *end_offset)
{
  u16 ptr = 0;

  while (ptr < EEPROM_SIZE_BOOT_CFG)
  {
    u8 tag = cfg_read_byte(ptr++);

    if (tag == CFG_TAG_END)
    {
      *end_offset = ptr - 1;
      return 1;
    }

    if (ptr >= EEPROM_SIZE_BOOT_CFG) return 0;

    u8 len = cfg_read_byte(ptr++);
    if ((u16)(ptr + len) > EEPROM_SIZE_BOOT_CFG) return 0;

    ptr += len;
  }

  return 0;
}

u8 cfg_get_field_eeprom(u8 tag_req, void *addr)
{
  return cfg_get_field_eeprom(tag_req, addr, 0);
}

u8 cfg_get_field_eeprom(u8 tag_req, void *addr, u8 maxlen)
{
  u16 value_offset;
  u8 len;
  u8 copy_len;

  if (cfg_find_eeprom_field(tag_req, &value_offset, &len) == 0) return 0;

  copy_len = len;
  if (maxlen) copy_len = cfg_min_u8(copy_len, maxlen);

  eeprom_read_block(addr, EEPROM_ADDR_BOOT_CFG + value_offset, copy_len);
  return copy_len;
}

u8 cfg_get_field(u8 tag_req, void *conf, void *addr)
{
  return cfg_get_field(tag_req, conf, addr, 0);
}

u8 cfg_get_field(u8 tag_req, void *conf, void *addr, u8 maxlen)
{
  u8 *cfg = (u8*)conf;
  u16 ptr = 0;

  while (ptr < BOOT_CFG_SIZE)
  {
    u8 tag = cfg[ptr++];

    if (tag == CFG_TAG_END) return 0;
    if (ptr >= BOOT_CFG_SIZE) return 0;

    u8 len = cfg[ptr++];
    if ((u16)(ptr + len) > BOOT_CFG_SIZE) return 0;

    if (tag == tag_req)
    {
      u8 copy_len = len;
      if (maxlen) copy_len = cfg_min_u8(copy_len, maxlen);
      memcpy(addr, &cfg[ptr], copy_len);
      return copy_len;
    }

    ptr += len;
  }

  return 0;
}

void cfg_builder_start(cfg_builder_t *builder, void *addr, u16 max_size)
{
  builder->addr = (u8*)addr;
  builder->size = 0;
  builder->max_size = max_size;
}

u8 cfg_builder_add(cfg_builder_t *builder, u8 tag, const void *addr_src, u8 len)
{
  if (tag == CFG_TAG_END)
  {
    if ((u16)(builder->size + 1) > builder->max_size) return 0;
    builder->addr[builder->size++] = tag;
    return 1;
  }

  if ((u16)(builder->size + 2 + len) > builder->max_size) return 0;

  builder->addr[builder->size++] = tag;
  builder->addr[builder->size++] = len;
  memcpy(&builder->addr[builder->size], addr_src, len);
  builder->size += len;

  return 1;
}

u16 cfg_builder_end(cfg_builder_t *builder)
{
  cfg_builder_add(builder, CFG_TAG_END, 0, 0);
  return builder->size;
}

u8 cfg_joy_default_byte(u8 offset, u8 mode)
{
  if (offset == CFG_JOY_SIG0) return 'J';
  if (offset == CFG_JOY_SIG1) return 'O';
  if (offset == CFG_JOY_SIG2) return 'Y';
  if (offset == CFG_JOY_MODE) return mode;
  if ((offset >= CFG_JOY_PAD0_MAP) && (offset < CFG_JOY_PAD1_MAP + 16)) return CFGIF_PAD_MAPPING_NO_KEY;
  return 0;
}

void cfg_write_field_header(u16 *ptr, u8 tag, u8 len)
{
  cfg_write_byte((*ptr)++, tag);
  cfg_write_byte((*ptr)++, len);
}

void cfg_write_name_field(u16 *ptr, u8 tag)
{
  u8 i;

  cfg_write_field_header(ptr, tag, CFG_BOOT_NAME_SIZE);
  for (i = 0; i < CFG_BOOT_NAME_SIZE; i++) cfg_write_byte((*ptr)++, 0);
}

void cfg_write_default_config_joy(u8 joy_mode, const u8 *joy_data)
{
  u16 ptr = 0;
  u8 i;

  cfg_write_field_header(&ptr, CFG_TAG_SIG, 4);
  cfg_write_byte(ptr++, CFG_SIG0);
  cfg_write_byte(ptr++, CFG_SIG1);
  cfg_write_byte(ptr++, CFG_SIG2);
  cfg_write_byte(ptr++, CFG_SIG3);

  cfg_write_field_header(&ptr, CFG_TAG_VER, 1);
  cfg_write_byte(ptr++, CFG_VER);

  cfg_write_name_field(&ptr, CFG_TAG_BSTREAM);
  cfg_write_name_field(&ptr, CFG_TAG_ROM);

  cfg_write_field_header(&ptr, CFG_TAG_ISBASE, 1);
  cfg_write_byte(ptr++, 1);

  cfg_write_field_header(&ptr, CFG_TAG_JOYSTICK, CFG_JOY_SIZE);
  for (i = 0; i < CFG_JOY_SIZE; i++)
  {
    if (joy_data) cfg_write_byte(ptr++, joy_data[i]);
    else cfg_write_byte(ptr++, cfg_joy_default_byte(i, joy_mode));
  }

  cfg_write_byte(ptr++, CFG_TAG_END);
}

void cfg_write_default_config(u8 joy_mode)
{
  cfg_write_default_config_joy(joy_mode, 0);
}

u8 cfg_config_valid()
{
  u16 offset;
  u8 len;

  if (cfg_find_eeprom_field(CFG_TAG_SIG, &offset, &len) == 0) return 0;
  if (len != 4) return 0;
  if (cfg_read_byte(offset + 0) != CFG_SIG0) return 0;
  if (cfg_read_byte(offset + 1) != CFG_SIG1) return 0;
  if (cfg_read_byte(offset + 2) != CFG_SIG2) return 0;
  if (cfg_read_byte(offset + 3) != CFG_SIG3) return 0;

  if (cfg_find_eeprom_field(CFG_TAG_VER, &offset, &len) == 0) return 0;
  if (len != 1) return 0;
  if (cfg_read_byte(offset) != CFG_VER) return 0;

  return 1;
}

u8 cfg_get_eeprom_record_count()
{
  u16 ptr = 0;
  u8 count = 0;

  if (cfg_config_valid() == 0) return 0;

  while (ptr < EEPROM_SIZE_BOOT_CFG)
  {
    u8 tag = cfg_read_byte(ptr++);

    if (tag == CFG_TAG_END) return count;
    if (ptr >= EEPROM_SIZE_BOOT_CFG) return 0;

    u8 len = cfg_read_byte(ptr++);
    if ((u16)(ptr + len) > EEPROM_SIZE_BOOT_CFG) return 0;

    count++;
    ptr += len;
  }

  return 0;
}

u16 cfg_get_eeprom_used_size()
{
  u16 end_offset;

  if (cfg_config_valid() == 0) return 0;
  if (cfg_find_eeprom_end(&end_offset) == 0) return 0;
  return end_offset + 1;
}

u8 cfg_get_eeprom_field_len(u8 tag_req, u8 *field_len)
{
  u16 offset;

  if (cfg_config_valid() == 0) return 0;
  if (cfg_find_eeprom_field(tag_req, &offset, field_len) == 0) return 0;
  return 1;
}

u8 cfg_get_string_field_eeprom(u8 tag_req, char *name, u8 maxlen)
{
  u16 offset;
  u8 len;
  u8 i;
  u8 limit;
  u8 ch;

  if (maxlen == 0) return 0;
  name[0] = 0;
  if (cfg_config_valid() == 0) return 0;
  if (cfg_find_eeprom_field(tag_req, &offset, &len) == 0) return 0;

  limit = len;
  if (limit >= maxlen) limit = maxlen - 1;

  for (i = 0; i < limit; i++)
  {
    ch = cfg_read_byte(offset + i);
    name[i] = (char)ch;
    if (ch == 0) return 1;
  }

  name[limit] = 0;
  return 1;
}

u8 cfg_get_boot_bitstream(char *name, u8 maxlen)
{
  return cfg_get_string_field_eeprom(CFG_TAG_BSTREAM, name, maxlen);
}

u8 cfg_get_boot_rom(char *name, u8 maxlen)
{
  return cfg_get_string_field_eeprom(CFG_TAG_ROM, name, maxlen);
}

u8 cfg_get_isbase(u8 *isbase)
{
  u16 offset;
  u8 len;

  if (cfg_config_valid() == 0) return 0;
  if (cfg_find_eeprom_field(CFG_TAG_ISBASE, &offset, &len) == 0) return 0;
  if (len == 0) return 0;
  *isbase = cfg_read_byte(offset);
  return 1;
}

u8 cfg_set_isbase(u8 isbase)
{
  u16 offset;
  u8 len;

  if (cfg_ensure_boot_fields() == 0) return 0;
  if (cfg_find_eeprom_field(CFG_TAG_ISBASE, &offset, &len) == 0) return 0;
  if (len != 1) return 0;
  cfg_write_byte(offset, isbase ? 1 : 0);
  return 1;
}

void cfg_read_or_default_joy(u8 *joy_data, u8 joy_mode)
{
  u16 offset;
  u8 i;

  if (cfg_joy_find(&offset))
  {
    for (i = 0; i < CFG_JOY_SIZE; i++) joy_data[i] = cfg_read_byte(offset + i);
    return;
  }

  for (i = 0; i < CFG_JOY_SIZE; i++) joy_data[i] = cfg_joy_default_byte(i, joy_mode);
}

u8 cfg_ensure_boot_fields()
{
  u8 len;
  u8 joy_data[CFG_JOY_SIZE];

  if ((cfg_config_valid() == 0) ||
      (cfg_get_eeprom_field_len(CFG_TAG_BSTREAM, &len) == 0) ||
      (len != CFG_BOOT_NAME_SIZE) ||
      (cfg_get_eeprom_field_len(CFG_TAG_ROM, &len) == 0) ||
      (len != CFG_BOOT_NAME_SIZE) ||
      (cfg_get_eeprom_field_len(CFG_TAG_ISBASE, &len) == 0) ||
      (len != 1) ||
      (cfg_joy_available() == 0))
  {
    cfg_read_or_default_joy(joy_data, joystick_get_mode());
    cfg_write_default_config_joy(joystick_get_mode(), joy_data);
  }

  return cfg_config_valid();
}

u8 cfg_set_string_field_eeprom(u8 tag_req, const char *name)
{
  u16 offset;
  u8 len;
  u8 i;
  u8 ch;

  for (i = 0; i < CFG_BOOT_NAME_SIZE; i++)
  {
    if (name[i] == 0) break;
  }

  if (i >= CFG_BOOT_NAME_SIZE) return 0;

  if (cfg_ensure_boot_fields() == 0) return 0;
  if (cfg_find_eeprom_field(tag_req, &offset, &len) == 0) return 0;
  if (len != CFG_BOOT_NAME_SIZE) return 0;

  for (i = 0; i < CFG_BOOT_NAME_SIZE; i++) cfg_write_byte(offset + i, 0);

  for (i = 0; i < (CFG_BOOT_NAME_SIZE - 1); i++)
  {
    ch = (u8)name[i];
    if (ch == 0) return 1;
    cfg_write_byte(offset + i, ch);
  }

  return 1;
}

u8 cfg_set_boot_bitstream(const char *name)
{
  return cfg_set_string_field_eeprom(CFG_TAG_BSTREAM, name);
}

u8 cfg_append_eeprom_field(u8 tag, u8 len, u8 joy_mode)
{
  u16 end_offset;
  u16 ptr;
  u8 i;

  if (cfg_find_eeprom_end(&end_offset) == 0) return 0;
  if ((u16)(end_offset + 2 + len + 1) > EEPROM_SIZE_BOOT_CFG) return 0;

  ptr = end_offset;
  cfg_write_field_header(&ptr, tag, len);

  if (tag == CFG_TAG_JOYSTICK)
  {
    for (i = 0; i < len; i++) cfg_write_byte(ptr++, cfg_joy_default_byte(i, joy_mode));
  }
  else
  {
    for (i = 0; i < len; i++) cfg_write_byte(ptr++, 0);
  }

  cfg_write_byte(ptr++, CFG_TAG_END);
  return 1;
}

u8 cfg_joy_find(u16 *offset)
{
  u8 len;

  if (cfg_config_valid() == 0) return 0;
  if (cfg_find_eeprom_field(CFG_TAG_JOYSTICK, offset, &len) == 0) return 0;
  if (len != CFG_JOY_SIZE) return 0;
  if (cfg_read_byte(*offset + CFG_JOY_SIG0) != 'J') return 0;
  if (cfg_read_byte(*offset + CFG_JOY_SIG1) != 'O') return 0;
  if (cfg_read_byte(*offset + CFG_JOY_SIG2) != 'Y') return 0;

  return 1;
}

u8 cfg_joy_available()
{
  u16 offset;
  return cfg_joy_find(&offset);
}

void cfg_joy_init(u8 mode)
{
  u16 offset;
  u8 i;

  if (cfg_config_valid() == 0)
  {
    cfg_write_default_config(mode);
    return;
  }

  if (cfg_joy_find(&offset))
  {
    for (i = 0; i < CFG_JOY_SIZE; i++) cfg_write_byte(offset + i, cfg_joy_default_byte(i, mode));
    return;
  }

  if (cfg_append_eeprom_field(CFG_TAG_JOYSTICK, CFG_JOY_SIZE, mode) == 0) cfg_write_default_config(mode);
}

u8 cfg_joy_read(u8 offset)
{
  u16 value_offset;

  if (offset >= CFG_JOY_SIZE) return 0xFF;
  if (cfg_joy_find(&value_offset) == 0) return 0xFF;

  return cfg_read_byte(value_offset + offset);
}

void cfg_joy_write(u8 offset, u8 data)
{
  u16 value_offset;

  if (offset >= CFG_JOY_SIZE) return;
  if (cfg_joy_find(&value_offset) == 0) cfg_joy_init(0);
  if (cfg_joy_find(&value_offset) == 0) return;

  cfg_write_byte(value_offset + offset, data);
}

void config_interface_command(u8 data)
{
  switch (data)
  {
  case CFGIF_CMD_REBOOT:
    func_reset();
    break;

  case CFGIF_CMD_REBOOT_FLASH:
    func_flash();
    break;

  default:
    break;
  }
}


u8 config_interface_read(u8 index)
{
  // reset joymap position
  if (prev_index != index)
  {
    joymap_pos = 0;
    prev_index = index;
  }

  switch (index)
  {
  case CFGIF_REG_MODES_VIDEO:
    // stub
    return (modes_register & MODE_VGA);

  case CFGIF_REG_MODES_MISC:
    // stub
    return 0;

  case CFGIF_REG_PAD_MODE:
    return joystick_get_mode();

  case CFGIF_REG_PAD_KEYMAP0:
  case CFGIF_REG_PAD_KEYMAP1:
    return joystick_keymap_read(index - CFGIF_REG_PAD_KEYMAP0, joymap_pos++);

  case CFGIF_REG_PAD_AUTOFIRE0:
  case CFGIF_REG_PAD_AUTOFIRE1:
    return joystick_autofire_read(index - CFGIF_REG_PAD_AUTOFIRE0, joymap_pos++);

  case CFGIF_REG_PROTECT:
    return cfg_protect;

  case CFGIF_REG_STATUS:
    // stub
    return 0xFF;

  default:
    break;
  }

  return 0xFF;
}

void config_interface_write(u8 index, u8 data)
{
  // reset joymap position
  if (prev_index != index)
  {
    joymap_pos = 0;
    prev_index = index;
  }

  // check for configuration protection enable
  if ((cfg_protect & CFGIF_PROTECT_ENABLE) && (index != CFGIF_REG_COMMAND)) return;

  switch (index)
  {
  case CFGIF_REG_PAD_MODE:
    joystick_set_mode(data);
    break;

  case CFGIF_REG_PAD_KEYMAP0:
  case CFGIF_REG_PAD_KEYMAP1:
    joystick_keymap_write(index - CFGIF_REG_PAD_KEYMAP0, joymap_pos++, data);
    break;

  case CFGIF_REG_PAD_AUTOFIRE0:
  case CFGIF_REG_PAD_AUTOFIRE1:
    joystick_autofire_write(index - CFGIF_REG_PAD_AUTOFIRE0, joymap_pos++, data);
    break;

  case CFGIF_REG_PROTECT:
    cfg_protect = data;
    break;

  case CFGIF_REG_COMMAND:
    config_interface_command(data);
    break;

  default:
    break;
  }
}
