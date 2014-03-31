#pragma once

struct FIFO
{
    U8 *addr;
    int size;
    int rdptr;
    int wrptr;
    int used;
};

void fifo_init(FIFO *, U8 *, int);
void fifo_clear(FIFO *);
void fifo_put_byte(FIFO *, U8);
U8 fifo_get_byte(FIFO *);
U8 fifo_read_byte(FIFO *);
U8 fifo_put(FIFO *, U8 *, int);
U8 fifo_get(FIFO *, U8 *, int);

#define fifo_used(a) ((int)(a.used))
#define fifo_free(a) ((int)((a.size) - (a.used)))