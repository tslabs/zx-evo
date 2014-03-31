
#include "stdafx.h"
#include "types.h"
#include "fifo.h"

void fifo_init(FIFO *pf, U8 *addr, int size)
{
    pf->addr = addr;
    pf->size = size;
    pf->rdptr = pf->wrptr = 0;
    pf->used = 0;
}

void fifo_clear(FIFO *pf)
{
    pf->rdptr = pf->wrptr = 0;
    pf->used = 0;
}

// Puts a byte into FIFO
void fifo_put_byte(FIFO *pf, U8 x)
{
    pf->used++;
    *(pf->addr + pf->wrptr) = x;
    pf->wrptr = (pf->wrptr == (pf->size - 1)) ? 0 : (pf->wrptr + 1);
}

// Extracts a byte from FIFO
U8 fifo_get_byte(FIFO *pf)
{
    pf->used--;
    U8 c = *(pf->addr + pf->rdptr);
    pf->rdptr = (pf->rdptr == (pf->size - 1)) ? 0 : (pf->rdptr + 1);
    return c;
}

// Reads first byte of FIFO without pushing it out
U8 fifo_read_byte(FIFO *pf)
{
    return *(pf->addr + pf->rdptr);
}

// Puts a number of bytes into FIFO
// Makes a check if there's enough place in FIFO
U8 fifo_put(FIFO *pf, U8 *buf, int num)
{
    if ((pf->size - pf->used) < num)
        return 0;        // not enough free space

    while (num--)
        fifo_put_byte(pf, *buf++);

    return 1;
}

// Extracts a number of bytes from FIFO
// Makes a check if there's enough data in FIFO
U8 fifo_get(FIFO *pf, U8 *buf, int num)
{
    if (pf->used < num)
        return 0;        // not enough data in FIFO

    while (num--)
        *buf++ = fifo_get_byte(pf);

    return 1;
}
