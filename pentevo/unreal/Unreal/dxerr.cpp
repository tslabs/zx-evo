#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

#ifdef EMUL_DEBUG
#include "dxerr8.h"
void printrdd(const char *pr, HRESULT r)
{
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, DXGetErrorString8(r));
}

void printrdi(const char *pr, HRESULT r)
{
   color(CONSCLR_ERROR);
   printf("%s: %s\n", pr, DXGetErrorString8(r));
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
   printf("%s: %s\n", pr, DXGetErrorString8(r));
}
#else
#define printrdd(x,y)
#define printrdi(x,y)
#define printrds(x,y)
#define printrmm(x,y)
#endif
