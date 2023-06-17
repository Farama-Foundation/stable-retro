/***************************************************************************

	Cinemat/Leland driver

	Leland sound hardware
	driver by Aaron Giles and Paul Leaman

	-------------------------------------------------------------------

	1st generation sound hardware was controlled by the master Z80.
	It drove an AY-8910/AY-8912 pair for music. It also had two DACs
	that were driven by the video refresh. At the end of each scanline
	there are 8-bit DAC samples that can be enabled via the output
	ports on the AY-8910. The DACs run at a fixed frequency of 15.3kHz,
	since they are clocked once each scanline.

	-------------------------------------------------------------------

	2nd generation sound hardware was used in Redline Racer. It
	consisted of an 80186 microcontroller driving 8 8-bit DACs. The
	frequency of the DACs were controlled by one of 3 Intel 8254
	programmable interval timers (PITs):

		DAC number	Clock source
		----------	-----------------
			0		8254 PIT 1 output 0
			1		8254 PIT 1 output 1
			2		8254 PIT 1 output 2
			3		8254 PIT 2 output 0
			4		8254 PIT 2 output 1
			5-7		8254 PIT 3 output 0

	The clock outputs for each DAC can be read, and are polled to
	determine when data should be updated on the chips. The 80186's
	two DMA channels are generally used to drive the first two DACs,
	with the remaining 6 DACs being fed manually via polling.

	-------------------------------------------------------------------

	3rd generation sound hardware appeared in the football games
	(Quarterback, AAFB) and the later games up through Pigout. This
	variant is closely based on the Redline Racer sound system, but
	they took out two of the DACs and replaced them with a higher
	resolution (10-bit) DAC. The driving clocks have been rearranged
	a bit, and the number of PITs reduced from 3 to 2:

		DAC number	Clock source
		----------	-----------------
			0		8254 PIT 1 output 0
			1		8254 PIT 1 output 1
			2		8254 PIT 1 output 2
			3		8254 PIT 2 output 0
			4		8254 PIT 2 output 1
			5		8254 PIT 2 output 2
			10-bit	80186 timer 0

	Like the 2nd generation board, the first two DACs are driven via
	the DMA channels, and the remaining 5 DACs are polled.

	-------------------------------------------------------------------

	4th generation sound hardware showed up in Ataxx, Indy Heat, and
	World Soccer Finals. For this variant, they removed one more PIT
	and 3 of the 8-bit DACs, and added a YM2151 music chip and an
	externally-fed 8-bit DAC.

		DAC number	Clock source
		----------	-----------------
			0		8254 PIT 1 output 0
			1		8254 PIT 1 output 1
			2		8254 PIT 1 output 2
			10-bit	80186 timer 0
			ext		80186 timer 1

	The externally driven DACs have registers for a start/stop address
	and triggers to control the clocking.

***************************************************************************/

#include "driver.h"
#include "cpu/i86/i186intf.h"
#include "cpu/z80/z80.h"
#include "leland.h"


/*************************************
 *
 *	1st generation sound
 *
 *************************************/

#define DAC_BUFFER_SIZE		1024
#define DAC_BUFFER_MASK		(DAC_BUFFER_SIZE - 1)

static UINT8 *dac_buffer[2];
static UINT32 dac_bufin[2];
static UINT32 dac_bufout[2];

static int dac_stream;

static void leland_update(int param, INT16 *buffer, int length)
{
	int dacnum;

	/* reset the buffer */
	memset(buffer, 0, length * sizeof(INT16));
	for (dacnum = 0; dacnum < 2; dacnum++)
	{
		int bufout = dac_bufout[dacnum];
		int count = (dac_bufin[dacnum] - bufout) & DAC_BUFFER_MASK;

		if (count > 300)
		{
			UINT8 *base = dac_buffer[dacnum];
			int i;

			for (i = 0; i < length && count > 0; i++, count--)
			{
				buffer[i] += ((INT16)base[bufout] - 0x80) * 0x40;
				bufout = (bufout + 1) & DAC_BUFFER_MASK;
			}
			dac_bufout[dacnum] = bufout;
		}
	}
}


int leland_sh_start(const struct MachineSound *msound)
{
	/* reset globals */
	dac_buffer[0] = dac_buffer[1] = NULL;
	dac_bufin[0]  = dac_bufin[1]  = 0;
	dac_bufout[0] = dac_bufout[1] = 0;

	/* skip if no sound */
	if (Machine->sample_rate == 0)
		return 0;

	/* allocate the stream */
	dac_stream = stream_init("Onboard DACs", 50, 256*60, 0, leland_update);

	/* allocate memory */
	dac_buffer[0] = auto_malloc(DAC_BUFFER_SIZE);
	dac_buffer[1] = auto_malloc(DAC_BUFFER_SIZE);
	if (!dac_buffer[0] || !dac_buffer[1])
		return 1;

	return 0;
}


void leland_sh_stop(void)
{
}


void leland_dac_update(int dacnum, UINT8 sample)
{
	UINT8 *buffer = dac_buffer[dacnum];
	int bufin = dac_bufin[dacnum];

	/* skip if nothing */
	if (!buffer)
		return;

	/* copy data from VRAM */
	buffer[bufin] = sample;
	bufin = (bufin + 1) & DAC_BUFFER_MASK;

	/* update the buffer */
	dac_bufin[dacnum] = bufin;
}



/*************************************
 *
 *	2nd-4th generation sound
 *
 *************************************/

#define LOG_INTERRUPTS		0
#define LOG_DMA				0
#define LOG_SHORTAGES		0
#define LOG_TIMER			0
#define LOG_COMM			0
#define LOG_PORTS			0
#define LOG_DAC				0
#define LOG_EXTERN			0
#define LOG_PIT				0
#define LOG_OPTIMIZATION	0


/* according to the Intel manual, external interrupts are not latched */
/* however, I cannot get this system to work without latching them */
#define LATCH_INTS	1

#define DAC_VOLUME_SCALE	4
#define CPU_RESUME_TRIGGER	7123


static int dma_stream;
static int nondma_stream;
static int extern_stream;

static UINT8 *ram_base;
static UINT8 has_ym2151;
static UINT8 is_redline;

static UINT8 last_control;
static UINT8 clock_active;
static UINT8 clock_tick;

static UINT8 sound_command[2];
static UINT8 sound_response;

static UINT32 ext_start;
static UINT32 ext_stop;
static UINT8 ext_active;
static UINT8 *ext_base;

static UINT8 *active_mask;
static int total_reads;

struct mem_state
{
	UINT16	lower;
	UINT16	upper;
	UINT16	middle;
	UINT16	middle_size;
	UINT16	peripheral;
};

struct timer_state
{
	UINT16	control;
	UINT16	maxA;
	UINT16	maxB;
	UINT16	count;
	void *	int_timer;
	void *	time_timer;
	UINT8	time_timer_active;
	double	last_time;
};

struct dma_state
{
	UINT32	source;
	UINT32	dest;
	UINT16	count;
	UINT16	control;
	UINT8	finished;
	void *	finish_timer;
};

struct intr_state
{
	UINT8	pending;
	UINT16	ack_mask;
	UINT16	priority_mask;
	UINT16	in_service;
	UINT16	request;
	UINT16	status;
	UINT16	poll_status;
	UINT16	timer;
	UINT16	dma[2];
	UINT16	ext[4];
};

static struct i186_state
{
	struct timer_state	timer[3];
	struct dma_state	dma[2];
	struct intr_state	intr;
	struct mem_state	mem;
} i186;


#define DAC_BUFFER_SIZE			1024
#define DAC_BUFFER_SIZE_MASK	(DAC_BUFFER_SIZE - 1)
static struct dac_state
{
	INT16	value;
	INT16	volume;
	UINT32	frequency;
	UINT32	step;
	UINT32	fraction;

	INT16	buffer[DAC_BUFFER_SIZE];
	UINT32	bufin;
	UINT32	bufout;
	UINT32	buftarget;
} dac[8];

