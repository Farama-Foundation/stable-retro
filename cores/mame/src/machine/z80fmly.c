/***************************************************************************

  Z80 FMLY.C   Z80 FAMILY CHIP EMURATOR for MAME Ver.0.1 alpha

  Support chip :  Z80PIO , Z80CTC

  Copyright(C) 1997 Tatsuyuki Satoh.

  This version are tested starforce driver.

  8/21/97 -- Heavily modified by Aaron Giles to be much more accurate for the MCR games
  8/27/97 -- Rewritten a second time by Aaron Giles, with the datasheets in hand

pending:
	Z80CTC , Counter mode & Timer with Trigrt start :not support Triger level

***************************************************************************/

#include <stdio.h>
#include "driver.h"
#include "z80fmly.h"
#include "cpu/z80/z80.h"

typedef struct
{
	int vector;                 /* interrupt vector */
	int clock;                  /* system clock */
	double invclock16;          /* 16/system clock */
	double invclock256;         /* 256/system clock */
	void (*intr)(int which);    /* interrupt callback */
	mem_write_handler zc[4];    /* zero crossing callbacks */
	int notimer;                /* no timer masks */
	int mask[4];                /* masked channel flags */
	int mode[4];                /* current mode */
	int tconst[4];              /* time constant */
	int down[4];                /* down counter (clock mode only) */
	int extclk[4];              /* current signal from the external clock */
	void *timer[4];             /* array of active timers */

	int int_state[4];           /* interrupt status (for daisy chain) */
} z80ctc;

static z80ctc ctcs[MAX_CTC];


/* these are the bits of the incoming commands to the CTC */
#define INTERRUPT			0x80
#define INTERRUPT_ON 	0x80
#define INTERRUPT_OFF	0x00

#define MODE				0x40
#define MODE_TIMER		0x00
#define MODE_COUNTER		0x40

#define PRESCALER			0x20
#define PRESCALER_256	0x20
#define PRESCALER_16		0x00

#define EDGE				0x10
#define EDGE_FALLING		0x00
#define EDGE_RISING		0x10

#define TRIGGER			0x08
#define TRIGGER_AUTO		0x00
#define TRIGGER_CLOCK	0x08

#define CONSTANT			0x04
#define CONSTANT_LOAD	0x04
#define CONSTANT_NONE	0x00

#define RESET				0x02
#define RESET_CONTINUE	0x00
#define RESET_ACTIVE		0x02

#define CONTROL			0x01
#define CONTROL_VECTOR	0x00
#define CONTROL_WORD		0x01

/* these extra bits help us keep things accurate */
#define WAITING_FOR_TRIG	0x100


static void z80ctc_timercallback (int param);


void z80ctc_init (z80ctc_interface *intf)
{
	int i;

	memset (ctcs, 0, sizeof (ctcs));

	for (i = 0; i < intf->num; i++)
	{
		ctcs[i].clock = intf->baseclock[i];
		ctcs[i].invclock16 = 16.0 / (double)intf->baseclock[i];
		ctcs[i].invclock256 = 256.0 / (double)intf->baseclock[i];
		ctcs[i].notimer = intf->notimer[i];
		ctcs[i].intr = intf->intr[i];
		ctcs[i].timer[0] = timer_alloc(z80ctc_timercallback);
		ctcs[i].timer[1] = timer_alloc(z80ctc_timercallback);
		ctcs[i].timer[2] = timer_alloc(z80ctc_timercallback);
		ctcs[i].timer[3] = timer_alloc(z80ctc_timercallback);
		ctcs[i].zc[0] = intf->zc0[i];
		ctcs[i].zc[1] = intf->zc1[i];
		ctcs[i].zc[2] = intf->zc2[i];
		ctcs[i].zc[3] = 0;
		z80ctc_reset (i);
	}
}


