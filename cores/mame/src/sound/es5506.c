/**********************************************************************************************
 *
 *   Ensoniq ES5505/6 driver
 *   by Aaron Giles
 *
 **********************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "adpcm.h"



/**********************************************************************************************

     CONSTANTS

***********************************************************************************************/

#define BACKEND_INTERPOLATE		1
/*#define LOG_COMMANDS			0*/
#define RAINE_CHECK				0
#define MAKE_WAVS				0

#if MAKE_WAVS
#include "wavwrite.h"
#endif


#define MAX_SAMPLE_CHUNK		10000
#define ULAW_MAXBITS			8

#define FRAC_BITS				14
#define FRAC_ONE				(1 << FRAC_BITS)
#define FRAC_MASK				(FRAC_ONE - 1)


#define CONTROL_BS1				0x8000
#define CONTROL_BS0				0x4000
#define CONTROL_CMPD			0x2000
#define CONTROL_CA2				0x1000
#define CONTROL_CA1				0x0800
#define CONTROL_CA0				0x0400
#define CONTROL_LP4				0x0200
#define CONTROL_LP3				0x0100
#define CONTROL_IRQ				0x0080
#define CONTROL_DIR				0x0040
#define CONTROL_IRQE			0x0020
#define CONTROL_BLE				0x0010
#define CONTROL_LPE				0x0008
#define CONTROL_LEI				0x0004
#define CONTROL_STOP1			0x0002
#define CONTROL_STOP0			0x0001

#define CONTROL_BSMASK			(CONTROL_BS1 | CONTROL_BS0)
#define CONTROL_CAMASK			(CONTROL_CA2 | CONTROL_CA1 | CONTROL_CA0)
#define CONTROL_LPMASK			(CONTROL_LP4 | CONTROL_LP3)
#define CONTROL_LOOPMASK		(CONTROL_BLE | CONTROL_LPE)
#define CONTROL_STOPMASK		(CONTROL_STOP1 | CONTROL_STOP0)



/**********************************************************************************************

     INTERNAL DATA STRUCTURES

***********************************************************************************************/

/* struct describing a single playing voice */
struct ES5506Voice
{
	/* external state */
	UINT32		control;				/* control register */
	UINT32		freqcount;				/* frequency count register */
	UINT32		start;					/* start register */
	UINT32		lvol;					/* left volume register */
	UINT32		end;					/* end register */
	UINT32		lvramp;					/* left volume ramp register */
	UINT32		accum;					/* accumulator register */
	UINT32		rvol;					/* right volume register */
	UINT32		rvramp;					/* right volume ramp register */
	UINT32		ecount;					/* envelope count register */
	UINT32		k2;						/* k2 register */
	UINT32		k2ramp;					/* k2 ramp register */
	UINT32		k1;						/* k1 register */
	UINT32		k1ramp;					/* k1 ramp register */
	INT32		o4n1;					/* filter storage O4(n-1) */
	INT32		o3n1;					/* filter storage O3(n-1) */
	INT32		o3n2;					/* filter storage O3(n-2) */
	INT32		o2n1;					/* filter storage O2(n-1) */
	INT32		o2n2;					/* filter storage O2(n-2) */
	INT32		o1n1;					/* filter storage O1(n-1) */
	UINT32		exbank;					/* external address bank */

	/* internal state */
	UINT8		index;					/* index of this voice */
	UINT8		filtcount;				/* filter count */
};

struct ES5506Chip
{
	int			stream;					/* which stream are we using */
	UINT16 *	region_base[4];			/* pointer to the base of the region */
	UINT32 		write_latch;			/* currently accumulated data for write */
	UINT32 		read_latch;				/* currently accumulated data for read */
	double 		master_clock;			/* master clock frequency */
	void 		(*irq_callback)(int);	/* IRQ callback */
	UINT16		(*port_read)(void);		/* input port read */

	UINT8 		current_page;			/* current register page */
	UINT8		active_voices;			/* number of active voices */
	UINT8		mode;					/* MODE register */
	UINT8		wst;					/* W_ST register */
	UINT8		wend;					/* W_END register */
	UINT8		lrend;					/* LR_END register */
	UINT8		irqv;					/* IRQV register */

	INT32		output_step;			/* step value for frequency conversion */
	INT32		output_pos;				/* current fractional position */
	INT32		last_lsample;			/* last sample output */
	INT32		last_rsample;			/* last sample output */
	INT32		curr_lsample;			/* current sample target */
	INT32		curr_rsample;			/* current sample target */

	struct 		ES5506Voice voice[32];	/* the 32 voices */

#if MAKE_WAVS
	void *		wavraw;					/* raw waveform */
	void *		wavresample;			/* resampled waveform */
#endif
};



/**********************************************************************************************

     GLOBALS

***********************************************************************************************/

static struct ES5506Chip es5506[MAX_ES5506];
static INT32 *accumulator;
static INT32 *scratch;

static INT16 *ulaw_lookup;
static UINT16 *volume_lookup;
static UINT32 accum_mask;

/*static FILE *eslog;*/



/**********************************************************************************************

     update_irq_state -- update the IRQ state

***********************************************************************************************/

static void update_irq_state(struct ES5506Chip *chip)
{
	/* ES5505/6 irq line has been set high - inform the host */
	if (chip->irq_callback)
		(*chip->irq_callback)(1); /* IRQB set high */
}

static void update_internal_irq_state(struct ES5506Chip *chip)
{
	/*	Host (cpu) has just read the voice interrupt vector (voice IRQ ack).
	
		Reset the voice vector to show the IRQB line is low (top bit set).
		If we have any stacked interrupts (other voices waiting to be
		processed - with their IRQ bit set) then they will be moved into
		the vector next time the voice is processed.  In emulation
		terms they get updated next time generate_samples() is called.	
	*/

	chip->irqv=0x80;

	if (chip->irq_callback)
		(*chip->irq_callback)(0); /* IRQB set low */
}

/**********************************************************************************************

     compute_tables -- compute static tables

***********************************************************************************************/

static int compute_tables(void)
{
	int i;

	/* allocate ulaw lookup table */
	if (!ulaw_lookup)
		ulaw_lookup = malloc(sizeof(ulaw_lookup[0]) << ULAW_MAXBITS);
	if (!ulaw_lookup)
		return 0;

	/* generate ulaw lookup table */
	for (i = 0; i < (1 << ULAW_MAXBITS); i++)
	{
		UINT16 rawval = (i << (16 - ULAW_MAXBITS)) | (1 << (15 - ULAW_MAXBITS));
		UINT8 exponent = rawval >> 13;
		UINT32 mantissa = (rawval << 3) & 0xffff;

		if (exponent == 0)
			ulaw_lookup[i] = (INT16)mantissa >> 7;
		else
		{
			mantissa = (mantissa >> 1) | (~mantissa & 0x8000);
			ulaw_lookup[i] = (INT16)mantissa >> (7 - exponent);
		}
	}

	/* allocate volume lookup table */
	if (!volume_lookup)
		volume_lookup = malloc(sizeof(volume_lookup[0]) * 4096);
	if (!volume_lookup)
		return 0;

	/* generate ulaw lookup table */
	for (i = 0; i < 4096; i++)
	{
		UINT8 exponent = i >> 8;
		UINT32 mantissa = (i & 0xff) | 0x100;

		volume_lookup[i] = (mantissa << 11) >> (20 - exponent);
	}

	return 1;
}



/**********************************************************************************************

     interpolate
     backend_interpolate -- interpolate between two samples

***********************************************************************************************/

#define interpolate(sample1, sample2, accum)										\
		(sample1 * (INT32)(0x800 - (voice->accum & 0x7ff)) + 						\
		 sample2 * (INT32)(voice->accum & 0x7ff)) >> 11;

#ifdef BACKEND_INTERPOLATE
#define backend_interpolate(sample1, sample2, position)								\
		(sample1 * (INT32)(FRAC_ONE - position) + 									\
		 sample2 * (INT32)position) >> FRAC_BITS;
#else
#define backend_interpolate(sample1, sample2, position)	sample1
#endif



/**********************************************************************************************

     apply_filters -- apply the 4-pole digital filter to the sample

***********************************************************************************************/