static struct counter_state
{
	void *timer;
	INT32 count;
	UINT8 mode;
	UINT8 readbyte;
	UINT8 writebyte;
} counter[9];

static void set_dac_frequency(int which, int frequency);

static READ_HANDLER( peripheral_r );
static WRITE_HANDLER( peripheral_w );



/*************************************
 *
 *	Manual DAC sound generation
 *
 *************************************/

static void leland_i186_dac_update(int param, INT16 *buffer, int length)
{
	int i, j, start, stop;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "----\n");

	/* reset the buffer */
	memset(buffer, 0, length * sizeof(INT16));

	/* if we're redline racer, we have more DACs */
	if (!is_redline)
		start = 2, stop = 7;
	else
		start = 0, stop = 8;

	/* loop over manual DAC channels */
	for (i = start; i < stop; i++)
	{
		struct dac_state *d = &dac[i];
		int count = (d->bufin - d->bufout) & DAC_BUFFER_SIZE_MASK;

		/* if we have data, process it */
		if (count > 0)
		{
			INT16 *base = d->buffer;
			int source = d->bufout;
			int frac = d->fraction;
			int step = d->step;

			/* sample-rate convert to the output frequency */
			for (j = 0; j < length && count > 0; j++)
			{
				buffer[j] += base[source];
				frac += step;
				source += frac >> 24;
				count -= frac >> 24;
				frac &= 0xffffff;
				source &= DAC_BUFFER_SIZE_MASK;
			}

			if (j < length)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "DAC #%d short by %d/%d samples\n", i, length - j, length);

			/* update the DAC state */
			d->fraction = frac;
			d->bufout = source;
		}

		/* update the clock status */
		if (count < d->buftarget)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "  - trigger due to clock active in update\n");
			cpu_trigger(CPU_RESUME_TRIGGER);
			clock_active |= 1 << i;
		}
	}
}



/*************************************
 *
 *	DMA-based DAC sound generation
 *
 *************************************/

static void leland_i186_dma_update(int param, INT16 *buffer, int length)
{
	int i, j;

	/* reset the buffer */
	memset(buffer, 0, length * sizeof(INT16));

	/* loop over DMA buffers */
	for (i = 0; i < 2; i++)
	{
		struct dma_state *d = &i186.dma[i];

		/* check for enabled DMA */
		if (d->control & 0x0002)
		{
			/* make sure the parameters meet our expectations */
			if ((d->control & 0xfe00) != 0x1600)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA control %02X\n", d->control);
			}
			else if (!is_redline && ((d->dest & 1) || (d->dest & 0x3f) > 0x0b))
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA destination %02X\n", d->dest);
			}
			else if (is_redline && (d->dest & 0xf000) != 0x4000 && (d->dest & 0xf000) != 0x5000)
			{
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA destination %02X\n", d->dest);
			}

			/* otherwise, we're ready for liftoff */
			else
			{
				UINT8 *base = memory_region(REGION_CPU3);
				int source = d->source;
				int count = d->count;
				int which, frac, step, volume;

				/* adjust for redline racer */
				if (!is_redline)
					which = (d->dest & 0x3f) / 2;
				else
					which = (d->dest >> 9) & 7;

				frac = dac[which].fraction;
				step = dac[which].step;
				volume = dac[which].volume;

				/* sample-rate convert to the output frequency */
				for (j = 0; j < length && count > 0; j++)
				{
					buffer[j] += ((int)base[source] - 0x80) * volume;
					frac += step;
					source += frac >> 24;
					count -= frac >> 24;
					frac &= 0xffffff;
				}

				/* update the DMA state */
				if (count > 0)
				{
					d->source = source;
					d->count = count;
				}
				else
				{
					/* let the timer callback actually mark the transfer finished */
					d->source = source + count - 1;
					d->count = 1;
					d->finished = 1;
				}

				log_cb(RETRO_LOG_DEBUG, LOGPRE "DMA Generated %d samples - new count = %04X, source = %04X\n", j, d->count, d->source);

				/* update the DAC state */
				dac[which].fraction = frac;
			}
		}
	}
}



/*************************************
 *
 *	Externally-driven DAC sound generation
 *
 *************************************/

static void leland_i186_extern_update(int param, INT16 *buffer, int length)
{
	struct dac_state *d = &dac[7];
	int count = ext_stop - ext_start;
	int j;

	/* reset the buffer */
	memset(buffer, 0, length * sizeof(INT16));

	/* if we have data, process it */
	if (count > 0 && ext_active)
	{
		int source = ext_start;
		int frac = d->fraction;
		int step = d->step;

		/* sample-rate convert to the output frequency */
		for (j = 0; j < length && count > 0; j++)
		{
			buffer[j] += ((INT16)ext_base[source] - 0x80) * d->volume;
			frac += step;
			source += frac >> 24;
			count -= frac >> 24;
			frac &= 0xffffff;
		}

		/* update the DAC state */
		d->fraction = frac;
		ext_start = source;
	}
}



/*************************************
 *
 *	Sound initialization
 *
 *************************************/

static void internal_timer_int(int which);
static void dma_timer_callback(int which);

int leland_i186_sh_start(const struct MachineSound *msound)
{
	int i;

	/* bail if nothing to play */
	if (Machine->sample_rate == 0)
		return 0;

	/* determine which sound hardware is installed */
	has_ym2151 = 0;
	for (i = 0; i < MAX_SOUND; i++)
		if (Machine->drv->sound[i].sound_type == SOUND_YM2151)
			has_ym2151 = 1;

	/* allocate separate streams for the DMA and non-DMA DACs */
	dma_stream = stream_init("80186 DMA-driven DACs", 100, Machine->sample_rate, 0, leland_i186_dma_update);
	nondma_stream = stream_init("80186 manually-driven DACs", 100, Machine->sample_rate, 0, leland_i186_dac_update);

	/* if we have a 2151, install an externally driven DAC stream */
	if (has_ym2151)
	{
		ext_base = memory_region(REGION_SOUND1);
		extern_stream = stream_init("80186 externally-driven DACs", 100, Machine->sample_rate, 0, leland_i186_extern_update);
	}

	/* by default, we're not redline racer */
	is_redline = 0;

	/* create timers here so they stick around */
	i186.timer[0].int_timer = timer_alloc(internal_timer_int);
	i186.timer[1].int_timer = timer_alloc(internal_timer_int);
	i186.timer[2].int_timer = timer_alloc(internal_timer_int);
	i186.timer[0].time_timer = timer_alloc(NULL);
	i186.timer[1].time_timer = timer_alloc(NULL);
	i186.timer[2].time_timer = timer_alloc(NULL);
	i186.dma[0].finish_timer = timer_alloc(dma_timer_callback);
	i186.dma[1].finish_timer = timer_alloc(dma_timer_callback);

	for (i = 0; i < 9; i++)
		counter[i].timer = timer_alloc(NULL);

	return 0;
}


int redline_i186_sh_start(const struct MachineSound *msound)
{
	int result = leland_i186_sh_start(msound);
	is_redline = 1;
	return result;
}


static void leland_i186_reset(void)
{
	struct i186_state oldstate = i186;
	void *counter_timer[9];
	int i;

	/* reset the i186 state, but save the timers */
	memset(&i186, 0, sizeof(i186));
	i186.timer[0].int_timer = oldstate.timer[0].int_timer;
	i186.timer[1].int_timer = oldstate.timer[1].int_timer;
	i186.timer[2].int_timer = oldstate.timer[2].int_timer;
	i186.timer[0].time_timer = oldstate.timer[0].time_timer;
	i186.timer[1].time_timer = oldstate.timer[1].time_timer;
	i186.timer[2].time_timer = oldstate.timer[2].time_timer;
	i186.dma[0].finish_timer = oldstate.dma[0].finish_timer;
	i186.dma[1].finish_timer = oldstate.dma[1].finish_timer;

	/* reset the interrupt state */
	i186.intr.priority_mask	= 0x0007;
	i186.intr.timer 		= 0x000f;
	i186.intr.dma[0]		= 0x000f;
	i186.intr.dma[1]		= 0x000f;
	i186.intr.ext[0]		= 0x000f;
	i186.intr.ext[1]		= 0x000f;
	i186.intr.ext[2]		= 0x000f;
	i186.intr.ext[3]		= 0x000f;

	/* reset the DAC and counter states as well */
	memset(&dac, 0, sizeof(dac));
	for (i = 0; i < 9; i++)
		counter_timer[i] = counter[i].timer;
	memset(&counter, 0, sizeof(counter));
	for (i = 0; i < 9; i++)
		counter[i].timer = counter_timer[i];

	/* send a trigger in case we're suspended */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  - trigger due to reset\n");
	cpu_trigger(CPU_RESUME_TRIGGER);
	total_reads = 0;
}


