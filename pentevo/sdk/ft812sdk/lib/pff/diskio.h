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
typedef BYTE	DSTATUS;


/* Results of Disk Functions */
typedef enum {
	RES_OK = 0,		/* 0: Function succeeded */
	RES_ERROR,		/* 1: Disk error */
	RES_NOTRDY,		/* 2: Not ready */
	RES_PARERR		/* 3: Invalid parameter */
} DRESULT;

// H/W defines
// -------------------------------------------

// Definitions for MMC/SDC command
#define APP_CMD             (0x40+55)
#define GO_IDLE_STATE       (0x40+0)
#define READ_OCR            (0x40+58)
#define SEND_OP_COND_MMC    (0x40+1)
#define SEND_OP_COND_SD     (0xC0+41)
#define SEND_IF_COND        (0x40+8)
#define SET_BLOCKLEN        (0x40+16)
#define READ_SINGLE_BLOCK   (0x40+17)
#define WRITE_BLOCK         (0x40+24)

#define ZC_SD_CFG   0x77
#define ZC_SD_DAT   0x57

// S/W defines
// -------------------------------------------

// Card type flags
#define CT_NONE             0x00
#define CT_MMC              0x01                // MMC ver 3
#define CT_SD1              0x02                // SD ver 1
#define CT_SD2              0x04                // SD ver 2
#define CT_SDC              (CT_SD1|CT_SD2)     // SD
#define CT_BLOCK            0x08                // Block addressing

// Vars
// -------------------------------------------

extern u8 ctype;

/*---------------------------------------*/
/* Prototypes for disk control functions */

DSTATUS disk_initialize (void);
DRESULT disk_readp (BYTE* buff, DWORD sector, UINT offser, UINT count);
DRESULT disk_writep (const BYTE* buff, DWORD sc);

void sd_cs_on();
void sd_cs_off();
void sd_rel();
u8 sd_rd();
void sd_wr(u8);
void sd_skip(u16);
u8 sd_cmd(u8, u32);
void sd_recv(void*, u16);
void sd_send(void*, u16);

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */

#ifdef __cplusplus
}
#endif
#endif	/* _DISKIO_DEFINED */
