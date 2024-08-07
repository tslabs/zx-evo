#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
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
#include "oled.h"
#include "rtc.h"
#include "atx.h"
#include "joystick.h"
#include "config_interface.h"
#include "tape.h"
#include "kbmap.h"
#include "config.h"

//if want Log than comment next string
#undef LOGENABLE

FILE uart_stdout;

/** FPGA data pointer [far address] (linker symbol). */
extern const u32 fpga_base PROGMEM;
extern const u32 fpga_ts   PROGMEM;
extern const u32 fpga_egg  PROGMEM;

// FPGA data index..
volatile u32 curFpga;

// Common flag register.
volatile u8 flags_register;
volatile u8 flags_ex_register;

// Common modes register.
volatile u8 modes_register;

// Type extensions of gluk registers
volatile u8 ext_type_gluk;

// Buffer for depacking FPGA configuration.
// You can USED for other purposed after setup FPGA.
u8 dbuf[DBSIZE];

// Callback for Slave SPI servicing
void (*zx_task_cb)();

void put_buffer(u16 size)
{
  // writes specified length of buffer to the output
  u8 *ptr = dbuf;

  do
    spi_send(*(ptr++));
      while(--size);
}

void hardware_init(void)
{
  //Initialize AVR pins

  cli(); // disable interrupts

  // configure pins

  PORTG = 0b11111111;
  DDRG  = 0b00000000; // inputs pulled up

//  PORTF = 0b11110000; // ATX off (zero output), fpga config/etc inputs
  DDRF  = 0b00001000;

  PORTE = 0b11110011;
  DDRE  = 0b00000000; // PLL to 2X [E2=Z,E3=Z], inputs pulled up

  PORTD = 0b11111111; // inputs pulled up
  DDRD  = 0b00100000; // RTS out


  PORTC = 0b11011111; // ZXCOL[4:0] used for 2nd gamepad
  DDRC  = 0b00000000; // PWRGOOD input, other pulled up

  PORTB = 0b11000001;
  DDRB  = 0b10000111; // LED off, spi outs inactive

  PORTA = 0b11111111;
  DDRA  = 0b00000000; // pulled up, PA2 used as output for SMD gamepad SEL

  ACSR = 0x80; // DISABLE analog comparator

	// SCL frequency = CPU clk / (16 + 2 * TWBR * 4 ^ TWPS)
	// 11052000 / (16 + 2 * 48) = 98678,5Hz (100000Hz recommended for PCF8583)
	TWSR = 0; // TWPS, bits0..1 {1, 4, 16, 64}
	TWBR = 48;
	TWAR = 0; //disable address match unit
}

void waittask(void)
{
  if (wait_irq_flag)
  {
    wait_irq_flag = 0;
    zx_task_cb();
  }
}

extern void* __vectors;
#define BOOTLOADER() ((void(*)(void))(&__vectors-4096))()

int main()
{
    // these flags control reset reason (hard reset/flash from SD card)
    // be sure to clear them on power-up
    flags_register = 0;
    flags_ex_register = 0;

start:

  // printf setup
  uart_stdout.put = stdout_putchar;
  uart_stdout.flags = _FDEV_SETUP_WRITE;
  uart_stdout.udata = 0; 
  stdout = &uart_stdout;
  
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

  // reflash?
  if (flags_ex_register & FLAG_EX_FLASH) {
      cli(); // disable interrupts
      // simulate soft reset button press for bootloader
      SOFTRES_DDR  |= (1<<SOFTRES);
      SOFTRES_PORT &= ~(1<<SOFTRES);
      // HACK!!!
      BOOTLOADER();
  }

  oled_init();
  oled_clear();
  oled_set_xy(0, 0); oled_print("ZX-Evolution");
  oled_set_xy(0, 1); oled_print("LCD Test 128x64");
  oled_set_xy(0, 2); oled_print("0123456789ABCDEFG");
  oled_set_xy(0, 4); oled_print("Line 4");
  oled_set_xy(0, 5); oled_print("Line 5");
  oled_set_xy(0, 6); oled_print("Line 6");
  oled_set_xy(0, 7); oled_print("Line 7");

  wait_for_atx_power();

  oled_set_xy(0, 3); oled_print("Config: ");

  spi_init();

  DDRF |= (1 << nCONFIG); // pull low for a time
  _delay_ms(50);
  DDRF &= ~(1 << nCONFIG);
  while(!(PINF & (1 << nSTATUS))); // wait ready

  switch (eeprom_read_byte((const u8*)ADDR_FPGA_CFG))
  {
    case FPGA_BASE:
      curFpga = GET_FAR_ADDRESS(fpga_base);
      zx_task_cb = zx_wait_task_old;
      oled_print("Base");
    break;

    case FPGA_EGG:
      curFpga = GET_FAR_ADDRESS(fpga_egg);
      zx_task_cb = zx_wait_task_old;
      oled_print("Egg");
    break;

    case FPGA_TS:
    default:
      curFpga = GET_FAR_ADDRESS(fpga_ts);
      zx_task_cb = zx_wait_task;
      oled_print("TS");
    break;
  }

#ifdef LOGENABLE
  {
  char log_fpga[]="F........\r\n";
  u8 b = (u8)((curFpga>>24) & 0xFF);
  log_fpga[1] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
  log_fpga[2] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
  b = (u8)((curFpga>>16) & 0xFF);
  log_fpga[3] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
  log_fpga[4] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
  b = (u8)((curFpga>>8) & 0xFF);
  log_fpga[5] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
  log_fpga[6] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
  b = (u8)(curFpga & 0xFF);
  log_fpga[7] = ((b >> 4) <= 9)?'0'+(b >> 4):'A'+(b >> 4)-10;
  log_fpga[8] = ((b & 0x0F) <= 9)?'0'+(b & 0x0F):'A'+(b & 0x0F)-10;
  to_log(log_fpga);
  }
#endif

  depacker_dirty();

#ifdef LOGENABLE
  to_log("depacker_dirty OK\r\n");
#endif

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
  wait_irq_flag = 0;

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
  joystick_init();

#ifdef LOGENABLE
  to_log("zx_init OK\r\n");
#endif

  sei();

  //set led on keyboard
  ps2keyboard_send_cmd_blocking(PS2KEYBOARD_CMD_AUTOREPEAT);
  ps2keyboard_send_cmd_blocking(PS2KEYBOARD_CMD_SETLED);

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
  }
  while((flags_register & FLAG_HARD_RESET) == 0);

  goto start;
}
