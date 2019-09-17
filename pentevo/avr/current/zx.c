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
#include "config.h"

// comment next string to enable Log
#undef LOGENABLE

// zx mouse registers
u8 zx_mouse_button;
u8 zx_mouse_x;
u8 zx_mouse_y;

// PS/2 keyboard control keys status (for additional functons)
volatile KB_CTRL_STATUS_t kb_ctrl_status;
// PS/2 keyboard control keys mapped to zx keyboard (unused)
volatile KB_CTRL_STATUS_t kb_ctrl_mapped;

KBMAP_VALUE t_kbmap;

#define ZX_FIFO_SIZE 256 /* do not change this since it must be exactly byte-wise */

u8 zx_fifo[ZX_FIFO_SIZE];

u8 zx_fifo_in_ptr;
u8 zx_fifo_out_ptr;

u8 zx_counters[40]; // filter ZX keystrokes here to assure every is pressed and released only once
u8 zx_map[5]; // keys bitmap. send order: LSbit first, from [4] to [0]

volatile u8 shift_pause;

u8 zx_realkbd[11];

// callbacks for functional keys
void (*cb_prt_scr)();
void (*cb_scr_lock)();
void (*cb_num_lock)();
void (*cb_menu_f1)();
void (*cb_menu_f2)();
void (*cb_menu_f3)();
void (*cb_menu_f4)();
void (*cb_menu_f5)();
void (*cb_menu_f6)();  // unused
void (*cb_menu_f7)();  // unused
void (*cb_menu_f8)();  // unused
void (*cb_menu_f9)();  // unused
void (*cb_menu_f10)(); // unused
void (*cb_menu_f11)();
void (*cb_menu_f12)();
void (*cb_ctrl_alt_f11)();
void (*cb_ctrl_alt_f12)();
void (*cb_ctrl_alt_del)();

// platform dependent config register setter
void (*cb_zx_set_config)();

// hard reset
void func_reset()
{
  flags_register |= FLAG_HARD_RESET;
  t_kbmap.tb.b2 = t_kbmap.tb.b1 = NO_KEY;
}

void func_service()
{
  u8 fpga_cfg;

  switch (eeprom_read_byte((const u8*)ADDR_FPGA_CFG))
  {
    case FPGA_BASE:
      fpga_cfg = FPGA_TS;
    break;

    case FPGA_TS:
    default:
      fpga_cfg = FPGA_BASE;
    break;
  }

  eeprom_write_byte((u8*)ADDR_FPGA_CFG, fpga_cfg); // switch config number

  flags_register |= FLAG_HARD_RESET;
  t_kbmap.tb.b1 = NO_KEY;
}

void func_egg()
{
  eeprom_write_byte((u8*)ADDR_FPGA_CFG, FPGA_EGG); // switch config number

  flags_register |= FLAG_HARD_RESET;
  t_kbmap.tb.b1 = NO_KEY;
}

// generate NMI
void func_nmi()
{
  flags_ex_register |= FLAG_EX_NMI;
  cb_zx_set_config();
}

// Floppy swap
void func_floppy()
{
  zx_mode_switcher(MODE_TS_FSWAP);
}

// Tape In sound
void func_tapein()
{
  zx_mode_switcher(MODE_TS_TAPEIN);
}

// Beeper sound
void func_beeper()
{
  zx_mode_switcher(MODE_TAPEOUT);
}

// VGA mode - TS-Conf
void func_vga_tv()
{
  zx_mode_switcher(MODE_VGA);
}

// ULA/VGA mode - BaseConf
void func_vga_tv_ula()
{
  u8 m = modes_register | (~MODE_BASE_VIDEO_MASK);
  m++; // increment bits not ORed with 1
  m ^= modes_register;
  m &= MODE_BASE_VIDEO_MASK;
  zx_mode_switcher(m);
}

// Sync polarity
void func_pol()
{
  // zx_mode_switcher(MODE_POL);
}

void func_dummy() {}

void load_config()
{
  cb_menu_f3      = func_dummy;
  cb_menu_f4      = func_dummy;
  cb_menu_f5      = func_dummy;
  // cb_menu_f6      = func_dummy;
  // cb_menu_f7      = func_dummy;
  // cb_menu_f8      = func_dummy;
  // cb_menu_f9      = func_dummy;
  // cb_menu_f10     = func_dummy;
  cb_menu_f11     = func_dummy;
  cb_menu_f12     = func_dummy;
  cb_ctrl_alt_f11 = func_egg;
  cb_ctrl_alt_f12 = func_service;
  cb_ctrl_alt_del = func_reset;

  switch (eeprom_read_byte((const u8*)ADDR_FPGA_CFG))
  {
    case FPGA_BASE:
      cb_prt_scr      = func_nmi;
      cb_num_lock     = func_beeper;
      cb_scr_lock     = func_vga_tv_ula;
      cb_menu_f1      = func_dummy;
      cb_menu_f2      = func_dummy;

      cb_zx_set_config = zx_set_config_base;
    break;

    case FPGA_TS:
    default:
      cb_prt_scr      = func_dummy;
      cb_num_lock     = func_dummy;
      cb_scr_lock     = func_vga_tv;
      cb_menu_f1      = func_floppy;
      cb_menu_f2      = func_beeper;
      cb_menu_f3      = func_tapein;

      cb_zx_set_config = zx_set_config_ts;
    break;
  }
}

