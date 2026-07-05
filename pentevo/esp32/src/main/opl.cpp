#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_console.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "spi_flash_mmap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "opl.h"

#define OPL_TAG "opl"
#define OPL_COMMAND_QUEUE_LEN 64
#define OPL_MOONSOUND_VOLUME (32767 * 2 / 10)
#define OPL_HOT_ALLOC_CAPS (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
#define OPL_FM_TAB_ALLOC_CAPS (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define OPL_VOLUME_DEFAULT 50
#define OPL_VOLUME_BASE 50

u8 opl_global_volume = OPL_VOLUME_DEFAULT;

// YMF262 FM core, adapted from moon.zip/openMSX code.
// The original internal output buffer is removed; render() accumulates directly into the shared float mix buffer.

typedef unsigned long  EmuTime;

#define MAX_BUFFER_SIZE 10000


class TimerCallback
{
  public:
    virtual void callback(u8 value) = 0;
};

extern void moonsoundTimerSet(void* ref, int timer, int count);
extern void moonsoundTimerStart(void* ref, int timer, int start, u8 timerRef);

template<int freq, u8 flag>
class Timer
{
  public:
        Timer(TimerCallback *cb, void* reference) {
            ref = reference;
            id = 12500 / freq;
        }
    virtual ~Timer() {}
    void setValue(u8 value) {
            //moonsoundTimerSet(ref, id, id * (256 - value));
        }
    void setStart(bool start, const EmuTime &time) {
            //moonsoundTimerStart(ref, id, start, flag);
        }

  private:
        void* ref;
        int id;
};


class IRQHelper 
{
public:
    IRQHelper() {}
    void set() {
        //boardSetInt(0x8);
    }
    void reset() {
        //boardClearInt(0x8);
    }
};

#ifndef OPENMSX_SOUNDDEVICE
#define OPENMSX_SOUNDDEVICE

class SoundDevice
{
  public:
        SoundDevice() : internalMuted(true) {}
    void setVolume(short newVolume) {
          setInternalVolume(newVolume);
        }

  protected:
    virtual void setInternalVolume(short newVolume) = 0;
        void setInternalMute(bool muted) { internalMuted = muted; }
        bool isInternalMuted() const { return internalMuted; }
  public:
    virtual void setSampleRate(int newSampleRate, int Oversampling) = 0;
    virtual int* updateBuffer(int length) = 0;

  private:
    bool internalMuted;
};

#endif

class YMF262Slot
{
  public:
    YMF262Slot();
    inline int volume_calc(u8 LFO_AM);
    inline void FM_KEYON(u8 key_set);
    inline void FM_KEYOFF(u8 key_clr);

    u8 ar;  // attack rate: AR<<2
    u8 dr;  // decay rate:  DR<<2
    u8 rr;  // release rate:RR<<2
    u8 KSR;  // key scale rate
    u8 ksl;  // keyscale level
    u8 ksr;  // key scale rate: kcode>>KSR
    u8 mul;  // multiple: mul_tab[ML]

    // Phase Generator 
    unsigned int Cnt;  // frequency counter
    unsigned int Incr;  // frequency counter step
    u8 FB;  // feedback shift value
    int op1_out[2];  // slot1 output for feedback
    u8 CON;  // connection (algorithm) type

    // Envelope Generator 
    u8 eg_type;  // percussive/non-percussive mode 
    u8 state;  // phase type
    unsigned int TL;  // total level: TL << 2
    int TLL;  // adjusted now TL
    int volume;  // envelope counter
    int sl;    // sustain level: sl_tab[SL]

    unsigned int eg_m_ar;// (attack state)
    u8 eg_sh_ar;  // (attack state)
    u8 eg_sel_ar;  // (attack state)
    unsigned int eg_m_dr;// (decay state)
    u8 eg_sh_dr;  // (decay state)
    u8 eg_sel_dr;  // (decay state)
    unsigned int eg_m_rr;// (release state)
    u8 eg_sh_rr;  // (release state)
    u8 eg_sel_rr;  // (release state)

    u8 key;  // 0 = KEY OFF, >0 = KEY ON

    // LFO 
    u8  AMmask;  // LFO Amplitude Modulation enable mask 
    u8 vib;  // LFO Phase Modulation enable flag (active high)

    // waveform select 
    u8 waveform_number;
    unsigned int wavetable;

    int connect;  // slot output pointer
};

class YMF262Channel
{
  public:
    YMF262Channel();
    void chan_calc(u8 LFO_AM);
    void chan_calc_ext(u8 LFO_AM);
    void CALC_FCSLOT(YMF262Slot &slot);

    YMF262Slot slots[2];

    int block_fnum;  // block+fnum
    int fc;    // Freq. Increment base
    int ksl_base;  // KeyScaleLevel Base step
    u8 kcode;  // key code (for key scaling)

    // there are 12 2-operator channels which can be combined in pairs
    // to form six 4-operator channel, they are:
    //  0 and 3,
    //  1 and 4,
    //  2 and 5,
    //  9 and 12,
    //  10 and 13,
    //  11 and 14
    u8 extended;  // set to 1 if this channel forms up a 4op channel with another channel(only used by first of pair of channels, ie 0,1,2 and 9,10,11) 
};

// Bitmask for register 0x04 
const int R04_ST1          = 0x01;  // Timer1 Start
const int R04_ST2          = 0x02;  // Timer2 Start
const int R04_MASK_T2      = 0x20;  // Mask Timer2 flag 
const int R04_MASK_T1      = 0x40;  // Mask Timer1 flag 
const int R04_IRQ_RESET    = 0x80;  // IRQ RESET 

// Bitmask for status register 
const int STATUS_T2      = R04_MASK_T2;
const int STATUS_T1      = R04_MASK_T1;

class YMF262 : public SoundDevice, public TimerCallback
{
  public:
    YMF262(short volume, const EmuTime &time, void* ref);
    virtual ~YMF262();
    
    virtual void reset(const EmuTime &time);
    void writeReg(int r, u8 v, const EmuTime &time);
    u8 peekReg(int reg);
    u8 readReg(int reg);
    u8 peekStatus();
    u8 readStatus();
    
    virtual void setInternalVolume(short volume);
    virtual void setSampleRate(int sampleRate, int Oversampling);
    virtual int* updateBuffer(int length);
    void setRenderMode(OPL_FM_RENDER_MODE mode);
    void render(float *mix, int length, int newSampleRate);

    void callback(u8 flag);

  private:
    void writeRegForce(int r, u8 v, const EmuTime &time);
    void init_tables(void);
    void setStatus(u8 flag);
    void resetStatus(u8 flag);
    void changeStatusMask(u8 flag);
    void advance_lfo();
    void advance();
    void set_fm_channel_active(int chan_no);
    void update_opl2_active_mask();
    void advance_opl2_melody();
    void render_opl2_melody(float *mix, int length);
    void chan_calc_rhythm(bool noise);
    void set_mul(u8 sl, u8 v);
    void set_ksl_tl(u8 sl, u8 v);
    void set_ar_dr(u8 sl, u8 v);
    void set_sl_rr(u8 sl, u8 v);
    void update_channels(YMF262Channel &ch);
    void checkMute();
    bool checkMuteHelper();

    IRQHelper irq;
    Timer<12500, STATUS_T1> timer1;  //  80us
    Timer< 3125, STATUS_T2> timer2;  // 320us

        int oplOversampling;
        int currentSampleRate;
        OPL_FM_RENDER_MODE renderMode;
        unsigned int fm_active_mask;

        YMF262Channel channels[18];  // OPL3 chips have 18 channels

    u8 reg[512];

        unsigned int pan[18*4];    // channels output masks (0xffffffff = enable); 4 masks per one channel 

    unsigned int eg_cnt;    // global envelope generator counter
    unsigned int eg_timer;    // global envelope generator counter works at frequency = chipclock/288 (288=8*36) 
    unsigned int eg_timer_add;    // step of eg_timer

    unsigned int fn_tab[1024];    // fnumber->increment counter

    // LFO 
    u8 LFO_AM;
    u8 LFO_PM;
    
    u8 lfo_am_depth;
    u8 lfo_pm_depth_range;
    unsigned int lfo_am_cnt;
    unsigned int lfo_am_inc;
    unsigned int lfo_pm_cnt;
    unsigned int lfo_pm_inc;

    unsigned int noise_rng;    // 23 bit noise shift register
    unsigned int noise_p;    // current noise 'phase'
    unsigned int noise_f;    // current noise period

    bool OPL3_mode;      // OPL3 extension enable flag
    u8 rhythm;      // Rhythm mode
    u8 nts;      // NTS (note select)

    u8 status;      // status flag
    u8 status2;
    u8 statusMask;    // status mask

    int chanout[20];    // 18 channels + two phase modulation
    short maxVolume;
};


const double PI = 3.14159265358979323846;

const int FREQ_SH   = 16;  // 16.16 fixed point (frequency calculations)
const int EG_SH     = 16;  // 16.16 fixed point (EG timing)
const int LFO_SH    = 24;  //  8.24 fixed point (LFO calculations)
const int TIMER_SH  = 16;  // 16.16 fixed point (timers calculations)
const int FREQ_MASK = (1 << FREQ_SH) - 1;
const unsigned int EG_TIMER_OVERFLOW = 1 << EG_SH;

// envelope output entries
const int ENV_BITS    = 10;
const int ENV_LEN     = 1 << ENV_BITS;
const double ENV_STEP = 128.0 / ENV_LEN;

const int MAX_ATT_INDEX = (1 << (ENV_BITS - 1)) - 1; //511
const int MIN_ATT_INDEX = 0;

// sinwave entries
const int SIN_BITS = 10;
const int SIN_LEN  = 1 << SIN_BITS;
const int SIN_MASK = SIN_LEN - 1;

const int TL_RES_LEN = 256;  // 8 bits addressing (real chip)

// register number to channel number , slot offset
const u8 SLOT1 = 0;
const u8 SLOT2 = 1;

// Envelope Generator phases
const int EG_ATT = 4;
const int EG_DEC = 3;
const int EG_SUS = 2;
const int EG_REL = 1;
const int EG_OFF = 0;


// mapping of register number (offset) to slot number used by the emulator
const int slot_array[32] =
{
   0,  2,  4,  1,  3,  5, -1, -1,
   6,  8, 10,  7,  9, 11, -1, -1,
  12, 14, 16, 13, 15, 17, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1
};

// key scale level
// table is 3dB/octave , DV converts this into 6dB/octave
// 0.1875 is bit 0 weight of the envelope counter (volume) expressed in the 'decibel' scale
#define DV(x) (int)(x / (0.1875/2.0))
const unsigned ksl_tab[8 * 16] =
{
  // OCT 0
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  // OCT 1
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 0.750), DV( 1.125), DV( 1.500),
  DV( 1.875), DV( 2.250), DV( 2.625), DV( 3.000),
  // OCT 2
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
  DV( 0.000), DV( 1.125), DV( 1.875), DV( 2.625),
  DV( 3.000), DV( 3.750), DV( 4.125), DV( 4.500),
  DV( 4.875), DV( 5.250), DV( 5.625), DV( 6.000),
  // OCT 3
  DV( 0.000), DV( 0.000), DV( 0.000), DV( 1.875),
  DV( 3.000), DV( 4.125), DV( 4.875), DV( 5.625),
  DV( 6.000), DV( 6.750), DV( 7.125), DV( 7.500),
  DV( 7.875), DV( 8.250), DV( 8.625), DV( 9.000),
  // OCT 4 
  DV( 0.000), DV( 0.000), DV( 3.000), DV( 4.875),
  DV( 6.000), DV( 7.125), DV( 7.875), DV( 8.625),
  DV( 9.000), DV( 9.750), DV(10.125), DV(10.500),
  DV(10.875), DV(11.250), DV(11.625), DV(12.000),
  // OCT 5 
  DV( 0.000), DV( 3.000), DV( 6.000), DV( 7.875),
  DV( 9.000), DV(10.125), DV(10.875), DV(11.625),
  DV(12.000), DV(12.750), DV(13.125), DV(13.500),
  DV(13.875), DV(14.250), DV(14.625), DV(15.000),
  // OCT 6 
  DV( 0.000), DV( 6.000), DV( 9.000), DV(10.875),
  DV(12.000), DV(13.125), DV(13.875), DV(14.625),
  DV(15.000), DV(15.750), DV(16.125), DV(16.500),
  DV(16.875), DV(17.250), DV(17.625), DV(18.000),
  // OCT 7 
  DV( 0.000), DV( 9.000), DV(12.000), DV(13.875),
  DV(15.000), DV(16.125), DV(16.875), DV(17.625),
  DV(18.000), DV(18.750), DV(19.125), DV(19.500),
  DV(19.875), DV(20.250), DV(20.625), DV(21.000)
};
#undef DV

// sustain level table (3dB per step) 
// 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)
#define SC(db) (unsigned) (db * (2.0/ENV_STEP))
const unsigned sl_tab[16] = {
 SC( 0), SC( 1), SC( 2), SC(3 ), SC(4 ), SC(5 ), SC(6 ), SC( 7),
 SC( 8), SC( 9), SC(10), SC(11), SC(12), SC(13), SC(14), SC(31)
};
#undef SC


const u8 RATE_STEPS = 8;
const u8 eg_inc[15 * RATE_STEPS] =
{
//cycle:0 1  2 3  4 5  6 7
    0,1, 0,1, 0,1, 0,1, //  0  rates 00..12 0 (increment by 0 or 1)
    0,1, 0,1, 1,1, 0,1, //  1  rates 00..12 1
    0,1, 1,1, 0,1, 1,1, //  2  rates 00..12 2
    0,1, 1,1, 1,1, 1,1, //  3  rates 00..12 3

    1,1, 1,1, 1,1, 1,1, //  4  rate 13 0 (increment by 1)
    1,1, 1,2, 1,1, 1,2, //  5  rate 13 1
    1,2, 1,2, 1,2, 1,2, //  6  rate 13 2
    1,2, 2,2, 1,2, 2,2, //  7  rate 13 3

    2,2, 2,2, 2,2, 2,2, //  8  rate 14 0 (increment by 2)
    2,2, 2,4, 2,2, 2,4, //  9  rate 14 1
    2,4, 2,4, 2,4, 2,4, // 10  rate 14 2
    2,4, 4,4, 2,4, 4,4, // 11  rate 14 3

    4,4, 4,4, 4,4, 4,4, // 12  rates 15 0, 15 1, 15 2, 15 3 for decay
    8,8, 8,8, 8,8, 8,8, // 13  rates 15 0, 15 1, 15 2, 15 3 for attack (zero time)
    0,0, 0,0, 0,0, 0,0, // 14  infinity rates for attack and decay(s)
};


#define O(a) (a*RATE_STEPS)
// note that there is no O(13) in this table - it's directly in the code
const u8 eg_rate_select[16 + 64 + 16] =
{
  // Envelope Generator rates (16 + 64 rates + 16 RKS)
  // 16 infinite time rates
  O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),
  O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),

  // rates 00-12
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),
  O( 0),O( 1),O( 2),O( 3),

  // rate 13 
  O( 4),O( 5),O( 6),O( 7),

  // rate 14 
  O( 8),O( 9),O(10),O(11),

  // rate 15 
  O(12),O(12),O(12),O(12),

  // 16 dummy rates (same as 15 3) 
  O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
  O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
};
#undef O

//rate  0,    1,    2,    3,   4,   5,   6,  7,  8,  9,  10, 11, 12, 13, 14, 15 
//shift 12,   11,   10,   9,   8,   7,   6,  5,  4,  3,  2,  1,  0,  0,  0,  0  
//mask  4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3,  1,  0,  0,  0,  0  
#define O(a) (a*1)
const u8 eg_rate_shift[16 + 64 + 16] =
{
  // Envelope Generator counter shifts (16 + 64 rates + 16 RKS) 
  // 16 infinite time rates 
  O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
  O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

  // rates 00-15 
  O(12),O(12),O(12),O(12),
  O(11),O(11),O(11),O(11),
  O(10),O(10),O(10),O(10),
  O( 9),O( 9),O( 9),O( 9),
  O( 8),O( 8),O( 8),O( 8),
  O( 7),O( 7),O( 7),O( 7),
  O( 6),O( 6),O( 6),O( 6),
  O( 5),O( 5),O( 5),O( 5),
  O( 4),O( 4),O( 4),O( 4),
  O( 3),O( 3),O( 3),O( 3),
  O( 2),O( 2),O( 2),O( 2),
  O( 1),O( 1),O( 1),O( 1),
  O( 0),O( 0),O( 0),O( 0),
  O( 0),O( 0),O( 0),O( 0),
  O( 0),O( 0),O( 0),O( 0),
  O( 0),O( 0),O( 0),O( 0),

  // 16 dummy rates (same as 15 3)
  O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
  O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
};
#undef O


// multiple table
#define ML(x) (u8)(2 * x)
const u8 mul_tab[16] =
{
  // 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,10,12,12,15,15
  ML( 0.5),ML( 1.0),ML( 2.0),ML( 3.0),ML( 4.0),ML( 5.0),ML( 6.0),ML( 7.0),
  ML( 8.0),ML( 9.0),ML(10.0),ML(10.0),ML(12.0),ML(12.0),ML(15.0),ML(15.0)
};
#undef ML

// TL_TAB_LEN is calculated as:
//  (12+1)=13 - sinus amplitude bits     (Y axis)
//  additional 1: to compensate for calculations of negative part of waveform
//  (if we don't add it then the greatest possible _negative_ value would be -2
//  and we really need -1 for waveform #7)
//  2  - sinus sign bit           (Y axis)
//  TL_RES_LEN - sinus resolution (X axis)

const int TL_TAB_LEN = 13 * 2 * TL_RES_LEN;
int *tl_tab = NULL;
const int ENV_QUIET = TL_TAB_LEN >> 4;
bool g_ymf262_tables_ready = false;

// sin waveform table in 'decibel' scale
// there are eight waveforms on OPL3 chips
unsigned int *sin_tab = NULL;

esp_err_t opl_alloc_fm_tables()
{
  if (tl_tab && sin_tab) return ESP_OK;

  if (!tl_tab)
  {
    tl_tab = (int *)heap_caps_malloc(sizeof(int) * TL_TAB_LEN, OPL_FM_TAB_ALLOC_CAPS);
    if (!tl_tab)
    {
      ESP_LOGE(OPL_TAG, "YMF262 tl_tab allocation failed: %u bytes", (unsigned)(sizeof(int) * TL_TAB_LEN));
      return ESP_ERR_NO_MEM;
    }
  }

  if (!sin_tab)
  {
    sin_tab = (unsigned int *)heap_caps_malloc(sizeof(unsigned int) * SIN_LEN * 8, OPL_FM_TAB_ALLOC_CAPS);
    if (!sin_tab)
    {
      ESP_LOGE(OPL_TAG, "YMF262 sin_tab allocation failed: %u bytes", (unsigned)(sizeof(unsigned int) * SIN_LEN * 8));
      heap_caps_free(tl_tab);
      tl_tab = NULL;
      return ESP_ERR_NO_MEM;
    }
  }

  g_ymf262_tables_ready = false;
  return ESP_OK;
}

