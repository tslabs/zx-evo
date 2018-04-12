#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "mytypes.h"
#include "zx.h"
#include "kbmap.h"
#include "pins.h"
#include "getfaraddress.h"
#include "main.h"
#include "spi.h"
#include "rs232.h"
#include "ps2.h"
#include "rtc.h"

// comment next string to enable Log
#undef LOGENABLE

// zx mouse registers
u8 zx_mouse_button;
u8 zx_mouse_x;
u8 zx_mouse_y;

// PS/2 keyboard control keys status (for additional functons)
volatile u8 kb_ctrl_status[2];
// PS/2 keyboard control keys mapped to zx keyboard (mapped keys not used in additional functions)
volatile u8 kb_ctrl_mapped[2];

#define ZX_FIFO_SIZE 256 /* do not change this since it must be exactly byte-wise */

u8 zx_fifo[ZX_FIFO_SIZE];

u8 zx_fifo_in_ptr;
u8 zx_fifo_out_ptr;

u8 zx_counters[40]; // filter ZX keystrokes here to assure every is pressed and released only once
u8 zx_map[5]; // keys bitmap. send order: LSbit first, from [4] to [0]

volatile u8 shift_pause;

u8 zx_realkbd[11];

extern u8 egg;

void zx_init(void)
{
  zx_fifo_in_ptr = zx_fifo_out_ptr = 0;

  zx_task(ZX_TASK_INIT);

  ATOMIC_BLOCK(ATOMIC_FORCEON) { rs232_init(); }
  //reset Z80
  zx_spi_send(SPI_RST_REG, 1, 0);
  zx_spi_send(SPI_RST_REG, 0, 0);
}

u8 zx_spi_send(u8 addr, u8 data, u8 mask)
{
  u8 status;
  u8 ret;
  nSPICS_PORT &= ~(1<<nSPICS); // fix for status locking
  nSPICS_PORT |= (1<<nSPICS);  // set address of SPI register
  status = spi_send(addr);
  nSPICS_PORT &= ~(1<<nSPICS); // send data for that register
  ret = spi_send(data);
  nSPICS_PORT |= (1<<nSPICS);

  //if CPU waited
  if (status & mask) zx_wait_task(status);

  return ret;
}

