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
  CMD_R1b             = 0x80,
  CMD_R7              = 0x40,
  
  GO_IDLE_STATE       = (0),
  SEND_OP_COND_MMC    = (1),
  SEND_IF_COND        = (8 | CMD_R7),
  STOP_TRANSMISSION   = (12 | CMD_R1b),
  SEND_STATUS         = (13 | CMD_R7),
  SET_BLOCKLEN        = (16),
  READ_SINGLE_BLOCK   = (17),
  READ_MULTIPLE_BLOCK = (18),
  WRITE_BLOCK         = (24),
  SEND_OP_COND_SD     = (41),
  LOCK_UNLOCK         = (42),
  APP_CMD             = (55),
  READ_OCR            = (58 | CMD_R7),
};

#define ZC_SD_CFG   0x77
#define ZC_SD_DAT   0x57

// S/W defines
// -------------------------------------------

// Card type flags
enum
{
  CT_NONE  = 0x00,
  CT_MMC   = 0x01,               // MMC ver 3
  CT_SD1   = 0x02,               // SD ver 1
  CT_SD2   = 0x04,               // SD ver 2
  CT_SDC   = (CT_SD1 | CT_SD2),  // SD
  CT_BLOCK = 0x08                // Block addressing
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

enum
{
  STA_INIT         = 0x00, // Drive initialized
  STA_NOINIT_IDLE  = 0x10, // Drive not initialized - GO_IDLE_STATE error
  STA_NOINIT_VOLT  = 0x11, // Drive not initialized - non compatible voltage range
  STA_NOINIT_OPSD  = 0x12, // Drive not initialized - SEND_OP_COND_SD error
  STA_NOINIT_OCR   = 0x13, // Drive not initialized - READ_OCR error
  STA_NOINIT_LEAVE = 0x14, // Drive not initialized - error leaving IDLE state
};

#define abort(a) { rc = (a); goto exit; }

#ifdef __cplusplus
}
#endif
#endif  /* _DISKIO_DEFINED */
