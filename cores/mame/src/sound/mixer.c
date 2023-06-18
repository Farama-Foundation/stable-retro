/***************************************************************************

  mixer.c

  Manage audio channels allocation, with volume and panning control

***************************************************************************/

#include "driver.h"
#include "filter.h"

#include <math.h>
#include <limits.h>
#include <assert.h>

/***************************************************************************/
/* Options */

/* Define it to enable the logerror output */
/* #define MIXER_USE_LOGERROR */

/***************************************************************************/
/* Config */

/* Internal log */
#ifdef MIXER_USE_LOGERROR
#define mixerlogerror(a) logerror a
#else
#define mixerlogerror(a) do { } while (0)
#endif

/* accumulators have ACCUMULATOR_SAMPLES samples (must be a power of 2) */
#define ACCUMULATOR_SAMPLES		8192
#define ACCUMULATOR_MASK		(ACCUMULATOR_SAMPLES - 1)

/* fractional numbers have FRACTION_BITS bits of resolution */
#define FRACTION_BITS			16
#define FRACTION_MASK			((1 << FRACTION_BITS) - 1)

/***************************************************************************/
/* Static data */

static int mixer_sound_enabled;

/* holds all the data for the a mixer channel */
struct mixer_channel_data
{
	char name[40];

	/* current volume, gain and pan */
	int left_volume;
	int right_volume;
	int gain;
	int pan;

	/* mixing levels */
	unsigned mixing_level;
	unsigned default_mixing_level;
	unsigned config_mixing_level;
	unsigned config_default_mixing_level;

	/* current playback positions */
	unsigned samples_available;

	/* resample state */
	int frac; /* resample fixed point state (used if filter is not active) */
	int pivot; /* resample brehesnam state (used if filter is active) */
	int step; /* fixed point increment */
	unsigned from_frequency; /* current source frequency */
	unsigned to_frequency; /* current destination frequency */
	unsigned lowpass_frequency; /* current lowpass arbitrary cut frequency, 0 if default */
	filter* filter; /* filter used, ==0 if none */
	filter_state* left; /* state of the filter for the left/mono channel */
	filter_state* right; /* state of the filter for the right channel */
	int is_reset_requested; /* state reset requested */

	/* lowpass filter request */
	unsigned request_lowpass_frequency; /* request for the lowpass arbitrary cut frequency, 0 if default */

	/* state of non-streamed playback */
	int is_stream;
	int is_playing;
	int is_looping;
	int is_paused;
	int is_16bit;
	void* data_start;
	void* data_end;
	void* data_current;
};

/* channel data */
static struct mixer_channel_data mixer_channel[MIXER_MAX_CHANNELS];
static unsigned config_mixing_level[MIXER_MAX_CHANNELS];
static unsigned config_default_mixing_level[MIXER_MAX_CHANNELS];
static int first_free_channel = 0;
static int is_config_invalid;
static int is_stereo;

/* 32-bit accumulators */
static unsigned accum_base;
static int left_accum[ACCUMULATOR_SAMPLES];
static int right_accum[ACCUMULATOR_SAMPLES];

/* 16-bit mix buffers */
static INT16 mix_buffer[ACCUMULATOR_SAMPLES*2]; /* *2 for stereo */

/* global sample tracking */
static unsigned samples_this_frame;

/***************************************************************************
	mixer_channel_resample
***************************************************************************/

/* Window size of the FIR filter in samples (must be odd) */
/* Greater values are more precise, lesser values are faster. */
#define FILTER_WIDTH 31

/* The number of samples that need to be played to flush the filter state */
/* For the FIR filters it's equal to the filter width */
#define FILTER_FLUSH FILTER_WIDTH

