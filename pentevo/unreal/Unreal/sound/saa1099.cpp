#include "std.h"
#include "emul.h"
#include "vars.h"
#include "saa1099.h"

/***************************************************************************

    Philips SAA1099 Sound driver

    By Juergen Buchmueller and Manuel Abadia

    SAA1099 register layout:
    ========================

    offs | 7654 3210 | description
    -----+-----------+---------------------------
    0x00 | ---- xxxx | Amplitude channel 0 (left)
    0x00 | xxxx ---- | Amplitude channel 0 (right)
    0x01 | ---- xxxx | Amplitude channel 1 (left)
    0x01 | xxxx ---- | Amplitude channel 1 (right)
    0x02 | ---- xxxx | Amplitude channel 2 (left)
    0x02 | xxxx ---- | Amplitude channel 2 (right)
    0x03 | ---- xxxx | Amplitude channel 3 (left)
    0x03 | xxxx ---- | Amplitude channel 3 (right)
    0x04 | ---- xxxx | Amplitude channel 4 (left)
    0x04 | xxxx ---- | Amplitude channel 4 (right)
    0x05 | ---- xxxx | Amplitude channel 5 (left)
    0x05 | xxxx ---- | Amplitude channel 5 (right)
         |           |
    0x08 | xxxx xxxx | Frequency channel 0
    0x09 | xxxx xxxx | Frequency channel 1
    0x0a | xxxx xxxx | Frequency channel 2
    0x0b | xxxx xxxx | Frequency channel 3
    0x0c | xxxx xxxx | Frequency channel 4
    0x0d | xxxx xxxx | Frequency channel 5
         |           |
    0x10 | ---- -xxx | Channel 0 octave select
    0x10 | -xxx ---- | Channel 1 octave select
    0x11 | ---- -xxx | Channel 2 octave select
    0x11 | -xxx ---- | Channel 3 octave select
    0x12 | ---- -xxx | Channel 4 octave select
    0x12 | -xxx ---- | Channel 5 octave select
         |           |
    0x14 | ---- ---x | Channel 0 frequency enable (0 = off, 1 = on)
    0x14 | ---- --x- | Channel 1 frequency enable (0 = off, 1 = on)
    0x14 | ---- -x-- | Channel 2 frequency enable (0 = off, 1 = on)
    0x14 | ---- x--- | Channel 3 frequency enable (0 = off, 1 = on)
    0x14 | ---x ---- | Channel 4 frequency enable (0 = off, 1 = on)
    0x14 | --x- ---- | Channel 5 frequency enable (0 = off, 1 = on)
         |           |
    0x15 | ---- ---x | Channel 0 noise enable (0 = off, 1 = on)
    0x15 | ---- --x- | Channel 1 noise enable (0 = off, 1 = on)
    0x15 | ---- -x-- | Channel 2 noise enable (0 = off, 1 = on)
    0x15 | ---- x--- | Channel 3 noise enable (0 = off, 1 = on)
    0x15 | ---x ---- | Channel 4 noise enable (0 = off, 1 = on)
    0x15 | --x- ---- | Channel 5 noise enable (0 = off, 1 = on)
         |           |
    0x16 | ---- --xx | Noise generator parameters 0
    0x16 | --xx ---- | Noise generator parameters 1
         |           |
    0x18 | --xx xxxx | Envelope generator 0 parameters
    0x18 | x--- ---- | Envelope generator 0 control enable (0 = off, 1 = on)
    0x19 | --xx xxxx | Envelope generator 1 parameters
    0x19 | x--- ---- | Envelope generator 1 control enable (0 = off, 1 = on)
         |           |
    0x1c | ---- ---x | All channels enable (0 = off, 1 = on)
    0x1c | ---- --x- | Synch & Reset generators

***************************************************************************/

#define LEFT    0x00
#define RIGHT   0x01

