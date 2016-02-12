#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

//#define DUMP_HDD_IO 1

const int MAX_DEVICES = MAX_PHYS_HD_DRIVES+2*MAX_PHYS_CD_DRIVES;

PHYS_DEVICE phys[MAX_DEVICES];
int n_phys = 0;

/*
// this function is untested
void ATA_DEVICE::exec_mode_select()
{
   intrq = 1;
   command_ok();

   struct {
      SCSI_PASS_THROUGH_DIRECT p;
      u8 sense[0x40];
   } srb = { 0 }, dst;

   srb.p.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
   *(CDB*)&srb.p.Cdb = cdb;
   srb.p.CdbLength = sizeof(CDB);
   srb.p.DataIn = SCSI_IOCTL_DATA_OUT;
   srb.p.TimeOutValue = 10;
   srb.p.DataBuffer = transbf;
   srb.p.DataTransferLength = transcount;
   srb.p.SenseInfoLength = sizeof(srb.sense);
   srb.p.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);

   DWORD outsize;
   int r = DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
                           &srb.p, sizeof(srb.p),
                           &dst, sizeof(dst),
                           &outsize, 0);

   if (!r) return;
   if (senselen = dst.p.SenseInfoLength) memcpy(sense, dst.sense, senselen);
   return;
}
*/

void init_hdd_cd()
{
   memset(&phys, 0, sizeof phys);
   if (conf.ide_skip_real)
       return;

   n_phys = 0;
   n_phys = ATA_PASSER::identify(phys + n_phys, MAX_DEVICES - n_phys);
   n_phys += ATAPI_PASSER::identify(phys + n_phys, MAX_DEVICES - n_phys);

   if (!n_phys)
       errmsg("HDD/CD emulator can't access physical drives");
}

void delstr_spaces(char *dst, char *src)
{
   for (; *src; src++)
      if (*src != ' ') *dst++ = *src;
   *dst = 0;
}

unsigned find_hdd_device(char *name)
{
   char s2[512];
   delstr_spaces(s2, name);
   for (int drive = 0; drive < n_phys; drive++)
   {
      char s1[512];
      delstr_spaces(s1, phys[drive].viewname);
      if (!stricmp(s1,s2))
          return drive;
   }
   return -1;
}

void ATA_DEVICE::configure(IDE_CONFIG *cfg)
{
   atapi_p.close(); ata_p.close();

   c = cfg->c, h = cfg->h, s = cfg->s, lba = cfg->lba; readonly = cfg->readonly;

   memset(regs, 0, sizeof(regs)); // Очищаем регистры
   command_ok(); // Сбрасываем состояние и позицию передачи данных

   phys_dev = -1;
   if (!*cfg->image)
       return;

   PHYS_DEVICE filedev, *dev;
   phys_dev = find_hdd_device(cfg->image);
   if (phys_dev == -1)
   {
      if (cfg->image[0] == '<')
      {
          errmsg("no physical device %s", cfg->image);
          *cfg->image = 0;
          return;
      }
      strcpy(filedev.filename, cfg->image);
      filedev.type = cfg->cd ? ATA_FILECD : ATA_FILEHDD;
      dev = &filedev;
   }
   else
   {
      dev = &phys[phys_dev];
      if (dev->type == ATA_NTHDD)
      {
         // read geometry from id sector
         c = *(u16*)(phys[phys_dev].idsector+2);
         h = *(u16*)(phys[phys_dev].idsector+6);
         s = *(u16*)(phys[phys_dev].idsector+12);
         lba = *(unsigned*)(phys[phys_dev].idsector+0x78);
         if (!lba)
             lba = c*h*s;
      }
   }
   DWORD errcode;
   if (dev->type == ATA_NTHDD || dev->type == ATA_FILEHDD)
   {
       dev->usage = ATA_OP_USE;
       errcode = ata_p.open(dev);
       atapi = 0;
   }

   if (dev->type == ATA_SPTI_CD || dev->type == ATA_ASPI_CD || dev->type == ATA_FILECD)
   {
       dev->usage = ATA_OP_USE;
       errcode = atapi_p.open(dev);
       atapi = 1;
   }

   if (errcode == NO_ERROR)
       return;
   errmsg("failed to open %s", cfg->image);
   //err_win32(errcode);
   *cfg->image = 0;
}