double z80ctc_getperiod (int which, int ch)
{
	z80ctc *ctc = ctcs + which;
	double clock;
	int mode;

	/* keep channel within range, and get the current mode */
	ch &= 3;
	mode = ctc->mode[ch];

	/* if reset active */
	if( (mode & RESET) == RESET_ACTIVE) return 0;
	/* if counter mode */
	if( (mode & MODE) == MODE_COUNTER)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC %d is CounterMode : Can't calcrate period\n", ch );
		return 0;
	}

	/* compute the period */
	clock = ((mode & PRESCALER) == PRESCALER_16) ? ctc->invclock16 : ctc->invclock256;
	return clock * (double)ctc->tconst[ch];
}

/* interrupt request callback with daisy-chain circuit */
static void z80ctc_interrupt_check( z80ctc *ctc )
{
	int state = 0;
	int ch;

	for( ch = 3 ; ch >= 0 ; ch-- )
	{
		/* if IEO disable , same and lower IRQ is masking */
/* ASG: changed this line because this state could have an interrupt pending as well! */
/*		if( ctc->int_state[ch] & Z80_INT_IEO ) state  = Z80_INT_IEO;*/
		if( ctc->int_state[ch] & Z80_INT_IEO ) state  = ctc->int_state[ch];
		else                                   state |= ctc->int_state[ch];
	}
	/* change interrupt status */
	if (ctc->intr) (*ctc->intr)(state);
}


void z80ctc_reset (int which)
{
	z80ctc *ctc = ctcs + which;
	int i;

	/* set up defaults */
	for (i = 0; i < 4; i++)
	{
		ctc->mode[i] = RESET_ACTIVE;
		ctc->tconst[i] = 0x100;
		timer_adjust(ctc->timer[i], TIME_NEVER, 0, 0);
		ctc->int_state[i] = 0;
	}
	z80ctc_interrupt_check( ctc );
}

void z80ctc_0_reset (void) { z80ctc_reset (0); }
void z80ctc_1_reset (void) { z80ctc_reset (1); }


void z80ctc_w (int which, int offset, int data)
{
	z80ctc *ctc = ctcs + which;
	int mode, ch;

	/* keep channel within range, and get the current mode */
	ch = offset & 3;
	mode = ctc->mode[ch];

	/* if we're waiting for a time constant, this is it */
	if ((mode & CONSTANT) == CONSTANT_LOAD)
	{
		/* set the time constant (0 -> 0x100) */
		ctc->tconst[ch] = data ? data : 0x100;

		/* clear the internal mode -- we're no longer waiting */
		ctc->mode[ch] &= ~CONSTANT;

		/* also clear the reset, since the constant gets it going again */
		ctc->mode[ch] &= ~RESET;

		/* if we're in timer mode.... */
		if ((mode & MODE) == MODE_TIMER)
		{
			/* if we're triggering on the time constant, reset the down counter now */
			if ((mode & TRIGGER) == TRIGGER_AUTO)
			{
				double clock = ((mode & PRESCALER) == PRESCALER_16) ? ctc->invclock16 : ctc->invclock256;
				if (!(ctc->notimer & (1<<ch)))
					timer_adjust(ctc->timer[ch], clock * (double)ctc->tconst[ch], (which << 2) + ch, clock * (double)ctc->tconst[ch]);
				else
					timer_adjust(ctc->timer[ch], TIME_NEVER, 0, 0);
			}

			/* else set the bit indicating that we're waiting for the appropriate trigger */
			else
				ctc->mode[ch] |= WAITING_FOR_TRIG;
		}

		/* also set the down counter in case we're clocking externally */
		ctc->down[ch] = ctc->tconst[ch];

		/* all done here */
		return;
	}

	/* if we're writing the interrupt vector, handle it specially */
#if 0	/* Tatsuyuki Satoh changes */
	/* The 'Z80family handbook' wrote,                            */
	/* interrupt vector is able to set for even channel (0 or 2)  */
	if ((data & CONTROL) == CONTROL_VECTOR && (ch&1) == 0)
#else
	if ((data & CONTROL) == CONTROL_VECTOR && ch == 0)
#endif
	{
		ctc->vector = data & 0xf8;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC Vector = %02x\n", ctc->vector);
		return;
	}

	/* this must be a control word */
	if ((data & CONTROL) == CONTROL_WORD)
	{
		/* set the new mode */
		ctc->mode[ch] = data;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC ch.%d mode = %02x\n", ch, data);

		/* if we're being reset, clear out any pending timers for this channel */
		if ((data & RESET) == RESET_ACTIVE)
		{
			timer_adjust(ctc->timer[ch], TIME_NEVER, 0, 0);

			if( ctc->int_state[ch] != 0 )
			{
				/* clear interrupt service , request */
				ctc->int_state[ch] = 0;
				z80ctc_interrupt_check( ctc );
			}
		}

		/* all done here */
		return;
	}
}

