#pragma once

#define SB_VER    SPG Builder ver.1.0 by TS-Labs
#define SB_CRT    Created by SPG Builder ver.1.0
#define SB_PAK    Used MHMT packer by LVD

// error codes and messages
enum RC_ERR
{
  RC_OK = 0,
  RC_ARG,
  RC_INI,
  RC_UNK,
  RC_ALGN,
  RC_FNFD,
  RC_FERR,
  RC_PACK,
  RC_0BLK,
  RC_BIG,
  RC_ZERO,
  RC_MHMT,
  RC_VER,
  RC_ADDR,
  RC_MALC
};

#define ER_OK     DONE!
#define ER_ARG    Wrong argument!
#define ER_INI    Wrong .ini!
#define ER_UNK    Unknown tag!
#define ER_ALGN   Block address is not a 512 multiple!
#define ER_FNFD   File not found!
#define ER_FERR   File error!
#define ER_PACK   Compression method not supported!
#define ER_0BLK   Zero blocks defined!
#define ER_BIG    Block is too BIG!
#define ER_ZERO   Block is zero size!
#define ER_MHMT   No mhmt.exe found! Place it in PATH or current dir.
#define ER_VER    Unsupported SPG version!
#define ER_ADDR   Block Address is not in 49152-65024 span!
#define ER_MALC   Malloc error!

#define	error(a) { \
  printf(STR(ER_##a)); \
  printf("\n\n"); \
  exit(RC_##a); }

void print_help();