#define apply_filters(voice, sample)															\
do																								\
{																								\
	/* pole 1 is always low-pass using K1 */													\
	sample = ((INT32)(voice->k1 >> 2) * (sample - voice->o1n1) / 16384) + voice->o1n1;			\
	voice->o1n1 = sample;																		\
																								\
	/* pole 2 is always low-pass using K1 */													\
	sample = ((INT32)(voice->k1 >> 2) * (sample - voice->o2n1) / 16384) + voice->o2n1;			\
	voice->o2n2 = voice->o2n1;																	\
	voice->o2n1 = sample;																		\
																								\
	/* remaining poles depend on the current filter setting */									\
	switch (voice->control & CONTROL_LPMASK)													\
	{																							\
		case 0:																					\
			/* pole 3 is high-pass using K2 */													\
			sample = sample - voice->o2n2 + ((INT32)(voice->k2 >> 2) * voice->o3n1) / 32768 + voice->o3n1 / 2; \
			voice->o3n2 = voice->o3n1;															\
			voice->o3n1 = sample;																\
																								\
			/* pole 4 is high-pass using K2 */													\
			sample = sample - voice->o3n2 + ((INT32)(voice->k2 >> 2) * voice->o4n1) / 32768 + voice->o4n1 / 2; \
			voice->o4n1 = sample;																\
			break;																				\
																								\
		case CONTROL_LP3:																		\
			/* pole 3 is low-pass using K1 */													\
			sample = ((INT32)(voice->k1 >> 2) * (sample - voice->o3n1) / 16384) + voice->o3n1;	\
			voice->o3n2 = voice->o3n1;															\
			voice->o3n1 = sample;																\
																								\
			/* pole 4 is high-pass using K2 */													\
			sample = sample - voice->o3n2 + ((INT32)(voice->k2 >> 2) * voice->o4n1) / 32768 + voice->o4n1 / 2; \
			voice->o4n1 = sample;																\
			break;																				\
																								\
		case CONTROL_LP4:																		\
			/* pole 3 is low-pass using K2 */													\
			sample = ((INT32)(voice->k2 >> 2) * (sample - voice->o3n1) / 16384) + voice->o3n1;	\
			voice->o3n2 = voice->o3n1;															\
			voice->o3n1 = sample;																\
																								\
			/* pole 4 is low-pass using K2 */													\
			sample = ((INT32)(voice->k2 >> 2) * (sample - voice->o4n1) / 16384) + voice->o4n1;	\
			voice->o4n1 = sample;																\
			break;																				\
																								\
		case CONTROL_LP4 | CONTROL_LP3:															\
			/* pole 3 is low-pass using K1 */													\
			sample = ((INT32)(voice->k1 >> 2) * (sample - voice->o3n1) / 16384) + voice->o3n1;	\
			voice->o3n2 = voice->o3n1;															\
			voice->o3n1 = sample;																\
																								\
			/* pole 4 is low-pass using K2 */													\
			sample = ((INT32)(voice->k2 >> 2) * (sample - voice->o4n1) / 16384) + voice->o4n1;	\
			voice->o4n1 = sample;																\
			break;																				\
	}																							\
} while (0)



/**********************************************************************************************

     update_envelopes -- update the envelopes

***********************************************************************************************/

#define update_envelopes(voice, samples)											\
do																					\
{																					\
	int count = (samples > 1 && samples > voice->ecount) ? voice->ecount : samples;	\
																					\
	/* decrement the envelope counter */											\
	voice->ecount -= count;															\
																					\
	/* ramp left volume */															\
	if (voice->lvramp)																\
	{																				\
		voice->lvol += (INT8)voice->lvramp * count;									\
		if ((INT32)voice->lvol < 0) voice->lvol = 0;								\
		else if (voice->lvol > 0xffff) voice->lvol = 0xffff;						\
	}																				\
																					\
	/* ramp right volume */															\
	if (voice->rvramp)																\
	{																				\
		voice->rvol += (INT8)voice->rvramp * count;									\
		if ((INT32)voice->rvol < 0) voice->rvol = 0;								\
		else if (voice->rvol > 0xffff) voice->rvol = 0xffff;						\
	}																				\
																					\
	/* ramp k1 filter constant */													\
	if (voice->k1ramp && ((INT32)voice->k1ramp >= 0 || !(voice->filtcount & 7)))	\
	{																				\
		voice->k1 += (INT8)voice->k1ramp * count;									\
		if ((INT32)voice->k1 < 0) voice->k1 = 0;									\
		else if (voice->k1 > 0xffff) voice->k1 = 0xffff;							\
	}																				\
																					\
	/* ramp k2 filter constant */													\
	if (voice->k2ramp && ((INT32)voice->k2ramp >= 0 || !(voice->filtcount & 7)))	\
	{																				\
		voice->k2 += (INT8)voice->k2ramp * count;									\
		if ((INT32)voice->k2 < 0) voice->k2 = 0;									\
		else if (voice->k2 > 0xffff) voice->k2 = 0xffff;							\
	}																				\
																					\
	/* update the filter constant counter */										\
	voice->filtcount += count;														\
																					\
} while (0)



/**********************************************************************************************

     check_for_end_forward
     check_for_end_reverse -- check for loop end and loop appropriately

***********************************************************************************************/

#define check_for_end_forward(voice, accum)											\
do																					\
{																					\
	/* are we past the end? */														\
	if (accum > voice->end && !(voice->control & CONTROL_LEI))						\
	{																				\
		/* generate interrupt if required */										\
		if (voice->control&CONTROL_IRQE)											\
			voice->control |= CONTROL_IRQ;											\
																					\
		/* handle the different types of looping */									\
		switch (voice->control & CONTROL_LOOPMASK)									\
		{																			\
			/* non-looping */														\
			case 0:																	\
				voice->control |= CONTROL_STOP0;									\
				goto alldone;														\
																					\
			/* uni-directional looping */											\
			case CONTROL_LPE:														\
				accum = (voice->start + (accum - voice->end)) & accum_mask;			\
				break;																\
																					\
			/* trans-wave looping */												\
			case CONTROL_BLE:														\
				accum = (voice->start + (accum - voice->end)) & accum_mask;			\
				voice->control = (voice->control & ~CONTROL_LOOPMASK) | CONTROL_LEI;\
				break;																\
																					\
			/* bi-directional looping */											\
			case CONTROL_LPE | CONTROL_BLE:											\
				accum = (voice->end - (accum - voice->end)) & accum_mask;			\
				voice->control ^= CONTROL_DIR;										\
				goto reverse;														\
		}																			\
	}																				\
} while (0)


#define check_for_end_reverse(voice, accum)											\
do																					\
{																					\
	/* are we past the end? */														\
	if (accum < voice->start && !(voice->control & CONTROL_LEI))					\
	{																				\
		/* generate interrupt if required */										\
		if (voice->control&CONTROL_IRQE)											\
			voice->control |= CONTROL_IRQ;											\
																					\
		/* handle the different types of looping */									\
		switch (voice->control & CONTROL_LOOPMASK)									\
		{																			\
			/* non-looping */														\
			case 0:																	\
				voice->control |= CONTROL_STOP0;									\
				goto alldone;														\
																					\
			/* uni-directional looping */											\
			case CONTROL_LPE:														\
				accum = (voice->end - (voice->start - accum)) & accum_mask;			\
				break;																\
																					\
			/* trans-wave looping */												\
			case CONTROL_BLE:														\
				accum = (voice->end - (voice->start - accum)) & accum_mask;			\
				voice->control = (voice->control & ~CONTROL_LOOPMASK) | CONTROL_LEI;\
				break;																\
																					\
			/* bi-directional looping */											\
			case CONTROL_LPE | CONTROL_BLE:											\
				accum = (voice->start + (voice->start - accum)) & accum_mask;		\
				voice->control ^= CONTROL_DIR;										\
				goto reverse;														\
		}																			\
	}																				\
} while (0)



/**********************************************************************************************

     generate_dummy -- generate nothing, just apply envelopes

***********************************************************************************************/