WRITE_HANDLER( z80ctc_0_w ) { z80ctc_w (0, offset, data); }
WRITE_HANDLER( z80ctc_1_w ) { z80ctc_w (1, offset, data); }


int z80ctc_r (int which, int ch)
{
	z80ctc *ctc = ctcs + which;
	int mode;

	/* keep channel within range */
	ch &= 3;
	mode = ctc->mode[ch];

	/* if we're in counter mode, just return the count */
	if ((mode & MODE) == MODE_COUNTER)
		return ctc->down[ch];

	/* else compute the down counter value */
	else
	{
		double clock = ((mode & PRESCALER) == PRESCALER_16) ? ctc->invclock16 : ctc->invclock256;

log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC clock %f\n",1.0/clock);


		if (ctc->timer[ch])
			return ((int)(timer_timeleft (ctc->timer[ch]) / clock) + 1) & 0xff;
		else
			return 0;
	}
}

READ_HANDLER( z80ctc_0_r ) { return z80ctc_r (0, offset); }
READ_HANDLER( z80ctc_1_r ) { return z80ctc_r (1, offset); }


int z80ctc_interrupt( int which )
{
	z80ctc *ctc = ctcs + which;
	int ch;

	for( ch = 0 ; ch < 4 ; ch++ )
	{
		if( ctc->int_state[ch] )
		{
			if( ctc->int_state[ch] == Z80_INT_REQ)
				ctc->int_state[ch] = Z80_INT_IEO;
			break;
		}
	}
	if( ch > 3 )
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC entry INT : non IRQ\n");
		ch = 0;
	}
	z80ctc_interrupt_check( ctc );
	return ctc->vector + ch * 2;
}

/* when operate RETI , soud be call this function for request pending interrupt */
void z80ctc_reti( int which )
{
	z80ctc *ctc = ctcs + which;
	int ch;

	for( ch = 0 ; ch < 4 ; ch++ )
	{
		if( ctc->int_state[ch] & Z80_INT_IEO )
		{
			/* highest served interrupt found */
			/* clear interrupt status */
			ctc->int_state[ch] &= ~Z80_INT_IEO;
			/* search next interrupt */
			break;
		}
	}
	/* set next interrupt stattus */
	z80ctc_interrupt_check( ctc );
}

static void z80ctc_timercallback (int param)
{
	int which = param >> 2;
	int ch = param & 3;
	z80ctc *ctc = ctcs + which;

	/* down counter has reached zero - see if we should interrupt */
	if ((ctc->mode[ch] & INTERRUPT) == INTERRUPT_ON)
	{
		if( !(ctc->int_state[ch] & Z80_INT_REQ) )
		{
			ctc->int_state[ch] |= Z80_INT_REQ;
			z80ctc_interrupt_check( ctc );
		}
	}
	/* generate the clock pulse */
	if (ctc->zc[ch])
	{
		(*ctc->zc[ch])(0,1);
		(*ctc->zc[ch])(0,0);
	}

	/* reset the down counter */
	ctc->down[ch] = ctc->tconst[ch];
}


