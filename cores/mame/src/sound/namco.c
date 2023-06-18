/***************************************************************************

	NAMCO sound driver.

	This driver handles the four known types of NAMCO wavetable sounds:

		- 3-voice mono (PROM-based design: Pac-Man, Pengo, Dig Dug, etc)
		- 8-voice quadrophonic (Pole Position 1, Pole Position 2)
		- 8-voice mono (custom 15XX: Mappy, Dig Dug 2, etc)
		- 8-voice stereo (System 1)

***************************************************************************/

#include "driver.h"
#include "namco.h"


/* 8 voices max */
#define MAX_VOICES 8

#define MAX_VOLUME 16

/* quality parameter: internal sample rate is 192 KHz, output is 48 KHz */
#define INTERNAL_RATE	192000
#define OUTPUT_RATE	48000

/* oversampling rate */
#define OVERSAMPLING_RATE	(INTERNAL_RATE / OUTPUT_RATE)

/* 16 bits:	sample bits of the stream buffer	*/
/* 4 bits:	volume					*/
/* 4 bits:	prom sample bits			*/
#define MIXLEVEL	((1 << (16 - 4 - 4)) / OVERSAMPLING_RATE)

/* stream output level */
#define OUTPUT_LEVEL(n)		((n) * MIXLEVEL / num_voices)

/* a position of waveform sample */
#define WAVEFORM_POSITION(n)	(((n) >> f_fracbits) & 0x1f)


/* this structure defines the parameters for a channel */
typedef struct
{
	UINT32 frequency;
	UINT32 counter;
	int volume[2];
	int noise_sw;
	int noise_state;
	int noise_seed;
	UINT32 noise_counter;
	int waveform_select;
} sound_channel;


/* globals available to everyone */
unsigned char *namco_soundregs;
unsigned char *namco_wavedata;

/* data about the sound system */
static sound_channel channel_list[MAX_VOICES];
static sound_channel *last_channel;

/* global sound parameters */
static int wave_size;
static int num_voices;
static int sound_enable;
static int stream;
static int namco_clock;
static int sample_rate;
static int f_fracbits;

/* decoded waveform table */
static INT16 *waveform[MAX_VOLUME];


/* update the decoded waveform data */
static void update_namco_waveform(int offset, data8_t data)
{
	if (wave_size == 1)
	{
		INT16 wdata;
		int v;

		/* use full byte, first 4 high bits, then low 4 bits */
		for (v = 0; v < MAX_VOLUME; v++)
		{
			wdata = ((data >> 4) & 0x0f) - 8;
			waveform[v][offset * 2] = OUTPUT_LEVEL(wdata * v);
			wdata = (data & 0x0f) - 8;
			waveform[v][offset * 2 + 1] = OUTPUT_LEVEL(wdata * v);
		}
	}
	else
	{
		int v;

		/* use only low 4 bits */
		for (v = 0; v < MAX_VOLUME; v++)
			waveform[v][offset] = OUTPUT_LEVEL(((data & 0x0f) - 8) * v);
	}
}


/* build the decoded waveform table */
static int build_decoded_waveform(int region)
{
	INT16 *p;
	int size;
	int offset;
	int v;

	/* 20pacgal has waves in RAM but old sound system */
	if (region == -1 && num_voices != 3)
	{
		wave_size = 1;
		size = 32 * 16;		/* 32 samples, 16 waveforms */
	}
	else
	{
		wave_size = 0;
		size = 32 * 8;		/* 32 samples, 8 waveforms */
	}

	if ((p = (INT16 *)auto_malloc(size * MAX_VOLUME * sizeof (INT16))) == NULL)
		return 1;

	for (v = 0; v < MAX_VOLUME; v++)
	{
		waveform[v] = p;
		p += size;
	}

	if (region != -1)
		namco_wavedata = memory_region(region);

	/* We need waveform data. It fails if region is not specified. */
	if (namco_wavedata)
	{
		for (offset = 0; offset < 256; offset++)
			update_namco_waveform(offset, namco_wavedata[offset]);
	}

	return 0;
}


