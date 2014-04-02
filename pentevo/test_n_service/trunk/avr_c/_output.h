#ifndef _OUTPUT_H
#define _OUTPUT_H 1

#ifdef __ASSEMBLER__
/* ------------------------------------------------------------------------- */
.extern print_hexbyte
.extern print_msg
/* ------------------------------------------------------------------------- */
#else // #ifdef __ASSEMBLER__

#include "_types.h"

void put_char(u8 ch);
void print_hexhalf(u8 b);
void print_hexbyte(u8 b);
void print_hexbyte_for_dump(u8 b);
void print_hexlong(u32 l);
void put_char_for_dump(u8 ch);
void print_dec99(u8 b);
void print_dec16(u16 w);
void print_msg(const u8 *msg);
void print_mlmsg(const u8 * const *mlmsg);
void print_short_vers(void);

#endif // #ifdef __ASSEMBLER__

#endif // #ifndef _OUTPUT_H
