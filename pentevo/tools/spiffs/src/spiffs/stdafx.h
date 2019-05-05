// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _CRT_SECURE_NO_WARNINGS 1

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <string.h>

typedef unsigned long long  U64;
typedef unsigned long       U32;
typedef unsigned short      U16;
typedef unsigned char       U8;
typedef unsigned long long  u64;
typedef unsigned long       u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef signed long long    I64;
typedef signed long         I32;
typedef signed short        I16;
typedef signed char         I8;
typedef signed long long    i64;
typedef signed long         i32;
typedef signed short        i16;
typedef signed char         i8;

typedef signed long long    S64;
typedef signed long         S32;
typedef signed short        S16;
typedef signed char         S8;
typedef signed long long    s64;
typedef signed long         s32;
typedef signed short        s16;
typedef signed char         s8;

#define lo(a)	(U8)(a)
#define hi(a)	(U8)((a) >> 8)

#define countof(a)  (sizeof(a) / sizeof(a[0]))
