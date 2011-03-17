#ifndef RS232_H
#define RS232_H

void rs232_init(void);
void rs232_transmit(UBYTE data);

//#define LOGENABLE
#ifdef LOGENABLE
void to_log(char* ptr);
#endif

#endif //RS232_H