void zx_task(u8 operation) // zx task, tracks when there is need to send new keymap to the fpga
{
  static u8 prev_code;
  static u8 task_state;
  //static u8 reset_type;

  u8 was_data;
  u8 code,keynum,keybit;

  if (operation==ZX_TASK_INIT)
  {
    //reset_type = 0;
    prev_code = KEY_V+1; // impossible scancode
    task_state = 0;
    shift_pause = 0;

    zx_clr_kb();

    //check control keys whoes mapped to zx keyboard
    //(mapped keys not used in additional functions)
    kb_ctrl_mapped[0] = 0;
    kb_ctrl_mapped[1] = 0;
    if (kbmap_get(0x14,0).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[0] |= KB_LCTRL_MASK;
    if (kbmap_get(0x14,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[0] |= KB_RCTRL_MASK;
    if (kbmap_get(0x11,0).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[0] |= KB_LALT_MASK;
    if (kbmap_get(0x11,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[0] |= KB_RALT_MASK;
    if (kbmap_get(0x1F,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[1] |= KB_LWIN_MASK_1;
    if (kbmap_get(0x27,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[1] |= KB_RWIN_MASK_1;
    if (kbmap_get(0x2F,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped[1] |= KB_MENU_MASK_1;
     /*
    //detect if CTRL-ALT-DEL keys mapped
//    if (((kbmap[0x14*2] == NO_KEY) && (kbmap[0x14*2+1] == NO_KEY)) ||
//       ((kbmap[0x11*2] == NO_KEY) && (kbmap[0x11*2+1] == NO_KEY)) ||
//       ((kbmap_E0[0x11*2] == NO_KEY) && (kbmap[0x11*2+1] == NO_KEY)))
    if ((kbmap_get(0x14,0).tw == (u16)NO_KEY+(((u16)NO_KEY)<<8)) ||
      (kbmap_get(0x11,0).tw == (u16)NO_KEY+(((u16)NO_KEY)<<8)) ||
      (kbmap_get(0x11,1).tw == (u16)NO_KEY+(((u16)NO_KEY)<<8)))
    {
      //not mapped
      kb_ctrl_status[0] &= ~KB_CTRL_ALT_DEL_MAPPED_MASK;
    }
    else
    {
      //mapped
      kb_ctrl_status[0] |= KB_CTRL_ALT_DEL_MAPPED_MASK;
    }
    */
  }
  else /*if (operation==ZX_TASK_WORK)*/

  // из фифы приходит: нажатия и отжатия ресетов, нажатия и отжатия кнопков, CLRKYS (только нажание).
  // задача: упдейтить в соответствии с этим битмап кнопок, посылать его в фпгу, посылать ресеты.
  // кроме того, делать паузу в упдейте битмапа и посылке его в фпга между нажатием CS|SS и последующей не-CS|SS кнопки,
  // равно как и между отжатием не-CS|SS кнопки и последующим отжатием CS|SS.

  // сначала делаем тупо без никаких пауз - чтобы работало вообще с фифой

  {
    //check and set/reset NMI
    if ((flags_ex_register & FLAG_EX_NMI)==0)
    {
      if ((NMI_PIN & (1<<NMI)) == 0)
      {
        //NMI button pressed
        flags_ex_register |= FLAG_EX_NMI; //set flag
        zx_set_config(); //set NMI to Z80
      }
    }
    else
    {
      if ((NMI_PIN & (1<<NMI)) != 0)
      {
        //NMI button pressed
        flags_ex_register &= ~FLAG_EX_NMI; //reset flag
        zx_set_config(); //reset NMI to Z80
      }
    }


    if (!task_state)
    {
      nSPICS_PORT |= (1<<nSPICS);

      was_data = 0;

      while(!zx_fifo_isempty())
      {
        code=zx_fifo_copy(); // don't remove byte from fifo!

        if (code==CLRKYS)
        {
          was_data = 1; // we've got something!

          zx_fifo_get(); // remove byte from fifo

          //reset_type = 0;
          prev_code  = KEY_V+1;

          zx_clr_kb();

          break; // flush changes immediately to the fpga
        }
//        else if ((code & KEY_MASK) >= RSTSYS)
//        {
//          was_data = 1; // we've got something!

//          zx_fifo_get(); // remove byte from fifo

//          if (code & PRESS_MASK) // reset key pressed
//          {
//            reset_type  = 0x30 & ((code+1)<<4);
//            reset_type += 2;

//            break; // flush immediately
//          }
//          else // reset key released
//          {
//            reset_type = 0;
//          }
//        }
        else /*if ((code & KEY_MASK) < 40)*/
        {
          if (shift_pause) // if we inside pause interval and need checking
          {
            if ((PRESS_MASK & prev_code) && (PRESS_MASK & code))
            {
              if (/* prev key was CS|SS down */
                ((PRESS_MASK|KEY_CS)<=prev_code && prev_code<=(PRESS_MASK|KEY_SS)) &&
                /* curr key is not-CS|SS down */
                (code<(PRESS_MASK|KEY_CS) || (PRESS_MASK|KEY_SS)<code)
            )
                break; // while loop
            }

            if ((!(PRESS_MASK & prev_code)) && (!(PRESS_MASK & code)))
            {
              if (/* prev key was not-CS|SS up */
                (prev_code<KEY_CS || KEY_SS<prev_code) &&
                /* curr key is CS|SS up */
                (KEY_CS<=prev_code && prev_code<=KEY_SS)
            )
                break;
            }
          }

          // just normal processing out of pause interval
          keynum = (code & KEY_MASK)>>3;

          keybit = 0x0080 >> (code & 7); // KEY_MASK - надмножество битов 7

          if (code & PRESS_MASK)
            zx_map[keynum] |=  keybit;
          else
            zx_map[keynum] &= (~keybit);

          prev_code = code;
          zx_fifo_get();
          shift_pause = SHIFT_PAUSE; // init wait timer

          was_data = 1;
        }
      }//while(!zx_fifo_isempty())

      if (zx_realkbd[10])
      {
        for (u8 i=0; i<5; i++)
        {
           u8 tmp;
           tmp = zx_realkbd[i+5];
           was_data |= zx_realkbd[i] ^ tmp;
           zx_realkbd[i] = tmp;
        }
        zx_realkbd[10] = 0;
      }

      if (was_data) // initialize transfer
      {
        task_state = 6;
      }
    }
    else // sending bytes one by one in each state
    {
      task_state--;
#ifdef LOGENABLE
  char log_task_state[] = "TS..\r\n";
  log_task_state[2] = ((task_state >> 4) <= 9)?'0'+(task_state >> 4):'A'+(task_state >> 4)-10;
  log_task_state[3] = ((task_state & 0x0F) <= 9)?'0'+(task_state & 0x0F):'A'+(task_state & 0x0F)-10;
  to_log(log_task_state);
#endif

      if (task_state>0)// task_state==5..1
      {
        u8 key_data;
        key_data = zx_map[task_state-1] | ~zx_realkbd[task_state-1];
        zx_spi_send(SPI_KBD_DAT, key_data, ZXW_MASK);
#ifdef LOGENABLE
  char log_zxmap_task_state[] = "TK.. .. ..\r\n";
  log_zxmap_task_state[2] = ((key_data >> 4) <= 9)?'0'+(key_data >> 4):'A'+(key_data >> 4)-10;
  log_zxmap_task_state[3] = ((key_data & 0x0F) <= 9)?'0'+(key_data & 0x0F):'A'+(key_data & 0x0F)-10;
  log_zxmap_task_state[5] = ((zx_map[task_state-1] >> 4) <= 9)?'0'+(zx_map[task_state-1] >> 4):'A'+(zx_map[task_state-1] >> 4)-10;
  log_zxmap_task_state[6] = ((zx_map[task_state-1] & 0x0F) <= 9)?'0'+(zx_map[task_state-1] & 0x0F):'A'+(zx_map[task_state-1] & 0x0F)-10;
  log_zxmap_task_state[8] = ((zx_realkbd[task_state-1] >> 4) <= 9)?'0'+(zx_realkbd[task_state-1] >> 4):'A'+(zx_realkbd[task_state-1] >> 4)-10;
  log_zxmap_task_state[9] = ((zx_realkbd[task_state-1] & 0x0F) <= 9)?'0'+(zx_realkbd[task_state-1] & 0x0F):'A'+(zx_realkbd[task_state-1] & 0x0F)-10;
  to_log(log_zxmap_task_state);
#endif
      }
      else // task_state==0
      {
        u8 status;
        nSPICS_PORT |= (1<<nSPICS);
        status = spi_send(SPI_KBD_STB);    // strobe input kbd data to the Z80 port engine
        nSPICS_PORT &= ~(1<<nSPICS);
        nSPICS_PORT |= (1<<nSPICS);
        if (status & ZXW_MASK) zx_wait_task(status);
#ifdef LOGENABLE
  to_log("STB\r\n");
#endif
      }
    }
  }

}

void zx_clr_kb(void)
{
  u8 i;

  for(i=0; i<sizeof(zx_map)/sizeof(zx_map[0]); i++)
  {
    zx_map[i] = 0;
  }

  for(i=0; i<sizeof(zx_realkbd)/sizeof(zx_realkbd[0]); i++)
  {
    zx_realkbd[i] = 0xff;
  }

  for(i=0; i<sizeof(zx_counters)/sizeof(zx_counters[0]); i++)
  {
    zx_counters[i] = 0;
  }

  kb_ctrl_status[0] = 0;
  kb_ctrl_status[1] = 0;
}

void to_zx(u8 scancode, u8 was_E0, u8 was_release)
{
  KBMAP_VALUE t;

  //F7 code (0x83) converted to 0x7F
  if (!was_E0 && (scancode == 0x83)) scancode = 0x7F;

  //get zx map values
  t = kbmap_get(scancode,was_E0);

  if (was_E0)
  {
    //additional functionality from ps/2 keyboard
    switch (scancode)
    {
      //Right Alt (Alt Gr)
      case  0x11:
        if (!was_release) kb_ctrl_status[0] |= KB_RALT_MASK;
        else kb_ctrl_status[0] &= ~KB_RALT_MASK;
      break;

      //Right Ctrl
      case  0x14:
        if (!was_release) kb_ctrl_status[0] |= KB_RCTRL_MASK;
        else kb_ctrl_status[0] &= ~KB_RCTRL_MASK;
      break;

      //Left Win
      case  0x1F:
        if (!was_release) kb_ctrl_status[1] |= KB_LWIN_MASK_1;
        else kb_ctrl_status[1] &= ~KB_LWIN_MASK_1;
      break;

      // Right Win
      case  0x27:
        if (!was_release) kb_ctrl_status[1] |= KB_RWIN_MASK_1;
        else kb_ctrl_status[1] &= ~KB_RWIN_MASK_1;
      break;

      //Menu
      case  0x2F:
        if (!was_release) kb_ctrl_status[1] |= KB_MENU_MASK_1;
        else kb_ctrl_status[1] &= ~KB_MENU_MASK_1;
      break;

      //Print Screen
      case 0x7C:
        //set/reset NMI
        if (((flags_ex_register & FLAG_EX_NMI)==0) && (was_release==0))
        {
          flags_ex_register |= FLAG_EX_NMI; //set flag
          zx_set_config(); //set NMI to Z80
        }
        else if (((flags_ex_register & FLAG_EX_NMI)!=0) && (was_release!=0))
        {
          flags_ex_register &= ~FLAG_EX_NMI; //reset flag
          zx_set_config(); //reset NMI to Z80
        }
      break;

      //Del
      case 0x71:
        //Ctrl-Alt-Del pressed
        if ((!was_release) &&
           /*(!(kb_status & KB_CTRL_ALT_DEL_MAPPED_MASK)) && */
           ((kb_ctrl_status[0] & (~kb_ctrl_mapped[0]) & (KB_LCTRL_MASK|KB_RCTRL_MASK)) !=0) &&
           ((kb_ctrl_status[0] & (~kb_ctrl_mapped[0]) & (KB_LALT_MASK|KB_RALT_MASK)) !=0))
        {
          //hard reset
          flags_register |= FLAG_HARD_RESET;
          t.tb.b2=t.tb.b1=NO_KEY;
        }
      break;
    }
  }

  else
  {
    //additional functionality from ps/2 keyboard
    switch (scancode)
    {
      //Scroll Lock
      case 0x7E:
        // VGA mode
        if (!was_release)
          zx_mode_switcher(MODE_VGA);
      break;

      // F1
      case 0x05:
        // Floppy swap
        if (!was_release && (kb_ctrl_status[1] & ~kb_ctrl_mapped[1] & KB_MENU_MASK_1))
          zx_mode_switcher(MODE_FSWAP);
        break;

      // F2
      case 0x06:
        // Tape In sound
        //  0 - beeper
        //  1 - tape out
        //  2 - tape in
        if (!was_release && (kb_ctrl_status[1] & ~kb_ctrl_mapped[1] & KB_MENU_MASK_1))
        {
          u8 m;
          switch (modes_register & (MODE_TAPEIN | MODE_TAPEOUT))
          {
            case 0:
              m = MODE_TAPEOUT;
            break;

            case MODE_TAPEIN:
              m = MODE_TAPEIN;
            break;

            default:
              m = MODE_TAPEOUT | MODE_TAPEIN;
          }
          zx_mode_switcher(m);
        }
        break;

      // F3
      case 0x04:
        // 50/60 Hz
        if (!was_release && (kb_ctrl_status[1] & ~kb_ctrl_mapped[1] & KB_MENU_MASK_1))
          zx_mode_switcher(MODE_60HZ);
        break;

      // F4
      // case 0x0C:
        // if (!was_release && (kb_ctrl_status[1] & ~kb_ctrl_mapped[1] & KB_MENU_MASK_1))
          // zx_mode_switcher(MODE_POL);
      // break;

      //Left Shift
      case  0x12:
        if (!was_release) kb_ctrl_status[0] |= KB_LSHIFT_MASK;
        else kb_ctrl_status[0] &= ~KB_LSHIFT_MASK;
      break;

      //Right Shift
      case  0x59:
        if (!was_release) kb_ctrl_status[0] |= KB_RSHIFT_MASK;
        else kb_ctrl_status[0] &= ~KB_RSHIFT_MASK;
        break;

      //Left Ctrl
      case  0x14:
        if (!was_release) kb_ctrl_status[0] |= KB_LCTRL_MASK;
        else kb_ctrl_status[0] &= ~KB_LCTRL_MASK;
        break;

      //Left Alt
      case  0x11:
        if (!was_release) kb_ctrl_status[0] |= KB_LALT_MASK;
        else kb_ctrl_status[0] &= ~KB_LALT_MASK;
      break;

      //F11
      case  0x78:
        // easter egg
        if ((!was_release) &&
           (!(kb_ctrl_status[0] & KB_CTRL_ALT_DEL_MAPPED_MASK)) &&
           ((kb_ctrl_status[0] & (KB_LCTRL_MASK|KB_RCTRL_MASK)) &&
           (kb_ctrl_status[0] & (KB_LALT_MASK|KB_RALT_MASK))))
        {
          egg = 1;
          //hard reset
          flags_register |= FLAG_HARD_RESET;
          t.tb.b1=NO_KEY;
        }
      break;

      //F12
      case  0x07:
        // switch config
        if ((!was_release) &&
           (!(kb_ctrl_status[0] & KB_CTRL_ALT_DEL_MAPPED_MASK)) &&
           ((kb_ctrl_status[0] & (KB_LCTRL_MASK|KB_RCTRL_MASK)) &&
           (kb_ctrl_status[0] & (KB_LALT_MASK|KB_RALT_MASK))))
        {
          eeprom_write_byte((u8*)0x0fff, !eeprom_read_byte((const u8*)0x0fff));
          //hard reset
          flags_register |= FLAG_HARD_RESET;
          t.tb.b1=NO_KEY;
          break;
        }
        else if (!was_release)
          kb_ctrl_status[0] |= KB_F12_MASK;
        else
          kb_ctrl_status[0] &= ~KB_F12_MASK;
      break;

      //keypad '+','-','*' - set ps2mouse resolution
      case  0x79:
      case  0x7B:
      case  0x7C:
        if (!was_release) ps2mouse_set_resolution(scancode);
      break;
    }
  }

  if (t.tb.b1!=NO_KEY)
  {
    update_keys(t.tb.b1,was_release);

    if (t.tb.b2!=NO_KEY) update_keys(t.tb.b2,was_release);
  }
}

void update_keys(u8 zxcode, u8 was_release)
{
    if (zxcode!=NO_KEY)
    {
        s8 i;

        if ((zxcode==CLRKYS) && (!was_release)) // does not have release option
        {
            i=39;
            do zx_counters[i]=0; while((--i)>=0);

            if (!zx_fifo_isfull())
                zx_fifo_put(CLRKYS);
        }
        // else if (zxcode>=RSTSYS) // resets - press and release
        // {
        //  if (!zx_fifo_isfull())
        //   zx_fifo_put((was_release ? 0 : PRESS_MASK) | zxcode);
        // }
        else if (zxcode < 40) // ordinary keys too
        {
            if (was_release)
            {
                if (zx_counters[zxcode] && !(--zx_counters[zxcode])) // left-to-right evaluation and shortcutting
                {
                if (!zx_fifo_isfull())
                zx_fifo_put(zxcode);
                }
            }
            else // key pressed
            {
                if (!(zx_counters[zxcode]++))
                {
                if (!zx_fifo_isfull())
                zx_fifo_put(PRESS_MASK | zxcode);
                }
            }
        }
    }
}

void zx_fifo_put(u8 input)
{
  zx_fifo[zx_fifo_in_ptr++] = input;
}

u8 zx_fifo_isfull(void)
{
  //always one byte unused, to distinguish between totally full fifo and empty fifo
  return((zx_fifo_in_ptr+1)==zx_fifo_out_ptr);
}

u8 zx_fifo_isempty(void)
{
  return (zx_fifo_in_ptr==zx_fifo_out_ptr);
}

u8 zx_fifo_get(void)
{
  return zx_fifo[zx_fifo_out_ptr++]; // get byte permanently
}

u8 zx_fifo_copy(void)
{
  return zx_fifo[zx_fifo_out_ptr]; // get byte but leave it in fifo
}

void zx_mouse_reset(u8 enable)
{
  if (enable)
  {
    //ZX autodetecting found mouse on this values
    zx_mouse_x = 0;
    zx_mouse_y = 1;
  }
  else
  {
    //ZX autodetecting not found mouse on this values
    zx_mouse_x = 0xFF;
    zx_mouse_y = 0xFF;
  }

  zx_mouse_button = 0xFF;
  flags_register |= FLAG_PS2MOUSE_ZX_READY;
}

void zx_mouse_task(void)
{
  if (flags_register & FLAG_PS2MOUSE_ZX_READY)
  {
#ifdef LOGENABLE
  char log_zxmouse[] = "ZXM.. .. ..\r\n";
  log_zxmouse[3] = ((zx_mouse_button >> 4) <= 9)?'0'+(zx_mouse_button >> 4):'A'+(zx_mouse_button >> 4)-10;
  log_zxmouse[4] = ((zx_mouse_button & 0x0F) <= 9)?'0'+(zx_mouse_button & 0x0F):'A'+(zx_mouse_button & 0x0F)-10;
  log_zxmouse[6] = ((zx_mouse_x >> 4) <= 9)?'0'+(zx_mouse_x >> 4):'A'+(zx_mouse_x >> 4)-10;
  log_zxmouse[7] = ((zx_mouse_x & 0x0F) <= 9)?'0'+(zx_mouse_x & 0x0F):'A'+(zx_mouse_x & 0x0F)-10;
  log_zxmouse[9] = ((zx_mouse_y >> 4) <= 9)?'0'+(zx_mouse_y >> 4):'A'+(zx_mouse_y >> 4)-10;
  log_zxmouse[10] = ((zx_mouse_y & 0x0F) <= 9)?'0'+(zx_mouse_y & 0x0F):'A'+(zx_mouse_y & 0x0F)-10;
  to_log(log_zxmouse);
#endif
    //TODO: пока сделал скопом, потом сделать по одному байту за заход
    zx_spi_send(SPI_MOUSE_BTN, zx_mouse_button, ZXW_MASK);
    zx_spi_send(SPI_MOUSE_X, zx_mouse_x, ZXW_MASK);
    zx_spi_send(SPI_MOUSE_Y, zx_mouse_y, ZXW_MASK);

    //data sended - reset flag
    flags_register &=~(FLAG_PS2MOUSE_ZX_READY);
  }
}

void zx_wait_task(u8 status)
{
  // LED_PORT |= 1<<LED;  //power led OFF (for debug)

  u8 addr = 0;
  u8 data = 0xFF;

  //reset flag
  flags_register &= ~FLAG_SPI_INT;

  //prepare data
  switch (status & ZXW_MASK)
  {
    case ZXW_GLUK_CLOCK:
      addr = zx_spi_send(SPI_GLUK_ADDR, data, 0);
      if (status & ZXW_MODE) data = gluk_get_reg(addr);
      break;

    case ZXW_KONDR_RS232:
      addr = zx_spi_send(SPI_RS232_ADDR, data, 0);
      if (status & ZXW_MODE) data = rs232_zx_read(addr);
      break;
  }

  if (status & ZXW_MODE) zx_spi_send(SPI_WAIT_DATA, data, 0);
  else data = zx_spi_send(SPI_WAIT_DATA, data, 0);

  if (!(status & ZXW_MODE))
  {
    //save data
    switch (status & ZXW_MASK)
    {
    case ZXW_GLUK_CLOCK:
      {
        gluk_set_reg(addr, data);
        break;
      }
    case ZXW_KONDR_RS232:
      {
        rs232_zx_write(addr, data);
        break;
      }
    }
  }
/*#ifdef LOGENABLE
  char log_wait[] = "W..A..D..\r\n";
  log_wait[1] = ((status >> 4) <= 9)?'0'+(status >> 4):'A'+(status >> 4)-10;
  log_wait[2] = ((status & 0x0F) <= 9)?'0'+(status & 0x0F):'A'+(status & 0x0F)-10;
  log_wait[4] = ((addr >> 4) <= 9)?'0'+(addr >> 4):'A'+(addr >> 4)-10;
  log_wait[5] = ((addr & 0x0F) <= 9)?'0'+(addr & 0x0F):'A'+(addr & 0x0F)-10;
  log_wait[7] = ((data >> 4) <= 9)?'0'+(data >> 4):'A'+(data >> 4)-10;
  log_wait[8] = ((data & 0x0F) <= 9)?'0'+(data & 0x0F):'A'+(data & 0x0F)-10;
  to_log(log_wait);
#endif   */

  // LED_PORT &= ~(1<<LED);   // power led ON (for debug)
}

void zx_mode_switcher(u8 mode)
{
  //invert mode
  modes_register ^= mode;

  //send configuration to FPGA
  zx_set_config();

  //save mode register to RTC NVRAM
  rtc_write(RTC_COMMON_MODE_REG, modes_register);

  //set led on keyboard
  ps2keyboard_send_cmd(PS2KEYBOARD_CMD_SETLED);
}

void zx_set_config()
{
  u8 m = (modes_register & (MODE_TAPEIN | MODE_FSWAP | MODE_WTP_INT | MODE_60HZ | MODE_TAPEOUT | MODE_VGA)) | (flags_register & FLAG_LAST_TAPE_VALUE) | (flags_ex_register & FLAG_EX_NMI);
  zx_spi_send(SPI_CONFIG_REG, m, ZXW_MASK);
}
