#pragma once
struct ATA_DEVICE
{
   unsigned c,h,s,lba;
   union
   {
      u8 regs[12];
      struct
      {
         u8 data;
         u8 err;             // for write, features
         union
         {
            u8 count;
            u8 intreason;
         };
         u8 sec;
         union
         {
            u16 cyl;
            u16 atapi_count;
            struct
            {
               u8 cyl_l;
               u8 cyl_h;
            };
         };
         u8 devhead;
         u8 status;          // for write, cmd
         /*                  */
         u8 control;         // reg8 - control (CS1,DA=6)
         u8 feat;
         u8 cmd;
         u8 reserved;        // reserved
      } reg;
   };
   u8 intrq;
   u8 readonly;
   u8 device_id;             // 0x00 - master, 0x10 - slave
   u8 atapi;                 // flag for CD-ROM device

   u8 read(unsigned n_reg);
   void write(unsigned n_reg, u8 data);
   unsigned read_data();
   void write_data(unsigned data);
   u8 read_intrq();

   char exec_ata_cmd(u8 cmd);
   char exec_atapi_cmd(u8 cmd);

   enum RESET_TYPE { RESET_HARD, RESET_SOFT, RESET_SRST };
   void reset_signature(RESET_TYPE mode = RESET_SOFT);

   void reset(RESET_TYPE mode);
   char seek();
   void recalibrate();
   void configure(IDE_CONFIG *cfg);
   void prepare_id();
   void command_ok();
   void next_sector();
   void read_sectors();
   void verify_sectors();
   void write_sectors();
   void format_track();

   enum ATAPI_INT_REASON
   {
      INT_COD       = 0x01,
      INT_IO        = 0x02,
      INT_RELEASE   = 0x04
   };

   enum HD_STATUS
   {
      STATUS_BSY   = 0x80,
      STATUS_DRDY  = 0x40,
      STATUS_DF    = 0x20,
      STATUS_DSC   = 0x10,
      STATUS_DRQ   = 0x08,
      STATUS_CORR  = 0x04,
      STATUS_IDX   = 0x02,
      STATUS_ERR   = 0x01
   };

   enum HD_ERROR
   {
      ERR_BBK   = 0x80,
      ERR_UNC   = 0x40,
      ERR_MC    = 0x20,
      ERR_IDNF  = 0x10,
      ERR_MCR   = 0x08,
      ERR_ABRT  = 0x04,
      ERR_TK0NF = 0x02,
      ERR_AMNF  = 0x01
   };

   enum HD_CONTROL
   {
      CONTROL_SRST = 0x04,
      CONTROL_nIEN = 0x02
   };

   enum HD_STATE
   {
      S_IDLE = 0, S_READ_ID,
      S_READ_SECTORS, S_VERIFY_SECTORS, S_WRITE_SECTORS, S_FORMAT_TRACK,
      S_RECV_PACKET, S_READ_ATAPI,
      S_MODE_SELECT
   };

   HD_STATE state;
   unsigned transptr, transcount;
   unsigned phys_dev;
   u8 transbf[0xFFFF]; // ATAPI is able to tranfer 0xFFFF bytes. passing more leads to error

   void handle_atapi_packet();
   void handle_atapi_packet_emulate();
   void exec_mode_select();

   ATA_PASSER ata_p;
   ATAPI_PASSER atapi_p;
   bool loaded() { return ata_p.loaded() || atapi_p.loaded(); } //was crashed at atapi_p.loaded() if no master or slave device!!! see fix in ATAPI_PASSER //Alone Coder
};

struct ATA_PORT
{
   ATA_DEVICE dev[2];
   u8 read_high, write_high;

   ATA_PORT() { dev[0].device_id = 0, dev[1].device_id = 0x10; reset(); }

   u8 read(unsigned n_reg);
   void write(unsigned n_reg, u8 data);
   unsigned read_data();
   void write_data(unsigned data);
   u8 read_intrq();

   void reset();
};

extern PHYS_DEVICE phys[];
extern int n_phys;

unsigned find_hdd_device(char *name);
void init_hdd_cd();
