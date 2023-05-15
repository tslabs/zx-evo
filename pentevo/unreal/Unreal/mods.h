#pragma once
/* edit this file to make optimized versions without unnecessary parts */

#define MOD_GSZ80     // exact gs emulation through Z80/ROM/DACs
#define MOD_GSBASS    // fast gs emulation via sampleplayer and BASS mod-player
#define MOD_FASTCORE  // use optimized code for Z80 when no breakpoints set
#define MOD_SETTINGS  // win32 dialog with emulation settings and tape browser
#define MOD_MONITOR   // debugger
#define MOD_MEMBAND_LED // 'memory band' led

/* ------------------------- console output colors ------------------------- */

const int CONSCLR_DEFAULT   = 0x07;
const int CONSCLR_TITLE     = 0x0F;
const int CONSCLR_ERROR     = 0x0C;
const int CONSCLR_ERRCODE   = 0x04;
const int CONSCLR_WARNING   = 0x0E;
const int CONSCLR_HARDITEM  = 0x03;
const int CONSCLR_HARDINFO  = 0x0B;
const int CONSCLR_INFO      = 0x02;

/* ************************************************************************* */
/* * don't edit below this line                                            * */
/* ************************************************************************* */

#if defined(MOD_GSBASS) || defined(MOD_GSZ80)
#define MOD_GS
#endif

#if defined(MOD_MONITOR) || defined(MOD_MEMBAND_LED)
#define MOD_DEBUGCORE
#endif

#if !defined(MOD_FASTCORE) && !defined(MOD_DEBUGCORE)
#define MOD_FASTCORE
#endif

/* ************************************************************************* */

#if (defined(_M_IX86_FP) && (_M_IX86_FP >= 2)) || (defined(_M_X64)) // compiled with option /arch:SSE2 or x64 build
 #define MOD_SSE2
#else
 #undef MOD_SSE2
#endif

