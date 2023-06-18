/******************************************************************************

     TMS5110 interface

     slightly modified from 5220intf by Jarek Burczynski

     Written for MAME by Frank Palazzolo
     With help from Neill Corlett
     Additional tweaking by Aaron Giles

******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "tms5110.h"


#define MAX_SAMPLE_CHUNK	10000

#define FRAC_BITS			14
#define FRAC_ONE			(1 << FRAC_BITS)
#define FRAC_MASK			(FRAC_ONE - 1)


/* the state of the streamed output */
static const struct TMS5110interface *intf;
static INT16 last_sample, curr_sample;
static UINT32 source_step;
static UINT32 source_pos;
static int stream;


/* static function prototypes */
static void tms5110_update(int ch, INT16 *buffer, int length);



/******************************************************************************

     tms5110_sh_start -- allocate buffers and reset the 5110

******************************************************************************/

int tms5110_sh_start(const struct MachineSound *msound)
{
    intf = msound->sound_interface;

    if (intf->M0_callback==NULL)
    {
      log_cb(RETRO_LOG_DEBUG, LOGPRE "\n file: 5110intf.c, tms5110_sh_start(), line 53:\n  Missing _mandatory_ 'M0_callback' function pointer in the TMS5110 interface\n  This function is used by TMS5110 to call for a single bits\n  needed to generate the speech\n  Aborting startup...\n");
      return 1;
    }
    tms5110_set_M0_callback( intf->M0_callback );

    /* reset the 5110 */
    tms5110_reset();

    /* set the initial frequency */
    stream = -1;
    tms5110_set_frequency(intf->baseclock);
    source_pos = 0;
    last_sample = curr_sample = 0;

	/* initialize a stream */
	stream = stream_init("TMS5110", intf->mixing_level, Machine->sample_rate, 0, tms5110_update);
	if (stream == -1)
		return 1;

    /* request a sound channel */
    return 0;
}



/******************************************************************************

     tms5110_sh_stop -- free buffers

******************************************************************************/

void tms5110_sh_stop(void)
{
}



/******************************************************************************

     tms5110_sh_update -- update the sound chip

******************************************************************************/

void tms5110_sh_update(void)
{
}



/******************************************************************************

     tms5110_CTL_w -- write Control Command to the sound chip
commands like Speech, Reset, etc., are loaded into the chip via the CTL pins

******************************************************************************/

WRITE_HANDLER( tms5110_CTL_w )
{
    /* bring up to date first */
    stream_update(stream, 0);
    tms5110_CTL_set(data);
}

/******************************************************************************

     tms5110_PDC_w -- write to PDC pin on the sound chip

******************************************************************************/

WRITE_HANDLER( tms5110_PDC_w )
{
    /* bring up to date first */
    stream_update(stream, 0);
    tms5110_PDC_set(data);
}



/******************************************************************************

     tms5110_status_r -- read status from the sound chip

******************************************************************************/

READ_HANDLER( tms5110_status_r )
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5110_status_read();
}



/******************************************************************************

     tms5110_ready_r -- return the not ready status from the sound chip

******************************************************************************/

int tms5110_ready_r(void)
{
    /* bring up to date first */
    stream_update(stream, 0);
    return tms5110_ready_read();
}



/******************************************************************************

     tms5110_update -- update the sound chip so that it is in sync with CPU execution

******************************************************************************/

static void tms5110_update(int ch, INT16 *buffer, int length)
{
	INT16 sample_data[MAX_SAMPLE_CHUNK], *curr_data = sample_data;
	INT16 prev = last_sample, curr = curr_sample;
	UINT32 final_pos;
	UINT32 new_samples;

	/* finish off the current sample */
	if (source_pos > 0)
	{
		/* interpolate */
		while (length > 0 && source_pos < FRAC_ONE)
		{
			*buffer++ = (((INT32)prev * (FRAC_ONE - source_pos)) + ((INT32)curr * source_pos)) >> FRAC_BITS;
			source_pos += source_step;
			length--;
		}

		/* if we're over, continue; otherwise, we're done */
		if (source_pos >= FRAC_ONE)
			source_pos -= FRAC_ONE;
		else
		{
			tms5110_process(sample_data, 0);
			return;
		}
	}

	/* compute how many new samples we need */
	final_pos = source_pos + length * source_step;
	new_samples = (final_pos + FRAC_ONE - 1) >> FRAC_BITS;
	if (new_samples > MAX_SAMPLE_CHUNK)
		new_samples = MAX_SAMPLE_CHUNK;

	/* generate them into our buffer */
	tms5110_process(sample_data, new_samples);
	prev = curr;
	curr = *curr_data++;

	/* then sample-rate convert with linear interpolation */
	while (length > 0)
	{
		/* interpolate */
		while (length > 0 && source_pos < FRAC_ONE)
		{
			*buffer++ = (((INT32)prev * (FRAC_ONE - source_pos)) + ((INT32)curr * source_pos)) >> FRAC_BITS;
			source_pos += source_step;
			length--;
		}

		/* if we're over, grab the next samples */
		if (source_pos >= FRAC_ONE)
		{
			source_pos -= FRAC_ONE;
			prev = curr;
			curr = *curr_data++;
		}
	}

	/* remember the last samples */
	last_sample = prev;
	curr_sample = curr;
}



/******************************************************************************

     tms5110_set_frequency -- adjusts the playback frequency

******************************************************************************/

void tms5110_set_frequency(int frequency)
{
	/* skip if output frequency is zero */
	if (!Machine->sample_rate)
		return;

	/* update the stream and compute a new step size */
	if (stream != -1)
		stream_update(stream, 0);
	source_step = (UINT32)((double)(frequency / 80) * (double)FRAC_ONE / (double)Machine->sample_rate);
}
