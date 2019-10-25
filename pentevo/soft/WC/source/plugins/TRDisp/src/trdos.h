#include "defs.h"

#define TRD_SIGN ((u8)0x10)

typedef enum
{
    DS_80 = (u8)0x16,
    DS_40 = (u8)0x17,
    SS_80 = (u8)0x18,
    SS_40 = (u8)0x19
} TRD_TYPE_t;

typedef struct
{
    u8 zero;
    u8 res0[224];
    u8 free_sec_next;
    u8 free_trk_next;
    TRD_TYPE_t type;
    u8 n_files;
    u16 n_free_sec;
    u8 ident;
    u8 res1[2];
    u8 res2[9];
    u8 res3;
    u8 n_del_files;
    char disk_title[8];
    u8 res4[3];
} TRD_9SEC_t;

typedef struct
{
    char name[8];
    union
    {
        struct
        {
            char type;
            u16 start;
        };
        struct
        {
            char ext[3];
        };
    };
    u16 length;
    u8 n_sec;
    u8 sector;
    u8 track;
} TRD_FILE_t;

typedef struct
{
    TRD_FILE_t catalogue[128];
    TRD_9SEC_t info;
} TRD_DISK_t;

void trd_read_sector(u8 *sector_buff, u8 track, u8 sector);
void trd_write_sector(u8 *sector_buff, u8 track, u8 sector);
void trd_read_cat(TRD_FILE_t *cat, TRD_9SEC_t *info);
