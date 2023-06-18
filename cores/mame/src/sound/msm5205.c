/*
 *   streaming ADPCM driver
 *   by Aaron Giles
 *
 *   Library to transcode from an ADPCM source to raw PCM.
 *   Written by Buffoni Mirko in 08/06/97
 *   References: various sources and documents.
 *
 *	 HJB 08/31/98
 *	 modified to use an automatically selected oversampling factor
 *	 for the current Machine->sample_rate
 *
 *	 01/06/99
 *	separate MSM5205 emulator form adpcm.c and some fix
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "msm5205.h"

/*
 * ADPCM lockup tabe
 */

/* step size index shift table */
static int index_shift[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* lookup table for the precomputed difference */
static int diff_lookup[49*16];

/*
 *   Compute the difference table
 */

static void ComputeTables (void)
{
	/* nibble to bit map */
	static int nbl2bit[16][4] =
	{
		{ 1, 0, 0, 0}, { 1, 0, 0, 1}, { 1, 0, 1, 0}, { 1, 0, 1, 1},
		{ 1, 1, 0, 0}, { 1, 1, 0, 1}, { 1, 1, 1, 0}, { 1, 1, 1, 1},
		{-1, 0, 0, 0}, {-1, 0, 0, 1}, {-1, 0, 1, 0}, {-1, 0, 1, 1},
		{-1, 1, 0, 0}, {-1, 1, 0, 1}, {-1, 1, 1, 0}, {-1, 1, 1, 1}
	};

	int step, nib;

	/* loop over all possible steps */
	for (step = 0; step <= 48; step++)
	{
		/* compute the step value */
		int stepval = floor (16.0 * pow (11.0 / 10.0, (double)step));

		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			diff_lookup[step*16 + nib] = nbl2bit[nib][0] *
				(stepval   * nbl2bit[nib][1] +
				 stepval/2 * nbl2bit[nib][2] +
				 stepval/4 * nbl2bit[nib][3] +
				 stepval/8);
		}
	}
}

/*
 *
 *	MSM 5205 ADPCM chip:
 *
 *	Data is streamed from a CPU by means of a clock generated on the chip.
 *
 *	A reset signal is set high or low to determine whether playback (and interrupts) are occuring
 *
 */

struct MSM5205Voice
{
	int stream;             /* number of stream system      */
	void *timer;              /* VCLK callback timer          */
	int data;               /* next adpcm data              */
	int vclk;               /* vclk signal (external mode)  */
	int reset;              /* reset pin signal             */
	int prescaler;          /* prescaler selector S1 and S2 */
	int bitwidth;           /* bit width selector -3B/4B    */
	int signal;             /* current ADPCM signal         */
	int step;               /* current ADPCM step           */
};

static const struct MSM5205interface *msm5205_intf;
static struct MSM5205Voice msm5205[MAX_MSM5205];

/* stream update callbacks */
static void MSM5205_update(int chip,INT16 *buffer,int length)
{
	struct MSM5205Voice *voice = &msm5205[chip];

	/* if this voice is active */
	if(voice->signal)
	{
		short val = voice->signal * 16;
		while (length)
		{
			*buffer++ = val;
			length--;
		}
	}
	else
		memset (buffer,0,length*sizeof(INT16));
}

/* timer callback at VCLK low eddge */
static void MSM5205_vclk_callback(int num)
{
	struct MSM5205Voice *voice = &msm5205[num];
	int val;
	int new_signal;
	/* callback user handler and latch next data */
	if(msm5205_intf->vclk_callback[num]) (*msm5205_intf->vclk_callback[num])(num);

	/* reset check at last hieddge of VCLK */
	if(voice->reset)
	{
		new_signal = 0;
		voice->step = 0;
	}
	else
	{
		/* update signal */
		/* !! MSM5205 has internal 12bit decoding, signal width is 0 to 8191 !! */
		val = voice->data;
		new_signal = voice->signal + diff_lookup[voice->step * 16 + (val & 15)];
		if (new_signal > 2047) new_signal = 2047;
		else if (new_signal < -2048) new_signal = -2048;
		voice->step += index_shift[val & 7];
		if (voice->step > 48) voice->step = 48;
		else if (voice->step < 0) voice->step = 0;
	}
	/* update when signal changed */
	if( voice->signal != new_signal)
	{
		stream_update(voice->stream,0);
		voice->signal = new_signal;
	}
}
/*
 *    Start emulation of an MSM5205-compatible chip
 */

