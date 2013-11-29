#include "std.h"
#include "emul.h"
#include "vars.h"
#include "init.h"
#include "hddio.h"
#include "util.h"

typedef int (__cdecl *GetASPI32SupportInfo_t)();
typedef int (__cdecl *SendASPI32Command_t)(void *SRB);
const int ATAPI_CDB_SIZE = 12; // sizeof(CDB) == 16
const int MAX_INFO_LEN = 48;

GetASPI32SupportInfo_t _GetASPI32SupportInfo = 0;
SendASPI32Command_t _SendASPI32Command = 0;
HMODULE hAspiDll = 0;
HANDLE hASPICompletionEvent;


DWORD ATA_PASSER::open(PHYS_DEVICE *dev)
{
   close();
   this->dev = dev;

   hDevice = CreateFile(dev->filename,
                GENERIC_READ | GENERIC_WRITE, // R/W required!
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                0, OPEN_EXISTING, 0, 0);

   if (hDevice == INVALID_HANDLE_VALUE)
   {
       ULONG Le = GetLastError();
       printf("can't open: `%s', %u\n", dev->filename, Le);
       return Le;
   }

   if (dev->type == ATA_NTHDD && dev->usage == ATA_OP_USE)
   {
       memset(Vols, 0, sizeof(Vols));

       // lock & dismount all volumes on disk
       char VolName[256];
       HANDLE VolEnum = FindFirstVolume(VolName, _countof(VolName));
       if (VolEnum == INVALID_HANDLE_VALUE)
       {
           ULONG Le = GetLastError();
           printf("can't enumerate volumes: %u\n", Le);
           return Le;
       }

       BOOL NextVol = TRUE;
       unsigned VolIdx = 0;
       unsigned i;
       for (; NextVol; NextVol = FindNextVolume(VolEnum, VolName, _countof(VolName)))
       {
           int l = strlen(VolName);
           if (VolName[l-1] == '\\')
               VolName[l-1] = 0;

           HANDLE Vol = CreateFile(VolName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
           if (Vol == INVALID_HANDLE_VALUE)
           {
               printf("can't open volume `%s'\n", VolName);
               continue;
           }

           UCHAR Buf[sizeof(VOLUME_DISK_EXTENTS) + 100 * sizeof(DISK_EXTENT)];
           PVOLUME_DISK_EXTENTS DiskExt = PVOLUME_DISK_EXTENTS(Buf);

           ULONG Junk;
           if (!DeviceIoControl(Vol, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 0, 0, Buf, sizeof(Buf), &Junk, 0))
           {
               Junk = GetLastError();
               CloseHandle(Vol);
               printf("can't get volume extents: `%s', %u\n", VolName, Junk);
               continue;
           }

           if (DiskExt->NumberOfDiskExtents == 0)
           {
               // bad volume
               CloseHandle(Vol);
               return ERROR_ACCESS_DENIED;
           }

           if (DiskExt->NumberOfDiskExtents > 1)
           {
               for (i = 0; i < DiskExt->NumberOfDiskExtents; i++)
               {
                   if (DiskExt->Extents[i].DiskNumber == dev->spti_id)
                   {
                       // complex volume (volume split over several disks)
                       CloseHandle(Vol);
                       return ERROR_ACCESS_DENIED;
                   }
               }
           }

           if (DiskExt->Extents[0].DiskNumber != dev->spti_id)
           {
               CloseHandle(Vol);
               continue;
           }

           Vols[VolIdx++] = Vol;
       }
       FindVolumeClose(VolEnum);

       for (i = 0; i < VolIdx; i++)
       {
           ULONG Junk;
           if (!DeviceIoControl(Vols[i], FSCTL_LOCK_VOLUME, 0, 0, 0, 0, &Junk, 0))
           {
               Junk = GetLastError();
               printf("can't lock volume: %u\n", Junk);
               return Junk;
           }
           if (!DeviceIoControl(Vols[i], FSCTL_DISMOUNT_VOLUME, 0, 0, 0, 0, &Junk, 0))
           {
               Junk = GetLastError();
               printf("can't dismount volume: %u\n", Junk);
               return Junk;
           }
       }
   }
   return NO_ERROR;
}

void ATA_PASSER::close()
{
   if (hDevice != INVALID_HANDLE_VALUE)
   {
       if (dev->type == ATA_NTHDD && dev->usage == ATA_OP_USE)
       {
           // unlock all volumes on disk
           for (unsigned i = 0; i < _countof(Vols) && Vols[i]; i++)
           {
               ULONG Junk;
               DeviceIoControl(Vols[i], FSCTL_UNLOCK_VOLUME, 0, 0, 0, 0, &Junk, 0);
               CloseHandle(Vols[i]);
               Vols[i] = 0;
           }
       }

       CloseHandle(hDevice);
   }
   hDevice = INVALID_HANDLE_VALUE;
   dev = 0;
}

unsigned ATA_PASSER::identify(PHYS_DEVICE *outlist, int max)
{
   int res = 0;
   ATA_PASSER ata;

   unsigned HddCount = get_hdd_count();

   for (unsigned drive = 0; drive < MAX_PHYS_HD_DRIVES && res < max; drive++)
   {

      PHYS_DEVICE *dev = outlist + res;
      dev->type = ATA_NTHDD;
      dev->spti_id = drive;
      dev->usage = ATA_OP_ENUM_ONLY;
      sprintf(dev->filename, "\\\\.\\PhysicalDrive%d", dev->spti_id);

      if (drive >= HddCount)
          continue;

      DWORD errcode = ata.open(dev);
      if (errcode == ERROR_FILE_NOT_FOUND)
          continue;

      color(CONSCLR_HARDITEM);
      printf("hd%d: ", drive);

      if (errcode != NO_ERROR)
      {
          color(CONSCLR_ERROR);
          printf("access failed\n");
          err_win32(errcode);
          continue;
      }

      SENDCMDINPARAMS in = { 512 };
      in.irDriveRegs.bCommandReg = ID_CMD;
      struct
      {
          SENDCMDOUTPARAMS out;
          char xx[512];
      } res_buffer;
      res_buffer.out.cBufferSize = 512;
      DWORD sz;

      DISK_GEOMETRY geo = { 0 };
      int res1 = DeviceIoControl(ata.hDevice, SMART_RCV_DRIVE_DATA, &in, sizeof in, &res_buffer, sizeof res_buffer, &sz, 0);
      if (!res1)
      {
          printf("cant get hdd info, %u\n", GetLastError());
      }
      int res2 = DeviceIoControl(ata.hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, 0, 0, &geo, sizeof geo, &sz, 0);
      if (geo.BytesPerSector != 512)
      {
          color(CONSCLR_ERROR);
          printf("unsupported sector size (%d bytes)\n", geo.BytesPerSector);
          continue;
      }

      ata.close();

      if (!res1)
      {
          color(CONSCLR_ERROR);
          printf("identification failed\n");
          continue;
      }

      memcpy(dev->idsector, res_buffer.out.bBuffer, 512);
      char model[42], serial[22];
      swap_bytes(model, res_buffer.out.bBuffer+54, 20);
      swap_bytes(serial, res_buffer.out.bBuffer+20, 10);

      dev->hdd_size = geo.Cylinders.LowPart * geo.SectorsPerTrack * geo.TracksPerCylinder;
      unsigned shortsize = dev->hdd_size / 2; char mult = 'K';
      if (shortsize >= 100000)
      {
         shortsize /= 1024, mult = 'M';
         if (shortsize >= 100000)
             shortsize /= 1024, mult = 'G';
      }

      color(CONSCLR_HARDINFO);
      printf("%-40s %-20s ", model, serial);
      color(CONSCLR_HARDITEM);
      printf("%8d %cb\n", shortsize, mult);
      if (dev->hdd_size > 0xFFFFFFF)
      {
          color(CONSCLR_WARNING);
          printf("     drive %d warning! LBA48 is not supported. only first 128GB visible\n", drive); //Alone Coder 0.36.7
      }

      print_device_name(dev->viewname, dev);
      res++;
   }

   return res;
}

unsigned ATA_PASSER::get_hdd_count() // static
{
    HDEVINFO DeviceInfoSet;
    ULONG MemberIndex;

    // create a HDEVINFO with all present devices
    DeviceInfoSet = SetupDiGetClassDevs(&GUID_DEVINTERFACE_DISK, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        assert(FALSE);
        return 0;
    }

    // enumerate through all devices in the set
    MemberIndex = 0;
    while (true)
    {
        // get device interfaces
        SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
        DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(DeviceInfoSet, 0, &GUID_DEVINTERFACE_DISK, MemberIndex, &DeviceInterfaceData))
        {
            if (GetLastError() != ERROR_NO_MORE_ITEMS)
            {
                // error
                assert(FALSE);
            }

            // ok, reached end of the device enumeration
            break;
        }

        // process the next device next time
        MemberIndex++;
    }

    // destroy device info list
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    return MemberIndex;
}