/* Setup the resample information
	from_frequency - input frequency
	lowpass_frequency - lowpass frequency, use 0 to automatically compute it from the resample operation
	restart - restart the resample state
*/
static void mixer_channel_resample_set(struct mixer_channel_data *channel, unsigned from_frequency, unsigned lowpass_frequency, int restart)
{
	unsigned to_frequency;
	to_frequency = Machine->sample_rate;

	mixerlogerror(("Mixer:mixer_channel_resample_set(%s,%d,%d)\n",channel->name,from_frequency,lowpass_frequency,restart));

	if (restart)
	{
		mixerlogerror(("\tpivot=0\n"));
		channel->pivot = 0;
		channel->frac = 0;
	}

	/* only if the filter change */
	if (from_frequency != channel->from_frequency
		|| to_frequency != channel->to_frequency
		|| lowpass_frequency != channel->lowpass_frequency)
	{
		/* delete the previous filter */
		if (channel->filter)
		{
			filter_free(channel->filter);
			channel->filter = 0;
		}

		/* make a new filter */
		if ((from_frequency != 0 && to_frequency != 0 && (from_frequency != to_frequency || lowpass_frequency != 0)))
		{
			double cut;
			unsigned cut_frequency;

			if (from_frequency < to_frequency) {
				/* upsampling */
				cut_frequency = from_frequency / 2;
				if (lowpass_frequency != 0 && cut_frequency > lowpass_frequency)
					cut_frequency = lowpass_frequency;
				cut = (double)cut_frequency / to_frequency;
			} else {
				/* downsampling */
				cut_frequency = to_frequency / 2;
				if (lowpass_frequency != 0 && cut_frequency > lowpass_frequency)
					cut_frequency = lowpass_frequency;
				cut = (double)cut_frequency / from_frequency;
			}

			channel->filter = filter_lp_fir_alloc(cut, FILTER_WIDTH);

			mixerlogerror(("\tfilter from %d Hz, to %d Hz, cut %f, cut %d Hz\n",from_frequency,to_frequency,cut,cut_frequency));
		}
	}

	channel->lowpass_frequency = lowpass_frequency;
	channel->from_frequency = from_frequency;
	channel->to_frequency = to_frequency;
	channel->step = (double)from_frequency * (1 << FRACTION_BITS) / to_frequency;

	/* reset the filter state */
	if (channel->filter && channel->is_reset_requested)
	{
		mixerlogerror(("\tstate clear\n"));
		channel->is_reset_requested = 0;
		filter_state_reset(channel->filter,channel->left);
		filter_state_reset(channel->filter,channel->right);
	}
}

/* Resample a channel
	channel - channel info
	state - filter state
	volume - volume (0-255)
	dst - destination vector
	dst_len - max number of destination samples
	src - source vector, (updated at the exit)
	src_len - max number of source samples
*/
static unsigned mixer_channel_resample_16(struct mixer_channel_data* channel, filter_state* state, int volume, int* dst, unsigned dst_len, INT16** psrc, unsigned src_len)
{
	unsigned dst_base = (accum_base + channel->samples_available) & ACCUMULATOR_MASK;
	unsigned dst_pos = dst_base;

	INT16* src = *psrc;

	assert( dst_len <= ACCUMULATOR_MASK );

	if (!channel->filter)
	{
		if (channel->from_frequency == channel->to_frequency)
		{
			/* copy */
			unsigned len;
			INT16* src_end;
			if (src_len > dst_len)
				len = dst_len;
			else
				len = src_len;

#ifdef X86_ASM /* this is very hardware dependant */
			/* optimized version (a small but measurable speedup) */
			while (len) {
				unsigned run;
				int* rundst;

				run = ACCUMULATOR_MASK + 1 - dst_pos;
				if (run > len)
					run = len;
				len -= run;

				src_end = src + (run & 3);
				while (src != src_end) {
					dst[dst_pos] += (*src * volume) >> 8;
					dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
					++src;
				}

				rundst = dst + dst_pos;
				src_end = src + (run & ~3);
				dst_pos = (dst_pos + (run & ~3)) & ACCUMULATOR_MASK;
				while (src != src_end) {
					rundst[0] += (src[0] * volume) >> 8;
					rundst[1] += (src[1] * volume) >> 8;
					rundst[2] += (src[2] * volume) >> 8;
					rundst[3] += (src[3] * volume) >> 8;
					rundst += 4;
					src += 4;
				}
			}
#else
			/* reference version */
			src_end = src + len;
			while (src != src_end)
			{
				dst[dst_pos] += (*src * volume) >> 8;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				++src;
			}
#endif
		} else {
			/* end address */
			INT16* src_end = src + src_len;
			unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

			int step = channel->step;
			int frac = channel->frac;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;

			while (src < src_end && dst_pos != dst_pos_end)
			{
				dst[dst_pos] += (*src * volume) >> 8;
				frac += step;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				src += frac >> FRACTION_BITS;
				frac &= FRACTION_MASK;
			}

			/* adjust the end if it's too big */
			if (src > src_end) {
				frac += (src - src_end) << FRACTION_BITS;
				src = src_end;
			}

			channel->frac = frac;
		}
	} else if (!channel->from_frequency) {
		dst_pos = (dst_pos + dst_len) & ACCUMULATOR_MASK;
	} else {
		int pivot = channel->pivot;

		/* end address */
		INT16* src_end = src + src_len;
		unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		/* volume */
		filter_real v = volume;

		if (channel->from_frequency < channel->to_frequency)
		{
			/* upsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v / 256.0);
				pivot += channel->from_frequency;
				if (pivot > 0)
				{
					pivot -= channel->to_frequency;
					++src;
				}
				/* dest */
				dst[dst_pos] += filter_compute(channel->filter,state);
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			}
		} else {
			/* downsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v / 256.0);
				pivot -= channel->to_frequency;
				++src;
				/* dest */
				if (pivot < 0)
				{
					pivot += channel->from_frequency;
					dst[dst_pos] += filter_compute(channel->filter,state);
					dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				}
			}
		}

		channel->pivot = pivot;
	}

	*psrc = src;

	return (dst_pos - dst_base) & ACCUMULATOR_MASK;
}

