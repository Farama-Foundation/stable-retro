/***************************************************************************

	Midway MCR system

***************************************************************************/

#include <stdio.h>

#include "driver.h"
#include "machine/z80fmly.h"
#include "sndhrdw/mcr.h"
#include "cpu/m6800/m6800.h"
#include "cpu/m6809/m6809.h"
#include "cpu/z80/z80.h"
#include "mcr.h"


#define LOG(x)


/*************************************
 *
 *	Global variables
 *
 *************************************/

double mcr68_timing_factor;

UINT8 mcr_cocktail_flip;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 m6840_status;
static UINT8 m6840_status_read_since_int;
static UINT8 m6840_msb_buffer;
static UINT8 m6840_lsb_buffer;
static struct counter_state
{
	UINT8	control;
	UINT16	latch;
	UINT16	count;
	void *	timer;
	UINT8	timer_active;
	double	period;
} m6840_state[3];

/* MCR/68k interrupt states */
static UINT8 m6840_irq_state;
static UINT8 m6840_irq_vector;
static UINT8 v493_irq_state;
static UINT8 v493_irq_vector;

static void (*v493_callback)(int param);

static UINT8 zwackery_sound_data;

static const double m6840_counter_periods[3] = { 1.0 / 30.0, 1000000.0, 1.0 / (512.0 * 30.0) };
static double m6840_internal_counter_period;	/* 68000 CLK / 10 */



/*************************************
 *
 *	Function prototypes
 *
 *************************************/

static void subtract_from_counter(int counter, int count);

static void mcr68_493_callback(int param);
static void zwackery_493_callback(int param);

static WRITE_HANDLER( zwackery_pia_2_w );
static WRITE_HANDLER( zwackery_pia_3_w );
static WRITE_HANDLER( zwackery_ca2_w );
static void zwackery_pia_irq(int state);

static void reload_count(int counter);
static void counter_fired_callback(int counter);



/*************************************
 *
 *	Graphics declarations
 *
 *************************************/

struct GfxLayout mcr_bg_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+1, 0, 1 },
	{  0,  0,  2,  2,  4,  4,  6,  6,
	   8,  8, 10, 10, 12, 12, 14, 14 },
	{ 0*8,  0*8,  2*8,  2*8,
	  4*8,  4*8,  6*8,  6*8,
	  8*8,  8*8, 10*8, 10*8,
	 12*8, 12*8, 14*8, 14*8 },
	16*8
};


struct GfxLayout mcr_sprite_layout =
{
	32,32,
	RGN_FRAC(1,4),
	4,
	{ 0, 1, 2, 3 },
	{ 0, 4,
	  RGN_FRAC(1,4)+0, RGN_FRAC(1,4)+4,
	  RGN_FRAC(2,4)+0, RGN_FRAC(2,4)+4,
	  RGN_FRAC(3,4)+0, RGN_FRAC(3,4)+4,
	  8, 12,
	  RGN_FRAC(1,4)+8, RGN_FRAC(1,4)+12,
	  RGN_FRAC(2,4)+8, RGN_FRAC(2,4)+12,
	  RGN_FRAC(3,4)+8, RGN_FRAC(3,4)+12,
	  16, 20,
	  RGN_FRAC(1,4)+16, RGN_FRAC(1,4)+20,
	  RGN_FRAC(2,4)+16, RGN_FRAC(2,4)+20,
	  RGN_FRAC(3,4)+16, RGN_FRAC(3,4)+20,
	  24, 28,
	  RGN_FRAC(1,4)+24, RGN_FRAC(1,4)+28,
	  RGN_FRAC(2,4)+24, RGN_FRAC(2,4)+28,
	  RGN_FRAC(3,4)+24, RGN_FRAC(3,4)+28 },
	{ 32*0,  32*1,  32*2,  32*3,
	  32*4,  32*5,  32*6,  32*7,
	  32*8,  32*9,  32*10, 32*11,
	  32*12, 32*13, 32*14, 32*15,
	  32*16, 32*17, 32*18, 32*19,
	  32*20, 32*21, 32*22, 32*23,
	  32*24, 32*25, 32*26, 32*27,
	  32*28, 32*29, 32*30, 32*31 },
	32*32
};



/*************************************
 *
 *	6821 PIA declarations
 *
 *************************************/