void z80ctc_trg_w (int which, int trg, int offset, int data)
{
	z80ctc *ctc = ctcs + which;
	int ch = trg & 3;
	int mode;

	data = data ? 1 : 0;
	mode = ctc->mode[ch];

	/* see if the trigger value has changed */
	if (data != ctc->extclk[ch])
	{
		ctc->extclk[ch] = data;

		/* see if this is the active edge of the trigger */
		if (((mode & EDGE) == EDGE_RISING && data) || ((mode & EDGE) == EDGE_FALLING && !data))
		{
			/* if we're waiting for a trigger, start the timer */
			if ((mode & WAITING_FOR_TRIG) && (mode & MODE) == MODE_TIMER)
			{
				double clock = ((mode & PRESCALER) == PRESCALER_16) ? ctc->invclock16 : ctc->invclock256;

log_cb(RETRO_LOG_DEBUG, LOGPRE "CTC clock %f\n",1.0/clock);

				if (!(ctc->notimer & (1<<ch)))
					timer_adjust(ctc->timer[ch], clock * (double)ctc->tconst[ch], (which << 2) + ch, clock * (double)ctc->tconst[ch]);
				else
					timer_adjust(ctc->timer[ch], TIME_NEVER, 0, 0);
			}

			/* we're no longer waiting */
			ctc->mode[ch] &= ~WAITING_FOR_TRIG;

			/* if we're clocking externally, decrement the count */
			if ((mode & MODE) == MODE_COUNTER)
			{
				ctc->down[ch]--;

				/* if we hit zero, do the same thing as for a timer interrupt */
				if (!ctc->down[ch])
					z80ctc_timercallback ((which << 2) + ch);
			}
		}
	}
}

WRITE_HANDLER( z80ctc_0_trg0_w ) { z80ctc_trg_w (0, 0, offset, data); }
WRITE_HANDLER( z80ctc_0_trg1_w ) { z80ctc_trg_w (0, 1, offset, data); }
WRITE_HANDLER( z80ctc_0_trg2_w ) { z80ctc_trg_w (0, 2, offset, data); }
WRITE_HANDLER( z80ctc_0_trg3_w ) { z80ctc_trg_w (0, 3, offset, data); }
WRITE_HANDLER( z80ctc_1_trg0_w ) { z80ctc_trg_w (1, 0, offset, data); }
WRITE_HANDLER( z80ctc_1_trg1_w ) { z80ctc_trg_w (1, 1, offset, data); }
WRITE_HANDLER( z80ctc_1_trg2_w ) { z80ctc_trg_w (1, 2, offset, data); }
WRITE_HANDLER( z80ctc_1_trg3_w ) { z80ctc_trg_w (1, 3, offset, data); }


/*---------------------- Z80 PIO ---------------------------------*/

/* starforce emurate Z80PIO subset */
/* ch.A mode 1 input handshake mode from sound command */
/* ch.b not use */


#define PIO_MODE0 0x00		/* output mode */
#define PIO_MODE1 0x01		/* input  mode */
#define PIO_MODE2 0x02		/* i/o    mode */
#define PIO_MODE3 0x03		/* bit    mode */
/* pio controll port operation (bit 0-3) */
#define PIO_OP_MODE 0x0f	/* mode select        */
#define PIO_OP_INTC 0x07	/* interrupt controll */
#define PIO_OP_INTE 0x03	/* interrupt enable   */
#define PIO_OP_INTE 0x03	/* interrupt enable   */
/* pio interrupt controll nit */
#define PIO_INT_ENABLE 0x80  /* ENABLE : 0=disable , 1=enable */
#define PIO_INT_AND    0x40  /* LOGIC  : 0=OR      , 1=AND    */
#define PIO_INT_HIGH   0x20  /* LEVEL  : 0=low     , 1=high   */
#define PIO_INT_MASK   0x10  /* MASK   : 0=off     , 1=on     */

typedef struct
{
	int vector[2];                        /* interrupt vector               */
	void (*intr)(int which);              /* interrupt callbacks            */
	void (*rdyr[2])(int data);            /* RDY active callback            */
	int mode[2];                          /* mode 00=in,01=out,02=i/o,03=bit*/
	int enable[2];                        /* interrupt enable               */
	int mask[2];                          /* mask folowers                  */
	int dir[2];                           /* direction (bit mode)           */
	int rdy[2];                           /* ready pin level                */
	int in[2];                            /* input port data                */
	int out[2];                           /* output port                    */
	int strobe[2];							/* strobe inputs */
	int int_state[2];                     /* interrupt status (daisy chain) */
} z80pio;