void opl_free_fm_tables()
{
  if (tl_tab)
  {
    heap_caps_free(tl_tab);
    tl_tab = NULL;
  }

  if (sin_tab)
  {
    heap_caps_free(sin_tab);
    sin_tab = NULL;
  }

  g_ymf262_tables_ready = false;
}


// LFO Amplitude Modulation table (verified on real YM3812)
//  27 output levels (triangle waveform); 1 level takes one of: 192, 256 or 448 samples
//
// Length: 210 elements
//
// Each of the elements has to be repeated
// exactly 64 times (on 64 consecutive samples).
// The whole table takes: 64 * 210 = 13440 samples.
//
// When AM = 1 data is used directly
// When AM = 0 data is divided by 4 before being used (loosing precision is important)

const unsigned int LFO_AM_TAB_ELEMENTS = 210;
const u8 lfo_am_table[LFO_AM_TAB_ELEMENTS] =
{
  0,0,0,0,0,0,0,
  1,1,1,1,
  2,2,2,2,
  3,3,3,3,
  4,4,4,4,
  5,5,5,5,
  6,6,6,6,
  7,7,7,7,
  8,8,8,8,
  9,9,9,9,
  10,10,10,10,
  11,11,11,11,
  12,12,12,12,
  13,13,13,13,
  14,14,14,14,
  15,15,15,15,
  16,16,16,16,
  17,17,17,17,
  18,18,18,18,
  19,19,19,19,
  20,20,20,20,
  21,21,21,21,
  22,22,22,22,
  23,23,23,23,
  24,24,24,24,
  25,25,25,25,
  26,26,26,
  25,25,25,25,
  24,24,24,24,
  23,23,23,23,
  22,22,22,22,
  21,21,21,21,
  20,20,20,20,
  19,19,19,19,
  18,18,18,18,
  17,17,17,17,
  16,16,16,16,
  15,15,15,15,
  14,14,14,14,
  13,13,13,13,
  12,12,12,12,
  11,11,11,11,
  10,10,10,10,
  9,9,9,9,
  8,8,8,8,
  7,7,7,7,
  6,6,6,6,
  5,5,5,5,
  4,4,4,4,
  3,3,3,3,
  2,2,2,2,
  1,1,1,1
};

// LFO Phase Modulation table (verified on real YM3812) 
const int8_t lfo_pm_table[8 * 8 * 2] =
{
  // FNUM2/FNUM = 00 0xxxxxxx (0x0000)
  0, 0, 0, 0, 0, 0, 0, 0,  //LFO PM depth = 0
  0, 0, 0, 0, 0, 0, 0, 0,  //LFO PM depth = 1

  // FNUM2/FNUM = 00 1xxxxxxx (0x0080)
  0, 0, 0, 0, 0, 0, 0, 0,  //LFO PM depth = 0
  1, 0, 0, 0,-1, 0, 0, 0,  //LFO PM depth = 1

  // FNUM2/FNUM = 01 0xxxxxxx (0x0100)
  1, 0, 0, 0,-1, 0, 0, 0,  //LFO PM depth = 0
  2, 1, 0,-1,-2,-1, 0, 1,  //LFO PM depth = 1

  // FNUM2/FNUM = 01 1xxxxxxx (0x0180)
  1, 0, 0, 0,-1, 0, 0, 0,  //LFO PM depth = 0
  3, 1, 0,-1,-3,-1, 0, 1,  //LFO PM depth = 1

  // FNUM2/FNUM = 10 0xxxxxxx (0x0200)
  2, 1, 0,-1,-2,-1, 0, 1,  //LFO PM depth = 0
  4, 2, 0,-2,-4,-2, 0, 2,  //LFO PM depth = 1

  // FNUM2/FNUM = 10 1xxxxxxx (0x0280)
  2, 1, 0,-1,-2,-1, 0, 1,  //LFO PM depth = 0
  5, 2, 0,-2,-5,-2, 0, 2,  //LFO PM depth = 1

  // FNUM2/FNUM = 11 0xxxxxxx (0x0300)
  3, 1, 0,-1,-3,-1, 0, 1,  //LFO PM depth = 0
  6, 3, 0,-3,-6,-3, 0, 3,  //LFO PM depth = 1

  // FNUM2/FNUM = 11 1xxxxxxx (0x0380)
  3, 1, 0,-1,-3,-1, 0, 1,  //LFO PM depth = 0
  7, 3, 0,-3,-7,-3, 0, 3  //LFO PM depth = 1
};


int *g_ymf262_chan_out;
#define PHASE_MOD1 18
#define PHASE_MOD2 19



YMF262Slot::YMF262Slot()
{
  ar = dr = rr = KSR = ksl = ksr = mul = 0;
  Cnt = Incr = FB = op1_out[0] = op1_out[1] = CON = 0;
  connect = 0;
  eg_type = state = TL = TLL = volume = sl = 0;
  eg_m_ar = eg_sh_ar = eg_sel_ar = eg_m_dr = eg_sh_dr = 0;
  eg_sel_dr = eg_m_rr = eg_sh_rr = eg_sel_rr = 0;
  key = AMmask = vib = waveform_number = wavetable = 0;
}

YMF262Channel::YMF262Channel()
{
  block_fnum = fc = ksl_base = kcode = extended = 0;
}


void YMF262::callback(u8 flag)
{
  setStatus(flag);
}

// status set and IRQ handling
void YMF262::setStatus(u8 flag)
{
  // set status flag masking out disabled IRQs 
  status |= flag;
  if (status & statusMask) {
    status |= 0x80;
    
  }
}

// status reset and IRQ handling 
void YMF262::resetStatus(u8 flag)
{
  // reset status flag 
  status &= ~flag;
  if (!(status & statusMask)) {
    status &= 0x7F;
    
  }
}

// IRQ mask set
void YMF262::changeStatusMask(u8 flag)
{
  statusMask = flag;
  status &= statusMask;
  if (status) {
    status |= 0x80;
    
  } else {
    status &= 0x7F;
    
  }
}


// advance LFO to next sample
void IRAM_ATTR YMF262::advance_lfo()
{
  // LFO 
  lfo_am_cnt += lfo_am_inc;
  if (lfo_am_cnt >= (LFO_AM_TAB_ELEMENTS << LFO_SH)) {
    // lfo_am_table is 210 elements long 
    lfo_am_cnt -= (LFO_AM_TAB_ELEMENTS << LFO_SH);
  }

  u8 tmp = lfo_am_table[lfo_am_cnt >> LFO_SH];
  if (lfo_am_depth) {
    LFO_AM = tmp;
  } else {
    LFO_AM = tmp >> 2;
  }
  lfo_pm_cnt += lfo_pm_inc;
  LFO_PM = ((lfo_pm_cnt >> LFO_SH) & 7) | lfo_pm_depth_range;
}

// advance to next sample 
void YMF262::advance()
{
  eg_timer += eg_timer_add;

    if (eg_timer > 4 * EG_TIMER_OVERFLOW) {
        eg_timer = EG_TIMER_OVERFLOW;
    }
  while (eg_timer >= EG_TIMER_OVERFLOW) {
    eg_timer -= EG_TIMER_OVERFLOW;
    eg_cnt++;

    for (int i = 0; i < 18 * 2; i++) {
      YMF262Channel &ch = channels[i / 2];
      YMF262Slot &op = ch.slots[i & 1];
      // Envelope Generator 
      switch(op.state) {
      case EG_ATT:  // attack phase 
        if (!(eg_cnt & op.eg_m_ar)) {
          op.volume += (~op.volume * eg_inc[op.eg_sel_ar + ((eg_cnt >> op.eg_sh_ar) & 7)]) >> 3;
          if (op.volume <= MIN_ATT_INDEX) {
            op.volume = MIN_ATT_INDEX;
            op.state = EG_DEC;
          }
        }
        break;

      case EG_DEC:  // decay phase 
        if (!(eg_cnt & op.eg_m_dr)) {
          op.volume += eg_inc[op.eg_sel_dr + ((eg_cnt >> op.eg_sh_dr) & 7)];
          if (op.volume >= op.sl) {
            op.state = EG_SUS;
          }
        }
        break;

      case EG_SUS:  // sustain phase 
        // this is important behaviour:
        // one can change percusive/non-percussive
        // modes on the fly and the chip will remain
        // in sustain phase - verified on real YM3812 
        if (op.eg_type) {
          // non-percussive mode 
          // do nothing 
        } else {
          // percussive mode 
          // during sustain phase chip adds Release Rate (in percussive mode) 
          if (!(eg_cnt & op.eg_m_rr)) {
            op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt>>op.eg_sh_rr) & 7)];
            if (op.volume >= MAX_ATT_INDEX) {
              op.volume = MAX_ATT_INDEX;
            }
          } else {
            // do nothing in sustain phase
          }
        }
        break;

      case EG_REL:  // release phase 
        if (!(eg_cnt & op.eg_m_rr)) {
          op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt>>op.eg_sh_rr) & 7)];
          if (op.volume >= MAX_ATT_INDEX) {
            op.volume = MAX_ATT_INDEX;
            op.state = EG_OFF;
          }
        }
      break;

      default:
        break;
      }
    }
  }

    int i;
  for (i = 0; i < 18 * 2; i++) {
    YMF262Channel &ch = channels[i / 2];
    YMF262Slot &op = ch.slots[i & 1];

    // Phase Generator 
    if (op.vib) {
      u8 block;
      unsigned int block_fnum = ch.block_fnum;
      unsigned int fnum_lfo   = (block_fnum & 0x0380) >> 7;
      signed int lfo_fn_table_index_offset = lfo_pm_table[LFO_PM + 16 * fnum_lfo];

      if (lfo_fn_table_index_offset) {
        // LFO phase modulation active 
        block_fnum += lfo_fn_table_index_offset;
        block = (block_fnum & 0x1c00) >> 10;
        op.Cnt += (fn_tab[block_fnum & 0x03ff] >> (7 - block)) * op.mul;
      } else {
        // LFO phase modulation  = zero 
        op.Cnt += op.Incr;
      }
    } else {
      // LFO phase modulation disabled for this operator 
      op.Cnt += op.Incr;
    }
  }

  // The Noise Generator of the YM3812 is 23-bit shift register.
  // Period is equal to 2^23-2 samples.
  // Register works at sampling frequency of the chip, so output
  // can change on every sample.
  //
  // Output of the register and input to the bit 22 is:
  // bit0 XOR bit14 XOR bit15 XOR bit22
  //
  // Simply use bit 22 as the noise output.
  noise_p += noise_f;
    i = (noise_p >> FREQ_SH) & 0x1f;    // number of events (shifts of the shift register) 
  noise_p &= FREQ_MASK;
  while (i--) {
    // unsigned j = ( (noise_rng) ^ (noise_rng>>14) ^ (noise_rng>>15) ^ (noise_rng>>22) ) & 1;
    // noise_rng = (j<<22) | (noise_rng>>1);
    //
    // Instead of doing all the logic operations above, we
    // use a trick here (and use bit 0 as the noise output).
    // The difference is only that the noise bit changes one
    // step ahead. This doesn't matter since we don't know
    // what is real state of the noise_rng after the reset.

    if (noise_rng & 1) {
      noise_rng ^= 0x800302;
    }
    noise_rng >>= 1;
  }
}

void YMF262::set_fm_channel_active(int chan_no)
{
  if (chan_no < 0 || chan_no >= 18)
    return;

  fm_active_mask |= 1u << chan_no;
}

void IRAM_ATTR YMF262::update_opl2_active_mask()
{
  unsigned int mask = fm_active_mask & 0x1FFu;

  for (int chan_no = 0; chan_no < 9; chan_no++)
  {
    unsigned int bit = 1u << chan_no;
    if (!(mask & bit))
      continue;

    YMF262Channel &ch = channels[chan_no];
    if (ch.slots[SLOT1].state == EG_OFF && ch.slots[SLOT2].state == EG_OFF)
      fm_active_mask &= ~bit;
  }
}

void IRAM_ATTR YMF262::advance_opl2_melody()
{
  eg_timer += eg_timer_add;

  if (eg_timer > 4 * EG_TIMER_OVERFLOW)
    eg_timer = EG_TIMER_OVERFLOW;

  while (eg_timer >= EG_TIMER_OVERFLOW)
  {
    eg_timer -= EG_TIMER_OVERFLOW;
    eg_cnt++;

    unsigned int active_mask = fm_active_mask & 0x1FFu;
    for (int chan_no = 0; chan_no < 9; chan_no++)
    {
      if (!(active_mask & (1u << chan_no)))
        continue;

      YMF262Channel &ch = channels[chan_no];
      for (int slot_no = 0; slot_no < 2; slot_no++)
      {
        YMF262Slot &op = ch.slots[slot_no];

        switch(op.state)
        {
          case EG_ATT:
            if (!(eg_cnt & op.eg_m_ar))
            {
              op.volume += (~op.volume * eg_inc[op.eg_sel_ar + ((eg_cnt >> op.eg_sh_ar) & 7)]) >> 3;
              if (op.volume <= MIN_ATT_INDEX)
              {
                op.volume = MIN_ATT_INDEX;
                op.state = EG_DEC;
              }
            }
          break;

          case EG_DEC:
            if (!(eg_cnt & op.eg_m_dr))
            {
              op.volume += eg_inc[op.eg_sel_dr + ((eg_cnt >> op.eg_sh_dr) & 7)];
              if (op.volume >= op.sl)
                op.state = EG_SUS;
            }
          break;

          case EG_SUS:
            if (!op.eg_type && !(eg_cnt & op.eg_m_rr))
            {
              op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt >> op.eg_sh_rr) & 7)];
              if (op.volume >= MAX_ATT_INDEX)
                op.volume = MAX_ATT_INDEX;
            }
          break;

          case EG_REL:
            if (!(eg_cnt & op.eg_m_rr))
            {
              op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt >> op.eg_sh_rr) & 7)];
              if (op.volume >= MAX_ATT_INDEX)
              {
                op.volume = MAX_ATT_INDEX;
                op.state = EG_OFF;
              }
            }
          break;

          default:
          break;
        }
      }
    }
  }

  update_opl2_active_mask();

  unsigned int active_mask = fm_active_mask & 0x1FFu;
  for (int chan_no = 0; chan_no < 9; chan_no++)
  {
    if (!(active_mask & (1u << chan_no)))
      continue;

    YMF262Channel &ch = channels[chan_no];
    for (int slot_no = 0; slot_no < 2; slot_no++)
    {
      YMF262Slot &op = ch.slots[slot_no];

      if (op.vib)
      {
        unsigned int block_fnum = ch.block_fnum;
        unsigned int fnum_lfo = (block_fnum & 0x0380) >> 7;
        signed int lfo_fn_table_index_offset = lfo_pm_table[LFO_PM + 16 * fnum_lfo];

        if (lfo_fn_table_index_offset)
        {
          block_fnum += lfo_fn_table_index_offset;
          u8 block = (block_fnum & 0x1C00) >> 10;
          op.Cnt += (fn_tab[block_fnum & 0x03FF] >> (7 - block)) * op.mul;
        }
        else
          op.Cnt += op.Incr;
      }
      else
        op.Cnt += op.Incr;
    }
  }
}


signed int IRAM_ATTR op_calc(unsigned phase, unsigned env, signed int pm, unsigned int wave_tab)
{
  int i = (phase & ~FREQ_MASK) + (pm << 16);
  int p = (env << 4) + sin_tab[wave_tab + ((i >> FREQ_SH ) & SIN_MASK)];
  if (p >= TL_TAB_LEN) {
    return 0;
  }
  return tl_tab[p];
}

signed int IRAM_ATTR op_calc1(unsigned phase, unsigned int env, signed int pm, unsigned int wave_tab)
{
  int i = (phase & ~FREQ_MASK) + pm;
  int p = (env << 4) + sin_tab[wave_tab + ((i >> FREQ_SH) & SIN_MASK)];
  if (p >= TL_TAB_LEN) {
    return 0;
  }
  return tl_tab[p];
}

inline int YMF262Slot::volume_calc(u8 LFO_AM)
{
  return TLL + volume + (LFO_AM & AMmask);
}

// calculate output of a standard 2 operator channel
// (or 1st part of a 4-op channel) 
void IRAM_ATTR YMF262Channel::chan_calc(u8 LFO_AM)
{
  g_ymf262_chan_out[PHASE_MOD1] = 0;
  g_ymf262_chan_out[PHASE_MOD2] = 0;

  // SLOT 1 
  int env = slots[SLOT1].volume_calc(LFO_AM);
  int out = slots[SLOT1].op1_out[0] + slots[SLOT1].op1_out[1];
  slots[SLOT1].op1_out[0] = slots[SLOT1].op1_out[1];
  slots[SLOT1].op1_out[1] = 0;
  if (env < ENV_QUIET) {
    if (!slots[SLOT1].FB) {
      out = 0;
    }
    slots[SLOT1].op1_out[1] = op_calc1(slots[SLOT1].Cnt, env, (out<<slots[SLOT1].FB), slots[SLOT1].wavetable);
  }
  g_ymf262_chan_out[slots[SLOT1].connect] += slots[SLOT1].op1_out[1];

  // SLOT 2 
  env = slots[SLOT2].volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    g_ymf262_chan_out[slots[SLOT2].connect] += op_calc(slots[SLOT2].Cnt, env, g_ymf262_chan_out[PHASE_MOD1], slots[SLOT2].wavetable);
  }
}

// calculate output of a 2nd part of 4-op channel 
void YMF262Channel::chan_calc_ext(u8 LFO_AM)
{
  g_ymf262_chan_out[PHASE_MOD1] = 0;

  // SLOT 1
  int env  = slots[SLOT1].volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    g_ymf262_chan_out[slots[SLOT1].connect] += op_calc(slots[SLOT1].Cnt, env, g_ymf262_chan_out[PHASE_MOD2], slots[SLOT1].wavetable );
  }

  // SLOT 2
  env = slots[SLOT2].volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    g_ymf262_chan_out[slots[SLOT2].connect] += op_calc(slots[SLOT2].Cnt, env, g_ymf262_chan_out[PHASE_MOD1], slots[SLOT2].wavetable);
  }
}

