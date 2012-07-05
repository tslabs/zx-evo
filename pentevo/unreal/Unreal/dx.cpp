#include "std.h"
#include "resource.h"
#include "emul.h"
#include "vars.h"
#include "init.h"
#include "dx.h"
#include "draw.h"
#include "dxrend.h"
#include "dxrendch.h"
#include "dxrframe.h"
#include "dxr_text.h"
#include "dxr_rsm.h"
#include "dxr_advm.h"
#include "dxovr.h"
#include "dxerr.h"
#include "sound.h"
#include "savesnd.h"
#include "emulkeys.h"
#include "util.h"

unsigned char active = 0, pause = 0;

static const DWORD SCU_SCALE1 = 0x10;
static const DWORD SCU_SCALE2 = 0x20;
static const DWORD SCU_SCALE3 = 0x30;
static const DWORD SCU_SCALE4 = 0x40;
static const DWORD SCU_LOCK_MOUSE = 0x50;

#define MAXDSPIECE (40000*4/20)
#define DSBUFFER (2*44100*4)

#define SLEEP_DELAY 2

static HMODULE D3d9Dll = 0;
static IDirect3D9 *D3d9 = 0;
static IDirect3DDevice9 *D3dDev = 0;
static IDirect3DSurface9 *SurfTexture = 0;

LPDIRECTDRAW2 dd;
LPDIRECTDRAWSURFACE sprim, surf0, surf1;

LPDIRECTINPUTDEVICE dikeyboard;
LPDIRECTINPUTDEVICE dimouse;
LPDIRECTINPUTDEVICE2 dijoyst;

LPDIRECTSOUND ds;
LPDIRECTSOUNDBUFFER dsbf;
LPDIRECTDRAWPALETTE pal;
LPDIRECTDRAWCLIPPER clip;

static HANDLE EventBufStop = 0;
unsigned dsoffset, dsbuffer = DSBUFFER;

/* ---------------------- renders ------------------------- */

RENDER renders[] =
{
   { "Normal",                    render_small,   "normal",    RF_DRIVER | RF_1X },
   { "Double size",               render_dbl,     "double",    RF_DRIVER | RF_2X },
   { "Triple size",               render_3x,      "triple",    RF_DRIVER | RF_3X },
   { "Quad size",                 render_quad,    "quad",      RF_DRIVER | RF_4X },
   { "Anti-Text64",               render_text,    "text",      RF_DRIVER | RF_2X | RF_USEFONT },
   { "Frame resampler",           render_rsm,     "resampler", RF_DRIVER | RF_8BPCH },

   { "TV Emulation",              render_tv,      "tv",        RF_YUY2 | RF_OVR },
   { "Bilinear filter (MMX,fullscr)", render_bil, "bilinear",  RF_2X | RF_PALB },
   { "Vector scaling (fullscr)",  render_scale,   "scale",     RF_2X },
   { "AdvMAME scale (fullscr)",   render_advmame, "advmame",   RF_DRIVER },

   { "Chunky overlay (fast!)",    render_ch_ov,   "ch_ov",     RF_OVR | 0*RF_128x96 | 0*RF_64x48 | RF_BORDER | RF_USE32AS16 },
   { "Chunky 32bit hardware (flt,fullscr)", render_ch_hw,   "ch_hw",     RF_CLIP | 0*RF_128x96 | 0*RF_64x48 | RF_BORDER | RF_32 | RF_USEC32 },

   { "Chunky 16bit, low-res, (flt,fullscr)",render_c16bl,   "ch_bl",     RF_16 | RF_BORDER | RF_USEC32 },
   { "Chunky 16bit, hi-res, (flt,fullscr)", render_c16b,    "ch_b",      RF_16 | RF_2X | RF_BORDER | RF_USEC32 },
   { "Ch4x4 true color (fullscr)",render_c4x32b,  "ch4true",   RF_32 | RF_2X | RF_BORDER | RF_USEC32 },

   { 0,0,0,0 }
};

size_t renders_count = _countof(renders);

const RENDER drivers[] =
{
   { "video memory (8bpp)",  0, "ddraw",  RF_8 },
   { "video memory (16bpp)", 0, "ddrawh", RF_16 },
   { "video memory (32bpp)", 0, "ddrawt", RF_32 },
   { "gdi device context",   0, "gdi",    RF_GDI },
   { "overlay",              0, "ovr",    RF_OVR },
   { "hardware blitter",     0, "blt",    RF_CLIP },
   { "hardware 3d",          0, "d3d",    RF_D3D },
   { 0,0,0,0 }
};

inline void switch_video()
{
   static unsigned char eff7 = -1;
   if ((comp.pEFF7 ^ eff7) & EFF7_HWMC)
   {
       video_timing_tables();
       eff7 = comp.pEFF7;
   }
}

static void FlipGdi()
{
    if (needclr)
    {
        needclr--;
        gdi_frame();
    }
    renders[conf.render].func(gdibuf, temp.ox*temp.obpp/8); // render to memory buffer
    // copy bitmap to gdi dc
    SetDIBitsToDevice(temp.gdidc, temp.gx, temp.gy, temp.ox, temp.oy, 0, 0, 0, temp.oy, gdibuf, &gdibmp.header, DIB_RGB_COLORS);
}

static void FlipBlt()
{
restore_lost:;
   DDSURFACEDESC desc;
   desc.dwSize = sizeof desc;
   HRESULT r = surf1->Lock(0, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
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
       exit();
   }

   renders[conf.render].func((unsigned char*)desc.lpSurface, desc.lPitch);

   surf1->Unlock(0);

   assert(!IsRectEmpty(&temp.client));

   DDBLTFX Fx;
   Fx.dwSize = sizeof(Fx);
   Fx.dwDDFX = 0;

   r = surf0->Blt(&temp.client, surf1, 0, DDBLT_WAIT | DDBLT_DDFX, &Fx);
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
       exit();
   }
}