READ_HANDLER( zwackery_port_2_r );

static struct pia6821_interface zwackery_pia_2_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ 0, input_port_0_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zwackery_pia_2_w, 0, 0, 0,
	/*irqs   : A/B             */ zwackery_pia_irq, zwackery_pia_irq
};

static struct pia6821_interface zwackery_pia_3_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_1_r, zwackery_port_2_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ zwackery_pia_3_w, 0, zwackery_ca2_w, 0,
	/*irqs   : A/B             */ 0, 0
};

static struct pia6821_interface zwackery_pia_4_intf =
{
	/*inputs : A/B,CA/B1,CA/B2 */ input_port_3_r, input_port_4_r, 0, 0, 0, 0,
	/*outputs: A/B,CA/B2       */ 0, 0, 0, 0,
	/*irqs   : A/B             */ 0, 0
};



/*************************************
 *
 *	Generic MCR CTC interface
 *
 *************************************/

static void ctc_interrupt(int state)
{
	cpu_set_irq_line_and_vector(0, 0, HOLD_LINE, Z80_VECTOR(0, state));
}


Z80_DaisyChain mcr_daisy_chain[] =
{
	{ z80ctc_reset, z80ctc_interrupt, z80ctc_reti, 0 }, /* CTC number 0 */
	{ 0, 0, 0, -1 }		/* end mark */
};


static z80ctc_interface ctc_intf =
{
	1,                  /* 1 chip */
	{ 0 },              /* clock (filled in from the CPU 0 clock) */
	{ 0 },              /* timer disables */
	{ ctc_interrupt },  /* interrupt handler */
	{ z80ctc_0_trg1_w },/* ZC/TO0 callback */
	{ 0 },              /* ZC/TO1 callback */
	{ 0 }               /* ZC/TO2 callback */
};



/*************************************
 *
 *	Generic MCR machine initialization
 *
 *************************************/

MACHINE_INIT( mcr )
{
	/* initialize the CTC */
	ctc_intf.baseclock[0] = Machine->drv->cpu[0].cpu_clock;
	z80ctc_init(&ctc_intf);

	/* reset cocktail flip */
	mcr_cocktail_flip = 0;

	/* initialize the sound */
	mcr_sound_init();
}



/*************************************
 *
 *	Generic MCR/68k machine initialization
 *
 *************************************/

static void mcr68_common_init(void)
{
	int i;

	/* reset the 6840's */
	m6840_status = 0x00;
	m6840_status_read_since_int = 0x00;
	m6840_msb_buffer = m6840_lsb_buffer = 0;
	for (i = 0; i < 3; i++)
	{
		m6840_state[i].control = 0x00;
		m6840_state[i].latch = 0xffff;
		m6840_state[i].count = 0xffff;
		m6840_state[i].timer = timer_alloc(counter_fired_callback);
		m6840_state[i].timer_active = 0;
		m6840_state[i].period = m6840_counter_periods[i];
	}

	/* initialize the clock */
	m6840_internal_counter_period = TIME_IN_HZ(Machine->drv->cpu[0].cpu_clock / 10);

	/* reset cocktail flip */
	mcr_cocktail_flip = 0;

	/* initialize the sound */
	pia_unconfig();
	mcr_sound_init();
}


MACHINE_INIT( mcr68 )
{
	/* for the most part all MCR/68k games are the same */
	mcr68_common_init();
	v493_callback = mcr68_493_callback;

	/* vectors are 1 and 2 */
	v493_irq_vector = 1;
	m6840_irq_vector = 2;
}


MACHINE_INIT( zwackery )
{
	/* for the most part all MCR/68k games are the same */
	mcr68_common_init();
	v493_callback = zwackery_493_callback;

	/* append our PIA state onto the existing one and reinit */
	pia_config(2, PIA_STANDARD_ORDERING, &zwackery_pia_2_intf);
	pia_config(3, PIA_STANDARD_ORDERING, &zwackery_pia_3_intf);
	pia_config(4, PIA_STANDARD_ORDERING, &zwackery_pia_4_intf);
	pia_reset();

	/* vectors are 5 and 6 */
	v493_irq_vector = 5;
	m6840_irq_vector = 6;
}



/*************************************
 *
 *	Generic MCR interrupt handler
 *
 *************************************/

