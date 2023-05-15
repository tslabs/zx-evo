#include "std.h"

#include <Poco/Util/Application.h>
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"

#include "mods.h"
#include "emul.h"

#include "config.h"
#include "vars.h"
#include "dx/dx.h"
#include "draw.h"
#include "emulkeys.h"
#include "leds.h"
#include "mainloop.h"
#include "savesnd.h"
#include "snapshot.h"
#include "tape.h"
#include "emulator/ui/iehelp.h"
#include "util.h"
#include "visuals.h"
#include "emulator/debugger/dbgbpx.h"
#include "emulator/debugger/dbglabls.h"
#include "hard/ft812.h"
#include "hard/memory.h"
#include "hard/fdd/wd93dat.h"
#include "hard/gs/gs.h"

#define SND_TEST_FAILURES
//#define SND_TEST_SHOWSTAT

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (DWORD)-1
#endif

unsigned frametime = 111111; //Alone Coder (GUI value for conf.frame)


int nmi_pending = 0;


void m_nmi(rom_mode page);
void showhelp(const char* anchor)
{
	sound_stop(); //Alone Coder 0.36.6
	showhelppp(anchor); //Alone Coder 0.36.6
	sound_play(); //Alone Coder 0.36.6
}

LONG __stdcall filter(EXCEPTION_POINTERS* pp)
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

bool Exit = false;

bool ConfirmExit()
{
	if (!conf.ConfirmExit)
		return true;

	return MessageBox(wnd, "Exit ?", "Unreal", MB_YESNO | MB_ICONQUESTION | MB_SETFOREGROUND) == IDYES;
}

BOOL WINAPI ConsoleHandler(DWORD CtrlType)
{
	switch (CtrlType)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		if (ConfirmExit())
			Exit = true;
		return TRUE;
	}
	return FALSE;
}

class unreal_speccy_app final : public Poco::Util::Application
{
public:
	unreal_speccy_app() = default;

protected:

	void defineOptions(Poco::Util::OptionSet& options) override
	{
		Application::defineOptions(options);

		options.addOption(
			Poco::Util::Option("config", "c", "set config file")
			.required(false)
			.repeatable(false)
			.argument("file")
		);

		options.addOption(
			Poco::Util::Option("bp-list", "b", "define breackpoint list file")
			.required(false)
			.repeatable(false)
			.argument("file")
		);

		options.addOption(
			Poco::Util::Option("labels", "l", "define label list file")
			.required(false)
			.repeatable(false)
			.argument("file")
		);
	}

	void handleOption(const std::string& name, const std::string& value) override
	{
		Application::handleOption(name, value);

		if (name == "config")
			config_file_ = value;

		if (name == "bp-list")
			bpx_file_ = value;

		if (name == "labels")
			label_file_ = value;
	}

	void initialize(Application& app) override
	{
		Poco::AutoPtr p_channel = new Poco::ColorConsoleChannel;

		p_channel->setProperty("traceColor", "gray");
		p_channel->setProperty("debugColor", "brown");
		p_channel->setProperty("informationColor", "green");
		p_channel->setProperty("noticeColor", "blue");
		p_channel->setProperty("warningColor", "yellow");
		p_channel->setProperty("errorColor", "magenta");
		p_channel->setProperty("criticalColor", "lightRed");
		p_channel->setProperty("fatalColor", "red");


		const Poco::AutoPtr p_fc = new Poco::FormattingChannel(new Poco::PatternFormatter("%s: %p: %t"), p_channel);
		Poco::Logger::root().setChannel(p_fc);

		loadConfiguration();
		Application::initialize(app);
	}

	void uninitialize() override
	{
		Application::uninitialize();
		exit();
	}

//#include "emulator/emul_config.h"

	int main(const std::vector<std::string>& args) override
	{


		const DWORD Ver = GetVersion();

		WinVerMajor = (DWORD)(LOBYTE(LOWORD(Ver)));
		WinVerMinor = (DWORD)(HIBYTE(LOWORD(Ver)));

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		nowait = *(unsigned*)&csbi.dwCursorPosition;

		SetThreadAffinityMask(GetCurrentThread(), 1);

		SetConsoleCtrlHandler(ConsoleHandler, TRUE);

		color(CONSCLR_TITLE);
		printf("UnrealSpeccy by SMT and Others\nBuild date: %s, %s\n\n", __DATE__, __TIME__);
		color();

#ifndef DEBUG
		SetUnhandledExceptionFilter(filter);
#endif

		SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
		rand_ram();
		load_spec_colors();
		init_all();
		//   applyconfig();
		sound_play();
		color();

		SetForegroundWindow(wnd);

		//emul_config cfg{};

		//alt_load_config(cfg, "Unreal.ini");

		mainloop(Exit);

		return EXIT_OK;
	}

private:
	std::string config_file_{};
	std::string bpx_file_{};
	std::string label_file_{};

	bool init_error_ = false;


