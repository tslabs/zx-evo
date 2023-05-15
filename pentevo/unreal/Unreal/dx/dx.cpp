#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "dx.h"
#include "draw.h"
#include "dxrend.h"
#include "dxrframe.h"
#include "dxerr.h"
#include "sound/sound.h"
#include "savesnd.h"
#include "emulkeys.h"
#include "leds.h"
#include "util.h"
// for menu
#include "snapshot.h"
#include "funcs.h"
#include "tape.h"

constexpr int size_x[3] = { 256, 320, 448 };
constexpr int size_y[3] = { 192, 240, 320 };

u8 active = 0, pause = 0;

static constexpr DWORD scu_scale1 = 0x10;
static constexpr DWORD scu_scale2 = 0x20;
static constexpr DWORD scu_scale3 = 0x30;
static constexpr DWORD scu_scale4 = 0x40;
static constexpr DWORD scu_lock_mouse = 0x50;

constexpr auto maxdspiece = (40000 * 4 / 20);
constexpr auto dsbuffer_sz = (2 * 44100 * 4);

constexpr auto sleep_delay = 2;

static HMODULE d3d_9dll = nullptr;
static IDirect3D9* d3d9 = nullptr;
static IDirect3DDevice9* d3d_dev = nullptr;
static IDirect3DSurface9* surf_texture = nullptr;

LPDIRECTDRAW2 dd;
LPDIRECTDRAWSURFACE sprim, surf0, surf1;

LPDIRECTINPUTDEVICE dikeyboard;
LPDIRECTINPUTDEVICE dimouse;
LPDIRECTINPUTDEVICE2 dijoyst;

LPDIRECTSOUND ds;
LPDIRECTSOUNDBUFFER dsbf;
LPDIRECTDRAWPALETTE pal;
LPDIRECTDRAWCLIPPER clip;

unsigned dsoffset, dsbuffer = dsbuffer_sz;

static HMENU main_menu;

/* ---------------------- renders ------------------------- */

RENDER renders[] =
{
   { "Normal",                    render_1x,      "normal",    RF_DRIVER | RF_1X },
   { "Double size",               render_2x,      "double",    RF_DRIVER | RF_2X },
   { "Double with scanlines",     render_2xs,     "dblscan",   RF_DRIVER | RF_2X },
   { "Triple size",               render_3x,      "triple",    RF_DRIVER | RF_3X },
   { "Quad size",                 render_4x,      "quad",      RF_DRIVER | RF_4X },
   { nullptr,nullptr,nullptr,0 }
};

size_t renders_count = _countof(renders);


BORDSIZE bordersizes[] =
{
   { "256x192 (ZX)", 140, 80, 256, 192 },
   { "320x240 (TS)", 108, 56, 320, 240 },
   { "360x288 (TS full)", 88, 32, 360, 288 },
   { "384x304 (AlCo)", 64, 16, 384, 304 },
   { "448x320 (Debug)", 0, 0, 448, 320 },
   { nullptr,0,0,0,0 }
};

size_t bordersizes_count = _countof(bordersizes);


const DRIVER drivers[] =
{
   { "gdi device context",   flip_gdi, "gdi",    RF_GDI },
   { "hardware blitter",     flip_blt, "blt",    RF_CLIP },
   { "hardware 3d",          flip_d3d, "d3d",    RF_D3D }
};

static void flip_gdi()
{
	if (needclr)
	{
		needclr--;
		gdi_frame();
	}
	renders[conf.render].func(gdibuf, temp.ox * temp.obpp / 8); // render to memory buffer
	showleds((u32*)gdibuf, temp.ox * temp.obpp / 8);

	// copy bitmap to gdi dc
	SetDIBitsToDevice(temp.gdidc, temp.gx, temp.gy, temp.ox, temp.oy, 0, 0, 0, temp.oy, gdibuf, &gdibmp.header, DIB_RGB_COLORS);
}

static void flip_blt()
{
	if (!surf1) return; // !!!
restore_lost:;
	DDSURFACEDESC desc;
	desc.dwSize = sizeof desc;
	HRESULT r = surf1->Lock(nullptr, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, nullptr);
	if (r != DD_OK)
	{
		if (r == DDERR_SURFACELOST)
		{
			surf1->Restore();
			if (surf1->IsLost() == DDERR_SURFACELOST)
				Sleep(1);
			goto restore_lost;
		}
		if (!active)
			return;
		printrdd("IDirectDrawSurface2::Lock() [buffer]", r);
		std::exit(-1);
	}

	renders[conf.render].func((u8*)desc.lpSurface, desc.lPitch);
	showleds((u32*)desc.lpSurface, desc.lPitch);

	surf1->Unlock(nullptr);

	assert(!IsRectEmpty(&temp.client));

	DDBLTFX fx;
	fx.dwSize = sizeof(fx);
	fx.dwDDFX = 0;

	r = surf0->Blt(&temp.client, surf1, nullptr, DDBLT_WAIT | DDBLT_DDFX, &fx);
	if (r != DD_OK)
	{
		if (r == DDERR_SURFACELOST)
		{
			surf0->Restore();
			surf1->Restore();
			if (surf0->IsLost() == DDERR_SURFACELOST || surf1->IsLost() == DDERR_SURFACELOST)
				Sleep(1);
			goto restore_lost;
		}
		printf("rect = %d, %d, %d, %d\n", temp.client.left, temp.client.top, temp.client.right, temp.client.bottom);
		printrdd("IDirectDrawSurface2::Blt()", r);
		std::exit(-1);
	}
}

static void flip_d3d()
{
	if (!d3d_dev) return; // !!!
	if (!SUCCEEDED(d3d_dev->BeginScene()))
		return;

	IDirect3DSurface9* surf_back_buffer0 = nullptr, * surf_back_buffer1 = nullptr;

	HRESULT hr = d3d_dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf_back_buffer0);
	if (hr != DD_OK)
	{
		printrdd("IDirect3DDevice9::GetBackBuffer(0)", hr); std::exit(-1);
	}

	hr = d3d_dev->GetBackBuffer(0, 1, D3DBACKBUFFER_TYPE_MONO, &surf_back_buffer1);
	if (hr != DD_OK)
	{
		printrdd("IDirect3DDevice9::GetBackBuffer(1)", hr); std::exit(-1);
	}

	D3DLOCKED_RECT rect = { 0 };

	hr = surf_texture->LockRect(&rect, nullptr, D3DLOCK_DISCARD);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	renders[conf.render].func((u8*)rect.pBits, rect.Pitch);
	showleds((u32*)rect.pBits, rect.Pitch);

	surf_texture->UnlockRect();

	hr = d3d_dev->StretchRect(surf_texture, nullptr, surf_back_buffer0, nullptr, D3DTEXF_POINT);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	hr = d3d_dev->ColorFill(surf_back_buffer1, nullptr, D3DCOLOR_XRGB(0, 0, 0));
	if (FAILED(hr))
	{
		__debugbreak();
	}

	/*
		Hr = D3dDev->StretchRect(SurfTexture, 0, SurfBackBuffer1, 0, D3DTEXF_POINT);
		if (FAILED(Hr))
		{
			__debugbreak();
		}
	*/
	d3d_dev->EndScene();

	// Present the backbuffer contents to the display
	hr = d3d_dev->Present(nullptr, nullptr, nullptr, nullptr);
	if (FAILED(hr))
	{
		__debugbreak();
	}

	surf_back_buffer0->Release();
	surf_back_buffer1->Release();
}

void flip()
{
	if (temp.Minimized)
		return;

	if (conf.flip && (temp.rflags & (RF_GDI | RF_CLIP)))
		dd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, nullptr);

	if (temp.rflags & RF_GDI) // gdi
	{
		flip_gdi();
		return;
	}

	if (surf0 && surf0->IsLost() == DDERR_SURFACELOST)
		surf0->Restore();
	if (surf1 && surf1 && surf1->IsLost() == DDERR_SURFACELOST)
		surf1->Restore();

	if (temp.rflags & RF_CLIP) // hardware blitter
	{
		if (IsRectEmpty(&temp.client)) // client area is empty
			return;

		flip_blt();
		return;
	}

	if (temp.rflags & RF_D3D) // direct 3d
	{
		if (IsRectEmpty(&temp.client)) // client area is empty
			return;

		flip_d3d();
		return;
	}

	// draw direct to video memory, overlay
 //   FlipVideoMem();
}