// operators used in the rhythm sounds generation process:
//
// Envelope Generator:
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// / slot   number    TL ARDR SLRR Wave Drum  Hat   Drum  Tom  Cymbal
//  6 / 0   12        50  70   90   f0  +
//  6 / 1   15        53  73   93   f3  +
//  7 / 0   13        51  71   91   f1        +
//  7 / 1   16        54  74   94   f4              +
//  8 / 0   14        52  72   92   f2                    +
//  8 / 1   17        55  75   95   f5                          +
//
// Phase Generator:
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// / slot   number    MULTIPLE          Drum  Hat   Drum  Tom  Cymbal
//  6 / 0   12        30                +
//  6 / 1   15        33                +
//  7 / 0   13        31                      +     +           +
//  7 / 1   16        34                -----  n o t  u s e d -----
//  8 / 0   14        32                                  +
//  8 / 1   17        35                      +                 +
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// number   number    BLK/FNUM2 FNUM    Drum  Hat   Drum  Tom  Cymbal
//    6     12,15     B6        A6      +
//
//    7     13,16     B7        A7            +     +           +
//
//    8     14,17     B8        A8            +           +     +

// calculate rhythm 
void YMF262::chan_calc_rhythm(bool noise)
{
  YMF262Slot& SLOT6_1 = channels[6].slots[SLOT1];
  YMF262Slot& SLOT6_2 = channels[6].slots[SLOT2];
  YMF262Slot& SLOT7_1 = channels[7].slots[SLOT1];
  YMF262Slot& SLOT7_2 = channels[7].slots[SLOT2];
  YMF262Slot& SLOT8_1 = channels[8].slots[SLOT1];
  YMF262Slot& SLOT8_2 = channels[8].slots[SLOT2];

  // Bass Drum (verified on real YM3812):
  //  - depends on the channel 6 'connect' register:
  //      when connect = 0 it works the same as in normal (non-rhythm) mode (op1->op2->out)
  //      when connect = 1 _only_ operator 2 is present on output (op2->out), operator 1 is ignored
  //  - output sample always is multiplied by 2

  g_ymf262_chan_out[PHASE_MOD1] = 0;

  // SLOT 1 
  int env = SLOT6_1.volume_calc(LFO_AM);
  int out = SLOT6_1.op1_out[0] + SLOT6_1.op1_out[1];
  SLOT6_1.op1_out[0] = SLOT6_1.op1_out[1];

  if (!SLOT6_1.CON) {
    g_ymf262_chan_out[PHASE_MOD1] = SLOT6_1.op1_out[0];
  } else {
    // ignore output of operator 1
  }

  SLOT6_1.op1_out[1] = 0;
  if (env < ENV_QUIET) {
    if (!SLOT6_1.FB) {
      out = 0;
    }
    SLOT6_1.op1_out[1] = op_calc1(SLOT6_1.Cnt, env, (out << SLOT6_1.FB), SLOT6_1.wavetable);
  }

  // SLOT 2 
  env = SLOT6_2.volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    chanout[6] += op_calc(SLOT6_2.Cnt, env, g_ymf262_chan_out[PHASE_MOD1], SLOT6_2.wavetable) * 2;
  }

  // Phase generation is based on: 
  // HH  (13) channel 7->slot 1 combined with channel 8->slot 2 (same combination as TOP CYMBAL but different output phases)
  // SD  (16) channel 7->slot 1
  // TOM (14) channel 8->slot 1
  // TOP (17) channel 7->slot 1 combined with channel 8->slot 2 (same combination as HIGH HAT but different output phases)

  // Envelope generation based on: 
  // HH  channel 7->slot1
  // SD  channel 7->slot2
  // TOM channel 8->slot1
  // TOP channel 8->slot2

  // The following formulas can be well optimized.
  // I leave them in direct form for now (in case I've missed something).

  // High Hat (verified on real YM3812) 
  env = SLOT7_1.volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    // high hat phase generation:
    // phase = d0 or 234 (based on frequency only)
    // phase = 34 or 2d0 (based on noise)

    // base frequency derived from operator 1 in channel 7 
    bool bit7 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x80) != 0;
    bool bit3 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x08) != 0;
    bool bit2 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x04) != 0;
    bool res1 = ((bit2 ^ bit7) | bit3) != 0;
    // when res1 = 0 phase = 0x000 | 0xd0; 
    // when res1 = 1 phase = 0x200 | (0xd0>>2); 
    unsigned phase = res1 ? (0x200|(0xd0>>2)) : 0xd0;

    // enable gate based on frequency of operator 2 in channel 8 
    bool bit5e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x20) != 0;
    bool bit3e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x08) != 0;
    bool res2 = (bit3e ^ bit5e) != 0;
    // when res2 = 0 pass the phase from calculation above (res1); 
    // when res2 = 1 phase = 0x200 | (0xd0>>2); 
    if (res2) {
      phase = (0x200|(0xd0>>2));
    }

    // when phase & 0x200 is set and noise=1 then phase = 0x200|0xd0 
    // when phase & 0x200 is set and noise=0 then phase = 0x200|(0xd0>>2), ie no change 
    if (phase&0x200) {
      if (noise) {
        phase = 0x200|0xd0;
      }
    } else {
    // when phase & 0x200 is clear and noise=1 then phase = 0xd0>>2 
    // when phase & 0x200 is clear and noise=0 then phase = 0xd0, ie no change 
      if (noise) {
        phase = 0xd0>>2;
      }
    }
    chanout[7] += op_calc(phase<<FREQ_SH, env, 0, SLOT7_1.wavetable) * 2;
  }

  // Snare Drum (verified on real YM3812) 
  env = SLOT7_2.volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    // base frequency derived from operator 1 in channel 7 
    bool bit8 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x100) != 0;
    // when bit8 = 0 phase = 0x100; 
    // when bit8 = 1 phase = 0x200; 
    unsigned phase = bit8 ? 0x200 : 0x100;

    // Noise bit XOR'es phase by 0x100 
    // when noisebit = 0 pass the phase from calculation above 
    // when noisebit = 1 phase ^= 0x100;
    // in other words: phase ^= (noisebit<<8); 
    if (noise) {
      phase ^= 0x100;
    }
    chanout[7] += op_calc(phase<<FREQ_SH, env, 0, SLOT7_2.wavetable) * 2;
  }

  // Tom Tom (verified on real YM3812) 
  env = SLOT8_1.volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    chanout[8] += op_calc(SLOT8_1.Cnt, env, 0, SLOT8_1.wavetable) * 2;
  }

  // Top Cymbal (verified on real YM3812) 
  env = SLOT8_2.volume_calc(LFO_AM);
  if (env < ENV_QUIET) {
    // base frequency derived from operator 1 in channel 7 
    bool bit7 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x80) != 0;
    bool bit3 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x08) != 0;
    bool bit2 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x04) != 0;
    bool res1 = ((bit2 ^ bit7) | bit3) != 0;
    // when res1 = 0 phase = 0x000 | 0x100; 
    // when res1 = 1 phase = 0x200 | 0x100; 
    unsigned phase = res1 ? 0x300 : 0x100;

    // enable gate based on frequency of operator 2 in channel 8 
    bool bit5e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x20) != 0;
    bool bit3e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x08) != 0;
    bool res2 = (bit3e ^ bit5e) != 0;
    // when res2 = 0 pass the phase from calculation above (res1);
    // when res2 = 1 phase = 0x200 | 0x100; 
    if (res2) {
      phase = 0x300;
    }
    chanout[8] += op_calc(phase<<FREQ_SH, env, 0, SLOT8_2.wavetable) * 2;
  }
}


// generic table initialize 
void YMF262::init_tables(void)
{
  int i;
  if (g_ymf262_tables_ready)
  {
    return;
  }

  if (opl_alloc_fm_tables() != ESP_OK)
  {
    return;
  }

  for (int x = 0; x < TL_RES_LEN; x++) {
    double m = (1 << 16) / pow((double)2, (x + 1) * (ENV_STEP / 4.0) / 8.0);
    m = floor(m);

    // we never reach (1<<16) here due to the (x+1) 
    // result fits within 16 bits at maximum 
    int n = (int)m;    // 16 bits here 
    n >>= 4;    // 12 bits here 
    if (n & 1) {    // round to nearest 
      n = (n >> 1) + 1;
    } else {
      n = n >> 1;
    }
    // 11 bits here (rounded) 
    n <<= 1;    // 12 bits here (as in real chip) 
    tl_tab[x * 2 + 0] = n;
    tl_tab[x * 2 + 1] = ~tl_tab[x * 2 + 0]; // this _is_ different from OPL2 (verified on real YMF262)

    for (i = 1; i < 13; i++) {
      tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN] =  tl_tab[x * 2 + 0] >> i;
      tl_tab[x * 2 + 1 + i * 2 * TL_RES_LEN] = ~tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN];  // this _is_ different from OPL2 (verified on real YMF262) 
    }
  }

  const double LOG2 = ::log((double)2);
  for (i = 0; i < SIN_LEN; i++) {
    // non-standard sinus
    double m = sin(((i * 2) + 1) * PI / SIN_LEN); // checked against the real chip 
    // we never reach zero here due to ((i * 2) + 1) 
    double o = (m > 0.0) ?
      8 * ::log( 1.0 / m) / LOG2:  // convert to 'decibels' 
      8 * ::log(-1.0 / m) / LOG2;  // convert to 'decibels'
    o = o / (ENV_STEP / 4);

    int n = (int)(2 * o);
    if (n & 1) {// round to nearest 
      n = (n>>1)+1;
    } else {
      n = n>>1;
    }
    sin_tab[i] = n * 2 + (m >=0.0 ? 0 : 1);
  }

  for (i = 0; i < SIN_LEN; i++) {
    // these 'pictures' represent _two_ cycles 
    // waveform 1:  __      __     
    //             /  \____/  \____
    // output only first half of the sinus waveform (positive one) 
    if (i & (1 << (SIN_BITS - 1))) {
      sin_tab[1*SIN_LEN+i] = TL_TAB_LEN;
    } else {
      sin_tab[1*SIN_LEN+i] = sin_tab[i];
    }
    
    // waveform 2:  __  __  __  __ 
    //             /  \/  \/  \/  \.
    // abs(sin) 
    sin_tab[2 * SIN_LEN + i] = sin_tab[i & (SIN_MASK >> 1)];

    // waveform 3:  _   _   _   _  
    //             / |_/ |_/ |_/ |_
    // abs(output only first quarter of the sinus waveform) 
    if (i & (1<<(SIN_BITS-2))) {
      sin_tab[3*SIN_LEN+i] = TL_TAB_LEN;
    } else {
      sin_tab[3*SIN_LEN+i] = sin_tab[i & (SIN_MASK>>2)];
    }

    // waveform 4:                 
    //             /\  ____/\  ____
    //               \/      \/    
    // output whole sinus waveform in half the cycle(step=2) and output 0 on the other half of cycle
    if (i & (1 << (SIN_BITS-1))) {
      sin_tab[4*SIN_LEN+i] = TL_TAB_LEN;
    } else {
      sin_tab[4*SIN_LEN+i] = sin_tab[i*2];
    }

    // waveform 5:                 
    //             /\/\____/\/\____
    //                             
    // output abs(whole sinus) waveform in half the cycle(step=2) and output 0 on the other half of cycle 
    if (i & (1 << (SIN_BITS-1))) {
      sin_tab[5*SIN_LEN+i] = TL_TAB_LEN;
    } else {
      sin_tab[5*SIN_LEN+i] = sin_tab[(i*2) & (SIN_MASK>>1)];
    }

    // waveform 6: ____    ____    
    //                             
    //                 ____    ____
    // output maximum in half the cycle and output minimum on the other half of cycle 
    if (i & (1 << (SIN_BITS - 1))) {
      sin_tab[6*SIN_LEN+i] = 1;  // negative 
    } else {
      sin_tab[6*SIN_LEN+i] = 0;  // positive
    }

    // waveform 7:                 
    //             |\____  |\____  
    //                   \|      \|
    // output sawtooth waveform    
    int x = (i & (1 << (SIN_BITS - 1))) ?
      ((SIN_LEN - 1) - i) * 16 + 1 : // negative: from 8177 to 1 
      i * 16;                        //positive: from 0 to 8176 
    if (x > TL_TAB_LEN) {
      x = TL_TAB_LEN;  // clip to the allowed range 
    }
    sin_tab[7 * SIN_LEN+i] = x;
  }

  g_ymf262_tables_ready = true;
}


void YMF262::setSampleRate(int sampleRate, int Oversampling)
{
    oplOversampling = Oversampling;
  const int CLCK_FREQ = 14318180;
  double freqbase  = ((double)CLCK_FREQ / (8.0 * 36)) / (double)(sampleRate * oplOversampling);

  // make fnumber -> increment counter table 
  for (int i = 0; i < 1024; i++) {
    // opn phase increment counter = 20bit 
    // -10 because chip works with 10.10 fixed point, while we use 16.16 
    fn_tab[i] = (unsigned)( (double)i * 64 * freqbase * (1<<(FREQ_SH - 10)));
  }

  // Amplitude modulation: 27 output levels (triangle waveform);
  // 1 level takes one of: 192, 256 or 448 samples 
  // One entry from LFO_AM_TABLE lasts for 64 samples 
  lfo_am_inc = (unsigned)((1 << LFO_SH) * freqbase / 64.0);

  // Vibrato: 8 output levels (triangle waveform); 1 level takes 1024 samples
  lfo_pm_inc = (unsigned)((1 << LFO_SH) * freqbase / 1024.0);

  // Noise generator: a step takes 1 sample 
  noise_f = (unsigned)((1 << FREQ_SH) * freqbase);

  eg_timer_add  = (unsigned)((1 << EG_SH) * freqbase);
}

void YMF262Slot::FM_KEYON(u8 key_set)
{
  if (!key) {
    // restart Phase Generator 
    Cnt = 0;
    // phase -> Attack 
    state = EG_ATT;
  }
  key |= key_set;
}

void YMF262Slot::FM_KEYOFF(u8 key_clr)
{
  if (key) {
    key &= key_clr;
    if (!key) {
      // phase -> Release 
      if (state > EG_REL) {
        state = EG_REL;
      }
    }
  }
}

// update phase increment counter of operator (also update the EG rates if necessary) 
void YMF262Channel::CALC_FCSLOT(YMF262Slot &slot)
{
  // (frequency) phase increment counter 
  slot.Incr = fc * slot.mul;
  int ksr = kcode >> slot.KSR;

  if (slot.ksr != ksr) {
    slot.ksr = ksr;

    // calculate envelope generator rates 
    if ((slot.ar + slot.ksr) < 16+60) {
      slot.eg_sh_ar  = eg_rate_shift [slot.ar + slot.ksr ];
      slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
      slot.eg_sel_ar = eg_rate_select[slot.ar + slot.ksr ];
    } else {
      slot.eg_sh_ar  = 0;
      slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
      slot.eg_sel_ar = 13 * RATE_STEPS;
    }
    slot.eg_sh_dr  = eg_rate_shift [slot.dr + slot.ksr ];
    slot.eg_m_dr   = (1 << slot.eg_sh_dr) - 1;
    slot.eg_sel_dr = eg_rate_select[slot.dr + slot.ksr ];
    slot.eg_sh_rr  = eg_rate_shift [slot.rr + slot.ksr ];
    slot.eg_m_rr   = (1 << slot.eg_sh_rr) - 1;
    slot.eg_sel_rr = eg_rate_select[slot.rr + slot.ksr ];
  }
}

// set multi,am,vib,EG-TYP,KSR,mul 
void YMF262::set_mul(u8 sl, u8 v)
{
  int chan_no = sl / 2;
  YMF262Channel &ch  = channels[chan_no];
  YMF262Slot &slot = ch.slots[sl & 1];

  slot.mul     = mul_tab[v & 0x0f];
  slot.KSR     = (v & 0x10) ? 0 : 2;
  slot.eg_type = (v & 0x20);
  slot.vib     = (v & 0x40);
  slot.AMmask  = (v & 0x80) ? ~0 : 0;

  if (OPL3_mode) {
    // in OPL3 mode
    // DO THIS:
    //  if this is one of the slots of 1st channel forming up a 4-op channel
    //  do normal operation
    //  else normal 2 operator function
    // OR THIS:
    //  if this is one of the slots of 2nd channel forming up a 4-op channel
    //  update it using channel data of 1st channel of a pair
    //  else normal 2 operator function
    switch(chan_no) {
    case 0: case 1: case 2:
    case 9: case 10: case 11:
      if (ch.extended) {
        // normal
        ch.CALC_FCSLOT(slot);
      } else {
        // normal 
        ch.CALC_FCSLOT(slot);
      }
      break;
    case 3: case 4: case 5:
    case 12: case 13: case 14: {
      YMF262Channel &ch3 = channels[chan_no - 3];
      if (ch3.extended) {
        // update this slot using frequency data for 1st channel of a pair 
        ch3.CALC_FCSLOT(slot);
      } else {
        // normal 
        ch.CALC_FCSLOT(slot);
      }
      break;
    }
    default:
      // normal 
      ch.CALC_FCSLOT(slot);
      break;
    }
  } else {
    // in OPL2 mode 
    ch.CALC_FCSLOT(slot);
  }
}

// set ksl & tl 
void YMF262::set_ksl_tl(u8 sl, u8 v)
{
  int chan_no = sl/2;
  YMF262Channel &ch = channels[chan_no];
  YMF262Slot &slot = ch.slots[sl & 1];

  int ksl = v >> 6; // 0 / 1.5 / 3.0 / 6.0 dB/OCT 

  slot.ksl = ksl ? 3 - ksl : 31;
  slot.TL  = (v & 0x3F) << (ENV_BITS - 1 - 7); // 7 bits TL (bit 6 = always 0) 

  if (OPL3_mode) {

    // in OPL3 mode 
    //DO THIS:
    //if this is one of the slots of 1st channel forming up a 4-op channel
    //do normal operation
    //else normal 2 operator function
    //OR THIS:
    //if this is one of the slots of 2nd channel forming up a 4-op channel
    //update it using channel data of 1st channel of a pair
    //else normal 2 operator function
    switch(chan_no) {
    case 0: case 1: case 2:
    case 9: case 10: case 11:
      if (ch.extended) {
        // normal 
        slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
      } else {
        // normal 
        slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
      }
      break;
    case 3: case 4: case 5:
    case 12: case 13: case 14: {
      YMF262Channel &ch3 = channels[chan_no - 3];
      if (ch3.extended) {
        // update this slot using frequency data for 1st channel of a pair 
        slot.TLL = slot.TL + (ch3.ksl_base >> slot.ksl);
      } else {
        // normal 
        slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
      }
      break;
    }
    default:
      // normal
      slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
      break;
    }
  } else {
    // in OPL2 mode 
    slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
  }
}