void zx_init(void)
{
  zx_fifo_in_ptr = zx_fifo_out_ptr = 0;

  zx_task(ZX_TASK_INIT);

  ATOMIC_BLOCK(ATOMIC_FORCEON) { rs232_init(); }
  //reset Z80
  zx_spi_send(SPI_RST_REG, 1, 0);
  zx_spi_send(SPI_RST_REG, 0, 0);

  load_config();
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
    kb_ctrl_mapped.all = 0;
    if (kbmap_get(0x14,0).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.lctrl = 1;
    if (kbmap_get(0x14,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.rctrl = 1;
    if (kbmap_get(0x11,0).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.lalt  = 1;
    if (kbmap_get(0x11,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.ralt  = 1;
    if (kbmap_get(0x1F,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.lwin  = 1;
    if (kbmap_get(0x27,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.rwin  = 1;
    if (kbmap_get(0x2F,1).tw != (u16)NO_KEY+(((u16)NO_KEY)<<8)) kb_ctrl_mapped.menu  = 1;
  }

  else /*if (operation==ZX_TASK_WORK)*/

  // из фифы приходит: нажатия и отжатия ресетов, нажатия и отжатия кнопков, CLRKYS (только нажатие).
  // задача: упдейтить в соответствии с этим битмап кнопок, посылать его в фпгу, посылать ресеты.
  // кроме того, делать паузу в упдейте битмапа и посылке его в фпга между нажатием CS|SS и последующей не-CS|SS кнопки,
  // равно как и между отжатием не-CS|SS кнопки и последующим отжатием CS|SS.

  // сначала делаем тупо без никаких пауз - чтобы работало вообще с фифой

  {
    //check and set/reset NMI
    if (!(flags_ex_register & FLAG_EX_NMI)) // s/w NMI de-asserted
    {
      if (!(NMI_PIN & (1<<NMI)))  // h/w NMI pin asserted
      {
        //NMI button pressed
        flags_ex_register |= FLAG_EX_NMI; //set flag
        cb_zx_set_config(); //set NMI to Z80
      }
    }
    else // s/w NMI asserted
    {
      if (NMI_PIN & (1<<NMI))  // h/w NMI pin de-asserted
      {
        //NMI button pressed
        flags_ex_register &= ~FLAG_EX_NMI; //reset flag
        cb_zx_set_config(); //reset NMI to Z80
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

  kb_ctrl_status.all = 0;
}

void to_zx(u8 scancode, u8 was_E0, u8 was_release)
{
  //F7 code (0x83) converted to 0x7F
  if (!was_E0 && (scancode == 0x83)) scancode = 0x7F;

  //get zx map values
  t_kbmap = kbmap_get(scancode,was_E0);

  if (was_E0)
  {
    //additional functionality from ps/2 keyboard
    switch (scancode)
    {
      //Right Alt (Alt Gr)
      case  0x11:
        kb_ctrl_status.ralt = !was_release;
      break;

      //Right Ctrl
      case  0x14:
        kb_ctrl_status.rctrl = !was_release;
      break;

      //Left Win
      case  0x1F:
        kb_ctrl_status.lwin = !was_release;
      break;

      // Right Win
      case  0x27:
        kb_ctrl_status.rwin = !was_release;
      break;

      //Menu
      case  0x2F:
        kb_ctrl_status.menu = !was_release;
      break;

      //Print Screen
      case 0x7C:
        if (!was_release)
          cb_prt_scr();
      break;

      //Del
      case 0x71:
        // Ctrl-Alt-Del pressed
        if (!was_release && (kb_ctrl_status.lctrl || kb_ctrl_status.rctrl) && (kb_ctrl_status.lalt || kb_ctrl_status.ralt))
          cb_ctrl_alt_del();
      break;
    }
  }

  else
  {
    //additional functionality from ps/2 keyboard
    switch (scancode)
    {
      //Left Shift
      case  0x12:
        kb_ctrl_status.lshift = !was_release;
      break;

      //Right Shift
      case  0x59:
        kb_ctrl_status.rshift = !was_release;
      break;

      //Left Ctrl
      case  0x14:
        kb_ctrl_status.lctrl = !was_release;
      break;

      //Left Alt
      case  0x11:
        kb_ctrl_status.lalt = !was_release;
      break;

      //Scroll Lock
      case 0x7E:
        if (!was_release)
          cb_scr_lock();
      break;

      //Num Lock
      case 0x77:
        if (!was_release)
          cb_num_lock();
      break;

      // F1
      case 0x05:
        if (!was_release && kb_ctrl_status.menu)
          cb_menu_f1();
      break;

      // F2
      case 0x06:
        if (!was_release && kb_ctrl_status.menu)
          cb_menu_f2();
      break;

      // F3
      case 0x04:
        if (!was_release && kb_ctrl_status.menu)
          cb_menu_f3();
      break;

      // F4
      case 0x0C:
        if (!was_release && kb_ctrl_status.menu)
          cb_menu_f4();
      break;

      // F5
      case 0x03:
        if (!was_release && kb_ctrl_status.menu)
          cb_menu_f5();
      break;

      // F11
      case 0x78:
        if (!was_release)
        {
          // Ctrl-Alt-F11 pressed
          if ((kb_ctrl_status.lctrl || kb_ctrl_status.rctrl) && (kb_ctrl_status.lalt || kb_ctrl_status.ralt))
            cb_ctrl_alt_f11();
          else if (kb_ctrl_status.menu)
            cb_menu_f11();
        }
      break;

      //F12
      case  0x07:
        kb_ctrl_status.f12 = !was_release;

        if (!was_release)
        {
          // Ctrl-Alt-F12 pressed
          if ((kb_ctrl_status.lctrl || kb_ctrl_status.rctrl) && (kb_ctrl_status.lalt || kb_ctrl_status.ralt))
            cb_ctrl_alt_f12();
          else if (kb_ctrl_status.menu)
            cb_menu_f12();
        }
      break;

      //keypad '+','-','*' - set ps2mouse resolution
      case  0x79:
      case  0x7B:
      case  0x7C:
        if (!was_release) ps2mouse_set_resolution(scancode);
      break;
    }
  }

  if (t_kbmap.tb.b1 != NO_KEY)
  {
    update_keys(t_kbmap.tb.b1, was_release);

    if (t_kbmap.tb.b2 != NO_KEY)
      update_keys(t_kbmap.tb.b2, was_release);
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
  cb_zx_set_config();

  //save mode register to RTC NVRAM
  switch (eeprom_read_byte((const u8*)ADDR_FPGA_CFG))
  {
    case FPGA_BASE:
      rtc_write(RTC_COMMON_MODE_REG_BASE, modes_register);
    break;

    case FPGA_TS:
    default:
      rtc_write(RTC_COMMON_MODE_REG_TS, modes_register);
    break;
  }

  //set led on keyboard
  ps2keyboard_send_cmd(PS2KEYBOARD_CMD_SETLED);
}

// 7..5 - (unused)
// 4..5 - ULA mode
//   00 - pentagon raster (71680 clocks)
//   01 - 60Hz raster
//   10 - 48k raster (69888 clocks)
//   11 - 128k raster (70908 clocks)
// 3 - Beeper Mux
//   0 - d4
//   1 - d3
// 2 - Tape In
// 1 - NMI
// 0 - VGA/TV
//   0 - TV
//   1 - VGA
void zx_set_config_base()
{
  u8 m = modes_register & (MODES_BASE_RASTER | MODE_TAPEOUT | MODE_VGA);
  zx_set_config(m);
}

// 7 - Tape/Sound Mux
//   0 - Beep/Tape Out
//   1 - Tape In
// 6 - Floppy Swap
// 5 - WTP INT
// 4 - 50/60Hz
//   0 - 50Hz
//   1 - 60Hz
// 3 - Beeper Mux
//   0 - d4
//   1 - d3
// 2 - Tape In
// 1 - (unused)
// 0 - VGA/TV
//   0 - TV
//   1 - VGA
void zx_set_config_ts()
{
  u8 m = modes_register & (MODE_TS_TAPEIN | MODE_TS_FSWAP | MODE_TS_WTP_INT | MODE_TS_60HZ | MODE_TAPEOUT | MODE_VGA);
  zx_set_config(m);
}

void zx_set_config(u8 m)
{
  m |= flags_register & FLAG_LAST_TAPE_VALUE;
  m |= flags_ex_register & FLAG_EX_NMI;
  zx_spi_send(SPI_CONFIG_REG, m, ZXW_MASK);
}