void ATA_PORT::reset()
{
   dev[0].reset(ATA_DEVICE::RESET_HARD);
   dev[1].reset(ATA_DEVICE::RESET_HARD);
}

u8 ATA_PORT::read(unsigned n_reg)
{
#ifdef DUMP_HDD_IO
u8 val = dev[0].read(n_reg) & dev[1].read(n_reg); printf("R%X:%02X ", n_reg, val); return val;
#endif
   return dev[0].read(n_reg) & dev[1].read(n_reg);
}

unsigned ATA_PORT::read_data()
{
#ifdef DUMP_HDD_IO
unsigned val = dev[0].read_data() & dev[1].read_data(); printf("r%04X ", val & 0xFFFF); return val;
#endif
   return dev[0].read_data() & dev[1].read_data();
}

void ATA_PORT::write(unsigned n_reg, u8 data)
{
#ifdef DUMP_HDD_IO
printf("R%X=%02X ", n_reg, data);
#endif
   dev[0].write(n_reg, data);
   dev[1].write(n_reg, data);
}

void ATA_PORT::write_data(unsigned data)
{
#ifdef DUMP_HDD_IO
printf("w%04X ", data & 0xFFFF);
#endif
   dev[0].write_data(data);
   dev[1].write_data(data);
}

u8 ATA_PORT::read_intrq()
{
#ifdef DUMP_HDD_IO
u8 i = dev[0].read_intrq() & dev[1].read_intrq(); printf("i%d ", !!i); return i;
#endif
   return dev[0].read_intrq() & dev[1].read_intrq();
}

void ATA_DEVICE::reset_signature(RESET_TYPE mode)
{
   reg.count = reg.sec = reg.err = 1;
   reg.cyl = atapi ? 0xEB14 : 0;
   reg.devhead &= (atapi && mode == RESET_SOFT) ? 0x10 : 0;
   reg.status = (mode == RESET_SOFT || !atapi) ? STATUS_DRDY | STATUS_DSC : 0;
}

void ATA_DEVICE::reset(RESET_TYPE mode)
{
   reg.control = 0; // clear SRST
   intrq = 0;

   command_ok();
   reset_signature(mode);
}

void ATA_DEVICE::command_ok()
{
   state = S_IDLE;
   transptr = -1;
   reg.err = 0;
   reg.status = STATUS_DRDY | STATUS_DSC;
}

u8 ATA_DEVICE::read_intrq()
{
   if (!loaded() || ((reg.devhead ^ device_id) & 0x10) || (reg.control & CONTROL_nIEN)) return 0xFF;
   return intrq? 0xFF : 0x00;
}

u8 ATA_DEVICE::read(unsigned n_reg)
{
   if (!loaded())
       return 0xFF;
   if ((reg.devhead ^ device_id) & 0x10)
   {
       return 0xFF;
   }

   if (n_reg == 7)
       intrq = 0;
   if (n_reg == 8)
       n_reg = 7; // read alt.status -> read status
   if (n_reg == 7 || (reg.status & STATUS_BSY))
   {
//	   printf("state=%d\n",state); //Alone Coder
       return reg.status;
   } // BSY=1 or read status
   // BSY = 0
   //// if (reg.status & STATUS_DRQ) return 0xFF;    // DRQ.  ATA-5: registers should not be queried while DRQ=1, but programs do this!
   // DRQ = 0
   return regs[n_reg];
}

