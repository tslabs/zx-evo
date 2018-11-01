#ifndef TYPEDEF_H
#define TYPEDEF_H

// common types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef u8 bool;
typedef void (*TASK)();
typedef void (*func_t)();

// macros
#define false 0
#define true 1
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define countof(a)  (sizeof(a) / sizeof(a[0]))

#endif
