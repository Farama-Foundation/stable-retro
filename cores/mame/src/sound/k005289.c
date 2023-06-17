/***************************************************************************

	Konami 005289 - SCC sound as used in Bubblesystem

	This file is pieced together by Bryan McPhail from a combination of
	Namco Sound, Amuse by Cab, Nemesis schematics and whoever first
	figured out SCC!

	The 005289 is a 2 channel sound generator, each channel gets it's
	waveform from a prom (4 bits wide).

	(From Nemesis schematics)

	Address lines A0-A4 of the prom run to the 005289, giving 32 bytes
	per waveform.  Address lines A5-A7 of the prom run to PA5-PA7 of
	the AY8910 control port A, giving 8 different waveforms. PA0-PA3
	of the AY8910 control volume.

	The second channel is the same as above except port B is used.

	The 005289 has no data bus, so data values written don't matter.

	There are 4 unknown pins, LD1, LD2, TG1, TG2.  Two of them look to be
	the selector for changing frequency.  The other two seem unused.

***************************************************************************/

#include "driver.h"

#define FREQBASEBITS	16

/* this structure defines the parameters for a channel */
typedef struct
{
	int frequency;
	int counter;
	int volume;
	const unsigned char *wave;
} k005289_sound_channel;

static k005289_sound_channel channel_list[2];

/* global sound parameters */
static const unsigned char *sound_prom;
static int stream,mclock,rate;

/* mixer tables and internal buffers */
static INT16 *mixer_table;
static INT16 *mixer_lookup;
static short *mixer_buffer;

static int k005289_A_frequency,k005289_B_frequency;
static int k005289_A_volume,k005289_B_volume;
static int k005289_A_waveform,k005289_B_waveform;
static int k005289_A_latch,k005289_B_latch;

/* build a table to divide by the number of voices */
static int make_mixer_table(int voices)
{
	int count = voices * 128;
	int i;
	int gain = 16;

	/* allocate memory */
	mixer_table = malloc(256 * voices * sizeof(INT16));
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
	mixer_lookup = mixer_table + (128 * voices);

	/* fill in the table - 16 bit case */
	for (i = 0; i < count; i++)
	{
		int val = i * gain * 16 / voices;
		if (val > 32767) val = 32767;
		mixer_lookup[ i] = val;
		mixer_lookup[-i] = -val;
	}

	return 0;
}


/* generate sound to the mix buffer */
static void K005289_update(int ch, INT16 *buffer, int length)
{
	k005289_sound_channel *voice=channel_list;
	short *mix;
	int i,v,f;

	/* zap the contents of the mixer buffer */
	memset(mixer_buffer, 0, length * sizeof(INT16));

	v=voice[0].volume;
	f=voice[0].frequency;
	if (v && f)
	{
		const unsigned char *w = voice[0].wave;
		int c = voice[0].counter;

		mix = mixer_buffer;

		/* add our contribution */
		for (i = 0; i < length; i++)
		{
			int offs;

			c+=(long)((((float)mclock / (float)(f * 16))*(float)(1<<FREQBASEBITS)) / (float)(rate / 32));
			offs = (c >> 16) & 0x1f;
			*mix++ += ((w[offs] & 0x0f) - 8) * v;
		}

		/* update the counter for this voice */
		voice[0].counter = c;
	}

	v=voice[1].volume;
	f=voice[1].frequency;
	if (v && f)
	{
		const unsigned char *w = voice[1].wave;
		int c = voice[1].counter;

		mix = mixer_buffer;

		/* add our contribution */
		for (i = 0; i < length; i++)
		{
			int offs;

			c+=(long)((((float)mclock / (float)(f * 16))*(float)(1<<FREQBASEBITS)) / (float)(rate / 32));
			offs = (c >> 16) & 0x1f;
			*mix++ += ((w[offs] & 0x0f) - 8) * v;
		}

		/* update the counter for this voice */
		voice[1].counter = c;
	}

	/* mix it down */
	mix = mixer_buffer;
	for (i = 0; i < length; i++)
		*buffer++ = mixer_lookup[*mix++];
}

int K005289_sh_start(const struct MachineSound *msound)
{
	const char *snd_name = "K005289";
	k005289_sound_channel *voice=channel_list;
	const struct k005289_interface *intf = msound->sound_interface;

	/* get stream channels */
	stream = stream_init(snd_name, intf->volume, Machine->sample_rate, 0, K005289_update);
	mclock = intf->master_clock;
	rate = Machine->sample_rate;

	/* allocate a pair of buffers to mix into - 1 second's worth should be more than enough */
	if ((mixer_buffer = malloc(2 * sizeof(short) * Machine->sample_rate)) == 0)
		return 1;

	/* build the mixer table */
	if (make_mixer_table(2))
	{
		free (mixer_buffer);
		return 1;
	}

	sound_prom = memory_region(intf->region);

	/* reset all the voices */
	voice[0].frequency = 0;
	voice[0].volume = 0;
	voice[0].wave = &sound_prom[0];
	voice[0].counter = 0;
	voice[1].frequency = 0;
	voice[1].volume = 0;
	voice[1].wave = &sound_prom[0x100];
	voice[1].counter = 0;

	return 0;
}


void K005289_sh_stop(void)
{
	free (mixer_table);
	free (mixer_buffer);
}

/********************************************************************************/

static void k005289_recompute(void)
{
	k005289_sound_channel *voice = channel_list;

	stream_update(stream,0); 	/* update the streams */

	voice[0].frequency = k005289_A_frequency;
	voice[1].frequency = k005289_B_frequency;
	voice[0].volume = k005289_A_volume;
	voice[1].volume = k005289_B_volume;
	voice[0].wave = &sound_prom[32 * k005289_A_waveform];
	voice[1].wave = &sound_prom[32 * k005289_B_waveform + 0x100];
}

WRITE_HANDLER( k005289_control_A_w )
{
	k005289_A_volume=data&0xf;
	k005289_A_waveform=data>>5;
	k005289_recompute();
}

WRITE_HANDLER( k005289_control_B_w )
{
	k005289_B_volume=data&0xf;
	k005289_B_waveform=data>>5;
	k005289_recompute();
}

WRITE_HANDLER( k005289_pitch_A_w )
{
	k005289_A_latch = 0x1000 - offset;
}

WRITE_HANDLER( k005289_pitch_B_w )
{
	k005289_B_latch = 0x1000 - offset;
}

WRITE_HANDLER( k005289_keylatch_A_w )
{
	k005289_A_frequency = k005289_A_latch;
	k005289_recompute();
}

WRITE_HANDLER( k005289_keylatch_B_w )
{
	k005289_B_frequency = k005289_B_latch;
	k005289_recompute();
}
