#pragma once
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
};

extern TZc Zc;