// set attack rate & decay rate  
void YMF262::set_ar_dr(u8 sl, u8 v)
{
  YMF262Channel &ch = channels[sl / 2];
  YMF262Slot &slot = ch.slots[sl & 1];

  slot.ar = (v >> 4) ? 16 + ((v >> 4) << 2) : 0;

  if ((slot.ar + slot.ksr) < 16 + 60) {
    // verified on real YMF262 - all 15 x rates take "zero" time 
    slot.eg_sh_ar  = eg_rate_shift [slot.ar + slot.ksr];
    slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
    slot.eg_sel_ar = eg_rate_select[slot.ar + slot.ksr];
  } else {
    slot.eg_sh_ar  = 0;
    slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
    slot.eg_sel_ar = 13 * RATE_STEPS;
  }

  slot.dr    = (v & 0x0F) ? 16 + ((v & 0x0F) << 2) : 0;
  slot.eg_sh_dr  = eg_rate_shift [slot.dr + slot.ksr];
  slot.eg_m_dr   = (1 << slot.eg_sh_dr) - 1;
  slot.eg_sel_dr = eg_rate_select[slot.dr + slot.ksr];
}

// set sustain level & release rate 
void YMF262::set_sl_rr(u8 sl, u8 v)
{
  YMF262Channel &ch = channels[sl / 2];
  YMF262Slot &slot = ch.slots[sl & 1];

  slot.sl  = sl_tab[v >> 4];
  slot.rr  = (v & 0x0F) ? 16 + ((v & 0x0F) << 2) : 0;
  slot.eg_sh_rr  = eg_rate_shift [slot.rr + slot.ksr];
  slot.eg_m_rr   = (1 << slot.eg_sh_rr) - 1;
  slot.eg_sel_rr = eg_rate_select[slot.rr + slot.ksr];
}

void YMF262::update_channels(YMF262Channel &ch)
{
  // update channel passed as a parameter and a channel at CH+=3; 
  if (ch.extended) {
    // we've just switched to combined 4 operator mode 
  } else {
    // we've just switched to normal 2 operator mode 
  }
}

u8 YMF262::peekReg(int r)
{
  return reg[r];
}

u8 YMF262::readReg(int r)
{
  return reg[r];
}

void YMF262::writeReg(int r, u8 v, const EmuTime &time)
{
  if (!OPL3_mode && (r != 0x105)) {
    // in OPL2 mode the only accessible in set #2 is register 0x05 
    r &= ~0x100;
  }
  writeRegForce(r, v, time);
  checkMute();
}
void YMF262::writeRegForce(int r, u8 v, const EmuTime &time)
{
  reg[r] = v;

  u8 ch_offset = 0;
  if (r & 0x100) {
    switch(r) {
    case 0x101:  // test register
      return;
    
    case 0x104: { // 6 channels enable 
      YMF262Channel &ch0 = channels[0];
      u8 prev = ch0.extended;
      ch0.extended = (v >> 0) & 1;
      if (prev != ch0.extended) {
        update_channels(ch0);
      }
      YMF262Channel &ch1 = channels[1];
      prev = ch1.extended;
      ch1.extended = (v >> 1) & 1;
      if (prev != ch1.extended) {
        update_channels(ch1);
      }
      YMF262Channel &ch2 = channels[2];
      prev = ch2.extended;
      ch2.extended = (v >> 2) & 1;
      if (prev != ch2.extended) {
        update_channels(ch2);
      }
      YMF262Channel &ch9 = channels[9];
      prev = ch9.extended;
      ch9.extended = (v >> 3) & 1;
      if (prev != ch9.extended) {
        update_channels(ch9);
      }
      YMF262Channel &ch10 = channels[10];
      prev = ch10.extended;
      ch10.extended = (v >> 4) & 1;
      if (prev != ch10.extended) {
        update_channels(ch10);
      }
      YMF262Channel &ch11 = channels[11];
      prev = ch11.extended;
      ch11.extended = (v >> 5) & 1;
      if (prev != ch11.extended) {
        update_channels(ch11);
      }
      return;
    }
    case 0x105:  // OPL3 extensions enable register 
      // OPL3 mode when bit0=1 otherwise it is OPL2 mode 
      OPL3_mode = v & 0x01;
      if (OPL3_mode) {
        status2 = 0x02;
      }
      
      // following behaviour was tested on real YMF262,
      // switching OPL3/OPL2 modes on the fly:
      //  - does not change the waveform previously selected
      //    (unless when ....)
      //  - does not update CH.A, CH.B, CH.C and CH.D output
      //    selectors (registers c0-c8) (unless when ....)
      //  - does not disable channels 9-17 on OPL3->OPL2 switch
      //  - does not switch 4 operator channels back to 2
      //    operator channels
      return;

    default:
      break;
    }
    ch_offset = 9;  // register page #2 starts from channel 9
  }

  r &= 0xFF;
  switch(r & 0xE0) {
  case 0x00: // 00-1F:control 
    switch(r & 0x1F) {
    case 0x01: // test register
      break;
      
    case 0x02: // Timer 1 
      timer1.setValue(v);
      break;

    case 0x03: // Timer 2 
      timer2.setValue(v);
      break;

    case 0x04: // IRQ clear / mask and Timer enable 
      if (v & 0x80) {
        // IRQ flags clear 
        resetStatus(0x60);
      } else {
        changeStatusMask((~v) & 0x60);
        timer1.setStart((v & R04_ST1) != 0, time);
        timer2.setStart((v & R04_ST2) != 0, time);
      }
      break;
      
    case 0x08: // x,NTS,x,x, x,x,x,x
      nts = v;
      break;
      
    default:
      break;
    }
    break;
  
  case 0x20: { // am ON, vib ON, ksr, eg_type, mul 
    int slot = slot_array[r & 0x1F];
    if (slot < 0) return;
    set_mul(slot + ch_offset * 2, v);
    break;
  }
  case 0x40: {
    int slot = slot_array[r & 0x1F];
    if (slot < 0) return;
    set_ksl_tl(slot + ch_offset * 2, v);
    break;
  }
  case 0x60: {
    int slot = slot_array[r & 0x1F];
    if (slot < 0) return;
    set_ar_dr(slot + ch_offset * 2, v);
    break;
  }
  case 0x80: {
    int slot = slot_array[r & 0x1F];
    if (slot < 0) return;
    set_sl_rr(slot + ch_offset * 2, v);
    break;
  }
  case 0xA0: {
    if (r == 0xBD) {
      // am depth, vibrato depth, r,bd,sd,tom,tc,hh 
      if (ch_offset != 0) {
        // 0xbd register is present in set #1 only 
        return;
      }
      lfo_am_depth = v & 0x80;
      lfo_pm_depth_range = (v & 0x40) ? 8 : 0;
      rhythm = v & 0x3F;

      if (rhythm & 0x20) {
        // BD key on/off 
        if (v & 0x10) {
          channels[6].slots[SLOT1].FM_KEYON ( 2);
          channels[6].slots[SLOT2].FM_KEYON ( 2);
        } else {
          channels[6].slots[SLOT1].FM_KEYOFF(~2);
          channels[6].slots[SLOT2].FM_KEYOFF(~2);
        }
        // HH key on/off 
        if (v & 0x01) {
          channels[7].slots[SLOT1].FM_KEYON ( 2);
        } else {
          channels[7].slots[SLOT1].FM_KEYOFF(~2);
        }
        // SD key on/off 
        if (v & 0x08) {
          channels[7].slots[SLOT2].FM_KEYON ( 2);
        } else {
          channels[7].slots[SLOT2].FM_KEYOFF(~2);
        }
        // TOM key on/off 
        if (v & 0x04) {
          channels[8].slots[SLOT1].FM_KEYON ( 2);
        } else {
          channels[8].slots[SLOT1].FM_KEYOFF(~2);
        }
        // TOP-CY key on/off 
        if (v & 0x02) {
          channels[8].slots[SLOT2].FM_KEYON ( 2);
        } else {
          channels[8].slots[SLOT2].FM_KEYOFF(~2);
        }
      } else {
        // BD key off 
        channels[6].slots[SLOT1].FM_KEYOFF(~2);
        channels[6].slots[SLOT2].FM_KEYOFF(~2);
        // HH key off 
        channels[7].slots[SLOT1].FM_KEYOFF(~2);
        // SD key off 
        channels[7].slots[SLOT2].FM_KEYOFF(~2);
        // TOM key off 
        channels[8].slots[SLOT1].FM_KEYOFF(~2);
        // TOP-CY off 
        channels[8].slots[SLOT2].FM_KEYOFF(~2);
      }
      return;
    }

    // keyon,block,fnum 
    if ((r & 0x0F) > 8) {
      return;
    }
    int chan_no = (r & 0x0F) + ch_offset;
    YMF262Channel &ch  = channels[chan_no];
    YMF262Channel &ch3 = channels[chan_no + 3];
    if ((r & 0x10) && (v & 0x20))
      set_fm_channel_active(chan_no);

    int block_fnum;
    if (!(r & 0x10)) {
      // a0-a8 
      block_fnum  = (ch.block_fnum&0x1F00) | v;
    } else {
      // b0-b8 
      block_fnum = ((v & 0x1F) << 8) | (ch.block_fnum & 0xFF);
      if (OPL3_mode) {
        // in OPL3 mode 
        // DO THIS:
        // if this is 1st channel forming up a 4-op channel
        // ALSO keyon/off slots of 2nd channel forming up 4-op channel
        // else normal 2 operator function keyon/off
        // OR THIS:
        // if this is 2nd channel forming up 4-op channel just do nothing
        // else normal 2 operator function keyon/off
        switch(chan_no) {
        case 0: case 1: case 2:
        case 9: case 10: case 11:
          if (ch.extended) {
            //if this is 1st channel forming up a 4-op channel
            //ALSO keyon/off slots of 2nd channel forming up 4-op channel
            if (v & 0x20) {
              ch.slots[SLOT1].FM_KEYON ( 1);
              ch.slots[SLOT2].FM_KEYON ( 1);
              ch3.slots[SLOT1].FM_KEYON( 1);
              ch3.slots[SLOT2].FM_KEYON( 1);
            } else {
              ch.slots[SLOT1].FM_KEYOFF (~1);
              ch.slots[SLOT2].FM_KEYOFF (~1);
              ch3.slots[SLOT1].FM_KEYOFF(~1);
              ch3.slots[SLOT2].FM_KEYOFF(~1);
            }
          } else {
            //else normal 2 operator function keyon/off
            if (v & 0x20) {
              ch.slots[SLOT1].FM_KEYON ( 1);
              ch.slots[SLOT2].FM_KEYON ( 1);
            } else {
              ch.slots[SLOT1].FM_KEYOFF(~1);
              ch.slots[SLOT2].FM_KEYOFF(~1);
            }
          }
          break;

        case 3: case 4: case 5:
        case 12: case 13: case 14: {
          YMF262Channel &ch_3 = channels[chan_no - 3];
          if (ch_3.extended) {
            //if this is 2nd channel forming up 4-op channel just do nothing
          } else {
            //else normal 2 operator function keyon/off
            if (v & 0x20) {
              ch.slots[SLOT1].FM_KEYON ( 1);
              ch.slots[SLOT2].FM_KEYON ( 1);
            } else {
              ch.slots[SLOT1].FM_KEYOFF(~1);
              ch.slots[SLOT2].FM_KEYOFF(~1);
            }
          }
          break;
        }
        default:
          if (v & 0x20) {
            ch.slots[SLOT1].FM_KEYON ( 1);
            ch.slots[SLOT2].FM_KEYON ( 1);
          } else {
            ch.slots[SLOT1].FM_KEYOFF(~1);
            ch.slots[SLOT2].FM_KEYOFF(~1);
          }
          break;
        }
      } else {
        if (v & 0x20) {
          ch.slots[SLOT1].FM_KEYON ( 1);
          ch.slots[SLOT2].FM_KEYON ( 1);
        } else {
          ch.slots[SLOT1].FM_KEYOFF(~1);
          ch.slots[SLOT2].FM_KEYOFF(~1);
        }
      }
    }
    // update
    if (ch.block_fnum != block_fnum) {
      u8 block  = block_fnum >> 10;
      ch.block_fnum = block_fnum;
      ch.ksl_base = ksl_tab[block_fnum >> 6];
      ch.fc       = fn_tab[block_fnum & 0x03FF] >> (7 - block);

      // BLK 2,1,0 bits -> bits 3,2,1 of kcode 
      ch.kcode = (ch.block_fnum & 0x1C00) >> 9;

      // the info below is actually opposite to what is stated
      // in the Manuals (verifed on real YMF262)
      // if notesel == 0 -> lsb of kcode is bit 10 (MSB) of fnum  
      // if notesel == 1 -> lsb of kcode is bit 9 (MSB-1) of fnum 
      if (nts & 0x40) {
        ch.kcode |= (ch.block_fnum & 0x100) >> 8;  // notesel == 1 
      } else {
        ch.kcode |= (ch.block_fnum & 0x200) >> 9;  // notesel == 0 
      }
      if (OPL3_mode) {
        int chan_no = (r & 0x0F) + ch_offset;
        // in OPL3 mode 
        //DO THIS:
        //if this is 1st channel forming up a 4-op channel
        //ALSO update slots of 2nd channel forming up 4-op channel
        //else normal 2 operator function keyon/off
        //OR THIS:
        //if this is 2nd channel forming up 4-op channel just do nothing
        //else normal 2 operator function keyon/off
        switch(chan_no) {
        case 0: case 1: case 2:
        case 9: case 10: case 11:
          if (ch.extended) {
            //if this is 1st channel forming up a 4-op channel
            //ALSO update slots of 2nd channel forming up 4-op channel

            // refresh Total Level in FOUR SLOTs of this channel and channel+3 using data from THIS channel 
            ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
            ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);
            ch3.slots[SLOT1].TLL = ch3.slots[SLOT1].TL + (ch.ksl_base >> ch3.slots[SLOT1].ksl);
            ch3.slots[SLOT2].TLL = ch3.slots[SLOT2].TL + (ch.ksl_base >> ch3.slots[SLOT2].ksl);

            // refresh frequency counter in FOUR SLOTs of this channel and channel+3 using data from THIS channel 
            ch.CALC_FCSLOT(ch.slots[SLOT1]);
            ch.CALC_FCSLOT(ch.slots[SLOT2]);
            ch.CALC_FCSLOT(ch3.slots[SLOT1]);
            ch.CALC_FCSLOT(ch3.slots[SLOT2]);
          } else {
            //else normal 2 operator function
            // refresh Total Level in both SLOTs of this channel 
            ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
            ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

            // refresh frequency counter in both SLOTs of this channel 
            ch.CALC_FCSLOT(ch.slots[SLOT1]);
            ch.CALC_FCSLOT(ch.slots[SLOT2]);
          }
          break;

        case 3: case 4: case 5:
        case 12: case 13: case 14: {
          YMF262Channel &ch_3 = channels[chan_no - 3];
          if (ch_3.extended) {
            //if this is 2nd channel forming up 4-op channel just do nothing
          } else {
            //else normal 2 operator function
            // refresh Total Level in both SLOTs of this channel 
            ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
            ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

            // refresh frequency counter in both SLOTs of this channel 
            ch.CALC_FCSLOT(ch.slots[SLOT1]);
            ch.CALC_FCSLOT(ch.slots[SLOT2]);
          }
          break;
        }
        default:
          // refresh Total Level in both SLOTs of this channel 
          ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
          ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

          // refresh frequency counter in both SLOTs of this channel 
          ch.CALC_FCSLOT(ch.slots[SLOT1]);
          ch.CALC_FCSLOT(ch.slots[SLOT2]);
          break;
        }
      } else {
        // in OPL2 mode 
        // refresh Total Level in both SLOTs of this channel 
        ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
        ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

        // refresh frequency counter in both SLOTs of this channel 
        ch.CALC_FCSLOT(ch.slots[SLOT1]);
        ch.CALC_FCSLOT(ch.slots[SLOT2]);
      }
    }
    break;
  }
  case 0xC0: {
    // CH.D, CH.C, CH.B, CH.A, FB(3bits), C 
    if ((r & 0xF) > 8) {
      return;
    }
    int chan_no = (r & 0x0F) + ch_offset;
    YMF262Channel &ch = channels[chan_no];

    int base = chan_no * 4;
    if (OPL3_mode) {
      // OPL3 mode 
      pan[base + 0] = (v & 0x10) ? ~0 : 0;  // ch.A 
      pan[base + 1] = (v & 0x20) ? ~0 : 0;  // ch.B 
      pan[base + 2] = (v & 0x40) ? ~0 : 0;  // ch.C 
      pan[base + 3] = (v & 0x80) ? ~0 : 0;  // ch.D
    } else {
      // OPL2 mode - always enabled 
      pan[base + 0] = ~0;  // ch.A 
      pan[base + 1] = ~0;  // ch.B 
      pan[base + 2] = ~0;  // ch.C 
      pan[base + 3] = ~0;  // ch.D 
    }

    ch.slots[SLOT1].FB  = (v >> 1) & 7 ? ((v >> 1) & 7) + 7 : 0;
    ch.slots[SLOT1].CON = v & 1;

    if (OPL3_mode) {
      switch(chan_no) {
      case 0: case 1: case 2:
      case 9: case 10: case 11:
        if (ch.extended) {
          YMF262Channel &ch3 = channels[chan_no + 3];
          u8 conn = (u8)((ch.slots[SLOT1].CON << 1) | ch3.slots[SLOT1].CON);
          switch(conn) {
          case 0:
            // 1 -> 2 -> 3 -> 4 - out 
            ch.slots[SLOT1].connect = PHASE_MOD1;
            ch.slots[SLOT2].connect = PHASE_MOD2;
            ch3.slots[SLOT1].connect = PHASE_MOD1;
            ch3.slots[SLOT2].connect = chan_no + 3;
            break;
            
          case 1:
            // 1 -> 2 -\.
            // 3 -> 4 -+- out 
            ch.slots[SLOT1].connect = PHASE_MOD1;
            ch.slots[SLOT2].connect = chan_no;
            ch3.slots[SLOT1].connect = PHASE_MOD1;
            ch3.slots[SLOT2].connect = chan_no + 3;
            break;
            
          case 2:
            // 1 -----------\.
            // 2 -> 3 -> 4 -+- out 
            ch.slots[SLOT1].connect = chan_no;
            ch.slots[SLOT2].connect = PHASE_MOD2;
            ch3.slots[SLOT1].connect = PHASE_MOD1;
            ch3.slots[SLOT2].connect = chan_no + 3;
            break;

          case 3:
            // 1 ------\.
            // 2 -> 3 -+- out
            // 4 ------/     
            ch.slots[SLOT1].connect = chan_no;
            ch.slots[SLOT2].connect = PHASE_MOD2;
            ch3.slots[SLOT1].connect = chan_no + 3;
            ch3.slots[SLOT2].connect = chan_no + 3;
            break;
          }
        } else {
          // 2 operators mode 
          ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
          ch.slots[SLOT2].connect = chan_no;
        }
        break;

      case 3: case 4: case 5:
      case 12: case 13: case 14: {
        YMF262Channel &ch3 = channels[chan_no - 3];
        if (ch3.extended) {
          u8 conn = (u8)((ch3.slots[SLOT1].CON << 1) | ch.slots[SLOT1].CON);
          switch(conn) {
          case 0:
            // 1 -> 2 -> 3 -> 4 - out 
            ch3.slots[SLOT1].connect = PHASE_MOD1;
            ch3.slots[SLOT2].connect = PHASE_MOD2;
            ch.slots[SLOT1].connect = PHASE_MOD1;
            ch.slots[SLOT2].connect = chan_no;
            break;

          case 1:
            // 1 -> 2 -\.
            // 3 -> 4 -+- out 
            ch3.slots[SLOT1].connect = PHASE_MOD1;
            ch3.slots[SLOT2].connect = chan_no - 3;
            ch.slots[SLOT1].connect = PHASE_MOD1;
            ch.slots[SLOT2].connect = chan_no;
            break;
            
          case 2:
            // 1 -----------\.
            // 2 -> 3 -> 4 -+- out 
            ch3.slots[SLOT1].connect = chan_no - 3;
            ch3.slots[SLOT2].connect = PHASE_MOD2;
            ch.slots[SLOT1].connect = PHASE_MOD1;
            ch.slots[SLOT2].connect = chan_no;
            break;
            
          case 3:
            // 1 ------\.
            // 2 -> 3 -+- out
            // 4 ------/     
            ch3.slots[SLOT1].connect = chan_no - 3;
            ch3.slots[SLOT2].connect = PHASE_MOD2;
            ch.slots[SLOT1].connect = chan_no;
            ch.slots[SLOT2].connect = chan_no;
            break;
          }
        } else {
          // 2 operators mode 
          ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
          ch.slots[SLOT2].connect = chan_no;
        }
        break;
      }
      default:
        // 2 operators mode 
        ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
        ch.slots[SLOT2].connect = chan_no;
        break;
      }
    } else {
      // OPL2 mode - always 2 operators mode
      ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
      ch.slots[SLOT2].connect = chan_no;
    }
    break;
  }
  case 0xE0: {
    // waveform select 
    int slot = slot_array[r & 0x1f];
    if (slot < 0) return;
    slot += ch_offset * 2;
    YMF262Channel &ch = channels[slot / 2];

    // store 3-bit value written regardless of current OPL2 or OPL3
    // mode... (verified on real YMF262) 
    v &= 7;
    ch.slots[slot & 1].waveform_number = v;
    // ... but select only waveforms 0-3 in OPL2 mode 
    if (!OPL3_mode) {
      v &= 3;
    }
    ch.slots[slot & 1].wavetable = v * SIN_LEN;
    break;
  }
  }
}