void leland_i186_sound_init(void)
{
	/* RAM is multiply mapped in the first 128k of address space */
	cpu_setbank(6, ram_base);
	cpu_setbank(7, ram_base);

	/* reset the I86 registers */
	leland_i186_reset();

	/* reset our internal stuff */
	last_control = 0xf8;
	clock_active = 0;

	/* reset the external DAC */
	ext_start = 0;
	ext_stop = 0;
	ext_active = 0;
}



/*************************************
 *
 *	80186 interrupt controller
 *
 *************************************/

static int int_callback(int line)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Acknowledged interrupt vector %02X\n", timer_get_time(), i186.intr.poll_status & 0x1f);

	/* clear the interrupt */
	i86_set_irq_line(0, CLEAR_LINE);
	i186.intr.pending = 0;

	/* clear the request and set the in-service bit */
#if LATCH_INTS
	i186.intr.request &= ~i186.intr.ack_mask;
#else
	i186.intr.request &= ~(i186.intr.ack_mask & 0x0f);
#endif
	i186.intr.in_service |= i186.intr.ack_mask;
	if (i186.intr.ack_mask == 0x0001)
	{
		switch (i186.intr.poll_status & 0x1f)
		{
			case 0x08:	i186.intr.status &= ~0x01;	break;
			case 0x12:	i186.intr.status &= ~0x02;	break;
			case 0x13:	i186.intr.status &= ~0x04;	break;
		}
	}
	i186.intr.ack_mask = 0;

	/* a request no longer pending */
	i186.intr.poll_status &= ~0x8000;

	/* return the vector */
	return i186.intr.poll_status & 0x1f;
}


static void update_interrupt_state(void)
{
	int i, j, new_vector = 0;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "update_interrupt_status: req=%02X stat=%02X serv=%02X\n", i186.intr.request, i186.intr.status, i186.intr.in_service);

	/* loop over priorities */
	for (i = 0; i <= i186.intr.priority_mask; i++)
	{
		/* note: by checking 4 bits, we also verify that the mask is off */
		if ((i186.intr.timer & 15) == i)
		{
			/* if we're already servicing something at this level, don't generate anything new */
			if (i186.intr.in_service & 0x01)
				return;

			/* if there's something pending, generate an interrupt */
			if (i186.intr.status & 0x07)
			{
				if (i186.intr.status & 1)
					new_vector = 0x08;
				else if (i186.intr.status & 2)
					new_vector = 0x12;
				else if (i186.intr.status & 4)
					new_vector = 0x13;
				else
					usrintf_showmessage("Invalid timer interrupt!");

				/* set the clear mask and generate the int */
				i186.intr.ack_mask = 0x0001;
				goto generate_int;
			}
		}

		/* check DMA interrupts */
		for (j = 0; j < 2; j++)
			if ((i186.intr.dma[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (i186.intr.in_service & (0x04 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (i186.intr.request & (0x04 << j))
				{
					new_vector = 0x0a + j;

					/* set the clear mask and generate the int */
					i186.intr.ack_mask = 0x0004 << j;
					goto generate_int;
				}
			}

		/* check external interrupts */
		for (j = 0; j < 4; j++)
			if ((i186.intr.ext[j] & 15) == i)
			{
				/* if we're already servicing something at this level, don't generate anything new */
				if (i186.intr.in_service & (0x10 << j))
					return;

				/* if there's something pending, generate an interrupt */
				if (i186.intr.request & (0x10 << j))
				{
					/* otherwise, generate an interrupt for this request */
					new_vector = 0x0c + j;

					/* set the clear mask and generate the int */
					i186.intr.ack_mask = 0x0010 << j;
					goto generate_int;
				}
			}
	}
	return;

generate_int:
	/* generate the appropriate interrupt */
	i186.intr.poll_status = 0x8000 | new_vector;
	if (!i186.intr.pending)
		cpu_set_irq_line(2, 0, ASSERT_LINE);
	i186.intr.pending = 1;
	cpu_trigger(CPU_RESUME_TRIGGER);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  - trigger due to interrupt pending\n");
	log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Requesting interrupt vector %02X\n", timer_get_time(), new_vector);
}


static void handle_eoi(int data)
{
	int i, j;

	/* specific case */
	if (!(data & 0x8000))
	{
		/* turn off the appropriate in-service bit */
		switch (data & 0x1f)
		{
			case 0x08:	i186.intr.in_service &= ~0x01;	break;
			case 0x12:	i186.intr.in_service &= ~0x01;	break;
			case 0x13:	i186.intr.in_service &= ~0x01;	break;
			case 0x0a:	i186.intr.in_service &= ~0x04;	break;
			case 0x0b:	i186.intr.in_service &= ~0x08;	break;
			case 0x0c:	i186.intr.in_service &= ~0x10;	break;
			case 0x0d:	i186.intr.in_service &= ~0x20;	break;
			case 0x0e:	i186.intr.in_service &= ~0x40;	break;
			case 0x0f:	i186.intr.in_service &= ~0x80;	break;
			default:	log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:ERROR - 80186 EOI with unknown vector %02X\n", activecpu_get_pc(), data & 0x1f);
		}
		log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Got EOI for vector %02X\n", timer_get_time(), data & 0x1f);
	}

	/* non-specific case */
	else
	{
		/* loop over priorities */
		for (i = 0; i <= 7; i++)
		{
			/* check for in-service timers */
			if ((i186.intr.timer & 7) == i && (i186.intr.in_service & 0x01))
			{
				i186.intr.in_service &= ~0x01;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Got EOI for timer\n", timer_get_time());
				return;
			}

			/* check for in-service DMA interrupts */
			for (j = 0; j < 2; j++)
				if ((i186.intr.dma[j] & 7) == i && (i186.intr.in_service & (0x04 << j)))
				{
					i186.intr.in_service &= ~(0x04 << j);
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Got EOI for DMA%d\n", timer_get_time(), j);
					return;
				}

			/* check external interrupts */
			for (j = 0; j < 4; j++)
				if ((i186.intr.ext[j] & 7) == i && (i186.intr.in_service & (0x10 << j)))
				{
					i186.intr.in_service &= ~(0x10 << j);
					log_cb(RETRO_LOG_DEBUG, LOGPRE "(%f) **** Got EOI for INT%d\n", timer_get_time(), j);
					return;
				}
		}
	}
}



/*************************************
 *
 *	80186 internal timers
 *
 *************************************/

static void internal_timer_int(int which)
{
	struct timer_state *t = &i186.timer[which];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Hit interrupt callback for timer %d\n", which);

	/* set the max count bit */
	t->control |= 0x0020;

	/* request an interrupt */
	if (t->control & 0x2000)
	{
		i186.intr.status |= 0x01 << which;
		update_interrupt_state();
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  Generating timer interrupt\n");
	}

	/* if we're continuous, reset */
	if (t->control & 0x0001)
	{
		int count = t->maxA ? t->maxA : 0x10000;
		timer_adjust(t->int_timer, (double)count * TIME_IN_HZ(2000000), which, 0);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  Repriming interrupt\n");
	}
	else
		timer_adjust(t->int_timer, TIME_NEVER, which, 0);
}


static void internal_timer_sync(int which)
{
	struct timer_state *t = &i186.timer[which];

	/* if we have a timing timer running, adjust the count */
	if (t->time_timer_active)
	{
		double current_time = timer_timeelapsed(t->time_timer);
		int net_clocks = (int)((current_time - t->last_time) * 2000000.);
		t->last_time = current_time;

		/* set the max count bit if we passed the max */
		if ((int)t->count + net_clocks >= t->maxA)
			t->control |= 0x0020;

		/* set the new count */
		if (t->maxA != 0)
			t->count = (t->count + net_clocks) % t->maxA;
		else
			t->count = t->count + net_clocks;
	}
}


static void internal_timer_update(int which, int new_count, int new_maxA, int new_maxB, int new_control)
{
	struct timer_state *t = &i186.timer[which];
	int update_int_timer = 0;

	/* if we have a new count and we're on, update things */
	if (new_count != -1)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(which);
			update_int_timer = 1;
		}
		t->count = new_count;
	}

	/* if we have a new max and we're on, update things */
	if (new_maxA != -1 && new_maxA != t->maxA)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(which);
			update_int_timer = 1;
		}
		t->maxA = new_maxA;
		if (new_maxA == 0) new_maxA = 0x10000;

		/* redline racer controls nothing externally? */
		if (is_redline)
			;

		/* on the common board, timer 0 controls the 10-bit DAC frequency */
		else if (which == 0)
			set_dac_frequency(6, 2000000 / new_maxA);

		/* timer 1 controls the externally driven DAC on Indy Heat/WSF */
		else if (which == 1 && has_ym2151)
			set_dac_frequency(7, 2000000 / (new_maxA * 2));
	}

	/* if we have a new max and we're on, update things */
	if (new_maxB != -1 && new_maxB != t->maxB)
	{
		if (t->control & 0x8000)
		{
			internal_timer_sync(which);
			update_int_timer = 1;
		}
		t->maxB = new_maxB;
		if (new_maxB == 0) new_maxB = 0x10000;

		/* timer 1 controls the externally driven DAC on Indy Heat/WSF */
		/* they alternate the use of maxA and maxB in a way that makes no */
		/* sense according to the 80186 documentation! */
		if (which == 1 && has_ym2151)
			set_dac_frequency(7, 2000000 / (new_maxB * 2));
	}

	/* handle control changes */
	if (new_control != -1)
	{
		int diff;

		/* merge back in the bits we don't modify */
		new_control = (new_control & ~0x1fc0) | (t->control & 0x1fc0);

		/* handle the /INH bit */
		if (!(new_control & 0x4000))
			new_control = (new_control & ~0x8000) | (t->control & 0x8000);
		new_control &= ~0x4000;

		/* check for control bits we don't handle */
		diff = new_control ^ t->control;
		if (diff & 0x001c)
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR! - unsupported timer mode %04X\n", new_control);

		/* if we have real changes, update things */
		if (diff != 0)
		{
			/* if we're going off, make sure our timers are gone */
			if ((diff & 0x8000) && !(new_control & 0x8000))
			{
				/* compute the final count */
				internal_timer_sync(which);

				/* nuke the timer and force the interrupt timer to be recomputed */
				timer_adjust(t->time_timer, TIME_NEVER, which, 0);
				t->time_timer_active = 0;
				update_int_timer = 1;
			}

			/* if we're going on, start the timers running */
			else if ((diff & 0x8000) && (new_control & 0x8000))
			{
				/* start the timing */
				timer_adjust(t->time_timer, TIME_NEVER, which, 0);
				t->time_timer_active = 1;
				update_int_timer = 1;
			}

			/* if something about the interrupt timer changed, force an update */
			if (!(diff & 0x8000) && (diff & 0x2000))
			{
				internal_timer_sync(which);
				update_int_timer = 1;
			}
		}

		/* set the new control register */
		t->control = new_control;
	}

	/* update the interrupt timer */

	/* kludge: the YM2151 games sometimes crank timer 1 really high, and leave interrupts */
	/* enabled, even though the handler for timer 1 does nothing. To alleviate this, we */
	/* just ignore it */
	if (!has_ym2151 || which != 1)
		if (update_int_timer)
		{
			if ((t->control & 0x8000) && (t->control & 0x2000))
			{
				int diff = t->maxA - t->count;
				if (diff <= 0) diff += 0x10000;
				timer_adjust(t->int_timer, (double)diff * TIME_IN_HZ(2000000), which, 0);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "Set interrupt timer for %d\n", which);
			}
			else
				timer_adjust(t->int_timer, TIME_NEVER, which, 0);
		}
}



