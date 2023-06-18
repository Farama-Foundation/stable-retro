/***************************************************************************
					Gaelco Sound Hardware

				By Manuel Abadia <manu@teleline.es>

CG-1V/GAE1 (Gaelco custom GFX & Sound chip):
	The CG-1V/GAE1 can handle up to 7 stereo channels.
	The chip output is connected to a TDA1543 (16 bit DAC).

Registers per channel:
======================
	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | xxxxxxxx xxxxxxxx | not used?
	  1  | xxxx---- -------- | left channel volume (0x00..0x0f)
	  1  | ----xxxx -------- | right channel volume (0x00..0x0f)
	  1  | -------- xxxx---- | sample type (0x0c = PCM 8 bits mono, 0x08 = PCM 8 bits stereo)
	  1  | -------- ----xxxx | ROM Bank
	  2  | xxxxxxxx xxxxxxxx | sample end position
	  3  | xxxxxxxx xxxxxxxx | remaining bytes to play

	  the following are used only when looping (usually used for music)

	  4  | xxxxxxxx xxxxxxxx | not used?
	  5  | xxxx---- -------- | left channel volume (0x00..0x0f)
	  5  | ----xxxx -------- | right channel volume (0x00..0x0f)
	  5  | -------- xxxx---- | sample type (0x0c = PCM 8 bits mono, 0x08 = PCM 8 bits stereo)
	  5  | -------- ----xxxx | ROM Bank
	  6  | xxxxxxxx xxxxxxxx | sample end position
	  7  | xxxxxxxx xxxxxxxx | remaining bytes to play

	The samples are played from (end position + length) to (end position)!

***************************************************************************/

#include "driver.h"
#include "gaelco.h"
#include "wavwrite.h"


/*#define LOG_SOUND 1*/
/*#define LOG_READ_WRITES 1*/
/*#define LOG_WAVE	1*/
/*#define ALT_MIX*/

#define GAELCO_NUM_CHANNELS 	0x07
#define VOLUME_LEVELS 			0x10

data16_t *gaelco_sndregs;

/* table for converting from 8 to 16 bits with volume control */
INT16 volume_table[VOLUME_LEVELS][256];

/* this structure defines a channel */
struct gaelcosnd_channel
{
	int active;			/* is it playing? */
	int loop;			/* = 0 no looping, = 1 looping */
	int chunkNum;		/* current chunk if looping */
};

/* this structure defines the Gaelco custom sound chip */
struct GAELCOSND
{
	int stream;												/* our stream */
	UINT8 *snd_data;										/* PCM data */
	int banks[4];											/* start of each ROM bank */
	struct gaelcosnd_channel channel[GAELCO_NUM_CHANNELS];	/* 7 stereo channels */
};

/* Gaelco CG-1V/GAE1 chip */
static struct GAELCOSND gaelcosnd;

#ifdef LOG_WAVE
void *	wavraw;					/* raw waveform */
#endif

/*============================================================================
						CG-1V/GAE1 Sound Update

			Writes length bytes to the sound buffer
  ============================================================================*/

static void gaelco_sh_update(int param, INT16 **buffer, int length)
{
	int j, ch;

    /* fill all data needed */
	for(j = 0; j < length; j++){
		int output_l = 0, output_r = 0;

		/* for each channel */
		for (ch = 0; ch < GAELCO_NUM_CHANNELS; ch ++){
			int ch_data_l = 0, ch_data_r = 0;
			struct gaelcosnd_channel *channel = &gaelcosnd.channel[ch];

			/* if the channel is playing */
			if (channel->active == 1){
				int data, chunkNum = 0;
				int base_offset, type, bank, vol_r, vol_l, end_pos;

				/* if the channel is looping, get current chunk to play */
				if (channel->loop == 1){
					chunkNum = channel->chunkNum;
				}

				base_offset = ch*8 + chunkNum*4;

				/* get channel parameters */
				type = ((gaelco_sndregs[base_offset + 1] >> 4) & 0x0f);
				bank = gaelcosnd.banks[((gaelco_sndregs[base_offset + 1] >> 0) & 0x03)];
				vol_l = ((gaelco_sndregs[base_offset + 1] >> 12) & 0x0f);
				vol_r = ((gaelco_sndregs[base_offset + 1] >> 8) & 0x0f);
				end_pos = gaelco_sndregs[base_offset + 2] << 8;

				/* generates output data (range 0x00000..0xffff) */
				if (type == 0x08){
					/* PCM, 8 bits mono */
					data = gaelcosnd.snd_data[bank + end_pos + gaelco_sndregs[base_offset + 3]];
					ch_data_l = volume_table[vol_l][data];
					ch_data_r = volume_table[vol_r][data];

					gaelco_sndregs[base_offset + 3]--;
				} else if (type == 0x0c){
					/* PCM, 8 bits stereo */
					data = gaelcosnd.snd_data[bank + end_pos + gaelco_sndregs[base_offset + 3]];
					ch_data_l = volume_table[vol_l][data];

					gaelco_sndregs[base_offset + 3]--;

					if (gaelco_sndregs[base_offset + 3] > 0){
						data = gaelcosnd.snd_data[bank + end_pos + gaelco_sndregs[base_offset + 3]];
						ch_data_r = volume_table[vol_r][data];

						gaelco_sndregs[base_offset + 3]--;
					}
				} else {
          log_cb(RETRO_LOG_DEBUG, LOGPRE "(GAE1) Playing unknown sample format in channel: %02d, type: %02x, bank: %02x, end: %08x, Length: %04x\n", ch, type, bank, end_pos, gaelco_sndregs[base_offset + 3]);

					channel->active = 0;
				}

				/* check if the current sample has finished playing */
				if (gaelco_sndregs[base_offset + 3] == 0){
					if (channel->loop == 0){	/* if no looping, we're done */
						channel->active = 0;
					} else {					/* if we're looping, swap chunks */
						channel->chunkNum = (channel->chunkNum + 1) & 0x01;

						/* if the length of the next chunk is 0, we're done */
						if (gaelco_sndregs[ch*8 + channel->chunkNum*4 + 3] == 0){
							channel->active = 0;
						}
					}
				}
			}

			/* add the contribution of this channel to the current data output */
			output_l += ch_data_l;
			output_r += ch_data_r;
		}

#ifndef ALT_MIX
		/* clip to max or min value */
      MAME_CLAMP_SAMPLE(output_l);
      MAME_CLAMP_SAMPLE(output_r);
#else
		/* ponderate channels */
		output_l /= GAELCO_NUM_CHANNELS;
		output_r /= GAELCO_NUM_CHANNELS;
#endif

		/* now that we have computed all channels, save current data to the output buffer */
		buffer[0][j] = output_l;
		buffer[1][j] = output_r;
	}

#ifdef LOG_WAVE
	wav_add_data_16lr(wavraw, buffer[0], buffer[1], length);
#endif
}

