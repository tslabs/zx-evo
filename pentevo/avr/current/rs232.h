#ifndef RS232_H
#define RS232_H

void rs232_init(void);
void rs232_transmit(UBYTE data);

//#define LOGENABLE
#ifdef LOGENABLE
void to_log(char* ptr);
#endif

/**
 * ZX write to Kondratiev's rs232 registers.
 * @param index [in] - index of Kondratiev's rs232 register
 * @param byte [in] - data
 */
void rs232_zx_write(UBYTE index, UBYTE data);

/**
 * ZX read from Kondratiev's rs232 registers.
 * @return registers data
 * @param index [in] - index of Kondratiev's rs232 register
 */
UBYTE rs232_zx_read(UBYTE index);

#endif //RS232_H
