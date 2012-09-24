#pragma once

#define SB_VER		SPG Builder ver.1.0 by TS-Labs
#define SB_CRT		Created by SPG Builder ver.1.0
#define SB_PAK		Used MHMT packer by LVD

// error codes and messages
#define RC_OK		0
#define ER_OK		DONE!
#define RC_ARG		1
#define ER_ARG		Wrong argument!
#define RC_INI		2
#define ER_INI		Wrong .ini!
#define RC_UNK		3
#define ER_UNK		Unknown tag!
#define RC_ALGN		4
#define ER_ALGN		Block address is not a 512 multiple!
#define RC_FILE		5
#define ER_FILE		File NOT found!
#define RC_PACK		6
#define ER_PACK		Compression method not supported!
#define RC_0BLK		7
#define ER_0BLK		Zero blocks defined!
#define RC_BIG		8
#define ER_BIG		Block is LARGER than 16384 bytes!
#define RC_ZERO		9
#define ER_ZERO		Block is ZERO size!
#define RC_MHMT		10
#define ER_MHMT		No mhmt.exe found! Place it in PATH or current dir.
#define RC_VER		11
#define ER_VER		Unsupported SPG version!

void print_help();
void error(int);