void YMF262::reset(const EmuTime &time)
{
  eg_timer = 0;
  eg_cnt   = 0;
  fm_active_mask = 0;

  noise_rng = 1;  // noise shift register
  nts       = 0;  // note split
  resetStatus(0x60);

  // reset with register write
  writeRegForce(0x01, 0, time); // test register
  writeRegForce(0x02, 0, time); // Timer1
  writeRegForce(0x03, 0, time); // Timer2
  writeRegForce(0x04, 0, time); // IRQ mask clear

  //FIX IT  registers 101, 104 and 105
  //FIX IT (dont change CH.D, CH.C, CH.B and CH.A in C0-C8 registers)
    int c;
  for (c = 0xFF; c >= 0x20; c--) {
    writeRegForce(c, 0, time);
  }
  //FIX IT (dont change CH.D, CH.C, CH.B and CH.A in C0-C8 registers)
  for (c = 0x1FF; c >= 0x120; c--) {
    writeRegForce(c, 0, time);
  }

  // reset operator parameters 
  for (c = 0; c < 9 * 2; c++) {
    YMF262Channel &ch = channels[c];
    for (int s = 0; s < 2; s++) {
      ch.slots[s].state  = EG_OFF;
      ch.slots[s].volume = MAX_ATT_INDEX;
    }
  }
  setInternalMute(true);
}

YMF262::YMF262(short volume, const EmuTime &time, void* ref)
  : timer1(this, ref), timer2(this, ref)
{
    g_ymf262_chan_out = chanout;

  LFO_AM = LFO_PM = 0;
  lfo_am_depth = lfo_pm_depth_range = lfo_am_cnt = lfo_pm_cnt = 0;
  noise_rng = noise_p = 0;
  rhythm = nts = 0;
  OPL3_mode = false;
  status = status2 = statusMask = 0;
  
    oplOversampling = 1;
    currentSampleRate = 44100;
    renderMode = OPL_FM_RENDER_OPL3_FULL;
    fm_active_mask = 0;

  init_tables();

  reset(time);
}

YMF262::~YMF262()
{
}

u8 YMF262::peekStatus()
{
  return status | status2;
}

u8 YMF262::readStatus()
{
  u8 result = status | status2;
  status2 = 0;
  return result;
}

void YMF262::checkMute()
{
  bool mute = checkMuteHelper();
  //PRT_DEBUG("YMF262: muted " << mute);
  setInternalMute(mute);
}
bool YMF262::checkMuteHelper()
{
  // TODO this doesn't always mute when possible
  for (int i = 0; i < 18; i++) {
    for (int j = 0; j < 2; j++) {
      YMF262Slot &sl = channels[i].slots[j];
      if (!((sl.state == EG_OFF) ||
            ((sl.state == EG_REL) &&
             ((sl.TLL + sl.volume) >= ENV_QUIET)))) {
        return false;
      }
    }
  }
  return true;
}

int* YMF262::updateBuffer(int length)
{
  (void)length;
  return NULL;
}

void YMF262::setRenderMode(OPL_FM_RENDER_MODE mode)
{
  renderMode = mode;
}

void IRAM_ATTR YMF262::render_opl2_melody(float *mix, int length)
{
  float volume_gain = (float)opl_global_volume / (float)OPL_VOLUME_BASE;

  for (int sample_idx = 0; sample_idx < length; sample_idx++)
  {
    if (!(fm_active_mask & 0x1FFu))
      return;

    int ab = 0;
    int count = oplOversampling;

    while (count--)
    {
      unsigned int active_mask = fm_active_mask & 0x1FFu;
      if (!active_mask)
        break;

      advance_lfo();

      for (int chan_no = 0; chan_no < 9; chan_no++)
      {
        unsigned int bit = 1u << chan_no;
        if (!(active_mask & bit))
          continue;

        chanout[chan_no] = 0;
        channels[chan_no].chan_calc(LFO_AM);
        ab += chanout[chan_no];
      }

      advance_opl2_melody();
    }

    float sample = (float)((ab << 3) / (oplOversampling * 10)) * volume_gain;
    mix[2 * sample_idx] += sample;
    mix[2 * sample_idx + 1] += sample;
  }
}

void YMF262::render(float *mix, int length, int newSampleRate)
{
  if (!mix || length <= 0) return;
  if (newSampleRate > 0 && newSampleRate != currentSampleRate)
  {
    currentSampleRate = newSampleRate;
    setSampleRate(currentSampleRate, 1);
  }

  if (isInternalMuted()) return;

  if (renderMode == OPL_FM_RENDER_OPL2_MELODY && !OPL3_mode && !(rhythm & 0x20))
  {
    render_opl2_melody(mix, length);
    checkMute();
    return;
  }

  bool rhythmEnabled = (rhythm & 0x20) != 0;
  float volume_gain = (float)opl_global_volume / (float)OPL_VOLUME_BASE;

  for (int sample_idx = 0; sample_idx < length; sample_idx++)
  {
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;
    int count = oplOversampling;
    while (count--)
    {
      advance_lfo();

      memset(chanout, 0, sizeof(int) * 18);

      channels[0].chan_calc(LFO_AM);
      if (channels[0].extended) channels[3].chan_calc_ext(LFO_AM);
      else channels[3].chan_calc(LFO_AM);

      channels[1].chan_calc(LFO_AM);
      if (channels[1].extended) channels[5].chan_calc_ext(LFO_AM);
      else channels[4].chan_calc(LFO_AM);

      channels[2].chan_calc(LFO_AM);
      if (channels[2].extended) channels[5].chan_calc_ext(LFO_AM);
      else channels[5].chan_calc(LFO_AM);

      if (!rhythmEnabled)
      {
        channels[6].chan_calc(LFO_AM);
        channels[7].chan_calc(LFO_AM);
        channels[8].chan_calc(LFO_AM);
      }
      else
      {
        chan_calc_rhythm(noise_rng & 1);
      }

      channels[9].chan_calc(LFO_AM);
      if (channels[9].extended) channels[12].chan_calc_ext(LFO_AM);
      else channels[12].chan_calc(LFO_AM);

      channels[10].chan_calc(LFO_AM);
      if (channels[10].extended) channels[13].chan_calc_ext(LFO_AM);
      else channels[13].chan_calc(LFO_AM);

      channels[11].chan_calc(LFO_AM);
      if (channels[11].extended) channels[14].chan_calc_ext(LFO_AM);
      else channels[14].chan_calc(LFO_AM);

      channels[15].chan_calc(LFO_AM);
      channels[16].chan_calc(LFO_AM);
      channels[17].chan_calc(LFO_AM);

      for (int i = 0; i < 18; i++)
      {
        a += chanout[i] & pan[4 * i + 0];
        b += chanout[i] & pan[4 * i + 1];
        c += chanout[i] & pan[4 * i + 2];
        d += chanout[i] & pan[4 * i + 3];
      }
      advance();
    }

    float left = (float)((a << 3) / (oplOversampling * 10)) * volume_gain;
    float right = (float)((b << 3) / (oplOversampling * 10)) * volume_gain;
    mix[2 * sample_idx] += left;
    mix[2 * sample_idx + 1] += right;
  }

  checkMute();
}

void YMF262::setInternalVolume(short newVolume)
{
  maxVolume = newVolume;
}

// End of YMF262 FM core.


// YMF278 OPL4 wave core, adapted from moon.zip/openMSX code.
// The original ROM/RAM allocations and internal output buffer are removed;
// render() accumulates directly into the shared float mix buffer.

class YMF278Slot
{
  public:
    YMF278Slot();
    void reset();
    int compute_rate(int val);
    inline int compute_vib();
    inline int compute_am();
    void set_lfo(int newlfo);

    short wave;
    short FN;
    char OCT;
    char PRVB;
    char LD;
    char TL;
    char pan;
    char lfo;
    char vib;
    char AM;

    char AR;
    char D1R;
    int DL;
    char D2R;
    char RC;
    char RR;

    int step;
    int stepptr;
    int pos;
    short sample1;
    short sample2;

    bool active;
    u8 bits;
    int startaddr;
    int loopaddr;
    int endaddr;

    u8 state;
    int env_vol;

    bool lfo_active;
    int lfo_cnt;
    int lfo_step;
    int lfo_max;
};

class YMF278
{
  public:
    YMF278(short volume, const u8 *rom_data, size_t rom_data_size, u8 *ram_data, size_t ram_data_size, const EmuTime &time);
    ~YMF278();
    void reset(const EmuTime &time);
    void writeRegOPL4(u8 reg, u8 data, const EmuTime &time);
    u8 peekRegOPL4(u8 reg, const EmuTime &time);
    u8 readRegOPL4(u8 reg, const EmuTime &time);
    u8 peekStatus(const EmuTime &time);
    u8 readStatus(const EmuTime &time);
    void setSampleRate(int sampleRate, int Oversampling);
    void setInternalVolume(short newVolume);
    void render(float *mix, int length, int sampleRate);

  private:
    u8 readMem(unsigned int address);
    void writeMem(unsigned int address, u8 value);
    short getSample(YMF278Slot &op);
    void advance();
    void checkMute();
    bool anyActive();
    void keyOnHelper(YMF278Slot &slot);
    void setInternalMute(bool muted) { internalMuted = muted; }
    bool isInternalMuted() const { return internalMuted; }

    bool internalMuted;
    const u8 *rom;
    size_t romSize;
    u8 *ram;
    size_t ramSize;
    int oplOversampling;
    YMF278Slot slots[24];

    unsigned int eg_cnt;
    unsigned int eg_timer;
    unsigned int eg_timer_add;

    char wavetblhdr;
    char memmode;
    int memadr;

    int fm_l;
    int fm_r;
    int pcm_l;
    int pcm_r;

    unsigned int endRom;
    unsigned int endRam;
    int volume[256 * 4];
    u8 regs[256];

    unsigned long LD_Time;
    unsigned long BUSY_Time;
};

const int Y278_EG_SH = 16;	// 16.16 fixed point (EG timing)
const unsigned int Y278_EG_TIMER_OVERFLOW = 1 << Y278_EG_SH;

// envelope output entries
const int Y278_ENV_BITS      = 10;
const int Y278_ENV_LEN       = 1 << Y278_ENV_BITS;
const double Y278_ENV_STEP   = 128.0 / Y278_ENV_LEN;
const int Y278_MAX_ATT_INDEX = (1 << (Y278_ENV_BITS - 1)) - 1; //511
const int Y278_MIN_ATT_INDEX = 0;

// Envelope Generator phases
const int Y278_EG_ATT = 4;
const int Y278_EG_DEC = 3;
const int Y278_EG_SUS = 2;
const int Y278_EG_REL = 1;
const int Y278_EG_OFF = 0;

const int Y278_EG_REV = 5;	//pseudo reverb
const int Y278_EG_DMP = 6;	//damp

// Pan values, units are -3dB, i.e. 8.
const int y278_pan_left[16]  = {
	0, 8, 16, 24, 32, 40, 48, 256, 256,   0,  0,  0,  0,  0,  0, 0
};
const int y278_pan_right[16] = {
	0, 0,  0,  0,  0,  0,  0,   0, 256, 256, 48, 40, 32, 24, 16, 8
};

// Mixing levels, units are -3dB, and add some marging to avoid clipping
const int y278_mix_level[8] = {
	8, 16, 24, 32, 40, 48, 56, 256
};

// decay level table (3dB per step)
// 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)
#define SC(db) (unsigned int)(db * (2.0 / Y278_ENV_STEP))
const unsigned int y278_dl_tab[16] = {
 SC( 0), SC( 1), SC( 2), SC(3 ), SC(4 ), SC(5 ), SC(6 ), SC( 7),
 SC( 8), SC( 9), SC(10), SC(11), SC(12), SC(13), SC(14), SC(31)
};
#undef SC

const u8 Y278_RATE_STEPS = 8;
const u8 y278_eg_inc[15 * Y278_RATE_STEPS] = {
//cycle:0 1  2 3  4 5  6 7
	0, 1,  0, 1,  0, 1,  0, 1, //  0  rates 00..12 0 (increment by 0 or 1)
	0, 1,  0, 1,  1, 1,  0, 1, //  1  rates 00..12 1
	0, 1,  1, 1,  0, 1,  1, 1, //  2  rates 00..12 2
	0, 1,  1, 1,  1, 1,  1, 1, //  3  rates 00..12 3

	1, 1,  1, 1,  1, 1,  1, 1, //  4  rate 13 0 (increment by 1)
	1, 1,  1, 2,  1, 1,  1, 2, //  5  rate 13 1
	1, 2,  1, 2,  1, 2,  1, 2, //  6  rate 13 2
	1, 2,  2, 2,  1, 2,  2, 2, //  7  rate 13 3

	2, 2,  2, 2,  2, 2,  2, 2, //  8  rate 14 0 (increment by 2)
	2, 2,  2, 4,  2, 2,  2, 4, //  9  rate 14 1
	2, 4,  2, 4,  2, 4,  2, 4, // 10  rate 14 2
	2, 4,  4, 4,  2, 4,  4, 4, // 11  rate 14 3

	4, 4,  4, 4,  4, 4,  4, 4, // 12  rates 15 0, 15 1, 15 2, 15 3 for decay
	8, 8,  8, 8,  8, 8,  8, 8, // 13  rates 15 0, 15 1, 15 2, 15 3 for attack (zero time)
	0, 0,  0, 0,  0, 0,  0, 0, // 14  infinity rates for attack and decay(s)
};

#define O(a) (a * Y278_RATE_STEPS)
const u8 y278_eg_rate_select[64] = {
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 4),O( 5),O( 6),O( 7),
	O( 8),O( 9),O(10),O(11),
	O(12),O(12),O(12),O(12),
};
#undef O

//rate  0,    1,    2,    3,   4,   5,   6,  7,  8,  9,  10, 11, 12, 13, 14, 15
//shift 12,   11,   10,   9,   8,   7,   6,  5,  4,  3,  2,  1,  0,  0,  0,  0
//mask  4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3,  1,  0,  0,  0,  0
#define O(a) (a)
const u8 y278_eg_rate_shift[64] = {
	O(12),O(12),O(12),O(12),
	O(11),O(11),O(11),O(11),
	O(10),O(10),O(10),O(10),
	O( 9),O( 9),O( 9),O( 9),
	O( 8),O( 8),O( 8),O( 8),
	O( 7),O( 7),O( 7),O( 7),
	O( 6),O( 6),O( 6),O( 6),
	O( 5),O( 5),O( 5),O( 5),
	O( 4),O( 4),O( 4),O( 4),
	O( 3),O( 3),O( 3),O( 3),
	O( 2),O( 2),O( 2),O( 2),
	O( 1),O( 1),O( 1),O( 1),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
};
#undef O


//number of steps to take in quarter of lfo frequency
//TODO check if frequency matches real chip
#define O(a) ((int)((Y278_EG_TIMER_OVERFLOW / a) / 6))
const int y278_lfo_period[8] = {
	O(0.168), O(2.019), O(3.196), O(4.206),
	O(5.215), O(5.888), O(6.224), O(7.066)
};
#undef O


#define O(a) ((int)(a * 65536))
const int y278_vib_depth[8] = {
	O(0),	   O(3.378),  O(5.065),  O(6.750),
	O(10.114), O(20.170), O(40.106), O(79.307)
};
#undef O


#define SC(db) (unsigned int) (db * (2.0 / Y278_ENV_STEP))
const int y278_am_depth[8] = {
	SC(0),	   SC(1.781), SC(2.906), SC(3.656),
	SC(4.406), SC(5.906), SC(7.406), SC(11.91)
};
#undef SC


YMF278Slot::YMF278Slot()
{
	reset();
}

