#include "std.h"
#include "emul.h"
#include "vars.h"
#include "sdcard.h"
#include "util.h"

//-----------------------------------------------------------------------------

void TSdCard::Reset()
{
//    printf(__FUNCTION__"\n");

    CurrState = ST_IDLE;
    ArgCnt = 0;
    Cmd = CMD_INVALID;
    DataBlockLen = 512;
    DataCnt = 0;

    CsdCnt = 0;
    //memset(Csd, 0, sizeof(Csd));
    Csd[0] = 0x00;
    Csd[1] = 0x0E;
    Csd[2] = 0x00;
    Csd[3] = 0x32;
    Csd[4] = 0x5B;
    Csd[5] = 0x59;
    Csd[6] = 0x03;
    Csd[7] = 0xFF;
    Csd[8] = 0xED;
    Csd[9] = 0xB7;
    Csd[10] = 0xBF;
    Csd[11] = 0xBF;
    Csd[12] = 0x06;
    Csd[13] = 0x40;
    Csd[14] = 0x00;
    Csd[15] = 0xF5;

    CidCnt = 0;
    //memset(Cid, 0, sizeof(Cid));
    Cid[0] = 0x00;
    Cid[1] = 'U';
    Cid[2] = 'S';
    Cid[3] = 'U';
    Cid[4] = 'S';
    Cid[5] = '3';
    Cid[6] = '7';
    Cid[7] = '6';
    Cid[8] = 0x03;
    Cid[9] = 0x12;
    Cid[10] = 0x34;
    Cid[11] = 0x56;
    Cid[12] = 0x78;
    Cid[13] = 0x00;
    Cid[14] = 0xC1;
    Cid[15] = 0x0F;

    OcrCnt = 0;

    R7_Cnt = 0;

    AppCmd = false;
}

//-----------------------------------------------------------------------------