static void generate_dummy(struct ES5506Voice *voice, UINT16 *base, INT32 *lbuffer, INT32 *rbuffer, int samples)
{
	UINT32 freqcount = voice->freqcount;
	UINT32 accum = voice->accum & accum_mask;

	/* outer loop, in case we switch directions */
	while (samples > 0 && !(voice->control & CONTROL_STOPMASK))
	{
reverse:
		/* two cases: first case is forward direction */
		if (!(voice->control & CONTROL_DIR))
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				accum = (accum + freqcount) & accum_mask;

				/* update filters/volumes */
				if (voice->ecount != 0)
					update_envelopes(voice, 1);

				/* check for loop end */
				check_for_end_forward(voice, accum);
			}
		}

		/* two cases: second case is backward direction */
		else
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				accum = (accum - freqcount) & accum_mask;

				/* update filters/volumes */
				if (voice->ecount != 0)
					update_envelopes(voice, 1);

				/* check for loop end */
				check_for_end_reverse(voice, accum);
			}
		}
	}

	/* if we stopped, process any additional envelope */
alldone:
	voice->accum = accum;
	if (samples > 0)
		update_envelopes(voice, samples);
}



/**********************************************************************************************

     generate_ulaw -- general u-law decoding routine

***********************************************************************************************/

static void generate_ulaw(struct ES5506Voice *voice, UINT16 *base, INT32 *lbuffer, INT32 *rbuffer, int samples)
{
	UINT32 freqcount = voice->freqcount;
	UINT32 accum = voice->accum & accum_mask;
	INT32 lvol = volume_lookup[voice->lvol >> 4];
	INT32 rvol = volume_lookup[voice->rvol >> 4];

	/* pre-add the bank offset */
	base += voice->exbank;

	/* outer loop, in case we switch directions */
	while (samples > 0 && !(voice->control & CONTROL_STOPMASK))
	{
reverse:
		/* two cases: first case is forward direction */
		if (!(voice->control & CONTROL_DIR))
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				INT32 val1 = base[accum >> 11];
				INT32 val2 = base[((accum + (1 << 11)) & accum_mask) >> 11];

				/* decompress u-law */
				val1 = ulaw_lookup[val1 >> (16 - ULAW_MAXBITS)];
				val2 = ulaw_lookup[val2 >> (16 - ULAW_MAXBITS)];

				/* interpolate */
				val1 = interpolate(val1, val2, accum);
				accum = (accum + freqcount) & accum_mask;

				/* apply filters */
				apply_filters(voice, val1);

				/* update filters/volumes */
				if (voice->ecount != 0)
				{
					update_envelopes(voice, 1);
					lvol = volume_lookup[voice->lvol >> 4];
					rvol = volume_lookup[voice->rvol >> 4];
				}

				/* apply volumes and add */
				*lbuffer++ += (val1 * lvol) >> 11;
				*rbuffer++ += (val1 * rvol) >> 11;

				/* check for loop end */
				check_for_end_forward(voice, accum);
			}
		}

		/* two cases: second case is backward direction */
		else
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				INT32 val1 = base[accum >> 11];
				INT32 val2 = base[((accum + (1 << 11)) & accum_mask) >> 11];

				/* decompress u-law */
				val1 = ulaw_lookup[val1 >> (16 - ULAW_MAXBITS)];
				val2 = ulaw_lookup[val2 >> (16 - ULAW_MAXBITS)];

				/* interpolate */
				val1 = interpolate(val1, val2, accum);
				accum = (accum - freqcount) & accum_mask;

				/* apply filters */
				apply_filters(voice, val1);

				/* update filters/volumes */
				if (voice->ecount != 0)
				{
					update_envelopes(voice, 1);
					lvol = volume_lookup[voice->lvol >> 4];
					rvol = volume_lookup[voice->rvol >> 4];
				}

				/* apply volumes and add */
				*lbuffer++ += (val1 * lvol) >> 11;
				*rbuffer++ += (val1 * rvol) >> 11;

				/* check for loop end */
				check_for_end_reverse(voice, accum);
			}
		}
	}

	/* if we stopped, process any additional envelope */
alldone:
	voice->accum = accum;
	if (samples > 0)
		update_envelopes(voice, samples);
}



/**********************************************************************************************

     generate_pcm -- general PCM decoding routine

***********************************************************************************************/

static void generate_pcm(struct ES5506Voice *voice, UINT16 *base, INT32 *lbuffer, INT32 *rbuffer, int samples)
{
	UINT32 freqcount = voice->freqcount;
	UINT32 accum = voice->accum & accum_mask;
	INT32 lvol = volume_lookup[voice->lvol >> 4];
	INT32 rvol = volume_lookup[voice->rvol >> 4];

	/* pre-add the bank offset */
	base += voice->exbank;

	/* outer loop, in case we switch directions */
	while (samples > 0 && !(voice->control & CONTROL_STOPMASK))
	{
reverse:
		/* two cases: first case is forward direction */
		if (!(voice->control & CONTROL_DIR))
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				INT32 val1 = (INT16)base[accum >> 11];
				INT32 val2 = (INT16)base[((accum + (1 << 11)) & accum_mask) >> 11];

				/* interpolate */
				val1 = interpolate(val1, val2, accum);
				accum = (accum + freqcount) & accum_mask;

				/* apply filters */
				apply_filters(voice, val1);

				/* update filters/volumes */
				if (voice->ecount != 0)
				{
					update_envelopes(voice, 1);
					lvol = volume_lookup[voice->lvol >> 4];
					rvol = volume_lookup[voice->rvol >> 4];
				}

				/* apply volumes and add */
				*lbuffer++ += (val1 * lvol) >> 11;
				*rbuffer++ += (val1 * rvol) >> 11;

				/* check for loop end */
				check_for_end_forward(voice, accum);
			}
		}

		/* two cases: second case is backward direction */
		else
		{
			/* loop while we still have samples to generate */
			while (samples--)
			{
				/* fetch two samples */
				INT32 val1 = (INT16)base[accum >> 11];
				INT32 val2 = (INT16)base[((accum + (1 << 11)) & accum_mask) >> 11];

				/* interpolate */
				val1 = interpolate(val1, val2, accum);
				accum = (accum - freqcount) & accum_mask;

				/* apply filters */
				apply_filters(voice, val1);

				/* update filters/volumes */
				if (voice->ecount != 0)
				{
					update_envelopes(voice, 1);
					lvol = volume_lookup[voice->lvol >> 4];
					rvol = volume_lookup[voice->rvol >> 4];
				}

				/* apply volumes and add */
				*lbuffer++ += (val1 * lvol) >> 11;
				*rbuffer++ += (val1 * rvol) >> 11;

				/* check for loop end */
				check_for_end_reverse(voice, accum);
			}
		}
	}

	/* if we stopped, process any additional envelope */
alldone:
	voice->accum = accum;
	if (samples > 0)
		update_envelopes(voice, samples);
}



/**********************************************************************************************

     generate_samples -- tell each voice to generate samples

***********************************************************************************************/

static void generate_samples(struct ES5506Chip *chip, INT32 *left, INT32 *right, int samples)
{
	int v;

	/* skip if nothing to do */
	if (!samples)
		return;

	/* clear out the accumulator */
	memset(left, 0, samples * sizeof(left[0]));
	memset(right, 0, samples * sizeof(right[0]));

	/* loop over voices */
	for (v = 0; v <= chip->active_voices; v++)
	{
		struct ES5506Voice *voice = &chip->voice[v];
		UINT16 *base = chip->region_base[voice->control >> 14];

		/* special case: if end == start, stop the voice */
		if (voice->start == voice->end)
			voice->control |= CONTROL_STOP0;

		/* generate from the appropriate source */
		if (!base)
		{
			/*logerror("NULL region base %d\n",voice->control >> 14);*/
			generate_dummy(voice, base, left, right, samples);
		}
		else if (voice->control & 0x2000)
			generate_ulaw(voice, base, left, right, samples);
		else
			generate_pcm(voice, base, left, right, samples);

		/* does this voice have it's IRQ bit raised? */
		if (voice->control&CONTROL_IRQ) 
		{
/*logerror("IRQ raised on voice %d!!\n",v);*/
			/* only update voice vector if existing IRQ is acked by host */
			if (chip->irqv&0x80)
			{
				/* latch voice number into vector, and set high bit low */
				chip->irqv=v&0x7f;

				/* take down IRQ bit on voice */
				voice->control&=~CONTROL_IRQ;

				/* inform host of irq */
				update_irq_state(chip);
			}
		}
	}
}



/**********************************************************************************************

     es5506_update -- update the sound chip so that it is in sync with CPU execution

***********************************************************************************************/

