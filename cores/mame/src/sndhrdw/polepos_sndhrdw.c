/***************************************************************************
	polepos.c
	Sound handler
****************************************************************************/
#include "driver.h"

static int sample_msb = 0;
static int sample_lsb = 0;
static int sample_enable = 0;

static int sound_stream;

#define AMP(r)	(r*128/10100)
static int volume_table[8] =
{
	AMP(2200), AMP(3200), AMP(4400), AMP(5400),
	AMP(6900), AMP(7900), AMP(9100), AMP(10100)
};


/************************************/
/* Stream updater                   */
/************************************/
static void engine_sound_update(int num, INT16 *buffer, int length)
{
	static UINT32 current_position;
	UINT32 step, clock, slot, volume;
	UINT8 *base;


	/* if we're not enabled, just fill with 0 */
	if (!sample_enable || Machine->sample_rate == 0)
	{
		memset(buffer, 0, length * sizeof(INT16));
		return;
	}

	/* determine the effective clock rate */
	clock = (Machine->drv->cpu[0].cpu_clock / 16) * ((sample_msb + 1) * 64 + sample_lsb + 1) / (64*64);
	step = (clock << 12) / Machine->sample_rate;

	/* determine the volume */
	slot = (sample_msb >> 3) & 7;
	volume = volume_table[slot];
	base = &memory_region(REGION_SOUND2)[slot * 0x800];

	/* fill in the sample */
	while (length--)
	{
		*buffer++ = (base[(current_position >> 12) & 0x7ff] * volume);
		current_position += step;
	}
}

/************************************/
/* Sound handler start              */
/************************************/
int polepos_sh_start(const struct MachineSound *msound)
{
	sound_stream = stream_init("Engine Sound", 25, Machine->sample_rate, 0, engine_sound_update);
	sample_msb = sample_lsb = 0;
	sample_enable = 0;
    return 0;
}

/************************************/
/* Sound handler stop               */
/************************************/
void polepos_sh_stop(void)
{
}

/************************************/
/* Write LSB of engine sound		*/
/************************************/
WRITE_HANDLER( polepos_engine_sound_lsb_w )
{
	stream_update(sound_stream, 0);
	sample_lsb = data & 62;
    sample_enable = data & 1;
}

/************************************/
/* Write MSB of engine sound		*/
/************************************/
WRITE_HANDLER( polepos_engine_sound_msb_w )
{
	stream_update(sound_stream, 0);
	sample_msb = data & 63;
}
