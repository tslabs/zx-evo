#pragma once
const int MAX_PHYS_HD_DRIVES = 8;
const int MAX_PHYS_CD_DRIVES = 8;
const int MAX_SENSE_LEN = 0x40;

enum DEVTYPE { ATA_NONE, ATA_FILEHDD, ATA_NTHDD, ATA_SPTI_CD, ATA_ASPI_CD, ATA_FILECD };
enum DEVUSAGE { ATA_OP_ENUM_ONLY, ATA_OP_USE };

struct PHYS_DEVICE
{
   DEVTYPE type;
   DEVUSAGE usage;
   unsigned hdd_size;
   unsigned spti_id;
   unsigned adapterid, targetid; // ASPI
   unsigned char idsector[512];
   char filename[512];
   char viewname[512];
};

struct ATA_PASSER
{
   HANDLE hDevice;
   PHYS_DEVICE *dev;
   HANDLE Vols[100];

   static unsigned identify(PHYS_DEVICE *outlist, int max);
   static unsigned get_hdd_count();

   DWORD open(PHYS_DEVICE *dev);
   void close();
   bool loaded() { return (hDevice != INVALID_HANDLE_VALUE); }
   BOOL flush() { return FlushFileBuffers(hDevice); }
   bool seek(unsigned nsector);
   bool read_sector(unsigned char *dst);
   bool write_sector(unsigned char *src);

   ATA_PASSER() { hDevice = INVALID_HANDLE_VALUE; }
   ~ATA_PASSER() { close(); }
};

struct ATAPI_PASSER
{
   HANDLE hDevice;
   PHYS_DEVICE *dev;

   CDB cdb;
   unsigned passed_length;
   unsigned char sense[MAX_SENSE_LEN]; unsigned senselen;

   static unsigned identify(PHYS_DEVICE *outlist, int max);

   DWORD open(PHYS_DEVICE *dev);
   void close();
//   bool loaded() { return (hDevice != INVALID_HANDLE_VALUE) || (dev->type == ATA_ASPI_CD); } //SMT (crashes if no master or slave device)
//   bool loaded() { return ((hDevice == INVALID_HANDLE_VALUE)?0:(dev->type == ATA_ASPI_CD)); } //Alone Coder (CD doesn't work with ==, !=, 1)
//   bool loaded() { return (hDevice != INVALID_HANDLE_VALUE) || ((dev)?(dev->type == ATA_ASPI_CD):0); } //Alone Coder (CD doesn't work if another device isn't set) (and SPTI won't work?)
//   bool loaded() { return (hDevice != INVALID_HANDLE_VALUE); } //Alone Coder (CD doesn't work)
   bool loaded() { return ((hDevice != INVALID_HANDLE_VALUE) || (dev)); } //Alone Coder (CD doesn't initialize (although works) if another device isn't set)

   int pass_through(void *buf, int bufsize);
   int read_atapi_id(unsigned char *idsector, char prefix);

   int SEND_ASPI_CMD(void *buf, int buf_sz);
   int SEND_SPTI_CMD(void *buf, int buf_sz);

   bool seek(unsigned nsector);
   bool read_sector(unsigned char *dst);

   ATAPI_PASSER() { hDevice = INVALID_HANDLE_VALUE; dev = 0; }
   ~ATAPI_PASSER() { close(); }
};

void make_ata_string(unsigned char *dst, unsigned n_words, const char *src);
void swap_bytes(char *dst, BYTE *src, unsigned n_words);
void print_device_name(char *dst, PHYS_DEVICE *dev);
void init_aspi();
void done_aspi();

// #define DUMP_HDD_IO