static const int amplitude_lookup[16] =
{
     0*32767/16,  1*32767/16,  2*32767/16,  3*32767/16,
     4*32767/16,  5*32767/16,  6*32767/16,  7*32767/16,
     8*32767/16,  9*32767/16, 10*32767/16, 11*32767/16,
    12*32767/16, 13*32767/16, 14*32767/16, 15*32767/16
};

static const u8 envelope[8][64] =
{
    /* zero amplitude */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* maximum amplitude */
    {15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
     15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
     15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
     15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15, },
    /* single decay */
    {15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* repetitive decay */
    {15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
    /* single triangular */
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* repetitive triangular */
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
     15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 },
    /* single attack */
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* repetitive attack */
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 }
};

static void saa1099_envelope(saa1099_state *saa, int ch)
{
    if (saa->env_enable[ch])
    {
        int step, mode, mask;
        /* step from 0..63 and then loop in steps 32..63 */
        step = saa->env_step[ch] =
            ((saa->env_step[ch] + 1) & 0x3f) | (saa->env_step[ch] & 0x20);

        mode = saa->env_mode[ch];

        if (saa->env_upd[ch])
        {
            if (
               ((mode == 1 || mode == 3 || mode == 7) && step && ((step & 0xF) == 0)) // 1, 3, 7
               ||
               ((mode == 5) && step && ((step & 0x1F) == 0)) // 5
               ||
               ((mode == 0 || mode == 2 || mode == 6) && step > 0x0F) // 0, 2, 6
               ||
               ((mode == 4) && step > 0x1F) // 4
              )
            {
                mode = saa->env_mode[ch] = saa->env_mode_buf[ch];
                saa->env_reverse_right[ch] = saa->env_reverse_right_buf[ch];
                saa->env_clock[ch] = saa->env_clock_buf[ch];

                /* reset the envelope */
                saa->env_step[ch] = 1;
                step = 1;
                saa->env_upd[ch] = false;
            }
        }

        mask = 15;
        if (saa->env_bits[ch])
            mask &= ~1;     /* 3 bit resolution, mask LSB */

        saa->channels[ch*3+2].envelope[ LEFT] = envelope[mode][step] & mask;
        if (saa->env_reverse_right[ch] & 0x01)
        {
            saa->channels[ch*3+2].envelope[RIGHT] = (15 - envelope[mode][step]) & mask;
        }
        else
        {
            saa->channels[ch*3+2].envelope[RIGHT] = envelope[mode][step] & mask;
        }
    }
    else
    {
        /* envelope mode off, set all envelope factors to 16 */
        saa->channels[ch*3+0].envelope[ LEFT] =
        saa->channels[ch*3+1].envelope[ LEFT] =
        saa->channels[ch*3+2].envelope[ LEFT] =
        saa->channels[ch*3+0].envelope[RIGHT] =
        saa->channels[ch*3+1].envelope[RIGHT] =
        saa->channels[ch*3+2].envelope[RIGHT] = 16;
    }
}


