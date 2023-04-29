#pragma once
#include "sysdefs.h"

const u8 _SDNCS = 1;
const u8 _SDDET = 2;
const u8 _SDWP  = 4;

class TSdCard
{
    enum TCmd
    {
        CMD_INVALID = -1,
        CMD0 = 0,
        CMD_GO_IDLE_STATE = CMD0,
        CMD1 = 1,
        CMD_SEND_OP_COND = CMD1,
        CMD8 = 8,
        CMD_SEND_IF_COND = CMD8,
        CMD9 = 9,
        CMD_SEND_CSD = CMD9,
        CMD10 = 10,
        CMD_SEND_CID = CMD10,
        CMD12 = 12,
        CMD_STOP_TRANSMISSION = CMD12,
        CMD13 = 13,
        CMD_SEND_STATUS = CMD13,
        CMD16 = 16,
        CMD_SET_BLOCKLEN = CMD16,
        CMD17 = 17,
        CMD_READ_SINGLE_BLOCK = CMD17,
        CMD18 = 18,
        CMD_READ_MULTIPLE_BLOCK = CMD18,
        CMD24 = 24,
        CMD_WRITE_BLOCK = CMD24,
        CMD25 = 25,
        CMD_WRITE_MULTIPLE_BLOCK = CMD25,
        CMD55 = 55,
        CMD_APP_CMD = CMD55,
        CMD58 = 58,
        CMD_READ_OCR = CMD58,
        CMD59 = 59,
        CMD_CRC_ON_OFF = CMD59,
        ACMD41 = 41,
        CMD_SD_SEND_OP_COND = ACMD41,
    };

    enum TState
    {
        ST_IDLE = 0,
        ST_RD_ARG,
        ST_RD_CRC,
        ST_R1,
        ST_R1b,
        ST_R2,
        ST_R3,
        ST_R7,
        ST_STARTBLOCK,
        ST_DELAY_1,
        ST_DELAY_2,
        ST_DELAY_S,
        // ST_WR_DATA_SIG,
        ST_WR_DATA,
        ST_WR_CRC16_1,
        ST_WR_CRC16_2,
        ST_RD_DATA_SIG,
        ST_RD_DATA,
        ST_RD_DATA_MUL,
        ST_RD_CRC16_1,
        ST_RD_CRC16_2,
        ST_WR_DATA_RESP,
        ST_RD_DATA_SIG_MUL
    };

    enum TDataStatus
    {
        STAT_DATA_ACCEPTED = 2,
        STAT_DATA_CRC_ERR = 5,
        STAT_DATA_WR_ERR = 6
    };
    TState CurrState;

#pragma pack(push, 1)
    union
    {
        u8 ArgArr[4];
        u32 Arg;
    };
#pragma pack(pop)

    u32 ArgCnt;

    u32 OcrCnt;

    u32 R2_Cnt = 0;
    u32 R7_Cnt = 0;

    u8 Cid[16];
    u8 Csd[16];

    u32 CidCnt;
    u32 CsdCnt;

    bool AppCmd;
    TCmd Cmd;
    u32 DataBlockLen;
    u32 DataCnt;
    u32 InitialCPUt;

    u8 Buf[4096];

    FILE *Image;

    TState NextState = ST_IDLE;

public:
    TSdCard() { Image = 0; Reset(); }
    void Reset();
    void Open(const char *Name);
    void Close();
    void Wr(u8 Val);
    u8 Rd();
private:
    TState GetRespondType();
};

extern TSdCard SdCard;