static unsigned mixer_channel_resample_8(struct mixer_channel_data *channel, filter_state* state, int volume, int* dst, unsigned dst_len, INT8** psrc, unsigned src_len)
{
	unsigned dst_base = (accum_base + channel->samples_available) & ACCUMULATOR_MASK;
	unsigned dst_pos = dst_base;

	INT8* src = *psrc;

	assert( dst_len <= ACCUMULATOR_MASK );

	if (!channel->filter)
	{
		if (channel->from_frequency == channel->to_frequency)
		{
			/* copy */
			unsigned len;
			INT8* src_end;
			if (src_len > dst_len)
				len = dst_len;
			else
				len = src_len;

			src_end = src + len;
			while (src != src_end)
			{
				dst[dst_pos] += *src * volume;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				++src;
			}
		} else {
			/* end address */
			INT8* src_end = src + src_len;
			unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

			int step = channel->step;
			int frac = channel->frac;
			src += frac >> FRACTION_BITS;
			frac &= FRACTION_MASK;

			while (src < src_end && dst_pos != dst_pos_end)
			{
				dst[dst_pos] += *src * volume;
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				frac += step;
				src += frac >> FRACTION_BITS;
				frac &= FRACTION_MASK;
			}

			/* adjust the end if it's too big */
			if (src > src_end) {
				frac += (src - src_end) << FRACTION_BITS;
				src = src_end;
			}

			channel->frac = frac;
		}
	} else if (!channel->from_frequency) {
		dst_pos = (dst_pos + dst_len) & ACCUMULATOR_MASK;
	} else {
		int pivot = channel->pivot;

		/* end address */
		INT8* src_end = src + src_len;
		unsigned dst_pos_end = (dst_pos + dst_len) & ACCUMULATOR_MASK;

		/* volume */
		filter_real v = volume;

		if (channel->from_frequency < channel->to_frequency)
		{
			/* upsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v);
				pivot += channel->from_frequency;
				if (pivot > 0)
				{
					pivot -= channel->to_frequency;
					++src;
				}
				/* dest */
				dst[dst_pos] += filter_compute(channel->filter,state);
				dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
			}
		} else {
			/* downsampling */
			while (src != src_end && dst_pos != dst_pos_end)
			{
				/* source */
				filter_insert(channel->filter,state,*src * v);
				pivot -= channel->to_frequency;
				++src;
				/* dest */
				if (pivot < 0)
				{
					pivot += channel->from_frequency;
					dst[dst_pos] += filter_compute(channel->filter,state);
					dst_pos = (dst_pos + 1) & ACCUMULATOR_MASK;
				}
			}
		}

		channel->pivot = pivot;
	}

	*psrc = src;

	return (dst_pos - dst_base) & ACCUMULATOR_MASK;
}

