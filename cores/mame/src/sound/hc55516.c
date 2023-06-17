#include "driver.h"
#include <math.h>


#define	INTEGRATOR_LEAK_TC		0.001
#define	FILTER_DECAY_TC			0.004
#define	FILTER_CHARGE_TC		0.004
#define	FILTER_MIN				0.0416
#define	FILTER_MAX				1.0954
#define	SAMPLE_GAIN				10000.0


struct hc55516_data
{
	INT8 	channel;
	UINT8	last_clock;
	UINT8	databit;
	UINT8	shiftreg;

	INT16	curr_value;
	INT16	next_value;

	UINT32	update_count;

	double 	filter;
	double	integrator;
};


static struct hc55516_data hc55516[MAX_HC55516];
static double charge, decay, leak;


static void hc55516_update(int num, INT16 *buffer, int length);



int hc55516_sh_start(const struct MachineSound *msound)
{
	const struct hc55516_interface *intf = msound->sound_interface;
	int i;

	/* compute the fixed charge, decay, and leak time constants */
	charge = pow(exp(-1), 1.0 / (FILTER_CHARGE_TC * 16000.0));
	decay = pow(exp(-1), 1.0 / (FILTER_DECAY_TC * 16000.0));
	leak = pow(exp(-1), 1.0 / (INTEGRATOR_LEAK_TC * 16000.0));

	/* loop over HC55516 chips */
	for (i = 0; i < intf->num; i++)
	{
		struct hc55516_data *chip = &hc55516[i];
		char name[40];

		/* reset the channel */
		memset(chip, 0, sizeof(*chip));

		/* create the stream */
		sprintf(name, "HC55516 #%d", i);
		chip->channel = stream_init(name, intf->volume[i] & 0xff, Machine->sample_rate, i, hc55516_update);

		/* bail on fail */
		if (chip->channel == -1)
			return 1;
	}

	/* success */
	return 0;
}


void hc55516_update(int num, INT16 *buffer, int length)
{
	struct hc55516_data *chip = &hc55516[num];
	INT32 data, slope;
	int i;

	/* zero-length? bail */
	if (length == 0)
		return;

	/* track how many samples we've updated without a clock */
	chip->update_count += length;
	if (chip->update_count > Machine->sample_rate / 32)
	{
		chip->update_count = Machine->sample_rate;
		chip->next_value = 0;
	}

	/* compute the interpolation slope */
	data = chip->curr_value;
	slope = ((INT32)chip->next_value - data) / length;
	chip->curr_value = chip->next_value;

	/* reset the sample count */
	for (i = 0; i < length; i++, data += slope)
		*buffer++ = data;
}


void hc55516_clock_w(int num, int state)
{
	struct hc55516_data *chip = &hc55516[num];
	int clock = state & 1, diffclock;

	/* update the clock */
	diffclock = clock ^ chip->last_clock;
	chip->last_clock = clock;

	/* speech clock changing (active on rising edge) */
	if (diffclock && clock)
	{
		double integrator = chip->integrator, temp;

		/* clear the update count */
		chip->update_count = 0;

		/* move the estimator up or down a step based on the bit */
		if (chip->databit)
		{
			chip->shiftreg = ((chip->shiftreg << 1) | 1) & 7;
			integrator += chip->filter;
		}
		else
		{
			chip->shiftreg = (chip->shiftreg << 1) & 7;
			integrator -= chip->filter;
		}

		/* simulate leakage */
		integrator *= leak;

		/* if we got all 0's or all 1's in the last n bits, bump the step up */
		if (chip->shiftreg == 0 || chip->shiftreg == 7)
		{
			chip->filter = FILTER_MAX - ((FILTER_MAX - chip->filter) * charge);
			if (chip->filter > FILTER_MAX)
				chip->filter = FILTER_MAX;
		}

		/* simulate decay */
		else
		{
			chip->filter *= decay;
			if (chip->filter < FILTER_MIN)
				chip->filter = FILTER_MIN;
		}

		/* compute the sample as a 32-bit word */
		temp = integrator * SAMPLE_GAIN;
		chip->integrator = integrator;

		/* compress the sample range to fit better in a 16-bit word */
		if (temp < 0)
			chip->next_value = (int)(temp / (-temp * (1.0 / 32768.0) + 1.0));
		else
			chip->next_value = (int)(temp / (temp * (1.0 / 32768.0) + 1.0));

		/* update the output buffer before changing the registers */
		stream_update(chip->channel, 0);
	}
}


void hc55516_digit_w(int num, int data)
{
	hc55516[num].databit = data & 1;
}


void hc55516_clock_clear_w(int num, int data)
{
	hc55516_clock_w(num, 0);
}


void hc55516_clock_set_w(int num, int data)
{
	hc55516_clock_w(num, 1);
}


void hc55516_digit_clock_clear_w(int num, int data)
{
	hc55516[num].databit = data & 1;
	hc55516_clock_w(num, 0);
}


WRITE_HANDLER( hc55516_0_digit_w )	{ hc55516_digit_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_w )	{ hc55516_clock_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_clear_w )	{ hc55516_clock_clear_w(0,data); }
WRITE_HANDLER( hc55516_0_clock_set_w )		{ hc55516_clock_set_w(0,data); }
WRITE_HANDLER( hc55516_0_digit_clock_clear_w )	{ hc55516_digit_clock_clear_w(0,data); }

WRITE_HANDLER( hc55516_1_digit_w ) { hc55516_digit_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_w ) { hc55516_clock_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_clear_w ) { hc55516_clock_clear_w(1,data); }
WRITE_HANDLER( hc55516_1_clock_set_w )  { hc55516_clock_set_w(1,data); }
WRITE_HANDLER( hc55516_1_digit_clock_clear_w ) { hc55516_digit_clock_clear_w(1,data); }
