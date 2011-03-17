#ifndef PS2_H
#define PS2_H

#include "mytypes.h"

extern volatile UWORD ps2_shifter;
extern volatile UBYTE ps2_count;
extern volatile UBYTE ps2_timeout;


#define PS2_TIMEOUT 20;


void ps2_task(void); // main entry function
UBYTE ps2_decode(void);
void ps2_parse(UBYTE);



#endif

