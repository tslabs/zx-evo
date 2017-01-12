#include "std.h"
#include "../emul.h"
#include "../vars.h"
#include "../util.h"
#include "dev_moonsound.h"

#include "ymf262.h"
#include "ymf278.h"

#define MASTER_CLOCK 33868800

struct ZXMMoonSound_priv
{
friend class ZXMMoonSound;

public:
	ZXMMoonSound_priv();
	~ZXMMoonSound_priv();

private:
	YMF262 *ymf262;
	int opl3latch;

	YMF278 *ymf278;
	int opl4latch;
};

ZXMMoonSound_priv::ZXMMoonSound_priv()
{
	EmuTime t = cpu.t;

	ymf262 = new YMF262( 0, t, this );
	ymf262->setSampleRate(44100, 1);
	ymf262->setVolume(32767 * 2 / 10);

	ymf278 = new YMF278( 0, 4096, 2048*1024, t );
	ymf278->setSampleRate(44100, 1);
	ymf278->setVolume(32767 * 2 / 10);
}

ZXMMoonSound_priv::~ZXMMoonSound_priv()
{
	delete ymf262;
	delete ymf278;
}

/*  */
ZXMMoonSound::ZXMMoonSound() :
	system_clock_rate( 0 )
{
	d = new ZXMMoonSound_priv();
	reset();
}

int ZXMMoonSound::load_rom(char *path)
{
	FILE *fp = fopen( path, "rb" );
	if ( !fp )
	{
		errmsg( "failed to load MoonSound ROM (%s): %s\n", path, strerror(errno) );
		return -1;
	}

	fread( d->ymf278->getRom(), 1, d->ymf278->getRomSize(), fp );

	fclose( fp );

	return 0;
}

void ZXMMoonSound::reset()
{
	EmuTime systemTime = cpu.t;
	d->ymf262->reset( systemTime );
	d->ymf278->reset( systemTime );
}

bool ZXMMoonSound::write( u8 port, u8 val )
{
	//printf("ZXM-MoonSound write(%.2x, %.2x)\n", port, val);

	EmuTime systemTime = cpu.t;

	if ( (port & 0xFE) == 0x7E )
	{
		switch (port & 0x01) {
		case 0: // select register
			d->opl4latch = val;
			break;
		case 1:
  			d->ymf278->writeRegOPL4(d->opl4latch, val, systemTime);
			break;
		}

		return true;
	}
	else if ( (port & 0xFC) == 0xC4 )
	{
		switch (port & 0x03) {
		case 0:
			d->opl3latch = val;
			break;
		case 2: // select register bank 1
			d->opl3latch = val | 0x100;
			break;
		case 1:
		case 3: // write fm register
			d->ymf262->writeReg(d->opl3latch, val, systemTime);
			break;
		}

		return true;
	}

	return false;
}

bool ZXMMoonSound::read( u8 port, u8 &val )
{
	//printf("ZXM-MoonSound read(%.2x)\n", port);

	EmuTime systemTime = cpu.t;

	if ( (port & 0xFE) == 0x7E )
	{
		switch (port & 0x01) {
		case 1: // read wave register
			val = d->ymf278->readRegOPL4(d->opl4latch, systemTime);
			break;
		}

		return true;
	}
	else if( (port & 0xFC) == 0xC4 )
	{
		switch (port & 0x03) {
		case 0: // read status
		case 2:
			val = d->ymf262->readStatus() | d->ymf278->readStatus(systemTime);
			break;
		case 1:
		case 3: // read fm register
			val = d->ymf262->readReg(d->opl3latch);
			break;
		}

		return true;
	}

	return false;
}


void ZXMMoonSound::set_timings(unsigned system_clock_rate, unsigned chip_clock_rate, unsigned sample_rate)
{
	d->ymf262->setSampleRate( sample_rate, 1 );
	d->ymf262->setVolume((u16)(1 * (conf.sound.moonsound_vol / 8192.0))); // doesn't work
	d->ymf278->setSampleRate( sample_rate, 1 );
	d->ymf278->setVolume((u16)(2000 * (conf.sound.moonsound_vol / 8192.0)));
	
	chip_clock_rate = sample_rate;

	ZXMMoonSound::system_clock_rate = system_clock_rate;
	ZXMMoonSound::chip_clock_rate = chip_clock_rate;

	SNDRENDER::set_timings(chip_clock_rate, sample_rate);
	passed_chip_ticks = passed_clk_ticks = 0;
	t = 0;
}

void ZXMMoonSound::start_frame(bufptr_t dst)
{
    SNDRENDER::start_frame(dst);
}

unsigned ZXMMoonSound::end_frame(unsigned clk_ticks)
{
	u64 end_chip_tick = ((passed_clk_ticks + clk_ticks) * chip_clock_rate) / system_clock_rate;

	flush((unsigned)(end_chip_tick - passed_chip_ticks));

    unsigned Val = SNDRENDER::end_frame(t);
	passed_clk_ticks += clk_ticks;
    passed_chip_ticks += t;
    t = 0;

    return Val;
}

void ZXMMoonSound::flush(unsigned chiptick)
{
	while (t < chiptick)
    {
		int buffer[2] = { 0, 0 };
		int *buf;

		t++;

		buf = d->ymf262->updateBuffer(1);
		if ( buf )
		{
			buffer[0] += buf[0] / 10;
			buffer[1] += buf[1] / 10;
		}

		buf = d->ymf278->updateBuffer(1);
		if ( buf )
		{
			buffer[0] += buf[0];
			buffer[1] += buf[1];
		}
		
		SNDRENDER::update( t, buffer[0], buffer[1] );
	}
}

ZXMMoonSound zxmmoonsound;