unsigned ATA_DEVICE::read_data()
{
   if (!loaded())
       return 0xFFFFFFFF;
   if ((reg.devhead ^ device_id) & 0x10)
       return 0xFFFFFFFF;
   if (/* (reg.status & (STATUS_DRQ | STATUS_BSY)) != STATUS_DRQ ||*/ transptr >= transcount)
       return 0xFFFFFFFF;

   // DRQ=1, BSY=0, data present
   unsigned result = *(unsigned*)(transbf + transptr*2);
   transptr++;
//   printf(__FUNCTION__" data=0x%04X\n", result & 0xFFFF);

   if (transptr < transcount)
       return result;
   // look to state, prepare next block
   if (state == S_READ_ID || state == S_READ_ATAPI)
       command_ok();
   if (state == S_READ_SECTORS)
   {
//       __debugbreak();
//       printf("dev=%d, cnt=%d\n", device_id, reg.count);
       if (!--reg.count)
           command_ok();
       else
       {
           next_sector();
           read_sectors();
       }
   }

   return result;
}

char ATA_DEVICE::exec_ata_cmd(u8 cmd)
{
//   printf(__FUNCTION__" cmd=%02X\n", cmd);
   // EXECUTE DEVICE DIAGNOSTIC for both ATA and ATAPI
   if (cmd == 0x90)
   {
       reset_signature(RESET_SOFT);
       return 1;
   }

   if (atapi)
       return 0;

   // INITIALIZE DEVICE PARAMETERS
   if (cmd == 0x91)
   {
     // pos = (reg.cyl * h + (reg.devhead & 0x0F)) * s + reg.sec - 1;
     h = (reg.devhead & 0xF) + 1;
     s = reg.count;
     if (s == 0)
     {
          reg.status = STATUS_DRDY | STATUS_DF | STATUS_DSC | STATUS_ERR;
          return 1;
     }

     c = lba / s / h;

     reg.status = STATUS_DRDY | STATUS_DSC;
     return 1;
   }

   if ((cmd & 0xFE) == 0x20) // ATA-3 (mandatory), read sectors
   { // cmd #21 obsolette, rqd for is-dos
//       printf(__FUNCTION__" sec_cnt=%d\n", reg.count);
       read_sectors();
       return 1;
   }

   if ((cmd & 0xFE) == 0x40) // ATA-3 (mandatory),  verify sectors
   { //rqd for is-dos
       verify_sectors();
       return 1;
   }

   if ((cmd & 0xFE) == 0x30 && !readonly) // ATA-3 (mandatory), write sectors
   {
      if (seek())
      {
          state = S_WRITE_SECTORS;
          reg.status = STATUS_DRQ | STATUS_DSC;
          transptr = 0;
          transcount = 0x100;
      }
      return 1;
   }

   if (cmd == 0x50) // format track (данная реализация - ничего не делает)
   {
      reg.sec = 1;
      if (seek())
      {
          state = S_FORMAT_TRACK;
          reg.status = STATUS_DRQ | STATUS_DSC;
          transptr = 0;
          transcount = 0x100;
      }
      return 1;
   }

   if (cmd == 0xEC)
   {
       prepare_id();
       return 1;
   }

   if (cmd == 0xE7)
   { // FLUSH CACHE
      if (ata_p.flush())
      {
          command_ok();
          intrq = 1;
      }
      else
          reg.status = STATUS_DRDY | STATUS_DF | STATUS_DSC | STATUS_ERR; // 0x71
      return 1;
   }

   if (cmd == 0x10)
   {
      recalibrate();
      command_ok();
      intrq = 1;
      return 1;
   }

   if (cmd == 0x08)		// reset
   {
      recalibrate();
      command_ok();
      intrq = 1;
      return 1;
   }

   if (cmd == 0x70)
   { // seek
      if (!seek())
          return 1;
      command_ok();
      intrq = 1;
      return 1;
   }

   printf("*** unknown ata cmd %02X ***\n", cmd);

   return 0;
}