INTERRUPT_GEN( mcr_interrupt )
{
	/* CTC line 2 is connected to VBLANK, which is once every 1/2 frame */
	/* for the 30Hz interlaced display */
	z80ctc_0_trg2_w(0, 1);
	z80ctc_0_trg2_w(0, 0);

	/* CTC line 3 is connected to 493, which is signalled once every */
	/* frame at 30Hz */
	if (cpu_getiloops() == 0)
	{
		z80ctc_0_trg3_w(0, 1);
		z80ctc_0_trg3_w(0, 0);
	}
}


INTERRUPT_GEN( mcr68_interrupt )
{
	/* update the 6840 VBLANK clock */
	if (!m6840_state[0].timer_active)
		subtract_from_counter(0, 1);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "--- VBLANK ---\n");

	/* also set a timer to generate the 493 signal at a specific time before the next VBLANK */
	/* the timing of this is crucial for Blasted and Tri-Sports, which check the timing of */
	/* VBLANK and 493 using counter 2 */
	timer_set(TIME_IN_HZ(30) - mcr68_timing_factor, 0, v493_callback);
}



/*************************************
 *
 *	MCR/68k interrupt central
 *
 *************************************/

static void update_mcr68_interrupts(void)
{
	int newstate = 0;

	/* all interrupts go through an LS148, which gives priority to the highest */
	if (v493_irq_state)
		newstate = v493_irq_vector;
	if (m6840_irq_state)
		newstate = m6840_irq_vector;

	/* set the new state of the IRQ lines */
	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


static void mcr68_493_off_callback(int param)
{
	v493_irq_state = 0;
	update_mcr68_interrupts();
}


static void mcr68_493_callback(int param)
{
	v493_irq_state = 1;
	update_mcr68_interrupts();
	timer_set(cpu_getscanlineperiod(), 0, mcr68_493_off_callback);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "--- (INT1) ---\n");
}



/*************************************
 *
 *	Generic MCR port write handlers
 *
 *************************************/

WRITE_HANDLER( mcr_control_port_w )
{
	/*
		Bit layout is as follows:
			D7 = n/c
			D6 = cocktail flip
			D5 = red LED
			D4 = green LED
			D3 = n/c
			D2 = coin meter 3
			D1 = coin meter 2
			D0 = coin meter 1
	*/

	coin_counter_w(0, (data >> 0) & 1);
	coin_counter_w(1, (data >> 1) & 1);
	coin_counter_w(2, (data >> 2) & 1);
	mcr_cocktail_flip = (data >> 6) & 1;
}


WRITE_HANDLER( mcrmono_control_port_w )
{
	/*
		Bit layout is as follows:
			D7 = n/c
			D6 = cocktail flip
			D5 = n/c
			D4 = n/c
			D3 = n/c
			D2 = n/c
			D1 = n/c
			D0 = coin meter 1
	*/

	coin_counter_w(0, (data >> 0) & 1);
	mcr_cocktail_flip = (data >> 6) & 1;
}


WRITE_HANDLER( mcr_scroll_value_w )
{
	switch (offset)
	{
		case 0:
			/* low 8 bits of horizontal scroll */
			spyhunt_scrollx = (spyhunt_scrollx & ~0xff) | data;
			break;

		case 1:
			/* upper 3 bits of horizontal scroll and upper 1 bit of vertical scroll */
			spyhunt_scrollx = (spyhunt_scrollx & 0xff) | ((data & 0x07) << 8);
			spyhunt_scrolly = (spyhunt_scrolly & 0xff) | ((data & 0x80) << 1);
			break;

		case 2:
			/* low 8 bits of vertical scroll */
			spyhunt_scrolly = (spyhunt_scrolly & ~0xff) | data;
			break;
	}
}



/*************************************
 *
 *	Zwackery-specific interfaces
 *
 *************************************/

WRITE_HANDLER( zwackery_pia_2_w )
{
	/* bit 7 is the watchdog */
	if (!(data & 0x80)) watchdog_reset_w(offset, data);

	/* bits 5 and 6 control hflip/vflip */
	/* bits 3 and 4 control coin counters? */
	/* bits 0, 1 and 2 control meters? */
}


WRITE_HANDLER( zwackery_pia_3_w )
{
	zwackery_sound_data = (data >> 4) & 0x0f;
}


