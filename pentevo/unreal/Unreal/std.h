#pragma once

#define _WIN32_WINNT        0x0501   // mouse wheel since win2k
#define _WIN32_IE           0x0500   // for property sheet in win95. without this will not start in 9x
#define DIRECTINPUT_VERSION 0x05b2   // joystick since dx 5.0 (for NT4, need 3.0)
#define DIRECTSOUND_VERSION 0x0800
#define DIRECTDRAW_VERSION  0x0500

#ifdef DEBUG
#define D3D_DEBUG_INFO 1
#endif

#include <Windows.h>
#include <WindowsX.h>
#include <SetupAPI.h>
#include <CommCtrl.h>
#include <GdiPlus.h>
#include <d3d9.h>
#ifdef __GNUC__
#define D3DVECTOR_DEFINED
#define COMMPROP_INITIALIZED ((DWORD)0xE73CF52E)
#else
#include <InitGuid.h>
#endif
#include <ddraw.h>
#include <dinput.h>
#include <dsound.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <malloc.h>
#include <conio.h>
#include <math.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/stat.h>

#include <exception>

#include <Poco/Util/Application.h>

#if _MSC_VER >= 1300
#include <intrin.h>
#include <emmintrin.h>
#endif

#ifdef __GNUC__
#undef NULL
#define NULL 0
#include <x86intrin.h>
#include <cpuid.h>
#include <mcx.h>
#include <winioctl.h>
//#include <ddk/scsi.h>
//#include <ddk/ntdddisk.h>
//#include <ddk/ntddvol.h>
//#include <ddk/ntifs.h>
#else
#endif

#include "ddk.h"
#include "mods.h"

constexpr auto CACHE_LINE = 64;

#if _MSC_VER >= 1300
#define CACHE_ALIGNED __declspec(align(CACHE_LINE))
#else
#define CACHE_ALIGNED /*Alone Coder*/
#endif
