#include <avr/io.h>
#include <avr/pgmspace.h>

#include "mytypes.h"
#include "main.h"
#include "depacker_dirty.h"


UWORD dbpos; // current position in buffer (wrappable)

UBYTE bitstream;
UBYTE bitcount;

void depacker_dirty(void)
{
	UBYTE j;

	UBYTE bits;
	WORD disp;


	dbpos=0;

	// get first byte of packed file and write to output
	put_byte(NEXT_BYTE);


	// second byte goes to bitstream
	bitstream=NEXT_BYTE;
	bitcount=8;


	// actual depacking loop!
	do
	{
		j=0;

		// get 1st bit - either OUTBYTE or beginning of LZ code
		if( get_bits_dirty(1) )
		{ // OUTBYTE
			put_byte(NEXT_BYTE);
		}
		else
		{ // LZ code
			switch( get_bits_dirty(2) )
			{
			case 0: // 000
				repeat( 0xFFF8|get_bits_dirty(3) ,1);
				break;
			case 1: // 001
				repeat( 0xFF00|NEXT_BYTE ,2);
				break;
			case 2: // 010
				repeat(get_bigdisp_dirty(),3);
				break;
			case 3: // 011
				// extract num of length bits
				do j++; while( !get_bits_dirty(1) );

				if( j<8 ) // check for exit code
				{
					// get length bits itself
					bits=get_bits_dirty(j);
					disp=get_bigdisp_dirty();
					repeat(disp,2+(1<<j)+bits);
				}
				break;
			}
		}

	} while( j<8 );


	if( (DBMASK&dbpos) )
	{
		put_buffer(DBMASK&dbpos);
	}

}




void repeat(WORD disp,UBYTE len)
{ // repeat len bytes with disp displacement (negative)
  // uses dbpos & dbuf

	UBYTE i; // since length is no more than 255

	for(i=0;i<len;i++)
	{
		put_byte(dbuf[DBMASK&(dbpos+disp)]);
	}
}




void put_byte(UBYTE byte)
{
	dbuf[dbpos]=byte;
	dbpos = DBMASK & (dbpos+1);

	if( !dbpos )
	{
		put_buffer(DBSIZE);
	}
}


UBYTE get_bits_dirty(UBYTE numbits)
{ // gets bits in a byte-wise style, no checks
  // numbits must be >0

	UBYTE bits;

	bits=0;

	do
	{
		if( !(bitcount--) )
		{
			bitcount=7;
			bitstream=NEXT_BYTE;
		}

		bits = (bits<<1)|(bitstream>>7); // all shifts byte-wise
		bitstream<<=1;

	} while (--numbits);

	return bits;
}

WORD get_bigdisp_dirty(void)
{ // fetches 'big' displacement (-1..-4352)
  // returns negative displacement

	UBYTE bits;

	if( get_bits_dirty(1) )
	{ // longer displacement
		bits=get_bits_dirty(4);
		return (((0xF0|bits)-1)<<8)|NEXT_BYTE;
	}
	else
	{ // shorter displacement
		return 0xFF00|NEXT_BYTE;
	}
}