WRITE_HANDLER( zwackery_ca2_w )
{
	csdeluxe_data_w(offset, (data << 4) | zwackery_sound_data);
}


void zwackery_pia_irq(int state)
{
	v493_irq_state = state;
	update_mcr68_interrupts();
}


static void zwackery_493_off_callback(int param)
{
	pia_2_ca1_w(0, 0);
}


static void zwackery_493_callback(int param)
{
	pia_2_ca1_w(0, 1);
	timer_set(cpu_getscanlineperiod(), 0, zwackery_493_off_callback);
}



/*************************************
 *
 *	M6840 timer utilities
 *
 *************************************/

static INLINE void update_interrupts(void)
{
	m6840_status &= ~0x80;

	if ((m6840_status & 0x01) && (m6840_state[0].control & 0x40)) m6840_status |= 0x80;
	if ((m6840_status & 0x02) && (m6840_state[1].control & 0x40)) m6840_status |= 0x80;
	if ((m6840_status & 0x04) && (m6840_state[2].control & 0x40)) m6840_status |= 0x80;

	m6840_irq_state = m6840_status >> 7;
	update_mcr68_interrupts();
}


static void subtract_from_counter(int counter, int count)
{
	/* dual-byte mode */
	if (m6840_state[counter].control & 0x04)
	{
		int lsb = m6840_state[counter].count & 0xff;
		int msb = m6840_state[counter].count >> 8;

		/* count the clocks */
		lsb -= count;

		/* loop while we're less than zero */
		while (lsb < 0)
		{
			/* borrow from the MSB */
			lsb += (m6840_state[counter].latch & 0xff) + 1;
			msb--;

			/* if MSB goes less than zero, we've expired */
			if (msb < 0)
			{
				m6840_status |= 1 << counter;
				m6840_status_read_since_int &= ~(1 << counter);
				update_interrupts();
				msb = (m6840_state[counter].latch >> 8) + 1;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "** Counter %d fired\n", counter);
			}
		}

		/* store the result */
		m6840_state[counter].count = (msb << 8) | lsb;
	}

	/* word mode */
	else
	{
		int word = m6840_state[counter].count;

		/* count the clocks */
		word -= count;

		/* loop while we're less than zero */
		while (word < 0)
		{
			/* borrow from the MSB */
			word += m6840_state[counter].latch + 1;

			/* we've expired */
			m6840_status |= 1 << counter;
			m6840_status_read_since_int &= ~(1 << counter);
			update_interrupts();
			log_cb(RETRO_LOG_DEBUG, LOGPRE "** Counter %d fired\n", counter);
		}

		/* store the result */
		m6840_state[counter].count = word;
	}
}


static void counter_fired_callback(int counter)
{
	int count = counter >> 2;
	counter &= 3;

	/* reset the timer */
	m6840_state[counter].timer_active = 0;

	/* subtract it all from the counter; this will generate an interrupt */
	subtract_from_counter(counter, count);
}


static void reload_count(int counter)
{
	double period;
	int count;

	/* copy the latched value in */
	m6840_state[counter].count = m6840_state[counter].latch;

	/* counter 0 is self-updating if clocked externally */
	if (counter == 0 && !(m6840_state[counter].control & 0x02))
	{
		timer_adjust(m6840_state[counter].timer, TIME_NEVER, 0, 0);
		m6840_state[counter].timer_active = 0;
		return;
	}

	/* determine the clock period for this timer */
	if (m6840_state[counter].control & 0x02)
		period = m6840_internal_counter_period;
	else
		period = m6840_counter_periods[counter];

	/* determine the number of clock periods before we expire */
	count = m6840_state[counter].count;
	if (m6840_state[counter].control & 0x04)
		count = ((count >> 8) + 1) * ((count & 0xff) + 1);
	else
		count = count + 1;

	/* set the timer */
	timer_adjust(m6840_state[counter].timer, period * (double)count, (count << 2) + counter, 0);
	m6840_state[counter].timer_active = 1;
}


