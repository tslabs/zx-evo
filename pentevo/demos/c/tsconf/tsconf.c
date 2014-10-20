
#include <defs.h>
#include "tsconf.h"

void ts_load_pal(void *addr, U16 num)
{
    outp(TS_FMADDR, 0 | FM_EN);
    memcpy(0, addr, num);
    outp(TS_FMADDR, 0);
}