void TSaa1099::update(unsigned TimeStamp)
{
    saa1099_state *saa = this;
    int ch;

    /* if the channels are disabled we're done */
    if (!saa->all_ch_enable)
    {
        /* init output data */
        SNDRENDER::update(TimeStamp, 0, 0);
        return;
    }

    for (ch = 0; ch < 2; ch++)
    {
        switch (saa->noise_params[ch])
        {
        case 0: saa->noise[ch].freq = 31250.0 * 2; break;
        case 1: saa->noise[ch].freq = 15625.0 * 2; break;
        case 2: saa->noise[ch].freq =  7812.5 * 2; break;
        case 3: saa->noise[ch].freq = saa->channels[ch * 3].freq; break;
        }
    }

    /* fill all data needed */
    int output_l = 0, output_r = 0;

    /* for each channel */
    for (ch = 0; ch < 6; ch++)
    {
        if (saa->channels[ch].freq == 0.0)
            saa->channels[ch].freq = (double)((2*15625) << saa->channels[ch].octave) /
                (511.0 - (double)saa->channels[ch].frequency);

        /* check the actual position in the square wave */
        saa->channels[ch].counter -= saa->channels[ch].freq;
        while (saa->channels[ch].counter < 0)
        {
            /* calculate new frequency now after the half wave is updated */
            saa->channels[ch].freq = (double)((2*15625) << saa->channels[ch].octave) /
                (511.0 - (double)saa->channels[ch].frequency);

            saa->channels[ch].counter += saa->sample_rate;
            saa->channels[ch].level ^= 1;

            /* eventually clock the envelope counters */
            if (ch == 1 && saa->env_clock[0] == 0)
                saa1099_envelope(saa, 0);
            if (ch == 4 && saa->env_clock[1] == 0)
                saa1099_envelope(saa, 1);
        }

        /* if the noise is enabled */
        if (saa->channels[ch].noise_enable)
        {
            /* if the noise level is high (noise 0: chan 0-2, noise 1: chan 3-5) */
            if (saa->noise[ch/3].level & 1)
            {
                /* subtract to avoid overflows, also use only half amplitude */
                output_l -= saa->channels[ch].amplitude[ LEFT] * saa->channels[ch].envelope[ LEFT] / 16 / 2;
                output_r -= saa->channels[ch].amplitude[RIGHT] * saa->channels[ch].envelope[RIGHT] / 16 / 2;
            }
        }

        /* if the square wave is enabled */
        if (saa->channels[ch].freq_enable)
        {
            /* if the channel level is high */
            if (saa->channels[ch].level & 1)
            {
                output_l += saa->channels[ch].amplitude[ LEFT] * saa->channels[ch].envelope[ LEFT] / 16;
                output_r += saa->channels[ch].amplitude[RIGHT] * saa->channels[ch].envelope[RIGHT] / 16;
            }
        }
        else if ((ch == 2 || ch == 5) && saa->env_enable[ch/3])
        {
            output_l += saa->channels[ch].amplitude[ LEFT] * saa->channels[ch].envelope[ LEFT] / 16;
            output_r += saa->channels[ch].amplitude[RIGHT] * saa->channels[ch].envelope[RIGHT] / 16;
        }
    }

    for (ch = 0; ch < 2; ch++)
    {
        /* check the actual position in noise generator */
        saa->noise[ch].counter -= saa->noise[ch].freq;
        while (saa->noise[ch].counter < 0)
        {
            saa->noise[ch].counter += saa->sample_rate;
            if ( ((saa->noise[ch].level & 0x4000) == 0) == ((saa->noise[ch].level & 0x0040) == 0) )
                saa->noise[ch].level = (saa->noise[ch].level << 1) | 1;
            else
                saa->noise[ch].level <<= 1;
        }
    }

    /* write sound data to the buffer */
    unsigned mix_l = (unsigned)( output_l * (conf.sound.saa1099_vol / 8192.0) / 6 );
    unsigned mix_r = (unsigned)( output_r * (conf.sound.saa1099_vol / 8192.0) / 6 );
    if ((mix_l ^ SNDRENDER::mix_l) | (mix_r ^ SNDRENDER::mix_r)) // similar check inside update()
        SNDRENDER::update(TimeStamp, mix_l, mix_r);
}

static const u32 SAM_SAA1099_CLK = 8000000; // Hz

TSaa1099::TSaa1099()
{
    saa1099_state *saa = this;

    memset(saa, 0, sizeof(saa1099_state));

    /* copy global parameters */
    saa1099_state::sample_rate = SAM_SAA1099_CLK / 256; // 31250 Hz

    for (unsigned ch = 0; ch < 6; ch++)
    {
        saa->channels[ch].envelope[ LEFT] =
        saa->channels[ch].envelope[RIGHT] = 16;
    }
}

