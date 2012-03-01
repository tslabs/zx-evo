
extern	bios
#include "conf.asm"
#include "macro.asm"


	rseg	CODE

    di
    xtr
    xt memconf, 0x0E
    xt page0, 0xF0
    
    ld hl, bios
    ld de, win0
    ld bc, 0x4000
    ldir
    
    jp 0
    
    end
