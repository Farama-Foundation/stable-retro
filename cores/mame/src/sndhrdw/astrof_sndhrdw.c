
#include "driver.h"

/* Make sure that the sample name definitions in drivers/astrof.c matches these */

#define SAMPLE_FIRE		 0
#define SAMPLE_EKILLED	 1
#define SAMPLE_WAVE		 2
#define SAMPLE_BOSSFIRE  6
#define SAMPLE_FUEL		 7
#define SAMPLE_DEATH	 8
#define SAMPLE_BOSSHIT	 9
#define SAMPLE_BOSSKILL  10

#define CHANNEL_FIRE	  0
#define CHANNEL_EXPLOSION 1
#define CHANNEL_WAVE      2   /* Background humm */
#define CHANNEL_BOSSFIRE  2	  /* There is no background humm on the boss level */
#define CHANNEL_FUEL	  3


/* Make sure that the #define's in sndhrdw/astrof.c matches these */
static const char *astrof_sample_names[] =
{
	"*astrof",
	"fire.wav",
	"ekilled.wav",
	"wave1.wav",
	"wave2.wav",
	"wave3.wav",
	"wave4.wav",
	"bossfire.wav",
	"fuel.wav",
	"death.wav",
	"bosshit.wav",
	"bosskill.wav",
	0   /* end of array */
};

struct Samplesinterface astrof_samples_interface =
{
	4,	/* 4 channels */
	25,	/* volume */
	astrof_sample_names
};

#if 0
static const char *tomahawk_sample_names[] =
{
	"*tomahawk",
	/* We don't have these yet */
	0   /* end of array */
};
#endif

struct Samplesinterface tomahawk_samples_interface =
{
	1,	/* 1 channel for now */
	25,	/* volume */
	0,/*tomahawk_sample_names*/
};


static int start_explosion = 0;
static int death_playing = 0;
static int bosskill_playing = 0;

WRITE_HANDLER( astrof_sample1_w )
{
	static int last = 0;

	if (death_playing)
	{
		death_playing = sample_playing(CHANNEL_EXPLOSION);
	}

	if (bosskill_playing)
	{
		bosskill_playing = sample_playing(CHANNEL_EXPLOSION);
	}

	/* Bit 2 - Explosion trigger */
	if ((data & 0x04) && !(last & 0x04))
	{
		/* I *know* that the effect select port will be written shortly
		   after this one, so this works */
		start_explosion = 1;
	}

	/* Bit 0/1/3 - Background noise */
	if ((data & 0x08) != (last & 0x08))
	{
		if (data & 0x08)
		{
			int sample = SAMPLE_WAVE + (data & 3);
			sample_start(CHANNEL_WAVE,sample,1);
		}
		else
		{
			sample_stop(CHANNEL_WAVE);
		}
	}

	/* Bit 4 - Boss Laser */
	if ((data & 0x10) && !(last & 0x10))
	{
		if (!bosskill_playing)
		{
			sample_start(CHANNEL_BOSSFIRE,SAMPLE_BOSSFIRE,0);
		}
	}

	/* Bit 5 - Fire */
	if ((data & 0x20) && !(last & 0x20))
	{
		if (!bosskill_playing)
		{
			sample_start(CHANNEL_FIRE,SAMPLE_FIRE,0);
		}
	}

	/* Bit 6 - Don't know. Probably something to do with the explosion sounds */

	/* Bit 7 - Don't know. Maybe a global sound enable bit? */

	last = data;
}


WRITE_HANDLER( astrof_sample2_w )
{
	static int last = 0;

	/* Bit 0-2 Explosion select (triggered by Bit 2 of the other port */
	if (start_explosion)
	{
		if (data & 0x04)
		{
			/* This is really a compound effect, made up of I believe 3 sound
			   effects, but since our sample contains them all, disable playing
			   the other effects while the explosion is playing */
			if (!bosskill_playing)
			{
				sample_start(CHANNEL_EXPLOSION,SAMPLE_BOSSKILL,0);
				bosskill_playing = 1;
			}
		}
		else if (data & 0x02)
		{
			sample_start(CHANNEL_EXPLOSION,SAMPLE_BOSSHIT,0);
		}
		else if (data & 0x01)
		{
			sample_start(CHANNEL_EXPLOSION,SAMPLE_EKILLED,0);
		}
		else
		{
			if (!death_playing)
			{
				sample_start(CHANNEL_EXPLOSION,SAMPLE_DEATH,0);
				death_playing = 1;
			}
		}

		start_explosion = 0;
	}

	/* Bit 3 - Low Fuel Warning */
	if ((data & 0x08) && !(last & 0x08))
	{
		sample_start(CHANNEL_FUEL,SAMPLE_FUEL,0);
	}

	last = data;
}