/* Mix a 8 bit channel */
static unsigned mixer_channel_resample_8_pan(struct mixer_channel_data *channel, int* volume, unsigned dst_len, INT8** src, unsigned src_len)
{
	unsigned count;

	if (!is_stereo || channel->pan == MIXER_PAN_LEFT) {
		count = mixer_channel_resample_8(channel, channel->left, volume[0], left_accum, dst_len, src, src_len);
	} else if (channel->pan == MIXER_PAN_RIGHT) {
		count = mixer_channel_resample_8(channel, channel->right, volume[1], right_accum, dst_len, src, src_len);
	} else {
		/* save */
		unsigned save_pivot = channel->pivot;
		unsigned save_frac = channel->frac;
		INT8* save_src = *src;
		count = mixer_channel_resample_8(channel, channel->left, volume[0], left_accum, dst_len, src, src_len);
		/* restore */
		channel->pivot = save_pivot;
		channel->frac = save_frac;
		*src = save_src;
		mixer_channel_resample_8(channel, channel->right, volume[1], right_accum, dst_len, src, src_len);
	}

	channel->samples_available += count;
	return count;
}

/* Mix a 16 bit channel */
static unsigned mixer_channel_resample_16_pan(struct mixer_channel_data *channel, int* volume, unsigned dst_len, INT16** src, unsigned src_len)
{
	unsigned count;

	if (!is_stereo || channel->pan == MIXER_PAN_LEFT) {
		count = mixer_channel_resample_16(channel, channel->left, volume[0], left_accum, dst_len, src, src_len);
	} else if (channel->pan == MIXER_PAN_RIGHT) {
		count = mixer_channel_resample_16(channel, channel->right, volume[1], right_accum, dst_len, src, src_len);
	} else {
		/* save */
		unsigned save_pivot = channel->pivot;
		unsigned save_frac = channel->frac;
		INT16* save_src = *src;
		count = mixer_channel_resample_16(channel, channel->left, volume[0], left_accum, dst_len, src, src_len);
		/* restore */
		channel->pivot = save_pivot;
		channel->frac = save_frac;
		*src = save_src;
		mixer_channel_resample_16(channel, channel->right, volume[1], right_accum, dst_len, src, src_len);
	}

	channel->samples_available += count;
	return count;
}

/***************************************************************************
	mix_sample_8
***************************************************************************/

void mix_sample_8(struct mixer_channel_data *channel, int samples_to_generate)
{
	INT8 *source, *source_end;
	int mixing_volume[2];

	/* compute the overall mixing volume */
	if (mixer_sound_enabled)
	{
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}
	/* get the initial state */
	source = channel->data_current;
	source_end = channel->data_end;

	/* an outer loop to handle looping samples */
	while (samples_to_generate > 0)
	{
		samples_to_generate -= mixer_channel_resample_8_pan(channel,mixing_volume,samples_to_generate,&source,source_end - source);

		assert( source <= source_end );

		/* handle the end case */
		if (source >= source_end)
		{
			/* if we're done, stop playing */
			if (!channel->is_looping)
			{
				channel->is_playing = 0;
				break;
			}

			/* if we're looping, wrap to the beginning */
			else
				source -= (INT8 *)source_end - (INT8 *)channel->data_start;
		}
	}

	/* update the final positions */
	channel->data_current = source;
}

/***************************************************************************
	mix_sample_16
***************************************************************************/

void mix_sample_16(struct mixer_channel_data *channel, int samples_to_generate)
{
	INT16 *source, *source_end;
	int mixing_volume[2];

	/* compute the overall mixing volume */
	if (mixer_sound_enabled)
	{
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}
	/* get the initial state */
	source = channel->data_current;
	source_end = channel->data_end;

	/* an outer loop to handle looping samples */
	while (samples_to_generate > 0)
	{
		samples_to_generate -= mixer_channel_resample_16_pan(channel,mixing_volume,samples_to_generate,&source,source_end - source);

		assert( source <= source_end );

		/* handle the end case */
		if (source >= source_end)
		{
			/* if we're done, stop playing */
			if (!channel->is_looping)
			{
				channel->is_playing = 0;
				break;
			}

			/* if we're looping, wrap to the beginning */
			else
				source -= (INT16 *)source_end - (INT16 *)channel->data_start;
		}
	}

	/* update the final positions */
	channel->data_current = source;
}

