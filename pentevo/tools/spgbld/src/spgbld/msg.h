#pragma once

#define SB_VER		SPG Builder ver.1.0 by TS-Labs
#define SB_CRT		Created by SPG Builder ver.1.0
#define SB_PAK		Used MHMT packer by LVD

// error codes and messages
#define RC_OK		0
#define ER_OK		DONE!
#define RC_NOARG	1
#define ER_NOARG	Wrong argument!
#define RC_NOINI	2
#define ER_NOINI	Wrong .ini!
#define RC_UNK		3
#define ER_UNK		Unknown tag!
#define RC_ALGN		4
#define ER_ALGN		Block address is not a 512 multiple!
#define RC_NOFILE	5
#define ER_NOFILE	File NOT found!
#define RC_PACK		6
#define ER_PACK		Compression method not supported!
#define RC_0BLK		7
#define ER_0BLK		Zero blocks defined!
#define RC_BIG		8
#define ER_BIG		Block is LARGER than 16384 bytes!
#define RC_ZERO		9
#define ER_ZERO		Block is ZERO size!

void print_help();
void error(int);
