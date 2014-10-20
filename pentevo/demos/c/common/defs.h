
#include <intrz80.h>

// common types
typedef unsigned char   U8;
typedef unsigned short  U16;
typedef unsigned long   U32;
typedef signed char     S8;
typedef signed short    S16;
typedef signed long     S32;

// common constants
typedef enum
{
    FALSE = 0,
    TRUE
} BOOL;

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