void YMF278Slot::reset()
{
	wave = FN = OCT = PRVB = LD = TL = pan = lfo = vib = AM = 0;
	AR = D1R = DL = D2R = RC = RR = 0;
	step = stepptr = 0;
	pos = 0;
	sample1 = 0;
	sample2 = 0;
	bits = startaddr = loopaddr = endaddr = 0;
	env_vol = Y278_MAX_ATT_INDEX;
	//env_vol_step = env_vol_lim = 0;

	lfo_active = false;
	lfo_cnt = lfo_step = 0;
	lfo_max = y278_lfo_period[0];

	state = Y278_EG_OFF;
	active = false;
}

int YMF278Slot::compute_rate(int val)
{
	if (val == 0) {
		return 0;
	} else if (val == 15) {
		return 63;
	}
	int res;
	if (RC != 15) {
		int oct = OCT;
		if (oct & 8) {
			oct |= -8;
		}
		res = (oct + RC) * 2 + (FN & 0x200 ? 1 : 0) + val * 4;
	} else {
		res = val * 4;
	}
	if (res < 0) {
		res = 0;
	} else if (res > 63) {
		res = 63;
	}
	return res;
}

int YMF278Slot::compute_vib()
{
	return (((lfo_step << 8) / lfo_max) * y278_vib_depth[(int)vib]) >> 24;
}


int YMF278Slot::compute_am()
{
	if (lfo_active && AM) {
		return (((lfo_step << 8) / lfo_max) * y278_am_depth[(int)AM]) >> 12;
	} else {
		return 0;
	}
}

void YMF278Slot::set_lfo(int newlfo)
{
	lfo_step = (((lfo_step << 8) / lfo_max) * newlfo) >> 8;
	lfo_cnt  = (((lfo_cnt  << 8) / lfo_max) * newlfo) >> 8;

	lfo = newlfo;
	lfo_max = y278_lfo_period[(int)lfo];
}


void YMF278::advance()
{
	eg_timer += eg_timer_add;
    
    if (eg_timer > 4 * Y278_EG_TIMER_OVERFLOW) {
        eg_timer = Y278_EG_TIMER_OVERFLOW;
    }

	while (eg_timer >= Y278_EG_TIMER_OVERFLOW) {
		eg_timer -= Y278_EG_TIMER_OVERFLOW;
		eg_cnt++;
		
		for (int i = 0; i < 24; i++) {
			YMF278Slot &op = slots[i];
			
			if (op.lfo_active) {
				op.lfo_cnt++;
				if (op.lfo_cnt < op.lfo_max) {
					op.lfo_step++;
				} else if (op.lfo_cnt < (op.lfo_max * 3)) {
					op.lfo_step--;
				} else {
					op.lfo_step++;
					if (op.lfo_cnt == (op.lfo_max * 4)) {
						op.lfo_cnt = 0;
					}
				}
			}
			
			// Envelope Generator
			switch(op.state) {
			case Y278_EG_ATT: {	// attack phase
				u8 rate = op.compute_rate(op.AR);
				if (rate < 4) {
					break;
				}
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) -1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += (~op.env_vol * y278_eg_inc[select + ((eg_cnt >> shift) & 7)]) >> 3;
					if (op.env_vol <= Y278_MIN_ATT_INDEX) {
						op.env_vol = Y278_MIN_ATT_INDEX;
                        if (op.DL == 0) {
    						op.state = Y278_EG_SUS;
                        }
                        else {
    						op.state = Y278_EG_DEC;
                        }
					}
				}
				break;
			}
			case Y278_EG_DEC: {	// decay phase 
				u8 rate = op.compute_rate(op.D1R);
				if (rate < 4) {
					break;
				}
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) -1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += y278_eg_inc[select + ((eg_cnt >> shift) & 7)];
					
					if (((unsigned int)op.env_vol > y278_dl_tab[6]) && op.PRVB) {
						op.state = Y278_EG_REV;
					} else {
						if (op.env_vol >= op.DL) {
							op.state = Y278_EG_SUS;
						}
					}
				} 
				break;
			}
			case Y278_EG_SUS: {	// sustain phase 
				u8 rate = op.compute_rate(op.D2R);
				if (rate < 4) {
					break;
				}
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) -1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += y278_eg_inc[select + ((eg_cnt >> shift) & 7)];
					
					if (((unsigned int)op.env_vol > y278_dl_tab[6]) && op.PRVB) {
						op.state = Y278_EG_REV;
					} else {
						if (op.env_vol >= Y278_MAX_ATT_INDEX) {
							op.env_vol = Y278_MAX_ATT_INDEX;
							op.active = false;
							checkMute();
						}
					}
				}
				break;
			}
			case Y278_EG_REL: {	// release phase 
				u8 rate = op.compute_rate(op.RR);
				if (rate < 4) {
					break;
				}
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) -1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += y278_eg_inc[select + ((eg_cnt >> shift) & 7)];
					
					if (((unsigned int)op.env_vol > y278_dl_tab[6]) && op.PRVB) {
						op.state = Y278_EG_REV;
					} else {
						if (op.env_vol >= Y278_MAX_ATT_INDEX) {
							op.env_vol = Y278_MAX_ATT_INDEX;
							op.active = false;
							checkMute();
						}
					}
				}
				break;
			}
			case Y278_EG_REV: {	//pseudo reverb
				//TODO improve env_vol update
				u8 rate = op.compute_rate(5);
				//if (rate < 4) {
				//	break;
				//}
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) - 1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += y278_eg_inc[select + ((eg_cnt >> shift) & 7)];
					
					if (op.env_vol >= Y278_MAX_ATT_INDEX) {
						op.env_vol = Y278_MAX_ATT_INDEX;
						op.active = false;
						checkMute();
					}
				}
				break;
			}
			case Y278_EG_DMP: {	//damping
				//TODO improve env_vol update, damp is just fastest decay now
				u8 rate = 56;
				u8 shift = y278_eg_rate_shift[rate];
				if (!(eg_cnt & ((1 << shift) - 1))) {
					u8 select = y278_eg_rate_select[rate];
					op.env_vol += y278_eg_inc[select + ((eg_cnt >> shift) & 7)];
					
					if (op.env_vol >= Y278_MAX_ATT_INDEX) {
						op.env_vol = Y278_MAX_ATT_INDEX;
						op.active = false;
						checkMute();
					}
				}
				break;
			}
			case Y278_EG_OFF:
				// nothing
				break;
			
			default:
				break;
			}
		}
	}
}

short YMF278::getSample(YMF278Slot &op)
{
	short sample;
	switch (op.bits) {
	case 0: {
		// 8 bit
		sample = readMem(op.startaddr + op.pos) << 8;
		break;
	}
	case 1: {
		// 12 bit
		int addr = op.startaddr + ((op.pos / 2) * 3);
		if (op.pos & 1) {
			sample = readMem(addr + 2) << 8 |
				 ((readMem(addr + 1) << 4) & 0xF0);
		} else {
			sample = readMem(addr + 0) << 8 |
				 (readMem(addr + 1) & 0xF0);
		}
		break;
	}
	case 2: {
		// 16 bit
		int addr = op.startaddr + (op.pos * 2);
		sample = (readMem(addr + 0) << 8) |
			 (readMem(addr + 1));
		break;
	}
	default:
		// TODO unspecified
		sample = 0;
	}
	return sample;
}

void YMF278::checkMute()
{
	setInternalMute(!anyActive());
}

bool YMF278::anyActive()
{
	for (int i = 0; i < 24; i++) {
		if (slots[i].active) {
			return true;
		}
	}
	return false;
}


void YMF278::render(float *mix, int length, int sampleRate)
{
  if (!mix || length <= 0) return;
  if (isInternalMuted()) return;

  if (sampleRate > 0 && sampleRate != 44100) setSampleRate(sampleRate, 1);

  int vl = y278_mix_level[pcm_l];
  int vr = y278_mix_level[pcm_r];
  float volume_gain = (float)opl_global_volume / (float)OPL_VOLUME_BASE;

  for (int sample_idx = 0; sample_idx < length; sample_idx++)
  {
    int left = 0;
    int right = 0;
    int cnt = oplOversampling;
    while (cnt--)
    {
      for (int i = 0; i < 24; i++)
      {
        YMF278Slot &sl = slots[i];
        if (!sl.active) continue;

        short sample = (sl.sample1 * (0x10000 - sl.stepptr) + sl.sample2 * sl.stepptr) >> 16;
        int vol = sl.TL + (sl.env_vol >> 2) + sl.compute_am();

        int volLeft = vol + y278_pan_left[(int)sl.pan] + vl;
        int volRight = vol + y278_pan_right[(int)sl.pan] + vr;

        if (volLeft < 0) volLeft = 0;
        if (volRight < 0) volRight = 0;

        left += (sample * volume[volLeft]) >> 10;
        right += (sample * volume[volRight]) >> 10;

        if (sl.lfo_active && sl.vib)
        {
          int oct = sl.OCT;
          if (oct & 8) oct |= -8;
          oct += 5;
          sl.stepptr += (oct >= 0 ? ((sl.FN | 1024) + sl.compute_vib()) << oct : ((sl.FN | 1024) + sl.compute_vib()) >> -oct) / oplOversampling;
        }
        else
        {
          sl.stepptr += sl.step / oplOversampling;
        }

        int count = (sl.stepptr >> 16) & 0x0F;
        sl.stepptr &= 0xFFFF;
        while (count--)
        {
          sl.sample1 = sl.sample2;
          sl.pos++;
          if (sl.pos >= sl.endaddr) sl.pos = sl.loopaddr;
          sl.sample2 = getSample(sl);
        }
      }
      advance();
    }

    mix[2 * sample_idx] += (float)(left / oplOversampling) * volume_gain;
    mix[2 * sample_idx + 1] += (float)(right / oplOversampling) * volume_gain;
  }
}

void YMF278::keyOnHelper(YMF278Slot& slot)
{
	slot.active = true;
	setInternalMute(false);
	
	int oct = slot.OCT;
	if (oct & 8) {
		oct |= -8;
	}
	oct += 5;
	slot.step = oct >= 0 ? (slot.FN | 1024) << oct : (slot.FN | 1024) >> -oct;
	slot.state = Y278_EG_ATT;
	slot.stepptr = 0;
	slot.pos = 0;
	slot.sample1 = getSample(slot);
	slot.pos = 1;
	slot.sample2 = getSample(slot);
}

void YMF278::writeRegOPL4(u8 reg, u8 data, const EmuTime &time)
{
	BUSY_Time = time + 88 * 6 / 9;
	
	// Handle slot registers specifically
	if (reg >= 0x08 && reg <= 0xF7) {
		int snum = (reg - 8) % 24;
		YMF278Slot& slot = slots[snum];
		switch ((reg - 8) / 24) {
		case 0: {
			LD_Time = time;
			slot.wave = (slot.wave & 0x100) | data;
			int base = (slot.wave < 384 || !wavetblhdr) ?
			           (slot.wave * 12) :
			           (wavetblhdr * 0x80000 + ((slot.wave - 384) * 12));
			u8 h0 = readMem(base + 0);
			u8 h1 = readMem(base + 1);
			u8 h2 = readMem(base + 2);
			u8 h3 = readMem(base + 3);
			u8 h4 = readMem(base + 4);
			u8 h5 = readMem(base + 5);
			u8 h6 = readMem(base + 6);
			u8 h7 = readMem(base + 7);
			u8 h8 = readMem(base + 8);
			u8 h9 = readMem(base + 9);
			u8 h10 = readMem(base + 10);
			u8 h11 = readMem(base + 11);
			slot.bits = (h0 & 0xC0) >> 6;
			slot.set_lfo((h7 >> 3) & 7);
			slot.vib  = h7 & 7;
			slot.AR   = h8 >> 4;
			slot.D1R  = h8 & 0xF;
			slot.DL   = y278_dl_tab[h9 >> 4];
			slot.D2R  = h9 & 0xF;
			slot.RC   = h10 >> 4;
			slot.RR   = h10 & 0xF;
			slot.AM   = h11 & 7;
			slot.startaddr = h2 | (h1 << 8) | ((h0 & 0x3F) << 16);
			slot.loopaddr = h4 + (h3 << 8);
			slot.endaddr  = (((h6 + (h5 << 8)) ^ 0xFFFF) + 1);
			if ((regs[reg + 4] & 0x080)) {
				keyOnHelper(slot);
			}
			break;
		}
		case 1: {
			slot.wave = (slot.wave & 0xFF) | ((data & 0x1) << 8);
			slot.FN = (slot.FN & 0x380) | (data >> 1);
			int oct = slot.OCT;
			if (oct & 8) {
				oct |= -8;
			}
	        oct += 5;
	        slot.step = oct >= 0 ? (slot.FN | 1024) << oct : (slot.FN | 1024) >> -oct;
			break;
		}
		case 2: {
			slot.FN = (slot.FN & 0x07F) | ((data & 0x07) << 7);
			slot.PRVB = ((data & 0x08) >> 3);
			slot.OCT =  ((data & 0xF0) >> 4);
			int oct = slot.OCT;
			if (oct & 8) {
				oct |= -8;
			}
	        oct += 5;
	        slot.step = oct >= 0 ? (slot.FN | 1024) << oct : (slot.FN | 1024) >> -oct;
            break;
		}
		case 3:
			slot.TL = data >> 1;
			slot.LD = data & 0x1;

			// TODO
			if (slot.LD) {
				// directly change volume
			} else {
				// interpolate volume
			}
			break;
		case 4:
			slot.pan = data & 0x0F;

			if (data & 0x020) {
				// LFO reset
				slot.lfo_active = false;
				slot.lfo_cnt = 0;
				slot.lfo_max = y278_lfo_period[(int)slot.vib];
				slot.lfo_step = 0;
			} else {
				// LFO activate
				slot.lfo_active = true;
			}

			switch (data >> 6) {
			case 0:	//tone off, no damp
				if (slot.active && (slot.state != Y278_EG_REV) ) {
					slot.state = Y278_EG_REL;
				}
				break;
			case 1:	//tone off, damp
				slot.state = Y278_EG_DMP;
				break;
			case 2:	//tone on, no damp
				if (!(regs[reg] & 0x080)) {
					keyOnHelper(slot);
				}
				break;
			case 3:	//tone on, damp
				slot.state = Y278_EG_DMP;
				break;
			}
			break;
		case 5:
			slot.vib = data & 0x7;
			slot.set_lfo((data >> 3) & 0x7);
			break;
		case 6:
			slot.AR  = data >> 4;
			slot.D1R = data & 0xF;
			break;
		case 7:
			slot.DL  = y278_dl_tab[data >> 4];
			slot.D2R = data & 0xF;
			break;
		case 8:
			slot.RC = data >> 4;
			slot.RR = data & 0xF;
			break;
		case 9:
			slot.AM = data & 0x7;
			break;
		}
	} else {
		// All non-slot registers
		switch (reg) {
		case 0x00:    	// TEST
		case 0x01:
			break;

		case 0x02:
			wavetblhdr = (data >> 2) & 0x7;
			memmode = data & 1;
			break;

		case 0x03:
			memadr = (memadr & 0x00FFFF) | (data << 16);
			break;

		case 0x04:
			memadr = (memadr & 0xFF00FF) | (data << 8);
			break;

		case 0x05:
			memadr = (memadr & 0xFFFF00) | data;
			break;

		case 0x06:  // memory data
			BUSY_Time += 28 * 6 / 9;
			writeMem(memadr, data);
			memadr = (memadr + 1) & 0xFFFFFF;
			break;
		
		case 0xF8:
			// TODO use these
			fm_l = data & 0x7;
			fm_r = (data >> 3) & 0x7;
			break;

		case 0xF9:
			pcm_l = data & 0x7;
			pcm_r = (data >> 3) & 0x7;
			break;
		}
	}
	
	regs[reg] = data;
}

u8 YMF278::peekRegOPL4(u8 reg, const EmuTime &time)
{
	BUSY_Time = time;
	
	u8 result;
	switch(reg) {
		case 2: // 3 upper bits are device ID
			result = (regs[2] & 0x1F) | 0x20;
			break;
			
		case 6: // Memory Data Register
			result = readMem(memadr);
			break;

		default:
			result = regs[reg];
			break;
	}
	return result;
}

u8 YMF278::readRegOPL4(u8 reg, const EmuTime &time)
{
	BUSY_Time = time;
	
	u8 result;
	switch(reg) {
		case 2: // 3 upper bits are device ID
			result = (regs[2] & 0x1F) | 0x20;
			break;
			
		case 6: // Memory Data Register
			BUSY_Time += 38 * 6 / 9;
			result = readMem(memadr);
			memadr = (memadr + 1) & 0xFFFFFF;
			break;

		default:
			result = regs[reg];
			break;
	}
	return result;
}

u8 YMF278::peekStatus(const EmuTime &time)
{
  return 0;
}

u8 YMF278::readStatus(const EmuTime &time)
{
  return 0;
}


YMF278::YMF278(short volume, const u8 *rom_data, size_t rom_data_size, u8 *ram_data, size_t ram_data_size, const EmuTime &time)
{
  internalMuted = true;
  rom = rom_data;
  romSize = rom_data_size;
  ram = ram_data;
  ramSize = ram_data_size;
  LD_Time = 0;
  BUSY_Time = 0;
  memadr = 0;
  endRom = rom_data_size;
  endRam = endRom + ram_data_size;
  oplOversampling = 1;
  setInternalVolume(volume);
  reset(time);
}

YMF278::~YMF278()
{
}

void YMF278::reset(const EmuTime &time)
{
	eg_timer = 0;
	eg_cnt   = 0;

    int i;
	for (i = 0; i < 24; i++) {
		slots[i].reset();
	}
	for (i = 255; i >= 0; i--) { // reverse order to avoid UMR
		writeRegOPL4(i, 0, time);
	}
	setInternalMute(true);
	wavetblhdr = memmode = memadr = 0;
	fm_l = fm_r = pcm_l = pcm_r = 0;
	BUSY_Time = time;
	LD_Time = time;
}

void YMF278::setSampleRate(int sampleRate, int Oversampling)
{
    oplOversampling = Oversampling;
	eg_timer_add = (unsigned int)((1 << Y278_EG_SH) / oplOversampling);
}

void YMF278::setInternalVolume(short newVolume)
{
    newVolume /= 32;
	// Volume table, 1 = -0.375dB, 8 = -3dB, 256 = -96dB
    int i;
	for (i = 0; i < 256; i++) {
		volume[i] = (int)(4.0 * (double)newVolume * pow(2.0, (-0.375 / 6) * i));
	}
	for (i = 256; i < 256 * 4; i++) {
		volume[i] = 0;
	}
}


u8 YMF278::readMem(unsigned int address)
{
  if (rom && address < endRom) return rom[address];
  if (ram && address >= endRom && address < endRam) return ram[address - endRom];
  return 0xFF;
}

void YMF278::writeMem(unsigned int address, u8 value)
{
  if (ram && address >= endRom && address < endRam) ram[address - endRom] = value;
}