/*************************************
 *
 *	80186 internal DMA
 *
 *************************************/

static void dma_timer_callback(int which)
{
	struct dma_state *d = &i186.dma[which];

	/* force an update and see if we're really done */
	stream_update(dma_stream, 0);

	/* complete the status update */
	d->control &= ~0x0002;
	d->source += d->count;
	d->count = 0;

	/* check for interrupt generation */
	if (d->control & 0x0100)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "DMA%d timer callback - requesting interrupt: count = %04X, source = %04X\n", which, d->count, d->source);
		i186.intr.request |= 0x04 << which;
		update_interrupt_state();
	}
}


static void update_dma_control(int which, int new_control)
{
	struct dma_state *d = &i186.dma[which];
	int diff;

	/* handle the CHG bit */
	if (!(new_control & 0x0004))
		new_control = (new_control & ~0x0002) | (d->control & 0x0002);
	new_control &= ~0x0004;

	/* check for control bits we don't handle */
	diff = new_control ^ d->control;
	if (diff & 0x6811)
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR! - unsupported DMA mode %04X\n", new_control);

	/* if we're going live, set a timer */
	if ((diff & 0x0002) && (new_control & 0x0002))
	{
		/* make sure the parameters meet our expectations */
		if ((new_control & 0xfe00) != 0x1600)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA control %02X\n", new_control);
		}
		else if (!is_redline && ((d->dest & 1) || (d->dest & 0x3f) > 0x0b))
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA destination %02X\n", d->dest);
		}
		else if (is_redline && (d->dest & 0xf000) != 0x4000 && (d->dest & 0xf000) != 0x5000)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Unexpected DMA destination %02X\n", d->dest);
		}

		/* otherwise, set a timer */
		else
		{
			int count = d->count;
			int dacnum;

			/* adjust for redline racer */
			if (!is_redline)
				dacnum = (d->dest & 0x3f) / 2;
			else
			{
				dacnum = (d->dest >> 9) & 7;
				dac[dacnum].volume = (d->dest & 0x1fe) / 2 / DAC_VOLUME_SCALE;
			}

			log_cb(RETRO_LOG_DEBUG, LOGPRE "Initiated DMA %d - count = %04X, source = %04X, dest = %04X\n", which, d->count, d->source, d->dest);

			d->finished = 0;
			timer_adjust(d->finish_timer, TIME_IN_HZ(dac[dacnum].frequency) * (double)count, which, 0);
		}
	}

	/* set the new control register */
	d->control = new_control;
}



/*************************************
 *
 *	80186 internal I/O reads
 *
 *************************************/