char ATA_DEVICE::exec_atapi_cmd(u8 cmd)
{
   if (!atapi)
       return 0;

   // soft reset
   if (cmd == 0x08)
   {
       reset(RESET_SOFT);
       return 1;
   }
   if (cmd == 0xA1) // IDENTIFY PACKET DEVICE
   {
       prepare_id();
       return 1;
   }

   if (cmd == 0xA0)
   { // packet
      state = S_RECV_PACKET;
      reg.status = STATUS_DRQ;
      reg.intreason = INT_COD;
      transptr = 0;
      transcount = 6;
      return 1;
   }

   if (cmd == 0xEC)
   {
       reg.count = 1;
       reg.sec = 1;
       reg.cyl = 0xEB14;

       reg.status = STATUS_DSC | STATUS_DRDY | STATUS_ERR;
       reg.err = ERR_ABRT;
       state = S_IDLE;
       intrq = 1;
       return 1;
   }

   printf("*** unknown atapi cmd %02X ***\n", cmd);
   // "command aborted" with ATAPI signature
   reg.count = 1;
   reg.sec = 1;
   reg.cyl = 0xEB14;
   return 0;
}

void ATA_DEVICE::write(unsigned n_reg, u8 data)
{
//   printf("dev=%d, reg=%d, data=%02X\n", device_id, n_reg, data);
   if (!loaded())
       return;
   if (n_reg == 1)
   {
       reg.feat = data;
       return;
   }

   if (n_reg != 7)
   {
      regs[n_reg] = data;
      if (reg.control & CONTROL_SRST)
      {
//          printf("dev=%d, reset\n", device_id);
          reset(RESET_SRST);
      }
      return;
   }

   // execute command!
   if (((reg.devhead ^ device_id) & 0x10) && data != 0x90)
       return;
   if (!(reg.status & STATUS_DRDY) && !atapi)
   {
       printf("warning: hdd not ready cmd = %02X (ignored)\n", data);
       return;
   }

   reg.err = 0; intrq = 0;

//{printf(" [");for (int q=1;q<9;q++) printf("-%02X",regs[q]);printf("]\n");}
   if (exec_atapi_cmd(data))
       return;
   if (exec_ata_cmd(data))
       return;
   reg.status = STATUS_DSC | STATUS_DRDY | STATUS_ERR;
   reg.err = ERR_ABRT;
   state = S_IDLE; intrq = 1;
}

void ATA_DEVICE::write_data(unsigned data)
{
   if (!loaded()) return;
   if ((reg.devhead ^ device_id) & 0x10)
       return;
   if (/* (reg.status & (STATUS_DRQ | STATUS_BSY)) != STATUS_DRQ ||*/ transptr >= transcount)
       return;
   *(u16*)(transbf + transptr*2) = (u16)data; transptr++;
   if (transptr < transcount)
       return;
   // look to state, prepare next block
   if (state == S_WRITE_SECTORS)
   {
       write_sectors();
       return;
   }

   if (state == S_FORMAT_TRACK)
   {
       format_track();
       return;
   }

   if (state == S_RECV_PACKET)
   {
       handle_atapi_packet();
       return;
   }
/*   if (state == S_MODE_SELECT) { exec_mode_select(); return; } */
}

char ATA_DEVICE::seek()
{
   unsigned pos;
   if (reg.devhead & 0x40)
   {
      pos = *(unsigned*)(regs+3) & 0x0FFFFFFF;
      if (pos >= lba)
      {
          seek_err:
//          printf("seek error: lba %d:%d\n", lba, pos);
          reg.status = STATUS_DRDY | STATUS_DF | STATUS_ERR;
          reg.err = ERR_IDNF | ERR_ABRT;
          intrq = 1;
          return 0;
      }
//      printf("lba %d:%d\n", lba, pos);
   }
   else
   {
      if (reg.cyl >= c || (unsigned)(reg.devhead & 0x0F) >= h || reg.sec > s || !reg.sec)
      {
//          printf("seek error: chs %4d/%02d/%02d\n", *(u16*)(regs+4), (reg.devhead & 0x0F), reg.sec);
          goto seek_err;
      }
      pos = (reg.cyl * h + (reg.devhead & 0x0F)) * s + reg.sec - 1;
//      printf("chs %4d/%02d/%02d: %8d\n", *(u16*)(regs+4), (reg.devhead & 0x0F), reg.sec, pos);
   }
//printf("[seek %I64d]", ((__int64)pos) << 9);
   if (!ata_p.seek(pos))
   {
      reg.status = STATUS_DRDY | STATUS_DF | STATUS_ERR;
      reg.err = ERR_IDNF | ERR_ABRT;
      intrq = 1;
      return 0;
   }
   return 1;
}