// End of YMF278 OPL4 wave core.

typedef enum
{
  OPL_COMMAND_WRITE_PORT = 0,
  OPL_COMMAND_WRITE_FM_REG,
  OPL_COMMAND_WRITE_WAVE_REG
} OPL_COMMAND_TYPE;

typedef struct
{
  u8 cmd;
  u16 index;
  u8 value;
} OPL_COMMAND;

typedef struct
{
  OPL_MODE mode;
  u8 enabled;
  const esp_partition_t *rom_part;
  const u8 *rom;
  spi_flash_mmap_handle_t rom_mmap;
  size_t rom_size;
  u8 *ram;
  size_t ram_size;
  u8 fm_latch_bank;
  u8 fm_latch[OPL_FM_BANK_COUNT];
  u8 wave_latch;
  u8 status;
  u8 fm_regs[OPL_FM_BANK_COUNT][OPL_FM_REG_COUNT];
  u8 wave_regs[OPL_WAVE_REG_COUNT];
  YMF262 *fm;
  void *fm_mem;
  YMF278 *wave;
  void *wave_mem;
  QueueHandle_t command_queue;
} OPL_STATE;

OPL_STATE *g_opl_state = NULL;
SemaphoreHandle_t g_opl_mtx = NULL;

esp_err_t opl_lock_init()
{
  if (g_opl_mtx) return ESP_OK;

  g_opl_mtx = xSemaphoreCreateMutexWithCaps(OPL_HOT_ALLOC_CAPS);
  if (!g_opl_mtx)
  {
    ESP_LOGE(OPL_TAG, "OPL mutex allocation failed");
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

int opl_lock()
{
  if (opl_lock_init() != ESP_OK) return 0;
  return xSemaphoreTake(g_opl_mtx, portMAX_DELAY) == pdTRUE;
}

void opl_unlock()
{
  if (g_opl_mtx) xSemaphoreGive(g_opl_mtx);
}

void opl_lock_deinit()
{
  if (!g_opl_mtx) return;

  vQueueDelete(g_opl_mtx);
  g_opl_mtx = NULL;
}

esp_err_t opl_alloc_state()
{
  if (g_opl_state) return ESP_OK;

  g_opl_state = (OPL_STATE *)heap_caps_calloc(1, sizeof(OPL_STATE), OPL_HOT_ALLOC_CAPS);
  if (!g_opl_state)
  {
    ESP_LOGE(OPL_TAG, "OPL state allocation failed: %u bytes internal SRAM", (unsigned)sizeof(OPL_STATE));
    return ESP_ERR_NO_MEM;
  }

  g_opl_state->command_queue = xQueueCreateWithCaps(
    OPL_COMMAND_QUEUE_LEN,
    sizeof(OPL_COMMAND),
    OPL_HOT_ALLOC_CAPS
  );
  if (!g_opl_state->command_queue)
  {
    ESP_LOGE(OPL_TAG, "OPL command queue allocation failed: %u commands internal SRAM", (unsigned)OPL_COMMAND_QUEUE_LEN);
    heap_caps_free(g_opl_state);
    g_opl_state = NULL;
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

void opl_free_state()
{
  if (!g_opl_state) return;

  if (g_opl_state->command_queue)
  {
    vQueueDelete(g_opl_state->command_queue);
    g_opl_state->command_queue = NULL;
  }

  heap_caps_free(g_opl_state);
  g_opl_state = NULL;
}

esp_err_t opl_map_rom(OPL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  if (state->rom) return ESP_OK;

  state->rom_part = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA,
    (esp_partition_subtype_t)OPL_ROM_PART_SUBTYPE,
    OPL_ROM_PART_LABEL
  );

  if (!state->rom_part)
  {
    ESP_LOGE(OPL_TAG, "OPL ROM partition '%s' not found", OPL_ROM_PART_LABEL);
    return ESP_ERR_NOT_FOUND;
  }

  if (state->rom_part->size < OPL_ROM_SIZE)
  {
    ESP_LOGE(OPL_TAG, "OPL ROM partition too small: %u", (unsigned)state->rom_part->size);
    return ESP_ERR_INVALID_SIZE;
  }

  const void *rom_ptr = NULL;
  esp_err_t err = esp_partition_mmap(
    state->rom_part,
    0,
    OPL_ROM_SIZE,
    ESP_PARTITION_MMAP_DATA,
    &rom_ptr,
    &state->rom_mmap
  );
  if (err != ESP_OK) return err;

  state->rom = (const u8 *)rom_ptr;
  state->rom_size = OPL_ROM_SIZE;
  return ESP_OK;
}

void opl_unmap_rom(OPL_STATE *state)
{
  if (!state || !state->rom) return;

  esp_partition_munmap(state->rom_mmap);
  state->rom = NULL;
  state->rom_mmap = 0;
  state->rom_size = 0;
  state->rom_part = NULL;
}

esp_err_t opl_alloc_ram(OPL_STATE *state, size_t ram_size)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  if (!ram_size) return ESP_OK;

  state->ram = (u8 *)heap_caps_malloc(ram_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!state->ram) return ESP_ERR_NO_MEM;

  memset(state->ram, 0, ram_size);
  state->ram_size = ram_size;
  return ESP_OK;
}

void opl_free_ram(OPL_STATE *state)
{
  if (!state || !state->ram) return;

  heap_caps_free(state->ram);
  state->ram = NULL;
  state->ram_size = 0;
}

esp_err_t opl_alloc_fm(OPL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  if (state->fm) return ESP_OK;

  esp_err_t err = opl_alloc_fm_tables();
  if (err != ESP_OK) return err;

  state->fm_mem = heap_caps_calloc(1, sizeof(YMF262), OPL_HOT_ALLOC_CAPS);
  if (!state->fm_mem)
  {
    ESP_LOGE(OPL_TAG, "YMF262 allocation failed: %u bytes internal SRAM", (unsigned)sizeof(YMF262));
    opl_free_fm_tables();
    return ESP_ERR_NO_MEM;
  }

  state->fm = new (state->fm_mem) YMF262(OPL_MOONSOUND_VOLUME, 0, NULL);
  state->fm->setVolume(OPL_MOONSOUND_VOLUME);
  state->fm->setSampleRate(44100, 1);
  return ESP_OK;
}

void opl_free_fm(OPL_STATE *state)
{
  if (!state || !state->fm) return;

  state->fm->~YMF262();
  heap_caps_free(state->fm_mem);
  state->fm = NULL;
  state->fm_mem = NULL;
  g_ymf262_chan_out = NULL;
  opl_free_fm_tables();
}

esp_err_t opl_alloc_wave(OPL_STATE *state)
{
  if (!state) return ESP_ERR_INVALID_ARG;
  if (state->wave) return ESP_OK;
  if (!state->rom || state->rom_size < OPL_ROM_SIZE) return ESP_ERR_INVALID_SIZE;

  state->wave_mem = heap_caps_calloc(1, sizeof(YMF278), OPL_HOT_ALLOC_CAPS);
  if (!state->wave_mem)
  {
    ESP_LOGE(OPL_TAG, "YMF278 allocation failed: %u bytes internal SRAM", (unsigned)sizeof(YMF278));
    return ESP_ERR_NO_MEM;
  }

  state->wave = new (state->wave_mem) YMF278(OPL_MOONSOUND_VOLUME, state->rom, state->rom_size, state->ram, state->ram_size, 0);
  state->wave->setInternalVolume(OPL_MOONSOUND_VOLUME);
  state->wave->setSampleRate(44100, 1);
  return ESP_OK;
}

void opl_free_wave(OPL_STATE *state)
{
  if (!state || !state->wave) return;

  state->wave->~YMF278();
  heap_caps_free(state->wave_mem);
  state->wave = NULL;
  state->wave_mem = NULL;
}

void opl_disable_unlocked()
{
  if (!g_opl_state)
  {
    opl_free_fm_tables();
    return;
  }

  if (g_opl_state->enabled)
  {
    g_opl_state->enabled = 0;
    if (g_opl_state->fm)
    {
      for (int i = 0; i < 9; i++)
        g_opl_state->fm->writeReg((uint8_t)(0xB0 + i), g_opl_state->fm_regs[0][0xB0 + i] & 0xDF, 0);
    }
    if (g_opl_state->wave)
    {
      for (int i = 0; i < 24; i++)
        g_opl_state->wave->writeRegOPL4((uint8_t)(0x68 + i), g_opl_state->wave_regs[0x68 + i] & 0x3F, 0);
    }
  }

  opl_free_wave(g_opl_state);
  opl_free_ram(g_opl_state);
  opl_unmap_rom(g_opl_state);
  opl_free_fm(g_opl_state);
  opl_free_state();
}

void opl_reset_unlocked()
{
  if (!g_opl_state) return;

  g_opl_state->fm_latch_bank = 0;
  g_opl_state->fm_latch[0] = 0;
  g_opl_state->fm_latch[1] = 0;
  g_opl_state->wave_latch = 0;
  g_opl_state->status = 0;
  memset(g_opl_state->fm_regs, 0, sizeof(g_opl_state->fm_regs));
  memset(g_opl_state->wave_regs, 0, sizeof(g_opl_state->wave_regs));
  if (g_opl_state->fm)
  {
    g_opl_state->fm->setRenderMode(OPL_FM_RENDER_OPL3_FULL);
    g_opl_state->fm->reset(0);
  }
  if (g_opl_state->wave) g_opl_state->wave->reset(0);
}

esp_err_t opl_enable(OPL_MODE mode, size_t ram_size)
{
  if (mode != OPL_MODE_YMF262 && mode != OPL_MODE_YMF278) return ESP_ERR_INVALID_ARG;
  if (!opl_lock()) return ESP_ERR_NO_MEM;

  esp_err_t err = ESP_OK;
  opl_disable_unlocked();

  err = opl_alloc_state();
  if (err != ESP_OK) goto fail;

  g_opl_state->mode = mode;

  err = opl_alloc_fm(g_opl_state);
  if (err != ESP_OK)
  {
    opl_free_state();
    goto fail;
  }

  if (mode == OPL_MODE_YMF278)
  {
    err = opl_map_rom(g_opl_state);
    if (err != ESP_OK)
    {
      opl_free_fm(g_opl_state);
      opl_free_state();
      goto fail;
    }

    if (!ram_size) ram_size = OPL_YMF278_DEFAULT_RAM_SIZE;
    err = opl_alloc_ram(g_opl_state, ram_size);
    if (err != ESP_OK)
    {
      opl_unmap_rom(g_opl_state);
      opl_free_fm(g_opl_state);
      opl_free_state();
      goto fail;
    }

    err = opl_alloc_wave(g_opl_state);
    if (err != ESP_OK)
    {
      opl_free_ram(g_opl_state);
      opl_unmap_rom(g_opl_state);
      opl_free_fm(g_opl_state);
      opl_free_state();
      goto fail;
    }
  }

  g_opl_state->enabled = 1;
  opl_reset_unlocked();

fail:
  opl_unlock();
  return err;
}

void opl_disable()
{
  if (!opl_lock()) return;
  opl_disable_unlocked();
  opl_unlock();
  opl_lock_deinit();
}

void opl_reset()
{
  if (!opl_lock()) return;
  opl_reset_unlocked();
  opl_unlock();
}

int opl_is_enabled()
{
  if (!opl_lock()) return 0;
  int enabled = g_opl_state && g_opl_state->enabled;
  opl_unlock();
  return enabled;
}

esp_err_t opl_get_info(OPL_INFO *info)
{
  if (!info) return ESP_ERR_INVALID_ARG;

  memset(info, 0, sizeof(OPL_INFO));
  if (!opl_lock()) return ESP_ERR_NO_MEM;
  if (!g_opl_state)
  {
    opl_unlock();
    return ESP_OK;
  }

  info->mode = g_opl_state->mode;
  info->enabled = g_opl_state->enabled;
  info->rom_size = g_opl_state->rom_size;
  info->ram_size = g_opl_state->ram_size;
  info->fm_latch_bank = g_opl_state->fm_latch_bank;
  info->fm_latch[0] = g_opl_state->fm_latch[0];
  info->fm_latch[1] = g_opl_state->fm_latch[1];
  info->wave_latch = g_opl_state->wave_latch;
  info->status = g_opl_state->status;
  if (g_opl_state->command_queue) info->queued_writes = uxQueueMessagesWaiting(g_opl_state->command_queue);
  opl_unlock();
  return ESP_OK;
}

esp_err_t opl_set_fm_render_mode(OPL_FM_RENDER_MODE mode)
{
  if (mode != OPL_FM_RENDER_OPL3_FULL && mode != OPL_FM_RENDER_OPL2_MELODY)
    return ESP_ERR_INVALID_ARG;

  if (!opl_lock()) return ESP_ERR_NO_MEM;
  if (!g_opl_state || !g_opl_state->enabled || !g_opl_state->fm)
  {
    opl_unlock();
    return ESP_ERR_INVALID_STATE;
  }

  if (mode == OPL_FM_RENDER_OPL2_MELODY && g_opl_state->mode != OPL_MODE_YMF262)
  {
    opl_unlock();
    return ESP_ERR_INVALID_STATE;
  }

  g_opl_state->fm->setRenderMode(mode);
  opl_unlock();
  return ESP_OK;
}

esp_err_t opl_set_volume(u8 volume)
{
  if (!g_opl_mtx)
  {
    opl_global_volume = volume;
    return ESP_OK;
  }

  if (xSemaphoreTake(g_opl_mtx, portMAX_DELAY) != pdTRUE)
    return ESP_ERR_TIMEOUT;

  opl_global_volume = volume;

  xSemaphoreGive(g_opl_mtx);
  return ESP_OK;
}

u8 opl_get_volume()
{
  return opl_global_volume;
}

u8 opl_read_status_unlocked()
{
  if (!g_opl_state) return 0;

  u8 status = g_opl_state->status;
  if (g_opl_state->fm) status |= g_opl_state->fm->peekStatus();
  if (g_opl_state->wave) status |= g_opl_state->wave->peekStatus(0);
  return status;
}

u8 opl_read_status()
{
  if (!opl_lock()) return 0;
  u8 status = opl_read_status_unlocked();
  opl_unlock();
  return status;
}

u8 opl_read_fm_reg_unlocked(u16 reg)
{
  if (!g_opl_state) return 0xFF;

  u8 bank = (reg >> 8) & 1;
  u8 reg_idx = reg & 0xFF;
  return g_opl_state->fm_regs[bank][reg_idx];
}

void opl_write_fm_reg_unlocked(u16 reg, u8 value)
{
  if (!g_opl_state) return;

  u8 bank = (reg >> 8) & 1;
  u8 reg_idx = reg & 0xFF;
  g_opl_state->fm_regs[bank][reg_idx] = value;
  if (g_opl_state->fm) g_opl_state->fm->writeReg(reg & 0x1FF, value, 0);
}

int opl_write_fm_reg_cached_unlocked(u16 reg, u8 value)
{
  if (!g_opl_state) return 0;

  u8 bank = (reg >> 8) & 1;
  u8 reg_idx = reg & 0xFF;
  if (g_opl_state->fm_regs[bank][reg_idx] == value) return 0;

  opl_write_fm_reg_unlocked(reg, value);
  return 1;
}

u8 opl_read_wave_reg_unlocked(u8 reg)
{
  if (!g_opl_state) return 0xFF;
  if (g_opl_state->mode != OPL_MODE_YMF278) return 0xFF;

  if (g_opl_state->wave) return g_opl_state->wave->peekRegOPL4(reg, 0);
  return g_opl_state->wave_regs[reg];
}

void opl_write_wave_reg_unlocked(u8 reg, u8 value)
{
  if (!g_opl_state) return;
  if (g_opl_state->mode != OPL_MODE_YMF278) return;

  g_opl_state->wave_regs[reg] = value;
  if (g_opl_state->wave) g_opl_state->wave->writeRegOPL4(reg, value, 0);
}

u8 opl_read_port_unlocked(u16 port)
{
  switch (port)
  {
    case OPL_PORT_FM_REG1:
    case OPL_PORT_FM_REG2:
      return opl_read_status_unlocked();

    case OPL_PORT_WAVE_DAT:
      if (!g_opl_state) return 0xFF;
      return opl_read_wave_reg_unlocked(g_opl_state->wave_latch);
  }

  return 0xFF;
}

void opl_write_port_unlocked(u16 port, u8 value)
{
  if (!g_opl_state) return;

  switch (port)
  {
    case OPL_PORT_FM_REG1:
      g_opl_state->fm_latch_bank = 0;
      g_opl_state->fm_latch[0] = value;
      return;

    case OPL_PORT_FM_REG2:
      g_opl_state->fm_latch_bank = 1;
      g_opl_state->fm_latch[1] = value;
      return;

    case OPL_PORT_FM_DAT1:
    case OPL_PORT_FM_DAT2:
    {
      u8 bank = g_opl_state->fm_latch_bank & 1;
      u8 reg = g_opl_state->fm_latch[bank];
      opl_write_fm_reg_unlocked(((u16)bank << 8) | reg, value);
      return;
    }

    case OPL_PORT_WAVE_REG:
      g_opl_state->wave_latch = value;
      return;

    case OPL_PORT_WAVE_DAT:
      opl_write_wave_reg_unlocked(g_opl_state->wave_latch, value);
      return;
  }
}

u8 opl_read_port(u16 port)
{
  if (!opl_lock()) return 0xFF;
  u8 value = opl_read_port_unlocked(port);
  opl_unlock();
  return value;
}

void opl_write_port(u16 port, u8 value)
{
  if (!opl_lock()) return;
  opl_write_port_unlocked(port, value);
  opl_unlock();
}

esp_err_t opl_queue_command(u8 cmd, u16 index, u8 value)
{
  if (!opl_lock()) return ESP_ERR_NO_MEM;
  if (!g_opl_state || !g_opl_state->command_queue)
  {
    opl_unlock();
    return ESP_ERR_INVALID_STATE;
  }

  OPL_COMMAND command = {};
  command.cmd = cmd;
  command.index = index;
  command.value = value;

  BaseType_t ok = xQueueSend(g_opl_state->command_queue, &command, 0);
  opl_unlock();
  if (ok != pdTRUE) return ESP_ERR_TIMEOUT;
  return ESP_OK;
}

esp_err_t opl_queue_write_port(u16 port, u8 value)
{
  return opl_queue_command(OPL_COMMAND_WRITE_PORT, port, value);
}

esp_err_t opl_queue_write_fm_reg(u16 reg, u8 value)
{
  return opl_queue_command(OPL_COMMAND_WRITE_FM_REG, reg & 0x01FF, value);
}

esp_err_t opl_queue_write_wave_reg(u8 reg, u8 value)
{
  return opl_queue_command(OPL_COMMAND_WRITE_WAVE_REG, reg, value);
}

void opl_process_queued_writes_unlocked()
{
  if (!g_opl_state || !g_opl_state->command_queue) return;

  OPL_COMMAND command;

  while (xQueueReceive(g_opl_state->command_queue, &command, 0) == pdTRUE)
  {
    switch (command.cmd)
    {
      case OPL_COMMAND_WRITE_PORT:
        opl_write_port_unlocked(command.index, command.value);
      break;

      case OPL_COMMAND_WRITE_FM_REG:
        opl_write_fm_reg_unlocked(command.index, command.value);
      break;

      case OPL_COMMAND_WRITE_WAVE_REG:
        opl_write_wave_reg_unlocked((u8)command.index, command.value);
      break;

      default:
      break;
    }
  }
}

u8 opl_read_fm_reg(u16 reg)
{
  if (!opl_lock()) return 0xFF;
  u8 value = opl_read_fm_reg_unlocked(reg);
  opl_unlock();
  return value;
}

void opl_write_fm_reg(u16 reg, u8 value)
{
  if (!opl_lock()) return;
  opl_write_fm_reg_unlocked(reg, value);
  opl_unlock();
}

esp_err_t opl_write_fm_regs(const OPL_FM_REG_WRITE *writes, size_t count)
{
  if (!writes && count) return ESP_ERR_INVALID_ARG;
  if (!count) return ESP_OK;
  if (!opl_lock()) return ESP_ERR_NO_MEM;

  for (size_t i = 0; i < count; i++)
    opl_write_fm_reg_cached_unlocked(writes[i].reg & 0x01FF, writes[i].value);

  opl_unlock();
  return ESP_OK;
}

u8 opl_read_wave_reg(u8 reg)
{
  if (!opl_lock()) return 0xFF;
  u8 value = opl_read_wave_reg_unlocked(reg);
  opl_unlock();
  return value;
}

void opl_write_wave_reg(u8 reg, u8 value)
{
  if (!opl_lock()) return;
  opl_write_wave_reg_unlocked(reg, value);
  opl_unlock();
}

void opl_render(float *mix, int sample_count, int sample_rate)
{
  if (!mix || sample_count <= 0 || sample_rate <= 0) return;
  if (!opl_lock()) return;
  if (!g_opl_state || !g_opl_state->enabled)
  {
    opl_unlock();
    return;
  }

  opl_process_queued_writes_unlocked();

  if (g_opl_state->fm) g_opl_state->fm->render(mix, sample_count, sample_rate);
  if (g_opl_state->wave) g_opl_state->wave->render(mix, sample_count, sample_rate);
  opl_unlock();
}


const char *opl_mode_name(OPL_MODE mode)
{
  switch (mode)
  {
    case OPL_MODE_OFF: return "off";
    case OPL_MODE_YMF262: return "ymf262";
    case OPL_MODE_YMF278: return "ymf278";
  }

  return "unknown";
}

int opl_parse_u32_arg(const char *text, u32 *out)
{
  if (!text || !out) return 0;

  char *endp = NULL;
  unsigned long value = strtoul(text, &endp, 0);
  if (!endp || *endp) return 0;

  *out = (u32)value;
  return 1;
}

int opl_cmd_on(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Usage: opl on <ymf262|ymf278> [ram_kb]\r\n");
    return 1;
  }

  OPL_MODE mode = OPL_MODE_OFF;
  size_t ram_size = 0;

  if (!strcmp(argv[2], "ymf262") || !strcmp(argv[2], "opl3") || !strcmp(argv[2], "adlib"))
  {
    mode = OPL_MODE_YMF262;
  }
  else if (!strcmp(argv[2], "ymf278") || !strcmp(argv[2], "opl4") || !strcmp(argv[2], "moon"))
  {
    mode = OPL_MODE_YMF278;
    ram_size = OPL_YMF278_DEFAULT_RAM_SIZE;
  }
  else
  {
    printf("Bad OPL mode: %s\r\n", argv[2]);
    return 1;
  }

  if (argc >= 4)
  {
    u32 ram_kb = 0;
    if (!opl_parse_u32_arg(argv[3], &ram_kb))
    {
      printf("Bad <ram_kb>: %s\r\n", argv[3]);
      return 1;
    }
    ram_size = (size_t)ram_kb * 1024;
  }

  esp_err_t err = opl_enable(mode, ram_size);
  if (err != ESP_OK)
  {
    printf("OPL enable failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  printf("OPL enabled: %s", opl_mode_name(mode));
  if (mode == OPL_MODE_YMF278) printf(", RAM %u KB", (unsigned)(ram_size / 1024));
  printf("\r\n");
  return 0;
}

int opl_cmd_info()
{
  OPL_INFO info;
  esp_err_t err = opl_get_info(&info);
  if (err != ESP_OK)
  {
    printf("OPL info failed: %s\r\n", esp_err_to_name(err));
    return err;
  }

  printf("OPL info\r\n");
  printf("  enabled        : %s\r\n", info.enabled ? "yes" : "no");
  printf("  mode           : %s\r\n", opl_mode_name(info.mode));
  printf("  ROM size       : %u\r\n", (unsigned)info.rom_size);
  printf("  RAM size       : %u\r\n", (unsigned)info.ram_size);
  printf("  status         : 0x%02X\r\n", opl_read_status());
  printf("  queued writes  : %u\r\n", (unsigned)info.queued_writes);
  printf("  volume         : %u\r\n", (unsigned)opl_get_volume());
  printf("  FM latch bank  : %u\r\n", info.fm_latch_bank);
  printf("  FM latch[0]    : 0x%02X\r\n", info.fm_latch[0]);
  printf("  FM latch[1]    : 0x%02X\r\n", info.fm_latch[1]);
  printf("  wave latch     : 0x%02X\r\n", info.wave_latch);
  return 0;
}


int opl_cmd_vol(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("OPL volume: %u\r\n", (unsigned)opl_get_volume());
    return 0;
  }

  u32 value = 0;
  if (!opl_parse_u32_arg(argv[2], &value) || value > 255)
  {
    printf("Bad <volume>: %s (expected 0..255)\r\n", argv[2]);
    return 1;
  }

  esp_err_t err = opl_set_volume((u8)value);
  if (err != ESP_OK)
  {
    printf("OPL volume failed: %s\r\n", esp_err_to_name(err));
    return 1;
  }

  printf("OPL volume: %u\r\n", (unsigned)value);
  return 0;
}

int opl_cmd_rd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Usage: opl rd <port>\r\n");
    return 1;
  }

  u32 port = 0;
  if (!opl_parse_u32_arg(argv[2], &port) || port > 0xFFFF)
  {
    printf("Bad <port>: %s\r\n", argv[2]);
    return 1;
  }

  printf("port 0x%04X = 0x%02X\r\n", (unsigned)port, opl_read_port((u16)port));
  return 0;
}

