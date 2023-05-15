#include "std.h"
#include "emul.h"
#include "vars.h"
#include "snapshot.h"
#include "util.h"

static void cpuid(unsigned CpuInfo[4], unsigned _eax)
{
#ifdef _MSC_VER
   __cpuid((int *)CpuInfo, _eax);
#endif

#ifdef __GNUC__
   __cpuid(_eax, CpuInfo[0], CpuInfo[1], CpuInfo[2], CpuInfo[3]);
#endif
}

void fillCpuString(char dst[49])
{
   dst[0] = dst[12] = dst[48] = 0;
   unsigned CpuInfo[4];
   unsigned *d = (unsigned *)dst;

   cpuid(CpuInfo, 0x80000000);
   if (CpuInfo[0] < 0x80000004)
   {
       cpuid(CpuInfo, 0);
       d[0] = CpuInfo[1];
       d[1] = CpuInfo[3];
       d[2] = CpuInfo[2];
       return;
   }

   cpuid(CpuInfo, 0x80000002);
   d[0] = CpuInfo[0];
   d[1] = CpuInfo[1];
   d[2] = CpuInfo[2];
   d[3] = CpuInfo[3];

   cpuid(CpuInfo, 0x80000003);
   d[4] = CpuInfo[0];
   d[5] = CpuInfo[1];
   d[6] = CpuInfo[2];
   d[7] = CpuInfo[3];

   cpuid(CpuInfo, 0x80000004);
   d[ 8] = CpuInfo[0];
   d[ 9] = CpuInfo[1];
   d[10] = CpuInfo[2];
   d[11] = CpuInfo[3];
}

unsigned cpuid(unsigned _eax, int ext)
{
   unsigned CpuInfo[4];

   cpuid(CpuInfo, _eax);

   return ext ? CpuInfo[3] : CpuInfo[0];
}

unsigned __int64 GetCPUFrequency()
{
   LARGE_INTEGER Frequency;
   LARGE_INTEGER Start;
   LARGE_INTEGER Stop;
   u64 c1, c2, c3, c4, c;
   timeBeginPeriod(1);
   QueryPerformanceFrequency(&Frequency);
   Sleep(20);

   c1 = rdtsc();
   QueryPerformanceCounter(&Start);
   c2 = rdtsc();
   Sleep(500);
   c3 = rdtsc();
   QueryPerformanceCounter(&Stop);
   c4 = rdtsc();
   timeEndPeriod(1);
   c = c3 - c2 + (c4-c3)/2 + (c2-c1)/2;
   Start.QuadPart = Stop.QuadPart - Start.QuadPart;

   return ((c * Frequency.QuadPart) / Start.QuadPart);
}

void trim(char *dst)
{
   unsigned i = strlen(dst);
   // trim right spaces
   while (i && isspace(dst[i-1])) i--;
   dst[i] = 0;
   // trim left spaces
   for (i = 0; isspace(dst[i]); i++);
   strcpy(dst, dst+i);
}


const char clrline[] = "\r\t\t\t\t\t\t\t\t\t       \r";

#define savetab(x) {FILE *ff=fopen("tab","wb");fwrite(x,sizeof(x),1,ff);fclose(ff);}

#define tohex(a) ((a) < 10 ? (a)+'0' : (a)-10+'A')

const char nop = 0;
const char * const nil = &nop;

