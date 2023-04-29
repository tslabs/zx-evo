#pragma once

#include "sndrender.h"

#ifdef SND_EXTERNAL_BUFFER

// to get available samples in external buffer, common for all SNDRENDERs,
// call begin(), count() for each SNDRENDER (SNDCHIP) object,
// and end() to get data position and size

class SNDCOUNTER
{
 public:
   void reset();
   SNDCOUNTER() { reset(); } // ctor

   void begin();
   void count(SNDRENDER &render);
   void end(bufptr_t &start_offset, unsigned &n_samples);

 private:
   bufptr_t bufstart;
   unsigned n_samples;
};

#endif // SND_EXTERNAL_BUFFER

#if 0 // USAGE EXAMPLE

  #define USE_SND_EXTERNAL_BUFFER
  #include "sndrender/*.h"
  #include "sndrender/*.cpp"

  SNDCHIP ay1, ay2;
  SNDRENDER beeper;
  SNDCOUNTER cnt;

  // global emulation loop
  for (;;) {
     ay1.start_frame();
     ay2.start_frame();
     beeper.start_frame();

     // Z80 emulation before INT
     for (int t = 0; t < 71680; t++) {
        ay1.select(0);
        ay1.write(t, t % 100);
        ay2.select(3);
        ay2.write(t, t % 100);
        beeper.update(t, t % 4000, t % 400);
     }
     ay1.end_frame(t);
     ay2.end_frame(t);
     beeper.end_frame(t);

     cnt.begin();
     cnt.count(ay1);
     cnt.count(ay2);
     cnt.count(beeper);
     unsigned bufplay, n_samples;
     cnt.end(bufplay, n_samples);

     unsigned sndplaybuf[10000];
     for (unsigned k = 0; k < n_samples; k++, bufplay++) {
        sndplaybuf[k] = sndbuf[bufplay & (SNDBUFSIZE-1)];
        sndbuf[bufplay & (SNDBUFSIZE-1)] = 0;
     }
     spbsize = n_samples*4;
     wav_play((SNDSAMPLE*)sndplaybuf, n_samples);
  }

#endif