static z80pio pios[MAX_PIO];

static void	z80pio_set_rdy(z80pio *pio, int ch, int state)
{
	/* set state */
	pio->rdy[ch] = state;

	/* call callback with state */
	if (pio->rdyr[ch]!=0)
		pio->rdyr[ch](pio->rdy[ch]);
}

/* initialize pio emurator */
void z80pio_init (z80pio_interface *intf)
{
	int i;

	memset (pios, 0, sizeof (pios));

	for (i = 0; i < intf->num; i++)
	{
		pios[i].intr = intf->intr[i];
		pios[i].rdyr[0] = intf->rdyA[i];
		pios[i].rdyr[1] = intf->rdyB[i];
		z80pio_reset (i);
	}
}

static void z80pio_interrupt_check( z80pio *pio )
{
	int state;

	if( pio->int_state[1] & Z80_INT_IEO ) state  = Z80_INT_IEO;
	else                                  state  = pio->int_state[1];
	if( pio->int_state[0] & Z80_INT_IEO ) state  = Z80_INT_IEO;
	else                                  state |= pio->int_state[0];
	/* change daisy chain status */
	if (pio->intr) (*pio->intr)(state);
}

static void z80pio_check_irq( z80pio *pio , int ch )
{
	int irq = 0;
	int data;
	int old_state;

	if( pio->enable[ch] & PIO_INT_ENABLE )
	{
		if( pio->mode[ch] == PIO_MODE3 )
		{
			data  =  pio->in[ch] & pio->dir[ch]; /* input data only */
			data &= ~pio->mask[ch];              /* mask follow     */
			if( !(pio->enable[ch]&PIO_INT_HIGH) )/* active level    */
				data ^= pio->mask[ch];             /* active low  */
			if( pio->enable[ch]&PIO_INT_AND )    /* logic      */
			     { if( data == pio->mask[ch] ) irq = 1; }
			else { if( data == 0             ) irq = 1; }
			/* if portB , portA mode 2 check */
			if( ch && (pio->mode[0]==PIO_MODE2) )
			{
				if( pio->rdy[ch] == 0 ) irq = 1;
			}
		}
		else if( pio->rdy[ch] == 0 ) irq = 1;
	}
	old_state = pio->int_state[ch];
	if( irq ) pio->int_state[ch] |=  Z80_INT_REQ;
	else      pio->int_state[ch] &= ~Z80_INT_REQ;

	if( old_state != pio->int_state[ch] )
		z80pio_interrupt_check( pio );
}

void z80pio_reset (int which)
{
	z80pio *pio = pios + which;
	int i;

	for( i = 0 ; i <= 1 ; i++){
		pio->mask[i]   = 0xff;	/* mask all on */
		pio->enable[i] = 0x00;	/* disable     */
		pio->mode[i]   = 0x01;	/* mode input  */
		pio->dir[i]    = 0x01;	/* dir  input  */
		z80pio_set_rdy(pio,i,0);	/* RDY = low   */
		pio->out[i]    = 0x00;	/* outdata = 0 */
		pio->int_state[i] = 0;
		pio->strobe[i] = 0;
	}
	z80pio_interrupt_check( pio );
}

/* pio data register write */
void z80pio_d_w( int which , int ch , int data )
{
	z80pio *pio = pios + which;
	if( ch ) ch = 1;

	pio->out[ch] = data;	/* latch out data */
	switch( pio->mode[ch] ){
	case PIO_MODE0:			/* mode 0 output */
	case PIO_MODE2:			/* mode 2 i/o */
		z80pio_set_rdy(pio, ch,1); /* ready = H */
		z80pio_check_irq( pio , ch );
		return;
	case PIO_MODE1:			/* mode 1 intput */
	case PIO_MODE3:			/* mode 0 bit */
		return;
	default:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c data write,bad mode\n",'A'+ch );
	}
}

