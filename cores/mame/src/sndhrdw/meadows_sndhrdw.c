/***************************************************************************
	meadows.c
	Sound handler
	Dead Eye, Gypsy Juggler

	J. Buchmueller, June '98
****************************************************************************/

#include "driver.h"
#include "cpu/s2650/s2650.h"
#include "meadows.h"


unsigned char meadows_0c00 = 0;
unsigned char meadows_0c01 = 0;
unsigned char meadows_0c02 = 0;
unsigned char meadows_0c03 = 0;
static unsigned char meadows_dac  = 0;
static int dac_enable;

#define BASE_CLOCK		5000000
#define BASE_CTR1       (BASE_CLOCK / 256)
#define BASE_CTR2		(BASE_CLOCK / 32)

#define DIV2OR4_CTR2	0x01
#define ENABLE_CTR2     0x02
#define ENABLE_DAC      0x04
#define ENABLE_CTR1     0x08

static	int channel;
static	int freq1 = 1000;
static	int freq2 = 1000;
static	signed char waveform[2] = { -120, 120 };

/************************************/
/* Sound handler start				*/
/************************************/
int meadows_sh_start(const struct MachineSound *msound)
{
	int vol[2];

	vol[0]=vol[1]=255;
	channel = mixer_allocate_channels(2,vol);
	mixer_set_volume(channel,0);
	mixer_play_sample(channel,waveform,sizeof(waveform),freq1,1);
	mixer_set_volume(channel+1,0);
	mixer_play_sample(channel+1,waveform,sizeof(waveform),freq2,1);
    return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void meadows_sh_stop(void)
{
	mixer_stop_sample(channel);
    mixer_stop_sample(channel+1);
}

/************************************/
/* Sound handler update 			*/
/************************************/
void meadows_sh_update(void)
{
static  unsigned char latched_0c01 = 0;
static	unsigned char latched_0c02 = 0;
static	unsigned char latched_0c03 = 0;
int preset, amp;

    if (latched_0c01 != meadows_0c01 || latched_0c03 != meadows_0c03)
	{
		/* amplitude is a combination of the upper 4 bits of 0c01 */
		/* and bit 4 merged from S2650's flag output */
		amp = ((meadows_0c03 & ENABLE_CTR1) == 0) ? 0 : (meadows_0c01 & 0xf0) >> 1;
		if( s2650_get_flag() )
			amp += 0x80;
		/* calculate frequency for counter #1 */
		/* bit 0..3 of 0c01 are ctr preset */
		preset = (meadows_0c01 & 15) ^ 15;
		if (preset)
			freq1 = BASE_CTR1 / (preset + 1);
		else amp = 0;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "meadows ctr1 channel #%d preset:%3d freq:%5d amp:%d\n", channel, preset, freq1, amp);
		mixer_set_sample_frequency(channel, freq1 * sizeof(waveform));
		mixer_set_volume(channel,amp*100/255);
    }

	if (latched_0c02 != meadows_0c02 || latched_0c03 != meadows_0c03)
	{
		/* calculate frequency for counter #2 */
		/* 0c02 is ctr preset, 0c03 bit 0 enables division by 2 */
		amp = ((meadows_0c03 & ENABLE_CTR2) != 0) ? 0xa0 : 0;
		preset = meadows_0c02 ^ 0xff;
		if (preset)
		{
			freq2 = BASE_CTR2 / (preset + 1) / 2;
			if ((meadows_0c03 & DIV2OR4_CTR2) == 0)
				freq2 >>= 1;
		}
		else amp = 0;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "meadows ctr2 channel #%d preset:%3d freq:%5d amp:%d\n", channel+1, preset, freq2, amp);
		mixer_set_sample_frequency(channel+1, freq2 * sizeof(waveform));
		mixer_set_volume(channel+1,amp*100/255);
    }

	if (latched_0c03 != meadows_0c03)
	{
		dac_enable = meadows_0c03 & ENABLE_DAC;

		if (dac_enable)
			DAC_data_w(0, meadows_dac);
		else
			DAC_data_w(0, 0);
    }

    latched_0c01 = meadows_0c01;
	latched_0c02 = meadows_0c02;
	latched_0c03 = meadows_0c03;
}

/************************************/
/* Write DAC value					*/
/************************************/
void meadows_sh_dac_w(int data)
{
	meadows_dac = data;
	if (dac_enable)
		DAC_data_w(0, meadows_dac);
	else
		DAC_data_w(0, 0);
}