/***************************************************************************
	mixer_flush
***************************************************************************/

/* Silence samples */
static unsigned char silence_data[FILTER_FLUSH];

/* Flush the state of the filter playing some 0 samples */
static void mixer_flush(struct mixer_channel_data *channel)
{
	INT8 *source_begin, *source_end;
	int mixing_volume[2];
	unsigned save_available;

	mixerlogerror(("Mixer:mixer_flush(%s)\n",channel->name));

	/* filter reset request */
	channel->is_reset_requested = 1;

	/* null volume */
	mixing_volume[0] = 0;
	mixing_volume[1] = 0;

	/* null data */
	source_begin = (INT8*)silence_data;
	source_end = (INT8*)silence_data + FILTER_FLUSH;

	/* save the number of samples availables */
	save_available = channel->samples_available;

	/* mix the silence */
	mixer_channel_resample_8_pan(channel,mixing_volume,ACCUMULATOR_MASK,&source_begin,source_end - source_begin);

	/* restore the number of samples availables */
	channel->samples_available = save_available;
}

/***************************************************************************
	mixer_sh_start
***************************************************************************/

int mixer_sh_start(void)
{
	struct mixer_channel_data *channel;
	int i;

	/* reset all channels to their defaults */
	memset(&mixer_channel, 0, sizeof(mixer_channel));
	for (i = 0, channel = mixer_channel; i < MIXER_MAX_CHANNELS; i++, channel++)
	{
		channel->mixing_level 					= 0xff;
		channel->default_mixing_level 			= 0xff;
		channel->config_mixing_level 			= config_mixing_level[i];
		channel->config_default_mixing_level 	= config_default_mixing_level[i];

		channel->left = filter_state_alloc();
		channel->right = filter_state_alloc();
	}

	/* determine if we're playing in stereo or not */
	first_free_channel = 0;
	is_stereo = ((Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO) != 0);

	/* clear the accumulators */
	accum_base = 0;
	memset(left_accum, 0, sizeof(left_accum));
	memset(right_accum, 0, sizeof(right_accum));

	samples_this_frame = osd_start_audio_stream(is_stereo);

	mixer_sound_enabled = 1;

	return 0;
}


/***************************************************************************
	mixer_sh_stop
***************************************************************************/

void mixer_sh_stop(void)
{
	struct mixer_channel_data *channel;
	int i;

	osd_stop_audio_stream();

	for (i = 0, channel = mixer_channel; i < MIXER_MAX_CHANNELS; i++, channel++)
	{
		if (channel->filter)
			filter_free(channel->filter);
		filter_state_free(channel->left);
		filter_state_free(channel->right);
	}
}

/***************************************************************************
	mixer_update_channel
***************************************************************************/

void mixer_update_channel(struct mixer_channel_data *channel, int total_sample_count)
{
	int samples_to_generate = total_sample_count - channel->samples_available;

	/* don't do anything for streaming channels */
	if (channel->is_stream)
		return;

	/* if we're all caught up, just return */
	if (samples_to_generate <= 0)
		return;

  if ( channel->is_paused) return;

        /* if we're playing, mix in the data */
	if (channel->is_playing)
	{
		if (channel->is_16bit)
			mix_sample_16(channel, samples_to_generate);
		else
			mix_sample_8(channel, samples_to_generate);

		if (!channel->is_playing)
			mixer_flush(channel);
	}
}

/***************************************************************************
	mixer_sh_update
***************************************************************************/

