#include "std.h"
#include "emul.h"
#include "vars.h"
#include "config.h"
#include "dx/dx.h"
#include "emulator/ui/iehelp.h"
#include "hard/gs/gs.h"
#include "leds.h"
#include "tape.h"
#include "emulkeys.h"
#include "init.h"
#include "snapshot.h"
#include "savesnd.h"
#include "hard/fdd/wd93dat.h"
#include "debugger/dbgbpx.h"
#include "debugger/dbglabls.h"
#include "util.h"
#include "getopt.h"
#include "debugger/debug.h"
#include "visuals.h"
#include "hard/ft812.h"

void cpu_info()
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

void init_all(int argc, char** argv)
{
	cpu_info();

	const char * config = nullptr;
	char * bpx = nullptr, * labels = nullptr;

	char opt;
	while ((opt = getopt(argc, argv, "i:l:b:")) != EOF)
		switch (opt)
		{
		case 'i':
			config = optarg;
			break;
		case 'b':
			bpx = optarg;
			break;
		case 'l':
			labels = optarg;
		default:;
		}

	temp.Minimized = false;

	init_z80tables();
	init_ie_help();
	load_config(config);
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
	init_bpx(bpx);
	init_labels(labels);
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

	for (; optind < argc; optind++)
	{
		char fname[0x200], * temp;
		GetFullPathName(argv[optind], sizeof fname, fname, &temp);

		trd_toload = default_drive; // auto-select
		if (!loadsnap(fname)) errmsg("error loading <%s>", argv[optind]), load_errors = 1;
	}

	if (load_errors) {
		int code = MessageBox(wnd, "Some files, specified in\r\ncommand line, failed to load\r\n\r\nContinue emulation?", "File loading error", MB_YESNO | MB_ICONWARNING);
		if (code != IDYES) exit();
	}

	SetCurrentDirectory(conf.workdir);
	//   timeBeginPeriod(1);

	InitializeCriticalSection(&tsu_toggle_cr);
}

void __declspec(noreturn) exit()
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
	exit(0);
}