static READ_HANDLER( i186_internal_port_r )
{
	int shift = 8 * (offset & 1);
	int temp, which;

	switch (offset & ~1)
	{
		case 0x22:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:ERROR - read from 80186 EOI\n", activecpu_get_pc());
			break;

		case 0x24:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt poll\n", activecpu_get_pc());
			if (i186.intr.poll_status & 0x8000)
				int_callback(0);
			return (i186.intr.poll_status >> shift) & 0xff;

		case 0x26:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt poll status\n", activecpu_get_pc());
			return (i186.intr.poll_status >> shift) & 0xff;

		case 0x28:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt mask\n", activecpu_get_pc());
			temp  = (i186.intr.timer  >> 3) & 0x01;
			temp |= (i186.intr.dma[0] >> 1) & 0x04;
			temp |= (i186.intr.dma[1] >> 0) & 0x08;
			temp |= (i186.intr.ext[0] << 1) & 0x10;
			temp |= (i186.intr.ext[1] << 2) & 0x20;
			temp |= (i186.intr.ext[2] << 3) & 0x40;
			temp |= (i186.intr.ext[3] << 4) & 0x80;
			return (temp >> shift) & 0xff;

		case 0x2a:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt priority mask\n", activecpu_get_pc());
			return (i186.intr.priority_mask >> shift) & 0xff;

		case 0x2c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt in-service\n", activecpu_get_pc());
			return (i186.intr.in_service >> shift) & 0xff;

		case 0x2e:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt request\n", activecpu_get_pc());
			temp = i186.intr.request & ~0x0001;
			if (i186.intr.status & 0x0007)
				temp |= 1;
			return (temp >> shift) & 0xff;

		case 0x30:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 interrupt status\n", activecpu_get_pc());
			return (i186.intr.status >> shift) & 0xff;

		case 0x32:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 timer interrupt control\n", activecpu_get_pc());
			return (i186.intr.timer >> shift) & 0xff;

		case 0x34:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA 0 interrupt control\n", activecpu_get_pc());
			return (i186.intr.dma[0] >> shift) & 0xff;

		case 0x36:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA 1 interrupt control\n", activecpu_get_pc());
			return (i186.intr.dma[1] >> shift) & 0xff;

		case 0x38:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 INT 0 interrupt control\n", activecpu_get_pc());
			return (i186.intr.ext[0] >> shift) & 0xff;

		case 0x3a:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 INT 1 interrupt control\n", activecpu_get_pc());
			return (i186.intr.ext[1] >> shift) & 0xff;

		case 0x3c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 INT 2 interrupt control\n", activecpu_get_pc());
			return (i186.intr.ext[2] >> shift) & 0xff;

		case 0x3e:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 INT 3 interrupt control\n", activecpu_get_pc());
			return (i186.intr.ext[3] >> shift) & 0xff;

		case 0x50:
		case 0x58:
		case 0x60:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 Timer %d count\n", activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			if (!(offset & 1))
				internal_timer_sync(which);
			return (i186.timer[which].count >> shift) & 0xff;

		case 0x52:
		case 0x5a:
		case 0x62:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 Timer %d max A\n", activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].maxA >> shift) & 0xff;

		case 0x54:
		case 0x5c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 Timer %d max B\n", activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].maxB >> shift) & 0xff;

		case 0x56:
		case 0x5e:
		case 0x66:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 Timer %d control\n", activecpu_get_pc(), (offset - 0x50) / 8);
			which = (offset - 0x50) / 8;
			return (i186.timer[which].control >> shift) & 0xff;

		case 0xa0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 upper chip select\n", activecpu_get_pc());
			return (i186.mem.upper >> shift) & 0xff;

		case 0xa2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 lower chip select\n", activecpu_get_pc());
			return (i186.mem.lower >> shift) & 0xff;

		case 0xa4:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 peripheral chip select\n", activecpu_get_pc());
			return (i186.mem.peripheral >> shift) & 0xff;

		case 0xa6:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 middle chip select\n", activecpu_get_pc());
			return (i186.mem.middle >> shift) & 0xff;

		case 0xa8:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 middle P chip select\n", activecpu_get_pc());
			return (i186.mem.middle_size >> shift) & 0xff;

		case 0xc0:
		case 0xd0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d lower source address\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].source >> shift) & 0xff;

		case 0xc2:
		case 0xd2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d upper source address\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].source >> (shift + 16)) & 0xff;

		case 0xc4:
		case 0xd4:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d lower dest address\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].dest >> shift) & 0xff;

		case 0xc6:
		case 0xd6:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d upper dest address\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].dest >> (shift + 16)) & 0xff;

		case 0xc8:
		case 0xd8:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d transfer count\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].count >> shift) & 0xff;

		case 0xca:
		case 0xda:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 DMA%d control\n", activecpu_get_pc(), (offset - 0xc0) / 0x10);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			return (i186.dma[which].control >> shift) & 0xff;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:read 80186 port %02X\n", activecpu_get_pc(), offset);
			break;
	}
	return 0x00;
}



/*************************************
 *
 *	80186 internal I/O writes
 *
 *************************************/

static WRITE_HANDLER( i186_internal_port_w )
{
	static UINT8 even_byte;
	int temp, which, data16;

	/* warning: this assumes all port writes here are word-sized */
	if (!(offset & 1))
	{
		even_byte = data;
		return;
	}
	data16 = (data << 8) | even_byte;

	switch (offset & ~1)
	{
		case 0x22:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 EOI = %04X\n", activecpu_get_pc(), data16);
			handle_eoi(0x8000);
			update_interrupt_state();
			break;

		case 0x24:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:ERROR - write to 80186 interrupt poll = %04X\n", activecpu_get_pc(), data16);
			break;

		case 0x26:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:ERROR - write to 80186 interrupt poll status = %04X\n", activecpu_get_pc(), data16);
			break;

		case 0x28:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 interrupt mask = %04X\n", activecpu_get_pc(), data16);
			i186.intr.timer  = (i186.intr.timer  & ~0x08) | ((data16 << 3) & 0x08);
			i186.intr.dma[0] = (i186.intr.dma[0] & ~0x08) | ((data16 << 1) & 0x08);
			i186.intr.dma[1] = (i186.intr.dma[1] & ~0x08) | ((data16 << 0) & 0x08);
			i186.intr.ext[0] = (i186.intr.ext[0] & ~0x08) | ((data16 >> 1) & 0x08);
			i186.intr.ext[1] = (i186.intr.ext[1] & ~0x08) | ((data16 >> 2) & 0x08);
			i186.intr.ext[2] = (i186.intr.ext[2] & ~0x08) | ((data16 >> 3) & 0x08);
			i186.intr.ext[3] = (i186.intr.ext[3] & ~0x08) | ((data16 >> 4) & 0x08);
			update_interrupt_state();
			break;

		case 0x2a:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 interrupt priority mask = %04X\n", activecpu_get_pc(), data16);
			i186.intr.priority_mask = data16 & 0x0007;
			update_interrupt_state();
			break;

		case 0x2c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 interrupt in-service = %04X\n", activecpu_get_pc(), data16);
			i186.intr.in_service = data16 & 0x00ff;
			update_interrupt_state();
			break;

		case 0x2e:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 interrupt request = %04X\n", activecpu_get_pc(), data16);
			i186.intr.request = (i186.intr.request & ~0x00c0) | (data16 & 0x00c0);
			update_interrupt_state();
			break;

		case 0x30:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:WARNING - wrote to 80186 interrupt status = %04X\n", activecpu_get_pc(), data16);
			i186.intr.status = (i186.intr.status & ~0x8007) | (data16 & 0x8007);
			update_interrupt_state();
			break;

		case 0x32:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 timer interrupt contol = %04X\n", activecpu_get_pc(), data16);
			i186.intr.timer = data16 & 0x000f;
			break;

		case 0x34:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA 0 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.dma[0] = data16 & 0x000f;
			break;

		case 0x36:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA 1 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.dma[1] = data16 & 0x000f;
			break;

		case 0x38:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 INT 0 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[0] = data16 & 0x007f;
			break;

		case 0x3a:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 INT 1 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[1] = data16 & 0x007f;
			break;

		case 0x3c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 INT 2 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[2] = data16 & 0x001f;
			break;

		case 0x3e:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 INT 3 interrupt control = %04X\n", activecpu_get_pc(), data16);
			i186.intr.ext[3] = data16 & 0x001f;
			break;

		case 0x50:
		case 0x58:
		case 0x60:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 Timer %d count = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, data16, -1, -1, -1);
			break;

		case 0x52:
		case 0x5a:
		case 0x62:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 Timer %d max A = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, data16, -1, -1);
			break;

		case 0x54:
		case 0x5c:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 Timer %d max B = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, -1, data16, -1);
			break;

		case 0x56:
		case 0x5e:
		case 0x66:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 Timer %d control = %04X\n", activecpu_get_pc(), (offset - 0x50) / 8, data16);
			which = (offset - 0x50) / 8;
			internal_timer_update(which, -1, -1, -1, data16);
			break;

		case 0xa0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 upper chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.upper = data16 | 0xc038;
			break;

		case 0xa2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 lower chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.lower = (data16 & 0x3fff) | 0x0038;
			break;

		case 0xa4:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 peripheral chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.peripheral = data16 | 0x0038;
			break;

		case 0xa6:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 middle chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.middle = data16 | 0x01f8;
			break;

		case 0xa8:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 middle P chip select = %04X\n", activecpu_get_pc(), data16);
			i186.mem.middle_size = data16 | 0x8038;

			temp = (i186.mem.peripheral & 0xffc0) << 4;
			if (i186.mem.middle_size & 0x0040)
			{
				install_mem_read_handler(2, temp, temp + 0x2ff, peripheral_r);
				install_mem_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}
			else
			{
				temp &= 0xffff;
				install_port_read_handler(2, temp, temp + 0x2ff, peripheral_r);
				install_port_write_handler(2, temp, temp + 0x2ff, peripheral_w);
			}

			/* we need to do this at a time when the I86 context is swapped in */
			/* this register is generally set once at startup and never again, so it's a good */
			/* time to set it up */
			i86_set_irq_callback(int_callback);
			break;

		case 0xc0:
		case 0xd0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d lower source address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			i186.dma[which].source = (i186.dma[which].source & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0xc2:
		case 0xd2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d upper source address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			i186.dma[which].source = (i186.dma[which].source & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0xc4:
		case 0xd4:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d lower dest address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			i186.dma[which].dest = (i186.dma[which].dest & ~0x0ffff) | (data16 & 0x0ffff);
			break;

		case 0xc6:
		case 0xd6:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d upper dest address = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			i186.dma[which].dest = (i186.dma[which].dest & ~0xf0000) | ((data16 << 16) & 0xf0000);
			break;

		case 0xc8:
		case 0xd8:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d transfer count = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			i186.dma[which].count = data16;
			break;

		case 0xca:
		case 0xda:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 DMA%d control = %04X\n", activecpu_get_pc(), (offset - 0xc0) / 0x10, data16);
			which = (offset - 0xc0) / 0x10;
			stream_update(dma_stream, 0);
			update_dma_control(which, data16);
			break;

		case 0xfe:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 relocation register = %04X\n", activecpu_get_pc(), data16);

			/* we assume here there that this doesn't happen too often */
			/* plus, we can't really remove the old memory range, so we also assume that it's */
			/* okay to leave us mapped where we were */
			temp = (data16 & 0x0fff) << 8;
			if (data16 & 0x1000)
			{
				install_mem_read_handler(2, temp, temp + 0xff, i186_internal_port_r);
				install_mem_write_handler(2, temp, temp + 0xff, i186_internal_port_w);
			}
			else
			{
				temp &= 0xffff;
				install_port_read_handler(2, temp, temp + 0xff, i186_internal_port_r);
				install_port_write_handler(2, temp, temp + 0xff, i186_internal_port_w);
			}
/*			usrintf_showmessage("Sound CPU reset");*/
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:80186 port %02X = %04X\n", activecpu_get_pc(), offset, data16);
			break;
	}
}



