
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <string.h>

#include <util/delay.h>

#include "mytypes.h"
#include "depacker_dirty.h"
#include "getfaraddress.h"
#include "pins.h"
#include "main.h"
#include "ps2.h"
#include "zx.h"
#include "spi.h"
#include "rs232.h"
#include "rtc.h"
#include "atx.h"
#include "joystick.h"
#include "tape.h"
#include "kbmap.h"
#include "config.h"
#include "spiflash.h"
#include "tsspiffs.h"

//if want Log than comment next string
#undef LOGENABLE

/** FPGA data pointer [far address] (linker symbol). */
extern const u32 fpga_base PROGMEM;
extern const u32 fpga_ts   PROGMEM;
extern const u32 fpga_egg  PROGMEM;

// FPGA data index..
volatile u32 curFpga;

// Common flag register.
volatile u8 flags_register;
volatile u8 flags_ex_register;

bool is_baseconf;
bool is_cold_reset;
bool is_hot_fpga;

// Common modes register.
volatile u8 modes_register;

// Type extensions of gluk registers
volatile u8 ext_type_gluk;

TSF_CONFIG tsf_cfg;
TSF_VOLUME tsf_vol;
TSF_FILE tsf_file;
TSF_FILE_STAT stat;

#define TSF_SIZE (4 * 1024 * 1024L)
#define TSF_BLK_SIZE 4096
#define TSFB_SIZE 256

// ATTENTION!!! Used for UART (4 x 256) and other functions!!!
// rs_rxbuff (dbuf+256)
// rs_txbuff (dbuf+512)
// zf_rxbuff (dbuf+768)
// zf_txbuff (dbuf+1024)
u8 dbuf[DBSIZE];       // 2kB buffer for depacking



u8 tsf_buf[TSFB_SIZE]; // SFI FS driver

// --------------------------------------------

void put_buffer(u16 size)
{
  u8 *ptr = dbuf;

  while(size--)
    spi_send(*ptr++);
}

u8 get_next_byte_pf()
{
  return pgm_read_byte_far(curFpga++);
}

TSF_RESULT spi_read(u32 addr, void *buf, u32 size)
{
  // printf("spi_read: %lu, %u, %u\r\n", addr, buf, size);
  // anykey();

  u8 *d = (u8*)buf;
  u16 sz = (u16)size;

  sf_addr = addr;
  sf_command(SPIFL_CMD_READ);

  while (sz--)
    *d++ = spi_flash_read(SPIFL_REG_DATA);

  sf_command(SPIFL_CMD_END);

  return TSF_RES_OK;
}

u8 sfi_next_byte_size;
u8 sfi_next_byte_cnt;
u8 *sfi_next_byte_buf;

u8 get_next_byte_sfi()
{
  static u8 *buf;

  if (!sfi_next_byte_cnt)
  {
    tsf_read(&tsf_file, sfi_next_byte_buf, sfi_next_byte_size);
    buf = sfi_next_byte_buf;
    sfi_next_byte_cnt = sfi_next_byte_size;
  }

  sfi_next_byte_cnt--;
  return *buf++;
}

void sfi_depacker()
{
  u8 buf[128];
  sfi_next_byte_size = sizeof(buf);
  sfi_next_byte_cnt = 0;
  sfi_next_byte_buf = buf;

  cb_next_byte = get_next_byte_sfi;
  depacker_dirty();
}

void sfi_raw_loader(u32 size)
{
  while (size)
  {
    u16 sz = min(DBSIZE, size);
    tsf_read(&tsf_file, dbuf, sz);
    put_buffer(sz);
    size -= sz;
  }
}

void tsf_init()
{
  // +++ SPIF size detect

  tsf_cfg.hal_read_func  = spi_read;
  tsf_cfg.buf = tsf_buf;
  tsf_cfg.buf_size = TSFB_SIZE;
  tsf_cfg.bulk_start = 0;
  tsf_cfg.bulk_size = TSF_SIZE;
  tsf_cfg.block_size = TSF_BLK_SIZE;
}

