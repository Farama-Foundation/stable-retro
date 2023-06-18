#include "driver.h"

void readsample(struct GameSample *SampleInfo, int channel, struct GameSamples *SamplesData, int load);

static int firstchannel,numchannels;
int leftSampleNum;
int rightSampleNum;

/* Start one of the samples loaded from disk. Note: channel must be in the range */
/* 0 .. Samplesinterface->channels-1. It is NOT the discrete channel to pass to */
/* mixer_play_sample() */
void sample_start(int channel,int samplenum,int loop)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (Machine->samples->sample[samplenum] == 0) return;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_start() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}
	if (samplenum >= Machine->samples->total)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_start() called with samplenum = %d, but only %d samples available\n",samplenum,Machine->samples->total);
		return;
	}

	if (Machine->samples->sample[samplenum] != NULL)
	{
		if (Machine->samples->sample[samplenum]->b_decoded == 0)
		{
			/* Lets decode this sample before playing it. */
			readsample(Machine->samples->sample[samplenum], samplenum, Machine->samples, 1);
		}

		if (Machine->samples->sample[samplenum]->b_decoded == 1)
		{
			if (channel == 0)
				leftSampleNum = samplenum;

			if (channel == 1)
				rightSampleNum = samplenum;
		}

		if (Machine->samples->sample[samplenum]->resolution == 8 )
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE"play 8 bit sample %d, channel %d\n",samplenum,channel);
			mixer_play_sample(firstchannel + channel,
			Machine->samples->sample[samplenum]->data,
			Machine->samples->sample[samplenum]->length,
			Machine->samples->sample[samplenum]->smpfreq,
			loop);
		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE"play 16 bit sample %d, channel %d\n",samplenum,channel);
			mixer_play_sample_16(firstchannel + channel,
			(short *) Machine->samples->sample[samplenum]->data,
			Machine->samples->sample[samplenum]->length,
			Machine->samples->sample[samplenum]->smpfreq,
			loop);
		}

	}
}

void sample_set_freq(int channel,int freq)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_adjust() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_sample_frequency(channel + firstchannel,freq);
}

/* Set sample volume by speaker. */
void sample_set_stereo_volume(int channel,int volume_left, int volume_right)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_adjust() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_stereo_volume(channel + firstchannel,volume_left * 100 / 255, volume_right * 100 / 255);
}

void sample_set_volume(int channel,int volume)
{
	if (Machine->sample_rate == 0) return;
	if (Machine->samples == 0) return;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_adjust() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_set_volume(channel + firstchannel,volume * 100 / 255);
}

void sample_stop(int channel)
{
	int c_sample=0;

	if (channel == 0)
		c_sample = leftSampleNum;
	else if (channel == 1)
		c_sample = rightSampleNum;

	if (Machine->sample_rate == 0) return;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_stop() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return;
	}

	mixer_stop_sample(channel + firstchannel);

	/*respect samples being disabled */
	if ( !options.use_samples ) return;
	if ( (!options.use_alt_sound) || (!options.content_flags[CONTENT_ALT_SOUND]) ) return;
	if ( ( channel != 0) || (channel !=1) ) return; /* return normally if not matching ost channel specs */

	if (Machine->samples->sample[c_sample] != NULL) {
		if (Machine->samples->sample[c_sample]->b_decoded == 1) {
			/* A non pre loaded sample, lets free from memory. Useful for devices with limited amount of RAM using large sample files. */
			if (Machine->samples->sample[c_sample]->length > GAME_SAMPLE_LARGE)
				readsample(Machine->samples->sample[c_sample], c_sample, Machine->samples, 0);

			if (channel == 0)
				leftSampleNum = NULL;

			if (channel == 1)
				rightSampleNum = NULL;
		}
	}
}

int sample_playing(int channel)
{
	if (Machine->sample_rate == 0) return 0;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_playing() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return 0;
	}

	return mixer_is_sample_playing(channel + firstchannel);
}



int samples_sh_start(const struct MachineSound *msound)
{
	int i;
	int vol[MIXER_MAX_CHANNELS];
	const struct Samplesinterface *intf = msound->sound_interface;

	/* read audio samples if available */
	Machine->samples = readsamples(intf->samplenames,Machine->gamedrv->name);

	numchannels = intf->channels;
	for (i = 0;i < numchannels;i++)
		vol[i] = intf->volume;
	firstchannel = mixer_allocate_channels(numchannels,vol);
	for (i = 0;i < numchannels;i++)
	{
		char buf[40];

		sprintf(buf,"Sample #%d",i);
		mixer_set_name(firstchannel + i,buf);
	}
	return 0;
}

int sample_set_pause(int channel,int pause)
{
  if (Machine->sample_rate == 0) return 0;
	if (channel >= numchannels)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE"error: sample_pause() called with channel = %d, but only %d channels allocated\n",channel,numchannels);
		return 0;
	}
  mixer_set_pause(channel + firstchannel, pause);
  return 1;
}