static void FlipD3d()
{
    if (!SUCCEEDED(D3dDev->BeginScene()))
        return;

    HRESULT Hr;
    IDirect3DSurface9 *SurfBackBuffer0 = 0, *SurfBackBuffer1 = 0;

    Hr = D3dDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &SurfBackBuffer0);
    if (Hr != DD_OK)
    { printrdd("IDirect3DDevice9::GetBackBuffer(0)", Hr); exit(); }

    Hr = D3dDev->GetBackBuffer(0, 1, D3DBACKBUFFER_TYPE_MONO, &SurfBackBuffer1);
    if (Hr != DD_OK)
    { printrdd("IDirect3DDevice9::GetBackBuffer(1)", Hr); exit(); }

    D3DLOCKED_RECT Rect = { 0 };

    Hr = SurfTexture->LockRect(&Rect, 0, D3DLOCK_DISCARD);
    if (FAILED(Hr))
    {
        __debugbreak();
    }

    renders[conf.render].func((u8 *)Rect.pBits, Rect.Pitch);

    SurfTexture->UnlockRect();

    Hr = D3dDev->StretchRect(SurfTexture, 0, SurfBackBuffer0, 0, D3DTEXF_POINT);
    if (FAILED(Hr))
    {
        __debugbreak();
    }

    Hr = D3dDev->ColorFill(SurfBackBuffer1, 0, D3DCOLOR_XRGB(0, 0, 0));
    if (FAILED(Hr))
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
    D3dDev->EndScene();

    // Present the backbuffer contents to the display
    Hr = D3dDev->Present(0, 0, 0, 0);
    if (FAILED(Hr))
    {
        __debugbreak();
    }

    Hr = D3dDev->Present(0, 0, 0, 0);
    if (FAILED(Hr))
    {
        __debugbreak();
    }
    SurfBackBuffer0->Release();
    SurfBackBuffer1->Release();
}

static void FlipVideoMem()
{
   // draw direct to video memory, overlay
   LPDIRECTDRAWSURFACE drawsurf = conf.flip ? surf1 : surf0;

   DDSURFACEDESC desc;
   desc.dwSize = sizeof desc;
restore_lost:;
   HRESULT r = drawsurf->Lock(0, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
   if (r != DD_OK)
   {
      if (!active)
          return;
      if (r == DDERR_SURFACELOST)
      {
          drawsurf->Restore();
          if (drawsurf->IsLost() == DDERR_SURFACELOST)
              Sleep(1);
          goto restore_lost;
      }
      printrdd("IDirectDrawSurface2::Lock()", r);
      if (!exitflag)
          exit();
   }
   if (needclr)
   {
       needclr--;
       _render_black((unsigned char*)desc.lpSurface, desc.lPitch);
   }
   renders[conf.render].func((unsigned char*)desc.lpSurface + desc.lPitch*temp.ody + temp.odx, desc.lPitch);
   drawsurf->Unlock(0);

   if (conf.flip) // draw direct to video memory
   {
      r = surf0->Flip(0, DDFLIP_WAIT);
      if (r != DD_OK)
      {
          if (r == DDERR_SURFACELOST)
          {
              surf0->Restore();
              if (surf0->IsLost() == DDERR_SURFACELOST)
                  Sleep(1);
              goto restore_lost;
          }
          printrdd("IDirectDrawSurface2::Flip()", r);
          exit();
      }
   }
}

void flip()
{
   if (temp.Minimized)
       return;
   switch_video();

   if (conf.flip && (temp.rflags & (RF_GDI | RF_CLIP)))
      dd->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, 0);

   if (temp.rflags & RF_GDI) // gdi
   {
      FlipGdi();
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

      FlipBlt();
      return;
   }

   if (temp.rflags & RF_D3D) // direct 3d
   {
      if (IsRectEmpty(&temp.client)) // client area is empty
          return;

      FlipD3d();
      return;
   }

   // draw direct to video memory, overlay
   FlipVideoMem();
}

HWAVEOUT hwo = 0;
WAVEHDR wq[MAXWQSIZE];
unsigned char wbuffer[MAXWQSIZE*MAXDSPIECE];
unsigned wqhead, wqtail;

void __fastcall do_sound_none()
{

}