/*============================================================================
						CG-1V/GAE1 Read Handler
  ============================================================================*/

READ16_HANDLER( gaelcosnd_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: (GAE1): read from %04x\n", activecpu_get_pc(), offset);

	return gaelco_sndregs[offset];
}

/*============================================================================
						CG-1V/GAE1 Write Handler
  ============================================================================*/

WRITE16_HANDLER( gaelcosnd_w )
{
	struct gaelcosnd_channel *channel = &gaelcosnd.channel[offset >> 3];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%06x: (GAE1): write %04x to %04x\n", activecpu_get_pc(), data, offset);

	/* first update the stream to this point in time */
	stream_update(gaelcosnd.stream, 0);

	COMBINE_DATA(&gaelco_sndregs[offset]);

	switch(offset & 0x07){
		case 0x03:
			/* trigger sound */
			if ((gaelco_sndregs[offset - 1] != 0) && (data != 0)){
				if (!channel->active){
					channel->active = 1;
					channel->chunkNum = 0;
					channel->loop = 0;
          log_cb(RETRO_LOG_DEBUG, LOGPRE "(GAE1) Playing sample channel: %02d, type: %02x, bank: %02x, end: %08x, Length: %04x\n", offset >> 3, (gaelco_sndregs[offset - 2] >> 4) & 0x0f, gaelco_sndregs[offset - 2] & 0x03, gaelco_sndregs[offset - 1] << 8, data);
				}
			} else {
				channel->active = 0;
			}

			break;

		case 0x07: /* enable/disable looping */
			if ((gaelco_sndregs[offset - 1] != 0) && (data != 0)){
        log_cb(RETRO_LOG_DEBUG, LOGPRE "(GAE1) Looping in channel: %02d, type: %02x, bank: %02x, end: %08x, Length: %04x\n", offset >> 3, (gaelco_sndregs[offset - 2] >> 4) & 0x0f, gaelco_sndregs[offset - 2] & 0x03, gaelco_sndregs[offset - 1] << 8, data);
				channel->loop = 1;
			} else {
				channel->loop = 0;
			}

			break;
	}
}

/*============================================================================
						CG-1V/GAE1 Init
  ============================================================================*/

int gaelcosnd_sh_start(const struct MachineSound *msound, int chip)
{
	int j, vol;
	const struct gaelcosnd_interface *intf = msound->sound_interface;

	int volume[2];
	char buf[2][64];
	const char *name[2];

	/* bag on a 0 sample_rate */
	if (Machine->sample_rate == 0)	return 0;

	/* init chip state */
	memset(&gaelcosnd, 0, sizeof(struct GAELCOSND));

	/* allocate one stereo stream */
	for (j = 0; j < 2; j++){
		sprintf(buf[j], chip ? "CG-1V Channel #%d" : "GAE1 Channel #%d", j);
		name[j] = buf[j];
		volume[j] = MIXER(intf->volume[j], j ? MIXER_PAN_RIGHT : MIXER_PAN_LEFT);
	}

	/* copy rom banks */
	for (j = 0; j < 4; j++){
		gaelcosnd.banks[j] = intf->banks[j];
	}
	gaelcosnd.stream = stream_init_multi(2, name, volume, 8000, 0, gaelco_sh_update);
	gaelcosnd.snd_data = (UINT8 *)memory_region(intf->region);

	/* init volume table */
	for (vol = 0; vol < VOLUME_LEVELS; vol++){
		for (j = -128; j <= 127; j++){
			volume_table[vol][(j ^ 0x80) & 0xff] = (vol*j*256)/(VOLUME_LEVELS - 1);
		}
	}

#ifdef LOG_WAVE
	wavraw = wav_open("gae1_snd.wav", 8000, 2);
#endif

	return 0;
}

int gaelco_gae1_sh_start(const struct MachineSound *msound)
{
	return gaelcosnd_sh_start(msound, 0);
}

int gaelco_cg1v_sh_start(const struct MachineSound *msound)
{
	return gaelcosnd_sh_start(msound, 1);
}


void gaelcosnd_sh_stop(void)
{
#ifdef LOG_WAVE
	wav_close(wavraw);
#endif
}
