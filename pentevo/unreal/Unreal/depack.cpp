#include "std.h"
#include "sysdefs.h"

//------------------- DeHrust --------------------------

/* BBStream class */
class BBStream
{
  private:
    u8* base;
    u8* p;
    int   idx;
    int   len;
    bool  eof;
    u16  bits;

  public:
    BBStream( u8* from, int blockSize )
    {
      base = p = from;

      len = blockSize;
      idx = 0;
      eof = false;

      bits  = getByte();
      bits += 256*getByte();
    }

    u8 getByte( void )
    {
      if( p - base == len ) { eof = true; return 0; }
      return *p++;
    }

    u8 getBit()
    {
      u16 mask[]  = { 0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400, 0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1 };

      u8 bit = ( bits & mask[idx] ) ? 1 : 0;
      if( idx == 15 )
      {
        bits  = getByte();
        bits += 256*getByte();
      }

      idx = (idx + 1) % 16;
      return bit;
    }

    u8 getBits( int n )
    {
      u8 r = 0;
      do { r = 2*r + getBit(); } while( --n );
      return r;
    }

    bool error( void ) { return eof; }
};

/* depacker */
u16 dehrust(u8* dest, u8* source, int size)
{

  u8 *from = source;

  BBStream s(from, size);

  u8* to = dest;

  *to++ = s.getByte();

  u8 noBits = 2;
  u8 mask[] = { 0, 0, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0 };

  while( !s.error() )
  {
    while (s.getBit())
		*to++ = s.getByte();

    u16 len = 0;
    u8 bb /* = 0*/;
    do
    {
      bb = s.getBits( 2 );
      len += bb;
    } while( bb == 0x03 && len != 0x0f );

    short offset = 0;

    if( len == 0 )
    {
      offset = 0xfff8 + s.getBits( 3 );
      *to++ = to[offset];
      continue;
    }

    if( len == 1 )
    {
      u8 code = s.getBits(2);

      if( code == 2 )
      {
        u8 b = s.getByte();
        if( b >= 0xe0 )
        {
          b <<= 1; ++b; // rlca
          b ^= 2;       // xor c

          if( b == 0xff ) { ++noBits; continue; }

          offset = 0xff00 + b - 0x0f;

          *to++ = to[offset];
          *to++ = s.getByte();
          *to++ = to[offset];
          continue;
        }
        offset = 0xff00 + b;
      }

      if( code == 0 || code == 1 )
      {
        offset = s.getByte();
        offset += 256*(code ? 0xfe : 0xfd );
      }
      if( code == 3 ) offset = 0xffe0 + s.getBits( 5 );

      for( u8 i = 0; i < 2; ++i ) *to++ = to[offset];
      continue;
    }

    if( len == 3 )
    {
      if( s.getBit() )
      {
        offset = 0xfff0 + s.getBits( 4 );
        *to++ = to[offset];
        *to++ = s.getByte();
        *to++ = to[offset];
        continue;
      }

      if( s.getBit() )
      {
        u8 noBytes = 6 + s.getBits(4);
        for( u8 i = 0; i < 2*noBytes; ++i ) *to++ = s.getByte();
        continue;
      }

      len = s.getBits( 7 );
      if( len == 0x0f )
		  break; // EOF
      if( len <  0x0f )
		  len = 256*len + s.getByte();
    }

    if( len == 2 ) ++len;

    u8 code = s.getBits( 2 );

    if( code == 1 )
    {
      u8 b = s.getByte();

      if( b >= 0xe0 )
      {
        if( len > 3 ) return false;

        b <<= 1; ++b; // rlca
        b ^= 3;       // xor c

        offset = 0xff00 + b - 0x0f;

        *to++ = to[offset];
        *to++ = s.getByte();
        *to++ = to[offset];
        continue;
      }
      offset = 0xff00 + b;
    }

    if( code == 0 ) offset = 0xfe00 + s.getByte();
    if( code == 2 ) offset = 0xffe0 + s.getBits( 5 );
    if( code == 3 )
    {
      offset  = 256*( mask[noBits] + s.getBits(noBits) );
      offset += s.getByte();
    }

    for( u16 i = 0; i < len; ++i ) *to++ = to[offset];
  }

  return to-dest;
}

//------------------- DeMegaLZ --------------------------
void demlz(u8 *dst, u8 *src, int size)
{
}
