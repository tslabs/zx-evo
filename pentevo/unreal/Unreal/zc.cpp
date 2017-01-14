// Z-Controller by KOE
// Only SD-card
#include "std.h"
#include "emul.h"
#include "vars.h"
#include "sdcard.h"
#include "zc.h"
#include "ft812.h"

void TZc::Reset()
{
  SdCard.Reset();
  Cfg = 3;
  Status = 0;
  RdBuff = 0xff;
}

void TZc::Wr(u32 Port, u8 Val)
{
  switch(Port & 0xFF)
  {
    case 0x77: // config
      Val &= 7;
    
      if ((comp.ts.vdac2) && ((Cfg & 4) != (Val & 4)))
        vdac2::set_ss((Val & 4) != 0);
      
      Cfg = Val;
    break;

    case 0x57: // data
      if (!(Cfg & 2))   // SD card selected
      {
        RdBuff = SdCard.Rd();
        SdCard.Wr(Val);
      }
      else if ((Cfg & 4) && (comp.ts.vdac2))   // FT812 selected
        RdBuff = vdac2::transfer(Val);
      else
        RdBuff = 255;
      //printf("\nOUT %02X  in %02X",Val,RdBuff);
    break;
  }
}

u8 TZc::Rd(u32 Port)
{
  switch(Port & 0xFF)
  {
    case 0x77: // status
      return Status;      // always returns 0

    case 0x57: // data
      u8 tmp = RdBuff;
      
      if (!(Cfg & 2))   // SD card selected
      {
        RdBuff = SdCard.Rd();
        SdCard.Wr(0xff);
      }
      else if ((Cfg & 4) && (comp.ts.vdac2))   // FT812 selected
        RdBuff = vdac2::transfer(0xFF);
      else
        RdBuff = 255;
      
      //printf("\nout FF  IN %02X (next %02X)",tmp,RdBuff);
      return tmp;
  }

  return 0xFF;
}

TZc Zc;