void mixer_sh_update(void)
{
	struct mixer_channel_data* channel;
	unsigned accum_pos = accum_base;
	INT16 *mix;
	int sample;
	int i;

	profiler_mark(PROFILER_MIXER);

	/* update all channels (for streams this is a no-op) */
	for (i = 0, channel = mixer_channel; i < first_free_channel; i++, channel++)
	{
		mixer_update_channel(channel, samples_this_frame);

		/* if we needed more than they could give, adjust their pointers */
		if (samples_this_frame > channel->samples_available)
			channel->samples_available = 0;
		else
			channel->samples_available -= samples_this_frame;
	}

	/* copy the mono 32-bit data to a 16-bit buffer, clipping along the way */
	if (!is_stereo)
	{
		mix = mix_buffer;
		for (i = 0; i < samples_this_frame; i++)
		{
			/* fetch and clip the sample */
			sample = left_accum[accum_pos];
         MAME_CLAMP_SAMPLE(sample);

			/* store and zero out behind us */
			*mix++ = sample;
			left_accum[accum_pos] = 0;

			/* advance to the next sample */
			accum_pos = (accum_pos + 1) & ACCUMULATOR_MASK;
		}
	}

	/* copy the stereo 32-bit data to a 16-bit buffer, clipping along the way */
	else
	{
		mix = mix_buffer;
		for (i = 0; i < samples_this_frame; i++)
		{
			/* fetch and clip the left sample */
			sample = left_accum[accum_pos];
         MAME_CLAMP_SAMPLE(sample);

			/* store and zero out behind us */
			*mix++ = sample;
			left_accum[accum_pos] = 0;

			/* fetch and clip the right sample */
			sample = right_accum[accum_pos];
         MAME_CLAMP_SAMPLE(sample);

			/* store and zero out behind us */
			*mix++ = sample;
			right_accum[accum_pos] = 0;

			/* advance to the next sample */
			accum_pos = (accum_pos + 1) & ACCUMULATOR_MASK;
		}
	}

	/* play the result */
	samples_this_frame = osd_update_audio_stream(mix_buffer);

	accum_base = accum_pos;

	profiler_mark(PROFILER_END);
}


/***************************************************************************
	mixer_allocate_channel
***************************************************************************/

int mixer_allocate_channel(int default_mixing_level)
{
	/* this is just a degenerate case of the multi-channel mixer allocate */
	return mixer_allocate_channels(1, &default_mixing_level);
}


/***************************************************************************
	mixer_allocate_channels
***************************************************************************/

int mixer_allocate_channels(int channels, const int *default_mixing_levels)
{
	int i, j;

	mixerlogerror(("Mixer:mixer_allocate_channels(%d)\n",channels));

	/* make sure we didn't overrun the number of available channels */
	if (first_free_channel + channels > MIXER_MAX_CHANNELS)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Too many mixer channels (requested %d, available %d)\n", first_free_channel + channels, MIXER_MAX_CHANNELS);
		exit(1);
	}

	/* loop over channels requested */
	for (i = 0; i < channels; i++)
	{
		struct mixer_channel_data *channel = &mixer_channel[first_free_channel + i];

		/* extract the basic data */
		channel->default_mixing_level 	= MIXER_GET_LEVEL(default_mixing_levels[i]);
		channel->pan 					= MIXER_GET_PAN(default_mixing_levels[i]);
		channel->gain 					= MIXER_GET_GAIN(default_mixing_levels[i]);
		/* add by hiro-shi */
		channel->left_volume 				= 100;
		channel->right_volume 				= 100;

		/* backwards compatibility with old 0-255 volume range */
		if (channel->default_mixing_level > 100)
			channel->default_mixing_level = channel->default_mixing_level * 25 / 255;

		/* attempt to load in the configuration data for this channel */
		channel->mixing_level = channel->default_mixing_level;
		if (!is_config_invalid)
		{
			/* if the defaults match, set the mixing level from the config */
			if (channel->default_mixing_level == channel->config_default_mixing_level && channel->config_mixing_level <= 100)
				channel->mixing_level = channel->config_mixing_level;

			/* otherwise, invalidate all channels that have been created so far */
			else
			{
				is_config_invalid = 1;
				for (j = 0; j < first_free_channel + i; j++)
					mixer_set_mixing_level(j, mixer_channel[j].default_mixing_level);
			}
		}

		/* set the default name */
		mixer_set_name(first_free_channel + i, 0);
	}

	/* increment the counter and return the first one */
	first_free_channel += channels;
	return first_free_channel - channels;
}


/***************************************************************************
	mixer_set_name
***************************************************************************/

void mixer_set_name(int ch, const char *name)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	/* either copy the name or create a default one */
	if (name != NULL)
		strcpy(channel->name, name);
	else
		sprintf(channel->name, "<channel #%d>", ch);

	/* append left/right onto the channel as appropriate */
	if (channel->pan == MIXER_PAN_LEFT)
		strcat(channel->name, " (Lt)");
	else if (channel->pan == MIXER_PAN_RIGHT)
		strcat(channel->name, " (Rt)");
}


