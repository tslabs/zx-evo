#pragma once
/*
typedef unsigned int UINT32;
typedef signed int INT32;
typedef unsigned short UINT16;
typedef signed short INT16;
typedef unsigned char UINT8;
typedef signed char INT8;
*/
typedef INT16 stream_sample_t;
typedef stream_sample_t FMSAMPLE;

/* select bit size of output : 8 or 16 */
#define FM_SAMPLE_BITS 16

/* select timer system internal or external */
#define FM_INTERNAL_TIMER 0

/* --- speedup optimize --- */
/* busy flag enulation , The definition of FM_GET_TIME_NOW() is necessary. */
#define FM_BUSY_FLAG_SUPPORT 1


/* struct describing a single operator (SLOT) */
typedef struct
{
    INT32	*DT;		/* detune          :dt_tab[DT] */
    UINT8	KSR;		/* key scale rate  :3-KSR */
    UINT32	ar;		/* attack rate  */
    UINT32	d1r;		/* decay rate   */
    UINT32	d2r;		/* sustain rate */
    UINT32	rr;		/* release rate */
    UINT8	ksr;		/* key scale rate  :kcode>>(3-KSR) */
    UINT32	mul;		/* multiple        :ML_TABLE[ML] */

    /* Phase Generator */
    UINT32	phase;		/* phase counter */
    UINT32	Incr;		/* phase step */

    /* Envelope Generator */
    UINT8	state;		/* phase type */
    UINT32	tl;		/* total level: TL << 3 */
    INT32	volume;		/* envelope counter */
    UINT32	sl;		/* sustain level:sl_table[SL] */
    UINT32	vol_out;	/* current output from EG circuit (without AM from LFO) */

    UINT8	eg_sh_ar;	/*  (attack state) */
    UINT8	eg_sel_ar;	/*  (attack state) */
    UINT8	eg_sh_d1r;	/*  (decay state) */
    UINT8	eg_sel_d1r;	/*  (decay state) */
    UINT8	eg_sh_d2r;	/*  (sustain state) */
    UINT8	eg_sel_d2r;	/*  (sustain state) */
    UINT8	eg_sh_rr;	/*  (release state) */
    UINT8	eg_sel_rr;	/*  (release state) */

    UINT8	ssg;		/* SSG-EG waveform */
    UINT8	ssgn;		/* SSG-EG negated output */

    UINT32	key;		/* 0=last key was KEY OFF, 1=KEY ON */

} FM_SLOT;

typedef struct
{
    FM_SLOT	SLOT[4];	/* four SLOTs (operators) */

    UINT8	ALGO;		/* algorithm */
    UINT8	FB;		/* feedback shift */
    INT32	op1_out[2];	/* op1 output for feedback */

    INT32	*connect1;	/* SLOT1 output pointer */
    INT32	*connect2;	/* SLOT2 output pointer */
    INT32	*connect3;	/* SLOT3 output pointer */
    INT32	*connect4;	/* SLOT4 output pointer */

    INT32	*mem_connect;/* where to put the delayed sample (MEM) */
    INT32	mem_value;	/* delayed sample (MEM) value */

    INT32	pms;		/* channel PMS */
    UINT8	ams;		/* channel AMS */

    UINT32	fc;		/* fnum,blk:adjusted to sample rate */
    UINT8	kcode;		/* key code:                        */
    UINT32	block_fnum;	/* current blk/fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_CH;

typedef struct
{
    void *	param;		/* this chip parameter  */
    int	clock;		/* master clock  (Hz)   */
    int	SSGclock;	/* clock for SSG (Hz)   */
    int	rate;		/* sampling rate (Hz)   */
    double	freqbase;	/* frequency base       */
    double	TimerBase;	/* Timer base time      */
#if FM_BUSY_FLAG_SUPPORT
    double	BusyExpire;	/* ExpireTime of Busy clear */
#endif
    UINT8	address;	/* address register     */
    UINT8	irq;		/* interrupt level      */
    UINT8	irqmask;	/* irq mask             */
    UINT8	status;		/* status flag          */
    UINT32	mode;		/* mode  CSM / 3SLOT    */
    UINT8	prescaler_sel;/* prescaler selector */
    UINT8	fn_h;		/* freq latch           */
    int	TA;		/* timer a              */
    int	TAC;		/* timer a counter      */
    UINT8	TB;		/* timer b              */
    int	TBC;		/* timer b counter      */
    /* local time tables */
    INT32	dt_tab[8][32];/* DeTune table       */
    /* Extention Timer and IRQ handler */
//	FM_TIMERHANDLER	Timer_Handler;
//	FM_IRQHANDLER	IRQ_Handler;
//	const struct ssg_callbacks *SSG;
} FM_ST;

/***********************************************************/
/* OPN unit                                                */
/***********************************************************/

/* OPN 3slot struct */
typedef struct
{
    UINT32  fc[3];			/* fnum3,blk3: calculated */
    UINT8	fn_h;			/* freq3 latch */
    UINT8	kcode[3];		/* key code */
    UINT32	block_fnum[3];	/* current fnum value for this slot (can be different betweeen slots of one channel in 3slot mode) */
} FM_3SLOT;

/* OPN/A/B common state */
typedef struct
{
    UINT8	type;			/* chip type */
    FM_ST	ST;			/* general state */
    FM_3SLOT SL3;			/* 3 slot mode state */
    FM_CH	*P_CH;			/* pointer of CH */

    UINT32	eg_cnt;			/* global envelope generator counter */
    UINT32	eg_timer;		/* global envelope generator counter works at frequency = chipclock/64/3 */
    UINT32	eg_timer_add;	/* step of eg_timer */
    UINT32	eg_timer_overflow;/* envelope generator timer overlfows every 3 samples (on real chip) */


    /* there are 2048 FNUMs that can be generated using FNUM/BLK registers
        but LFO works with one more bit of a precision so we really need 4096 elements */

    UINT32	fn_table[2048];	/* fnumber->increment counter */

} FM_OPN;

/* here's the virtual YM2203(OPN) */
typedef struct
{
    UINT8 REGS[256];		/* registers         */
    FM_OPN OPN;				/* OPN state         */
    FM_CH CH[3];			/* channel state     */
} YM2203;

#define FM_GET_TIME_NOW() 0
//timer_get_time()

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* -------------------- YM2203(OPN) Interface -------------------- */

/*
** Initialize YM2203 emulator(s).
**
** 'num'           is the number of virtual YM2203's to allocate
** 'baseclock'
** 'rate'          is sampling rate
** 'TimerHandler'  timer callback handler when timer start and clear
** 'IRQHandler'    IRQ callback handler when changed IRQ level
** return      0 = success
*/
void * YM2203Init(void *param, int index, int baseclock, int rate
//			,FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler, const struct ssg_callbacks *ssg
               );

/*
** shutdown the YM2203 emulators
*/
void YM2203Shutdown(void *chip);

/*
** reset all chip registers for YM2203 number 'num'
*/
void YM2203ResetChip(void *chip);

/*
** update one of chip
*/
void YM2203UpdateOne(void *chip, FMSAMPLE *buffer, int length);

/*
** Write
** return : InterruptLevel
*/
int YM2203Write(void *chip,int a,unsigned char v);

/*
** Read
** return : InterruptLevel
*/
unsigned char YM2203Read(void *chip,int a);

/*
**  Timer OverFlow
*/
int YM2203TimerOver(void *chip, int c);

/*
**  State Save
*/
void YM2203Postload(void *chip);

void YM2203_save_state(void *chip, int index);
void OPNPrescaler_w(FM_OPN *OPN , int addr, int pre_divider);
