#include "std.h"
#include "sndcounter.h"

#ifdef SND_EXTERNAL_BUFFER

//#define SND_TEST_FAILURES // disable after careful debug
//#define SND_TEST_SHOWSTAT

void SNDCOUNTER::begin()
{
   n_samples = SND_EXTERNAL_BUFFER_SIZE;
   #ifdef SND_TEST_SHOWSTAT
   printf("CNT:");
   #endif // SND_TEST_SHOWSTAT
}

void SNDCOUNTER::count(SNDRENDER &render)
{
   unsigned rendsamples = (render.dstpos - bufstart) & (SND_EXTERNAL_BUFFER_SIZE-1);
//   assert(rendsamples != 0);
   if (rendsamples < n_samples)
       n_samples = rendsamples;
   #ifdef SND_TEST_FAILURES
   unsigned lastframe_samples = (render.dstpos - render.dst_start) & (SND_EXTERNAL_BUFFER_SIZE-1);
   if (lastframe_samples > rendsamples)
      errexit("SNDRENDER object is out of sync with other sound objects");
   #endif // SND_TEST_FAILURES
   #ifdef SND_TEST_SHOWSTAT
   printf(" %I64d", render.passed_snd_ticks);
   #endif // SND_TEST_SHOWSTAT
}

void SNDCOUNTER::end(bufptr_t &start_offset, unsigned &n_samples)
{
   start_offset = bufstart;
   n_samples = SNDCOUNTER::n_samples;
   bufstart = (bufstart + n_samples) & (SND_EXTERNAL_BUFFER_SIZE-1);
   #ifdef SND_TEST_SHOWSTAT
   printf("\n");
   #endif // SND_TEST_SHOWSTAT
}

void SNDCOUNTER::reset()
{
   bufstart = 0;
}

#endif // SND_EXTERNAL_BUFFER