void ATA_DEVICE::format_track()
{
   intrq = 1;
   if (!seek())
       return;

   command_ok();
   return;
}

void ATA_DEVICE::write_sectors()
{
   intrq = 1;
//printf(" [write] ");
   if (!seek())
       return;

   if (!ata_p.write_sector(transbf))
   {
      reg.status = STATUS_DRDY | STATUS_DSC | STATUS_ERR;
      reg.err = ERR_UNC;
      state = S_IDLE;
      return;
   }

   if (!--reg.count)
   {
       command_ok();
       return;
   }
   next_sector();

   transptr = 0, transcount = 0x100;
   state = S_WRITE_SECTORS;
   reg.err = 0;
   reg.status = STATUS_DRQ | STATUS_DSC;
}

void ATA_DEVICE::read_sectors()
{
//   __debugbreak();
   intrq = 1;
   if (!seek())
      return;

   if (!ata_p.read_sector(transbf))
   {
      reg.status = STATUS_DRDY | STATUS_DSC | STATUS_ERR;
      reg.err = ERR_UNC | ERR_IDNF;
      state = S_IDLE;
      return;
   }
   transptr = 0;
   transcount = 0x100;
   state = S_READ_SECTORS;
   reg.err = 0;
   reg.status = STATUS_DRDY | STATUS_DRQ | STATUS_DSC;

/*
   if (reg.devhead & 0x40)
       printf("dev=%d lba=%d\n", device_id, *(unsigned*)(regs+3) & 0x0FFFFFFF);
   else
       printf("dev=%d c/h/s=%d/%d/%d\n", device_id, reg.cyl, (reg.devhead & 0xF), reg.sec);
*/
}

void ATA_DEVICE::verify_sectors()
{
   intrq = 1;
//   __debugbreak();

   do
   {
       --reg.count;
/*
       if (reg.devhead & 0x40)
           printf("lba=%d\n", *(unsigned*)(regs+3) & 0x0FFFFFFF);
       else
           printf("c/h/s=%d/%d/%d\n", reg.cyl, (reg.devhead & 0xF), reg.sec);
*/
       if (!seek())
           return;
/*
       u8 Buf[512];
       if (!ata_p.read_sector(Buf))
       {
          reg.status = STATUS_DRDY | STATUS_DF | STATUS_CORR | STATUS_DSC | STATUS_ERR;
          reg.err = ERR_UNC | ERR_IDNF | ERR_ABRT | ERR_AMNF;
          state = S_IDLE;
          return;
       }
*/
       if (reg.count)
           next_sector();
   }while (reg.count);
   command_ok();
}

void ATA_DEVICE::next_sector()
{
   if (reg.devhead & 0x40)
   { // LBA
      *(unsigned*)&reg.sec = (*(unsigned*)&reg.sec & 0xF0000000) + ((*(unsigned*)&reg.sec+1) & 0x0FFFFFFF);
      return;
   }
   // need to recalc CHS for every sector, coz ATA registers
   // should contain current position on failure
   if (reg.sec < s)
   {
       reg.sec++;
       return;
   }
   reg.sec = 1;
   u8 head = (reg.devhead & 0x0F) + 1;
   if (head < h)
   {
       reg.devhead = (reg.devhead & 0xF0) | head;
       return;
   }
   reg.devhead &= 0xF0;
   reg.cyl++;
}