static void es5506_update(int num, INT16 **buffer, int length)
{
	struct ES5506Chip *chip = &es5506[num];
	INT32 *lsrc = scratch, *rsrc = scratch;
	INT32 lprev = chip->last_lsample;
	INT32 rprev = chip->last_rsample;
	INT32 lcurr = chip->curr_lsample;
	INT32 rcurr = chip->curr_rsample;
	INT16 *ldest = buffer[0];
	INT16 *rdest = buffer[1];
	INT32 interp;
	int remaining = length;
	int samples_left = 0;

#if MAKE_WAVS
	/* start the logging once we have a sample rate */
	if (chip->output_step)
	{
		if (!chip->wavraw)
		{
			int sample_rate = (int)((double)Machine->sample_rate / (double)(1 << FRAC_BITS) * (double)chip->output_step);
			chip->wavraw = wav_open("raw.wav", sample_rate, 2);
		}
		if (!chip->wavresample)
			chip->wavresample = wav_open("resamp.wav", Machine->sample_rate, 2);
	}
#endif

	/* then sample-rate convert with linear interpolation */
	while (remaining > 0)
	{
		/* if we're over, grab the next samples */
		while (chip->output_pos >= FRAC_ONE)
		{
			/* do we have any samples available? */
			if (samples_left == 0)
			{
				/* compute how many new samples we need */
				UINT32 final_pos = chip->output_pos + (remaining - 1) * chip->output_step;
				samples_left = final_pos >> FRAC_BITS;
				if (samples_left > MAX_SAMPLE_CHUNK)
					samples_left = MAX_SAMPLE_CHUNK;

				/* determine left/right source data */
				lsrc = scratch;
				rsrc = scratch + samples_left;
				generate_samples(chip, lsrc, rsrc, samples_left);

#if MAKE_WAVS
				/* log the raw data */
				if (chip->wavraw)
					wav_add_data_32lr(chip->wavraw, lsrc, rsrc, samples_left, 4);
#endif
			}

			/* adjust the positions */
			chip->output_pos -= FRAC_ONE;
			lprev = lcurr;
			rprev = rcurr;

			/* fetch new samples */
			lcurr = *lsrc++ >> 4;
			rcurr = *rsrc++ >> 4;
			samples_left--;
		}

		/* interpolate between the two current samples */
		while (remaining > 0 && chip->output_pos < FRAC_ONE)
		{
			/* left channel */
			interp = backend_interpolate(lprev, lcurr, chip->output_pos);
         MAME_CLAMP_SAMPLE(interp);
			*ldest++ = interp;

			/* right channel */
			interp = backend_interpolate(rprev, rcurr, chip->output_pos);
         MAME_CLAMP_SAMPLE(interp);
			*rdest++ = interp;

			/* advance */
			chip->output_pos += chip->output_step;
			remaining--;
		}
	}

	/* remember the last samples */
	chip->last_lsample = lprev;
	chip->last_rsample = rprev;
	chip->curr_lsample = lcurr;
	chip->curr_rsample = rcurr;

#if MAKE_WAVS
	/* log the resampled data */
	if (chip->wavresample)
		wav_add_data_16lr(chip->wavresample, buffer[0], buffer[1], length);
#endif
}



/**********************************************************************************************

     ES5506_sh_start -- start emulation of the ES5506

***********************************************************************************************/

int ES5506_sh_start(const struct MachineSound *msound)
{
	const struct ES5506interface *intf = msound->sound_interface;
	char stream_name[2][40];
	const char *stream_name_ptrs[2];
	int vol[2];
	int i, j;

	/* debugging */
#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && !eslog)
		eslog = fopen("es.log", "w");
#endif
	/* compute the tables */
	if (!compute_tables())
		return 1;

	/* initialize the voices */
	memset(&es5506, 0, sizeof(es5506));
	for (i = 0; i < intf->num; i++)
	{
		/* generate the name and create the stream */
		sprintf(stream_name[0], "%s #%d Ch1", sound_name(msound), i);
		sprintf(stream_name[1], "%s #%d Ch2", sound_name(msound), i);
		stream_name_ptrs[0] = stream_name[0];
		stream_name_ptrs[1] = stream_name[1];

		/* set the volumes */
		vol[0] = intf->mixing_level[i] & 0xffff;
		vol[1] = intf->mixing_level[i] >> 16;

		/* create the stream */
		es5506[i].stream = stream_init_multi(2, stream_name_ptrs, vol, Machine->sample_rate, i, es5506_update);
		if (es5506[i].stream == -1)
			return 1;

		/* initialize the regions */
		es5506[i].region_base[0] = intf->region0[i] ? (UINT16 *)memory_region(intf->region0[i]) : NULL;
		es5506[i].region_base[1] = intf->region1[i] ? (UINT16 *)memory_region(intf->region1[i]) : NULL;
		es5506[i].region_base[2] = intf->region2[i] ? (UINT16 *)memory_region(intf->region2[i]) : NULL;
		es5506[i].region_base[3] = intf->region3[i] ? (UINT16 *)memory_region(intf->region3[i]) : NULL;

		/* initialize the rest of the structure */
		es5506[i].master_clock = (double)intf->baseclock[i];
		es5506[i].irq_callback = intf->irq_callback[i];
		es5506[i].irqv = 0x80;

		/* init the voices */
		for (j = 0; j < 32; j++)
		{
			es5506[i].voice[j].index = j;
			es5506[i].voice[j].control = CONTROL_STOPMASK;
			es5506[i].voice[j].lvol = 0xffff;
			es5506[i].voice[j].rvol = 0xffff;
			es5506[i].voice[j].exbank = 0;
		}
	}
	accum_mask = 0xffffffff;

	/* allocate memory */
	accumulator = malloc(sizeof(accumulator[0]) * 2 * MAX_SAMPLE_CHUNK);
	scratch = malloc(sizeof(scratch[0]) * 2 * MAX_SAMPLE_CHUNK);
	if (!accumulator || !scratch)
		return 1;

	/* success */
	return 0;
}



/**********************************************************************************************

     ES5506_sh_stop -- stop emulation of the ES5506

***********************************************************************************************/

void ES5506_sh_stop(void)
{
	/* free memory */
	if (accumulator)
		free(accumulator);
	accumulator = NULL;

	if (scratch)
		free(scratch);
	scratch = NULL;

	if (ulaw_lookup)
		free(ulaw_lookup);
	ulaw_lookup = NULL;

	if (volume_lookup)
		free(volume_lookup);
	volume_lookup = NULL;

	/* debugging */
#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && eslog)
	{
		fclose(eslog);
		eslog = NULL;
	}
#endif

#if MAKE_WAVS
{
	int i;

	for (i = 0; i < MAX_ES5506; i++)
	{
		if (es5506[i].wavraw)
			wav_close(es5506[i].wavraw);
		if (es5506[i].wavresample)
			wav_close(es5506[i].wavresample);
	}
}
#endif
}



/**********************************************************************************************

     es5506_reg_write -- handle a write to the selected ES5506 register

***********************************************************************************************/