bool ATA_PASSER::seek(unsigned nsector)
{
   LARGE_INTEGER offset;
   offset.QuadPart = ((__int64)nsector) << 9;
   DWORD code = SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
   return (code != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
}

bool ATA_PASSER::read_sector(u8 *dst)
{
   DWORD sz = 0;
   if (!ReadFile(hDevice, dst, 512, &sz, 0))
       return false;
   if (sz < 512)
       memset(dst+sz, 0, 512-sz); // on EOF, or truncated file, read 00
   return true;
}

bool ATA_PASSER::write_sector(u8 *src)
{
   DWORD sz = 0;
   return (WriteFile(hDevice, src, 512, &sz, 0) && sz == 512);
}

DWORD ATAPI_PASSER::open(PHYS_DEVICE *dev)
{
   close();
   this->dev = dev;
   if (dev->type == ATA_ASPI_CD)
       return NO_ERROR;

   hDevice = CreateFile(dev->filename,
                GENERIC_READ | GENERIC_WRITE, // R/W required!
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                0, OPEN_EXISTING, 0, 0);

   if (hDevice != INVALID_HANDLE_VALUE)
       return NO_ERROR;
   return GetLastError();
}

void ATAPI_PASSER::close()
{
   if (!dev || dev->type == ATA_ASPI_CD)
       return;
   if (hDevice != INVALID_HANDLE_VALUE)
       CloseHandle(hDevice);
   hDevice = INVALID_HANDLE_VALUE;
   dev = 0;
}

unsigned ATAPI_PASSER::identify(PHYS_DEVICE *outlist, int max)
{
   int res = 0;
   ATAPI_PASSER atapi;

   if (conf.cd_aspi)
   {

      init_aspi();


      for (int adapterid = 0; ; adapterid++)
      {

         SRB_HAInquiry SRB = { 0 };
         SRB.SRB_Cmd        = SC_HA_INQUIRY;
         SRB.SRB_HaId       = (u8)adapterid;
         DWORD ASPIStatus = _SendASPI32Command(&SRB);

         if (ASPIStatus != SS_COMP) break;

         char b1[20], b2[20];
         memcpy(b1, SRB.HA_ManagerId, 16); b1[16] = 0;
         memcpy(b2, SRB.HA_Identifier, 16); b2[16] = 0;

         if (adapterid == 0) {
            color(CONSCLR_HARDITEM); printf("using ");
            color(CONSCLR_WARNING); printf("%s", b1);
            color(CONSCLR_HARDITEM); printf(" %s\n", b2);
         }
         if (adapterid >= (int)SRB.HA_Count) break;
         // int maxTargets = (int)SRB.HA_Unique[3]; // always 8 (?)

         for (int targetid = 0; targetid < 8; targetid++) {

            PHYS_DEVICE *dev = outlist + res;
            dev->type = ATA_ASPI_CD;
            dev->adapterid = adapterid; // (int)SRB.HA_SCSI_ID; // does not work with Nero ASPI
            dev->targetid = targetid;

            DWORD errcode = atapi.open(dev);
            if (errcode != NO_ERROR) continue;

            int ok = atapi.read_atapi_id(dev->idsector, 1);
            atapi.close();
            if (ok != 2) continue; // not a CD-ROM

            print_device_name(dev->viewname, dev);
            res++;
         }
      }


       return res;
   }
   
   // spti
   for (int drive = 0; drive < MAX_PHYS_CD_DRIVES && res < max; drive++)
   {

      PHYS_DEVICE *dev = outlist + res;
      dev->type = ATA_SPTI_CD;
      dev->spti_id = drive;
      dev->usage = ATA_OP_ENUM_ONLY;
      sprintf(dev->filename, "\\\\.\\CdRom%d", dev->spti_id);

      DWORD errcode = atapi.open(dev);
      if (errcode == ERROR_FILE_NOT_FOUND)
          continue;

      color(CONSCLR_HARDITEM);
      printf("cd%d: ", drive);
      if (errcode != NO_ERROR)
      {
          color(CONSCLR_ERROR);
          printf("access failed\n");
          err_win32(errcode);
          continue;
      }


      int ok = atapi.read_atapi_id(dev->idsector, 0);
      atapi.close();
      if (!ok)
      {
          color(CONSCLR_ERROR);
          printf("identification failed\n");
          continue;
      }
      if (ok < 2)
          continue; // not a CD-ROM

      print_device_name(dev->viewname, dev);
      res++;
   }

   return res;
}

int ATAPI_PASSER::pass_through(void *databuf, int bufsize)
{
   int res = (conf.cd_aspi)? SEND_ASPI_CMD(databuf, bufsize) : SEND_SPTI_CMD(databuf, bufsize);
   return res;
}

int ATAPI_PASSER::SEND_SPTI_CMD(void *databuf, int bufsize)
{
   memset(databuf, 0, bufsize);

   struct {
      SCSI_PASS_THROUGH_DIRECT p;
      u8 sense[MAX_SENSE_LEN];
   } srb = { 0 }, dst;

   srb.p.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
   *(CDB*)&srb.p.Cdb = cdb;
   srb.p.CdbLength = sizeof(CDB);
   srb.p.DataIn = SCSI_IOCTL_DATA_IN;
   srb.p.TimeOutValue = 10;
   srb.p.DataBuffer = databuf;
   srb.p.DataTransferLength = bufsize;
   srb.p.SenseInfoLength = sizeof(srb.sense);
   srb.p.SenseInfoOffset = sizeof(SCSI_PASS_THROUGH_DIRECT);

   DWORD outsize;
   int r = DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT,
                           &srb.p, sizeof(srb.p),
                           &dst, sizeof(dst),
                           &outsize, 0);

   if (!r) { return 0; }

   passed_length = dst.p.DataTransferLength;
   if (senselen = dst.p.SenseInfoLength) memcpy(sense, dst.sense, senselen);

#ifdef DUMP_HDD_IO
printf("sense=%d, data=%d, srbsz=%d/%d, dir=%d. ok%d\n", senselen, passed_length, outsize, sizeof(srb.p), dst.p.DataIn, res);
printf("srb:"); dump1((BYTE*)&dst, outsize);
printf("data:"); dump1((BYTE*)databuf, 0x40);
#endif

   return 1;
}

