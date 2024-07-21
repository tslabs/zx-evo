// Z-Controller by KOE
// Only SD-card
#include "std.h"
#include "emul.h"
#include "vars.h"
#include "sdcard.h"
#include "zc.h"
#include "ft812.h"
#include "zifi32/zifi32.h"

void TZc::Reset()
{
  SdCard.Reset();
  Cfg = 3;
  Status = 0;
  RdBuff = 0xff;
}

void TZc::DmaRdStart(u32 size)
{
  if ((Cfg & 16) && comp.ts.zifi32)   // ZiFi32 selected
    zf32::dma_r_start(size);
}

void TZc::DmaWrStart()
{
  if ((Cfg & 16) && comp.ts.zifi32)   // ZiFi32 selected
    zf32::dma_w_start();
}

void TZc::DmaWrEnd()
{
  if ((Cfg & 16) && comp.ts.zifi32)   // ZiFi32 selected
    zf32::dma_w_end();
}

void TZc::Wr(u32 Port, u8 Val)
{
  switch(Port & 0xFF)
  {
    case 0x77: // config
      // printf("0x77: %02X\n", Val);
      Val &= 0x1F;
    
      // bit 2 = ~ftcs_n
      if ((comp.ts.vdac2) && ((Cfg & 4) != (Val & 4)))
        vdac2::set_ss((Val & 4) != 0);
      
      // bit 4 = ~espcs_n
      if (comp.ts.zifi32 && ((Cfg & 16) != (Val & 16)))
        zf32::set_ss((Val & 16) != 0);

      Cfg = Val;
    break;

    case 0x57: // data
      if (!(Cfg & 2))   // SD card selected
      {
        RdBuff = SdCard.Rd();
        // printf(__FUNCTION__" ret=0x%02X\n", RdBuff);
        SdCard.Wr(Val);
      }

      else if ((Cfg & 4) && (comp.ts.vdac2))   // FT812 selected
        RdBuff = vdac2::transfer(Val);

      else if ((Cfg & 16) && comp.ts.zifi32)   // ZiFi32 selected
        RdBuff = (Port & 0x10000) ? zf32::transfer_dma(Val) : zf32::transfer(Val);

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
        // printf(__FUNCTION__" ret=0x%02X\n", RdBuff);
        SdCard.Wr(0xff);
      }

      else if ((Cfg & 4) && (comp.ts.vdac2))   // FT812 selected
        RdBuff = vdac2::transfer(0xFF);

      else if ((Cfg & 16) && comp.ts.zifi32)   // ZiFi32 selected
        RdBuff = (Port & 0x10000) ? zf32::transfer_dma(0xFF) : zf32::transfer(0xFF);

      else
        RdBuff = 255;
      
      //printf("\nout FF  IN %02X (next %02X)",tmp,RdBuff);
      return tmp;
  }

  return 0xFF;
}

TZc Zc;
