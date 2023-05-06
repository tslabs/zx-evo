#include "std.h"
#include "emul.h"
#include "vars.h"
#include "vs1001.h"
#include "bass.h"
#include "sound/snd_bass.h"

bool SkipZeroes = true;

void TRingBuffer::Reset()
{
    CritSect.Lock();
    WrPos = RdPos = 0;
    Dreq = true;
    Count = 0;
    Cancelled = false;
    NotEmpty.Reset();
    CritSect.Unlock();
}

void TRingBuffer::Put(u8 Val)
{
    CritSect.Lock();
    Buf[WrPos++] = Val;
    WrPos &= (Size - 1);
    Count++;
    assert(Count < Size);
    Dreq = (Size - Count) > 32;
    NotEmpty.Set();
    CritSect.Unlock();
}

u32 TRingBuffer::Get(void *Data, u32 ReqLen)
{
    u32 RetLen;
    u8 *Ptr = (u8 *)Data;

retry:
    NotEmpty.Wait();

    CritSect.Lock();
    if (Count == 0)
    {
        CritSect.Unlock();
        if (Cancelled)
            return 0;
        goto retry;
    }

    assert(Count != 0);

    if (Cancelled)
    {
        CritSect.Unlock();
        return 0;
    }

    RetLen = min(Count, ReqLen);
    if (RetLen)
    {
        u32 RdLen = min(RetLen, Size - RdPos);
        memcpy(Ptr, &Buf[RdPos], RdLen);
        RdPos += RdLen;
        RdPos &= (Size - 1);
        Ptr += RdLen;
        if (RetLen > RdLen)
        {
            assert(RdPos == 0);
            RdLen = RetLen - RdLen;
            memcpy(Ptr, Buf, RdLen);
            RdPos += RdLen;
            RdPos &= (Size - 1);
        }
        Count -= RetLen;

        Dreq = (Size - Count) > 32;

        if (Count == 0)
            NotEmpty.Reset();
    }
    CritSect.Unlock();

    return RetLen;
}

BASS_FILEPROCS TVs1001::Procs = { TVs1001::StreamClose, TVs1001::StreamLen, TVs1001::StreamRead, TVs1001::StreamSeek };

TVs1001::TVs1001() : EventStreamClose(FALSE)
{
    Mp3Stream = 0;
    ThreadHandle = 0;

    Regs[VOL] = 0; // max volume on all channels

    Reset();

    SetVol(Regs[VOL]);
}

void TVs1001::Reset()
{
//    printf(__FUNCTION__"\n");

    memset(Regs, 0, sizeof(Regs));

    CurState = ST_IDLE;
    Idx = 0;
    Msb = Lsb = 0xFF;
    nCs = true;
    RingBuffer.Reset();

    if (conf.gs_type != 1)
        return;

    static bool BassInit = false;
    if (!BassInit)
    {
        BASS::Init(-1, conf.sound.fq, BASS_DEVICE_LATENCY, wnd, 0);
        ThreadHandle = (HANDLE)_beginthreadex(0, 0, Thread, this, 0, 0);
        BassInit = true;
    }
}

void TVs1001::SetNcs(bool nCs)
{
    TVs1001::nCs = nCs;
    if (nCs)
        CurState = ST_IDLE;
}

u8 TVs1001::Rd()
{
    if (nCs) // codec not selected
        return 0xFF;

    u16 Val = Rd(Idx);
    u8 RetVal = 0xFF;
    TState NextState = CurState;
    switch(CurState)
    {
    case ST_RD_MSB: RetVal = (Val >> 8U) & 0xFF; NextState = ST_RD_LSB; break;
    case ST_RD_LSB: RetVal = Val & 0xFF;  NextState = ST_IDLE; break;
    }
//    printf("Vs1001: RD %s->%s, val = 0x%X\n", State2Str(CurState), State2Str(NextState), RetVal);
    CurState = NextState;

    return RetVal;
}

u16 TVs1001::Rd(int Idx)
{
    switch(Idx)
    {
    case MODE:
    case CLOCKF:
    case STATUS:
    case AUDATA:
    case HDAT0:
    case HDAT1:
    case AIADDR:
    case VOL:
    case AICTRL0:
    case AICTRL1:
        return Regs[Idx];
    case DECODE_TIME:
        return GetDecodeTime();
    }
    return 0xFFFF;
}

void TVs1001::Wr(int Idx, u16 Val)
{
    switch(Idx)
    {
    case CLOCKF:
    case WRAM_W:
    case WRAMADDR_W:
    case AIADDR:
    case AICTRL0:
    case AICTRL1:
        Regs[Idx] = Val;
    break;

    case VOL:
        Regs[Idx] = Val;
        SetVol(Val);
    break;

    case MODE:
        Regs[Idx] = Val;
        if (Val & SM_RESET)
            SoftReset();
    break;

    case STATUS:
        Regs[Idx] = Val & 0xF; // vs1001
    break;
    }
}