static UINT16 compute_counter(int counter)
{
	double period;
	int remaining;

	/* if there's no timer, return the count */
	if (!m6840_state[counter].timer_active)
		return m6840_state[counter].count;

	/* determine the clock period for this timer */
	if (m6840_state[counter].control & 0x02)
		period = m6840_internal_counter_period;
	else
		period = m6840_counter_periods[counter];

	/* see how many are left */
	remaining = (int)(timer_timeleft(m6840_state[counter].timer) / period);

	/* adjust the count for dual byte mode */
	if (m6840_state[counter].control & 0x04)
	{
		int divisor = (m6840_state[counter].count & 0xff) + 1;
		int msb = remaining / divisor;
		int lsb = remaining % divisor;
		remaining = (msb << 8) | lsb;
	}

	return remaining;
}



/*************************************
 *
 *	M6840 timer I/O
 *
 *************************************/

static WRITE_HANDLER( mcr68_6840_w_common )
{
	int i;

	/* offsets 0 and 1 are control registers */
	if (offset < 2)
	{
		int counter = (offset == 1) ? 1 : (m6840_state[1].control & 0x01) ? 0 : 2;
		UINT8 diffs = data ^ m6840_state[counter].control;

		m6840_state[counter].control = data;

		/* reset? */
		if (counter == 0 && (diffs & 0x01))
		{
			/* holding reset down */
			if (data & 0x01)
			{
				for (i = 0; i < 3; i++)
				{
					timer_adjust(m6840_state[i].timer, TIME_NEVER, 0, 0);
					m6840_state[i].timer_active = 0;
				}
			}

			/* releasing reset */
			else
			{
				for (i = 0; i < 3; i++)
					reload_count(i);
			}

			m6840_status = 0;
			update_interrupts();
		}

		/* changing the clock source? (needed for Zwackery) */
		if (diffs & 0x02)
			reload_count(counter);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Counter %d control = %02X\n", activecpu_get_previouspc(), counter, data);
	}

	/* offsets 2, 4, and 6 are MSB buffer registers */
	else if ((offset & 1) == 0)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:MSB = %02X\n", activecpu_get_previouspc(), data);
		m6840_msb_buffer = data;
	}

	/* offsets 3, 5, and 7 are Write Timer Latch commands */
	else
	{
		int counter = (offset - 2) / 2;
		m6840_state[counter].latch = (m6840_msb_buffer << 8) | (data & 0xff);

		/* clear the interrupt */
		m6840_status &= ~(1 << counter);
		update_interrupts();

		/* reload the count if in an appropriate mode */
		if (!(m6840_state[counter].control & 0x10))
			reload_count(counter);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Counter %d latch = %04X\n", activecpu_get_previouspc(), counter, m6840_state[counter].latch);
	}
}


static READ16_HANDLER( mcr68_6840_r_common )
{
	/* offset 0 is a no-op */
	if (offset == 0)
		return 0;

	/* offset 1 is the status register */
	else if (offset == 1)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Status read = %04X\n", activecpu_get_previouspc(), m6840_status);
		m6840_status_read_since_int |= m6840_status & 0x07;
		return m6840_status;
	}

	/* offsets 2, 4, and 6 are Read Timer Counter commands */
	else if ((offset & 1) == 0)
	{
		int counter = (offset - 2) / 2;
		int result = compute_counter(counter);

		/* clear the interrupt if the status has been read */
		if (m6840_status_read_since_int & (1 << counter))
			m6840_status &= ~(1 << counter);
		update_interrupts();

		m6840_lsb_buffer = result & 0xff;

		log_cb(RETRO_LOG_DEBUG, LOGPRE "%06X:Counter %d read = %04X\n", activecpu_get_previouspc(), counter, result);
		return result >> 8;
	}

	/* offsets 3, 5, and 7 are LSB buffer registers */
	else
		return m6840_lsb_buffer;
}


WRITE16_HANDLER( mcr68_6840_upper_w )
{
	if (ACCESSING_MSB)
		mcr68_6840_w_common(offset, (data >> 8) & 0xff);
}


WRITE16_HANDLER( mcr68_6840_lower_w )
{
	if (ACCESSING_LSB)
		mcr68_6840_w_common(offset, data & 0xff);
}


READ16_HANDLER( mcr68_6840_upper_r )
{
	return (mcr68_6840_r_common(offset,0) << 8) | 0x00ff;
}


READ16_HANDLER( mcr68_6840_lower_r )
{
	return mcr68_6840_r_common(offset,0) | 0xff00;
}