void __fastcall do_sound_wave()
{
   HRESULT r;

   for (;;) // release all done items
   {
      while ((wqtail != wqhead) && (wq[wqtail].dwFlags & WHDR_DONE))
      { // ready!
         if ((r = waveOutUnprepareHeader(hwo, &wq[wqtail], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
         {
             printrmm("waveOutUnprepareHeader()", r);
             exit();
         }
         if (++wqtail == conf.soundbuffer)
             wqtail = 0;
      }
      if ((wqhead+1)%conf.soundbuffer != wqtail)
          break; // some empty item in queue
/* [vv]
*/
      if (conf.sleepidle)
          Sleep(SLEEP_DELAY);
   }

   if (!spbsize)
       return;

//   __debugbreak();
   // put new item and play
   PCHAR bfpos = PCHAR(wbuffer + wqhead*MAXDSPIECE);
   memcpy(bfpos, sndplaybuf, spbsize);
   wq[wqhead].lpData = bfpos;
   wq[wqhead].dwBufferLength = spbsize;
   wq[wqhead].dwFlags = 0;

   if ((r = waveOutPrepareHeader(hwo, &wq[wqhead], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
   {
       printrmm("waveOutPrepareHeader()", r);
       exit();
   }

   if ((r = waveOutWrite(hwo, &wq[wqhead], sizeof(WAVEHDR))) != MMSYSERR_NOERROR)
   {
       printrmm("waveOutWrite()", r);
       exit();
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
   void *ptr1, *ptr2;
   DWORD sz1, sz2;
   r = dsbf->Lock(0, 0, &ptr1, &sz1, &ptr2, &sz2, DSBLOCK_ENTIREBUFFER);
   if (r != DS_OK)
       return;
   memset(ptr1, 0, sz1);
   //memset(ptr2, 0, sz2);
   dsbf->Unlock(ptr1, sz1, ptr2, sz2);
}

int maxgap;

void __fastcall do_sound_ds()
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
         exit();
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

      if ((r = dsbf->Play(0,0, DSBPLAY_LOOPING)) != DS_OK)
      {
          printrds("IDirectSoundBuffer::Play()", r);
          exit();
      }

      if (conf.sleepidle)
          Sleep(SLEEP_DELAY);
   }

/*
   if (dsoffset >= dsbuffer)
       dsoffset -= dsbuffer;
*/
   dsoffset %= dsbuffer;

   if (!spbsize)
       return;

   void *ptr1, *ptr2;
   DWORD sz1, sz2;
   r = dsbf->Lock(dsoffset, spbsize, &ptr1, &sz1, &ptr2, &sz2, 0);
   if (r != DS_OK)
   {
//       __debugbreak();
       printf("dsbuffer=%d, dsoffset=%d, spbsize=%d\n", dsbuffer, dsoffset, spbsize);
       printrds("IDirectSoundBuffer::Lock()", r);
       exit();
   }
   memcpy(ptr1, sndplaybuf, sz1);
   if (ptr2)
       memcpy(ptr2, ((char *)sndplaybuf)+sz1, sz2);

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
       exit();
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
      if (fwrite(sndplaybuf,1,spbsize,savesnd)!=spbsize)
         savesnddialog(); // write error - disk full - close file

   conf.sound.do_sound();
}

void set_priority()
{
   if (!conf.highpriority || !conf.sleepidle) return;
   SetPriorityClass(GetCurrentProcess(), conf.sound.enabled? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}

void adjust_mouse_cursor()
{
   unsigned showcurs = conf.input.joymouse || !active || !(conf.fullscr || conf.lockmouse) || dbgbreak;
   while (ShowCursor(0) >= 0); // hide cursor
   if (showcurs) while (ShowCursor(1) <= 0); // show cursor
   if (active && conf.lockmouse && !dbgbreak)
   {
      RECT rc; GetClientRect(wnd, &rc);
      POINT p = { rc.left, rc.top };
      ClientToScreen(wnd, &p); rc.left=p.x, rc.top=p.y;
      p.x = rc.right, p.y = rc.bottom;
      ClientToScreen(wnd, &p); rc.right=p.x, rc.bottom=p.y;
      ClipCursor(&rc);
   } else ClipCursor(0);
}

HCURSOR crs[9];
unsigned char mousedirs[9] = { 10, 8, 9, 2, 0, 1, 6, 4, 5 };

void updatebitmap()
{
   RECT rc;
   GetClientRect(wnd, &rc);
   DWORD newdx = rc.right - rc.left, newdy = rc.bottom - rc.top;
   if (hbm && (bm_dx != newdx || bm_dy != newdy))
   {
       DeleteObject(hbm);
       hbm = 0;
   }
   if (sprim)
       return; // don't trace window contents in overlay mode

   if (hbm)
   {
        DeleteObject(hbm);
        hbm = 0; // keeping bitmap is unsafe - screen paramaters may change
   }

   if (!hbm)
       hbm = CreateCompatibleBitmap(temp.gdidc, newdx, newdy);

   HDC dc = CreateCompatibleDC(temp.gdidc);

   bm_dx = newdx; bm_dy = newdy;
   HGDIOBJ PrevObj = SelectObject(dc, hbm);
   //SetDIBColorTable(dc, 0, 0x100, (RGBQUAD*)pal0);
   BitBlt(dc, 0, 0, newdx, newdy, temp.gdidc, 0, 0, SRCCOPY);
   SelectObject(dc, PrevObj);
   DeleteDC(dc);
}

static void StartD3d(HWND Wnd);
static void DoneD3d(bool DeInitDll = true);
static void SetVideoModeD3d();

INT_PTR CALLBACK WndProc(HWND hwnd,UINT uMessage,WPARAM wparam,LPARAM lparam)
{
   static bool moving = false;

   if (uMessage == WM_QUIT) // never heppens
   {
//       __debugbreak();
//       printf("WM_QUIT\n");
       exit();
   }

   if (uMessage == WM_CLOSE) // never heppens
   {
//       __debugbreak();
//       printf("WM_CLOSE\n");
       return 1;
   }

   if (uMessage == WM_SETFOCUS && !pause)
   {
      active = 1; setpal(0);
//      sound_play();
      adjust_mouse_cursor();
      input.nokb = 20;
      uMessage = WM_USER;
   }

   if (uMessage == WM_KILLFOCUS && !pause)
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
      if (uMessage == WM_LBUTTONDOWN || uMessage == WM_LBUTTONUP)
      {
         input.mousejoy = (input.mousejoy & 0x0F) | (uMessage == WM_LBUTTONDOWN ? 0x10 : 0);
         input.kjoy = (input.kjoy & 0x0F) | (uMessage == WM_LBUTTONDOWN ? 0x10 : 0);
      }

      if (uMessage == WM_MOUSEMOVE)
      {
         RECT rc; GetClientRect(hwnd, &rc);
         unsigned xx = LOWORD(lparam)*3/(rc.right - rc.left);
         unsigned yy = HIWORD(lparam)*3/(rc.bottom - rc.top);
         unsigned nn = yy*3 + xx;
//         SetClassLongPtr(hwnd, GCLP_HCURSOR, (LONG)crs[nn]); //Alone Coder
         SetCursor(crs[nn]); //Alone Coder
         input.mousejoy = (input.mousejoy & 0x10) | mousedirs[nn];
         input.kjoy = (input.kjoy & 0x10) | mousedirs[nn];
         return 0;
      }
   }
   else if (uMessage == WM_LBUTTONDOWN && !conf.lockmouse)
   {
//       printf("%s\n", __FUNCTION__);
       input.nomouse = 20;
       main_mouse();
   }

   if (uMessage == WM_ENTERSIZEMOVE)
   {
       sound_stop();
   }

   if (uMessage == WM_EXITSIZEMOVE)
   {
       sound_play();
   }

   if (uMessage == WM_SIZE || uMessage == WM_MOVE || uMessage == WM_USER)
   {
      GetClientRect(wnd, &temp.client);
      temp.gdx = temp.client.right-temp.client.left;
      temp.gdy = temp.client.bottom-temp.client.top;
      temp.gx = (temp.gdx > temp.ox) ? (temp.gdx-temp.ox)/2 : 0;
      temp.gy = (temp.gdy > temp.oy) ? (temp.gdy-temp.oy)/2 : 0;
      ClientToScreen(wnd, (POINT*)&temp.client.left);
      ClientToScreen(wnd, (POINT*)&temp.client.right);
      adjust_mouse_cursor();
      if (sprim)
          uMessage = WM_PAINT;
      needclr = 2;
      if ((temp.rflags & RF_D3D) && dd && uMessage == WM_SIZE && temp.ox && temp.oy)
      {
          DoneD3d(false);
          SetVideoModeD3d();
      }
   }

   if (uMessage == WM_PAINT)
   {
      if (sprim)
      { // background for overlay
         RECT rc;
         GetClientRect(hwnd, &rc);
         HBRUSH br = CreateSolidBrush(RGB(0xFF,0x00,0xFF));
         FillRect(temp.gdidc, &rc, br);
         DeleteObject(br);
         update_overlay();
      }
      else if (hbm && !active)
      {
//       printf("%s, WM_PAINT\n", __FUNCTION__);
         HDC hcom = CreateCompatibleDC(temp.gdidc);
         HGDIOBJ PrevObj = SelectObject(hcom, hbm);
         BitBlt(temp.gdidc, 0, 0, bm_dx, bm_dy, hcom, 0, 0, SRCCOPY);
         SelectObject(hcom, PrevObj);
         DeleteDC(hcom);
      }
   }

   if (uMessage == WM_SYSCOMMAND)
   {
//       printf("%s, WM_SYSCOMMAND 0x%04X\n", __FUNCTION__, (ULONG)wparam);

      switch(wparam & 0xFFF0)
      {
      case SCU_SCALE1: temp.scale = 1; wnd_resize(1);  return 0;
      case SCU_SCALE2: temp.scale = 2; wnd_resize(2);  return 0;
      case SCU_SCALE3: temp.scale = 3; wnd_resize(3);  return 0;
      case SCU_SCALE4: temp.scale = 4; wnd_resize(4);  return 0;
      case SCU_LOCK_MOUSE: main_mouse();  return 0;
      case SC_CLOSE:
          if (ConfirmExit())
              correct_exit();
      return 0;
      case SC_MINIMIZE: temp.Minimized = true; break;

      case SC_RESTORE: temp.Minimized = false; break;
      }
   }

   if (uMessage == WM_DROPFILES)
   {
      HDROP hDrop = (HDROP)wparam;
      DragQueryFile(hDrop, 0, droppedFile, sizeof(droppedFile));
      DragFinish(hDrop);
      return 0;
   }

   return DefWindowProc(hwnd, uMessage, wparam, lparam);
}

void readdevice(VOID *md, DWORD sz, LPDIRECTINPUTDEVICE dev)
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
          exit();
      }
      r = dev->GetDeviceState(sz, md);
   }
   if (r != DI_OK)
   {
       printrdi("IDirectInputDevice::GetDeviceState()", r);
       exit();
   }
}

void readmouse(DIMOUSESTATE *md)
{
   memset(md, 0, sizeof *md);
   readdevice(md, sizeof *md, dimouse);
}

void ReadKeyboard(PVOID KbdData)
{
    readdevice(KbdData, 256, dikeyboard);
}

void setpal(char system)
{
   if (!active || !dd || !surf0 || !pal) return;
   HRESULT r;
   if (surf0->IsLost() == DDERR_SURFACELOST) surf0->Restore();
   if ((r = pal->SetEntries(0, 0, 0x100, system ? syspalette : pal0)) != DD_OK)
   { printrdd("IDirectDrawPalette::SetEntries()", r); exit(); }
}

void trim_right(char *str)
{
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = strlen(str); i && str[i-1] == ' '; i--);
   str[i] = 0;
}

#define MAX_MODES 512
struct MODEPARAM {
   unsigned x,y,b,f;
} modes[MAX_MODES];
unsigned max_modes;

static void SetVideoModeD3d()
{
    HRESULT r;

    if (!D3dDev)
        StartD3d(wnd);

    if (!D3dDev)
        return;

    IDirect3DTexture9 *Texture = 0;
    D3DDISPLAYMODE DispMode;
    r = D3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &DispMode);
    if (r != DD_OK)
    { printrdd("IDirect3D::GetAdapterDisplayMode()", r); exit(); }
    r = D3dDev->CreateTexture(temp.ox, temp.oy, 1, D3DUSAGE_DYNAMIC, DispMode.Format, D3DPOOL_DEFAULT, &Texture, 0);
    if (r != DD_OK)
    { printrdd("IDirect3DDevice9::CreateTexture()", r); exit(); }
    r = Texture->GetSurfaceLevel(0, &SurfTexture);
    if (r != DD_OK)
    { printrdd("IDirect3DTexture::GetSurfaceLevel()", r); exit(); }
    if (!SurfTexture)
        __debugbreak();
    if (Texture)
        Texture->Release();
}

