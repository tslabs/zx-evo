#pragma once
#include "sysdefs.h"
#include "sdcard.h"

// Z-Controller by KOE
// Only SD-card

class TZc
{
    TSdCard SdCard;
    u8 Cfg;
    u8 Status;
    u8 RdBuff;
public:
    void Reset();
    void Open(const char *Name) { SdCard.Open(Name); }
    void Close() { SdCard.Close(); }
    void Wr(u32 Port, u8 Val);
    u8 Rd(u32 Port);
    void DmaRdStart(u32);
    void DmaWrStart();
    void DmaWrEnd();
};

extern TZc Zc;