/* generate sound by oversampling */
static INLINE UINT32 namco_update_one(INT16 *buffer, int length, const INT16 *wave, UINT32 counter, UINT32 freq)
{
	while (length-- > 0)
	{
		INT16 data = 0;
		int i;

		for (i = 0; i < OVERSAMPLING_RATE; i++)
		{
			data += wave[WAVEFORM_POSITION(counter)];
			counter += freq;
		}

		*buffer++ += data;
	}

	return counter;
}


/* generate sound to the mix buffer in mono */
static void namco_update_mono(int ch, INT16 *buffer, int length)
{
	sound_channel *voice;

	/* zap the contents of the buffer */
	memset(buffer, 0, length * sizeof(INT16));

	/* if no sound, we're done */
	if (sound_enable == 0)
		return;

	/* loop over each voice and add its contribution */
	for (voice = channel_list; voice < last_channel; voice++)
	{
		INT16 *mix = buffer;
		int v = voice->volume[0];

		if (voice->noise_sw)
		{
			int f = voice->frequency & 0xff;

			/* only update if we have non-zero volume and frequency */
			if (v && f)
			{
				UINT32 delta = (f << (f_fracbits - 15 + 4)) * OVERSAMPLING_RATE;
				UINT32 c = voice->noise_counter;
				INT16 noise_data = OUTPUT_LEVEL(0x07 * (v >> 1) * OVERSAMPLING_RATE);
				int i;

				/* add our contribution */
				for (i = 0; i < length; i++)
				{
					int cnt;

					if (voice->noise_state)
						*mix++ += noise_data;
					else
						*mix++ -= noise_data;

					c += delta;
					cnt = (c >> 12);
					c &= (1 << 12) - 1;
					for( ;cnt > 0; cnt--)
					{
						if ((voice->noise_seed + 1) & 2) voice->noise_state ^= 1;
						if (voice->noise_seed & 1) voice->noise_seed ^= 0x28000;
						voice->noise_seed >>= 1;
					}
				}

				/* update the counter for this voice */
				voice->noise_counter = c;
			}
		}
		else
		{
			/* only update if we have non-zero volume and frequency */
			if (v && voice->frequency)
			{
				const INT16 *w = &waveform[v][voice->waveform_select * 32];

				/* generate sound into buffer and update the counter for this voice */
				voice->counter = namco_update_one(mix, length, w, voice->counter, voice->frequency);
			}
		}
	}
}


/* generate sound to the mix buffer in stereo */
static void namco_update_stereo(int ch, INT16 **buffer, int length)
{
	sound_channel *voice;

	/* zap the contents of the buffers */
	memset(buffer[0], 0, length * sizeof(INT16));
	memset(buffer[1], 0, length * sizeof(INT16));

	/* if no sound, we're done */
	if (sound_enable == 0)
		return;

	/* loop over each voice and add its contribution */
	for (voice = channel_list; voice < last_channel; voice++)
	{
		INT16 *lmix = buffer[0];
		INT16 *rmix = buffer[1];
		int lv = voice->volume[0];
		int rv = voice->volume[1];

		if (voice->noise_sw)
		{
			int f = voice->frequency & 0xff;

			/* only update if we have non-zero volume and frequency */
			if ((lv || rv) && f)
			{
				UINT32 delta = (f << (f_fracbits - 15 + 4)) * OVERSAMPLING_RATE;
				UINT32 c = voice->noise_counter;
				INT16 l_noise_data = OUTPUT_LEVEL(0x07 * (lv >> 1) * OVERSAMPLING_RATE);
				INT16 r_noise_data = OUTPUT_LEVEL(0x07 * (rv >> 1) * OVERSAMPLING_RATE);
				int i;

				/* add our contribution */
				for (i = 0; i < length; i++)
				{
					int cnt;

					if (voice->noise_state)
					{
						*lmix++ += l_noise_data;
						*rmix++ += r_noise_data;
					}
					else
					{
						*lmix++ -= l_noise_data;
						*rmix++ -= r_noise_data;
					}

					c += delta;
					cnt = (c >> 12);
					c &= (1 << 12) - 1;
					for( ;cnt > 0; cnt--)
					{
						if ((voice->noise_seed + 1) & 2) voice->noise_state ^= 1;
						if (voice->noise_seed & 1) voice->noise_seed ^= 0x28000;
						voice->noise_seed >>= 1;
					}
				}

				/* update the counter for this voice */
				voice->noise_counter = c;
			}
		}
		else
		{
			/* only update if we have non-zero frequency */
			if (voice->frequency)
			{
				/* save the counter for this voice */
				UINT32 c = voice->counter;

				/* only update if we have non-zero left volume */
				if (lv)
				{
					const INT16 *lw = &waveform[lv][voice->waveform_select * 32];

					/* generate sound into the buffer */
					c = namco_update_one(lmix, length, lw, voice->counter, voice->frequency);
				}

				/* only update if we have non-zero right volume */
				if (rv)
				{
					const INT16 *rw = &waveform[rv][voice->waveform_select * 32];

					/* generate sound into the buffer */
					c = namco_update_one(rmix, length, rw, voice->counter, voice->frequency);
				}

				/* update the counter for this voice */
				voice->counter = c;
			}
		}
	}
}