void TSdCard::Wr(u8 Val)
{
    static u32 WrPos = -1;
    // printf(__FUNCTION__" Val = %X\n", Val);

    if (!Image)
        return;

    switch(CurrState)
    {
        case ST_IDLE:
        //case ST_WR_DATA_SIG:
        {
            if ((Val & 0xC0) != 0x40) // start=0, transm=1
               break;

            Cmd = TCmd(Val & 0x3F);
            if (!AppCmd)
            {
                switch(Cmd) // Check commands
                {

                case CMD_SEND_CSD:
                    CsdCnt = 0;
                break;

                case CMD_SEND_CID:
                    CidCnt = 0;
                break;

                case CMD_READ_OCR:
                    OcrCnt = 0;
                break;

                case CMD_SEND_IF_COND:
                    R7_Cnt = 0;
                //break;

                //case CMD_APP_CMD:
                //    AppCmd = true;
                }
            }
            NextState = ST_RD_ARG;
            ArgCnt = 0;
        }
        break;

        case ST_RD_ARG:
            NextState = ST_RD_ARG;
            ArgArr[3 - ArgCnt++] = Val;

//            printf(__FUNCTION__" ST_RD_ARG val=0x%X\n", Val);
            if (ArgCnt == 4)
            {
                if (!AppCmd)
                {
                    switch(Cmd)
                    {
                    case CMD_SET_BLOCKLEN:
                        if (Arg<=4096)  DataBlockLen = Arg;
                    break;

                    case CMD_READ_SINGLE_BLOCK:
//                        printf(__FUNCTION__" CMD_READ_SINGLE_BLOCK, Addr = 0x%X\n", Arg);
                        fseek(Image, Arg, SEEK_SET);
                        fread(Buf, DataBlockLen, 1, Image);
                    break;

                    case CMD_READ_MULTIPLE_BLOCK:
//                        printf(__FUNCTION__" CMD_READ_MULTIPLE_BLOCK, Addr = 0x%X\n", Arg);
                        fseek(Image, Arg, SEEK_SET);
                        fread(Buf, DataBlockLen, 1, Image);
                    break;

                    case CMD_WRITE_BLOCK:
//                        printf(__FUNCTION__" CMD_WRITE_BLOCK, Addr = 0x%X\n", Arg);
                    break;

                    case CMD_WRITE_MULTIPLE_BLOCK:
                        WrPos = Arg;
//                        printf(__FUNCTION__" CMD_WRITE_MULTIPLE_BLOCK, Addr = 0x%X\n", Arg);
                    break;
                    }
                }

                NextState = ST_RD_CRC;
                ArgCnt = 0;
            }
        break;

        case ST_RD_CRC:
//            printf(__FUNCTION__" ST_RD_CRC val=0x%X\n", Val);
            NextState = GetRespondType();
            CurrState = ST_DELAY_1;
            return;

        case ST_RD_DATA_SIG:
            if (Val==0xFE) // Проверка сигнатуры данных
            {
                DataCnt = 0;
                NextState = ST_RD_DATA;
            }
            else
                NextState = ST_RD_DATA_SIG;
        break;

        case ST_RD_DATA_SIG_MUL:
            switch(Val)
            {
            case 0xFC: // Проверка сигнатуры данных
//                printf(__FUNCTION__" ST_RD_DATA_SIG_MUL, Start\n");
                DataCnt = 0;
                NextState = ST_RD_DATA_MUL;
            break;
            case 0xFD: // Окончание передачи
//                printf(__FUNCTION__" ST_RD_DATA_SIG_MUL, Stop\n");
                DataCnt = 0;
                NextState = ST_IDLE;
            break;
            default:
                NextState = ST_RD_DATA_SIG_MUL;
            }
        break;

        case ST_RD_DATA: // Прием данных в буфер
        {
//            printf(__FUNCTION__" ST_RD_DATA, Addr = 0x%X, Idx=%d\n", Arg, DataCnt);
            Buf[DataCnt++] = Val;
            NextState = ST_RD_DATA;
            if (DataCnt == DataBlockLen) // Запись данных в SD карту
            {
                DataCnt = 0;
//                printf(__FUNCTION__" ST_RD_DATA, Addr = 0x%X, write to disk\n", Arg);
                fseek(Image, Arg, SEEK_SET);
                fwrite(Buf, DataBlockLen, 1, Image);
                NextState = ST_RD_CRC16_1;
            }
        }
        break;

        case ST_RD_DATA_MUL: // Прием данных в буфер
        {
//            printf(__FUNCTION__" ST_RD_DATA_MUL, Addr = 0x%X, Idx=%d\n", WrPos, DataCnt);
            Buf[DataCnt++] = Val;
            NextState = ST_RD_DATA_MUL;
            if (DataCnt == DataBlockLen) // Запись данных в SD карту
            {
                DataCnt = 0;
//                printf(__FUNCTION__" ST_RD_DATA_MUL, Addr = 0x%X, write to disk\n", WrPos);
                fseek(Image, WrPos, SEEK_SET);
                fwrite(Buf, DataBlockLen, 1, Image);
                WrPos += DataBlockLen;
                NextState = ST_RD_CRC16_1;
            }
        }
        break;

        case ST_RD_CRC16_1: // Чтение старшего байта CRC16
            NextState = ST_RD_CRC16_2;
        break;

        case ST_RD_CRC16_2: // Чтение младшего байта CRC16
            NextState = ST_WR_DATA_RESP;
        break;

        default:
//            printf(__FUNCTION__" St=0x%X,  val=0x%X\n", CurrState, Val);
            return;
    }

    CurrState = NextState;
}

//-----------------------------------------------------------------------------

