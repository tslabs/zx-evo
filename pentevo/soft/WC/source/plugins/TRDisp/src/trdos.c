#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "wc_api.h"
#include "trdos.h"

static u8 sector_buff[256];

void trd_read_sector(u8 *sector_buff, u8 track, u8 sector)
{
    wc_vpage3(track>>2);
    memcpy(sector_buff, (u16*)((((track&0x03)*4096) + ((sector&0x0F)<<8)) | 0xC000), 256);
}

void trd_write_sector(u8 *sector_buff, u8 track, u8 sector)
{
    wc_vpage3(track>>2);
    memcpy((u16*)((((track&0x03)*4096) + ((sector&0x0F)<<8)) | 0xC000), sector_buff, 256);
}

u8 get_free_entry(TRD_9SEC_t *info)
{
    return info->num_files;
}

void trd_read_cat(TRD_FILE_t *cat, TRD_9SEC_t *info)
{
    for(u8 sec=0; sec<8; sec++)
    {
        trd_read_sector(sector_buff, 0, sec);
        memcpy(&cat[sec*16], sector_buff, 256);
    }

    trd_read_sector(sector_buff, 0, 8);
    memcpy(info, sector_buff, 256);
}

void trd_save_cat(TRD_FILE_t *cat, TRD_9SEC_t *info)
{
    for(u8 sec=0; sec<8; sec++)
    {
        memcpy(sector_buff, &cat[sec*16], 256);
        trd_write_sector(sector_buff, 0, sec);
    }
    memcpy(sector_buff, info, 256);
    trd_write_sector(sector_buff, 0, 8);
}

bool is_trd(TRD_9SEC_t *info)
{
    if((info->ident == TRD_SIGN) && (info->zero2 == 0)) return true;
    return false;
}