static INLINE void es5506_reg_write_low(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT32 data)
{
	switch (offset)
	{
		case 0x00/8:	/* CR */
			voice->control = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, control=%04x\n", chip->current_page & 0x1f, voice->control);
#endif
			break;
		case 0x08/8:	/* FC */
			voice->freqcount = data & 0x1ffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, freq count=%08x\n", chip->current_page & 0x1f, voice->freqcount);
#endif
			break;

		case 0x10/8:	/* LVOL */
			voice->lvol = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, left vol=%04x\n", chip->current_page & 0x1f, voice->lvol);
#endif
			break;


		case 0x18/8:	/* LVRAMP */
			voice->lvramp = (data & 0xff00) >> 8;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, left vol ramp=%04x\n", chip->current_page & 0x1f, voice->lvramp);
#endif
			break;

		case 0x20/8:	/* RVOL */
			voice->rvol = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, right vol=%04x\n", chip->current_page & 0x1f, voice->rvol);
#endif
			break;

		case 0x28/8:	/* RVRAMP */
			voice->rvramp = (data & 0xff00) >> 8;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, right vol ramp=%04x\n", chip->current_page & 0x1f, voice->rvramp);
#endif
			break;

		case 0x30/8:	/* ECOUNT */
			voice->ecount = data & 0x1ff;
			voice->filtcount = 0;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, envelope count=%04x\n", chip->current_page & 0x1f, voice->ecount);
#endif
			break;

		case 0x38/8:	/* K2 */
			voice->k2 = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, K2=%04x\n", chip->current_page & 0x1f, voice->k2);
#endif
			break;

		case 0x40/8:	/* K2RAMP */
			voice->k2ramp = ((data & 0xff00) >> 8) | ((data & 0x0001) << 31);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, K2 ramp=%04x\n", chip->current_page & 0x1f, voice->k2ramp);
#endif
			break;

		case 0x48/8:	/* K1 */
			voice->k1 = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, K1=%04x\n", chip->current_page & 0x1f, voice->k1);
#endif
			break;

		case 0x50/8:	/* K1RAMP */
			voice->k1ramp = ((data & 0xff00) >> 8) | ((data & 0x0001) << 31);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, K1 ramp=%04x\n", chip->current_page & 0x1f, voice->k1ramp);
#endif
			break;

		case 0x58/8:	/* ACTV */
		{
			double sample_rate = chip->master_clock / (double)(16 * ((data & 0x1f) + 1));
			if (Machine->sample_rate)
				chip->output_step = (int)(sample_rate * (double)(1 << FRAC_BITS) / (double)Machine->sample_rate);
			chip->active_voices = data & 0x1f;

#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "active voices=%d, sample_rate=%d, output_step=%08x\n", chip->active_voices, (int)sample_rate, chip->output_step);
#endif
			break;
		}

		case 0x60/8:	/* MODE */
			chip->mode = data & 0x1f;
			break;

		case 0x68/8:	/* PAR - read only */
		case 0x70/8:	/* IRQV - read only */
			break;

		case 0x78/8:	/* PAGE */
			chip->current_page = data & 0x7f;
			break;
	}
}


static INLINE void es5506_reg_write_high(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT32 data)
{
	switch (offset)
	{
		case 0x00/8:	/* CR */
			voice->control = data & 0xffff;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, control=%04x\n", chip->current_page & 0x1f, voice->control);
#endif
			break;

		case 0x08/8:	/* START */
			voice->start = data & 0xfffff800;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, loop start=%08x\n", chip->current_page & 0x1f, voice->start);
#endif
			break;

		case 0x10/8:	/* END */
			voice->end = data & 0xffffff80;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, loop end=%08x\n", chip->current_page & 0x1f, voice->end);
#endif
			break;

		case 0x18/8:	/* ACCUM */
			voice->accum = data;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, accum=%08x\n", chip->current_page & 0x1f, voice->accum);
#endif
			break;

		case 0x20/8:	/* O4(n-1) */
			voice->o4n1 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O4(n-1)=%05x\n", chip->current_page & 0x1f, voice->o4n1 & 0x3ffff);
#endif
			break;

		case 0x28/8:	/* O3(n-1) */
			voice->o3n1 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O3(n-1)=%05x\n", chip->current_page & 0x1f, voice->o3n1 & 0x3ffff);
#endif
			break;

		case 0x30/8:	/* O3(n-2) */
			voice->o3n2 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O3(n-2)=%05x\n", chip->current_page & 0x1f, voice->o3n2 & 0x3ffff);
#endif
			break;

		case 0x38/8:	/* O2(n-1) */
			voice->o2n1 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O2(n-1)=%05x\n", chip->current_page & 0x1f, voice->o2n1 & 0x3ffff);
#endif
			break;

		case 0x40/8:	/* O2(n-2) */
			voice->o2n2 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O2(n-2)=%05x\n", chip->current_page & 0x1f, voice->o2n2 & 0x3ffff);
#endif
			break;

		case 0x48/8:	/* O1(n-1) */
			voice->o1n1 = (INT32)(data << 14) >> 14;
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "voice %d, O1(n-1)=%05x\n", chip->current_page & 0x1f, voice->o1n1 & 0x3ffff);
#endif
			break;

		case 0x50/8:	/* W_ST */
			chip->wst = data & 0x7f;
			break;

		case 0x58/8:	/* W_END */
			chip->wend = data & 0x7f;
			break;

		case 0x60/8:	/* LR_END */
			chip->lrend = data & 0x7f;
			break;

		case 0x68/8:	/* PAR - read only */
		case 0x70/8:	/* IRQV - read only */
			break;

		case 0x78/8:	/* PAGE */
			chip->current_page = data & 0x7f;
			break;
	}
}

static INLINE void es5506_reg_write_test(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT32 data)
{
	switch (offset)
	{
#ifdef LOG_COMMANDS
		case 0x00/8:	/* CHANNEL 0 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 0 left test write %08x\n", data);
			break;
		case 0x08/8:	/* CHANNEL 0 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 0 right test write %08x\n", data);
			break;
		case 0x10/8:	/* CHANNEL 1 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 1 left test write %08x\n", data);
			break;

		case 0x18/8:	/* CHANNEL 1 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 1 right test write %08x\n", data);
			break;

		case 0x20/8:	/* CHANNEL 2 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 2 left test write %08x\n", data);
			break;

		case 0x28/8:	/* CHANNEL 2 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 2 right test write %08x\n", data);
			break;

		case 0x30/8:	/* CHANNEL 3 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 3 left test write %08x\n", data);
			break;

		case 0x38/8:	/* CHANNEL 3 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 3 right test write %08x\n", data);
			break;

		case 0x40/8:	/* CHANNEL 4 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 4 left test write %08x\n", data);
			break;

		case 0x48/8:	/* CHANNEL 4 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 4 right test write %08x\n", data);
			break;

		case 0x50/8:	/* CHANNEL 5 LEFT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 5 left test write %08x\n", data);
			break;

		case 0x58/8:	/* CHANNEL 6 RIGHT */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Channel 5 right test write %08x\n", data);
			break;

		case 0x60/8:	/* EMPTY */
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "Test write EMPTY %08x\n", data);
			break;
#endif
		case 0x68/8:	/* PAR - read only */
		case 0x70/8:	/* IRQV - read only */
			break;

		case 0x78/8:	/* PAGE */
			chip->current_page = data & 0x7f;
			break;
	}
}

static void es5506_reg_write(struct ES5506Chip *chip, offs_t offset, data8_t data)
{
	struct ES5506Voice *voice = &chip->voice[chip->current_page & 0x1f];
	int shift = 8 * (offset & 3);

	/* accumulate the data */
	chip->write_latch = (chip->write_latch & ~(0xff000000 >> shift)) | (data << (24 - shift));

	/* wait for a write to complete */
	if (shift != 24)
		return;

	/* force an update */
	stream_update(chip->stream, 0);

	/* switch off the page and register */
	if (chip->current_page < 0x20)
		es5506_reg_write_low(chip, voice, offset / 4, chip->write_latch);
	else if (chip->current_page < 0x40)
		es5506_reg_write_high(chip, voice, offset / 4, chip->write_latch);
	else
		es5506_reg_write_test(chip, voice, offset / 4, chip->write_latch);

	/* clear the write latch when done */
	chip->write_latch = 0;
}



/**********************************************************************************************

     es5506_reg_read -- read from the specified ES5506 register

***********************************************************************************************/