u8 TSdCard::Rd()
{
    // printf(__FUNCTION__" cmd=0x%X, St=0x%X\n", Cmd, CurrState);

    if (!Image)
        return 0xFF;

    static u32 delay = 0;

    switch (CurrState)
    {
        case ST_DELAY_1:
            delay = 2;
            CurrState = ST_DELAY_2;

        case ST_DELAY_2:
            if (delay)
            {
                delay--;
                return 0xFF;
            }
            else
                CurrState = NextState;
                NextState = ST_IDLE;
        break;
    }

    switch(Cmd)
    {
    case CMD_GO_IDLE_STATE:
        if (CurrState == ST_R1)
        {
//            Cmd = CMD_INVALID;
            CurrState = ST_IDLE;
            return 1;
        }

    case CMD_SEND_OP_COND:
        if (CurrState == ST_R1)
        {
//            Cmd = CMD_INVALID;
            CurrState = ST_IDLE;
            return 0;
        }
    break;

    case CMD_SEND_STATUS:
        if (CurrState == ST_R2)
        {
            switch (R2_Cnt++)
            {
             case 0: return 0x00; // R1
             case 1: return 0x00;
             default:
                 CurrState = ST_IDLE;
                 R2_Cnt = 0;
            }
        }
    break;

    case CMD_SET_BLOCKLEN:
        if (CurrState == ST_R1)
        {
//            Cmd = CMD_INVALID;
            CurrState = ST_IDLE;
            return 0;
        }
    break;

    case CMD_SEND_IF_COND:
        if (CurrState == ST_R7)
        {
            switch (R7_Cnt++)
            {
             case 0: return 0x01; // R1
             case 1: return 0x00;
             case 2: return 0x00;
             case 3: return 0x01;
             default:
                 CurrState = ST_IDLE;
                 R7_Cnt = 0;
                 return ArgArr[0]; // echo-back
            }
        }
    break;

    case CMD_READ_OCR:
        if (CurrState == ST_R3)
        {
            switch (OcrCnt++)
            {
             case 0: return 0x00; // R1
             case 1: return 0x80;
             case 2: return 0xFF;
             case 3: return 0x80;
             default:
                CurrState = ST_IDLE;
                OcrCnt = 0;
                return 0x00;
            }
        }
    break;

    case CMD_APP_CMD:
        if (CurrState == ST_R1)
        {
            CurrState = ST_IDLE;
            return 0;
        }
    break;

    case CMD_SD_SEND_OP_COND:
        if (CurrState == ST_R1)
        {
            CurrState = ST_IDLE;
            return 0;
        }
    break;

    case CMD_CRC_ON_OFF:
        if (CurrState == ST_R1)
        {
            CurrState = ST_IDLE;
            return 0;
        }
    break;

    case CMD_STOP_TRANSMISSION:
        switch(CurrState)
        {
        case ST_R1:
            DataCnt = 0;
            CurrState = ST_R1b;
            return 0;
        case ST_R1b:
            CurrState = ST_IDLE;
            return 0xFF;
        }
    break;

    case CMD_READ_SINGLE_BLOCK:
        switch(CurrState)
        {
          i32 cpu_dt;

          case ST_R1:
              CurrState = ST_DELAY_S;
              InitialCPUt = cpu.t;
              return 0;

          case ST_DELAY_S:
              cpu_dt = cpu.t - InitialCPUt;
              if (cpu_dt < 0)
                cpu_dt += conf.frame;
              if ((u32)cpu_dt >= conf.sd_delay)
                CurrState = ST_STARTBLOCK;
              return 0xFF;

          case ST_STARTBLOCK:
              CurrState = ST_WR_DATA;
              DataCnt = 0;
              return 0xFE;

          case ST_WR_DATA:
          {
              u8 Val = Buf[DataCnt++];
              if (DataCnt == DataBlockLen)
              {
                DataCnt = 0;
                CurrState = ST_WR_CRC16_1;
              }
              return Val;
          }

          case ST_WR_CRC16_1:
              CurrState = ST_WR_CRC16_2;
              return 0xFF; // crc
          case ST_WR_CRC16_2:
              CurrState = ST_IDLE;
              Cmd = CMD_INVALID;
              return 0xFF; // crc
        }
//        Cmd = CMD_INVALID;
    break;

    case CMD_READ_MULTIPLE_BLOCK:
        switch(CurrState)
        {
          i32 cpu_dt;

          case ST_R1:
              CurrState = ST_DELAY_S;
              InitialCPUt = cpu.t;
              return 0;

          case ST_DELAY_S:
              cpu_dt = cpu.t - InitialCPUt;
              if (cpu_dt < 0)
                cpu_dt += conf.frame;
              if ((u32)cpu_dt >= conf.sd_delay)
                CurrState = ST_STARTBLOCK;
              return 0xFF;

          case ST_STARTBLOCK:
              CurrState = ST_IDLE;
              DataCnt = 0;
              return 0xFE;

          case ST_IDLE:
          {
              if (DataCnt<DataBlockLen)
              {
                u8 Val = Buf[DataCnt++];
                if (DataCnt == DataBlockLen)
                    fread(Buf, DataBlockLen, 1, Image);
                return Val;
              }
              else if (DataCnt>(DataBlockLen+8))
              {
                DataCnt=0;
                return 0xFE; // next startblock
              }
              else
              {
                DataCnt++;
                return 0xFF; // crc & pause
              }
          }

/*
        case ST_R1:
            CurrState = ST_WR_DATA_SIG;
            return 0;
        case ST_WR_DATA_SIG:
            CurrState = ST_IDLE;
            DataCnt = 0;
            return 0xFE;
        case ST_IDLE:
        {
            u8 Val = Buf[DataCnt++];
            if (DataCnt == DataBlockLen)
            {
                DataCnt = 0;
                fread(Buf, DataBlockLen, 1, Image);
                CurrState = ST_WR_CRC16_1;
            }
            return Val;
        }
        case ST_WR_CRC16_1:
            CurrState = ST_WR_CRC16_2;
            return 0xFF;
        case ST_WR_CRC16_2:
            CurrState = ST_WR_DATA_SIG;
            return 0xFF;
*/
        }
    break;

    case CMD_SEND_CSD:
        switch(CurrState)
        {
        case ST_R1:
            CurrState = ST_DELAY_S;
            return 0;
        case ST_DELAY_S:
            CurrState = ST_STARTBLOCK;
            return 0xFF;
        case ST_STARTBLOCK:
            CurrState = ST_WR_DATA;
            return 0xFE;
        case ST_WR_DATA:
        {
            u8 Val = Csd[CsdCnt++];
            if (CsdCnt == 16)
            {
                CsdCnt = 0;
                CurrState = ST_IDLE;
                Cmd = CMD_INVALID;
            }
            return Val;
        }
        }
//        Cmd = CMD_INVALID;
    break;

    case CMD_SEND_CID:
        switch(CurrState)
        {
        case ST_R1:
            CurrState = ST_DELAY_S;
            return 0;
        case ST_DELAY_S:
            CurrState = ST_STARTBLOCK;
            return 0xFF;
        case ST_STARTBLOCK:
            CurrState = ST_WR_DATA;
            return 0xFE;
        case ST_WR_DATA:
        {
            u8 Val = Cid[CidCnt++];
            if (CidCnt == 16)
            {
                CidCnt = 0;
                CurrState = ST_IDLE;
                Cmd = CMD_INVALID;
            }
            return Val;
        }
        }
//        Cmd = CMD_INVALID;
    break;

    case CMD_WRITE_BLOCK:
//        printf(__FUNCTION__" cmd=0x%X, St=0x%X\n", Cmd, CurrState);
        switch(CurrState)
        {
        case ST_R1:
            CurrState = ST_RD_DATA_SIG;
            return 0x00;

        case ST_WR_DATA_RESP:
        {
            CurrState = ST_IDLE;
            u8 Resp = ((STAT_DATA_ACCEPTED) << 1) | 1;
            return Resp;
        }
        }
    break;

    case CMD_WRITE_MULTIPLE_BLOCK:
        switch(CurrState)
        {
        case ST_R1:
            CurrState = ST_RD_DATA_SIG_MUL;
            return 0x00;    // !!! check this !!!
        case ST_WR_DATA_RESP:
        {
            CurrState = ST_RD_DATA_SIG_MUL;
            u8 Resp = ((STAT_DATA_ACCEPTED) << 1) | 1;
            return Resp;
        }
        }
    break;
    }

    if (CurrState == ST_R1) // CMD_INVALID
    {
        CurrState = ST_IDLE;
        return 0x05;
    }

    return 0xFF;
}

