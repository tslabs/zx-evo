// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.

#ifndef __YMF262_HH__
#define __YMF262_HH__

#include <string>
#include "sysdefs.h"

using namespace std;


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
    static void set() {
        //boardSetInt(0x8);
    }
    static void reset() {
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

		u8 ar;	// attack rate: AR<<2
		u8 dr;	// decay rate:  DR<<2
		u8 rr;	// release rate:RR<<2
		u8 KSR;	// key scale rate
		u8 ksl;	// keyscale level
		u8 ksr;	// key scale rate: kcode>>KSR
		u8 mul;	// multiple: mul_tab[ML]

		// Phase Generator 
		unsigned int Cnt;	// frequency counter
		unsigned int Incr;	// frequency counter step
		u8 FB;	// feedback shift value
		int op1_out[2];	// slot1 output for feedback
		u8 CON;	// connection (algorithm) type

		// Envelope Generator 
		u8 eg_type;	// percussive/non-percussive mode 
		u8 state;	// phase type
		unsigned int TL;	// total level: TL << 2
		int TLL;	// adjusted now TL
		int volume;	// envelope counter
		int sl;		// sustain level: sl_tab[SL]

		unsigned int eg_m_ar;// (attack state)
		u8 eg_sh_ar;	// (attack state)
		u8 eg_sel_ar;	// (attack state)
		unsigned int eg_m_dr;// (decay state)
		u8 eg_sh_dr;	// (decay state)
		u8 eg_sel_dr;	// (decay state)
		unsigned int eg_m_rr;// (release state)
		u8 eg_sh_rr;	// (release state)
		u8 eg_sel_rr;	// (release state)

		u8 key;	// 0 = KEY OFF, >0 = KEY ON

		// LFO 
		u8  AMmask;	// LFO Amplitude Modulation enable mask 
		u8 vib;	// LFO Phase Modulation enable flag (active high)

		// waveform select 
		u8 waveform_number;
		unsigned int wavetable;

		int connect;	// slot output pointer
};

class YMF262Channel
{
	public:
		YMF262Channel();
		void chan_calc(u8 LFO_AM);
		void chan_calc_ext(u8 LFO_AM);
		void CALC_FCSLOT(YMF262Slot &slot);

		YMF262Slot slots[2];

		int block_fnum;	// block+fnum
		int fc;		// Freq. Increment base
		int ksl_base;	// KeyScaleLevel Base step
		u8 kcode;	// key code (for key scaling)

		// there are 12 2-operator channels which can be combined in pairs
		// to form six 4-operator channel, they are:
		//  0 and 3,
		//  1 and 4,
		//  2 and 5,
		//  9 and 12,
		//  10 and 13,
		//  11 and 14
		u8 extended;	// set to 1 if this channel forms up a 4op channel with another channel(only used by first of pair of channels, ie 0,1,2 and 9,10,11) 
};

// Bitmask for register 0x04 
static const int R04_ST1          = 0x01;	// Timer1 Start
static const int R04_ST2          = 0x02;	// Timer2 Start
static const int R04_MASK_T2      = 0x20;	// Mask Timer2 flag 
static const int R04_MASK_T1      = 0x40;	// Mask Timer1 flag 
static const int R04_IRQ_RESET    = 0x80;	// IRQ RESET 

// Bitmask for status register 
static const int STATUS_T2      = R04_MASK_T2;
static const int STATUS_T1      = R04_MASK_T1;

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

		void callback(u8 flag);

	private:
		void writeRegForce(int r, u8 v, const EmuTime &time);
		void init_tables(void);
		void setStatus(u8 flag);
		void resetStatus(u8 flag);
		void changeStatusMask(u8 flag);
		void advance_lfo();
		void advance();
		void chan_calc_rhythm(bool noise);
		void set_mul(u8 sl, u8 v);
		void set_ksl_tl(u8 sl, u8 v);
		void set_ar_dr(u8 sl, u8 v);
		void set_sl_rr(u8 sl, u8 v);
		void update_channels(YMF262Channel &ch);
		void checkMute();
		bool checkMuteHelper();

        int buffer[MAX_BUFFER_SIZE];
		IRQHelper irq;
		Timer<12500, STATUS_T1> timer1;	//  80us
		Timer< 3125, STATUS_T2> timer2;	// 320us

        int oplOversampling;

        YMF262Channel channels[18];	// OPL3 chips have 18 channels

		u8 reg[512];

        unsigned int pan[18*4];		// channels output masks (0xffffffff = enable); 4 masks per one channel 

		unsigned int eg_cnt;		// global envelope generator counter
		unsigned int eg_timer;		// global envelope generator counter works at frequency = chipclock/288 (288=8*36) 
		unsigned int eg_timer_add;		// step of eg_timer

		unsigned int fn_tab[1024];		// fnumber->increment counter

		// LFO 
		u8 LFO_AM;
		u8 LFO_PM;
		
		u8 lfo_am_depth;
		u8 lfo_pm_depth_range;
		unsigned int lfo_am_cnt;
		unsigned int lfo_am_inc;
		unsigned int lfo_pm_cnt;
		unsigned int lfo_pm_inc;

		unsigned int noise_rng;		// 23 bit noise shift register
		unsigned int noise_p;		// current noise 'phase'
		unsigned int noise_f;		// current noise period

		bool OPL3_mode;			// OPL3 extension enable flag
		u8 rhythm;			// Rhythm mode
		u8 nts;			// NTS (note select)

		u8 status;			// status flag
		u8 status2;
		u8 statusMask;		// status mask

		int chanout[20];		// 18 channels + two phase modulation
		short maxVolume;
};

#endif

