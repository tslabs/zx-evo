#pragma once

// Either of these typedefs should be included before the #include "fifo.h"
typedef u8  FIFO_SIZE_TYPE;
// typedef u16 FIFO_SIZE_TYPE;
// typedef u32 FIFO_SIZE_TYPE;

class FIFO
{
private:

    u8 *addr;
    FIFO_SIZE_TYPE size;
    FIFO_SIZE_TYPE rdptr;
    FIFO_SIZE_TYPE wrptr;

public:

    FIFO_SIZE_TYPE used;
    FIFO_SIZE_TYPE free;

    FIFO(u8 *a, FIFO_SIZE_TYPE s)
    {
      addr = a;
      size = s;
      clear();
    }

    void clear(void)
    {
      rdptr = wrptr = 0;
      used = 0;
      free = size;
    }

    // Puts a byte into FIFO
    void put_byte(u8 x)
    {
      used++; free--;
      *(addr + wrptr) = x;
      wrptr = (wrptr == (size - 1)) ? 0 : (wrptr + 1);
    }

    // Extracts a byte from FIFO
    u8 get_byte(void)
    {
      used--; free++;
      u8 c = *(addr + rdptr);
      rdptr = (rdptr == (size - 1)) ? 0 : (rdptr + 1);
      return c;
    }

    // Reads first byte of FIFO without pushing it out
    u8 peek_byte(void)
    {
      return *(addr + rdptr);
    }

    // Puts a number of bytes into FIFO
    // Makes a check if there's enough place in FIFO
    u8 put(u8 *buf, FIFO_SIZE_TYPE num)
    {
      if ((size - used) < num)
        return 0;        // not enough free space

      while (num--)
        put_byte(*buf++);

      return 1;
    }

    // Extracts a number of bytes from FIFO
    // Makes a check if there's enough data in FIFO
    u8 get(u8 *buf, FIFO_SIZE_TYPE num)
    {
      if (used < num)
        return 0;        // not enough data in FIFO

      while (num--)
        *buf++ = get_byte();

      return 1;
    }
};
