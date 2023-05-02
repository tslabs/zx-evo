#include "std.h"
#include "emul.h"
#include "vars.h"
#include "hard/gs/gs.h"
#include "tape.h"
#include "config.h"
#include "saa1099.h"
#include "sndcounter.h"
#include "hard/gs/gsz80.h"
#include "sound/dev_moonsound.h"

extern SNDRENDER sound;
extern SNDCHIP ay[2];
extern SNDCOUNTER sndcounter;

int spkr_dig = 0, mic_dig = 0, covFB_vol = 0, covDD_vol = 0, sd_l = 0, sd_r = 0;
int covProfiL = 0, covProfiR = 0;

void flush_dig_snd()
{
	//   __debugbreak();
	if (temp.sndblock)
		return;
	const unsigned mono = (spkr_dig + mic_dig + covFB_vol + covDD_vol);
	//   printf("mono=%u\n", mono);
	//[vv]   
	sound.update(cpu.t - temp.cpu_t_at_frame_start, mono + sd_l + covProfiL, mono + sd_r + covProfiR);
}

void init_snd_frame()
{
	temp.cpu_t_at_frame_start = cpu.t;
	//[vv]   
	sound.start_frame();
	//   comp.tape.sound.start_frame(); //Alone Coder
	comp.tape_sound.start_frame(); //Alone Coder

	if (conf.sound.ay_scheme != ay_scheme::none)
	{
		ay[0].start_frame();
		if (conf.sound.ay_scheme > ay_scheme::single)
			ay[1].start_frame();
	}

	Saa1099.start_frame();
	zxmmoonsound.start_frame();

	init_gs_frame();
}

float y_1[2] = { 0.0 };
i16 x_1[2] = { 0 };

void flush_snd_frame()
{
	tape_bit();
	flush_gs_frame();

	if (temp.sndblock)
		return;

	const unsigned endframe = cpu.t - temp.cpu_t_at_frame_start;

	if (conf.sound.ay_scheme != ay_scheme::none)
	{ // sound chip present

		ay[0].end_frame(endframe);
		// if (conf.sound.ay_samples) mix_dig(ay[0]);

		if (conf.sound.ay_scheme > ay_scheme::single)
		{

			ay[1].end_frame(endframe);
			// if (conf.sound.ay_samples) mix_dig(ay[1]);

			if (conf.sound.ay_scheme == ay_scheme::pseudo)
			{
				const u8 last = ay[0].get_r13_reloaded() ? 13 : 12;
				for (u8 r = 0; r <= last; r++) {
					ay[1].select(r);
					ay[1].write(0, ay[0].get_reg(r));
				}
			}
		}

		if (savesndtype == 2)
		{
			if (!vtxbuf)
			{
				vtxbuf = (u8*)malloc(32768);
				vtxbufsize = 32768;
				vtxbuffilled = 0;
			}

			if (vtxbuffilled + 14 >= vtxbufsize)
			{
				vtxbufsize += 32768;
				vtxbuf = (u8*)realloc(vtxbuf, vtxbufsize);
			}

			for (u8 r = 0; r < 14; r++)
				vtxbuf[vtxbuffilled + r] = ay[0].get_reg(r);

			if (!ay[0].get_r13_reloaded())
				vtxbuf[vtxbuffilled + 13] = 0xFF;

			vtxbuffilled += 14;
		}
	}
	Saa1099.end_frame(endframe);
	zxmmoonsound.end_frame(endframe);

	sound.end_frame(endframe);
	// if (comp.tape.play_pointer) // play tape pulses
 //      comp.tape.sound.end_frame(endframe); //Alone Coder
	comp.tape_sound.end_frame(endframe); //Alone Coder
	// else comp.tape.sound.end_empty_frame(endframe);

	unsigned bufplay, n_samples;
	sndcounter.begin();

	sndcounter.count(sound);
	//   sndcounter.count(comp.tape.sound); //Alone Coder
	sndcounter.count(comp.tape_sound); //Alone Coder
	if (conf.sound.ay_scheme != ay_scheme::none)
	{
		sndcounter.count(ay[0]);
		if (conf.sound.ay_scheme > ay_scheme::single)
			sndcounter.count(ay[1]);
	}

	sndcounter.count(Saa1099);
	sndcounter.count(zxmmoonsound);


	if (conf.gs_type == 1)
		sndcounter.count(z80gs::sound);

	sndcounter.end(bufplay, n_samples);

	for (unsigned k = 0; k < n_samples; k++, bufplay++)
	{
		const u32 v = sndbuf[bufplay & (SNDBUFSIZE - 1)];
		u32 Y;
		if (conf.RejectDC) // DC rejection filter
		{
			i16 x[2];
			float y[2];
			x[0] = i16(v & 0xFFFF);
			x[1] = i16(v >> 16U);
			y[0] = 0.995f * (x[0] - x_1[0]) + 0.99f * y_1[0];
			y[1] = 0.995f * (x[1] - x_1[1]) + 0.99f * y_1[1];
			x_1[0] = x[0];
			x_1[1] = x[1];
			y_1[0] = y[0];
			y_1[1] = y[1];

			Y = ((i16(y[1]) & 0xFFFF) << 16) | (i16(y[0]) & 0xFFFF);
		}
		else
		{
			Y = v;
		}

		sndplaybuf[k] = Y;
		sndbuf[bufplay & (SNDBUFSIZE - 1)] = 0;
	}

	spbsize = n_samples * 4;

	return;

}

