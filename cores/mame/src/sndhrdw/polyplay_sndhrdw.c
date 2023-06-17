/***************************************************************************

  Poly-Play
  (c) 1985 by VEB Polytechnik Karl-Marx-Stadt

  sound hardware

  driver written by Martin Buchholz (buchholz@mail.uni-greifswald.de)

***************************************************************************/
#include "driver.h"
#include <math.h>

#define LFO_VOLUME 25
#define SAMPLE_LENGTH 32
#define SAMPLE_AMPLITUDE 0x4000

static int freq1, freq2, channellfo, channel_playing1, channel_playing2;
int lfovol[2] = {LFO_VOLUME,LFO_VOLUME};

static INT16 backgroundwave[SAMPLE_LENGTH];

int polyplay_sh_start(const struct MachineSound *msound)
{
	int i;

	for (i = 0; i < SAMPLE_LENGTH / 2; i++) {
		backgroundwave[i] = + SAMPLE_AMPLITUDE;
	}
	for (i = SAMPLE_LENGTH / 2; i < SAMPLE_LENGTH; i++) {
		backgroundwave[i] = - SAMPLE_AMPLITUDE;
	}
	freq1 = freq2 = 110;
	channellfo = mixer_allocate_channels(2,lfovol);
	mixer_set_name(channellfo+0,"Polyplay #0");
	mixer_set_name(channellfo+1,"Polyplay #1");
	mixer_set_volume(channellfo+0,0);
	mixer_set_volume(channellfo+1,0);

	channel_playing1 = 0;
	channel_playing2 = 0;
	return 0;
}

void polyplay_sh_stop(void)
{
	mixer_stop_sample(channellfo+0);
	mixer_stop_sample(channellfo+1);
}

void polyplay_sh_update(void)
{
	mixer_set_sample_frequency(channellfo+0, sizeof(backgroundwave)*freq1 );
	mixer_set_sample_frequency(channellfo+1, sizeof(backgroundwave)*freq2 );
}

void set_channel1(int active)
{
	channel_playing1 = active;
}

void set_channel2(int active)
{
	channel_playing2 = active;
}

void play_channel1(int data)
{
	if (data) {
		freq1 = 2457600 / 16 / data / 8;
		mixer_set_volume(channellfo+0, channel_playing1 * 100);
		mixer_play_sample_16(channellfo+0, backgroundwave, sizeof(backgroundwave), sizeof(backgroundwave)*freq1,1);
	}
	else {
		polyplay_sh_stop();
	}
}

void play_channel2(int data)
{
	if (data) {
		freq2 = 2457600 / 16 / data / 8;
		mixer_set_volume(channellfo+1, channel_playing2 * 100);
		mixer_play_sample_16(channellfo+1, backgroundwave, sizeof(backgroundwave), sizeof(backgroundwave)*freq2,1);
	}
	else {
		polyplay_sh_stop();
	}
}