/***************************************************************************
	mixer_get_name
***************************************************************************/

const char *mixer_get_name(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	/* return a pointer to the name or a NULL for an unused channel */
	if (channel->name[0] != 0)
		return channel->name;
	else
		return NULL;
}


/***************************************************************************
	mixer_set_volume
***************************************************************************/

void mixer_set_volume(int ch, int volume)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->left_volume  = volume;
	channel->right_volume = volume;
}

/***************************************************************************
	mixer_set_Pause
***************************************************************************/

void mixer_set_pause(int ch, int pause)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->is_paused  = pause;
}

/***************************************************************************
	mixer_set_mixing_level
***************************************************************************/

void mixer_set_mixing_level(int ch, int level)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->mixing_level = level;
}


/***************************************************************************
	mixer_set_stereo_volume
***************************************************************************/
void mixer_set_stereo_volume(int ch, int l_vol, int r_vol )
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	channel->left_volume  = l_vol;
	channel->right_volume = r_vol;
}

/***************************************************************************
	mixer_get_mixing_level
***************************************************************************/

int mixer_get_mixing_level(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	return channel->mixing_level;
}


/***************************************************************************
	mixer_get_default_mixing_level
***************************************************************************/

int mixer_get_default_mixing_level(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	return channel->default_mixing_level;
}


/***************************************************************************
	mixer_load_config
***************************************************************************/

void mixer_load_config(const struct mixer_config *config)
{
	int i;

	for (i = 0; i < MIXER_MAX_CHANNELS; i++)
	{
		config_default_mixing_level[i] = config->default_levels[i];
		config_mixing_level[i] = config->mixing_levels[i];
	}
	is_config_invalid = 0;
}


/***************************************************************************
	mixer_save_config
***************************************************************************/

void mixer_save_config(struct mixer_config *config)
{
	int i;

	for (i = 0; i < MIXER_MAX_CHANNELS; i++)
	{
		config->default_levels[i] = mixer_channel[i].default_mixing_level;
		config->mixing_levels[i] = mixer_channel[i].mixing_level;
	}
}


/***************************************************************************
	mixer_read_config
***************************************************************************/

void mixer_read_config(mame_file *f)
{
	struct mixer_config config;

	if (mame_fread(f, config.default_levels, MIXER_MAX_CHANNELS) < MIXER_MAX_CHANNELS ||
	    mame_fread(f, config.mixing_levels, MIXER_MAX_CHANNELS) < MIXER_MAX_CHANNELS)
	{
		memset(config.default_levels, 0xff, sizeof(config.default_levels));
		memset(config.mixing_levels, 0xff, sizeof(config.mixing_levels));
	}

	mixer_load_config(&config);
}


/***************************************************************************
	mixer_write_config
***************************************************************************/

void mixer_write_config(mame_file *f)
{
	struct mixer_config config;

	mixer_save_config(&config);
	mame_fwrite(f, config.default_levels, MIXER_MAX_CHANNELS);
	mame_fwrite(f, config.mixing_levels, MIXER_MAX_CHANNELS);
}


/***************************************************************************
	mixer_play_streamed_sample_16
***************************************************************************/

