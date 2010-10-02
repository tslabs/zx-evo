#ifndef PINS_H
#define PINS_H

/**
 * @file
 * @brief ATMEGA128 pins definition.
 * @author http://www.nedopc.com
 *
 * ATMEGA128 pins definition:
 * - define PIN number;
 * - define PORT register;
 * - define input PINs register;
 * - define Data Direction Register.
 */

/** nCONFIG fpga pin number. */
#define nCONFIG      PF0
/** nCONFIG fpga port register. */
#define nCONFIG_PORT PORTF
/** nCONFIG fpga pins register. */
#define nCONFIG_PIN  PINF
/** nCONFIG fpga direction register. */
#define nCONFIG_DDR  DDRF

/** nSTATUS fpga pin number. */
#define nSTATUS      PF1
/** nSTATUS fpga port register. */
#define nSTATUS_PORT PORTF
/** nSTATUS fpga pins register. */
#define nSTATUS_PIN  PINF
/** nSTATUS fpga direction register. */
#define nSTATUS_DDR  DDRF

/** CONF_DONE (configuration done) fpga pin number. */
#define CONF_DONE PF2
/** CONF_DONE (configuration done) fpga port register. */
#define CONF_DONE_PORT PORTF
/** CONF_DONE (configuration done) fpga pins register. */
#define CONF_DONE_PIN  PINF
/** CONF_DONE (configuration done) fpga direction register. */
#define CONF_DONE_DDR  DDRF

/** LED indicator pin number. */
#define LED      PB7
/** LED indicator port register. */
#define LED_PORT PORTB
/** LED indicator pins register. */
#define LED_PIN  PINB
/** LED indicator direction register. */
#define LED_DDR  DDRB

/** PS2 keyboard clock pin number. */
#define PS2KBCLK PE4
/** PS2 keyboard clock port register. */
#define PS2KBCLK_PORT PORTE
/** PS2 keyboard clock pins register. */
#define PS2KBCLK_PIN  PINE
/** PS2 keyboard clock direction register. */
#define PS2KBCLK_DDR  DDRE

/** PS2 keyboard data pin number. */
#define PS2KBDAT PD6
/** PS2 keyboard data port register. */
#define PS2KBDAT_PORT PORTD
/** PS2 keyboard data pins register. */
#define PS2KBDAT_PIN  PIND
/** PS2 keyboard data direction register. */
#define PS2KBDAT_DDR  DDRD

/** PS2 mouse clock pin number. */
#define PS2MSCLK PE5
/** PS2 mouse clock port register. */
#define PS2MSCLK_PORT PORTE
/** PS2 mouse clock pins register. */
#define PS2MSCLK_PIN  PINE
/** PS2 mouse clock direction register. */
#define PS2MSCLK_DDR  DDRE

/** PS2 mouse data pin number. */
#define PS2MSDAT PD7
/** PS2 mouse data port register. */
#define PS2MSDAT_PORT PORTD
/** PS2 mouse data pins register. */
#define PS2MSDAT_PIN  PIND
/** PS2 mouse data direction register. */
#define PS2MSDAT_DDR  DDRD

/** RS232 TXD pin number. */
#define RS232TXD PD3
/** RS232 TXD port register. */
#define RS232TXD_PORT PORTD
/** RS232 TXD pins register. */
#define RS232TXD_PIN  PIND
/** RS232 TXD direction register. */
#define RS232TXD_DDR  DDRD

/** nSPICS fpga pin number. */
#define nSPICS      PB0
/** nSPICS fpga port register. */
#define nSPICS_PORT PORTB
/** nSPICS fpga pins register. */
#define nSPICS_PIN  PINB
/** nSPICS fpga direction register. */
#define nSPICS_DDR  DDRB

/** ATX POWER ON pin number. */
#define ATXPWRON      PF3
/** ATX POWER ON port register. */
#define ATXPWRON_PORT PORTF
/** ATX POWER ON pins register. */
#define ATXPWRON_PIN  PINF
/** ATX POWER ON direction register. */
#define ATXPWRON_DDR  DDRF

/** SOFT RESET pin number. */
#define SOFTRES      PC7
/** SOFT RESET port register. */
#define SOFTRES_PORT PORTC
/** SOFT RESET pins register. */
#define SOFTRES_PIN  PINC
/** SOFT RESET direction register. */
#define SOFTRES_DDR  DDRC

/** JOYSTICK RIGHT pin number. */
#define JOYSTICK_RIGHT PG0
/** JOYSTICK LEFT pin number. */
#define JOYSTICK_LEFT  PG1
/** JOYSTICK DOWN pin number. */
#define JOYSTICK_DOWN  PG2
/** JOYSTICK UP pin number. */
#define JOYSTICK_UP    PG3
/** JOYSTICK FIRE pin number. */
#define JOYSTICK_FIRE  PG4
/** JOYSTICK pins mask. */
#define JOYSTICK_MASK  ((1<<JOYSTICK_RIGHT)|(1<<JOYSTICK_LEFT)|(1<<JOYSTICK_UP)|(1<<JOYSTICK_DOWN)|(1<<JOYSTICK_FIRE))
/** JOYSTICK port register. */
#define JOYSTICK_PORT  PORTG
/** JOYSTICK pins register. */
#define JOYSTICK_PIN   PING
/** JOYSTICK direction register. */
#define JOYSTICK_DDR   DDRC

#endif