int opl_cmd_wr(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Usage: opl wr <port> <value>\r\n");
    return 1;
  }

  u32 port = 0;
  u32 value = 0;
  if (!opl_parse_u32_arg(argv[2], &port) || port > 0xFFFF)
  {
    printf("Bad <port>: %s\r\n", argv[2]);
    return 1;
  }
  if (!opl_parse_u32_arg(argv[3], &value) || value > 0xFF)
  {
    printf("Bad <value>: %s\r\n", argv[3]);
    return 1;
  }

  opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
  opl_write_port((u16)port, (u8)value);
  printf("port 0x%04X <- 0x%02X\r\n", (unsigned)port, (unsigned)value);
  return 0;
}

int opl_cmd_fmwr(int argc, char **argv)
{
  if (argc < 5)
  {
    printf("Usage: opl fmwr <bank> <reg> <value>\r\n");
    return 1;
  }

  u32 bank = 0;
  u32 reg = 0;
  u32 value = 0;
  if (!opl_parse_u32_arg(argv[2], &bank) || bank >= OPL_FM_BANK_COUNT)
  {
    printf("Bad <bank>: %s\r\n", argv[2]);
    return 1;
  }
  if (!opl_parse_u32_arg(argv[3], &reg) || reg >= OPL_FM_REG_COUNT)
  {
    printf("Bad <reg>: %s\r\n", argv[3]);
    return 1;
  }
  if (!opl_parse_u32_arg(argv[4], &value) || value > 0xFF)
  {
    printf("Bad <value>: %s\r\n", argv[4]);
    return 1;
  }

  opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
  opl_write_fm_reg((u16)((bank << 8) | reg), (u8)value);
  printf("fm[%u][0x%02X] <- 0x%02X\r\n", (unsigned)bank, (unsigned)reg, (unsigned)value);
  return 0;
}

int opl_cmd_fmrd(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Usage: opl fmrd <bank> <reg>\r\n");
    return 1;
  }

  u32 bank = 0;
  u32 reg = 0;
  if (!opl_parse_u32_arg(argv[2], &bank) || bank >= OPL_FM_BANK_COUNT)
  {
    printf("Bad <bank>: %s\r\n", argv[2]);
    return 1;
  }
  if (!opl_parse_u32_arg(argv[3], &reg) || reg >= OPL_FM_REG_COUNT)
  {
    printf("Bad <reg>: %s\r\n", argv[3]);
    return 1;
  }

  printf("fm[%u][0x%02X] = 0x%02X\r\n", (unsigned)bank, (unsigned)reg,
    opl_read_fm_reg((u16)((bank << 8) | reg)));
  return 0;
}

int opl_cmd_wavwr(int argc, char **argv)
{
  if (argc < 4)
  {
    printf("Usage: opl wavwr <reg> <value>\r\n");
    return 1;
  }

  u32 reg = 0;
  u32 value = 0;
  if (!opl_parse_u32_arg(argv[2], &reg) || reg >= OPL_WAVE_REG_COUNT)
  {
    printf("Bad <reg>: %s\r\n", argv[2]);
    return 1;
  }
  if (!opl_parse_u32_arg(argv[3], &value) || value > 0xFF)
  {
    printf("Bad <value>: %s\r\n", argv[3]);
    return 1;
  }

  opl_write_wave_reg((u8)reg, (u8)value);
  printf("wave[0x%02X] <- 0x%02X\r\n", (unsigned)reg, (unsigned)value);
  return 0;
}

int opl_cmd_wavrd(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Usage: opl wavrd <reg>\r\n");
    return 1;
  }

  u32 reg = 0;
  if (!opl_parse_u32_arg(argv[2], &reg) || reg >= OPL_WAVE_REG_COUNT)
  {
    printf("Bad <reg>: %s\r\n", argv[2]);
    return 1;
  }

  printf("wave[0x%02X] = 0x%02X\r\n", (unsigned)reg, opl_read_wave_reg((u8)reg));
  return 0;
}


void opl_test_write_port(u16 port, u8 value)
{
  opl_write_port(port, value);
  printf("  out 0x%04X <- 0x%02X\r\n", (unsigned)port, (unsigned)value);
}

void opl_test_write_fm1(u8 reg, u8 value)
{
  opl_test_write_port(OPL_PORT_FM_REG1, reg);
  opl_test_write_port(OPL_PORT_FM_DAT1, value);
}

void opl_test_write_fm2(u8 reg, u8 value)
{
  opl_test_write_port(OPL_PORT_FM_REG2, reg);
  opl_test_write_port(OPL_PORT_FM_DAT2, value);
}

void opl_test_write_wave(u8 reg, u8 value)
{
  opl_test_write_port(OPL_PORT_WAVE_REG, reg);
  opl_test_write_port(OPL_PORT_WAVE_DAT, value);
}

esp_err_t opl_test_ensure_mode(OPL_MODE mode)
{
  OPL_INFO info;
  esp_err_t err = opl_get_info(&info);
  if (err != ESP_OK) return err;

  if (info.enabled)
  {
    if (info.mode == mode) return ESP_OK;
    if (mode == OPL_MODE_YMF262 && info.mode == OPL_MODE_YMF278) return ESP_OK;
  }

  return opl_enable(mode, 0);
}

void opl_test_write_mdr_init()
{
  printf("MDR MoonSound init register stream\r\n");
  opl_test_write_fm2(0x04, 0x00);
  opl_test_write_fm2(0x05, 0x03);
  opl_test_write_fm1(0xBD, 0x00);
  opl_test_write_wave(0x02, 0x10);
}

void opl_test_write_fm_note()
{
  printf("FM two-operator test note register stream\r\n");
  opl_test_write_fm1(0xBD, 0x00);
  opl_test_write_fm2(0x05, 0x03);

  opl_test_write_fm1(0x20, 0x21);
  opl_test_write_fm1(0x23, 0x01);
  opl_test_write_fm1(0x40, 0x3F);
  opl_test_write_fm1(0x43, 0x00);
  opl_test_write_fm1(0x60, 0xF0);
  opl_test_write_fm1(0x63, 0xF0);
  opl_test_write_fm1(0x80, 0x77);
  opl_test_write_fm1(0x83, 0x77);
  opl_test_write_fm1(0xE0, 0x00);
  opl_test_write_fm1(0xE3, 0x00);
  opl_test_write_fm1(0xC0, 0x30);
  opl_test_write_fm1(0xA0, 0x6B);
  opl_test_write_fm1(0xB0, 0x31);
}

void opl_test_write_wave_note(u16 tone)
{
  u16 fnum = 0x0200;
  u8 octave = 0;

  printf("OPL4 wave channel 0 test note register stream, tone %u\r\n", (unsigned)tone);
  opl_test_write_mdr_init();

  opl_test_write_wave(0x68, 0x40);
  opl_test_write_wave(0x08, tone & 0xFF);
  opl_test_write_wave(0x20, ((tone >> 8) & 1) | ((fnum & 0x7F) << 1));
  opl_test_write_wave(0x38, ((octave & 0x0F) << 4) | ((fnum >> 7) & 0x07));
  opl_test_write_wave(0x50, 0x01);
  opl_test_write_wave(0x80, 0x00);
  opl_test_write_wave(0x98, 0xF0);
  opl_test_write_wave(0xB0, 0xF0);
  opl_test_write_wave(0xC8, 0x0F);
  opl_test_write_wave(0xE0, 0x00);
  opl_test_write_wave(0x68, 0xA0);
}

void opl_test_write_stop()
{
  printf("OPL test key-off register stream\r\n");
  opl_test_write_fm1(0xB0, opl_read_fm_reg(0x00B0) & 0xDF);
  opl_test_write_wave(0x68, opl_read_wave_reg(0x68) & 0x3F);
}

int opl_cmd_test(int argc, char **argv)
{
  if (argc < 3)
  {
    printf("Usage:\r\n");
    printf("  opl test mdr-init\r\n");
    printf("  opl test fm\r\n");
    printf("  opl test wave [tone]\r\n");
    printf("  opl test stop\r\n");
    return 1;
  }

  const char *test = argv[2];

  if (!strcmp(test, "mdr-init"))
  {
    esp_err_t err = opl_test_ensure_mode(OPL_MODE_YMF278);
    if (err != ESP_OK)
    {
      printf("OPL test init failed: %s\r\n", esp_err_to_name(err));
      return err;
    }
    opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
    opl_test_write_mdr_init();
    return 0;
  }

  if (!strcmp(test, "fm"))
  {
    esp_err_t err = opl_test_ensure_mode(OPL_MODE_YMF262);
    if (err != ESP_OK)
    {
      printf("OPL FM test failed: %s\r\n", esp_err_to_name(err));
      return err;
    }
    opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
    opl_test_write_fm_note();
    return 0;
  }

  if (!strcmp(test, "wave"))
  {
    u32 tone = 0;
    if (argc >= 4 && (!opl_parse_u32_arg(argv[3], &tone) || tone > 0x1FF))
    {
      printf("Bad [tone]: %s\r\n", argv[3]);
      return 1;
    }

    esp_err_t err = opl_test_ensure_mode(OPL_MODE_YMF278);
    if (err != ESP_OK)
    {
      printf("OPL wave test failed: %s\r\n", esp_err_to_name(err));
      return err;
    }
    opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
    opl_test_write_wave_note((u16)tone);
    return 0;
  }

  if (!strcmp(test, "stop"))
  {
    if (!opl_is_enabled())
    {
      printf("OPL is not enabled\r\n");
      return 1;
    }
    opl_set_fm_render_mode(OPL_FM_RENDER_OPL3_FULL);
    opl_test_write_stop();
    return 0;
  }

  printf("Unknown OPL test: %s\r\n", test);
  return 1;
}

int opl_cmd(int argc, char **argv)
{
  if (argc < 2 || !argv[1])
  {
    printf("Usage:\r\n");
    printf("  opl on <ymf262|ymf278> [ram_kb]\r\n");
    printf("  opl off\r\n");
    printf("  opl reset\r\n");
    printf("  opl info\r\n");
    printf("  opl vol [0..255]\r\n");
    printf("  opl rd <port>\r\n");
    printf("  opl wr <port> <value>\r\n");
    printf("  opl fmrd <bank> <reg>\r\n");
    printf("  opl fmwr <bank> <reg> <value>\r\n");
    printf("  opl wavrd <reg>\r\n");
    printf("  opl wavwr <reg> <value>\r\n");
    printf("  opl test <mdr-init|fm|wave|stop> [tone]\r\n");
    return 0;
  }

  const char *op = argv[1];

  if (!strcmp(op, "on"))
    return opl_cmd_on(argc, argv);

  if (!strcmp(op, "off"))
  {
    opl_disable();
    printf("OPL disabled\r\n");
    return 0;
  }

  if (!strcmp(op, "reset"))
  {
    opl_reset();
    printf("OPL reset\r\n");
    return 0;
  }

  if (!strcmp(op, "info"))
    return opl_cmd_info();

  if (!strcmp(op, "vol"))
    return opl_cmd_vol(argc, argv);

  if (!strcmp(op, "rd"))
    return opl_cmd_rd(argc, argv);

  if (!strcmp(op, "wr"))
    return opl_cmd_wr(argc, argv);

  if (!strcmp(op, "fmrd"))
    return opl_cmd_fmrd(argc, argv);

  if (!strcmp(op, "fmwr"))
    return opl_cmd_fmwr(argc, argv);

  if (!strcmp(op, "wavrd"))
    return opl_cmd_wavrd(argc, argv);

  if (!strcmp(op, "wavwr"))
    return opl_cmd_wavwr(argc, argv);

  if (!strcmp(op, "test"))
    return opl_cmd_test(argc, argv);

  printf("Unknown opl subcommand: %s\r\n", op);
  return 1;
}

void opl_console_register_system_commands()
{
  const esp_console_cmd_t cmd =
  {
    .command  = "opl",
    .help     = "OPL commands: on/off/reset/info/vol/rd/wr/fmrd/fmwr/wavrd/wavwr/test",
    .hint     = NULL,
    .func     = &opl_cmd,
    .argtable = NULL,
  };

  ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
}
