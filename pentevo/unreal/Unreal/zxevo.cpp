#include "std.h"
#include "zxevo.h"
#include "fontatm2.h"

unsigned int zxevo_readfont_pos;

unsigned char zxevo_readfont(void)
{
	// read sequence for Z80: all first bytes of symbols 0-15, then all second bytes of same symbols, etc until 7th bytes.
	// then all first bytes of symbols 16-31, and so on.
	//
	// unreal fontrom sequence: all first bytes if all symbols (0-255), then all second bytes and so on
	
	unsigned int idx;
	
	idx =  (zxevo_readfont_pos & 0x000F)     |
	      ((zxevo_readfont_pos & 0x0070)<<4) |
	      ((zxevo_readfont_pos & 0x0780)>>3) ;

	zxevo_readfont_pos++;
	
	return fontatm2[idx];
}


void zxevo_set_readfont_pos(void)
{
	zxevo_readfont_pos = 0;
}