HWAVEOUT hwo = nullptr;
WAVEHDR wq[MAXWQSIZE];
u8 wbuffer[MAXWQSIZE * maxdspiece];
unsigned wqhead, wqtail;

void do_sound_none()
{
}

void do_sound_wave()
{
	HRESULT r;

	for (;;) // release all done items
	{
		while ((wqtail != wqhead) && (wq[wqtail].dwFlags & WHDR_DONE))
		{ // ready!
			if ((r = waveOutUnprepareHeader(hwo, &wq[wqtail], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
			{
				printrmm("waveOutUnprepareHeader()", r);
				std::exit(-1);
			}
			if (++wqtail == conf.soundbuffer)
				wqtail = 0;
		}
		if ((wqhead + 1) % conf.soundbuffer != wqtail)
			break; // some empty item in queue
		/* [vv]
		*/
		if (conf.sleepidle)
			Sleep(sleep_delay);
	}

	if (!spbsize)
		return;

	//   __debugbreak();
	   // put new item and play
	const auto bfpos = PCHAR(wbuffer + wqhead * maxdspiece);
	memcpy(bfpos, sndplaybuf, spbsize);
	wq[wqhead].lpData = bfpos;
	wq[wqhead].dwBufferLength = spbsize;
	wq[wqhead].dwFlags = 0;

	if ((r = waveOutPrepareHeader(hwo, &wq[wqhead], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
	{
		printrmm("waveOutPrepareHeader()", r);
		terminate();
	}

	if ((r = waveOutWrite(hwo, &wq[wqhead], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
	{
		printrmm("waveOutWrite()", r);
		terminate();
	}

	if (++wqhead == conf.soundbuffer)
		wqhead = 0;

	//  int bs = wqhead-wqtail; if (bs<0)bs+=conf.soundbuffer; if (bs < 8) goto again;

}

// directsound part
// begin
void restore_sound_buffer()
{
	//   for (;;) {
	DWORD status = 0;
	dsbf->GetStatus(&status);
	if (status & DSBSTATUS_BUFFERLOST)
	{
		printf("%s\n", __FUNCTION__);
		Sleep(18);
		dsbf->Restore();
		// Sleep(200);
	}
	//      else break;
	//   }
}

void clear_buffer()
{
	//   printf("%s\n", __FUNCTION__);
	//   __debugbreak();

	restore_sound_buffer();
	HRESULT r;
	void* ptr1, * ptr2;
	DWORD sz1, sz2;
	r = dsbf->Lock(0, 0, &ptr1, &sz1, &ptr2, &sz2, DSBLOCK_ENTIREBUFFER);
	if (r != DS_OK)
		return;
	memset(ptr1, 0, sz1);
	//memset(ptr2, 0, sz2);
	dsbf->Unlock(ptr1, sz1, ptr2, sz2);
}

int maxgap;

void do_sound_ds()
{
	HRESULT r;

	for (;;)
	{
		int play, write;
		if ((r = dsbf->GetCurrentPosition((DWORD*)&play, (DWORD*)&write)) != DS_OK)
		{
			if (r == DSERR_BUFFERLOST)
			{
				restore_sound_buffer();
				return;
			}

			printrds("IDirectSoundBuffer::GetCurrentPosition()", r);
			terminate();
		}

		int gap = write - play;
		if (gap < 0)
			gap += dsbuffer;

		int pos = dsoffset - play;
		if (pos < 0)
			pos += dsbuffer;
		maxgap = max(maxgap, gap);

		if (pos < maxgap || pos > 10 * (int)maxgap)
		{
			dsoffset = 3 * maxgap;
			clear_buffer();
			dsbf->Stop();
			dsbf->SetCurrentPosition(0);
			break;
		}

		if (pos < 2 * maxgap)
			break;

		if ((r = dsbf->Play(0, 0, DSBPLAY_LOOPING)) != DS_OK)
		{
			printrds("IDirectSoundBuffer::Play()", r);
			terminate();
		}

		if (conf.sleepidle)
			Sleep(sleep_delay);
	}

	/*
	   if (dsoffset >= dsbuffer)
		   dsoffset -= dsbuffer;
	*/
	dsoffset %= dsbuffer;

	if (!spbsize)
		return;

	void* ptr1, * ptr2;
	DWORD sz1, sz2;
	r = dsbf->Lock(dsoffset, spbsize, &ptr1, &sz1, &ptr2, &sz2, 0);
	if (r != DS_OK)
	{
		//       __debugbreak();
		printf("dsbuffer=%d, dsoffset=%d, spbsize=%d\n", dsbuffer, dsoffset, spbsize);
		printrds("IDirectSoundBuffer::Lock()", r);
		terminate();
	}
	memcpy(ptr1, sndplaybuf, sz1);
	if (ptr2)
		memcpy(ptr2, ((char*)sndplaybuf) + sz1, sz2);

	dsbf->Unlock(ptr1, sz1, ptr2, sz2);

	dsoffset += spbsize;
	dsoffset %= dsbuffer;
	/*
	   if (dsoffset >= dsbuffer)
		   dsoffset -= dsbuffer;
	*/
	/*
	   if ((r = dsbf->Play(0, 0, DSBPLAY_LOOPING)) != DS_OK)
	   {
		   printrds("IDirectSoundBuffer::Play()", r);
		   terminate();
	   }
	*/
}
// directsound part
// end


void sound_play()
{
	//   printf("%s\n", __FUNCTION__);
	maxgap = 2000;
	restart_sound();
}


void sound_stop()
{
	//   printf("%s\n", __FUNCTION__);
	//   __debugbreak();
	if (dsbf)
	{
		dsbf->Stop(); // don't check
		clear_buffer();
	}
}

void do_sound()
{
	if (savesndtype == 1)
		if (fwrite(sndplaybuf, 1, spbsize, savesnd) != spbsize)
			savesnddialog(); // write error - disk full - close file

	conf.sound.do_sound();
}

void set_priority()
{
	if (!conf.highpriority || !conf.sleepidle) return;
	SetPriorityClass(GetCurrentProcess(), conf.sound.enabled ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}

void adjust_mouse_cursor()
{
	const unsigned showcurs = conf.input.joymouse || !active || !(conf.fullscr || conf.lockmouse) || dbgbreak;
	while (ShowCursor(0) >= 0); // hide cursor
	if (showcurs) while (ShowCursor(1) <= 0); // show cursor
	if (active && conf.lockmouse && !dbgbreak)
	{
		RECT rc; GetClientRect(wnd, &rc);
		POINT p = { rc.left, rc.top };
		ClientToScreen(wnd, &p); rc.left = p.x, rc.top = p.y;
		p.x = rc.right, p.y = rc.bottom;
		ClientToScreen(wnd, &p); rc.right = p.x, rc.bottom = p.y;
		ClipCursor(&rc);
	}
	else ClipCursor(nullptr);
}

HCURSOR crs[9];
u8 mousedirs[9] = { 10, 8, 9, 2, 0, 1, 6, 4, 5 };

void updatebitmap()
{
	RECT rc;
	GetClientRect(wnd, &rc);
	const DWORD newdx = rc.right - rc.left;
	const DWORD newdy = rc.bottom - rc.top;
	if (hbm && (bm_dx != newdx || bm_dy != newdy))
	{
		DeleteObject(hbm);
		hbm = nullptr;
	}
	if (sprim)
		return; // don't trace window contents in overlay mode

	if (hbm)
	{
		DeleteObject(hbm);
		hbm = nullptr; // keeping bitmap is unsafe - screen paramaters may change
	}

	if (!hbm)
		hbm = CreateCompatibleBitmap(temp.gdidc, newdx, newdy);

	HDC dc = CreateCompatibleDC(temp.gdidc);

	bm_dx = newdx; bm_dy = newdy;
	const HGDIOBJ prev_obj = SelectObject(dc, hbm);
	//SetDIBColorTable(dc, 0, 0x100, (RGBQUAD*)pal0);
	BitBlt(dc, 0, 0, newdx, newdy, temp.gdidc, 0, 0, SRCCOPY);
	SelectObject(dc, prev_obj);
	DeleteDC(dc);
}

static void start_d3d(HWND wnd);
static void done_d3d(bool de_init_dll = true);
static void set_video_mode_d3d();

static INT_PTR CALLBACK wnd_proc(const HWND hwnd, UINT u_message, const WPARAM wparam, const LPARAM lparam)
{
	static bool moving = false;
	// printf("WM_x = %04X\n", uMessage);

	if ((u_message == WM_QUIT) || (u_message == WM_NCDESTROY))
	{
		//       __debugbreak();
		//       printf("WM_QUIT\n");
		terminate();
	}

	if (u_message == WM_CLOSE)
	{
		//       __debugbreak();
		//       printf("WM_CLOSE\n");
		return 1;
	}

	if (u_message == WM_SETFOCUS && !pause)
	{
		active = 1; setpal(0);
		//      sound_play();
		adjust_mouse_cursor();
		input.nokb = 20;
		u_message = WM_USER;
	}

	if (u_message == WM_KILLFOCUS && !pause)
	{
		if (dd)
			dd->FlipToGDISurface();
		updatebitmap();
		setpal(1);
		active = 0;
		//      sound_stop();
		conf.lockmouse = 0;
		adjust_mouse_cursor();
	}

	if (conf.input.joymouse)
	{
		if (u_message == WM_LBUTTONDOWN || u_message == WM_LBUTTONUP)
		{
			input.mousejoy = (input.mousejoy & 0x0F) | (u_message == WM_LBUTTONDOWN ? 0x10 : 0);
			input.kjoy = (input.kjoy & 0x0F) | (u_message == WM_LBUTTONDOWN ? 0x10 : 0);
		}

		if (u_message == WM_MOUSEMOVE)
		{
			RECT rc; GetClientRect(hwnd, &rc);
			const unsigned xx = LOWORD(lparam) * 3 / (rc.right - rc.left);
			const unsigned yy = HIWORD(lparam) * 3 / (rc.bottom - rc.top);
			const unsigned nn = yy * 3 + xx;
			//         SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG)crs[nn]); //Alone Coder
			SetCursor(crs[nn]); //Alone Coder
			input.mousejoy = (input.mousejoy & 0x10) | mousedirs[nn];
			input.kjoy = (input.kjoy & 0x10) | mousedirs[nn];
			return 0;
		}
	}
	else if (u_message == WM_LBUTTONDOWN && !conf.lockmouse)
	{
		//       printf("%s\n", __FUNCTION__);
		input.nomouse = 20;
		main_mouse();
	}

	if (u_message == WM_ENTERSIZEMOVE)
	{
		sound_stop();
	}

	if (u_message == WM_EXITSIZEMOVE)
	{
		sound_play();
	}

	if (u_message == WM_SIZE || u_message == WM_MOVE || u_message == WM_USER)
	{
		GetClientRect(wnd, &temp.client);
		temp.gdx = temp.client.right - temp.client.left;
		temp.gdy = temp.client.bottom - temp.client.top;
		temp.gx = (temp.gdx > temp.ox) ? (temp.gdx - temp.ox) / 2 : 0;
		temp.gy = (temp.gdy > temp.oy) ? (temp.gdy - temp.oy) / 2 : 0;
		ClientToScreen(wnd, reinterpret_cast<POINT*>(&temp.client.left));
		ClientToScreen(wnd, reinterpret_cast<POINT*>(&temp.client.right));
		adjust_mouse_cursor();
		if (sprim)
			u_message = WM_PAINT;
		needclr = 2;
		if ((temp.rflags & RF_D3D) && dd && u_message == WM_SIZE && temp.ox && temp.oy)
		{
			done_d3d(false);
			set_video_mode_d3d();
		}
	}

	if (u_message == WM_PAINT)
	{
		if (sprim)
		{ // background for overlay
			RECT rc;
			GetClientRect(hwnd, &rc);
			const HBRUSH br = CreateSolidBrush(RGB(0xFF, 0x00, 0xFF));
			FillRect(temp.gdidc, &rc, br);
			DeleteObject(br);
		}
		else if (hbm /*&& !active*/)
		{
			//printf("%s, WM_PAINT\n", __FUNCTION__);
			/*HDC hcom = CreateCompatibleDC(temp.gdidc);
			HGDIOBJ PrevObj = SelectObject(hcom, hbm);
			BitBlt(temp.gdidc, 0, 0, bm_dx, bm_dy, hcom, 0, 0, SRCCOPY);
			SelectObject(hcom, PrevObj);
			DeleteDC(hcom);*/
			drivers[conf.driver].func();
		}
	}

	if (u_message == WM_SYSCOMMAND)
	{
		//       printf("%s, WM_SYSCOMMAND 0x%04X\n", __FUNCTION__, (ULONG)wparam);

		switch (wparam & 0xFFF0)
		{
		case scu_scale1: temp.scale = 1; wnd_resize(1);  return 0;
		case scu_scale2: temp.scale = 2; wnd_resize(2);  return 0;
		case scu_scale3: temp.scale = 3; wnd_resize(3);  return 0;
		case scu_scale4: temp.scale = 4; wnd_resize(4);  return 0;
		case scu_lock_mouse: main_mouse();  return 0;
		case SC_CLOSE:
			if (ConfirmExit())
				correct_exit();
			return 0;
		case SC_MINIMIZE: temp.Minimized = true; break;

		case SC_RESTORE: temp.Minimized = false; break;
		}
	}

	if (u_message == WM_DROPFILES)
	{
		const auto h_drop = reinterpret_cast<HDROP>(wparam);
		DragQueryFile(h_drop, 0, droppedFile, sizeof(droppedFile));
		DragFinish(h_drop);
		return 0;
	}

	if (u_message == WM_COMMAND)
	{
		int disk = -1;
		switch (wparam) {
			// File menu
		case IDM_EXIT: correct_exit();

		case IDM_LOAD: opensnap(); break;

		case IDM_SAVE: savesnap(); break;
		case IDM_SAVE_DISKB: disk = 1; goto save_disk;
		case IDM_SAVE_DISKC: disk = 2; goto save_disk;
		case IDM_SAVE_DISKD: disk = 3; goto save_disk;
		save_disk:
			sound_stop();
			savesnap(disk);
			eat();
			sound_play();
			break;

		case IDM_QUICKLOAD_1: qload1(); break;
		case IDM_QUICKLOAD_2: qload2(); break;
		case IDM_QUICKLOAD_3: qload3(); break;
		case IDM_QUICKSAVE_1: qsave1(); break;
		case IDM_QUICKSAVE_2: qsave2(); break;
		case IDM_QUICKSAVE_3: qsave3(); break;

		case IDM_RESET: main_reset(); break;
		case IDM_RESET_128: main_reset128(); break;
		case IDM_RESET_48: main_resetbas(); break;
		case IDM_RESET_DOS: main_resetdos(); break;
		case IDM_RESET_SERVICE: main_resetsys(); break;
		case IDM_RESET_CACHE: main_resetcache(); break;
		case IDM_RESET_GS: reset_gs(); break;

		case IDM_NMI: main_nmi(); break;
		case IDM_NMI_DOS: main_nmidos(); break;
		case IDM_NMI_CACHE: main_nmicache(); break;

		case IDM_POKE: main_poke(); break;

		case IDM_AUDIOREC: savesnddialog(); break;
		case IDM_MAKESCREENSHOT: main_scrshot(); break;

		case IDM_SETTINGS: setup_dlg(); break;
		case IDM_VIDEOFILTER: main_selectfilter(); break;
		case IDM_FULLSCREEN: main_fullscr(); break;

		case IDM_MAXIMUMSPEED: main_maxspeed(); break;

		case IDM_TAPE_CONTROL: main_starttape(); break;
		case IDM_USETAPETRAPS: conf.tape_traps ^= 1; break;
		case IDM_AUTOSTARTTAPE: conf.tape_autostart ^= 1; break;

			// Debugger
		case IDM_DEBUGGER: main_debug(); break;

			// Help
		case IDM_HELP_SHORTKEYS: main_help(); break;
			//case IDM_HELP_ABOUT: DialogBox(hIn, MAKEINTRESOURCE(IDD_ABOUT), wnd, aboutdlg); break;
		}
		//needclr=1;
	}
	if (u_message == WM_INITMENU)
	{
		if (wparam == reinterpret_cast<WPARAM>(main_menu))
		{
			sound_stop();
			ModifyMenu(main_menu, IDM_SAVE_DISKB,
				(comp.wd.fdd[1].rawdata ? MF_ENABLED : MF_GRAYED | MF_DISABLED), IDM_SAVE_DISKB, "Disk B");
			ModifyMenu(main_menu, IDM_SAVE_DISKC,
				(comp.wd.fdd[2].rawdata ? MF_ENABLED : MF_GRAYED | MF_DISABLED), IDM_SAVE_DISKC, "Disk C");
			ModifyMenu(main_menu, IDM_SAVE_DISKD,
				(comp.wd.fdd[3].rawdata ? MF_ENABLED : MF_GRAYED | MF_DISABLED), IDM_SAVE_DISKD, "Disk D");

			ModifyMenu(main_menu, IDM_AUDIOREC, MF_BYCOMMAND, IDM_AUDIOREC, savesndtype ? "Stop audio recording" : "Start audio recording");

			ModifyMenu(main_menu, IDM_MAXIMUMSPEED,
				(conf.sound.enabled ? MF_UNCHECKED : MF_CHECKED), IDM_MAXIMUMSPEED, "Maximum speed");

			ModifyMenu(main_menu, IDM_TAPE_CONTROL,
				(tape_infosize ? MF_ENABLED : MF_GRAYED | MF_DISABLED), IDM_TAPE_CONTROL, comp.tape.play_pointer ? "Stop tape" : "Start tape");
			ModifyMenu(main_menu, IDM_USETAPETRAPS,
				(conf.tape_traps ? MF_CHECKED : MF_UNCHECKED), IDM_USETAPETRAPS, "Use tape traps");
			ModifyMenu(main_menu, IDM_AUTOSTARTTAPE,
				(conf.tape_autostart ? MF_CHECKED : MF_UNCHECKED), IDM_AUTOSTARTTAPE, "Autostart tape");
		}
	}

	return DefWindowProc(hwnd, u_message, wparam, lparam);
}

void readdevice(VOID* md, const DWORD sz, const LPDIRECTINPUTDEVICE dev)
{
	if (!active || !dev)
		return;
	HRESULT r = dev->GetDeviceState(sz, md);
	if (r == DIERR_INPUTLOST || r == DIERR_NOTACQUIRED)
	{
		r = dev->Acquire();
		while (r == DIERR_INPUTLOST)
			r = dev->Acquire();

		if (r == DIERR_OTHERAPPHASPRIO) // Приложение находится в background
			return;

		if (r != DI_OK)
		{
			printrdi("IDirectInputDevice::Acquire()", r);
			terminate();
		}
		r = dev->GetDeviceState(sz, md);
	}
	if (r != DI_OK)
	{
		printrdi("IDirectInputDevice::GetDeviceState()", r);
		terminate();
	}
}

void readmouse(DIMOUSESTATE* md)
{
	memset(md, 0, sizeof * md);
	readdevice(md, sizeof * md, dimouse);
}

void read_keyboard(const PVOID kbd_data)
{
	readdevice(kbd_data, 256, dikeyboard);
}

void setpal(const char system)
{
	if (!active || !dd || !surf0 || !pal) return;
	HRESULT r;
	if (surf0->IsLost() == DDERR_SURFACELOST) surf0->Restore();
	if ((r = pal->SetEntries(0, 0, 0x100, system ? syspalette : pal0)) != DD_OK)
	{
		printrdd("IDirectDrawPalette::SetEntries()", r); terminate();
	}
}

void trim_right(char* str)
{
	unsigned i; //Alone Coder 0.36.7
	for (/*unsigned*/ i = strlen(str); i && str[i - 1] == ' '; i--);
	str[i] = 0;
}

#define MAX_MODES 512
struct modeparam {
	unsigned x, y, b, f;
} modes[MAX_MODES];
unsigned max_modes;

static void set_video_mode_d3d()
{
	if (!d3d_dev)
		start_d3d(wnd);

	if (!d3d_dev)
		return;

	IDirect3DTexture9* texture = nullptr;
	D3DDISPLAYMODE disp_mode;
	HRESULT r = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &disp_mode);
	if (r != DD_OK)
	{
		printrdd("IDirect3D::GetAdapterDisplayMode()", r); terminate();
	}
	r = d3d_dev->CreateTexture(temp.ox, temp.oy, 1, D3DUSAGE_DYNAMIC, disp_mode.Format, D3DPOOL_DEFAULT, &texture, nullptr);
	if (r != DD_OK)
	{
		printrdd("IDirect3DDevice9::CreateTexture()", r); terminate();
	}
	r = texture->GetSurfaceLevel(0, &surf_texture);
	if (r != DD_OK)
	{
		printrdd("IDirect3DTexture::GetSurfaceLevel()", r); terminate();
	}
	if (!surf_texture)
		__debugbreak();
	if (texture)
		texture->Release();
}

void set_vidmode()
{
	if (pal)
	{
		pal->Release();
		pal = nullptr;
	}

	if (surf1)
	{
		surf1->Release();
		surf1 = nullptr;
	}

	if (surf0)
	{
		surf0->Release();
		surf0 = nullptr;
	}

	if (sprim)
	{
		sprim->Release();
		sprim = nullptr;
	}

	if (clip)
	{
		clip->Release();
		clip = nullptr;
	}

	if (surf_texture)
	{
		surf_texture->Release();
		surf_texture = nullptr;
	}

	HRESULT r;

	DDSURFACEDESC desc;
	desc.dwSize = sizeof desc;
	r = dd->GetDisplayMode(&desc);
	if (r != DD_OK) { printrdd("IDirectDraw2::GetDisplayMode()", r); terminate(); }
	temp.ofq = desc.dwRefreshRate; // nt only?
	if (!temp.ofq)
		temp.ofq = conf.refresh;

	// select fullscreen, set window style
	if (temp.rflags & RF_DRIVER)
		temp.rflags |= drivers[conf.driver].flags;
	if (!(temp.rflags & (RF_GDI | RF_OVR | RF_CLIP | RF_D3D)))
		conf.fullscr = 1;
	if ((temp.rflags & RF_32) && desc.ddpfPixelFormat.dwRGBBitCount != 32)
		conf.fullscr = 1; // for chunks via blitter

	static RECT rc;
	DWORD oldstyle = GetWindowLong(wnd, GWL_STYLE);
	if (oldstyle & WS_CAPTION)
		GetWindowRect(wnd, &rc);

	unsigned style = conf.fullscr ? WS_VISIBLE | WS_POPUP : WS_VISIBLE | WS_OVERLAPPEDWINDOW;
	if ((oldstyle ^ style) & WS_CAPTION)
		SetWindowLong(wnd, GWL_STYLE, style);

	// select exclusive
	char excl = conf.fullscr;
	if ((temp.rflags & RF_CLIP) && (desc.ddpfPixelFormat.dwRGBBitCount == 8))
		excl = 1;

	if (!(temp.rflags & RF_MON))
	{
		r = dd->SetCooperativeLevel(wnd, excl ? DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN : DDSCL_ALLOWREBOOT | DDSCL_NORMAL);
		if (r != DD_OK) { printrdd("IDirectDraw2::SetCooperativeLevel()", r); terminate(); }
	}

	// select resolution
	temp.ox = temp.scx = conf.framexsize;
	temp.oy = temp.scy = conf.frameysize;
	// temp.ox = temp.scx = 448;
	// temp.oy = temp.scy = 320;

	if (temp.rflags & RF_2X)
	{
		temp.ox *= 2; temp.oy *= 2;
		if (conf.fast_sl && (temp.rflags & RF_DRIVER) && (temp.rflags & (RF_CLIP | RF_OVR)))
			temp.oy /= 2;
	}

	if (temp.rflags & RF_3X) temp.ox *= 3, temp.oy *= 3;
	if (temp.rflags & RF_4X) temp.ox *= 4, temp.oy *= 4;
	if (temp.rflags & RF_64x48) temp.ox = 64, temp.oy = 48;
	if (temp.rflags & RF_128x96) temp.ox = 128, temp.oy = 96;

	//   printf("temp.ox=%d, temp.oy=%d\n", temp.ox, temp.oy);

	   // select color depth
	temp.obpp = 32;
	if (temp.rflags & (RF_CLIP | RF_D3D))
		temp.obpp = desc.ddpfPixelFormat.dwRGBBitCount;
	if (temp.rflags & (RF_16 | RF_OVR))
		temp.obpp = 16;
	if (temp.rflags & RF_32)
		temp.obpp = 32;
	if ((temp.rflags & (RF_GDI | RF_8BPCH)) == (RF_GDI | RF_8BPCH))
		temp.obpp = 32;

	if (conf.fullscr || ((temp.rflags & RF_MON) && desc.dwHeight < 480))
	{
		// select minimal screen mode
		unsigned newx = 100000, newy = 100000, newfq = conf.refresh, newb = temp.obpp;
		unsigned minx = temp.ox, miny = temp.oy, needb = temp.obpp;

		if (temp.rflags & (RF_64x48 | RF_128x96))
		{
			needb = (temp.rflags & RF_16) ? 16 : 32;
			minx = desc.dwWidth; if (minx < 640) minx = 640;
			miny = desc.dwHeight; if (miny < 480) miny = 480;
		}
		// if (temp.rflags & RF_MON) // - ox=640, oy=480 - set above

		for (unsigned i = 0; i < max_modes; i++)
		{
			if (modes[i].y < miny || modes[i].x < minx)
				continue;
			if (!(temp.rflags & RF_MON) && modes[i].b != temp.obpp)
				continue;
			if (modes[i].y < conf.minres)
				continue;

			if (modes[i].x < newx || modes[i].y < newy)
			{
				newx = modes[i].x, newy = modes[i].y;
				if (!conf.refresh && modes[i].f > newfq)
					newfq = modes[i].f;
			}
		}

		if (newx == 100000)
		{
			color(CONSCLR_ERROR);
			printf("can't find situable mode for %d x %d * %d bits\n", temp.ox, temp.oy, temp.obpp);
			terminate();
		}

		// use minimal or current mode
		if (temp.rflags & (RF_OVR | RF_GDI | RF_CLIP))
		{
			// leave screen size, if enough width/height
			newx = desc.dwWidth, newy = desc.dwHeight;
			if (newx < minx || newy < miny)
			{
				newx = minx;
				newy = miny;
			}
			// leave color depth, until specified directly
			if (!(temp.rflags & (RF_16 | RF_32)))
				newb = desc.ddpfPixelFormat.dwRGBBitCount;
		}

		if (desc.dwWidth != newx || desc.dwHeight != newy || desc.ddpfPixelFormat.dwRGBBitCount != newb)
		{
			if ((r = dd->SetDisplayMode(newx, newy, newb, newfq, 0)) != DD_OK)
			{
				printrdd("IDirectDraw2::SetDisplayMode()", r); terminate();
			}
			GetSystemPaletteEntries(temp.gdidc, 0, 0x100, syspalette);
			if (newfq)
				temp.ofq = newfq;
		}
		temp.odx = temp.obpp * (newx - temp.ox) / 16, temp.ody = (newy - temp.oy) / 2;
		temp.rsx = newx, temp.rsy = newy;
		ShowWindow(wnd, SW_SHOWMAXIMIZED);
	}
	else
	{
		// restore window position to last saved position in non-fullscreen mode
		SetMenu(wnd, main_menu);
		ShowWindow(wnd, SW_SHOWNORMAL);
		if (temp.rflags & RF_GDI)
		{
			RECT client = { 0,0, temp.ox, temp.oy };
			AdjustWindowRect(&client, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0);
			rc.right = rc.left + (client.right - client.left);
			rc.bottom = rc.top + (client.bottom - client.top);
		}
		MoveWindow(wnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 1);
	}

	dd->FlipToGDISurface(); // don't check result

	desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	desc.dwFlags = DDSD_CAPS;

	DWORD pl[0x101]; pl[0] = 0x01000300; memcpy(pl + 1, pal0, 0x400);
	HPALETTE hpal = CreatePalette((LOGPALETTE*)&pl);
	DeleteObject(SelectPalette(temp.gdidc, hpal, 0));
	RealizePalette(temp.gdidc); // for RF_GDI and for bitmap, used in WM_PAINT

	if (temp.rflags & RF_GDI)
	{

		gdibmp.header.bmiHeader.biWidth = temp.ox;
		gdibmp.header.bmiHeader.biHeight = -(int)temp.oy;
		gdibmp.header.bmiHeader.biBitCount = temp.obpp;

	}
	else if (temp.rflags & RF_OVR)
	{

		temp.odx = temp.ody = 0;
		if ((r = dd->CreateSurface(&desc, &sprim, nullptr)) != DD_OK)
		{
			printrdd("IDirectDraw2::CreateSurface() [primary,test]", r); terminate();
		}

		desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
		desc.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
		desc.dwWidth = temp.ox, desc.dwHeight = temp.oy;

		// conf.flip = 0; // overlay always synchronized without Flip()! on radeon videocards
						  // double flip causes fps drop

		if (conf.flip)
		{
			desc.dwBackBufferCount = 1;
			desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
			desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		}

		static DDPIXELFORMAT ddpf_overlay_format16 = { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0xF800}, {0x07E0}, {0x001F}, {0} };
		static DDPIXELFORMAT ddpf_overlay_format15 = { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x7C00}, {0x03E0}, {0x001F}, {0} };
		static DDPIXELFORMAT ddpf_overlay_format_yuy2 = { sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','Y','2'), {0},{0},{0},{0},{0} };

		if (temp.rflags & RF_YUY2)
			goto YUY2;

		temp.hi15 = 0;
		desc.ddpfPixelFormat = ddpf_overlay_format16;
		r = dd->CreateSurface(&desc, &surf0, nullptr);

		if (r == DDERR_INVALIDPIXELFORMAT)
		{
			temp.hi15 = 1;
			desc.ddpfPixelFormat = ddpf_overlay_format15;
			r = dd->CreateSurface(&desc, &surf0, nullptr);
		}

		if (r == DDERR_INVALIDPIXELFORMAT /*&& !(temp.rflags & RF_RGB)*/)
		{
		YUY2:
			temp.hi15 = 2;
			desc.ddpfPixelFormat = ddpf_overlay_format_yuy2;
			r = dd->CreateSurface(&desc, &surf0, nullptr);
		}

		if (r != DD_OK)
		{
			printrdd("IDirectDraw2::CreateSurface() [overlay]", r); terminate();
		}

	}
	else if (temp.rflags & RF_D3D)
	{
		set_video_mode_d3d();
	}
	else  // blt, direct video mem
	{
		//      desc.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
		if (conf.flip && !(temp.rflags & RF_CLIP))
		{
			desc.dwBackBufferCount = 1;
			desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
			desc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
		}

		if ((r = dd->CreateSurface(&desc, &surf0, nullptr)) != DD_OK)
		{
			printrdd("IDirectDraw2::CreateSurface() [primary]", r); terminate();
		}

		if (temp.rflags & RF_CLIP)
		{
			r = dd->CreateClipper(0, &clip, nullptr);
			if (r != DD_OK) { printrdd("IDirectDraw2::CreateClipper()", r); terminate(); }
			r = clip->SetHWnd(0, wnd);
			if (r != DD_OK) { printrdd("IDirectDraw2::SetHWnd()", r); terminate(); }
			r = surf0->SetClipper(clip);
			if (r != DD_OK) { printrdd("IDirectDrawSurface2::SetClipper()", r); terminate(); }

			r = dd->GetDisplayMode(&desc);
			if (r != DD_OK) { printrdd("IDirectDraw2::GetDisplayMode()", r); terminate(); }
			if ((temp.rflags & RF_32) && desc.ddpfPixelFormat.dwRGBBitCount != 32)
				errexit("video driver requires 32bit color depth on desktop for this mode");
			desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
			desc.dwWidth = temp.ox; desc.dwHeight = temp.oy;
			desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
			r = dd->CreateSurface(&desc, &surf1, nullptr);
			if (r != DD_OK) { printrdd("IDirectDraw2::CreateSurface()", r); terminate(); }
		}

		if (temp.obpp == 16)
		{
			DDPIXELFORMAT fm; fm.dwSize = sizeof fm;
			if ((r = surf0->GetPixelFormat(&fm)) != DD_OK)
			{
				printrdd("IDirectDrawSurface2::GetPixelFormat()", r); terminate();
			}

			if (fm.dwRBitMask == 0xF800 && fm.dwGBitMask == 0x07E0 && fm.dwBBitMask == 0x001F)
				temp.hi15 = 0;
			else if (fm.dwRBitMask == 0x7C00 && fm.dwGBitMask == 0x03E0 && fm.dwBBitMask == 0x001F)
				temp.hi15 = 1;
			else
				errexit("invalid pixel format (need RGB:5-6-5 or URGB:1-5-5-5)");

		}
		else if (temp.obpp == 8)
		{

			if ((r = dd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, syspalette, &pal, nullptr)) != DD_OK)
			{
				printrdd("IDirectDraw2::CreatePalette()", r); terminate();
			}
			if ((r = surf0->SetPalette(pal)) != DD_OK)
			{
				printrdd("IDirectDrawSurface2::SetPalette()", r); terminate();
			}
		}
	}

	if (conf.flip && !(temp.rflags & (RF_GDI | RF_CLIP | RF_D3D)))
	{
		DDSCAPS caps = { DDSCAPS_BACKBUFFER };
		if ((r = surf0->GetAttachedSurface(&caps, &surf1)) != DD_OK)
		{
			printrdd("IDirectDraw2::GetAttachedSurface()", r); terminate();
		}
	}

	// Настраиваем функцию конвертирования из текущего формата в BGR24
	switch (temp.obpp)
	{
	case 8: ConvBgr24 = ConvPal8ToBgr24; break;
	case 16:
		switch (temp.hi15)
		{
		case 0: ConvBgr24 = ConvRgb16ToBgr24; break; // RGB16
		case 1: ConvBgr24 = ConvRgb15ToBgr24; break; // RGB15
		case 2: ConvBgr24 = ConvYuy2ToBgr24; break; // YUY2
		}
	case 32: ConvBgr24 = ConvBgr32ToBgr24; break;
	}

	SendMessage(wnd, WM_USER, 0, 0); // setup rectangle for RF_GDI,OVR,CLIP, adjust cursor
	if (!conf.fullscr)
		scale_normal();
}

HRESULT set_di_dword_property(const LPDIRECTINPUTDEVICE pdev, REFGUID guid_property,
	const DWORD dw_object, const DWORD dw_how, const DWORD dw_value)
{
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwObj = dw_object;
	dipdw.diph.dwHow = dw_how;
	dipdw.dwData = dw_value;
	return pdev->SetProperty(guid_property, &dipdw.diph);
}

BOOL CALLBACK init_joystick_input(const LPCDIDEVICEINSTANCE pdinst, const LPVOID pv_ref)
{
	HRESULT r;
	const auto pdi = static_cast<LPDIRECTINPUT>(pv_ref);
	LPDIRECTINPUTDEVICE dijoyst1;
	LPDIRECTINPUTDEVICE2 dijoyst = nullptr;
	if ((r = pdi->CreateDevice(pdinst->guidInstance, &dijoyst1, nullptr)) != DI_OK)
	{
		printrdi("IDirectInput::CreateDevice() (joystick)", r);
		return DIENUM_CONTINUE;
	}

	r = dijoyst1->QueryInterface(IID_IDirectInputDevice2, (void**)&dijoyst);
	if (r != S_OK)
	{
		printrdi("IDirectInputDevice::QueryInterface(IID_IDirectInputDevice2) [dx5 not found]", r);
		dijoyst1->Release();
		dijoyst1 = nullptr;
		return DIENUM_CONTINUE;
	}
	dijoyst1->Release();

	DIDEVICEINSTANCE dide = { sizeof dide };
	if ((r = dijoyst->GetDeviceInfo(&dide)) != DI_OK)
	{
		printrdi("IDirectInputDevice::GetDeviceInfo()", r);
		return DIENUM_STOP;
	}

	DIDEVCAPS dc = { sizeof dc };
	if ((r = dijoyst->GetCapabilities(&dc)) != DI_OK)
	{
		printrdi("IDirectInputDevice::GetCapabilities()", r);
		return DIENUM_STOP;
	}

	DIPROPDWORD JoyId;
	JoyId.diph.dwSize = sizeof(JoyId);
	JoyId.diph.dwHeaderSize = sizeof(JoyId.diph);
	JoyId.diph.dwObj = 0;
	JoyId.diph.dwHow = DIPH_DEVICE;
	if ((r = dijoyst->GetProperty(DIPROP_JOYSTICKID, &JoyId.diph)) != DI_OK)
	{
		printrdi("IDirectInputDevice::GetProperty(DIPROP_JOYSTICKID)", r); terminate();
	}

	trim_right(dide.tszInstanceName);
	trim_right(dide.tszProductName);

	CharToOem(dide.tszInstanceName, dide.tszInstanceName);
	CharToOem(dide.tszProductName, dide.tszProductName);
	if (strcmp(dide.tszProductName, dide.tszInstanceName))
		strcat(dide.tszInstanceName, ", ");
	else
		dide.tszInstanceName[0] = 0;

	const bool use_joy = (JoyId.dwData == conf.input.JoyId);
	color(CONSCLR_HARDINFO);
	printf("%cjoy(%u): %s%s (%d axes, %d buttons, %d POVs)\n", use_joy ? '*' : ' ', JoyId.dwData,
		dide.tszInstanceName, dide.tszProductName, dc.dwAxes, dc.dwButtons, dc.dwPOVs);

	if (use_joy)
	{
		if ((r = dijoyst->SetDataFormat(&c_dfDIJoystick)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetDataFormat() (joystick)", r);
			terminate();
		}

		if ((r = dijoyst->SetCooperativeLevel(wnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetCooperativeLevel() (joystick)", r);
			terminate();
		}

		DIPROPRANGE diprg;
		diprg.diph.dwSize = sizeof(diprg);
		diprg.diph.dwHeaderSize = sizeof(diprg.diph);
		diprg.diph.dwObj = DIJOFS_X;
		diprg.diph.dwHow = DIPH_BYOFFSET;
		diprg.lMin = -1000;
		diprg.lMax = +1000;

		if ((r = dijoyst->SetProperty(DIPROP_RANGE, &diprg.diph)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetProperty(DIPH_RANGE)", r); terminate();
		}

		diprg.diph.dwObj = DIJOFS_Y;

		if ((r = dijoyst->SetProperty(DIPROP_RANGE, &diprg.diph)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetProperty(DIPH_RANGE) (y)", r); terminate();
		}

		if ((r = set_di_dword_property(dijoyst, DIPROP_DEADZONE, DIJOFS_X, DIPH_BYOFFSET, 2000)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetProperty(DIPH_DEADZONE)", r); terminate();
		}

		if ((r = set_di_dword_property(dijoyst, DIPROP_DEADZONE, DIJOFS_Y, DIPH_BYOFFSET, 2000)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetProperty(DIPH_DEADZONE) (y)", r); terminate();
		}
		::dijoyst = dijoyst;
	}
	else
	{
		dijoyst->Release();
	}
	return DIENUM_CONTINUE;
}

HRESULT WINAPI callb(const LPDDSURFACEDESC surf, void* lpContext)
{
	if (max_modes >= MAX_MODES)
		return DDENUMRET_CANCEL;
	modes[max_modes].x = surf->dwWidth;
	modes[max_modes].y = surf->dwHeight;
	modes[max_modes].b = surf->ddpfPixelFormat.dwRGBBitCount;
	modes[max_modes].f = surf->dwRefreshRate;
	max_modes++;
	return DDENUMRET_OK;
}

void scale_normal()
{
	ULONG cmd;
	switch (temp.scale)
	{
	default:
	case 1: cmd = scu_scale1; break;
	case 2: cmd = scu_scale2; break;
	case 3: cmd = scu_scale3; break;
	case 4: cmd = scu_scale4; break;
	}
	SendMessage(wnd, WM_SYSCOMMAND, cmd, 0); // set window size
}

static void start_d3d(const HWND wnd)
{
	if (!d3d_9dll)
		d3d_9dll = LoadLibrary("d3d9.dll");

	typedef IDirect3D9* (WINAPI* TDirect3DCreate9)(UINT SDKVersion);
	const auto direct_3d_create9 = (TDirect3DCreate9)GetProcAddress(d3d_9dll, "Direct3DCreate9");
	d3d9 = direct_3d_create9(DIRECT3D_VERSION);
	if (!d3d9)
		return;

	D3DDISPLAYMODE disp_mode;
	HRESULT r = d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &disp_mode);
	if (r != DD_OK)
	{
		printrdd("IDirect3D::GetAdapterDisplayMode()", r); terminate();
	}

	D3DPRESENT_PARAMETERS d3d_pp = { 0 };
	d3d_pp.Windowed = TRUE; // FALSE;
	d3d_pp.SwapEffect = D3DSWAPEFFECT_FLIP; //D3DSWAPEFFECT_DISCARD;
	d3d_pp.BackBufferCount = 2;
	d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	//    D3dPp.BackBufferWidth        = 640;
	//    D3dPp.BackBufferHeight       = 480;
	//    D3dPp.BackBufferFormat       = D3DFMT_X8R8G8B8;
	d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; //conf.flip ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
	d3d_pp.BackBufferFormat = disp_mode.Format;

	r = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3d_pp, &d3d_dev);
	if (r != DD_OK)
	{
		printrdd("IDirect3D::CreateDevice()", r); terminate();
	}
}

static void calc_window_size()
{
	temp.rflags = renders[conf.render].flags;

	if (temp.rflags & RF_DRIVER)
		temp.rflags |= drivers[conf.driver].flags;

	// select resolution
	temp.ox = temp.scx = bordersizes[conf.bordersize].xsize;
	temp.oy = temp.scy = bordersizes[conf.bordersize].ysize;


	if (temp.rflags & RF_2X)
	{
		temp.ox *= 2; temp.oy *= 2;
		if (conf.fast_sl && (temp.rflags & RF_DRIVER) && (temp.rflags & (RF_CLIP | RF_OVR)))
			temp.oy /= 2;
	}

	if (temp.rflags & RF_3X) temp.ox *= 3, temp.oy *= 3;
	if (temp.rflags & RF_4X) temp.ox *= 4, temp.oy *= 4;
	if (temp.rflags & RF_64x48) temp.ox = 64, temp.oy = 48;
	if (temp.rflags & RF_128x96) temp.ox = 128, temp.oy = 96;
	if (temp.rflags & RF_MON) temp.ox = 640, temp.oy = 480;
}

void start_dx()
{
	WNDCLASS  wc = { 0 };

	wc.lpfnWndProc = (WNDPROC)wnd_proc;
	hIn = wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = "EMUL_WND";
	wc.hIcon = LoadIcon(hIn, MAKEINTRESOURCE(IDI_MAIN));
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClass(&wc);

	for (int i = 0; i < 9; i++)
		crs[i] = LoadCursor(hIn, MAKEINTRESOURCE(IDC_C0 + i));
	//Alone Coder 0.36.6
	RECT rect1;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect1, 0);
	//~
	calc_window_size();

	int cx = temp.ox * temp.scale, cy = temp.oy * temp.scale;

	RECT Client = { 0, 0, cx, cy };
	AdjustWindowRect(&Client, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0);
	cx = Client.right - Client.left;
	cy = Client.bottom - Client.top;
	int winx = rect1.left + (rect1.right - rect1.left - cx) / 2;
	int winy = rect1.top + (rect1.bottom - rect1.top - cy) / 2;
	winx = winx < 0 ? 0 : winx;
	winy = winy < 0 ? 0 : winy;

	main_menu = LoadMenu(hIn, MAKEINTRESOURCE(IDR_MAINMENU));
	wnd = CreateWindow("EMUL_WND", "UnrealSpeccy", WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		winx, winy, cx, cy, nullptr, main_menu, hIn, NULL);
	//                    winx, winy, cx, cy, 0, 0, hIn, NULL);

	DragAcceptFiles(wnd, 1);

	temp.gdidc = GetDC(wnd);
	GetSystemPaletteEntries(temp.gdidc, 0, 0x100, syspalette);

	HMENU sys = GetSystemMenu(wnd, 0);
	AppendMenu(sys, MF_SEPARATOR, 0, nullptr);
	AppendMenu(sys, MF_STRING, scu_scale1, "x1");
	AppendMenu(sys, MF_STRING, scu_scale2, "x2");
	AppendMenu(sys, MF_STRING, scu_scale3, "x3");
	AppendMenu(sys, MF_STRING, scu_scale4, "x4");
	AppendMenu(sys, MF_STRING, scu_lock_mouse, "&Lock mouse");

	InitCommonControls();

	HRESULT r = E_UNEXPECTED;
	LPDIRECTDRAW dd0;

	if (HMODULE h_ddraw = LoadLibrary("ddraw.dll"))
	{
		typedef HRESULT(WINAPI* DIRECTDRAWCREATE) (LPGUID, LPDIRECTDRAW*, LPUNKNOWN);
		auto direct_draw_create = (DIRECTDRAWCREATE)GetProcAddress(h_ddraw, "DirectDrawCreate");
		if (direct_draw_create) r = direct_draw_create(nullptr, &dd0, nullptr);
	}

	if (r != DD_OK)
	{
		printrdd("DirectDrawCreate()", r); terminate();
	}

	if ((r = dd0->QueryInterface(IID_IDirectDraw2, (void**)&dd)) != DD_OK)
	{
		printrdd("IDirectDraw::QueryInterface(IID_IDirectDraw2)", r); terminate();
	}

	dd0->Release();

	if ((temp.rflags & RF_D3D))
		start_d3d(wnd);

	color(CONSCLR_HARDITEM); printf("gfx: ");

	char vmodel[MAX_DDDEVICEID_STRING + 32]; *vmodel = 0;
	if (conf.detect_video)
	{
		LPDIRECTDRAW4 dd4;
		if ((r = dd->QueryInterface(IID_IDirectDraw4, (void**)&dd4)) == DD_OK)
		{
			DDDEVICEIDENTIFIER di;
			if (dd4->GetDeviceIdentifier(&di, 0) == DD_OK)
			{
				trim_right(di.szDescription);
				CharToOem(di.szDescription, di.szDescription);
				sprintf(vmodel, "%04X-%04X (%s)", di.dwVendorId, di.dwDeviceId, di.szDescription);
			}
			else
				sprintf(vmodel, "unknown device");
			dd4->Release();
		}
		if (*vmodel)
			strcat(vmodel, ", ");
	}
	DDCAPS caps;
	caps.dwSize = sizeof caps;
	dd->GetCaps(&caps, nullptr);

	color(CONSCLR_HARDINFO);
	printf("%s%dMb VRAM available\n", vmodel, (caps.dwVidMemTotal + 512 * 1024) / (1024 * 1024));

	max_modes = 0;
	dd->EnumDisplayModes(DDEDM_REFRESHRATES | DDEDM_STANDARDVGAMODES, nullptr, nullptr, callb);

	WAVEFORMATEX wf = { 0 };
	wf.wFormatTag = WAVE_FORMAT_PCM;
	wf.nSamplesPerSec = conf.sound.fq;
	wf.nChannels = 2;
	wf.wBitsPerSample = 16;
	wf.nBlockAlign = 4;
	wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

	if (conf.sound.do_sound == do_sound_wave) {
		if ((r = waveOutOpen(&hwo, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL)) != MMSYSERR_NOERROR)
		{
			printrmm("waveOutOpen()", r); hwo = nullptr; goto sfail;
		}
		wqhead = 0, wqtail = 0;
	}
	else if (conf.sound.do_sound == do_sound_ds) {

		HMODULE hDdraw = LoadLibrary("dsound.dll");
		if (hDdraw)
		{
			typedef HRESULT(WINAPI* DIRECTSOUNDCREATE) (LPGUID, LPDIRECTSOUND*, LPUNKNOWN);
			auto direct_sound_create = (DIRECTSOUNDCREATE)GetProcAddress(hDdraw, "DirectSoundCreate");
			if (direct_sound_create) r = direct_sound_create(nullptr, &ds, nullptr);
		}

		if (r != DD_OK)
		{
			printrds("DirectSoundCreate()", r); goto sfail;
		}

		r = -1;
		if (conf.sound.dsprimary) r = ds->SetCooperativeLevel(wnd, DSSCL_WRITEPRIMARY);
		if (r != DS_OK) r = ds->SetCooperativeLevel(wnd, DSSCL_NORMAL), conf.sound.dsprimary = 0;
		if (r != DS_OK) { printrds("IDirectSound::SetCooperativeLevel()", r); goto sfail; }

		DSBUFFERDESC dsdesc = { sizeof(DSBUFFERDESC) }; r = -1;

		if (conf.sound.dsprimary) {

			dsdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_PRIMARYBUFFER;
			dsdesc.dwBufferBytes = 0;
			dsdesc.lpwfxFormat = nullptr;
			r = ds->CreateSoundBuffer(&dsdesc, &dsbf, nullptr);

			if (r != DS_OK) { printrds("IDirectSound::CreateSoundBuffer() [primary]", r); }
			else {
				r = dsbf->SetFormat(&wf);
				if (r != DS_OK) { printrds("IDirectSoundBuffer::SetFormat()", r); goto sfail; }
				DSBCAPS caps; caps.dwSize = sizeof caps; dsbf->GetCaps(&caps);
				dsbuffer = caps.dwBufferBytes;
			}
		}

		if (r != DS_OK)
		{
			dsdesc.lpwfxFormat = &wf;
			dsdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
			dsbuffer = dsdesc.dwBufferBytes = dsbuffer_sz;
			if ((r = ds->CreateSoundBuffer(&dsdesc, &dsbf, nullptr)) != DS_OK)
			{
				printrds("IDirectSound::CreateSoundBuffer()", r);
				goto sfail;
			}

			conf.sound.dsprimary = 0;
		}

		dsoffset = dsbuffer / 4;

	}
	else {
	sfail:
		conf.sound.do_sound = do_sound_none;
	}

	r = E_UNEXPECTED;
	LPDIRECTINPUT di = { nullptr };
	HMODULE h_dinput = LoadLibrary("dinput.dll");
	if (h_dinput)
	{
		typedef HRESULT(WINAPI* DIRECTINPUTCREATE) (HINSTANCE, DWORD, LPDIRECTINPUTA*, LPUNKNOWN);
		auto direct_input_create_a = reinterpret_cast<DIRECTINPUTCREATE>(GetProcAddress(h_dinput, "DirectInputCreateA"));
		if (direct_input_create_a)
		{
			r = direct_input_create_a(hIn, 0x0500, &di, nullptr);
			if (r != DI_OK) r = direct_input_create_a(hIn, 0x0300, &di, nullptr);
		}
	}

	if (r != DI_OK)
	{
		printrdi("DirectInputCreate()", r);
		terminate();
	}

	if ((r = di->CreateDevice(GUID_SysKeyboard, &dikeyboard, nullptr)) != DI_OK)
	{
		printrdi("IDirectInputDevice::CreateDevice() (keyboard)", r);
		terminate();
	}

	if ((r = dikeyboard->SetDataFormat(&c_dfDIKeyboard)) != DI_OK)
	{
		printrdi("IDirectInputDevice::SetDataFormat() (keyboard)", r);
		terminate();
	}

	if ((r = dikeyboard->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)) != DI_OK)
	{
		printrdi("IDirectInputDevice::SetCooperativeLevel() (keyboard)", r);
		terminate();
	}

	if ((r = di->CreateDevice(GUID_SysMouse, &dimouse, nullptr)) == DI_OK)
	{
		if ((r = dimouse->SetDataFormat(&c_dfDIMouse)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetDataFormat() (mouse)", r);
			terminate();
		}

		if ((r = dimouse->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetCooperativeLevel() (mouse)", r);
			terminate();
		}
		DIPROPDWORD dipdw = { 0 };
		dipdw.diph.dwSize = sizeof(dipdw);
		dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = DIPROPAXISMODE_ABS;
		if ((r = dimouse->SetProperty(DIPROP_AXISMODE, &dipdw.diph)) != DI_OK)
		{
			printrdi("IDirectInputDevice::SetProperty() (mouse)", r);
			terminate();
		}
	}
	else
	{
		color(CONSCLR_WARNING);
		printf("warning: no mouse\n");
		dimouse = nullptr;
	}

	if ((r = di->EnumDevices(DIDEVTYPE_JOYSTICK, init_joystick_input, di, DIEDFL_ATTACHEDONLY)) != DI_OK)
	{
		printrdi("IDirectInput::EnumDevices(DIDEVTYPE_JOYSTICK,...)", r);
		terminate();
	}

	di->Release();
	//[vv]   SetKeyboardState(kbdpc); // fix bug in win95
}

static void done_d3d(const bool de_init_dll)
{
	if (surf_texture)
	{
		surf_texture->Release();
		surf_texture = nullptr;
	}
	if (d3d_dev)
	{
		d3d_dev->Release();
		d3d_dev = nullptr;
	}
	if (d3d9)
	{
		d3d9->Release();
		d3d9 = nullptr;
	}
	if (de_init_dll && d3d_9dll)
	{
		FreeLibrary(d3d_9dll);
		d3d_9dll = nullptr;
	}
}

void done_dx()
{
	sound_stop();
	if (pal) pal->Release(); pal = nullptr;
	if (surf1) surf1->Release(); surf1 = nullptr;
	if (surf0) surf0->Release(); surf0 = nullptr;
	if (sprim) sprim->Release(); sprim = nullptr;
	if (clip) clip->Release(); clip = nullptr;
	if (dd) dd->Release(); dd = nullptr;
	if (dikeyboard) dikeyboard->Unacquire(), dikeyboard->Release(); dikeyboard = nullptr;
	if (dimouse) dimouse->Unacquire(), dimouse->Release(); dimouse = nullptr;
	if (dijoyst) dijoyst->Unacquire(), dijoyst->Release(); dijoyst = nullptr;
	if (hwo) { waveOutReset(hwo); /* waveOutUnprepareHeader()'s ? */ waveOutClose(hwo); }
	if (dsbf) dsbf->Release(); dsbf = nullptr;
	if (ds) ds->Release(); ds = nullptr;
	if (hbm) DeleteObject(hbm); hbm = nullptr;
	if (temp.gdidc) ReleaseDC(wnd, temp.gdidc); temp.gdidc = nullptr;
	done_d3d();
	//if (wnd) DestroyWindow(wnd);
}

