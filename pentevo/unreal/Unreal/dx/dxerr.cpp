#include "std.h"
#include "emul.h"
#include "util.h"
#include <mmsystem.h>

typedef struct {
    HRESULT      hr;
    const CHAR*  resultA;
    const WCHAR* resultW;
    const CHAR*  descriptionA;
    const WCHAR* descriptionW;
} error_info;

#include "dxerrors.h"

const char * WINAPI DXGetErrorString8A(HRESULT hr)
{
    unsigned int i, j, k = 0;

    for (i = sizeof(info) / sizeof(info[0]); i != 0; i /= 2) {
        j = k + (i / 2);
        if (hr == info[j].hr)
            return info[j].resultA;
        if ((unsigned int)hr > (unsigned int)info[j].hr) {
            k = j + 1;
            i--;
        }
    }

    return "Unknown";
}

#ifdef EMUL_DEBUG

void printrdd(const char *pr, HRESULT r)
{
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, DXGetErrorString8A(r));
}

void printrdi(const char *pr, HRESULT r)
{
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, DXGetErrorString8A(r));
}

void printrmm(const char *pr, HRESULT r)
{
   char buf[200]; sprintf(buf, "unknown error (%08X)", r);
   const char *str = buf;
   switch (r)
   {
      case MMSYSERR_NOERROR: str = "ok"; break;
      case MMSYSERR_INVALHANDLE: str = "MMSYSERR_INVALHANDLE"; break;
      case MMSYSERR_NODRIVER: str = "MMSYSERR_NODRIVER"; break;
      case WAVERR_UNPREPARED: str = "WAVERR_UNPREPARED"; break;
      case MMSYSERR_NOMEM: str = "MMSYSERR_NOMEM"; break;
      case MMSYSERR_ALLOCATED: str = "MMSYSERR_ALLOCATED"; break;
      case WAVERR_BADFORMAT: str = "WAVERR_BADFORMAT"; break;
      case WAVERR_SYNC: str = "WAVERR_SYNC"; break;
      case MMSYSERR_INVALFLAG: str = "MMSYSERR_INVALFLAG"; break;
   }
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, str);
}

void printrds(const char *pr, HRESULT r)
{
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, DXGetErrorString8A(r));
}
#else
#define printrdd(x,y)
#define printrdi(x,y)
#define printrds(x,y)
#define printrmm(x,y)
#endif