static INLINE UINT32 es5506_reg_read_low(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT32 result = 0;

	switch (offset)
	{
		case 0x00/8:	/* CR */
			result = voice->control;
			break;

		case 0x08/8:	/* FC */
			result = voice->freqcount;
			break;

		case 0x10/8:	/* LVOL */
			result = voice->lvol;
			break;

		case 0x18/8:	/* LVRAMP */
			result = voice->lvramp << 8;
			break;

		case 0x20/8:	/* RVOL */
			result = voice->rvol;
			break;

		case 0x28/8:	/* RVRAMP */
			result = voice->rvramp << 8;
			break;

		case 0x30/8:	/* ECOUNT */
			result = voice->ecount;
			break;

		case 0x38/8:	/* K2 */
			result = voice->k2;
			break;

		case 0x40/8:	/* K2RAMP */
			result = (voice->k2ramp << 8) | (voice->k2ramp >> 31);
			break;

		case 0x48/8:	/* K1 */
			result = voice->k1;
			break;

		case 0x50/8:	/* K1RAMP */
			result = (voice->k1ramp << 8) | (voice->k1ramp >> 31);
			break;

		case 0x58/8:	/* ACTV */
			result = chip->active_voices;
			break;

		case 0x60/8:	/* MODE */
			result = chip->mode;
			break;

		case 0x68/8:	/* PAR */
			if (chip->port_read)
				result = (*chip->port_read)();
			break;

		case 0x70/8:	/* IRQV */
			result = chip->irqv;
			update_internal_irq_state(chip);
			break;

		case 0x78/8:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}


static INLINE UINT32 es5506_reg_read_high(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT32 result = 0;

	switch (offset)
	{
		case 0x00/8:	/* CR */
			result = voice->control;
			break;

		case 0x08/8:	/* START */
			result = voice->start;
			break;

		case 0x10/8:	/* END */
			result = voice->end;
			break;

		case 0x18/8:	/* ACCUM */
			result = voice->accum;
			break;

		case 0x20/8:	/* O4(n-1) */
			result = voice->o4n1 & 0x3ffff;
			break;

		case 0x28/8:	/* O3(n-1) */
			result = voice->o3n1 & 0x3ffff;
			break;

		case 0x30/8:	/* O3(n-2) */
			result = voice->o3n2 & 0x3ffff;
			break;

		case 0x38/8:	/* O2(n-1) */
			result = voice->o2n1 & 0x3ffff;
			break;

		case 0x40/8:	/* O2(n-2) */
			result = voice->o2n2 & 0x3ffff;
			break;

		case 0x48/8:	/* O1(n-1) */
			result = voice->o1n1 & 0x3ffff;
			break;

		case 0x50/8:	/* W_ST */
			result = chip->wst;
			break;

		case 0x58/8:	/* W_END */
			result = chip->wend;
			break;

		case 0x60/8:	/* LR_END */
			result = chip->lrend;
			break;

		case 0x68/8:	/* PAR */
			if (chip->port_read)
				result = (*chip->port_read)();
			break;

		case 0x70/8:	/* IRQV */
			result = chip->irqv;
			update_internal_irq_state(chip);
			break;

		case 0x78/8:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}

static INLINE UINT32 es5506_reg_read_test(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT32 result = 0;

	switch (offset)
	{
		case 0x68/8:	/* PAR */
			if (chip->port_read)
				result = (*chip->port_read)();
			break;

		case 0x70/8:	/* IRQV */
			result = chip->irqv;
			break;

		case 0x78/8:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}

static data8_t es5506_reg_read(struct ES5506Chip *chip, offs_t offset)
{
	struct ES5506Voice *voice = &chip->voice[chip->current_page & 0x1f];
	int shift = 8 * (offset & 3);

	/* only read on offset 0 */
	if (shift != 0)
		return chip->read_latch >> (24 - shift);

#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && eslog)
		fprintf(eslog, "read from %02x/%02x -> ", chip->current_page, offset / 4 * 8);
#endif

	/* force an update */
	stream_update(chip->stream, 0);

	/* switch off the page and register */
	if (chip->current_page < 0x20)
		chip->read_latch = es5506_reg_read_low(chip, voice, offset / 4);
	else if (chip->current_page < 0x40)
		chip->read_latch = es5506_reg_read_high(chip, voice, offset / 4);
	else
		chip->read_latch = es5506_reg_read_test(chip, voice, offset / 4);

#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && eslog)
		fprintf(eslog, "%08x\n", chip->read_latch);
#endif

	/* return the high byte */
	return chip->read_latch >> 24;
}

/**********************************************************************************************

     ES5506_data_0_r/ES5506_data_1_r -- handle a read from the status register

***********************************************************************************************/

READ_HANDLER( ES5506_data_0_r )
{
	return es5506_reg_read(&es5506[0], offset);
}

READ_HANDLER( ES5506_data_1_r )
{
	return es5506_reg_read(&es5506[1], offset);
}

READ16_HANDLER( ES5506_data_0_word_r )
{
	return es5506_reg_read(&es5506[0], offset);
}

READ16_HANDLER( ES5506_data_1_word_r )
{
	return es5506_reg_read(&es5506[1], offset);
}



/**********************************************************************************************

     ES5506_data_0_w/ES5506_data_1_w -- handle a write to the current register

***********************************************************************************************/

WRITE_HANDLER( ES5506_data_0_w )
{
	es5506_reg_write(&es5506[0], offset, data);
}

WRITE_HANDLER( ES5506_data_1_w )
{
	es5506_reg_write(&es5506[1], offset, data);
}

WRITE16_HANDLER( ES5506_data_0_word_w )
{
	if (ACCESSING_LSB)
		es5506_reg_write(&es5506[0], offset, data);
}

WRITE16_HANDLER( ES5506_data_1_word_w )
{
	if (ACCESSING_LSB)
		es5506_reg_write(&es5506[1], offset, data);
}



/**********************************************************************************************

     ES5505_sh_start -- start emulation of the ES5505

***********************************************************************************************/

int ES5505_sh_start(const struct MachineSound *msound)
{
	const struct ES5505interface *intf = msound->sound_interface;
	struct ES5506interface es5506intf;
	struct MachineSound es5506msound;
	int result;

	memset(&es5506intf, 0, sizeof(es5506intf));

	es5506intf.num = intf->num;
	memcpy(es5506intf.baseclock, intf->baseclock, sizeof(es5506intf.baseclock));
	memcpy(es5506intf.region0, intf->region0, sizeof(es5506intf.region0));
	memcpy(es5506intf.region1, intf->region1, sizeof(es5506intf.region1));
	memcpy(es5506intf.mixing_level, intf->mixing_level, sizeof(es5506intf.mixing_level));
	memcpy(es5506intf.irq_callback, intf->irq_callback, sizeof(es5506intf.irq_callback));
	memcpy(es5506intf.read_port, intf->read_port, sizeof(es5506intf.read_port));

	es5506msound.sound_interface = &es5506intf;
	es5506msound.sound_type = msound->sound_type;

	result = ES5506_sh_start(&es5506msound);
	accum_mask = 0x7fffffff;

	return result;
}



/**********************************************************************************************

     ES5505_sh_stop -- stop emulation of the ES5506

***********************************************************************************************/

void ES5505_sh_stop(void)
{
	ES5506_sh_stop();
}



/**********************************************************************************************

     es5505_reg_write -- handle a write to the selected ES5505 register

***********************************************************************************************/

static INLINE void es5505_reg_write_low(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	switch (offset)
	{
		case 0x00:	/* CR */
			if (ACCESSING_LSB)
			{
#if RAINE_CHECK
				voice->control &= ~(CONTROL_STOPMASK | CONTROL_LOOPMASK | CONTROL_DIR);
#else
				voice->control &= ~(CONTROL_STOPMASK | CONTROL_BS0 | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ);
#endif
				voice->control |= (data & (CONTROL_STOPMASK | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ)) |
								  ((data << 12) & CONTROL_BS0);
			}
			if (ACCESSING_MSB)
			{
				voice->control &= ~(CONTROL_CA0 | CONTROL_CA1 | CONTROL_LPMASK);
				voice->control |= ((data >> 2) & CONTROL_LPMASK) |
								  ((data << 2) & (CONTROL_CA0 | CONTROL_CA1));
			}
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, control=%04x (raw=%04x & %04x)\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->control, data, mem_mask ^ 0xffff);
#endif
			break;

		case 0x01:	/* FC */
			if (ACCESSING_LSB)
				voice->freqcount = (voice->freqcount & ~0x001fe) | ((data & 0x00ff) << 1);
			if (ACCESSING_MSB)
				voice->freqcount = (voice->freqcount & ~0x1fe00) | ((data & 0xff00) << 1);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, freq count=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->freqcount);
#endif
			break;

		case 0x02:	/* STRT (hi) */
			if (ACCESSING_LSB)
				voice->start = (voice->start & ~0x03fc0000) | ((data & 0x00ff) << 18);
			if (ACCESSING_MSB)
				voice->start = (voice->start & ~0x7c000000) | ((data & 0x1f00) << 18);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, loop start=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->start);
#endif
			break;

		case 0x03:	/* STRT (lo) */
			if (ACCESSING_LSB)
				voice->start = (voice->start & ~0x00000380) | ((data & 0x00e0) << 2);
			if (ACCESSING_MSB)
				voice->start = (voice->start & ~0x0003fc00) | ((data & 0xff00) << 2);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, loop start=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->start);
#endif
			break;

		case 0x04:	/* END (hi) */
			if (ACCESSING_LSB)
				voice->end = (voice->end & ~0x03fc0000) | ((data & 0x00ff) << 18);
			if (ACCESSING_MSB)
				voice->end = (voice->end & ~0x7c000000) | ((data & 0x1f00) << 18);
#if RAINE_CHECK
			voice->control |= CONTROL_STOP0;
#endif
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, loop end=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->end);
#endif
			break;

		case 0x05:	/* END (lo) */
			if (ACCESSING_LSB)
				voice->end = (voice->end & ~0x00000380) | ((data & 0x00e0) << 2);
			if (ACCESSING_MSB)
				voice->end = (voice->end & ~0x0003fc00) | ((data & 0xff00) << 2);
