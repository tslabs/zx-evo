#pragma once
#include "std.h"
#include "sysdefs.h"
#include "util.h"

#undef SD_SEND

// S_CTRL bits
const u8 _MPNCS  = 2;
const u8 _MPXRS  = 4;
const u8 _MPHLF  = 8;

// S_STAT bits
const u8 _MPDRQ  = 1;

const u8 S_CTRL  = 0x11; // RW
const u8 S_STAT  = 0x12; // R

const u8 SD_SEND = 0x13; // W
const u8 SD_READ = 0x13; // R
const u8 SD_RSTR = 0x14; // R

const u8 MD_SEND = 0x14; // W same as SD_RSTR!!!

const u8 MC_SEND = 0x15; // W
const u8 MC_READ = 0x15; // R

class TRingBuffer
{
    TCriticalSection CritSect;
    TEvent NotEmpty;
    const u32 Size;
    u8 Buf[4*1024];
    u32 WrPos;
    u32 RdPos;
    u32 Count;
    bool Dreq;
    bool Cancelled;
public:
    TRingBuffer() : NotEmpty(FALSE), Size(4*1024)
    {
        Cancelled = false;
        Reset();
    }
    void Cancel()
    {
        CritSect.Lock();
        Cancelled = true;
        NotEmpty.Set();
        CritSect.Unlock();
    }
    void Reset();
    void Put(u8 Val);
    u32 Get(void *Data, u32 ReqLen);
    bool GetDreq() const { return Dreq; }
};

class TVs1001
{
    enum TRegs
    {
       MODE = 0,   // RW
       STATUS,     // RW
       INT_FCTLH,  // NA
       CLOCKF,     // RW
       DECODE_TIME,// R
       AUDATA,     // R
       WRAM_W,     // W
       WRAMADDR_W, // W
       HDAT0,      // R
       HDAT1,      // R
       AIADDR,     // RW
       VOL,        // RW
       RESERVED,   // NA
       AICTRL0,    // RW
       AICTRL1,    // RW
       REG_COUNT
    };

    static const u8 SM_RESET = 4;

    enum TState { ST_IDLE, ST_RD, ST_WR, ST_RD_IDX, ST_WR_IDX, ST_RD_MSB, ST_RD_LSB, ST_WR_MSB, ST_WR_LSB };
    enum TCmd { CMD_WR = 2, CMD_RD = 3 };
    u16 Regs[REG_COUNT];
    TState CurState;
    bool nCs;
    int Idx;
    u8 Msb, Lsb;
    TRingBuffer RingBuffer;
    TEvent EventStreamClose;
    HANDLE ThreadHandle;
    HSTREAM Mp3Stream;
    static BASS_FILEPROCS Procs;
public:
    TVs1001();
    void Reset();
    void ShutDown();
    void SetNcs(bool nCs);
    void Wr(u8 Val); // sdi mpeg data write
    void WrCmd(u8 Val); // sci write
    u8 Rd(); // sci read
    void Play(); // play buffered data
    bool GetDreq() const { return RingBuffer.GetDreq(); }
private:
    void Wr(int Idx, u16 Val); // sci write
    u16 Rd(int Idx); // sci read

    void SoftReset();
    void SetVol(u16 Vol);
    u16 GetDecodeTime();

    void Thread();
    void StreamClose();
    u64 StreamLen();
    DWORD StreamRead(void *Buffer, DWORD Length);
    BOOL StreamSeek(u64 offset);

    static unsigned CALLBACK Thread(void *This)
    {
        ((TVs1001 *)This)->Thread();
        return 0;
    }

    static void CALLBACK StreamClose(void *This)
    { ((TVs1001 *)This)->StreamClose(); }

    static u64 CALLBACK StreamLen(void *This)
    { return ((TVs1001 *)This)->StreamLen(); }

    static DWORD CALLBACK StreamRead(void *Buffer, DWORD Length, void *This)
    { return ((TVs1001 *)This)->StreamRead(Buffer, Length); }

    static BOOL CALLBACK StreamSeek(u64 offset, void *This)
    { return ((TVs1001 *)This)->StreamSeek(offset); }

    static const char *State2Str(TState State);
};

extern TVs1001 Vs1001;