int namco_sh_start(const struct MachineSound *msound)
{
	sound_channel *voice;
	const struct namco_interface *intf = msound->sound_interface;
	int clock_multiple;

	/* extract globals from the interface */
	num_voices = intf->voices;
	last_channel = channel_list + num_voices;

	/* adjust internal clock */
	namco_clock = intf->samplerate;
	for (clock_multiple = 0; namco_clock < INTERNAL_RATE; clock_multiple++)
		namco_clock *= 2;

	f_fracbits = clock_multiple + 15;

	/* adjust output clock */
	sample_rate = namco_clock / OVERSAMPLING_RATE;

	logerror("Namco: freq fractional bits = %d: internal freq = %d, output freq = %d\n", f_fracbits, namco_clock, sample_rate);

	/* build the waveform table */
	if (build_decoded_waveform(intf->region))
		return 1;

	/* get stream channels */
	if (intf->stereo)
	{
		int vol[2];
		char buf[2][40];
		const char *name[2];

		name[0] = buf[0];
		sprintf(buf[0],"%s left",sound_name(msound));
		name[1] = buf[1];
		sprintf(buf[1],"%s right",sound_name(msound));
		vol[0] = MIXER(intf->volume,MIXER_PAN_LEFT);
		vol[1] = MIXER(intf->volume,MIXER_PAN_RIGHT);
		stream = stream_init_multi(2, name, vol, sample_rate, 0, namco_update_stereo);
	}
	else
	{
		stream = stream_init(sound_name(msound), intf->volume, sample_rate, 0, namco_update_mono);
	}

	/* start with sound enabled, many games don't have a sound enable register */
	sound_enable = 1;

	/* reset all the voices */
	for (voice = channel_list; voice < last_channel; voice++)
	{
		voice->frequency = 0;
		voice->volume[0] = voice->volume[1] = 0;
		voice->waveform_select = 0;
		voice->counter = 0;
		voice->noise_sw = 0;
		voice->noise_state = 0;
		voice->noise_seed = 1;
		voice->noise_counter = 0;
	}

	return 0;
}


void namco_sh_stop(void)
{
}


/********************************************************************************/

/* pengo register map
	0x05:		ch 0	waveform select
	0x0a:		ch 1	waveform select
	0x0f:		ch 2	waveform select

	0x10:		ch 0	the first voice has extra frequency bits
	0x11-0x14:	ch 0	frequency
	0x15:		ch 0	volume

	0x16-0x19:	ch 1	frequency
	0x1a:		ch 1	volume

	0x1b-0x1e:	ch 2	frequency
	0x1f:		ch 2	volume
*/

WRITE_HANDLER( pengo_sound_enable_w )
{
	sound_enable = data;
}