#if RAINE_CHECK
			voice->control |= CONTROL_STOP0;
#endif
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, loop end=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->end);
#endif
			break;

		case 0x06:	/* K2 */
			if (ACCESSING_LSB)
				voice->k2 = (voice->k2 & ~0x00f0) | (data & 0x00f0);
			if (ACCESSING_MSB)
				voice->k2 = (voice->k2 & ~0xff00) | (data & 0xff00);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, K2=%04x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->k2);
#endif
			break;

		case 0x07:	/* K1 */
			if (ACCESSING_LSB)
				voice->k1 = (voice->k1 & ~0x00f0) | (data & 0x00f0);
			if (ACCESSING_MSB)
				voice->k1 = (voice->k1 & ~0xff00) | (data & 0xff00);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, K1=%04x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->k1);
#endif
			break;

		case 0x08:	/* LVOL */
			if (ACCESSING_MSB)
				voice->lvol = (voice->lvol & ~0xff00) | (data & 0xff00);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, left vol=%04x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->lvol);
#endif
			break;

		case 0x09:	/* RVOL */
			if (ACCESSING_MSB)
				voice->rvol = (voice->rvol & ~0xff00) | (data & 0xff00);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, right vol=%04x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->rvol);
#endif
			break;

		case 0x0a:	/* ACC (hi) */
			if (ACCESSING_LSB)
				voice->accum = (voice->accum & ~0x03fc0000) | ((data & 0x00ff) << 18);
			if (ACCESSING_MSB)
				voice->accum = (voice->accum & ~0x7c000000) | ((data & 0x1f00) << 18);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, accum=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->accum);
#endif
			break;

		case 0x0b:	/* ACC (lo) */
			if (ACCESSING_LSB)
				voice->accum = (voice->accum & ~0x000003fc) | ((data & 0x00ff) << 2);
			if (ACCESSING_MSB)
				voice->accum = (voice->accum & ~0x0003fc00) | ((data & 0xff00) << 2);
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, accum=%08x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->accum);
#endif
			break;

		case 0x0c:	/* unused */
			break;

		case 0x0d:	/* ACT */
			if (ACCESSING_LSB)
			{
				double sample_rate = chip->master_clock / (double)(16 * ((data & 0x1f) + 1));
				if (Machine->sample_rate)
					chip->output_step = (int)(sample_rate * (double)(1 << FRAC_BITS) / (double)Machine->sample_rate);
				chip->active_voices = data & 0x1f;

#ifdef LOG_COMMANDS
				if (LOG_COMMANDS && eslog)
					fprintf(eslog, "active voices=%d, sample_rate=%d, output_step=%08x\n", chip->active_voices, (int)sample_rate, chip->output_step);
#endif
			}
			break;

		case 0x0e:	/* IRQV - read only */
			break;

		case 0x0f:	/* PAGE */
			if (ACCESSING_LSB)
				chip->current_page = data & 0x7f;
			break;
	}
}


