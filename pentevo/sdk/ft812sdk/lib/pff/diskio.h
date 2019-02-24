/*-----------------------------------------------------------------------
/  PFF - Low level disk interface modlue include file    (C)ChaN, 2014
/-----------------------------------------------------------------------*/

#ifndef _DISKIO_DEFINED
#define _DISKIO_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#include "integer.h"
#include "string.h"

/* Status of Disk Functions */
typedef BYTE  DSTATUS;


/* Results of Disk Functions */
typedef enum {
  RES_OK = 0,    /* 0: Function succeeded */
  RES_ERROR,    /* 1: Disk error */
  RES_NOTRDY,    /* 2: Not ready */
  RES_PARERR    /* 3: Invalid parameter */
} DRESULT;

// H/W defines
// -------------------------------------------

// Definitions for MMC/SDC command
enum
{
  // Command modifiers
  SDC_X1           = 0x40,    // extra 1 byte
  SDC_X4           = 0x80,    // extra 4 bytes
  SDC_R1B          = 0xC0,

  // CMD
  SDC_GO_IDLE_STATE         = 0,
  SDC_SEND_OP_COND_MMC      = 1,
  SDC_SWITCH_FUNC           = 6,            // R1 + data (64 bytes)
  SDC_SEND_IF_COND          = 8  | SDC_X4,  // R7 + 4 extra bytes
  SDC_SEND_CSD              = 9,            // R1 + data (16 bytes)
  SDC_SEND_CID              = 10,           // R1 + data (16 bytes)
  SDC_STOP_TRANSMISSION     = 12 | SDC_R1B,
  SDC_SEND_STATUS           = 13 | SDC_X1,  // R2 + 1 extra byte
  SDC_SET_BLOCKLEN          = 16,
  SDC_READ_SINGLE_BLOCK     = 17,
  SDC_READ_MULTIPLE_BLOCK   = 18,
  SDC_WRITE_BLOCK           = 24,
  SDC_WRITE_MULTIPLE_BLOCK  = 25,
  SDC_PROGRAM_CSD           = 27,
  SDC_SEND_WRITE_PROT       = 30,
  SDC_SEND_OP_COND_SD       = 41,
  SDC_LOCK_UNLOCK           = 42,
  SDC_APP_CMD               = 55,
  SDC_READ_OCR              = 58 | SDC_X4,  // R3 + 4 extra bytes

  // ACMD
  SDC_SD_STATUS             = 13 | SDC_X1,  // R2 + 1 extra byte
  SDC_SD_SEND_OP_COND       = 41,
  SDC_SEND_SCR              = 51,           // R1 + data (8 bytes)
  
  // Lock/unlock operations
  SDC_UNLOCK      = 0,
  SDC_SET_PWD     = 1,
  SDC_CLR_PWD     = 2,
  SDC_LOCK        = 4,
  SDC_FORCE_ERASE = 8
};

#define ZC_SD_CFG   0x77
#define ZC_SD_DAT   0x57

// S/W defines
// -------------------------------------------

// Card type flags
enum
{
  CT_NONE  = 0x00,  // No card
  CT_MMC   = 0x01,  // MMC ver 3
  CT_SD1   = 0x02,  // SD ver 1
  CT_SD2   = 0x03,  // SD ver 2
  CT_SDHC  = 0x04,  // SDHC
  CT_BLOCK = 0x10   // Block addressing
};

// Vars
// -------------------------------------------

extern u8 ctype;
extern u8 sd_rbuf[4];
extern u8 sdcrc;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize (void);
DRESULT disk_readp (BYTE* buff, DWORD sector, UINT offser, UINT count);
DRESULT disk_writep (const BYTE* buff, DWORD sc);

void sd_cs_on();
void sd_cs_off();
u8 sd_rd();
void sd_wr(u8);
void sd_skip(u16);
u8 sd_cmd(u8, u32);
u8 sd_acmd(u8, u32);
void sd_recv(void*, u16);
void sd_send(void*, u16);
bool sd_wait_busy();
bool sd_wait_busy_long();
bool sd_wait_dtoken();
bool sd_recv_data(u8*, u16);

enum
{
  STA_INIT         = 0x00, // Drive initialized
  STA_NOINIT_IDLE  = 0x01, // Drive not initialized - GO_IDLE_STATE error
  STA_NOINIT_VOLT  = 0x02, // Drive not initialized - non compatible voltage range
  STA_NOINIT_OPSD  = 0x03, // Drive not initialized - SEND_OP_COND_SD error
  STA_NOINIT_OCR   = 0x04, // Drive not initialized - READ_OCR error
  STA_NOINIT_LEAVE = 0x05, // Drive not initialized - error leaving IDLE state
};

#define abort(a) { rc = (a); goto exit; }

#ifdef __cplusplus
}
#endif
#endif  /* _DISKIO_DEFINED */
