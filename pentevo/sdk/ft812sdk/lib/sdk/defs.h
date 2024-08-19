
#pragma once

// common types
typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long       u32;
typedef unsigned long long  u64;
typedef signed char         i8;
typedef signed short        i16;
typedef signed long         i32;
typedef signed long long    i64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed long         s32;
typedef signed long long    s64;
typedef u8 bool;
typedef void (*TASK)();
typedef void (*func_t)();

// macros
#define false 0
#define true 1
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define countof(a)  (sizeof(a) / sizeof(a[0]))

// keys
#define KEY_CS_  40
#define KEY_Z    39
#define KEY_X    38
#define KEY_C    37
#define KEY_V    36
#define KEY_A    35
#define KEY_S    34
#define KEY_D    33
#define KEY_F    32
#define KEY_G    31
#define KEY_Q    30
#define KEY_W    29
#define KEY_E    28
#define KEY_R    27
#define KEY_T    26
#define KEY_1    25
#define KEY_2    24
#define KEY_3    23
#define KEY_4    22
#define KEY_5    21
#define KEY_0    20
#define KEY_9    19
#define KEY_8    18
#define KEY_7    17
#define KEY_6    16
#define KEY_P    15
#define KEY_O    14
#define KEY_I    13
#define KEY_U    12
#define KEY_Y    11
#define KEY_EN   10
#define KEY_L    9
#define KEY_K    8
#define KEY_J    7
#define KEY_H    6
#define KEY_SP   5
#define KEY_SS_  4
#define KEY_M    3
#define KEY_N    2
#define KEY_B    1
#define KEY_NONE 0
#define KEY_CS   64
#define KEY_SS   128