static INLINE void es5505_reg_write_high(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	switch (offset)
	{
		case 0x00:	/* CR */
			if (ACCESSING_LSB)
			{
				voice->control &= ~(CONTROL_STOPMASK | CONTROL_BS0 | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ);
				voice->control |= (data & (CONTROL_STOPMASK | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ)) |
								  ((data << 12) & CONTROL_BS0);
			}
			if (ACCESSING_MSB)
			{
				voice->control &= ~(CONTROL_CA0 | CONTROL_CA1 | CONTROL_LPMASK);
				voice->control |= ((data >> 2) & CONTROL_LPMASK) |
								  ((data << 2) & (CONTROL_CA0 | CONTROL_CA1));
			}
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, control=%04x (raw=%04x & %04x)\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->control, data, mem_mask ^ 0xffff);
#endif
			break;

		case 0x01:	/* O4(n-1) */
			if (ACCESSING_LSB)
				voice->o4n1 = (voice->o4n1 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o4n1 = (INT16)((voice->o4n1 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O4(n-1)=%05x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o4n1 & 0x3ffff);
#endif
			break;

		case 0x02:	/* O3(n-1) */
			if (ACCESSING_LSB)
				voice->o3n1 = (voice->o3n1 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o3n1 = (INT16)((voice->o3n1 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O3(n-1)=%05x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o3n1 & 0x3ffff);
#endif
			break;

		case 0x03:	/* O3(n-2) */
			if (ACCESSING_LSB)
				voice->o3n2 = (voice->o3n2 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o3n2 = (INT16)((voice->o3n2 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O3(n-2)=%05x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o3n2 & 0x3ffff);
#endif
			break;

		case 0x04:	/* O2(n-1) */
			if (ACCESSING_LSB)
				voice->o2n1 = (voice->o2n1 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o2n1 = (INT16)((voice->o2n1 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O2(n-1)=%05x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o2n1 & 0x3ffff);
#endif
			break;

		case 0x05:	/* O2(n-2) */
			if (ACCESSING_LSB)
				voice->o2n2 = (voice->o2n2 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o2n2 = (INT16)((voice->o2n2 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O2(n-2)=%05x\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o2n2 & 0x3ffff);
#endif
			break;

		case 0x06:	/* O1(n-1) */
			if (ACCESSING_LSB)
				voice->o1n1 = (voice->o1n1 & ~0x00ff) | (data & 0x00ff);
			if (ACCESSING_MSB)
				voice->o1n1 = (INT16)((voice->o1n1 & ~0xff00) | (data & 0xff00));
#ifdef LOG_COMMANDS
			if (LOG_COMMANDS && eslog)
				fprintf(eslog, "%06x:voice %d, O1(n-1)=%05x (accum=%08x)\n", activecpu_get_previouspc(), chip->current_page & 0x1f, voice->o2n1 & 0x3ffff, voice->accum);
#endif
			break;

		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:	/* unused */
			break;

		case 0x0d:	/* ACT */
			if (ACCESSING_LSB)
			{
				double sample_rate = chip->master_clock / (double)(16 * ((data & 0x1f) + 1));
				if (Machine->sample_rate)
					chip->output_step = (int)(sample_rate * (double)(1 << FRAC_BITS) / (double)Machine->sample_rate);
				chip->active_voices = data & 0x1f;

#ifdef LOG_COMMANDS
				if (LOG_COMMANDS && eslog)
					fprintf(eslog, "active voices=%d, sample_rate=%d, output_step=%08x\n", chip->active_voices, (int)sample_rate, chip->output_step);
#endif
			}
			break;

		case 0x0e:	/* IRQV - read only */
			break;

		case 0x0f:	/* PAGE */
			if (ACCESSING_LSB)
				chip->current_page = data & 0x7f;
			break;
	}
}


static INLINE void es5505_reg_write_test(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset, UINT16 data, UINT16 mem_mask)
{
	switch (offset)
	{
		case 0x00:	/* CH0L */
		case 0x01:	/* CH0R */
		case 0x02:	/* CH1L */
		case 0x03:	/* CH1R */
		case 0x04:	/* CH2L */
		case 0x05:	/* CH2R */
		case 0x06:	/* CH3L */
		case 0x07:	/* CH3R */
			break;

		case 0x08:	/* SERMODE */
			chip->mode = data & 0x0007;
			break;

		case 0x09:	/* PAR */
			break;

		case 0x0d:	/* ACT */
			if (ACCESSING_LSB)
			{
				double sample_rate = chip->master_clock / (double)(16 * ((data & 0x1f) + 1));
				if (Machine->sample_rate)
					chip->output_step = (int)(sample_rate * (double)(1 << FRAC_BITS) / (double)Machine->sample_rate);
				chip->active_voices = data & 0x1f;

#ifdef LOG_COMMANDS
				if (LOG_COMMANDS && eslog)
					fprintf(eslog, "active voices=%d, sample_rate=%d, output_step=%08x\n", chip->active_voices, (int)sample_rate, chip->output_step);
#endif
			}
			break;

		case 0x0e:	/* IRQV - read only */
			break;

		case 0x0f:	/* PAGE */
			if (ACCESSING_LSB)
				chip->current_page = data & 0x7f;
			break;
	}
}


static void es5505_reg_write(struct ES5506Chip *chip, offs_t offset, data16_t data, data16_t mem_mask)
{
	struct ES5506Voice *voice = &chip->voice[chip->current_page & 0x1f];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04x:ES5505 write %02x/%02x = %04x & %04x\n", activecpu_get_previouspc(), chip->current_page, offset, data, mem_mask ^ 0xffff);

	/* force an update */
	stream_update(chip->stream, 0);

	/* switch off the page and register */
	if (chip->current_page < 0x20)
		es5505_reg_write_low(chip, voice, offset, data, mem_mask);
	else if (chip->current_page < 0x40)
		es5505_reg_write_high(chip, voice, offset, data, mem_mask);
	else
		es5505_reg_write_test(chip, voice, offset, data, mem_mask);
}



/**********************************************************************************************

     es5505_reg_read -- read from the specified ES5505 register

***********************************************************************************************/

static INLINE UINT16 es5505_reg_read_low(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT16 result = 0;

	switch (offset)
	{
		case 0x00:	/* CR */
			result = (voice->control & (CONTROL_STOPMASK | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ)) |
					 ((voice->control & CONTROL_BS0) >> 12) |
					 ((voice->control & CONTROL_LPMASK) << 2) |
					 ((voice->control & (CONTROL_CA0 | CONTROL_CA1)) >> 2) |
					 0xf000;
			break;

		case 0x01:	/* FC */
			result = voice->freqcount >> 1;
			break;

		case 0x02:	/* STRT (hi) */
			result = voice->start >> 18;
			break;

		case 0x03:	/* STRT (lo) */
			result = voice->start >> 2;
			break;

		case 0x04:	/* END (hi) */
			result = voice->end >> 18;
			break;

		case 0x05:	/* END (lo) */
			result = voice->end >> 2;
			break;

		case 0x06:	/* K2 */
			result = voice->k2;
			break;

		case 0x07:	/* K1 */
			result = voice->k1;
			break;

		case 0x08:	/* LVOL */
			result = voice->lvol;
			break;

		case 0x09:	/* RVOL */
			result = voice->rvol;
			break;

		case 0x0a:	/* ACC (hi) */
			result = voice->accum >> 18;
			break;

		case 0x0b:	/* ACC (lo) */
			result = voice->accum >> 2;
			break;

		case 0x0c:	/* unused */
			break;

		case 0x0d:	/* ACT */
			result = chip->active_voices;
			break;

		case 0x0e:	/* IRQV */
			result = chip->irqv;
			update_internal_irq_state(chip);
			break;

		case 0x0f:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}


static INLINE UINT16 es5505_reg_read_high(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT16 result = 0;

	switch (offset)
	{
		case 0x00:	/* CR */
			result = (voice->control & (CONTROL_STOPMASK | CONTROL_LOOPMASK | CONTROL_IRQE | CONTROL_DIR | CONTROL_IRQ)) |
					 ((voice->control & CONTROL_BS0) >> 12) |
					 ((voice->control & CONTROL_LPMASK) << 2) |
					 ((voice->control & (CONTROL_CA0 | CONTROL_CA1)) >> 2) |
					 0xf000;
			break;

		case 0x01:	/* O4(n-1) */
			result = voice->o4n1;
			break;

		case 0x02:	/* O3(n-1) */
			result = voice->o3n1;
			break;

		case 0x03:	/* O3(n-2) */
			result = voice->o3n2;
			break;

		case 0x04:	/* O2(n-1) */
			result = voice->o2n1;
			break;

		case 0x05:	/* O2(n-2) */
			result = voice->o2n2;
			break;

		case 0x06:	/* O1(n-1) */
			/* special case for the Taito F3 games: they set the accumulator on a stopped */
			/* voice and assume the filters continue to process the data. They then read */
			/* the O1(n-1) in order to extract raw data from the sound ROMs. Since we don't */
			/* want to waste time filtering stopped channels, we just look for a read from */
			/* this register on a stopped voice, and return the raw sample data at the */
			/* accumulator */
			if ((voice->control & CONTROL_STOPMASK) && chip->region_base[voice->control >> 14]) {
				voice->o1n1 = chip->region_base[voice->control >> 14][voice->exbank + (voice->accum >> 11)];
				/*logerror("%02x %08x ==> %08x\n",voice->o1n1,voice->control >> 14,voice->exbank + (voice->accum >> 11));*/
			}
				result = voice->o1n1;
			break;

		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:	/* unused */
			break;

		case 0x0d:	/* ACT */
			result = chip->active_voices;
			break;

		case 0x0e:	/* IRQV */
			result = chip->irqv;
			update_internal_irq_state(chip);
			break;

		case 0x0f:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}


static INLINE UINT16 es5505_reg_read_test(struct ES5506Chip *chip, struct ES5506Voice *voice, offs_t offset)
{
	UINT16 result = 0;

	switch (offset)
	{
		case 0x00:	/* CH0L */
		case 0x01:	/* CH0R */
		case 0x02:	/* CH1L */
		case 0x03:	/* CH1R */
		case 0x04:	/* CH2L */
		case 0x05:	/* CH2R */
		case 0x06:	/* CH3L */
		case 0x07:	/* CH3R */
			break;

		case 0x08:	/* SERMODE */
			result = chip->mode;
			break;

		case 0x09:	/* PAR */
			if (chip->port_read)
				result = (*chip->port_read)();
			break;

		case 0x0f:	/* PAGE */
			result = chip->current_page;
			break;
	}
	return result;
}


static data16_t es5505_reg_read(struct ES5506Chip *chip, offs_t offset)
{
	struct ES5506Voice *voice = &chip->voice[chip->current_page & 0x1f];
	data16_t result = 0;

#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && eslog)
		fprintf(eslog, "read from %02x/%02x -> ", chip->current_page, offset);
#endif

	/* force an update */
	stream_update(chip->stream, 0);

	/* switch off the page and register */
	if (chip->current_page < 0x20)
		result = es5505_reg_read_low(chip, voice, offset);
	else if (chip->current_page < 0x40)
		result = es5505_reg_read_high(chip, voice, offset);
	else
		result = es5505_reg_read_test(chip, voice, offset);

#ifdef LOG_COMMANDS
	if (LOG_COMMANDS && eslog)
		fprintf(eslog, "%04x (accum=%08x)\n", result, voice->accum);
#endif

	/* return the high byte */
	return result;
}



/**********************************************************************************************

     ES5505_data_0_r/ES5505_data_1_r -- handle a read from the status register

***********************************************************************************************/

READ16_HANDLER( ES5505_data_0_r )
{
	return es5505_reg_read(&es5506[0], offset);
}

READ16_HANDLER( ES5505_data_1_r )
{
	return es5505_reg_read(&es5506[1], offset);
}

/**********************************************************************************************

     ES5505_data_0_w/ES5505_data_1_w -- handle a write to the current register

***********************************************************************************************/

WRITE16_HANDLER( ES5505_data_0_w )
{
	es5505_reg_write(&es5506[0], offset, data, mem_mask);
}

WRITE16_HANDLER( ES5505_data_1_w )
{
	es5505_reg_write(&es5506[1], offset, data, mem_mask);
}

void ES5506_voice_bank_0_w(int voice, int bank)
{
#if RAINE_CHECK
	es5506[0].voice[voice].control = CONTROL_STOPMASK;
#endif
	es5506[0].voice[voice].exbank=bank;
}

void ES5506_voice_bank_1_w(int voice, int bank)
{
#if RAINE_CHECK
	es5506[1].voice[voice].control = CONTROL_STOPMASK;
#endif
	es5506[1].voice[voice].exbank=bank;
}