void set_vidmode()
{
   if (pal)
   {
       pal->Release();
       pal = 0;
   }

   if (surf1)
   {
       surf1->Release();
       surf1 = 0;
   }

   if (surf0)
   {
       surf0->Release();
       surf0 = 0;
   }

   if (sprim)
   {
       sprim->Release();
       sprim = 0;
   }

   if (clip)
   {
       clip->Release();
       clip = 0;
   }

   if (SurfTexture)
   {
       SurfTexture->Release();
       SurfTexture = 0;
   }

   HRESULT r;

   DDSURFACEDESC desc;
   desc.dwSize = sizeof desc;
   r = dd->GetDisplayMode(&desc);
   if (r != DD_OK) { printrdd("IDirectDraw2::GetDisplayMode()", r); exit(); }
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
      if (r != DD_OK) { printrdd("IDirectDraw2::SetCooperativeLevel()", r); exit(); }
   }

   // select resolution
   static const int size_x[3] = { 256, 320, 384 };
   static const int size_y[3] = { 192, 240, 300 };
   temp.ox = temp.scx = size_x[conf.bordersize];
   temp.oy = temp.scy = size_y[conf.bordersize];

   if (temp.rflags & RF_2X)
   {
      temp.ox *=2; temp.oy *= 2;
      if (conf.fast_sl && (temp.rflags & RF_DRIVER) && (temp.rflags & (RF_CLIP | RF_OVR)))
          temp.oy /= 2;
   }

   if (temp.rflags & RF_3X) temp.ox *= 3, temp.oy *= 3;
   if (temp.rflags & RF_4X) temp.ox *= 4, temp.oy *= 4;
   if (temp.rflags & RF_64x48) temp.ox = 64, temp.oy = 48;
   if (temp.rflags & RF_128x96) temp.ox = 128, temp.oy = 96;
   if (temp.rflags & RF_MON) temp.ox = 640, temp.oy = 480;

