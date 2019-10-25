
#pragma once

/* Starting conditions*/
typedef enum
{
    SC_EXT      = 0x00,
    SC_ONLOAD   = 0x01,
    SC_TIMER    = 0x02,
    SC_MENU     = 0x03,
    SC_EMENU    = 0x04,
    SC_EDIT     = 0x14
} SCOND;

typedef struct
{
    u8 page;
    u8 size;
} WC_HDR_BLK;

typedef struct
{
    u8 res0[16];        /* +00 reserved             */
    char mdl[16];       /* +16 Magic string         */
    u8 ver;             /* +32 Format version       */
    u8 res1[1];         /* +33 reserved             */
    u8 num_pages;       /* +34 Number of pages      */
    u8 page2;           /* +35 Page at 0x8000       */
    WC_HDR_BLK blk[6];  /* +36 Page definitions     */
    u8 res2[16];        /* +48 -reserved-           */
    char ext[32][3];    /* +64 Extensions           */
    u8 res3[1];         /* +160 0x00                */
    u32 file_max_size;  /* +161 Max file size       */
    char name[32];      /* +165 Plugin name         */
    SCOND cond;         /* +197 Starting condition  */
    char edit_txt[6];   /* +198 Text for F4 menu    */
    char menu_txt[24];  /* +204 Text for viewer menu*/
    u8 res4[284];       /* +228 -reserved-          */
} WC_HDR;
