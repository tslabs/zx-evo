#include "std.h"
#include "sysdefs.h"
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
#include "memory.h"

#define SND_TEST_FAILURES
//#define SND_TEST_SHOWSTAT

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (DWORD)-1
#endif

unsigned frametime = 111111; //Alone Coder (GUI value for conf.frame)

#ifdef LOG_FE_OUT
	FILE *f_log_FE_out;
#endif
#ifdef LOG_FE_IN
	FILE *f_log_FE_in;
#endif
#ifdef LOG_TAPE_IN
	FILE *f_log_tape_in;
#endif

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
   printf("UnrealSpeccy by SMT and Others\nBuild date: %s, %s\n\n", __DATE__, __TIME__);
#ifdef __ICL
   printf("Intel C++ Compiler: %d.%02d\n", __ICL/100, __ICL % 100);
#endif
   color();

#ifndef DEBUG
   SetUnhandledExceptionFilter(filter);
#endif

   SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
   rand_ram();
   load_spec_colors();
   init_all(argc-1, argv+1);
//   applyconfig();
   sound_play();
   color();

#ifdef LOG_FE_OUT
	f_log_FE_out = fopen("log_FE_out.txt", "wb");
	fprintf(f_log_FE_out, "CPU_tact\tFE_val\r\n");
#endif
#ifdef LOG_FE_IN
	f_log_FE_in = fopen("log_FE_in.txt", "wb");
	fprintf(f_log_FE_in, "CPU_tact\tFE_val\tA[15:8]\r\n");
#endif
#ifdef LOG_TAPE_IN
	f_log_tape_in = fopen("log_tape_in.txt", "wb");
	fprintf(f_log_tape_in, "CPU_tact\ttape_bit\r\n");
#endif

   mainloop(Exit);

#ifdef LOG_FE_OUT
   fclose(f_log_FE_out);
#endif
#ifdef LOG_FE_IN
   fclose(f_log_FE_in);
#endif
#ifdef LOG_TAPE
   fclose(f_log_tape_in);
#endif

   return 0;
}
