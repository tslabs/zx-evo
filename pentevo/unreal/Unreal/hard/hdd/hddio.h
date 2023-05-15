#pragma once
#include "std.h"
#include "sysdefs.h"

constexpr int max_phys_hd_drives = 8;
constexpr int max_phys_cd_drives = 8;
constexpr int max_sense_len = 0x40;

enum class ata_devtype_t { none, filehdd, nthdd, spti_cd, aspi_cd, filecd };
enum ata_devusage_t { enum_only, use };

struct phys_device_t
{
   ata_devtype_t type;
   ata_devusage_t usage;
   unsigned hdd_size;
   unsigned spti_id;
   unsigned adapterid, targetid; // ASPI
   u8 idsector[512];
   char filename[512];
   char viewname[512];
};

struct ata_passer_t
{
   HANDLE h_device;
   phys_device_t *dev;
   HANDLE vols[100];

   static unsigned identify(phys_device_t *outlist, unsigned max);
   static unsigned get_hdd_count();

   DWORD open(phys_device_t *phy_dev);
   void close();
   bool loaded() const { return (h_device != INVALID_HANDLE_VALUE); }
   BOOL flush() const { return FlushFileBuffers(h_device); }
   bool seek(unsigned nsector) const;
   bool read_sector(u8 *dst) const;
   bool write_sector(const u8 *src) const;

   ata_passer_t(): dev(nullptr), vols{} { h_device = INVALID_HANDLE_VALUE; }
   ~ata_passer_t() { close(); }
};

struct atapi_passer_t
{
   HANDLE h_device;
   phys_device_t *dev;

   CDB cdb;
   unsigned passed_length;
   u8 sense[max_sense_len]; unsigned senselen;

   static unsigned identify(phys_device_t *outlist, int max);

   DWORD open(phys_device_t *phy_dev);
   void close();
   bool loaded() const { return ((h_device != INVALID_HANDLE_VALUE) || (dev)); } //Alone Coder (CD doesn't initialize (although works) if another device isn't set)

   int pass_through(void *buf, int buf_sz);
   int read_atapi_id(u8 *idsector, char prefix);

   int send_aspi_cmd(void *buf, int buf_sz);
   int send_spti_cmd(void *buf, int buf_sz);

   bool seek(unsigned nsector) const;
   bool read_sector(u8 *dst) const;

   atapi_passer_t(): cdb(), passed_length(0), sense{}, senselen(0)
   {
	   h_device = INVALID_HANDLE_VALUE;
	   dev = nullptr;
   }

   ~atapi_passer_t() { close(); }
};

void make_ata_string(u8 *dst, unsigned n_words, const char *src);
void swap_bytes(char *dst, const BYTE *src, unsigned n_words);
void print_device_name(char *dst, const phys_device_t *dev);
void init_aspi();
void done_aspi();

// #define DUMP_HDD_IO