void TSaa1099::WrCtl(u8 data)
{
    saa1099_state *saa = this;

    saa->selected_reg = data & 0x1f;
    if (saa->selected_reg == 0x18 || saa->selected_reg == 0x19)
    {
        /* clock the envelope channels */
        if (saa->env_clock[0])
            saa1099_envelope(saa,0);
        if (saa->env_clock[1])
            saa1099_envelope(saa,1);
    }
}


void TSaa1099::WrData(unsigned TimeStamp, u8 data)
{
    saa1099_state *saa = this;
    int reg = saa->selected_reg;
    int ch;

    /* first update the stream to this point in time */
   if (TimeStamp)
       flush((TimeStamp * chip_clock_rate) / system_clock_rate);

    switch (reg)
    {
    /* channel i amplitude */
    case 0x00:  case 0x01:  case 0x02:  case 0x03:  case 0x04:  case 0x05:
        ch = reg & 7;
        saa->channels[ch].amp[LEFT] = data & 0x0f;
        saa->channels[ch].amp[RIGHT] = (data >> 4) & 0x0f;

        saa->channels[ch].amplitude[LEFT] = amplitude_lookup[data & 0x0f];
        saa->channels[ch].amplitude[RIGHT] = amplitude_lookup[(data >> 4) & 0x0f];

        if (ch == 0)
        {
            if (saa->env_enable[0])
            {
                saa->channels[2].amplitude[LEFT] = amplitude_lookup[saa->channels[2].amp[LEFT] & ~1];
                saa->channels[2].amplitude[RIGHT] = amplitude_lookup[saa->channels[2].amp[RIGHT] & ~1];
            }
            else
            {
                saa->channels[2].amplitude[LEFT] = amplitude_lookup[saa->channels[2].amp[LEFT]];
                saa->channels[2].amplitude[RIGHT] = amplitude_lookup[saa->channels[2].amp[RIGHT]];
            }
        }
        else if (ch == 1)
        {
            if (saa->env_enable[1])
            {
                saa->channels[5].amplitude[LEFT] = amplitude_lookup[saa->channels[5].amp[LEFT] & ~1];
                saa->channels[5].amplitude[RIGHT] = amplitude_lookup[saa->channels[5].amp[RIGHT] & ~1];
            }
            else
            {
                saa->channels[5].amplitude[LEFT] = amplitude_lookup[saa->channels[5].amp[LEFT]];
                saa->channels[5].amplitude[RIGHT] = amplitude_lookup[saa->channels[5].amp[RIGHT]];
            }
        }
        break;
    /* channel i frequency */
    case 0x08:  case 0x09:  case 0x0a:  case 0x0b:  case 0x0c:  case 0x0d:
        ch = reg & 7;
        saa->channels[ch].frequency = data & 0xff;
        break;
    /* channel i octave */
    case 0x10:  case 0x11:  case 0x12:
        ch = (reg - 0x10) << 1;
        saa->channels[ch + 0].octave = data & 0x07;
        saa->channels[ch + 1].octave = (data >> 4) & 0x07;
        break;
    /* channel i frequency enable */
    case 0x14:
        saa->channels[0].freq_enable = data & 0x01;
        saa->channels[1].freq_enable = data & 0x02;
        saa->channels[2].freq_enable = data & 0x04;
        saa->channels[3].freq_enable = data & 0x08;
        saa->channels[4].freq_enable = data & 0x10;
        saa->channels[5].freq_enable = data & 0x20;
        break;
    /* channel i noise enable */
    case 0x15:
        saa->channels[0].noise_enable = data & 0x01;
        saa->channels[1].noise_enable = data & 0x02;
        saa->channels[2].noise_enable = data & 0x04;
        saa->channels[3].noise_enable = data & 0x08;
        saa->channels[4].noise_enable = data & 0x10;
        saa->channels[5].noise_enable = data & 0x20;
        break;
    /* noise generators parameters */
    case 0x16:
        saa->noise_params[0] = data & 0x03;
        saa->noise_params[1] = (data >> 4) & 0x03;
        break;
    /* envelope generators parameters */
    case 0x18:  case 0x19:
        ch = reg - 0x18;

        // direct
        saa->env_bits[ch] = data & 0x10;
        saa->env_enable[ch] = data & 0x80;
        if (!(data & 0x80))
            saa->env_step[ch] = 0; // reset envelope

        // buffered
        saa->env_reverse_right_buf[ch] = data & 0x01;
        saa->env_mode_buf[ch] = (data >> 1) & 0x07;
        saa->env_clock_buf[ch] = data & 0x20;
        saa->env_upd[ch] = true;


        if (ch == 0)
        {
            if (saa->env_enable[0])
            {
                saa->channels[2].amplitude[LEFT] = amplitude_lookup[saa->channels[2].amp[LEFT] & ~1];
                saa->channels[2].amplitude[RIGHT] = amplitude_lookup[saa->channels[2].amp[RIGHT] & ~1];
            }
            else
            {
                saa->channels[2].amplitude[LEFT] = amplitude_lookup[saa->channels[2].amp[LEFT]];
                saa->channels[2].amplitude[RIGHT] = amplitude_lookup[saa->channels[2].amp[RIGHT]];
            }
        }

        else if (ch == 1)
        {
            if (saa->env_enable[1])
            {
                saa->channels[5].amplitude[LEFT] = amplitude_lookup[saa->channels[5].amp[LEFT] & ~1];
                saa->channels[5].amplitude[RIGHT] = amplitude_lookup[saa->channels[5].amp[RIGHT] & ~1];
            }
            else
            {
                saa->channels[5].amplitude[LEFT] = amplitude_lookup[saa->channels[5].amp[LEFT]];
                saa->channels[5].amplitude[RIGHT] = amplitude_lookup[saa->channels[5].amp[RIGHT]];
            }
        }
        break;
    /* channels enable & reset generators */
    case 0x1c:
        saa->all_ch_enable = data & 0x01;
        saa->sync_state = data & 0x02;
        if (data & 0x02)
        {
            int i;

            /* Synch & Reset generators */
            for (i = 0; i < 6; i++)
            {
                saa->channels[i].level = 0;
                saa->channels[i].counter = 0.0;
            }
        }
        break;
    default:    /* Error! */
        ;
    }
}