void load_bitstream()
{
  bool is_spif = false;
  char bs_name[32] = {};

  // start FPGA upload
  DDRF |= (1<<nCONFIG); // pull low for a time
  _delay_ms(50);
  DDRF &= ~(1<<nCONFIG);
  while(!(PINF & (1<<nSTATUS))); // wait ready


  sfi_enable();
  sfi_cs_off();

  do
  {
    u32 sig;

    if (tsf_mount(&tsf_cfg, &tsf_vol) != TSF_RES_OK) break;                               // mount TSF volume

    if (!is_hot_fpga)
    {
      if (tsf_open(&tsf_vol, &tsf_file, "boot.cfg", TSF_MODE_READ) != TSF_RES_OK) break;  // search for config
      if (tsf_read(&tsf_file, dbuf, BOOT_CFG_SIZE) != TSF_RES_OK) break;                  // read config
    }

    if (!cfg_get_field(CFG_TAG_SIG, dbuf, &sig, sizeof(sig))) break;                      // get signature
    if (sig != CFG_SIG) break;                                                            // check signature validity
    if (!cfg_get_field(CFG_TAG_BSTREAM, dbuf, bs_name, sizeof(bs_name) - 1)) break;       // get bitstream filename
    cfg_get_field(CFG_TAG_ISBASE, dbuf, &is_baseconf, sizeof(is_baseconf));               // get config type, if present

    if (tsf_open(&tsf_vol, &tsf_file, bs_name, TSF_MODE_READ) != TSF_RES_OK) break;       // open bitstream file
    is_spif = true;
  } while (0);

  // load from SPIF
  if (is_spif)
  {
    const char *s = bs_name + strlen(bs_name) - 3;

    if (!strcmp(s, "mlz"))
      sfi_depacker();

    else if (!strcmp(s, "rbf"))
    {
      tsf_stat(&tsf_vol, &stat, bs_name);  // get bitstream size
      sfi_raw_loader(stat.size);
    }

    else
      goto pf_load;
  }

  // load from PFLASH
  else
  {
  pf_load:
    switch (eeprom_read_byte(EEPROM_ADDR_FPGA_CFG))
    {
      case FPGA_BASE:
        is_baseconf = true;
        curFpga = GET_FAR_ADDRESS(fpga_base);
      break;

      case FPGA_EGG:
        curFpga = GET_FAR_ADDRESS(fpga_egg);
      break;

      case FPGA_TS:
      default:
        is_baseconf = false;
        curFpga = GET_FAR_ADDRESS(fpga_ts);
      break;
    }

    cb_next_byte = get_next_byte_pf;
    depacker_dirty();
  }
}

void hardware_init(void)
{
  // configure pins
  PORTG = 0b11111111;
  DDRG  = 0b00000000; // inputs pulled up

//  PORTF = 0b11110000; // ATX off (zero output), fpga config/etc inputs
  DDRF  = 0b00001000;

  PORTE = 0b11110011;
  DDRE  = 0b00000000; // PLL to 2X [E2=Z,E3=Z], inputs pulled up

  PORTD = 0b11111111; // inputs pulled up
  DDRD  = 0b00100000; // RTS out


  PORTC = 0b11011111;
  DDRC  = 0b00000000; // PWRGOOD input, other pulled up

  PORTB = 0b11000001;
  DDRB  = 0b10000111; // LED off, spi outs inactive

  PORTA = 0b11111111;
  DDRA  = 0b00000000; // pulled up

  ACSR = 0x80; // DISABLE analog comparator
}

void waittask(void)
{
  if (flags_register & FLAG_SPI_INT)
  {
    //get status byte
    nSPICS_PORT &= ~(1<<nSPICS);
    nSPICS_PORT |= (1<<nSPICS);
    u8 status = spi_send(0);
    zx_wait_task(status);
  }
}

