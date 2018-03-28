
#pragma once

// common types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef u8 bool;
typedef void (*TASK)();

// macros
#define false 0
#define true 1
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define countof(a)  (sizeof(a) / sizeof(a[0]))

// keys
enum
{
  KEY_CS = 0,
  KEY_Z,
  KEY_X,
  KEY_C,
  KEY_V,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_F,
  KEY_G,
  KEY_Q,
  KEY_W,
  KEY_E,
  KEY_R,
  KEY_T,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_0,
  KEY_9,
  KEY_8,
  KEY_7,
  KEY_6,
  KEY_P,
  KEY_O,
  KEY_I,
  KEY_U,
  KEY_Y,
  KEY_EN,
  KEY_L,
  KEY_K,
  KEY_J,
  KEY_H,
  KEY_SP,
  KEY_SS,
  KEY_M,
  KEY_N,
  KEY_B,
  KEY_NONE
};