//   printf("temp.ox=%d, temp.oy=%d\n", temp.ox, temp.oy);

   // select color depth
   temp.obpp = 8;
   if (temp.rflags & (RF_CLIP | RF_D3D))
       temp.obpp = desc.ddpfPixelFormat.dwRGBBitCount;
   if (temp.rflags & (RF_16 | RF_OVR))
       temp.obpp = 16;
   if (temp.rflags & RF_32)
       temp.obpp = 32;
   if ((temp.rflags & (RF_GDI|RF_8BPCH)) == (RF_GDI|RF_8BPCH))
       temp.obpp = 32;

   if (conf.fullscr || ((temp.rflags & RF_MON) && desc.dwHeight < 480))
   {
      // select minimal screen mode
      unsigned newx = 100000, newy = 100000, newfq = conf.refresh, newb = temp.obpp;
      unsigned minx = temp.ox, miny = temp.oy, needb = temp.obpp;

      if (temp.rflags & (RF_64x48 | RF_128x96))
      {
         needb = (temp.rflags & RF_16)? 16:32;
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

      if (newx==100000)
      {
          color(CONSCLR_ERROR);
          printf("can't find situable mode for %d x %d * %d bits\n", temp.ox, temp.oy, temp.obpp);
          exit();
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
         { printrdd("IDirectDraw2::SetDisplayMode()", r); exit(); }
         GetSystemPaletteEntries(temp.gdidc, 0, 0x100, syspalette);
         if (newfq)
             temp.ofq = newfq;
      }
      temp.odx = temp.obpp*(newx - temp.ox)/16, temp.ody = (newy - temp.oy)/2;
      temp.rsx = newx, temp.rsy = newy;
      ShowWindow(wnd, SW_SHOWMAXIMIZED);
   }
   else
   {
      // restore window position to last saved position in non-fullscreen mode
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

   DWORD pl[0x101]; pl[0] = 0x01000300; memcpy(pl+1, pal0, 0x400);
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
      if ((r = dd->CreateSurface(&desc, &sprim, 0)) != DD_OK)
      { printrdd("IDirectDraw2::CreateSurface() [primary,test]", r); exit(); }

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

      static DDPIXELFORMAT ddpfOverlayFormat16 = { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0xF800}, {0x07E0}, {0x001F}, {0} };
      static DDPIXELFORMAT ddpfOverlayFormat15 = { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, {16}, {0x7C00}, {0x03E0}, {0x001F}, {0} };
      static DDPIXELFORMAT ddpfOverlayFormatYUY2 = { sizeof(DDPIXELFORMAT), DDPF_FOURCC, MAKEFOURCC('Y','U','Y','2'), {0},{0},{0},{0},{0} };

      if (temp.rflags & RF_YUY2)
          goto YUY2;

      temp.hi15 = 0;
      desc.ddpfPixelFormat = ddpfOverlayFormat16;
      r = dd->CreateSurface(&desc, &surf0, 0);

      if (r == DDERR_INVALIDPIXELFORMAT)
      {
         temp.hi15 = 1;
         desc.ddpfPixelFormat = ddpfOverlayFormat15;
         r = dd->CreateSurface(&desc, &surf0, 0);
      }

      if (r == DDERR_INVALIDPIXELFORMAT /*&& !(temp.rflags & RF_RGB)*/)
      {
       YUY2:
         temp.hi15 = 2;
         desc.ddpfPixelFormat = ddpfOverlayFormatYUY2;
         r = dd->CreateSurface(&desc, &surf0, 0);
      }

      if (r != DD_OK)
      { printrdd("IDirectDraw2::CreateSurface() [overlay]", r); exit(); }

   }
   else if (temp.rflags & RF_D3D)
   {
      SetVideoModeD3d();
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

      if ((r = dd->CreateSurface(&desc, &surf0, 0)) != DD_OK)
      { printrdd("IDirectDraw2::CreateSurface() [primary]", r); exit(); }

      if (temp.rflags & RF_CLIP)
      {
         r = dd->CreateClipper(0, &clip, 0);
         if (r != DD_OK) { printrdd("IDirectDraw2::CreateClipper()", r); exit(); }
         r = clip->SetHWnd(0, wnd);
         if (r != DD_OK) { printrdd("IDirectDraw2::SetHWnd()", r); exit(); }
         r = surf0->SetClipper(clip);
         if (r != DD_OK) { printrdd("IDirectDrawSurface2::SetClipper()", r); exit(); }

         r = dd->GetDisplayMode(&desc);
         if (r != DD_OK) { printrdd("IDirectDraw2::GetDisplayMode()", r); exit(); }
         if ((temp.rflags & RF_32) && desc.ddpfPixelFormat.dwRGBBitCount != 32)
             errexit("video driver requires 32bit color depth on desktop for this mode");
         desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
         desc.dwWidth = temp.ox; desc.dwHeight = temp.oy;
         desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
         r = dd->CreateSurface(&desc, &surf1, 0);
         if (r != DD_OK) { printrdd("IDirectDraw2::CreateSurface()", r); exit(); }
      }

      if (temp.obpp == 16)
      {
         DDPIXELFORMAT fm; fm.dwSize = sizeof fm;
         if ((r = surf0->GetPixelFormat(&fm)) != DD_OK)
         { printrdd("IDirectDrawSurface2::GetPixelFormat()", r); exit(); }

         if (fm.dwRBitMask == 0xF800 && fm.dwGBitMask == 0x07E0 && fm.dwBBitMask == 0x001F)
            temp.hi15 = 0;
         else if (fm.dwRBitMask == 0x7C00 && fm.dwGBitMask == 0x03E0 && fm.dwBBitMask == 0x001F)
            temp.hi15 = 1;
         else
            errexit("invalid pixel format (need RGB:5-6-5 or URGB:1-5-5-5)");

      }
      else if (temp.obpp == 8)
      {

         if ((r = dd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, syspalette, &pal, 0)) != DD_OK)
         { printrdd("IDirectDraw2::CreatePalette()", r); exit(); }
         if ((r = surf0->SetPalette(pal)) != DD_OK)
         { printrdd("IDirectDrawSurface2::SetPalette()", r); exit(); }
      }
   }

   if (conf.flip && !(temp.rflags & (RF_GDI|RF_CLIP|RF_D3D)))
   {
      DDSCAPS caps = { DDSCAPS_BACKBUFFER };
      if ((r = surf0->GetAttachedSurface(&caps, &surf1)) != DD_OK)
      { printrdd("IDirectDraw2::GetAttachedSurface()", r); exit(); }
   }

   // Настраиваем функцию конвертирования из текущего формата в BGR24
   switch(temp.obpp)
   {
   case 8: ConvBgr24 = ConvPal8ToBgr24; break;
   case 16:
       switch(temp.hi15)
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

HRESULT SetDIDwordProperty(LPDIRECTINPUTDEVICE pdev, REFGUID guidProperty,
                   DWORD dwObject, DWORD dwHow, DWORD dwValue)
{
   DIPROPDWORD dipdw;
   dipdw.diph.dwSize       = sizeof(dipdw);
   dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
   dipdw.diph.dwObj        = dwObject;
   dipdw.diph.dwHow        = dwHow;
   dipdw.dwData            = dwValue;
   return pdev->SetProperty(guidProperty, &dipdw.diph);
}

BOOL CALLBACK InitJoystickInput(LPCDIDEVICEINSTANCE pdinst, LPVOID pvRef)
{
   HRESULT r;
   LPDIRECTINPUT pdi = (LPDIRECTINPUT)pvRef;
   LPDIRECTINPUTDEVICE dijoyst1;
   LPDIRECTINPUTDEVICE2 dijoyst;
   if ((r = pdi->CreateDevice(pdinst->guidInstance, &dijoyst1, 0)) != DI_OK)
   {
       printrdi("IDirectInput::CreateDevice() (joystick)", r);
       return DIENUM_CONTINUE;
   }

   r = dijoyst1->QueryInterface(IID_IDirectInputDevice2, (void**)&dijoyst);
   if (r != S_OK)
   {
      printrdi("IDirectInputDevice::QueryInterface(IID_IDirectInputDevice2) [dx5 not found]", r);
      dijoyst1->Release();
      dijoyst1=0;
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
   JoyId.diph.dwSize       = sizeof(JoyId);
   JoyId.diph.dwHeaderSize = sizeof(JoyId.diph);
   JoyId.diph.dwObj        = 0;
   JoyId.diph.dwHow        = DIPH_DEVICE;
   if ((r = dijoyst->GetProperty(DIPROP_JOYSTICKID, &JoyId.diph)) != DI_OK)
   { printrdi("IDirectInputDevice::GetProperty(DIPROP_JOYSTICKID)", r); exit(); }

   trim_right(dide.tszInstanceName);
   trim_right(dide.tszProductName);

   CharToOem(dide.tszInstanceName, dide.tszInstanceName);
   CharToOem(dide.tszProductName, dide.tszProductName);
   if (strcmp(dide.tszProductName, dide.tszInstanceName))
       strcat(dide.tszInstanceName, ", ");
   else
       dide.tszInstanceName[0] = 0;

   bool UseJoy = (JoyId.dwData == conf.input.JoyId);
   color(CONSCLR_HARDINFO);
   printf("%cjoy(%u): %s%s (%d axes, %d buttons, %d POVs)\n", UseJoy ? '*' : ' ', JoyId.dwData,
      dide.tszInstanceName, dide.tszProductName, dc.dwAxes, dc.dwButtons, dc.dwPOVs);

   if (UseJoy)
   {
       if ((r = dijoyst->SetDataFormat(&c_dfDIJoystick)) != DI_OK)
       {
           printrdi("IDirectInputDevice::SetDataFormat() (joystick)", r);
           exit();
       }

       if ((r = dijoyst->SetCooperativeLevel(wnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)) != DI_OK)
       {
           printrdi("IDirectInputDevice::SetCooperativeLevel() (joystick)", r);
           exit();
       }

       DIPROPRANGE diprg;
       diprg.diph.dwSize       = sizeof(diprg);
       diprg.diph.dwHeaderSize = sizeof(diprg.diph);
       diprg.diph.dwObj        = DIJOFS_X;
       diprg.diph.dwHow        = DIPH_BYOFFSET;
       diprg.lMin              = -1000;
       diprg.lMax              = +1000;

       if ((r = dijoyst->SetProperty(DIPROP_RANGE, &diprg.diph)) != DI_OK)
       { printrdi("IDirectInputDevice::SetProperty(DIPH_RANGE)", r); exit(); }

       diprg.diph.dwObj        = DIJOFS_Y;

       if ((r = dijoyst->SetProperty(DIPROP_RANGE, &diprg.diph)) != DI_OK)
       { printrdi("IDirectInputDevice::SetProperty(DIPH_RANGE) (y)", r); exit(); }

       if ((r = SetDIDwordProperty(dijoyst, DIPROP_DEADZONE, DIJOFS_X, DIPH_BYOFFSET, 2000)) != DI_OK)
       { printrdi("IDirectInputDevice::SetProperty(DIPH_DEADZONE)", r); exit(); }

       if ((r = SetDIDwordProperty(dijoyst, DIPROP_DEADZONE, DIJOFS_Y, DIPH_BYOFFSET, 2000)) != DI_OK)
       { printrdi("IDirectInputDevice::SetProperty(DIPH_DEADZONE) (y)", r); exit(); }
       ::dijoyst = dijoyst;
   }
   else
   {
      dijoyst->Release();
   }
   return DIENUM_CONTINUE;
}

HRESULT WINAPI callb(LPDDSURFACEDESC surf, void *lpContext)
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
    switch(temp.scale)
    {
    default:
    case 1: cmd = SCU_SCALE1; break;
    case 2: cmd = SCU_SCALE2; break;
    case 3: cmd = SCU_SCALE3; break;
    case 4: cmd = SCU_SCALE4; break;
    }
    SendMessage(wnd, WM_SYSCOMMAND, cmd, 0); // set window size
}

static void StartD3d(HWND Wnd)
{
    HRESULT r;
    if (!D3d9Dll)
        D3d9Dll = LoadLibrary("d3d9.dll");

    typedef IDirect3D9 * (WINAPI *TDirect3DCreate9)(UINT SDKVersion);
    TDirect3DCreate9 Direct3DCreate9 = (TDirect3DCreate9)GetProcAddress(D3d9Dll, "Direct3DCreate9");
    D3d9 = Direct3DCreate9(DIRECT3D_VERSION);
    if (!D3d9)
        return;

    D3DDISPLAYMODE DispMode;
    r = D3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &DispMode);
    if (r != DD_OK)
    { printrdd("IDirect3D::GetAdapterDisplayMode()", r); exit(); }

    D3DPRESENT_PARAMETERS D3dPp = { 0 };
    D3dPp.Windowed = TRUE; // FALSE;
    D3dPp.SwapEffect = D3DSWAPEFFECT_FLIP; //D3DSWAPEFFECT_DISCARD;
    D3dPp.BackBufferCount = 2;
    D3dPp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
//    D3dPp.BackBufferWidth        = 640;
//    D3dPp.BackBufferHeight       = 480;
//    D3dPp.BackBufferFormat       = D3DFMT_X8R8G8B8;
    D3dPp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; //conf.flip ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
    D3dPp.BackBufferFormat = DispMode.Format;

    r = D3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, Wnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &D3dPp, &D3dDev);
    if (r != DD_OK)
    { printrdd("IDirect3D::CreateDevice()", r); exit(); }
}

