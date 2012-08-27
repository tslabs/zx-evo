#include "std.h"
#include "resource.h"
#include "mods.h"
#include "emul_2203.h"
#include "sndrender.h"
#include "emul.h"
#include "sndchip.h"
#include "sndcounter.h"
#include "init.h"
#include "funcs.h"
#include "debug.h"
#include "vars.h"
#include "dx.h"
#include "draw.h"
#include "mainloop.h"
#include "iehelp.h"
#include "util.h"

#define SND_TEST_FAILURES
//#define SND_TEST_SHOWSTAT

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (DWORD)-1
#endif

typedef unsigned int UINT32;
typedef signed int INT32;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned char UINT8;
typedef signed char INT8;
unsigned frametime = 111111; //Alone Coder (GUI value for conf.frame)
//~------- Alone Coder ---------

int nmi_pending = 0;

bool ConfirmExit();
BOOL WINAPI ConsoleHandler(DWORD CtrlType);

void m_nmi(ROM_MODE page);
void showhelp(const char *anchor)
{
   sound_stop(); //Alone Coder 0.36.6
   showhelppp(anchor); //Alone Coder 0.36.6
   sound_play(); //Alone Coder 0.36.6
}

LONG __stdcall filter(EXCEPTION_POINTERS *pp)
{
   color(CONSCLR_ERROR);
   printf("\nexception %08X at eip=%p\n",
                pp->ExceptionRecord->ExceptionCode,
                pp->ExceptionRecord->ExceptionAddress);
#if _M_IX86
   printf("eax=%08X ebx=%08X ecx=%08X edx=%08X\n"
          "esi=%08X edi=%08X ebp=%08X esp=%08X\n",
          pp->ContextRecord->Eax, pp->ContextRecord->Ebx,
          pp->ContextRecord->Ecx, pp->ContextRecord->Edx,
          pp->ContextRecord->Esi, pp->ContextRecord->Edi,
          pp->ContextRecord->Ebp, pp->ContextRecord->Esp);
#endif
#if _M_IX64
   printf("rax=%08X rbx=%08X rcx=%08X rdx=%08X\n"
          "rsi=%08X rdi=%08X rbp=%08X rsp=%08X\n",
          pp->ContextRecord->Rax, pp->ContextRecord->Rbx,
          pp->ContextRecord->Rcx, pp->ContextRecord->Rdx,
          pp->ContextRecord->Rsi, pp->ContextRecord->Rdi,
          pp->ContextRecord->Rbp, pp->ContextRecord->Rsp);
#endif
   color();
   return EXCEPTION_CONTINUE_SEARCH;
}

static bool Exit = false;

bool ConfirmExit()
{
    if (!conf.ConfirmExit)
        return true;

    return MessageBox(wnd, "Exit ?", "Unreal", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND) == IDYES;
}

BOOL WINAPI ConsoleHandler(DWORD CtrlType)
{
    switch(CtrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        if (ConfirmExit())
            Exit = true;
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char **argv)
{

   DWORD Ver = GetVersion();

   WinVerMajor = (DWORD)(LOBYTE(LOWORD(Ver)));
   WinVerMinor = (DWORD)(HIBYTE(LOWORD(Ver)));

   CONSOLE_SCREEN_BUFFER_INFO csbi;
   GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
   nowait = *(unsigned*)&csbi.dwCursorPosition;

   SetThreadAffinityMask(GetCurrentThread(), 1);

   SetConsoleCtrlHandler(ConsoleHandler, TRUE);

   color(CONSCLR_TITLE);
   printf("UnrealSpeccy %s by SMT and Others, %s\n", VERS_STRING_, __DATE__);
   printf("Integrated YM2203 version 1.4A by Tatsuyuki Satoh, Jarek Burczynski, Alone Coder\n"); //Dexus
#ifdef __ICL
   printf("Intel C++ Compiler: %d.%02d\n", __ICL/100, __ICL % 100);
#endif
   color();
   // http://sourceforge.net/projects/unrealspeccy
   // http://alonecoder.narod.ru
   // http://dlcorp.ucoz.ru/forum/22
   // http://dlcorp.nedopc.com/viewforum.php?f=8
   // https://zx-evo-fpga.googlecode.com/svn/branches/tslabs/pentevo/unreal
   printf("new versions & sources ->\n");
   printf("   https://zx-evo-fpga.googlecode.com/svn/branches/tslabs/pentevo/unreal\n");

#ifndef DEBUG
   SetUnhandledExceptionFilter(filter);
#endif

   SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
   load_spec_colors();
   init_all(argc-1, argv+1);
//   applyconfig();
   sound_play();
   color();

   mainloop(Exit);
   return 0;
}