void ATA_DEVICE::recalibrate()
{
   reg.cyl = 0;
   reg.devhead &= 0xF0;

   if (reg.devhead & 0x40) // LBA
   {
      reg.sec = 0;
      return;
   }

   reg.sec = 1;
}

#define TOC_DATA_TRACK          0x04

// [vv] Работа с файлом - образом диска напрямую
void ATA_DEVICE::handle_atapi_packet_emulate()
{
//    printf("%s\n", __FUNCTION__);
    memcpy(&atapi_p.cdb, transbf, 12);

    switch(atapi_p.cdb.CDB12.OperationCode)
    {
    case SCSIOP_TEST_UNIT_READY:; // 6
          command_ok();
          return;

    case SCSIOP_READ:; // 10
    {
      unsigned cnt = (u32(atapi_p.cdb.CDB10.TransferBlocksMsb) << 8) | atapi_p.cdb.CDB10.TransferBlocksLsb;
      unsigned pos = (u32(atapi_p.cdb.CDB10.LogicalBlockByte0) << 24) |
                     (u32(atapi_p.cdb.CDB10.LogicalBlockByte1) << 16) |
                     (u32(atapi_p.cdb.CDB10.LogicalBlockByte2) << 8) |
                     atapi_p.cdb.CDB10.LogicalBlockByte3;

      if (cnt * 2048 > sizeof(transbf))
      {
          reg.status = STATUS_DRDY | STATUS_DSC | STATUS_ERR;
          reg.err = ERR_UNC | ERR_IDNF;
          state = S_IDLE;
          return;
      }

      for (unsigned i = 0; i < cnt; i++, pos++)
      {
          if (!atapi_p.seek(pos))
          {
             reg.status = STATUS_DRDY | STATUS_DSC | STATUS_ERR;
             reg.err = ERR_UNC | ERR_IDNF;
             state = S_IDLE;
             return;
          }

          if (!atapi_p.read_sector(transbf + i * 2048))
          {
             reg.status = STATUS_DRDY | STATUS_DSC | STATUS_ERR;
             reg.err = ERR_UNC | ERR_IDNF;
             state = S_IDLE;
             return;
          }
      }
      intrq = 1;
      reg.atapi_count = cnt * 2048;
      reg.intreason = INT_IO;
      reg.status = STATUS_DRQ;
      transcount = (cnt * 2048)/2;
      transptr = 0;
      state = S_READ_ATAPI;
      return;
    }

    case SCSIOP_READ_TOC:; // 10
    {
      u8 TOC_DATA[] =
      {
        0, 4+8*2 - 2, 1, 0xAA,
        0, TOC_DATA_TRACK, 1, 0, 0, 0, 0, 0,
        0, TOC_DATA_TRACK, 0xAA, 0, 0, 0, 0, 0,
      };
      unsigned len = sizeof(TOC_DATA);
      memcpy(transbf, TOC_DATA, len);
      reg.atapi_count = len;
      reg.intreason = INT_IO;
      reg.status = STATUS_DRQ;
      transcount = (len + 1)/2;
      transptr = 0;
      state = S_READ_ATAPI;
      return;
    }
    case SCSIOP_START_STOP_UNIT:; // 10
          command_ok();
          return;

    case SCSIOP_SET_CD_SPEED:; // 12
          command_ok();
          return;
    }

    printf("*** unknown scsi cmd %02X ***\n", atapi_p.cdb.CDB12.OperationCode);

    reg.err = 0;
    state = S_IDLE;
    reg.status = STATUS_DSC | STATUS_ERR | STATUS_DRDY;
}