/*************************************
 *
 *	8254 PIT accesses
 *
 *************************************/

static INLINE void counter_update_count(int which)
{
	/* only update if the timer is running */
	if (counter[which].timer)
	{
		/* determine how many 2MHz cycles are remaining */
		int count = (int)(timer_timeleft(counter[which].timer) / TIME_IN_HZ(2000000));
		counter[which].count = (count < 0) ? 0 : count;
	}
}


static READ_HANDLER( pit8254_r )
{
	struct counter_state *ctr;
	int which = offset / 0x80;
	int reg = (offset / 2) & 3;

	/* ignore odd offsets */
	if (offset & 1)
		return 0;

	/* switch off the register */
	switch (offset & 3)
	{
		case 0:
		case 1:
		case 2:
			/* warning: assumes LSB/MSB addressing and no latching! */
			which = (which * 3) + reg;
			ctr = &counter[which];

			/* update the count */
			counter_update_count(which);

			/* return the LSB */
			if (counter[which].readbyte == 0)
			{
				counter[which].readbyte = 1;
				return counter[which].count & 0xff;
			}

			/* write the MSB and reset the counter */
			else
			{
				counter[which].readbyte = 0;
				return (counter[which].count >> 8) & 0xff;
			}
			break;
	}
	return 0;
}


static WRITE_HANDLER( pit8254_w )
{
	struct counter_state *ctr;
	int which = offset / 0x80;
	int reg = (offset / 2) & 3;

	/* ignore odd offsets */
	if (offset & 1)
		return;

	/* switch off the register */
	switch (reg)
	{
		case 0:
		case 1:
		case 2:
			/* warning: assumes LSB/MSB addressing and no latching! */
			which = (which * 3) + reg;
			ctr = &counter[which];

			/* write the LSB */
			if (ctr->writebyte == 0)
			{
				ctr->count = (ctr->count & 0xff00) | (data & 0x00ff);
				ctr->writebyte = 1;
			}

			/* write the MSB and reset the counter */
			else
			{
				ctr->count = (ctr->count & 0x00ff) | ((data << 8) & 0xff00);
				ctr->writebyte = 0;

				/* treat 0 as $10000 */
				if (ctr->count == 0) ctr->count = 0x10000;

				/* reset/start the timer */
				timer_adjust(ctr->timer, TIME_NEVER, 0, 0);

				log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT counter %d set to %d (%d Hz)\n", which, ctr->count, 4000000 / ctr->count);

				/* set the frequency of the associated DAC */
				if (!is_redline)
					set_dac_frequency(which, 4000000 / ctr->count);
				else
				{
					if (which < 5)
						set_dac_frequency(which, 7000000 / ctr->count);
					else if (which == 6)
					{
						set_dac_frequency(5, 7000000 / ctr->count);
						set_dac_frequency(6, 7000000 / ctr->count);
						set_dac_frequency(7, 7000000 / ctr->count);
					}
				}
			}
			break;

		case 3:
			/* determine which counter */
			if ((data & 0xc0) == 0xc0) break;
			which = (which * 3) + (data >> 6);
			ctr = &counter[which];

			/* set the mode */
			ctr->mode = (data >> 1) & 7;
			break;
	}
}



/*************************************
 *
 *	External 80186 control
 *
 *************************************/

WRITE_HANDLER( leland_i86_control_w )
{
	/* see if anything changed */
	int diff = (last_control ^ data) & 0xf8;
	if (!diff)
		return;
	last_control = data;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04X:I86 control = %02X", activecpu_get_previouspc(), data);
		if (!(data & 0x80)) log_cb(RETRO_LOG_DEBUG, LOGPRE "  /RESET");
		if (!(data & 0x40)) log_cb(RETRO_LOG_DEBUG, LOGPRE "  ZNMI");
		if (!(data & 0x20)) log_cb(RETRO_LOG_DEBUG, LOGPRE "  INT0");
		if (!(data & 0x10)) log_cb(RETRO_LOG_DEBUG, LOGPRE "  /TEST");
		if (!(data & 0x08)) log_cb(RETRO_LOG_DEBUG, LOGPRE "  INT1");
		log_cb(RETRO_LOG_DEBUG, LOGPRE "\n");


	/* /RESET */
	cpu_set_reset_line(2, data & 0x80  ? CLEAR_LINE : ASSERT_LINE);

	/* /NMI */
/* 	If the master CPU doesn't get a response by the time it's ready to send
	the next command, it uses an NMI to force the issue; unfortunately, this
	seems to really screw up the sound system. It turns out it's better to
	just wait for the original interrupt to occur naturally */
/*	cpu_set_nmi_line  (2, data & 0x40  ? CLEAR_LINE : ASSERT_LINE);*/

	/* INT0 */
	if (data & 0x20)
	{
		if (!LATCH_INTS) i186.intr.request &= ~0x10;
	}
	else if (i186.intr.ext[0] & 0x10)
		i186.intr.request |= 0x10;
	else if (diff & 0x20)
		i186.intr.request |= 0x10;

	/* INT1 */
	if (data & 0x08)
	{
		if (!LATCH_INTS) i186.intr.request &= ~0x20;
	}
	else if (i186.intr.ext[1] & 0x10)
		i186.intr.request |= 0x20;
	else if (diff & 0x08)
		i186.intr.request |= 0x20;

	/* handle reset here */
	if ((diff & 0x80) && (data & 0x80))
		leland_i186_reset();

	update_interrupt_state();
}



