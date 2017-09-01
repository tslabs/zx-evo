
#pragma once

#include <defs.h>

typedef enum
{
  SC_EXT,
  SC_ONLOAD,
  SC_TIMER,
  SC_MENU,
} SCOND;

typedef struct
{
  u8 page;
  u8 size;
} WC_HDR_BLK;

typedef struct
{
  u8 res0[16];         /* +00 reserved            */
  char mdl[16];        /* +16 Magic string        */
  u8 ver;              /* +32 Format version      */
  u8 res1[1];          /* +33 reserved            */
  u8 num_pages;        /* +34 Number of pages     */
  u8 page2;            /* +35 Page at 0x8000      */
  WC_HDR_BLK blk[6];   /* +36 Page definitions    */
  u8 res2[16];         /* +48 -reserved-          */
  char ext[32][3];     /* +64 Extensions          */
  u8 res3[1];          /* +160 0x00               */
  u32 file_max_size;   /* +161 Max file size      */
  char name[32];       /* +165 Plugin name        */
  SCOND cond;          /* +197 Starting condition */
  u8 res4[314];        /* +198 -reserved-         */
} WC_HDR;
