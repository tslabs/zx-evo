#include <stdio.h>
#include <avr/io.h>

#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#include "pins.h"
#include "mytypes.h"

#include "main.h"
#include "zx.h"
#include "joystick.h"
#include "config_interface.h"

// current joystick mapping position
u8 joymap_pos = 0;
u8 prev_index = 0;

u8 cfg_protect = 0;

void config_interface_command(u8 data) {
    switch (data) {
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
    if (prev_index != index) {
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
    if (prev_index != index) {
        joymap_pos = 0;
        prev_index = index;
    }

    // check for configuration protection enable
    //if ((cfg_protect & CFGIF_PROTECT_ENABLE) && (index != CFGIF_REG_COMMAND))
    //    return;

    switch (index)
    {
        case CFGIF_REG_PAD_MODE:
            joystick_set_mode(data);
            break;
        
        case CFGIF_REG_PAD_KEYMAP0:
        case CFGIF_REG_PAD_KEYMAP1:
            joystick_keymap_write(index - CFGIF_REG_PAD_KEYMAP0, joymap_pos++, data);
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