void TSaa1099::start_frame(bufptr_t dst)
{
    SNDRENDER::start_frame(dst);
}

unsigned TSaa1099::end_frame(unsigned clk_ticks)
{
    u64 end_chip_tick = ((passed_clk_ticks + clk_ticks) * chip_clock_rate) / system_clock_rate;

    flush((unsigned)(end_chip_tick - passed_chip_ticks));

    unsigned Val = SNDRENDER::end_frame(t);

    passed_clk_ticks += clk_ticks;
    passed_chip_ticks += t;
    t = 0;

    return Val;
}

void TSaa1099::flush(unsigned chiptick)
{
    while (t < chiptick)
    {
        t++;
        update(t);
    }
}

void TSaa1099::set_timings(unsigned system_clock_rate, unsigned chip_clock_rate, unsigned sample_rate)
{
   chip_clock_rate /= 256;

   TSaa1099::system_clock_rate = system_clock_rate;
   TSaa1099::chip_clock_rate = chip_clock_rate;
   TSaa1099::saa1099_state::sample_rate = chip_clock_rate;

   SNDRENDER::set_timings(chip_clock_rate, sample_rate);
   passed_chip_ticks = passed_clk_ticks = 0;
   t = 0;
}

void TSaa1099::reset(unsigned TimeStamp)
{
    WrCtl(0x1C);
    WrData(TimeStamp, 2);
}

TSaa1099 Saa1099;