//-----------------------------------------------------------------------------

TSdCard::TState TSdCard::GetRespondType()
{
    if (!AppCmd)
    {
        switch(Cmd)
        {
        case CMD_APP_CMD:
            AppCmd = true;
            return ST_R1;

        case CMD_GO_IDLE_STATE:
        case CMD_SEND_OP_COND:
        case CMD_SET_BLOCKLEN:
        case CMD_READ_SINGLE_BLOCK:
        case CMD_READ_MULTIPLE_BLOCK:
        case CMD_CRC_ON_OFF:
        case CMD_STOP_TRANSMISSION:
        case CMD_SEND_CSD:
        case CMD_SEND_CID:
            return ST_R1;

        case CMD_SEND_STATUS:
            return ST_R2;

        case CMD_READ_OCR:
            return ST_R3;

        case CMD_SEND_IF_COND:
            return ST_R7;

        case CMD_WRITE_BLOCK:
        case CMD_WRITE_MULTIPLE_BLOCK:
            return ST_R1;
        }
    }
    else
    {
        AppCmd = false;
        switch(Cmd)
        {
            case CMD_SD_SEND_OP_COND:
                return ST_R1;
        }
    }

    Cmd = CMD_INVALID;
    return ST_R1;
    //return ST_IDLE;
}

//-----------------------------------------------------------------------------

void TSdCard::Open(const char *Name)
{
//    printf(__FUNCTION__"\n");
    assert(!Image);
    Image = fopen(Name, "r+b");
    if (!Image && Name[0])
    {
        errmsg("can't find SD card image `%s'", Name);
    }
}

//-----------------------------------------------------------------------------

void TSdCard::Close()
{
    if (Image)
    {
        fclose(Image);
        Image = 0;
    }
}

//-----------------------------------------------------------------------------

TSdCard SdCard;

//-----------------------------------------------------------------------------
