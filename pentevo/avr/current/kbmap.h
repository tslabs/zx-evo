#ifndef __KB_MAP_H__
#define __KB_MAP_H__

/**
 * @file
 * @brief PS/2 to ZX keyboard mapping support.
 * @author http://www.nedopc.com
 *
 * PS/2 to ZX keyboard mapping support.
 */

/** ZX keyboard values */
//
#define KEY_SP	0
#define KEY_EN	1
#define KEY_P	2
#define KEY_0	3
#define KEY_1	4
#define KEY_Q	5
#define KEY_A	6
#define KEY_CS	7
//
#define KEY_SS	8
#define KEY_L	9
#define KEY_O  10
#define KEY_9  11
#define KEY_2  12
#define KEY_W  13
#define KEY_S  14
#define KEY_Z  15
//
#define KEY_M  16
#define KEY_K  17
#define KEY_I  18
#define KEY_8  19
#define KEY_3  20
#define KEY_E  21
#define KEY_D  22
#define KEY_X  23
//
#define KEY_N  24
#define KEY_J  25
#define KEY_U  26
#define KEY_7  27
#define KEY_4  28
#define KEY_R  29
#define KEY_F  30
#define KEY_C  31
//
#define KEY_B  32
#define KEY_H  33
#define KEY_Y  34
#define KEY_6  35
#define KEY_5  36
#define KEY_T  37
#define KEY_G  38
#define KEY_V  39
//
#define NO_KEY 0x7F
//#define RST_48 0x7E
//#define RST128 0x7D
//#define RSTRDS 0x7C
//#define RSTSYS 0x7B
#define CLRKYS 0x7A
//

/** Pointer to map. */
//extern UBYTE* kbmap;
/** Pointer to map (extent E0). */
//extern UBYTE* kbmap_E0;

/** Pointer to default map. */
//extern const UBYTE default_kbmap[];
/** Pointer to default map (extent E0). */
//extern const UBYTE default_kbmap_E0[];

/** Init keyboard mapping. */
void kbmap_init(void);

/** Data type for map values. */
typedef union
{
	struct
	{
		UBYTE b1;
		UBYTE b2;
	} tb;
	UWORD tw;
}
KBMAP_VALUE;

/**
 * Get keyboard map value.
 * @return map values
 * @param scancode [in] - code from PS/2 keyboard
 * @param was_E0 [in] - 0: code without prefix, >0: code with prefix E0
 */
KBMAP_VALUE kbmap_get(UBYTE scancode, UBYTE was_E0);

#endif //__KB_MAP_H__
