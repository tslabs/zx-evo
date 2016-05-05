/*

Copyright (c) Future Technology Devices International 2014

THIS SOFTWARE IS PROVIDED BY FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
FUTURE TECHNOLOGY DEVICES INTERNATIONAL LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES LOSS OF USE, DATA, OR PROFITS OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

FTDI DRIVERS MAY BE USED ONLY IN CONJUNCTION WITH PRODUCTS BASED ON FTDI PARTS.

FTDI DRIVERS MAY BE DISTRIBUTED IN ANY FORM AS LONG AS LICENSE INFORMATION IS NOT MODIFIED.

IF A CUSTOM VENDOR ID AND/OR PRODUCT ID OR DESCRIPTION STRING ARE USED, IT IS THE
RESPONSIBILITY OF THE PRODUCT MANUFACTURER TO MAINTAIN ANY CHANGES AND SUBSEQUENT WHQL
RE-CERTIFICATION AS A RESULT OF MAKING THESE CHANGES.

Abstract:

This file contains is functions for all UI fields.

Author : FTDI 

Revision History: 
0.1 - date 2013.04.24  - initial version
0.2 - date 2014.04.28 - Split in individual files according to platform

*/

#ifndef _FT_DATATYPES_H_
#define _FT_DATATYPES_H_


#define FT_FALSE           (0)
#define FT_TRUE            (1)

/* Inclusion of datatypes from MSVC */

typedef char ft_char8_t;
typedef signed char ft_schar8_t;
typedef unsigned char ft_uchar8_t;
typedef ft_uchar8_t ft_uint8_t;
typedef short  ft_int16_t;
typedef unsigned short ft_uint16_t;
typedef unsigned int ft_uint32_t;
typedef int ft_int32_t;
typedef void ft_void_t;
typedef long long ft_int64_t;
typedef unsigned long long ft_uint64_t;
typedef float ft_float_t;
typedef double ft_double_t;
typedef char ft_bool_t;

#define FT_BYTE_SIZE (1)
#define FT_SHORT_SIZE (2)
#define FT_WORD_SIZE (4)
#define FT_DWORD_SIZE (8)

#define FT_NUMBITS_IN_BYTE (1*8)
#define FT_NUMBITS_IN_SHORT (2*8)
#define FT_NUMBITS_IN_WORD (4*8)
#define FT_NUMBITS_IN_DWORD (8*8)

#define ft_prog_uchar8_t  ft_uchar8_t
#define ft_prog_char8_t   ft_char8_t
#define ft_prog_uint16_t  ft_uint16_t


#define ft_random(x)		(rand() % (x))
#define ft_millis()         GetTickCount()

#define ft_pgm_read_byte_near(x)   (*(x))
#define ft_pgm_read_byte(x)        (*(x))

#define ft_strcpy_P     strcpy
#define ft_strlen_P     strlen

#define ft_delay(x) Ft_Gpu_Hal_Sleep(x)
#define FT_DBGPRINT(x)  printf(x)
#define FT_PROGMEM

#define ft_pgm_read_byte_near(x)   (*(x))
#define ft_pgm_read_byte(x)        (*(x))

#define ft_pgm_read_word(addr)   (*(ft_int16_t*)(addr))

#ifdef _MSC_VER
#ifndef FT8XXEMU_INTTYPES_DEFINED_BASE
#define FT8XXEMU_INTTYPES_DEFINED_BASE
typedef unsigned __int8 uint8_t;
typedef signed __int8 int8_t;
typedef unsigned __int16 uint16_t;
typedef signed __int16 int16_t;
typedef unsigned __int32 uint32_t;
typedef signed __int32 int32_t;
typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
#endif
#else
#include <stdint.h>
#include <stdlib.h>
#endif
#ifndef FT8XXEMU_INTTYPES_DEFINED_COLOR
#define FT8XXEMU_INTTYPES_DEFINED_COLOR
typedef uint32_t argb8888;
#endif

#ifdef WIN32
#	define FT8XXEMU_FORCE_INLINE __forceinline
#else
#	define FT8XXEMU_FORCE_INLINE inline __attribute__((always_inline))
#endif


#ifndef FT8XXEMU_INTTYPES_DEFINED_FORCEINLINE
#define FT8XXEMU_INTTYPES_DEFINED_FORCEINLINE
#ifdef _MSC_VER
#	define FT8XXEMU_FORCE_INLINE __forceinline
#else
#	define FT8XXEMU_FORCE_INLINE inline __attribute__((always_inline))
#endif
#endif




#endif /*_FT_DATATYPES_H_*/


/* Nothing beyond this*/




