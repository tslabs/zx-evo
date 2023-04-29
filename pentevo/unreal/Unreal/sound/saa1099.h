#pragma once

#ifndef __SAA1099_H__
#define __SAA1099_H__

#include "sysdefs.h"
#include "sndrender.h"

/**********************************************
    Philips SAA1099 Sound driver
**********************************************/
/* this structure defines a channel */
struct saa1099_channel
{
    int frequency;          /* frequency (0x00..0xff) */
    int freq_enable;        /* frequency enable */
    int noise_enable;       /* noise enable */
    int octave;             /* octave (0x00..0x07) */
    int amplitude[2];       /* value from amplitude lookup table */
    int amp[2];             /* amplitude (0x00..0x0f) */
    int envelope[2];        /* envelope (0x00..0x0f or 0x10 == off) */

    /* vars to simulate the square wave */
    double counter;
    double freq;
    int level;
};

/* this structure defines a noise channel */
struct saa1099_noise
{
    /* vars to simulate the noise generator output */
    double counter;
    double freq;
    int level;                      /* noise polynomal shifter */
};

/* this structure defines a SAA1099 chip */
struct saa1099_state
{
//    running_device *device;
//    sound_stream * stream;          /* our stream */
    int noise_params[2];            /* noise generators parameters */
    int env_enable[2];              /* envelope generators enable */
    int env_reverse_right[2];       /* envelope reversed for right channel */
    int env_reverse_right_buf[2];   /* envelope reversed for right channel buffered value */
    int env_mode[2];                /* envelope generators mode */
    int env_mode_buf[2];            /* envelope generators mode buffered value */
    int env_bits[2];                /* non zero = 3 bits resolution */
    int env_clock[2];               /* envelope clock mode (non-zero external) */
    int env_clock_buf[2];           /* envelope clock mode (non-zero external) buffered value */
    int env_step[2];                /* current envelope step */
    bool env_upd[2];                // true if buffered data present
    int all_ch_enable;              /* all channels enable */
    int sync_state;                 /* sync all channels */
    int selected_reg;               /* selected register */
    saa1099_channel channels[6];    /* channels */
    saa1099_noise noise[2]; /* noise generators */
    double sample_rate;
};

// 8MHz in sam coupe
class TSaa1099 : public saa1099_state,  public SNDRENDER
{
private:
   unsigned chip_clock_rate;
   unsigned system_clock_rate;
   u64 passed_chip_ticks;
   u64 passed_clk_ticks;

   unsigned t;
public:
   TSaa1099();

   void set_timings(unsigned system_clock_rate, unsigned chip_clock_rate, unsigned sample_rate);
   void reset(unsigned TimeStamp = 0);

   // set of functions that fills buffer in emulation progress
   void start_frame() { SNDRENDER::start_frame(); }
   void start_frame(bufptr_t dst);
   unsigned end_frame(unsigned clk_ticks);
   void WrCtl(u8 Val);
   void WrData(unsigned TimeStamp, u8 Val);
private:
   void update(unsigned TimeStamp);
   void flush(unsigned chiptick);
};

extern TSaa1099 Saa1099;

//void saa1099_control_w(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED u8 data);
//void saa1099_data_w(ATTR_UNUSED device_t *device, ATTR_UNUSED offs_t offset, ATTR_UNUSED u8 data);

#endif /* __SAA1099_H__ */