/* pio controll register write */
void z80pio_c_w( int which , int ch , int data )
{
	z80pio *pio = pios + which;
	if( ch ) ch = 1;

	/* load direction phase ? */
	if( pio->mode[ch] == 0x13 ){
		pio->dir[ch] = data;
		pio->mode[ch] = 0x03;
		return;
	}
	/* load mask folows phase ? */
	if( pio->enable[ch] & PIO_INT_MASK ){	/* load mask folows */
		pio->mask[ch] = data;
		pio->enable[ch] &= ~PIO_INT_MASK;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c interrupt mask %02x\n",'A'+ch,data );
		return;
	}
	switch( data & 0x0f ){
	case PIO_OP_MODE:	/* mode select 0=out,1=in,2=i/o,3=bit */
		pio->mode[ch] = (data >> 6 );
		if( pio->mode[ch] == 0x03 ) pio->mode[ch] = 0x13;
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c Mode %x\n",'A'+ch,pio->mode[ch] );
		break;
	case PIO_OP_INTC:		/* interrupt control */
		pio->enable[ch] = data & 0xf0;
		pio->mask[ch]   = 0x00;
		/* when interrupt enable , set vector request flag */
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c Controll %02x\n",'A'+ch,data );
		break;
	case PIO_OP_INTE:		/* interrupt enable controll */
		pio->enable[ch] &= ~PIO_INT_ENABLE;
		pio->enable[ch] |= (data & PIO_INT_ENABLE);
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c enable %02x\n",'A'+ch,data&0x80 );
		break;
	default:
			if( !(data&1) )
			{
				pio->vector[ch] = data;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c vector %02x\n",'A'+ch,data);
			}
			else log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c illegal command %02x\n",'A'+ch,data );
	}
	/* interrupt check */
	z80pio_check_irq( pio , ch );
}

/* pio controll register read */
int z80pio_c_r( int which , int ch )
{
	if( ch ) ch = 1;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c controll read\n",'A'+ch );
	return 0;
}

