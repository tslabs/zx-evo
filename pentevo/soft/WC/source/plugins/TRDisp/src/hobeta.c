#include <stdio.h>
#include "defs.h"
#include "hobeta.h"

u16 hobeta_checksum(u8 *header)
{
    u16 checksum = 0;
    u8 i;

    for (i=0; i<=14; i++) checksum += (header[i] * 257) + i;
    return checksum;
}

bool is_hobeta(HOBETA_t *header)
{
    if(header->zero) return false;
    if(header->checksum != hobeta_checksum((u8*)header)) return false;
    return true;
}