WRITE_HANDLER( pengo_sound_w )
{
	sound_channel *voice;
	int ch;

	data &= 0x0f;
	if (namco_soundregs[offset] == data)
		return;

	/* update the streams */
	stream_update(stream, 0);

	/* set the register */
	namco_soundregs[offset] = data;

	if (offset < 0x10)
		ch = (offset - 5) / 5;
	else if (offset == 0x10)
		ch = 0;
	else
		ch = (offset - 0x11) / 5;

	if (ch >= num_voices)
		return;

	/* recompute the voice parameters */
	voice = channel_list + ch;
	switch (offset - ch * 5)
	{
	case 0x05:
		voice->waveform_select = data & 7;
		break;

	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
		/* the frequency has 20 bits */
		/* the first voice has extra frequency bits */
		voice->frequency = (ch == 0) ? namco_soundregs[0x10] : 0;
		voice->frequency += (namco_soundregs[ch * 5 + 0x11] << 4);
		voice->frequency += (namco_soundregs[ch * 5 + 0x12] << 8);
		voice->frequency += (namco_soundregs[ch * 5 + 0x13] << 12);
		voice->frequency += (namco_soundregs[ch * 5 + 0x14] << 16);	/* always 0 */
		break;

	case 0x15:
		voice->volume[0] = data;
		break;
	}
}


/********************************************************************************/

/* polepos register map
Note: even if there are 8 voices, the game doesn't use the first 2 because
it select the 54XX/52XX outputs on those channels

	0x00-0x01	ch 0	frequency
	0x02		ch 0	xxxx---- GAIN 2 volume
	0x03		ch 0	xxxx---- GAIN 3 volume
	                    ----xxxx GAIN 4 volume

	0x04-0x07	ch 1

	.
	.
	.

	0x1c-0x1f	ch 7

	0x23		ch 0	xxxx---- GAIN 1 volume
	                    -----xxx waveform select
	                    ----x-xx channel output select
						         0-7 (all the same, shared with waveform select) = wave
								 8 = CHANL1 (54XX pins 17-20)
								 9 = CHANL2 (54XX pins 8-11)
								 A = CHANL3 (54XX pins 4-7)
								 B = CHANL4 (52XX)
	0x27		ch 1
	0x2b		ch 2
	0x2f		ch 3
	0x33		ch 4
	0x37		ch 5
	0x3b		ch 6
	0x3f		ch 7
*/

void polepos_sound_enable(int enable)
{
	sound_enable = enable;
}

WRITE_HANDLER( polepos_sound_w )
{
	sound_channel *voice;
	int ch;

	if (namco_soundregs[offset] == data)
		return;

	/* update the streams */
	stream_update(stream, 0);

	/* set the register */
	namco_soundregs[offset] = data;

	ch = (offset & 0x1f) / 4;

	/* recompute the voice parameters */
	voice = channel_list + ch;
	switch (offset & 0x23)
	{
	case 0x00:
	case 0x01:
		/* the frequency has 16 bits */
		voice->frequency = namco_soundregs[ch * 4 + 0x00];
		voice->frequency += namco_soundregs[ch * 4 + 0x01] << 8;
		break;

	case 0x23:
		voice->waveform_select = data & 7;
		/* fall through */
	case 0x02:
	case 0x03:
		voice->volume[0] = voice->volume[1] = 0;
		/* front speakers ? */
		voice->volume[0] += namco_soundregs[ch * 4 + 0x03] >> 4;
		voice->volume[1] += namco_soundregs[ch * 4 + 0x03] & 0x0f;
		/* rear speakers ? */
		voice->volume[0] += namco_soundregs[ch * 4 + 0x23] >> 4;
		voice->volume[1] += namco_soundregs[ch * 4 + 0x02] >> 4;

		voice->volume[0] /= 2;
		voice->volume[1] /= 2;

		/* if 54XX or 52XX selected, silence this voice */
		if (namco_soundregs[ch * 4 + 0x23] & 8)
			voice->volume[0] = voice->volume[1] = 0;
		break;
	}
}


/********************************************************************************/

/* 15XX register map
	0x03		ch 0	volume
	0x04-0x05	ch 0	frequency
	0x06		ch 0	waveform select & frequency

	0x0b		ch 1	volume
	0x0c-0x0d	ch 1	frequency
	0x0e		ch 1	waveform select & frequency

	.
	.
	.

	0x3b		ch 7	volume
	0x3c-0x3d	ch 7	frequency
	0x3e		ch 7	waveform select & frequency
*/


WRITE_HANDLER( mappy_sound_enable_w ) /*remove this when drivers are updated this is so our old drivers work */
{
	sound_enable = offset;
}

