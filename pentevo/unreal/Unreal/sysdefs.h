#pragma once

//#define inline __inline
#define forceinline __forceinline
#define fastcall             // parameters in registers

typedef unsigned long long u64;
typedef signed long long i64;
typedef unsigned long u32;
typedef signed long i32;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned char u8;
typedef signed char i8;

#define countof(a)  (sizeof(a) / sizeof(a[0]))

#ifdef _MSC_VER
#define ATTR_ALIGN(x) __declspec(align(x))
#endif

#if __ICL >= 1000 || defined(__GNUC__)
static inline u8 rol8(u8 val, u8 shift)
{
    __asm__ volatile ("rolb %1,%0" : "=r"(val) : "cI"(shift), "0"(val));
    return val;
}

static inline u8 ror8(u8 val, u8 shift)
{
    __asm__ volatile ("rorb %1,%0" : "=r"(val) : "cI"(shift), "0"(val));
    return val;
}
static inline void asm_pause() { __asm__("pause"); }
#else
extern "C" unsigned char __cdecl _rotr8(unsigned char value, unsigned char shift);
extern "C" unsigned char __cdecl _rotl8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotr8)
#pragma intrinsic(_rotl8)
static inline u8 rol8(u8 val, u8 shift) { return _rotl8(val, shift); }
static inline u8 ror8(u8 val, u8 shift) { return _rotr8(val, shift); }
static inline void asm_pause() { __asm {rep nop} }
#endif

#if defined(_MSC_VER) && _MSC_VER < 1300
static inline u16 _byteswap_ushort(u16 i){return (i>>8)|(i<<8);}
static inline u32 _byteswap_ulong(u32 i){return _byteswap_ushort((u16)(i>>16))|(_byteswap_ushort((u16)i)<<16);};
#endif

#ifdef __GNUC__
#include <stdint.h>
#define HANDLE_PRAGMA_PACK_PUSH_POP
#define __forceinline __attribute__((always_inline))
#undef forceinline
#define forceinline __forceinline
#define override

#define ATTR_ALIGN(x) __attribute__((aligned(x)))

static __inline__ void __debugbreak(void)
{
  __asm__ __volatile__ ("int $3");
}

static __inline__ unsigned short _byteswap_ushort(unsigned short x)
{
    __asm__("xchgb %b0,%h0" : "=q"(x) :  "0"(x));
    return x;
}

#define _byteswap_ulong(x) _bswap(x) 

#define _countof(x) (sizeof(x)/sizeof((x)[0]))
#define __assume(x)

#endif // __GNUC__