int main()
{
  is_cold_reset = true;
  is_hot_fpga = false;
  is_baseconf = false;

start:
  cli(); // disable interrupts
  if (!is_cold_reset) goto hot_reset;

  hardware_init();
  rs232_init();

#ifdef LOGENABLE
  to_log("VER:");
  {
    u8 b,i;
    u32 version = 0x1DFF0;
    u8 tmpbuff[13];
    char VER[]="..";
    for(i=0; i<12; i++)
    {
      tmpbuff[i] = pgm_read_byte_far(version+i);
    }
    tmpbuff[i]=0;
    to_log((char*)tmpbuff);
    to_log(" ");
    u8 b1 = pgm_read_byte_far(version+12);
    u8 b2 = pgm_read_byte_far(version+13);
    u8 day = b1 & 0x1F;
    u8 mon = ((b2<<3)+(b1>>5)) & 0x0F;
    u8 year = (b2>>1) & 0x3F;
    VER[0] = '0'+(day/10);
    VER[1] = '0'+(day%10);
    to_log(VER);
    to_log(".");
    VER[0] = '0'+(mon/10);
    VER[1] = '0'+(mon%10);
    to_log(VER);
    to_log(".");
    VER[0] = '0'+(year/10);
    VER[1] = '0'+(year%10);
    to_log(VER);
    to_log("\r\n");
    //
    for(i=0; i<16; i++)
    {
      b = pgm_read_byte_far(version+i);
      VER[0] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
      VER[1] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
      to_log(VER);
    }
    to_log("\r\n");
  }
#endif

  wait_for_atx_power();

  spi_init();
  tsf_init();

hot_reset:
  load_bitstream();

  //power led ON
  LED_PORT &= ~_BV(LED);

  // start timer2 (timeouts for ps/2)
  TCCR2 = 0b01000011; // FOC2=0, {WGM21,WGM20}=01, {COM21,COM20}=00, {CS22,CS21,CS20}=011
        // clk/64 clocking,
        // 1/512 overflow rate, total 11.059/32768 = 337.5 Hz interrupt rate
  TIFR = _BV(TOV2);

  // start timer1 (SysTick)
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS10);
  OCR1A = 11059;  // 11.059MHz, thus 1ms Systick

  // enable interrupts
  TIMSK = (_BV(TOIE2) | _BV(OCIE1A));

  //init counters and registers
  ps2keyboard_count = 12;
  ps2keyboard_cmd_count = 0;
  ps2keyboard_cmd = 0;
  ps2mouse_count = 12;
  ps2mouse_initstep = 0;
  ps2mouse_resp_count = 0;
  ps2mouse_cmd = PS2MOUSE_CMD_SET_RESOLUTION;
  flags_register = 0;
  flags_ex_register = 0;
  modes_register = 0;
  ext_type_gluk = 0;

  //reset ps2 keyboard log
  ps2keyboard_reset_log();

  //enable mouse
  zx_mouse_reset(1);

  //set external interrupt
  //INT4 - PS2 Keyboard (falling edge)
  //INT5 - PS2 Mouse (falling edge)
  //INT6 - SPI (falling edge)
  //INT7 - RTC (falling edge)
  EICRB = (1<<ISC41)+(0<<ISC40) + (1<<ISC51)+(0<<ISC50) + (1<<ISC61)+(0<<ISC60) + (1<<ISC71)+(0<<ISC70); // set condition for interrupt
  EIFR = (1<<INTF4)|(1<<INTF5)|(1<<INTF6)|(1<<INTF7); // clear spurious ints there
  EIMSK |= (1<<INT4)|(1<<INT5)|(1<<INT6)|(1<<INT7); // enable

  kbmap_init();
  zx_init();
  rtc_init();
  spif_init();
  is_cold_reset = true;

#ifdef LOGENABLE
  to_log("zx_init OK\r\n");
#endif

  sei();

  //set led on keyboard
  ps2keyboard_send_cmd(PS2KEYBOARD_CMD_SETLED);

  //main loop
  do
  {
    tape_task();           waittask();
    ps2mouse_task();       waittask();
    ps2keyboard_task();    waittask();
    zx_task(ZX_TASK_WORK); waittask();
    zx_mouse_task();       waittask();
    joystick_task();       waittask();
    rs232_task();          waittask();
    atx_power_task();      waittask();
    spif_task();           waittask();
  }
  while((flags_register & FLAG_HARD_RESET) == 0);

  goto start;
}