void mixer_play_streamed_sample_16(int ch, INT16 *data, int len, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];
	int mixing_volume[2];

	mixerlogerror(("Mixer:mixer_play_streamed_sample_16(%s,,%d,%d)\n",channel->name,len/2,freq));

	/* skip if sound is off */
	if (Machine->sample_rate == 0)
		return;
	channel->is_stream = 1;

	profiler_mark(PROFILER_MIXER);

	/* compute the overall mixing volume */
	if (mixer_sound_enabled) {
		mixing_volume[0] = ((channel->left_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
		mixing_volume[1] = ((channel->right_volume * channel->mixing_level * 256) << channel->gain) / (100*100);
	} else {
		mixing_volume[0] = 0;
		mixing_volume[1] = 0;
	}

	mixer_channel_resample_set(channel,freq,channel->request_lowpass_frequency,0);

	/* compute the length in fractional form */
	len = len / 2; /* convert len from byte to word */

	mixer_channel_resample_16_pan(channel,mixing_volume,ACCUMULATOR_MASK,&data,len);

	profiler_mark(PROFILER_END);
}


/***************************************************************************
	mixer_samples_this_frame
***************************************************************************/

int mixer_samples_this_frame(void)
{
	return samples_this_frame;
}


/***************************************************************************
	mixer_need_samples_this_frame
***************************************************************************/
#define EXTRA_SAMPLES 1    /* safety margin for sampling rate conversion*/
int mixer_need_samples_this_frame(int channel,int freq)
{
	return (samples_this_frame - mixer_channel[channel].samples_available)
			* freq / Machine->sample_rate + EXTRA_SAMPLES;
}


/***************************************************************************
	mixer_play_sample
***************************************************************************/

void mixer_play_sample(int ch, INT8 *data, int len, int freq, int loop)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_play_sample_8(%s,,%d,%d,%s)\n",channel->name,len,freq,loop ? "loop" : "single"));

	/* skip if sound is off, or if this channel is a stream */
	if (Machine->sample_rate == 0 || channel->is_stream)
		return;

	/* update the state of this channel */
	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	mixer_channel_resample_set(channel,freq,channel->request_lowpass_frequency,1);

	/* now determine where to mix it */
	channel->data_start = data;
	channel->data_current = data;
	channel->data_end = (UINT8 *)data + len;
	channel->is_playing = 1;
	channel->is_looping = loop;
	channel->is_16bit = 0;
}


/***************************************************************************
	mixer_play_sample_16
***************************************************************************/

void mixer_play_sample_16(int ch, INT16 *data, int len, int freq, int loop)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_play_sample_16(%s,,%d,%d,%s)\n",channel->name,len/2,freq,loop ? "loop" : "single"));

	/* skip if sound is off, or if this channel is a stream */
	if (Machine->sample_rate == 0 || channel->is_stream)
		return;

	/* update the state of this channel */
	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	mixer_channel_resample_set(channel,freq,channel->request_lowpass_frequency,1);

	/* now determine where to mix it */
	channel->data_start = data;
	channel->data_current = data;
	channel->data_end = (UINT8 *)data + len;
	channel->is_playing = 1;
	channel->is_looping = loop;
	channel->is_16bit = 1;
}


/***************************************************************************
	mixer_stop_sample
***************************************************************************/

void mixer_stop_sample(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixerlogerror(("Mixer:mixer_stop_sample(%s)\n",channel->name));

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

	if (channel->is_playing) {
		channel->is_playing = 0;
		mixer_flush(channel);
	}
}

/***************************************************************************
	mixer_is_sample_playing
***************************************************************************/

int mixer_is_sample_playing(int ch)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	return channel->is_playing;
}


/***************************************************************************
	mixer_set_sample_frequency
***************************************************************************/

void mixer_set_sample_frequency(int ch, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	assert( !channel->is_stream );

	if (channel->is_playing) {
		mixerlogerror(("Mixer:mixer_set_sample_frequency(%s,%d)\n",channel->name,freq));

		mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));

		mixer_channel_resample_set(channel,freq,channel->request_lowpass_frequency,0);
	}
}

/***************************************************************************
	mixer_set_lowpass_frequency
***************************************************************************/

/* Set the desidered lowpass cut frequency.
This function should be called immeditially after the mixer_allocate() and
before the first play() call. Otherwise the lowpass frequency may be
unused until the next filter recompute.
	ch - channel
	freq - frequency in Hz. Use 0 to disable
*/
void mixer_set_lowpass_frequency(int ch, int freq)
{
	struct mixer_channel_data *channel = &mixer_channel[ch];

	assert(!channel->is_playing && !channel->is_stream);

	mixerlogerror(("Mixer:mixer_set_lowpass_frequency(%s,%d)\n",channel->name,freq));

	channel->request_lowpass_frequency = freq;
}

/***************************************************************************
	mixer_sound_enable_global_w
***************************************************************************/

void mixer_sound_enable_global_w(int enable)
{
	int i;
	struct mixer_channel_data *channel;

	/* update all channels (for streams this is a no-op) */
	for (i = 0, channel = mixer_channel; i < first_free_channel; i++, channel++)
	{
		mixer_update_channel(channel, sound_scalebufferpos(samples_this_frame));
	}

	mixer_sound_enabled = enable;
}