void ATA_DEVICE::handle_atapi_packet()
{
#if defined(DUMP_HDD_IO)
   {
       printf(" [packet");
       for (int i = 0; i < 12; i++)
           printf("-%02X", transbf[i]);
       printf("]\n");
   }
#endif
   if (phys_dev == -1)
       return handle_atapi_packet_emulate();

   memcpy(&atapi_p.cdb, transbf, 12);

   intrq = 1;

   if (atapi_p.cdb.MODE_SELECT10.OperationCode == 0x55)
   { // MODE SELECT requires additional data from host

      state = S_MODE_SELECT;
      reg.status = STATUS_DRQ;
      reg.intreason = 0;
      transptr = 0;
      transcount = atapi_p.cdb.MODE_SELECT10.ParameterListLength[0]*0x100 + atapi_p.cdb.MODE_SELECT10.ParameterListLength[1];
      return;
   }

   if (atapi_p.cdb.CDB6READWRITE.OperationCode == 0x03 && atapi_p.senselen)
   { // REQ.SENSE - read cached
      memcpy(transbf, atapi_p.sense, atapi_p.senselen);
      atapi_p.passed_length = atapi_p.senselen; atapi_p.senselen = 0; // next time read from device
      goto ok;
   }

   if (atapi_p.pass_through(transbf, sizeof transbf))
   {
      if (atapi_p.senselen)
      {
          reg.err = atapi_p.sense[2] << 4;
          goto err;
      } // err = sense key //win9x hangs on drq after atapi packet when emulator does goto err (see walkaround in SEND_ASPI_CMD)
    ok:
      if (!atapi_p.cdb.CDB6READWRITE.OperationCode)
          atapi_p.passed_length = 0; // bugfix in cdrom driver: TEST UNIT READY has no data
      if (!atapi_p.passed_length /* || atapi_p.passed_length == sizeof transbf */ )
      {
          command_ok();
          return;
      }
      reg.atapi_count = atapi_p.passed_length;
      reg.intreason = INT_IO;
      reg.status = STATUS_DRQ;
      transcount = (atapi_p.passed_length+1)/2;
      transptr = 0;
      state = S_READ_ATAPI;
   }
   else
   { // bus error
      reg.err = 0;
    err:
      state = S_IDLE;
      reg.status = STATUS_DSC | STATUS_ERR | STATUS_DRDY;
   }
}

void ATA_DEVICE::prepare_id()
{
   if (phys_dev == -1)
   {
      memset(transbf, 0, 512);
      make_ata_string(transbf+54, 20, "UNREAL SPECCY HARD DRIVE IMAGE");
      make_ata_string(transbf+20, 10, "0000");
      *(u16*)transbf = 0x045A;
      ((u16*)transbf)[1] = (u16)c;
      ((u16*)transbf)[3] = (u16)h;
      ((u16*)transbf)[6] = (u16)s;
      *(unsigned*)(transbf+60*2) = lba;
      ((u16*)transbf)[20] = 3; // a dual ported multi-sector buffer capable of simultaneous transfers with a read caching capability
      ((u16*)transbf)[21] = 512; // cache size=256k
      ((u16*)transbf)[22] = 4; // ECC bytes
      ((u16*)transbf)[49] = 0x200; // LBA supported
      ((u16*)transbf)[80] = 0x3E; // support specifications up to ATA-5
      ((u16*)transbf)[81] = 0x13; // ATA/ATAPI-5 T13 1321D revision 3
      ((u16*)transbf)[82] = 0x60; // supported look-ahead and write cache

      // make checksum
      transbf[510] = 0xA5;
      u8 cs = 0;
      for (unsigned i = 0; i < 511; i++)
          cs += transbf[i];
      transbf[511] = 0-cs;
   }
   else
   { // copy as is...
      memcpy(transbf, phys[phys_dev].idsector, 512);
   }

   state = S_READ_ID;
   transptr = 0;
   transcount = 0x100;
   intrq = 1;
   reg.status = STATUS_DRDY | STATUS_DRQ | STATUS_DSC;
   reg.err = 0;
}
