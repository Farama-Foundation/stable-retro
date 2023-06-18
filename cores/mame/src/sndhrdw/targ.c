/*************************************************************************

	Exidy 6502 hardware

*************************************************************************/

/* Sound channel usage
   0 = CPU music,  Shoot
   1 = Crash
   2 = Spectar sound
   3 = Tone generator
*/

#include "driver.h"
#include "exidy.h"

static int tone_channel;

unsigned char targ_spec_flag;
static unsigned char targ_sh_ctrl0=0;
static unsigned char targ_sh_ctrl1=0;
static unsigned char tone_active;

#define MAXFREQ_A_TARG 125000
#define MAXFREQ_A_SPECTAR 525000

static int sound_a_freq;
static unsigned char tone_pointer;
static unsigned char tone_offset;

static unsigned char tone_prom[32] =
{
    0xE5,0xE5,0xED,0xED,0xE5,0xE5,0xED,0xED,0xE7,0xE7,0xEF,0xEF,0xE7,0xE7,0xEF,0xEF,
    0xC1,0xE1,0xC9,0xE9,0xC5,0xE5,0xCD,0xED,0xC3,0xE3,0xCB,0xEB,0xC7,0xE7,0xCF,0xEF
};

/* waveforms for the audio hardware */
static unsigned char waveform1[32] =
{
	/* sine-wave */
	0x0F, 0x0F, 0x0F, 0x06, 0x06, 0x09, 0x09, 0x06, 0x06, 0x09, 0x06, 0x0D, 0x0F, 0x0F, 0x0D, 0x00,
	0xE6, 0xDE, 0xE1, 0xE6, 0xEC, 0xE6, 0xE7, 0xE7, 0xE7, 0xEC, 0xEC, 0xEC, 0xE7, 0xE1, 0xE1, 0xE7,
};

/* some macros to make detecting bit changes easier */
#define RISING_EDGE(bit)  ((data & bit) && !(targ_sh_ctrl0 & bit))
#define FALLING_EDGE(bit) (!(data & bit) && (targ_sh_ctrl0 & bit))


void targ_tone_generator(int data)
{
	int maxfreq;


	if (targ_spec_flag) maxfreq = MAXFREQ_A_TARG;
	else maxfreq = MAXFREQ_A_SPECTAR;

    sound_a_freq = data;
    if (sound_a_freq == 0xFF || sound_a_freq == 0x00)
	{
		mixer_set_volume(tone_channel,0);
    }
    else
	{
		mixer_set_sample_frequency(tone_channel,maxfreq/(0xFF-sound_a_freq));
		mixer_set_volume(tone_channel,tone_active*100);
	}
}

int targ_sh_start(const struct MachineSound *msound)
{
	tone_channel = mixer_allocate_channel(50);

	tone_pointer=0;
	tone_offset=0;
	tone_active=0;
	sound_a_freq = 0x00;
	mixer_set_volume(tone_channel,0);
	mixer_play_sample(tone_channel,(signed char*)waveform1,32,1000,1);
	return 0;
}

void targ_sh_stop(void)
{
    mixer_stop_sample(tone_channel);
}

WRITE_HANDLER( targ_sh_w )
{
	int maxfreq;


	if (targ_spec_flag) maxfreq = MAXFREQ_A_TARG;
	else maxfreq = MAXFREQ_A_SPECTAR;

    if (offset) {
        if (targ_spec_flag) {
            if (data & 0x02)
                tone_offset=16;
            else
                tone_offset=0;

            if ((data & 0x01) && !(targ_sh_ctrl1 & 0x01)) {
                tone_pointer++;
                if (tone_pointer > 15) tone_pointer = 0;
                targ_tone_generator(tone_prom[tone_pointer+tone_offset]);
            }
       }
       else {
            targ_tone_generator(data);
       }
       targ_sh_ctrl1=data;
    }
    else
    {
        /* cpu music */
        if ((data & 0x01) != (targ_sh_ctrl0 & 0x01)) {
            DAC_data_w(0,(data & 0x01) * 0xFF);
        }
        /* Shoot */
        if FALLING_EDGE(0x02) {
            if (!sample_playing(0))  sample_start(0,1,0);
        }
        if RISING_EDGE(0x02) {
            sample_stop(0);
        }

        /* Crash */
        if RISING_EDGE(0x20) {
            if (data & 0x40) {
                sample_start(1,2,0); }
            else {
                sample_start(1,0,0); }
        }

        /* Sspec */
        if (data & 0x10) {
            sample_stop(2);
        }
        else {
            if ((data & 0x08) != (targ_sh_ctrl0 & 0x08)) {
            if (data & 0x08) {
                sample_start(2,3,1); }
            else {
                sample_start(2,4,1); }
            }
        }

        /* Game (tone generator enable) */
        if FALLING_EDGE(0x80) {
           tone_pointer=0;
           tone_active=0;
           if (sound_a_freq == 0xFF || sound_a_freq == 0x00)
		   {
				mixer_set_volume(tone_channel,0);
           }
           else
		   {
             mixer_set_sample_frequency(tone_channel,maxfreq/(0xFF-sound_a_freq));
             mixer_set_volume(tone_channel,0);
		   }
        }
        if RISING_EDGE(0x80) {
            tone_active=1;
        }
        targ_sh_ctrl0 = data;
    }
}