int ATAPI_PASSER::read_atapi_id(u8 *idsector, char prefix)
{
   memset(&cdb, 0, sizeof(CDB));
   memset(idsector, 0, 512);

   INQUIRYDATA inq;
   cdb.CDB6INQUIRY.OperationCode = 0x12; // INQUIRY
   cdb.CDB6INQUIRY.AllocationLength = sizeof(inq);
   if (!pass_through(&inq, sizeof(inq))) return 0;

   char vendor[10], product[18], revision[6], id[22], ata_name[26];

   memcpy(vendor, inq.VendorId, sizeof(inq.VendorId)); vendor[sizeof(inq.VendorId)] = 0;
   memcpy(product, inq.ProductId, sizeof(inq.ProductId)); product[sizeof(inq.ProductId)] = 0;
   memcpy(revision, inq.ProductRevisionLevel, sizeof(inq.ProductRevisionLevel)); revision[sizeof(inq.ProductRevisionLevel)] = 0;
   memcpy(id, inq.VendorSpecific, sizeof(inq.VendorSpecific)); id[sizeof(inq.VendorSpecific)] = 0;

   if (prefix) {
      color(CONSCLR_HARDITEM);
      if (dev->type == ATA_ASPI_CD) printf("%d.%d: ", dev->adapterid, dev->targetid);
      if (dev->type == ATA_SPTI_CD) printf("cd%d: ", dev->spti_id);
   }

   trim(vendor); trim(product); trim(revision); trim(id);
   sprintf(ata_name, "%s %s", vendor, product);

   idsector[0] = 0xC0; // removable, accelerated DRQ, 12-byte packet
   idsector[1] = 0x85; // protocol: ATAPI, device type: CD-ROM

   make_ata_string(idsector+54, 20, ata_name);
   make_ata_string(idsector+20, 10, id);
   make_ata_string(idsector+46,  4, revision);

   idsector[0x63] = 0x0B; // caps: IORDY,LBA,DMA
   idsector[0x67] = 4; // PIO timing
   idsector[0x69] = 2; // DMA timing

   if (inq.DeviceType == 5) color(CONSCLR_HARDINFO);
   printf("%-40s %-20s  ", ata_name, id);
   color(CONSCLR_HARDITEM);
   printf("rev.%-4s\n", revision);

   return 1 + (inq.DeviceType == 5);
}

