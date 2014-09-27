
#include <pff.h>
#include "defs.h"

#define border(a)   output8(0xFE, (a))
#define page128(a)  output(0x7FFD, (a))

DIR dir;
FILINFO fno;
FATFS fs;
UINT num_read;

void main (void)
{
    U8 screen;
    
    disable_interrupt();
    
    border(0);
    page128(0x15);
    memset((void*)0xD800, 0x47, 768);
    page128(0x17);
    memset((void*)0xD800, 0x47, 768);

    disk_initialize();
	pf_mount(&fs);

	pf_open("1.zxv");
    
    screen = 0x17;
    
    while(1)
    {
        page128(screen);
        pf_read((U8*)0xC000, 6144, &num_read);
        screen ^= (8 | 2);
        
        if (num_read != 6144)
        {
            border(2);
            break;
        }
    }
}
