#pragma once

// My types definition
typedef unsigned long long	U64;
typedef signed long long	I64;
typedef unsigned long		U32;
typedef signed long			I32;
typedef unsigned short		U16;
typedef signed short		I16;
typedef unsigned char		U8;
typedef signed char			I8;

#define lo(a)	(U8)(a)
#define hi(a)	(U8)((a) >> 8)