bool ATAPI_PASSER::read_sector(u8 *dst)
{
   DWORD sz = 0;
   if (!ReadFile(hDevice, dst, 2048, &sz, 0))
       return false;
   if (sz < 2048)
       memset(dst+sz, 0, 2048-sz); // on EOF, or truncated file, read 00
   return true;
}

bool ATAPI_PASSER::seek(unsigned nsector)
{
   LARGE_INTEGER offset;
   offset.QuadPart = i64(nsector) * 2048;
   DWORD code = SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
   return (code != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
}

void make_ata_string(u8 *dst, unsigned n_words, const char *src)
{
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < n_words*2 && src[i]; i++) dst[i] = src[i];
   while (i < n_words*2) dst[i++] = ' ';
   u8 tmp;
   for (i = 0; i < n_words*2; i += 2)
      tmp = dst[i], dst[i] = dst[i+1], dst[i+1] = tmp;
}

void swap_bytes(char *dst, BYTE *src, unsigned n_words)
{
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < n_words; i++)
   {
      char c1 = src[2*i], c2 = src[2*i+1];
      dst[2*i] = c2, dst[2*i+1] = c1;
   }
   dst[2*i] = 0;
   trim(dst);
}

void print_device_name(char *dst, PHYS_DEVICE *dev)
{
   char model[42], serial[22];
   swap_bytes(model, dev->idsector + 54, 20);
   swap_bytes(serial, dev->idsector + 20, 10);
   sprintf(dst, "<%s,%s>", model, serial);
}