void TVs1001::WrCmd(u8 Val)
{
    TState NextState = ST_IDLE;

    if (nCs) // codec not selected
        return;

    switch(CurState)
    {
    case ST_IDLE:
        switch(Val)
        {
        case CMD_RD: NextState = ST_RD_IDX; break;
        case CMD_WR: NextState = ST_WR_IDX; break;
        default:
//            printf("Vs1001: invalid cmd = 0x%X\n", Val);
           ;
        }
    break;
    case ST_RD_IDX: Idx = Val; NextState = ST_RD_MSB; break;
    case ST_WR_IDX: Idx = Val; NextState = ST_WR_MSB; break;
    case ST_WR_MSB: Msb = Val; NextState = ST_WR_LSB; break;
    case ST_WR_LSB:
        Lsb = Val;
        Wr(Idx, (Msb << 8U) | Lsb);
    break;

    case ST_RD_MSB:
    case ST_RD_LSB:
        NextState = CurState;
    break;

    default:
//        printf("Vs1001: WR invalid state = %s\n", State2Str(CurState));
        ;
    }
//    printf("Vs1001: WR %s->%s, val = 0x%X\n", State2Str(CurState), State2Str(NextState), Val);
    CurState = NextState;
}

void TVs1001::Wr(u8 Val)
{
//    printf(__FUNCTION__" val = 0x%X\n", Val);
    if (!Mp3Stream && SkipZeroes)
    {
        if (Val == 0) // skip zeroes before format detection
            return;
        SkipZeroes = false;
    }

    RingBuffer.Put(Val);
}

void TVs1001::SetVol(u16 Vol)
{
//   __debugbreak();
   if (!Mp3Stream)
       return;

   float VolDbR = (Vol & 0xFF) / 2.0f;
   float VolDbL = ((Vol >> 8U) & 0xFF) / 2.0f;
   float VolR = powf(10.0f, -VolDbR / 10.0f);
   float VolL = powf(10.0f, -VolDbL / 10.0f);
   float Pan = VolR - VolL;
   float VolM = (VolR + VolL) / 2.0f;

//   printf("%s, Vol = %u\n", __FUNCTION__, Vol);
   if (!BASS::ChannelSetAttribute(Mp3Stream, BASS_ATTRIB_VOL, VolM))
       printf("BASS_ChannelSetAttribute() [vol]\n");
   if (!BASS::ChannelSetAttribute(Mp3Stream, BASS_ATTRIB_PAN, Pan))
       printf("BASS_ChannelSetAttribute() [pan]\n");
}

u16 TVs1001::GetDecodeTime()
{
    if (!Mp3Stream)
        return 0;

    u64 Pos = BASS::ChannelGetPosition(Mp3Stream, BASS_POS_BYTE);
    double Time = BASS::ChannelBytes2Seconds(Mp3Stream, Pos);
    return (u16)Time;
}

// called from main loop when processing ngs z80 code
void TVs1001::SoftReset()
{
//    printf("%s\n", __FUNCTION__);

    ShutDown();

    if (Mp3Stream)
    {
    	if (!BASS::ChannelStop(Mp3Stream))
    	{
            printf("BASS_ChannelStop()\n");
            assert(false);
        }

        if (!BASS::StreamFree(Mp3Stream))
        {
            printf("BASS_ChannelStop()\n");
            assert(false);
        }

        EventStreamClose.Wait();
        EventStreamClose.Reset();
        Mp3Stream = 0;
    }

    RingBuffer.Reset();
    SkipZeroes = true;
    ThreadHandle = (HANDLE)_beginthreadex(0, 0, Thread, this, 0, 0);
}

void TVs1001::ShutDown()
{
    RingBuffer.Cancel();

    if (ThreadHandle)
    {
        WaitForSingleObject(ThreadHandle, INFINITE);
        CloseHandle(ThreadHandle);
        ThreadHandle = 0;
    }
}

// called from main loop priodicaly
void TVs1001::Play()
{
   if (!Mp3Stream)
       return;

   BASS::ChannelPlay(Mp3Stream, FALSE);
}

// stream file format detection tread
void TVs1001::Thread()
{
//    printf(__FUNCTION__"\n");
    Mp3Stream = BASS::StreamCreateFileUser(STREAMFILE_BUFFER, BASS_SAMPLE_FLOAT | BASS_STREAM_BLOCK, &Procs, this);
    if (!Mp3Stream)
    {
//        printf(__FUNCTION__" Err = %d\n", BASS_ErrorGetCode());
    }
//    printf(__FUNCTION__"->Exit\n");
}

// bass asynchronous callback
void TVs1001::StreamClose()
{
    EventStreamClose.Set();
}

// bass asynchronous callback
u64 TVs1001::StreamLen()
{
    return 0;
}

// bass asynchronous callback
DWORD TVs1001::StreamRead(void *Buffer, DWORD Length)
{
    DWORD Len = RingBuffer.Get(Buffer, Length);
//    printf(__FUNCTION__"->%d/%d\n", Length, Len);

/*
    static FILE *f = fopen("dump.mp3", "wb");
    if (Len != 0)
        fwrite(Buffer, Len, 1, f);
*/
    return Len;
}

// bass asynchronous callback
BOOL TVs1001::StreamSeek(u64 offset)
{
    return FALSE;
}

#define STATE2STR(x) case x:  return #x

const char *TVs1001::State2Str(TState State)
{
    switch(State)
    {
    STATE2STR(ST_IDLE);
    STATE2STR(ST_RD);
    STATE2STR(ST_WR);
    STATE2STR(ST_RD_IDX);
    STATE2STR(ST_WR_IDX);
    STATE2STR(ST_RD_MSB);
    STATE2STR(ST_RD_LSB);
    STATE2STR(ST_WR_MSB);
    STATE2STR(ST_WR_LSB);
    }
    return "???";
}

TVs1001 Vs1001;
