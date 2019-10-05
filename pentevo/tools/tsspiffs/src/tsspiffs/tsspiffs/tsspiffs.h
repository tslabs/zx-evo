
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// #define TSF_CHECK_BLANK             // to test block after erase
// #define TSF_CHECK_EXIST_ON_CREATE   // to look for existing filename when file is created

typedef unsigned long long u64;
typedef unsigned long      u32;
typedef unsigned short     u16;
typedef unsigned char      u8;
typedef signed long long   s64;
typedef signed long        s32;
typedef signed short       s16;
typedef signed char        s8;

#ifndef min
#define min(x,y) ((x) < (y) ? (x) : (y))
#endif
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif

// enums
enum
{
 TSF_MAGIC = ((u32)0x78E15CA3)
};

typedef enum
{
  TSF_RES_OK = 0,
  TSF_RES_FS_ERROR,
  TSF_RES_FILE_NOT_FOUND,
  TSF_RES_FILE_EXISTS,
  TSF_RES_MODE_ERROR,
  TSF_RES_BULK_FULL,
  TSF_RES_NOT_BLANK,
  TSF_RES_NO_MORE_FILES
} TSF_RESULT;

typedef enum
{
  TSF_CHUNK_HEAD = 0x00,
  TSF_CHUNK_BODY = 0x01,
  TSF_CHUNK_FREE = 0xFF
} TSF_CHUNK_TYPE;

typedef enum
{
  TSF_MODE_READ   = 0x01,
  TSF_MODE_WRITE  = 0x02,
  TSF_MODE_CREATE = 0x04,
  TSF_MODE_CREATE_WRITE = TSF_MODE_CREATE | TSF_MODE_WRITE,
} TSF_MODE;

typedef enum
{
  TSF_LIST_NEXT = 0,
  TSF_LIST_START
} TSF_LIST;

// types
typedef TSF_RESULT (*tsf_read_t)(u32, void*, u32);
typedef TSF_RESULT (*tsf_write_t)(u32, const void*, u32);
typedef TSF_RESULT (*tsf_erase_t)(u32);

#ifdef _MSC_VER
#pragma pack(1)
#endif

typedef struct
{
  u32 magic;
  u16 next_chunk;
  u8 type;
} TSF_CHUNK;

typedef struct
{
  u32 size;
  u8 fnlen;
} TSF_HDR;

typedef struct
{
  u32 bulk_start;          // start address in flash IC
  u32 bulk_size;           // size of the TSF
  u32 block_size;          // erase block size
  u16 last_written_chunk;  // for wear levelling

  tsf_read_t  hal_read_func;
  tsf_write_t hal_write_func;
  tsf_erase_t hal_erase_func;

  u8 *buf;
  u16 buf_size;
} TSF_CONFIG;

typedef struct
{
  u32 free;
  u16 chunks_number;
  u16 files_number;
  TSF_CONFIG *cfg;
} TSF_VOLUME;

typedef struct
{
  u32 addr;             // first chunk address
  u32 size;             // file size
  u32 seek;             // current pointer
  u32 chunk_addr;       // current chunk address
  u32 chunk_offset;     // current offset in chunk
  u32 prev_chunk_addr;  // previous chunk address
  u16 next_chunk;       // next chunk
  u8 mode;
  TSF_VOLUME *vol;
} TSF_FILE;

typedef struct
{
  u32 size;
} TSF_FILE_STAT;

#ifdef _MSC_VER
#pragma pack()
#endif

// functions
// file operations
TSF_RESULT tsf_open(TSF_VOLUME*, TSF_FILE*, const char*, u8);
TSF_RESULT tsf_read(TSF_FILE*, void*, u32);
TSF_RESULT tsf_write(TSF_FILE*, const void*, u32);
TSF_RESULT tsf_close(TSF_FILE*);
TSF_RESULT tsf_seek(TSF_FILE*, u32);
TSF_RESULT tsf_delete(TSF_VOLUME*, const char*);
TSF_RESULT tsf_stat(TSF_VOLUME*, TSF_FILE_STAT*, const char*);
TSF_RESULT tsf_list(TSF_VOLUME*, u8);

// volume operations
TSF_RESULT tsf_mount(TSF_CONFIG*, TSF_VOLUME*);
TSF_RESULT tsf_format(TSF_CONFIG*);
TSF_RESULT tsf_check(TSF_CONFIG*);

// auxilliary functions
TSF_RESULT tsf_init_chunk(TSF_CONFIG*, u32);
TSF_RESULT tsf_check_blank(TSF_CONFIG*, u32, u32);
TSF_RESULT tsf_search(TSF_VOLUME*, u32*, const char*);
TSF_RESULT tsf_open_for_read(TSF_FILE*, const char*);
TSF_RESULT tsf_create(TSF_FILE*, const char*);
TSF_RESULT tsf_take_new_chunk(TSF_VOLUME*, u8, u32*);
TSF_RESULT tsf_vol_stat(TSF_VOLUME*);
TSF_RESULT tsf_read_int(TSF_FILE*, void*, u32);

#ifdef __cplusplus
}
#endif