/*************************************
 *
 *	Sound command handling
 *
 *************************************/

static void command_lo_sync(int data)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04X:Write sound command latch lo = %02X\n", activecpu_get_previouspc(), data);
	sound_command[0] = data;
}


WRITE_HANDLER( leland_i86_command_lo_w )
{
	timer_set(TIME_NOW, data, command_lo_sync);
}


WRITE_HANDLER( leland_i86_command_hi_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04X:Write sound command latch hi = %02X\n", activecpu_get_previouspc(), data);
	sound_command[1] = data;
}


static READ_HANDLER( main_to_sound_comm_r )
{
	if (!(offset & 1))
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Read sound command latch lo = %02X\n", activecpu_get_pc(), sound_command[0]);
		return sound_command[0];
	}
	else
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Read sound command latch hi = %02X\n", activecpu_get_pc(), sound_command[1]);
		return sound_command[1];
	}
}




/*************************************
 *
 *	Sound response handling
 *
 *************************************/

static void delayed_response_r(int checkpc)
{
	int pc = cpunum_get_reg(0, Z80_PC);
	int oldaf = cpunum_get_reg(0, Z80_AF);

	/* This is pretty cheesy, but necessary. Since the CPUs run in round-robin order,
	   synchronizing on the write to this register from the slave side does nothing.
	   In order to make sure the master CPU get the real response, we synchronize on
	   the read. However, the value we returned the first time around may not be
	   accurate, so after the system has synced up, we go back into the master CPUs
	   state and put the proper value into the A register. */
	if (pc == checkpc)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "(Updated sound response latch to %02X)\n", sound_response);

		oldaf = (oldaf & 0x00ff) | (sound_response << 8);
		cpunum_set_reg(0, Z80_AF, oldaf);
	}
	else
		log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: delayed_response_r - current PC = %04X, checkPC = %04X\n", pc, checkpc);
}


READ_HANDLER( leland_i86_response_r )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%04X:Read sound response latch = %02X\n", activecpu_get_previouspc(), sound_response);

	/* if sound is disabled, toggle between FF and 00 */
	if (Machine->sample_rate == 0)
		return sound_response ^= 0xff;
	else
	{
		/* synchronize the response */
		timer_set(TIME_NOW, activecpu_get_previouspc() + 2, delayed_response_r);
		return sound_response;
	}
}


static WRITE_HANDLER( sound_to_main_comm_w )
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Write sound response latch = %02X\n", activecpu_get_pc(), data);
	sound_response = data;
}



/*************************************
 *
 *	Low-level DAC I/O
 *
 *************************************/

static void set_dac_frequency(int which, int frequency)
{
	struct dac_state *d = &dac[which];
	int count = (d->bufin - d->bufout) & DAC_BUFFER_SIZE_MASK;

	/* set the frequency of the associated DAC */
	d->frequency = frequency;
	d->step = (int)((double)frequency * (double)(1 << 24) / (double)Machine->sample_rate);

	/* also determine the target buffer size */
	d->buftarget = dac[which].frequency / 60 + 50;
	if (d->buftarget > DAC_BUFFER_SIZE - 1)
		d->buftarget = DAC_BUFFER_SIZE - 1;

	/* reevaluate the count */
	if (count > d->buftarget)
		clock_active &= ~(1 << which);
	else if (count < d->buftarget)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "  - trigger due to clock active in set_dac_frequency\n");
		cpu_trigger(CPU_RESUME_TRIGGER);
		clock_active |= 1 << which;
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "DAC %d frequency = %d, step = %08X\n", which, d->frequency, d->step);
}


static WRITE_HANDLER( dac_w )
{
	int which = offset / 2;
	struct dac_state *d = &dac[which];

	/* handle value changes */
	if (!(offset & 1))
	{
		int count = (d->bufin - d->bufout) & DAC_BUFFER_SIZE_MASK;

		/* set the new value */
		d->value = (INT16)data - 0x80;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:DAC %d value = %02X\n", activecpu_get_pc(), offset / 2, data);

		/* if we haven't overflowed the buffer, add the value value to it */
		if (count < DAC_BUFFER_SIZE - 1)
		{
			/* if this is the first byte, sync the stream */
			if (count == 0)
				stream_update(nondma_stream, 0);

			/* prescale by the volume */
			d->buffer[d->bufin] = d->value * d->volume;
			d->bufin = (d->bufin + 1) & DAC_BUFFER_SIZE_MASK;

			/* update the clock status */
			if (++count > d->buftarget)
				clock_active &= ~(1 << which);
		}
	}

	/* handle volume changes */
	else
	{
		d->volume = (data ^ 0x00) / DAC_VOLUME_SCALE;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:DAC %d volume = %02X\n", activecpu_get_pc(), offset / 2, data);
	}
}


static WRITE_HANDLER( redline_dac_w )
{
	int which = offset / 0x200;
	struct dac_state *d = &dac[which];
	int count = (d->bufin - d->bufout) & DAC_BUFFER_SIZE_MASK;

	/* set the new value */
	d->value = (INT16)data - 0x80;

	/* if we haven't overflowed the buffer, add the value value to it */
	if (count < DAC_BUFFER_SIZE - 1)
	{
		/* if this is the first byte, sync the stream */
		if (count == 0)
			stream_update(nondma_stream, 0);

		/* prescale by the volume */
		d->buffer[d->bufin] = d->value * d->volume;
		d->bufin = (d->bufin + 1) & DAC_BUFFER_SIZE_MASK;

		/* update the clock status */
		if (++count > d->buftarget)
			clock_active &= ~(1 << which);
	}

	/* update the volume */
	d->volume = (offset & 0x1fe) / 2 / DAC_VOLUME_SCALE;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:DAC %d value = %02X, volume = %02X\n", activecpu_get_pc(), which, data, (offset & 0x1fe) / 2);
}


static WRITE_HANDLER( dac_10bit_w )
{
	static UINT8 even_byte;
	struct dac_state *d = &dac[6];
	int count = (d->bufin - d->bufout) & DAC_BUFFER_SIZE_MASK;
	int data16;

	/* warning: this assumes all port writes here are word-sized */
	/* if the offset is even, just stash the value */
	if (!(offset & 1))
	{
		even_byte = data;
		return;
	}
	data16 = (data << 8) | even_byte;

	/* set the new value */
	d->value = (INT16)data16 - 0x200;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:DAC 10-bit value = %02X\n", activecpu_get_pc(), data16);

	/* if we haven't overflowed the buffer, add the value value to it */
	if (count < DAC_BUFFER_SIZE - 1)
	{
		/* if this is the first byte, sync the stream */
		if (count == 0)
			stream_update(nondma_stream, 0);

		/* prescale by the volume */
		d->buffer[d->bufin] = d->value * (0xff / DAC_VOLUME_SCALE / 2);
		d->bufin = (d->bufin + 1) & DAC_BUFFER_SIZE_MASK;

		/* update the clock status */
		if (++count > d->buftarget)
			clock_active &= ~0x40;
	}
}


