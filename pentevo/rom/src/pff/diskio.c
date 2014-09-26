
// Low level disk I/O module for ZC
// (c) TSL 2014
//
// -------------------------------------------
// This module uses some IAR-specific things,
// so it is not fully portable
// -------------------------------------------

#include "diskio.h"
#include "defs.h"

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

U8 ctype = CT_NONE;
DWORD sec_cached = -1;
U8 sec_buf[512];

// Low-level functions
// -------------------------------------------

#define sd_cs_off()     output8(ZC_SD_CFG, 0x03)
#define sd_cs_on()      output8(ZC_SD_CFG, 0x01)
#define sd_rel()        sd_cs_off(); sd_wr(0xFF)
#define sd_rd()         input8(ZC_SD_DAT)
#define sd_wr(a)        output8(ZC_SD_DAT, (a))
#define sd_rd_sec(a)    input_block_inc(ZC_SD_DAT, (a), 0); input_block_inc(ZC_SD_DAT, (a + 256), 0)

// Receive data block
void sd_recv(U8 *buf, U16 n)
{
    while (n--)
        *buf++ = sd_rd();
}

// Send data block
void sd_send(U8 *buf, U16 n)
{
    while (n--)
        sd_wr(*buf++);
}

// Send dummy 0xFF bytes
void sd_skip(U16 n)
{
    while (n--)
        sd_wr(0xFF);
}

// Send command packet
U8 sd_cmd(U8 cmd, U32 arg)
{
    U8 rc;
    U8 n;

	// ACMD<n> is the command sequence of CMD55, CMD<n>
	if (cmd & 0x80)
    {
		cmd &= 0x7F;
		if ((rc = sd_cmd(APP_CMD, 0)) > 1) goto exit;
	}

    // select the card
    sd_cs_off(); sd_wr(0xFF);
    sd_cs_on(); sd_wr(0xFF);

    // send command packet
    sd_wr(cmd);
    sd_wr(((U8*)&arg)[3]);
    sd_wr(((U8*)&arg)[2]);
    sd_wr(((U8*)&arg)[1]);
    sd_wr(((U8*)&arg)[0]);
    sd_wr((cmd == GO_IDLE_STATE) ? 0x95 : ((cmd == SEND_IF_COND) ? 0x87 : 0x01));   // real CRCs for certain commands of dummy + stop

    // receive command response
    n = 10;
	do
        rc = sd_rd();
      while ((rc & 0x80) && --n);

exit:
    return rc;
}

// FAT-FS functions
// -------------------------------------------

// Initialize Disk Drive
DSTATUS disk_initialize (void)
{
    DSTATUS stat = STA_NOINIT;
    U8 buf[4];
    U8 cmd;
    U16 i = -1;

    ctype = CT_NONE;
    sec_cached = -1;

    sd_cs_off();
    sd_skip(512 + 10);   // to clean the pipes

    if (sd_cmd(GO_IDLE_STATE, 0) != 1) goto exit;

    // SDv2
    if (sd_cmd(SEND_IF_COND, 0x1AA) == 1)
    {
        sd_recv(buf, 4);    // get trailing return value of R7 resp

        // non compatible voltage range
        if ((buf[2] != 0x01) || (buf[3] != 0xAA)) goto exit;

        // wait for leaving idle state (ACMD41 with HCS bit)
        while ((sd_cmd(SEND_OP_COND_SD, 1UL << 30) != 0) && --i);
        if (!i) goto exit;

        // check CCS bit in the OCR
        if (!sd_cmd(READ_OCR, 0) == 0) goto exit;

        sd_recv(buf, 4);    // get trailing return value of R7 resp
        ctype = (buf[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;     // SDv2 (HC or SC)
    }

    // SDv1 or MMCv3
    else
    {
        // SDv1
		if (sd_cmd(SEND_OP_COND_SD, 0) <= 1)
        {
			ctype = CT_SD1;
            cmd = SEND_OP_COND_SD;
		}

        // MMCv3
        else
        {
			ctype = CT_MMC;
            cmd = SEND_OP_COND_MMC;
		}

        // wait for leaving idle state
        while ((sd_cmd(cmd, 0) != 0) && --i);
        if (!i)
        {
			ctype = CT_NONE;
            goto exit;
        }

		// set R/W block length to 512
		if (sd_cmd(SET_BLOCKLEN, 512) != 0)
			ctype = CT_NONE;
	}

    stat = 0;   // set success status

exit:
    sd_rel();   // release SPI
    return stat;
}

// Read Partial Sector

DRESULT disk_readp
(
    BYTE* buff,     // Pointer to the destination object (NULL: Forward data to the stream)
    DWORD sector,   // Sector number (LBA)
    UINT offset,    // Offset in the sector
    UINT count      // Byte count to read (offset + count mus be <= 512)
)
{
    DRESULT res = RES_ERROR;
    U16 i;
    U8 d;


    if (sec_cached != sector)
    {
        if (sd_cmd(READ_SINGLE_BLOCK, (ctype & CT_BLOCK) ? sector : (sector * 512)) != 0) goto exit;

        output8(0xFE, 5);   // !!!
        // wait for data packet
        i = -1;
        do
        {
            d = sd_rd();
        } while ((d == 0xFF) && --i);

        if (d != 0xFE) goto exit;

        output8(0xFE, 1);   // !!!
        // receive a sector
        if (!offset && (count == 512))
        {
            sd_rd_sec(buff);
            sd_skip(2);     // Skip CRC
            goto done;
        }

        else
        {
            sd_rd_sec(sec_buf);
            sd_skip(2);     // Skip CRC
            sec_cached = sector;
        }
    }

    memcpy(buff, &sec_buf[offset], count);

done:
    res = RES_OK;

exit:
    sd_rel();   // release SPI
    output8(0xFE, 0);   // !!!

    return res;
}



// Write Partial Sector

DRESULT disk_writep
(
    const BYTE* buff,   // Pointer to the data to be written, NULL:Initiate/Finalize write operation
    DWORD sc            // Sector number (LBA) or Number of bytes to send
)
{
    DRESULT res;


    if (!buff)
    {
        // Initiate write process
        if (sc)
        {


        }

        // Finalize write process
        else
        {


        }
    }

    // Send data to the disk
    else
    {


    }

    return res;
}