int ishex(char c)
{
   return (isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f'));
}

u8 hex(char p)
{
   p = tolower(p);
   return (p < 'a') ? p-'0' : p-'a'+10;
}

u8 hex(const char *p)
{
   return 0x10*hex(p[0]) + hex(p[1]);
}

unsigned process_msgs()
{
   MSG msg; unsigned key = 0;
   while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
   {
/*
      if (msg.message == WM_NCLBUTTONDOWN)
          continue;
*/
      // mouse messages must be processed further
      if (msg.message == WM_LBUTTONDOWN)
      {
          mousepos = msg.lParam;
          key = VK_LMB;
      }
      if (msg.message == WM_RBUTTONDOWN)
      {
          mousepos = msg.lParam | 0x80000000;
          key = VK_RMB;
      }
      if (msg.message == WM_MBUTTONDOWN)
      {
          mousepos = msg.lParam | 0x80000000;
          key = VK_MMB;
      }

      if (msg.message == WM_MOUSEWHEEL) // [vv] Process mouse whell only in client area
      {
          POINT Pos = { GET_X_LPARAM(msg.lParam), GET_Y_LPARAM(msg.lParam) };

          RECT ClientRect;
          GetClientRect(msg.hwnd, &ClientRect);
          MapWindowPoints(msg.hwnd, HWND_DESKTOP, (LPPOINT)&ClientRect, 2);

          if (PtInRect(&ClientRect, Pos))
              key = GET_WHEEL_DELTA_WPARAM(msg.wParam) < 0 ? VK_MWD : VK_MWU;
      }

      // WM_KEYDOWN and WM_SYSKEYDOWN must not be dispatched,
      // bcoz window will be closed on alt-f4
      if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
      {
         switch (( msg.lParam>>16)&0x1FF)
         {
            case 0x02a: kbdpcEX[0]=(kbdpcEX[0]^0x01)|0x80; break;
            case 0x036: kbdpcEX[1]=(kbdpcEX[1]^0x01)|0x80; break;
            case 0x01d: kbdpcEX[2]=(kbdpcEX[2]^0x01)|0x80; break;
            case 0x11d: kbdpcEX[3]=(kbdpcEX[3]^0x01)|0x80; break;
            case 0x038: kbdpcEX[4]=(kbdpcEX[4]^0x01)|0x80; break;
            case 0x138: kbdpcEX[5]=(kbdpcEX[5]^0x01)|0x80; break;
         } //Dexus
//         printf("%s, WM_KEYDOWN, WM_SYSKEYDOWN\n", __FUNCTION__);
         key = msg.wParam;
      }
      else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
      {
         switch (( msg.lParam>>16)&0x1FF)
         {
            case 0x02a: kbdpcEX[0]&=0x01; kbdpcEX[1]&=0x01; break;
            case 0x036: kbdpcEX[0]&=0x01; kbdpcEX[1]&=0x01; break;
            case 0x01d: kbdpcEX[2]&=0x01; break;
            case 0x11d: kbdpcEX[3]&=0x01; break;
            case 0x038: kbdpcEX[4]&=0x01; break;
            case 0x138: kbdpcEX[5]&=0x01; break;
         } //Dexus

//         printf("%s, WM_KEYUP, WM_SYSKEYUP\n", __FUNCTION__);
         DispatchMessage(&msg); //Dexus
      }
      else
         DispatchMessage(&msg);
   }
   return key;
}

void eat() // eat messages
{
   Sleep(20); while (process_msgs()) Sleep(10);
}

static u8 kbd_vk[VK_MAX];
char dispatch_more(action *table)
{
   if (!table)
       return -1;

    GetKeyboardState( kbd_vk );
	if (input.lastkey)
    {
        kbd_vk[input.lastkey] = 0x80;
    }

   kbd_vk[0] = 0x80; // nil button is always pressed

//   __debugbreak();
   while (table->name)
   {
//      printf("%02X|%02X|%02X|%02X\n", table->k1, table->k2, table->k3, table->k4);

	   if ( kbd_vk[table->k1] & kbd_vk[table->k2] &
           kbd_vk[table->k3] & kbd_vk[table->k4] & 0x80 )
      {
         table->func();
         return 1;
      }

      table++;
   }
   return -1;
}

char dispatch(action *table)
{
   if (*droppedFile)
   {
       trd_toload = default_drive;
       loadsnap(droppedFile);
       *droppedFile = 0;
   }
   if ( dbgbreak )
   {
	   input.lastkey = process_msgs();
	   if ( !input.lastkey )
		   return 0;
   }
   else if (!input.readdevices())
       return 0;

   dispatch_more(table);
   return 1;
}

bool wcmatch(char *string, char *wc)
{
   for (;;wc++, string++) {
      if (!*string && !*wc) return 1;
      if (*wc == '?') { if (*string) continue; else return 0; }
      if (*wc == '*') {
         for (wc++; *string; string++)
            if (wcmatch(string, wc)) return 1;
         return 0;
      }
      if (tolower(*string) != tolower(*wc)) return 0;
   }
}

void dump1(BYTE *p, unsigned sz)
{
   while (sz) {
      printf("\t");
      unsigned chunk = (sz > 16)? 16 : sz;
      unsigned i; //Alone Coder 0.36.7
      for (/*unsigned*/ i = 0; i < chunk; i++) printf("%02X ", p[i]);
      for (; i < 16; i++) printf("   ");
      for (i = 0; i < chunk; i++) printf("%c", (p[i] < 0x20)? '.' : p[i]);
      printf("\n");
      sz -= chunk, p += chunk;
   }
   printf("\n");
}

void color(int ink)
{
   SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ink);
}

void err_win32(DWORD errcode)
{
   if (errcode == 0xFFFFFFFF) errcode = GetLastError();

   char msg[512];
   if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, errcode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  msg, sizeof(msg), 0)) *msg = 0;

   trim(msg); CharToOem(msg, msg);

   color();
   printf((*msg)? "win32 error message: " : "win32 error code: ");
   color(CONSCLR_ERRCODE);
   if (*msg) printf("%s\n", msg); else printf("%04X\n", errcode);
   color();
}

void verr(const char *err, va_list args)
{
   color();
   printf("error: ");
   color(CONSCLR_ERROR);
   vprintf(err, args);
   color();
   printf("\n");
}

void errmsg(const char *err, ...)
{
   va_list args;
   va_start(args, err);
   verr(err, args);  
   va_end(args);
}

void __declspec(noreturn) errexit(const char *err, ...)
{
   va_list args;
   va_start(args, err);
   verr(err, args);  
   va_end(args);
   terminate();
}

extern "C" void * _ReturnAddress(void);

#ifndef __INTEL_COMPILER
#pragma intrinsic(_ReturnAddress)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_byteswap_ushort)
#endif
