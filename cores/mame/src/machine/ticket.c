/***************************************************************************

  machine.c

  Functions to emulate a prototypical ticket dispenser hardware.

  Right now, this is an *extremely* basic ticket dispenser.
  TODO:	Active Bit may not be Bit 7 in all applications.
  	    Add a ticket dispenser interface instead of passing a bunch
		of arguments to ticket_dispenser_init.
		Add sound, graphical output?
***************************************************************************/

#include "driver.h"
#include "machine/ticket.h"

/*#define DEBUG_TICKET*/

#define MAX_DISPENSERS	2

extern unsigned int dispensed_tickets;

struct ticket_state
{
	int status;
	int power;
	void *timer;
};

static int active_bit = 0x80;
static int time_msec;
static int motoron;
static int ticketdispensed;
static int ticketnotdispensed;

static struct ticket_state dispenser[MAX_DISPENSERS];

/* Callback routine used during ticket dispensing */
static void ticket_dispenser_toggle(int parm);


/***************************************************************************
  ticket_dispenser_init

***************************************************************************/
void ticket_dispenser_init(int msec, int motoronhigh, int statusactivehigh)
{
	int i;

	time_msec			= msec;
	motoron				= motoronhigh  ? active_bit : 0;
	ticketdispensed		= statusactivehigh ? active_bit : 0;
	ticketnotdispensed	= ticketdispensed ^ active_bit;
	dispensed_tickets	= 0;

	for (i = 0; i < MAX_DISPENSERS; i++)
	{
		dispenser[i].status	= ticketnotdispensed;
		dispenser[i].power 	= 0x00;
		dispenser[i].timer 	= timer_alloc(ticket_dispenser_toggle);
	}
}

/***************************************************************************
  ticket_dispenser_r
***************************************************************************/
READ_HANDLER( ticket_dispenser_r )
{
	return ticket_dispenser_0_r(offset);
}

READ_HANDLER( ticket_dispenser_0_r )
{
#ifdef DEBUG_TICKET
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Status Read = %02X\n", activecpu_get_pc(), status);
#endif
	return dispenser[0].status;
}

READ_HANDLER( ticket_dispenser_1_r )
{
#ifdef DEBUG_TICKET
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Status Read = %02X\n", activecpu_get_pc(), status);
#endif
	return dispenser[1].status;
}

/***************************************************************************
  ticket_dispenser_w
***************************************************************************/
WRITE_HANDLER( ticket_dispenser_w )
{
	ticket_dispenser_0_w(offset, data);
}

WRITE_HANDLER( ticket_dispenser_0_w )
{
	/* On an activate signal, start dispensing! */
	if ((data & active_bit) == motoron)
	{
		if (!dispenser[0].power)
		{
#ifdef DEBUG_TICKET
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Power On\n", activecpu_get_pc());
#endif
			timer_adjust(dispenser[0].timer, TIME_IN_MSEC(time_msec), 0, 0);
			dispenser[0].power = 1;

			dispenser[0].status = ticketnotdispensed;
		}
	}
	else
	{
		if (dispenser[0].power)
		{
#ifdef DEBUG_TICKET
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Power Off\n", activecpu_get_pc());
#endif
			timer_adjust(dispenser[0].timer, TIME_NEVER, 0, 0);
			set_led_status(2,0);
			dispenser[0].power = 0;
		}
	}
}

WRITE_HANDLER( ticket_dispenser_1_w )
{
	/* On an activate signal, start dispensing! */
	if ((data & active_bit) == motoron)
	{
		if (!dispenser[1].power)
		{
#ifdef DEBUG_TICKET
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Power On\n", activecpu_get_pc());
#endif
			timer_adjust(dispenser[1].timer, TIME_IN_MSEC(time_msec), 1, 0);
			dispenser[1].power = 1;

			dispenser[1].status = ticketnotdispensed;
		}
	}
	else
	{
		if (dispenser[1].power)
		{
#ifdef DEBUG_TICKET
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PC: %04X  Ticket Power Off\n", activecpu_get_pc());
#endif
			timer_adjust(dispenser[1].timer, TIME_NEVER, 1, 0);
			set_led_status(2,0);
			dispenser[1].power = 0;
		}
	}
}


/***************************************************************************
  ticket_dispenser_toggle

  How I think this works:
  When a ticket dispenses, there is N milliseconds of status = high,
  and N milliseconds of status = low (a wait cycle?).
***************************************************************************/
static void ticket_dispenser_toggle(int which)
{
	/* If we still have power, keep toggling ticket states. */
	if (dispenser[which].power)
	{
		dispenser[which].status ^= active_bit;
#ifdef DEBUG_TICKET
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Ticket Status Changed to %02X\n", status);
#endif
		timer_adjust(dispenser[which].timer, TIME_IN_MSEC(time_msec), which, 0);
	}

	if (dispenser[which].status == ticketdispensed)
	{
		set_led_status(2,1);
		dispensed_tickets++;

#ifdef DEBUG_TICKET
		log_cb(RETRO_LOG_DEBUG, LOGPRE "Ticket Dispensed\n");
#endif
	}
	else
	{
		set_led_status(2,0);
	}
}