void init_aspi()
{
   if (_SendASPI32Command)
       return;
   hAspiDll = LoadLibrary("WNASPI32.DLL");
   if (!hAspiDll)
   {
       errmsg("failed to load WNASPI32.DLL");
       err_win32();
       exit();
   }
   _GetASPI32SupportInfo = (GetASPI32SupportInfo_t)GetProcAddress(hAspiDll, "GetASPI32SupportInfo");
   _SendASPI32Command = (SendASPI32Command_t)GetProcAddress(hAspiDll, "SendASPI32Command");
   if (!_GetASPI32SupportInfo || !_SendASPI32Command) errexit("invalid ASPI32 library");
   DWORD init = _GetASPI32SupportInfo();
   if (((init >> 8) & 0xFF) != SS_COMP)
       errexit("error in ASPI32 initialization");
   hASPICompletionEvent = CreateEvent(0,0,0,0);
}

int ATAPI_PASSER::SEND_ASPI_CMD(void *buf, int buf_sz)
{
   SRB_ExecSCSICmd SRB = { 0 };
   SRB.SRB_Cmd        = SC_EXEC_SCSI_CMD;
   SRB.SRB_HaId       = (u8)dev->adapterid;
   SRB.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY | SRB_ENABLE_RESIDUAL_COUNT;
   SRB.SRB_Target     = (u8)dev->targetid;
   SRB.SRB_BufPointer = (u8*)buf;
   SRB.SRB_BufLen     = buf_sz;
   SRB.SRB_SenseLen   = sizeof(SRB.SenseArea);
   SRB.SRB_CDBLen     = ATAPI_CDB_SIZE;
   SRB.SRB_PostProc   = hASPICompletionEvent;
   memcpy(SRB.CDBByte, &cdb, ATAPI_CDB_SIZE);

   /* DWORD ASPIStatus = */ _SendASPI32Command(&SRB);
   passed_length = SRB.SRB_BufLen;

   if (SRB.SRB_Status == SS_PENDING)
   {
      DWORD ASPIEventStatus = WaitForSingleObject(hASPICompletionEvent, 10000); // timeout 10sec
      if (ASPIEventStatus == WAIT_OBJECT_0)
          ResetEvent(hASPICompletionEvent);
   }
   if (senselen = SRB.SRB_SenseLen)
       memcpy(sense, SRB.SenseArea, senselen);
   if (passed_length >= 0xffff)
       passed_length = 2048; //Alone Coder //was >=65535 in win9x //makes possible to work in win9x (HDDoct, WDC, Time Gal) //XP fails too

#ifdef DUMP_HDD_IO
printf("sense=%d, data=%d/%d, ok%d\n", senselen, passed_length, buf_sz, SRB.SRB_Status);
printf("srb:"); dump1((BYTE*)&SRB, sizeof(SRB));
printf("data:"); dump1((BYTE*)buf, 0x40);
#endif

   return (SRB.SRB_Status == SS_COMP);
}

void done_aspi()
{
   if (!hAspiDll)
       return;
   FreeLibrary(hAspiDll);
   CloseHandle(hASPICompletionEvent);
}