int MSM5205_sh_start (const struct MachineSound *msound)
{
	int i;

	/* save a global pointer to our interface */
	msm5205_intf = msound->sound_interface;

	/* compute the difference tables */
	ComputeTables ();

	/* initialize the voices */
	memset (msm5205, 0, sizeof (msm5205));

	/* stream system initialize */
	for (i = 0;i < msm5205_intf->num;i++)
	{
		struct MSM5205Voice *voice = &msm5205[i];
		char name[20];
		sprintf(name,"MSM5205 #%d",i);
		voice->stream = stream_init(name,msm5205_intf->mixing_level[i],
                                Machine->sample_rate,i,
		                        MSM5205_update);
		voice->timer = timer_alloc(MSM5205_vclk_callback);
	}
	/* initialize */
	MSM5205_sh_reset();
	/* success */
	return 0;
}

/*
 *    Stop emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_stop (void)
{
}

/*
 *    Update emulation of an MSM5205-compatible chip
 */

void MSM5205_sh_update (void)
{
}


/*
 *    Reset emulation of an MSM5205-compatible chip
 */
void MSM5205_sh_reset(void)
{
	int i;

	/* bail if we're not emulating sound */
	if (Machine->sample_rate == 0)
		return;

	for (i = 0; i < msm5205_intf->num; i++)
	{
		struct MSM5205Voice *voice = &msm5205[i];
		/* initialize work */
		voice->data    = 0;
		voice->vclk    = 0;
		voice->reset   = 0;
		voice->signal  = 0;
		voice->step    = 0;
		/* timer and bitwidth set */
		MSM5205_playmode_w(i,msm5205_intf->select[i]);
	}
}

/*
 *    Handle an update of the vclk status of a chip (1 is reset ON, 0 is reset OFF)
 *    This function can use selector = MSM5205_SEX only
 */
void MSM5205_vclk_w (int num, int vclk)
{
	/* range check the numbers */
	if (num >= msm5205_intf->num)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "error: MSM5205_vclk_w() called with chip = %d, but only %d chips allocated\n", num, msm5205_intf->num);
		return;
	}
	if( msm5205[num].prescaler != 0 )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "error: MSM5205_vclk_w() called with chip = %d, but VCLK selected master mode\n", num);
	}
	else
	{
		if( msm5205[num].vclk != vclk)
		{
			msm5205[num].vclk = vclk;
			if( !vclk ) MSM5205_vclk_callback(num);
		}
	}
}

/*
 *    Handle an update of the reset status of a chip (1 is reset ON, 0 is reset OFF)
 */

void MSM5205_reset_w (int num, int reset)
{
	/* range check the numbers */
	if (num >= msm5205_intf->num)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "error: MSM5205_reset_w() called with chip = %d, but only %d chips allocated\n", num, msm5205_intf->num);
		return;
	}
	msm5205[num].reset = reset;
}

/*
 *    Handle an update of the data to the chip
 */

void MSM5205_data_w (int num, int data)
{
	if( msm5205[num].bitwidth == 4)
		msm5205[num].data = data & 0x0f;
	else
		msm5205[num].data = (data & 0x07)<<1; /* unknown */
}

/*
 *    Handle an change of the selector
 */

void MSM5205_playmode_w(int num,int select)
{
	struct MSM5205Voice *voice = &msm5205[num];
	static int prescaler_table[4] = {96,48,64,0};
	int prescaler = prescaler_table[select & 3];
	int bitwidth = (select & 4) ? 4 : 3;


	if( voice->prescaler != prescaler )
	{
		stream_update(voice->stream,0);

		voice->prescaler = prescaler;
		/* timer set */
		if( prescaler )
		{
			double period = TIME_IN_HZ(msm5205_intf->baseclock / prescaler);
			timer_adjust(voice->timer, period, num, period);
		}
		else
			timer_adjust(voice->timer, TIME_NEVER, 0, 0);
	}

	if( voice->bitwidth != bitwidth )
	{
		stream_update(voice->stream,0);

		voice->bitwidth = bitwidth;
	}
}


void MSM5205_set_volume(int num,int volume)
{
	struct MSM5205Voice *voice = &msm5205[num];

	mixer_set_volume(voice->stream,volume);
}
