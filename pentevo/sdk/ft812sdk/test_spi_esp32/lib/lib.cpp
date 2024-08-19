
#include <stdint.h>
#include <stddef.h>
#include <math.h>

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

enum
{
  F_INIT    = 0,
  F_RAND    = 1,
  F_FACT    = 2,
  F_SIN     = 3,
  F_TUNN    = 10,
  F_COBRA   = 11,
};

#define DEGS 360
#define PI 3.14159265358979323846

#include "ft812.h"
#include "ft812.cpp"

float sin_tab[DEGS];
int phase = 0;
u32 seed = 0x12345678;

unsigned long long factorial(int n)
{
  unsigned long long result = 1;

  for (int i = 2; i <= n; i++)
    result *= i;

  return result;
}

void calc_sin()
{
  for (int i = 0; i < DEGS; i++)
  {
    float s = sin((float)i * PI / (DEGS / 2));
    sin_tab[i] = s;
  }
}

int gen_rand(int size, void *out)
{
  u16 *p = (u16*)out;
  u32 s = seed;

  for (int i = 0; i < size / 2; i++)
  {
    s = s * 1103515245 + 12345;
    p[i] = s >> 16;
  }

  seed = s;
}

#include "tunnel.cpp"
#include "cobra.cpp"

int init(int s)
{
  seed = s;
  phase = 0;
  calc_sin();

  return 200;
}

extern "C" int lib(int func, int arg, void *arr1)
{
  int rc = 0;

  switch (func)
  {
    case F_INIT:
      rc = init(arg);
    break;

    case F_RAND:
      rc = gen_rand(arg, arr1);
    break;

    case F_FACT:
      rc = factorial(arg);
    break;

    case F_TUNN:
      rc = tunnel(arr1);
    break;

    case F_COBRA:
      rc = cobra(arr1);
    break;
  }
  
  return rc;
}