static void CalcWindowSize()
{
    temp.rflags = renders[conf.render].flags;

    if (renders[conf.render].func == render_advmame)
    {
        if (conf.videoscale == 2)
            temp.rflags |= RF_2X;
        if (conf.videoscale == 3)
            temp.rflags |= RF_3X;
        if (conf.videoscale == 4)
            temp.rflags |= RF_4X;
    }
    if (temp.rflags & RF_DRIVER)
        temp.rflags |= drivers[conf.driver].flags;

    // select resolution
    static const int size_x[3] = { 256, 320, 384 };
    static const int size_y[3] = { 192, 240, 300 };
    temp.ox = temp.scx = size_x[conf.bordersize];
    temp.oy = temp.scy = size_y[conf.bordersize];

    if (temp.rflags & RF_2X)
    {
        temp.ox *=2; temp.oy *= 2;
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

   wc.lpfnWndProc = (WNDPROC)WndProc;
   hIn = wc.hInstance = GetModuleHandle(0);
   wc.lpszClassName = "EMUL_WND";
   wc.hIcon = LoadIcon(hIn, MAKEINTRESOURCE(IDI_MAIN));
   wc.hCursor = LoadCursor(0, IDC_ARROW);
   RegisterClass(&wc);

   for (int i = 0; i < 9; i++)
      crs[i] = LoadCursor(hIn, MAKEINTRESOURCE(IDC_C0+i));
//Alone Coder 0.36.6
   RECT rect1;
   SystemParametersInfo(SPI_GETWORKAREA, 0, &rect1, 0);
//~
   CalcWindowSize();

   int cx = temp.ox*temp.scale, cy = temp.oy*temp.scale;

   RECT Client = { 0, 0, cx, cy };
   AdjustWindowRect(&Client, WS_VISIBLE | WS_OVERLAPPEDWINDOW, 0);
   cx = Client.right - Client.left;
   cy = Client.bottom - Client.top;
   int winx = rect1.left + (rect1.right - rect1.left - cx) / 2;
   int winy = rect1.top + (rect1.bottom - rect1.top - cy) / 2;

   wnd = CreateWindow("EMUL_WND", "UnrealSpeccy", WS_VISIBLE|WS_OVERLAPPEDWINDOW,
                    winx, winy, cx, cy, 0, 0, hIn, NULL);

   DragAcceptFiles(wnd, 1);

   temp.gdidc = GetDC(wnd);
   GetSystemPaletteEntries(temp.gdidc, 0, 0x100, syspalette);

   HMENU sys = GetSystemMenu(wnd, 0);
   AppendMenu(sys, MF_SEPARATOR, 0, 0);
   AppendMenu(sys, MF_STRING, SCU_SCALE1, "x1");
   AppendMenu(sys, MF_STRING, SCU_SCALE2, "x2");
   AppendMenu(sys, MF_STRING, SCU_SCALE3, "x3");
   AppendMenu(sys, MF_STRING, SCU_SCALE4, "x4");
   AppendMenu(sys, MF_STRING, SCU_LOCK_MOUSE, "&Lock mouse");

   InitCommonControls();

   HRESULT r;
   LPDIRECTDRAW dd0;

   HMODULE hDdraw = LoadLibrary("ddraw.dll");
   if (hDdraw)
   {
        typedef HRESULT(WINAPI * DIRECTDRAWCREATE) (LPGUID, LPDIRECTDRAW *, LPUNKNOWN);
        DIRECTDRAWCREATE DirectDrawCreate = (DIRECTDRAWCREATE)GetProcAddress(hDdraw, "DirectDrawCreate");
        if (DirectDrawCreate) r = DirectDrawCreate(0, &dd0, 0);
   }

   if (r != DD_OK)
   { printrdd("DirectDrawCreate()", r); exit(); }

   if ((r = dd0->QueryInterface(IID_IDirectDraw2, (void**)&dd)) != DD_OK)
   { printrdd("IDirectDraw::QueryInterface(IID_IDirectDraw2)", r); exit(); }

   dd0->Release();

   if ((temp.rflags & RF_D3D))
       StartD3d(wnd);

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
   dd->GetCaps(&caps, 0);

   color(CONSCLR_HARDINFO);
   printf("%s%dMb VRAM available\n", vmodel, (caps.dwVidMemTotal+512*1024)/(1024*1024));

   max_modes = 0;
   dd->EnumDisplayModes(DDEDM_REFRESHRATES | DDEDM_STANDARDVGAMODES, 0, 0, callb);

   WAVEFORMATEX wf = { 0 };
   wf.wFormatTag = WAVE_FORMAT_PCM;
   wf.nSamplesPerSec = conf.sound.fq;
   wf.nChannels = 2;
   wf.wBitsPerSample = 16;
   wf.nBlockAlign = 4;
   wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

   if (conf.sound.do_sound == do_sound_wave) {
      if ((r = waveOutOpen(&hwo, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL)) != MMSYSERR_NOERROR)
      { printrmm("waveOutOpen()", r); hwo = 0; goto sfail; }
      wqhead = 0, wqtail = 0;
   } else if (conf.sound.do_sound == do_sound_ds) {

      HMODULE hDdraw = LoadLibrary("dsound.dll");
      if (hDdraw)
      {
          typedef HRESULT(WINAPI * DIRECTSOUNDCREATE) (LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
          DIRECTSOUNDCREATE DirectSoundCreate = (DIRECTSOUNDCREATE)GetProcAddress(hDdraw, "DirectSoundCreate");
          if (DirectSoundCreate) r = DirectSoundCreate(0, &ds, 0);
      }

      if (r != DD_OK)
      { printrds("DirectSoundCreate()", r); goto sfail; }

      r = -1;
      if (conf.sound.dsprimary) r = ds->SetCooperativeLevel(wnd, DSSCL_WRITEPRIMARY);
      if (r != DS_OK) r = ds->SetCooperativeLevel(wnd, DSSCL_NORMAL), conf.sound.dsprimary = 0;
      if (r != DS_OK) { printrds("IDirectSound::SetCooperativeLevel()", r); goto sfail; }

      DSBUFFERDESC dsdesc = { sizeof(DSBUFFERDESC) }; r = -1;

      if (conf.sound.dsprimary) {

         dsdesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_PRIMARYBUFFER;
         dsdesc.dwBufferBytes = 0;
         dsdesc.lpwfxFormat = 0;
         r = ds->CreateSoundBuffer(&dsdesc, &dsbf, 0);

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
         dsbuffer = dsdesc.dwBufferBytes = DSBUFFER;
         if ((r = ds->CreateSoundBuffer(&dsdesc, &dsbf, 0)) != DS_OK)
         {
             printrds("IDirectSound::CreateSoundBuffer()", r);
             goto sfail;
         }

         conf.sound.dsprimary = 0;
      }

      dsoffset = dsbuffer/4;

   } else {
   sfail:
      conf.sound.do_sound = do_sound_none;
   }

   LPDIRECTINPUT di;
   r = DirectInputCreate(hIn,0x0500,&di,0);

   if ((r != DI_OK) && (r = DirectInputCreate(hIn,0x0300,&di,0)) != DI_OK)
   {
       printrdi("DirectInputCreate()", r);
       exit();
   }

   if ((r = di->CreateDevice(GUID_SysKeyboard, &dikeyboard, 0)) != DI_OK)
   {
       printrdi("IDirectInputDevice::CreateDevice() (keyboard)", r);
       exit();
   }

   if ((r = dikeyboard->SetDataFormat(&c_dfDIKeyboard)) != DI_OK)
   {
       printrdi("IDirectInputDevice::SetDataFormat() (keyboard)", r);
       exit();
   }

   if ((r = dikeyboard->SetCooperativeLevel(wnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)) != DI_OK)
   {
       printrdi("IDirectInputDevice::SetCooperativeLevel() (keyboard)", r);
       exit();
   }


   if ((r = di->CreateDevice(GUID_SysMouse, &dimouse, 0)) == DI_OK)
   {
      if ((r = dimouse->SetDataFormat(&c_dfDIMouse)) != DI_OK)
      {
          printrdi("IDirectInputDevice::SetDataFormat() (mouse)", r);
          exit();
      }

      if ((r = dimouse->SetCooperativeLevel(wnd, DISCL_FOREGROUND|DISCL_NONEXCLUSIVE)) != DI_OK)
      {
          printrdi("IDirectInputDevice::SetCooperativeLevel() (mouse)", r);
          exit();
      }
      DIPROPDWORD dipdw = { 0 };
      dipdw.diph.dwSize       = sizeof(dipdw);
      dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
      dipdw.diph.dwHow        = DIPH_DEVICE;
      dipdw.dwData            = DIPROPAXISMODE_ABS;
      if ((r = dimouse->SetProperty(DIPROP_AXISMODE, &dipdw.diph)) != DI_OK)
      {
          printrdi("IDirectInputDevice::SetProperty() (mouse)", r);
          exit();
      }
   }
   else
   {
       color(CONSCLR_WARNING);
       printf("warning: no mouse\n");
       dimouse = 0;
   }

   if ((r = di->EnumDevices(DIDEVTYPE_JOYSTICK, InitJoystickInput, di, DIEDFL_ATTACHEDONLY)) != DI_OK)
   {
       printrdi("IDirectInput::EnumDevices(DIDEVTYPE_JOYSTICK,...)", r);
       exit();
   }

   di->Release();
//[vv]   SetKeyboardState(kbdpc); // fix bug in win95
}

static void DoneD3d(bool DeInitDll)
{
    if (SurfTexture)
    {
        SurfTexture->Release();
        SurfTexture = 0;
    }
    if (D3dDev)
    {
        D3dDev->Release();
        D3dDev = 0;
    }
    if (D3d9)
    {
        D3d9->Release();
        D3d9 = 0;
    }
    if (DeInitDll && D3d9Dll)
    {
        FreeLibrary(D3d9Dll);
        D3d9Dll = 0;
    }
}

void done_dx()
{
   sound_stop();
   if (pal) pal->Release(); pal = 0;
   if (surf1) surf1->Release(); surf1 = 0;
   if (surf0) surf0->Release(); surf0 = 0;
   if (sprim) sprim->Release(); sprim = 0;
   if (clip) clip->Release(); clip = 0;
   if (dd) dd->Release(); dd = 0;
   if (dikeyboard) dikeyboard->Unacquire(), dikeyboard->Release(); dikeyboard = 0;
   if (dimouse) dimouse->Unacquire(), dimouse->Release(); dimouse = 0;
   if (dijoyst) dijoyst->Unacquire(), dijoyst->Release(); dijoyst = 0;
   if (hwo) { waveOutReset(hwo); /* waveOutUnprepareHeader()'s ? */ waveOutClose(hwo); }
   if (dsbf) dsbf->Release(); dsbf = 0;
   if (ds) ds->Release(); ds = 0;
   if (hbm) DeleteObject(hbm); hbm = 0;
   if (temp.gdidc) ReleaseDC(wnd, temp.gdidc); temp.gdidc = 0;
   DoneD3d();
   if (wnd) DestroyWindow(wnd);
}
