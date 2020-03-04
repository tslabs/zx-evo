#include "defs.h"

#define TRD_SIGN ((u8)0x10)

typedef enum
{
    DS_80 = (u8)0x16,
    DS_40 = (u8)0x17,
    SS_80 = (u8)0x18,
    SS_40 = (u8)0x19
} TRD_TYPE_t;

typedef struct TRD_9SEC_t
{
    u8 buff[223];
    u16 dcu_sec;
    u8 free_sec_next;
    u8 free_trk_next;
    u8 type;
    u8 num_files;
    u16 num_free_sec;
    u8 ident;
    u16 zero2;
    u8 space9[9];
    u8 zero1;
    u8 num_del_files;
    char disk_title[8];
    u8 zero3[3];
} TRD_9SEC_t;

typedef struct TRD_FILE_t
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
    u8 num_sec;
    u8 sector;
    u8 track;
} TRD_FILE_t;

/*
typedef struct TRD_TRACK_t
{
    u8 track;
    u8 sector;
} TRD_TRACK_t;
*/

typedef struct TRD_DISK_t
{
    TRD_FILE_t catalogue[128];
    TRD_9SEC_t info;
} TRD_DISK_t;

void trd_read_sector(u8 *sector_buff, u8 track, u8 sector);
void trd_write_sector(u8 *sector_buff, u8 track, u8 sector);
void trd_read_cat(TRD_FILE_t *cat, TRD_9SEC_t *info);
void trd_save_cat(TRD_FILE_t *cat, TRD_9SEC_t *info);
bool is_trd(TRD_9SEC_t *info);
