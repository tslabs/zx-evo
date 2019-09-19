#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "mytypes.h"
#include "main.h"
#include "zx.h"
#include "config.h"

// search EEPROM for a tag and read its value to RAM
// rv: lentgh of the field or 0 if not found
u8 cfg_get_field(u8 tag_req, void *addr)
{
  u8 *eep = EEPROM_ADDR_BOOT_CFG;
  
  while (eep < (EEPROM_ADDR_BOOT_CFG + EEPROM_SIZE_BOOT_CFG))
  {
    u8 tag = eeprom_read_byte(eep++);

    if (tag == CFG_TAG_END)
      break;

    u8 len = eeprom_read_byte(eep++);
    
    if (tag == tag_req)
    {
      eeprom_read_block(addr, eep, len);
      return len;
    }
    
    eep += len;
  }
  
  return 0;
}