/* pio data register read */
int z80pio_d_r( int which , int ch )
{
	z80pio *pio = pios + which;
	if( ch ) ch = 1;

	switch( pio->mode[ch] ){
	case PIO_MODE0:			/* mode 0 output */
		return pio->out[ch];
	case PIO_MODE1:			/* mode 1 intput */
		z80pio_set_rdy(pio, ch, 1);	/* ready = H */
		z80pio_check_irq( pio , ch );
		return pio->in[ch];
	case PIO_MODE2:			/* mode 2 i/o */
		if( ch ) log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-B mode 2 \n");
		z80pio_set_rdy(pio, 1, 1); /* brdy = H */
		z80pio_check_irq( pio , ch );
		return pio->in[ch];
	case PIO_MODE3:			/* mode 3 bit */
		return (pio->in[ch]&pio->dir[ch])|(pio->out[ch]&~pio->dir[ch]);
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c data read,bad mode\n",'A'+ch );
	return 0;
}

int z80pio_interrupt( int which )
{
	z80pio *pio = pios + which;
	int ch = 0;

	/* port A */
	if( pio->int_state[0] == Z80_INT_REQ )
	{
		pio->int_state[0] |= Z80_INT_IEO;
	} if( pio->int_state[0] == 0 )
	{
		/* port B */
		ch = 1;
		if( pio->int_state[1] == Z80_INT_REQ )
		{
			pio->int_state[1] |= Z80_INT_IEO;
		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO entry INT : non IRQ\n");
			ch = 0;
		}
	}
	z80pio_interrupt_check( pio );
	return pio->vector[ch];
}

void z80pio_reti( int which )
{
	z80pio *pio = pios + which;

	if( pio->int_state[0] & Z80_INT_IEO )
	{
		pio->int_state[0] &= ~Z80_INT_IEO;
	} else if( pio->int_state[1] & Z80_INT_IEO )
	{
		pio->int_state[1] &= ~Z80_INT_IEO;
	}
	/* set next interrupt stattus */
	z80pio_interrupt_check( pio );
}

/* z80pio port write */
void z80pio_p_w( int which , int ch , int data )
{
	z80pio *pio = pios + which;

	if( ch ) ch = 1;

	pio->in[ch]  = data;
	switch( pio->mode[ch] ){
	case PIO_MODE0:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c OUTPUT mode and data write\n",'A'+ch );
		break;
	case PIO_MODE2:	/* only port A */
		ch = 1;		/* handshake and IRQ is use portB */
	case PIO_MODE1:
		z80pio_set_rdy(pio, ch, 0);
		z80pio_check_irq( pio , ch );
		break;
	case PIO_MODE3:
		/* irq check */
		z80pio_check_irq( pio , ch );
		break;
	}
}

/* z80pio port read */
int z80pio_p_r( int which , int ch )
{
	z80pio *pio = pios + which;

	if( ch ) ch = 1;

	switch( pio->mode[ch] ){
	case PIO_MODE2:		/* port A only */
	case PIO_MODE0:
		z80pio_set_rdy(pio, ch, 0);
		z80pio_check_irq( pio , ch );
		break;
	case PIO_MODE1:
		log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c INPUT mode and data read\n",'A'+ch );
		break;
	case PIO_MODE3:
		/*     input bits                , output bits                */
		return (pio->in[ch]&pio->dir[ch])|(pio->out[ch]&~pio->dir[ch]);
	}
	return pio->out[ch];
}

/* for mame interface */

void z80pio_0_reset (void) { z80pio_reset (0); }

WRITE_HANDLER( z80pio_0_w )
{
	if(offset&1) z80pio_c_w(0,(offset/2)&1,data);
	else         z80pio_d_w(0,(offset/2)&1,data);
}

READ_HANDLER( z80pio_0_r )
{
	return (offset&1) ? z80pio_c_r(0,(offset/2)&1) : z80pio_d_r(0,(offset/2)&1);
}

WRITE_HANDLER( z80pioA_0_p_w ) { z80pio_p_w(0,0,data);   }
WRITE_HANDLER( z80pioB_0_p_w ) { z80pio_p_w(0,1,data);   }
READ_HANDLER( z80pioA_0_p_r )           { return z80pio_p_r(0,0); }
READ_HANDLER( z80pioB_0_p_r )           { return z80pio_p_r(0,1); }

static void z80pio_update_strobe(int which, int ch, int state)
{
	z80pio *pio = pios + which;

	if (ch) ch=1;

	switch (pio->mode[ch])
	{
		/* output mode */
		case PIO_MODE0:
		{
			/* ensure valid */
			state = state & 0x01;

			/* strobe changed state? */
			if ((pio->strobe[ch]^state)!=0)
			{
				/* yes */
				if (state!=0)
				{
					/* positive edge */
					log_cb(RETRO_LOG_DEBUG, LOGPRE "PIO-%c positive strobe\n",'A'+ch );
					/* ready is now inactive */
					z80pio_set_rdy(pio, ch, 0);

					/* int enabled? */
					if (pio->enable[ch] & PIO_INT_ENABLE)
					{
						/* trigger an int request */
						pio->int_state[ch] |= Z80_INT_REQ;
					}
				}
			}

			/* store strobe state */
			pio->strobe[ch] = state;

			/* check interrupt */
			z80pio_interrupt_check( pio );
		}
		break;

		default:
			break;
	}
}

/* set /astb input */
/* /astb is active low */
/* output mode: a positive edge is used by peripheral to acknowledge
the receipt of data */
/* input mode: strobe is used by peripheral to load data from the peripheral
into port a input register, data loaded into pio when signal is active */
void	z80pio_astb_w(int which, int state)
{
	z80pio_update_strobe(which, 0, state);
}

/* set bstb input */
void	z80pio_bstb_w(int which, int state)
{
	z80pio_update_strobe(which, 1, state);
}