	static void cpu_info()
	{
		char idstr[64];
		idstr[0] = 0;

		fillCpuString(idstr);

		trim(idstr);

		const unsigned cpuver = cpuid(1, 0);
		const unsigned features = cpuid(1, 1);
		temp.mmx = (features >> 23) & 1;
		temp.sse = (features >> 25) & 1;
		temp.sse2 = (features >> 26) & 1;

		temp.cpufq = GetCPUFrequency();

		color(CONSCLR_HARDITEM); printf("cpu: ");

		color(CONSCLR_HARDINFO);
		printf("%s ", idstr);

		color(CONSCLR_HARDITEM);
		printf("%d.%d.%d [MMX:%s,SSE:%s,SSE2:%s] ",
			(cpuver >> 8) & 0x0F, (cpuver >> 4) & 0x0F, cpuver & 0x0F,
			temp.mmx ? "YES" : "NO",
			temp.sse ? "YES" : "NO",
			temp.sse2 ? "YES" : "NO");

		color(CONSCLR_HARDINFO);
		printf("at %d MHz\n", (unsigned)(temp.cpufq / 1000000));

#ifdef MOD_SSE2
		if (!temp.sse2) {
			color(CONSCLR_WARNING);
			printf("warning: this is an SSE2 build, recompile or download non-P4 version\n");
		}
#else //MOD_SSE2
		if (temp.sse2) {
			color(CONSCLR_WARNING);
			printf("warning: SSE2 disabled in compile-time, recompile or download P4 version\n");
		}
#endif
	}

	void init_all()
	{
		cpu_info();

		temp.Minimized = false;

		init_z80tables();
		init_ie_help();
		load_config(config_file_.c_str());
		//make_samples();
#ifdef MOD_GS
		init_gs();
#endif
		init_leds();
		init_tape();
		init_hdd_cd();
		start_dx();
		init_debug();
		init_visuals();
		applyconfig();
		main_reset();
		autoload();
		init_bpx(bpx_file_.c_str());
		init_labels(label_file_.c_str());
		temp.Gdiplus = gdiplus_startup();
		if (!temp.Gdiplus)
		{
			color(CONSCLR_WARNING);
			printf("warning: gdiplus.dll was not loaded, only SCR and BMP screenshots available\n");
		}

		if (comp.ts.vdac2)
		{
			const char* ver = nullptr;
			int rc = vdac2::open_ft8xx(&ver);

			if (rc == 0)
			{
				if (ver)
				{
					color(CONSCLR_HARDITEM);
					printf("FT library: ", ver);
					color(CONSCLR_HARDINFO);
					printf("%s\n", ver);
				}
			}
			else
			{
				color(CONSCLR_WARNING);
				printf("Warning: FT8xx emulator failed! (error: %d, %lX)\n", rc, GetLastError());
				comp.ts.vdac2 = false;
			}
		}

		load_errors = 0;
		trd_toload = 0;
		*(DWORD*)trd_loaded = 0; // clear loaded flags, don't see autoload'ed images

		/*for (; optind < argc; optind++)
		{
			char fname[0x200], * temp;
			GetFullPathName(argv[optind], sizeof fname, fname, &temp);

			trd_toload = default_drive; // auto-select
			if (!loadsnap(fname)) errmsg("error loading <%s>", argv[optind]), load_errors = 1;
		}
		*/

		if (load_errors) {
			const int code = MessageBox(wnd, "Some files, specified in\r\ncommand line, failed to load\r\n\r\nContinue emulation?", "File loading error", MB_YESNO | MB_ICONWARNING);
			if (code != IDYES)
			{
				init_error_ = true;
			}
		}

		SetCurrentDirectory(conf.workdir);
		//   timeBeginPeriod(1);

		InitializeCriticalSection(&tsu_toggle_cr);
	}

	static void exit()
	{
		//   EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_ENABLED);
		exitflag = 1;
		if (savesndtype)
			savesnddialog();
		if (videosaver_state)
			main_savevideo();  // stop saving video

		if (!normal_exit)
			done_fdd(false);
		done_tape();
		done_dx();
		done_gs();
		done_leds();
		save_nv();
		zf232.rs_close();
		zf232.zf_close();
		done_ie_help();
		done_bpx();
		gdiplus_shutdown();

		//   timeEndPeriod(1);
		if (ay[1].Chip2203) YM2203Shutdown(ay[1].Chip2203); //Dexus
		if (ay[0].Chip2203) YM2203Shutdown(ay[0].Chip2203); //Dexus
		if (comp.ts.vdac2) vdac2::close_ft8xx();

		color();
		printf("\nsee you later!\n");
		if (!nowait)
		{
			SetConsoleTitle("press a key...");
			FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
			getch();
		}
		fflush(stdout);
		SetConsoleCtrlHandler(ConsoleHandler, FALSE);
	}
};


POCO_APP_MAIN(unreal_speccy_app);