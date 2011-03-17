#ifndef DEPACKER_DIRTY_H
#define DEPACKER_DIRTY_H

#include "mytypes.h"




// size and mask of output buffer
#define DBSIZE 2048
#define DBMASK 2047

extern UBYTE dbuf[DBSIZE];

extern ULONG indata;

// putting outbut buffer to file: for use of depacker_dirty();
void put_buffer(UWORD size);


#define NEXT_BYTE (pgm_read_byte_far(indata++))

void  depacker_dirty(void); // actual depacker, 8bit-oriented and with no any checks!

UBYTE get_bits_dirty(UBYTE numbits);
WORD get_bigdisp_dirty(void);

void put_byte(UBYTE);
void repeat(WORD,UBYTE);

#endif