void restart_sound()
{
	//   printf("%s\n", __FUNCTION__);

	const unsigned cpufq = conf.intfq * conf.frame;
	sound.set_timings(cpufq, conf.sound.fq);
	//   comp.tape.sound.set_timings(cpufq, conf.sound.fq); //Alone Coder
	comp.tape_sound.set_timings(cpufq, conf.sound.fq); //Alone Coder
	if (conf.sound.ay_scheme != ay_scheme::none)
	{
		ay[0].set_timings(cpufq, conf.sound.ayfq, conf.sound.fq);
		if (conf.sound.ay_scheme > ay_scheme::single) ay[1].set_timings(cpufq, conf.sound.ayfq, conf.sound.fq);
	}

	Saa1099.set_timings(cpufq, conf.sound.saa1099fq, conf.sound.fq);
	zxmmoonsound.set_timings(cpufq, 33868800, conf.sound.fq);

	// comp.tape.sound.clear();
#ifdef MOD_GS
	reset_gs_sound();
#endif

	sndcounter.reset();

	memset(sndbuf, 0, sizeof sndbuf);
}

void apply_sound()
{
	if (conf.sound.ay_scheme < ay_scheme::quadro) comp.active_ay = 0;

	load_ay_stereo();
	load_ay_vols();

	ay[0].set_chip((SNDCHIP::CHIP_TYPE)conf.sound.ay_chip);
	ay[1].set_chip((SNDCHIP::CHIP_TYPE)conf.sound.ay_chip);

	const SNDCHIP_VOLTAB* voltab = (SNDCHIP_VOLTAB*)&conf.sound.ay_voltab;
	const SNDCHIP_PANTAB* stereo = (SNDCHIP_PANTAB*)&conf.sound.ay_stereo_tab;
	ay[0].set_volumes(conf.sound.ay_vol, voltab, stereo);

	SNDCHIP_PANTAB reversed;
	if (conf.sound.ay_scheme == ay_scheme::pseudo) {
		for (int i = 0; i < 6; i++)
			reversed.raw[i] = stereo->raw[i ^ 1]; // swap left/right
		stereo = &reversed;
	}
	ay[1].set_volumes(conf.sound.ay_vol, voltab, stereo);


#ifdef MOD_GS
	apply_gs();
#endif

	restart_sound();
}