static WRITE_HANDLER( ataxx_dac_control )
{
	/* handle common offsets */
	switch (offset)
	{
		case 0x00:
		case 0x02:
		case 0x04:
			dac_w(offset, data);
			return;

		case 0x06:
			dac_w(1, ((data << 5) & 0xe0) | ((data << 2) & 0x1c) | (data & 0x03));
			dac_w(3, ((data << 2) & 0xe0) | ((data >> 1) & 0x1c) | ((data >> 4) & 0x03));
			dac_w(5, (data & 0xc0) | ((data >> 2) & 0x30) | ((data >> 4) & 0x0c) | ((data >> 6) & 0x03));
			return;
	}

	/* if we have a YM2151 (and an external DAC), handle those offsets */
	if (has_ym2151)
	{
		stream_update(extern_stream, 0);
		switch (offset)
		{
			case 0x08:
			case 0x09:
				ext_active = 1;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC active\n");
				return;

			case 0x0a:
			case 0x0b:
				ext_active = 0;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC inactive\n");
				return;

			case 0x0c:
				ext_start = (ext_start & 0xff00f) | ((data << 4) & 0x00ff0);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC start = %05X\n", ext_start);
				return;

			case 0x0d:
				ext_start = (ext_start & 0x00fff) | ((data << 12) & 0xff000);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC start = %05X\n", ext_start);
				return;

			case 0x0e:
				ext_stop = (ext_stop & 0xff00f) | ((data << 4) & 0x00ff0);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC stop = %05X\n", ext_stop);
				return;

			case 0x0f:
				ext_stop = (ext_stop & 0x00fff) | ((data << 12) & 0xff000);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "External DAC stop = %05X\n", ext_stop);
				return;

			case 0x42:
			case 0x43:
				dac_w(offset - 0x42 + 14, data);
				return;
		}
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Unexpected peripheral write %d/%02X = %02X\n", activecpu_get_pc(), 5, offset, data);
}



/*************************************
 *
 *	Peripheral chip dispatcher
 *
 *************************************/

static READ_HANDLER( peripheral_r )
{
	int select = offset / 0x80;
	offset &= 0x7f;

	switch (select)
	{
		case 0:
			if (offset & 1)
				return 0;

			/* we have to return 0 periodically so that they handle interrupts */
			if ((++clock_tick & 7) == 0)
				return 0;

			/* if we've filled up all the active channels, we can give this CPU a reset */
			/* until the next interrupt */
			{
				UINT8 result;

				if (!is_redline)
					result = ((clock_active >> 1) & 0x3e);
				else
					result = ((clock_active << 1) & 0x7e);

				if (!i186.intr.pending && active_mask && (*active_mask & result) == 0 && ++total_reads > 100)
				{
					log_cb(RETRO_LOG_DEBUG, LOGPRE "Suspended CPU: active_mask = %02X, result = %02X\n", *active_mask, result);
					cpu_spinuntil_trigger(CPU_RESUME_TRIGGER);
				}
				else if (LOG_OPTIMIZATION)
				{
					if (i186.intr.pending) log_cb(RETRO_LOG_DEBUG, LOGPRE "(can't suspend - interrupt pending)\n");
					else if (active_mask && (*active_mask & result) != 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "(can't suspend: mask=%02X result=%02X\n", *active_mask, result);
				}

				return result;
			}

		case 1:
			return main_to_sound_comm_r(offset);

		case 2:
			return pit8254_r(offset);

		case 3:
			if (!has_ym2151)
				return pit8254_r(offset | 0x80);
			else
				return (offset & 1) ? 0 : YM2151_status_port_0_r(offset);

		case 4:
			if (is_redline)
				return pit8254_r(offset | 0x100);
			else
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Unexpected peripheral read %d/%02X\n", activecpu_get_pc(), select, offset);
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Unexpected peripheral read %d/%02X\n", activecpu_get_pc(), select, offset);
			break;
	}
	return 0xff;
}


static WRITE_HANDLER( peripheral_w )
{
	int select = offset / 0x80;
	offset &= 0x7f;

	switch (select)
	{
		case 1:
			sound_to_main_comm_w(offset, data);
			break;

		case 2:
			pit8254_w(offset, data);
			break;

		case 3:
			if (!has_ym2151)
				pit8254_w(offset | 0x80, data);
			else if (offset == 0)
				YM2151_register_port_0_w(offset, data);
			else if (offset == 2)
				YM2151_data_port_0_w(offset, data);
			break;

		case 4:
			if (is_redline)
				pit8254_w(offset | 0x100, data);
			else
				dac_10bit_w(offset, data);
			break;

		case 5:	/* Ataxx/WSF/Indy Heat only */
			ataxx_dac_control(offset, data);
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "%05X:Unexpected peripheral write %d/%02X = %02X\n", activecpu_get_pc(), select, offset, data);
			break;
	}
}



/*************************************
 *
 *	Optimizations
 *
 *************************************/

void leland_i86_optimize_address(offs_t offset)
{
	if (offset)
		active_mask = memory_region(REGION_CPU3) + offset;
	else
		active_mask = NULL;
}



/*************************************
 *
 *	Game-specific handlers
 *
 *************************************/

WRITE_HANDLER( ataxx_i86_control_w )
{
	/* compute the bit-shuffled variants of the bits and then write them */
	int modified = 	((data & 0x01) << 7) |
					((data & 0x02) << 5) |
					((data & 0x04) << 3) |
					((data & 0x08) << 1);
	leland_i86_control_w(offset, modified);
}



/*************************************
 *
 *	Sound CPU memory handlers
 *
 *************************************/

MEMORY_READ_START( leland_i86_readmem )
	{ 0x00000, 0x03fff, MRA_RAM },
	{ 0x0c000, 0x0ffff, MRA_BANK6 },	/* used by Ataxx */
	{ 0x1c000, 0x1ffff, MRA_BANK7 },	/* used by Super Offroad */
	{ 0x20000, 0xfffff, MRA_ROM },
MEMORY_END


MEMORY_WRITE_START( leland_i86_writemem )
	{ 0x00000, 0x03fff, MWA_RAM, &ram_base },
	{ 0x0c000, 0x0ffff, MWA_BANK6 },
	{ 0x1c000, 0x1ffff, MWA_BANK7 },
	{ 0x20000, 0xfffff, MWA_ROM },
MEMORY_END


PORT_READ_START( leland_i86_readport )
	{ 0xff00, 0xffff, i186_internal_port_r },
PORT_END


PORT_WRITE_START( redline_i86_writeport )
	{ 0x6000, 0x6fff, redline_dac_w },
	{ 0xff00, 0xffff, i186_internal_port_w },
PORT_END


PORT_WRITE_START( leland_i86_writeport )
	{ 0x0000, 0x000b, dac_w },
	{ 0x0080, 0x008b, dac_w },
	{ 0x00c0, 0x00cb, dac_w },
	{ 0xff00, 0xffff, i186_internal_port_w },
PORT_END


PORT_WRITE_START( ataxx_i86_writeport )
	{ 0xff00, 0xffff, i186_internal_port_w },
PORT_END


/************************************************************************

Memory configurations:

	Redline Racer:
		FFDF7:80186 upper chip select = E03C		-> E0000-FFFFF, 128k long
		FFDF7:80186 lower chip select = 00FC		-> 00000-00FFF, 4k long
		FFDF7:80186 peripheral chip select = 013C	-> 01000, 01080, 01100, 01180, 01200, 01280, 01300
		FFDF7:80186 middle chip select = 81FC		-> 80000-C0000, 64k chunks, 256k total
		FFDF7:80186 middle P chip select = A0FC

	Quarterback, Team Quarterback, AAFB, Super Offroad, Track Pack, Pigout, Viper:
		FFDFA:80186 upper chip select = E03C		-> E0000-FFFFF, 128k long
		FFDFA:80186 peripheral chip select = 203C	-> 20000, 20080, 20100, 20180, 20200, 20280, 20300
		FFDFA:80186 middle chip select = 01FC		-> 00000-7FFFF, 128k chunks, 512k total
		FFDFA:80186 middle P chip select = C0FC

	Ataxx, Indy Heat, World Soccer Finals:
		FFD9D:80186 upper chip select = E03C		-> E0000-FFFFF, 128k long
		FFD9D:80186 peripheral chip select = 043C	-> 04000, 04080, 04100, 04180, 04200, 04280, 04300
		FFD9D:80186 middle chip select = 01FC		-> 00000-7FFFF, 128k chunks, 512k total
		FFD9D:80186 middle P chip select = C0BC

************************************************************************/