void mappy_sound_enable(int enable)
{
	sound_enable = enable;
}

WRITE_HANDLER( namco_15xx_w )
{
	sound_channel *voice;
	int ch;

	if (namco_soundregs[offset] == data)
		return;

	/* update the streams */
	stream_update(stream, 0);

	/* set the register */
	namco_soundregs[offset] = data;

	ch = offset / 8;
	if (ch >= num_voices)
		return;

	/* recompute the voice parameters */
	voice = channel_list + ch;
	switch (offset - ch * 8)
	{
	case 0x03:
		voice->volume[0] = data & 0x0f;
		break;

	case 0x06:
		voice->waveform_select = (data >> 4) & 7;
	case 0x04:
	case 0x05:
		/* the frequency has 20 bits */
		voice->frequency = namco_soundregs[ch * 8 + 0x04];
		voice->frequency += namco_soundregs[ch * 8 + 0x05] << 8;
		voice->frequency += (namco_soundregs[ch * 8 + 0x06] & 15) << 16;	/* high bits are from here */
		break;
	}
}

/********************************************************************************/

/* namcos1 register map
	0x00		ch 0	left volume
	0x01		ch 0	waveform select & frequency
	0x02-0x03	ch 0	frequency
	0x04		ch 0	right volume AND
	0x04		ch 1	noise sw

	0x08		ch 1	left volume
	0x09		ch 1	waveform select & frequency
	0x0a-0x0b	ch 1	frequency
	0x0c		ch 1	right volume AND
	0x0c		ch 2	noise sw

	.
	.
	.

	0x38		ch 7	left volume
	0x39		ch 7	waveform select & frequency
	0x3a-0x3b	ch 7	frequency
	0x3c		ch 7	right volume AND
	0x3c		ch 0	noise sw
*/

WRITE_HANDLER( namcos1_sound_w )
{
	sound_channel *voice;
	int ch;
	int nssw;

	/* verify the offset */
	if (offset > 63)
	{
		logerror("NAMCOS1 sound: Attempting to write past the 64 registers segment\n");
		return;
	}

	if (namco_soundregs[offset] == data)
		return;

	/* update the streams */
	stream_update(stream,0);

	/* set the register */
	namco_soundregs[offset] = data;

	ch = offset / 8;
	if (ch >= num_voices)
		return;

	/* recompute the voice parameters */
	voice = channel_list + ch;
	switch (offset - ch * 8)
	{
	case 0x00:
		voice->volume[0] = data & 0x0f;
		break;

	case 0x01:
		voice->waveform_select = (data >> 4) & 15;
	case 0x02:
	case 0x03:
		/* the frequency has 20 bits */
		voice->frequency = (namco_soundregs[ch * 8 + 0x01] & 15) << 16;	/* high bits are from here */
		voice->frequency += namco_soundregs[ch * 8 + 0x02] << 8;
		voice->frequency += namco_soundregs[ch * 8 + 0x03];
		break;

	case 0x04:
		voice->volume[1] = data & 0x0f;

		nssw = ((data & 0x80) >> 7);
		if (++voice == last_channel)
			voice = channel_list;
		voice->noise_sw = nssw;
		break;
	}
}

READ_HANDLER( namcos1_sound_r )
{
	return namco_soundregs[offset];
}

WRITE_HANDLER( namcos1_wavedata_w )
{
	if (namco_wavedata[offset] != data)
	{
		/* update the streams */
		stream_update(stream,0);

		namco_wavedata[offset] = data;

		/* update the decoded waveform table */
		update_namco_waveform(offset, data);
	}
}

READ_HANDLER( namcos1_wavedata_r )
{
	return namco_wavedata[offset];
}


/********************************************************************************/

WRITE_HANDLER( snkwave_w )
{
	static int freq0 = 0xff;
	sound_channel *voice = channel_list;
	if( offset==0 ) freq0 = data;
	if( offset==1 )
	{
		stream_update(stream, 0);
		if( data==0xff || freq0==0 )
		{
			voice->volume[0] = 0x0;
		}
		else
		{
			voice->volume[0] = 0x8;
			voice->frequency = (data<<16)/freq0;
		}
	}
}
