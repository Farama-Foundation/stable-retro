/***************************************************************************

  RIOT 6532 emulation

***************************************************************************/

#include "driver.h"
#include "machine/6532riot.h"


struct R6532
{
	struct R6532interface intf;

	UINT8 DRA;
	UINT8 DRB;
	UINT8 DDRA;
	UINT8 DDRB;

	int shift;
	int cleared;

	UINT32 target;
};


static struct R6532* r6532[2];


static UINT8 r6532_combineA(int n, UINT8 val)
{
	return (r6532[n]->DDRA & r6532[n]->DRA) | (~r6532[n]->DDRA & val);
}


static UINT8 r6532_combineB(int n, UINT8 val)
{
	return (r6532[n]->DDRB & r6532[n]->DRB) | (~r6532[n]->DDRB & val);
}


static UINT8 r6532_read_portA(int n)
{
	if (r6532[n]->intf.portA_r != NULL)
	{
		return r6532_combineA(n, r6532[n]->intf.portA_r(0));
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Read from unhandled 6532 #%d port A\n", n);

	return 0;
}


static UINT8 r6532_read_portB(int n)
{
	if (r6532[n]->intf.portB_r != NULL)
	{
		return r6532_combineB(n, r6532[n]->intf.portB_r(2));
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Read from unhandled 6532 #%d port B\n", n);

	return 0;
}


static void r6532_write_portA(int n, UINT8 data)
{
	r6532[n]->DRA = data;

	if (r6532[n]->intf.portA_w != NULL)
	{
		r6532[n]->intf.portA_w(0, r6532_combineA(n, 0xFF));
	}
}


static void r6532_write_portB(int n, UINT8 data)
{
	r6532[n]->DRB = data;

	if (r6532[n]->intf.portB_w != NULL)
	{
		r6532[n]->intf.portB_w(0, r6532_combineB(n, 0xFF));
	}
}


static void r6532_write(int n, offs_t offset, UINT8 data)
{
	if (offset & 4)
	{
		if (offset & 16)
		{
			r6532[n]->cleared = 0;

			switch (offset & 3)
			{
			case 0:
				r6532[n]->shift = 0;
				break;
			case 1:
				r6532[n]->shift = 3;
				break;
			case 2:
				r6532[n]->shift = 6;
				break;
			case 3:
				r6532[n]->shift = 10;
				break;
			}

			r6532[n]->target = activecpu_gettotalcycles() + (data << r6532[n]->shift);
		}
		else
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Write to unimplemented 6532 #%d edge detect control\n", n);
		}
	}
	else
	{
		offset &= 3;

		switch (offset)
		{
		case 0:
			r6532_write_portA(n, data);
			break;
		case 1:
			r6532[n]->DDRA = data;
			break;
		case 2:
			r6532_write_portB(n, data);
			break;
		case 3:
			r6532[n]->DDRB = data;
			break;
		}
	}
}


static UINT8 r6532_read_timer(int n, int enable)
{
	int count = r6532[n]->target - activecpu_gettotalcycles();

	if (count >= 0)
	{
		return count >> r6532[n]->shift;
	}
	else
	{
		if (count != -1)
		{
			r6532[n]->cleared = 1;
		}

		return (count >= -256) ? count : 0;
	}
}


static UINT8 r6532_read_flags(int n)
{
	int count = r6532[n]->target - activecpu_gettotalcycles();

	if (count >= 0 || r6532[n]->cleared)
	{
		return 0x00;
	}
	else
	{
		return 0x80;
	}
}


static UINT8 r6532_read(int n, offs_t offset)
{
	UINT8 val = 0;

	switch (offset & 7)
	{
	case 0:
		val = r6532_read_portA(n);
		break;
	case 1:
		val = r6532[n]->DDRA;
		break;
	case 2:
		val = r6532_read_portB(n);
		break;
	case 3:
		val = r6532[n]->DDRB;
		break;
	case 4:
		val = r6532_read_timer(n, 0);
		break;
	case 5:
		val = r6532_read_flags(n);
		break;
	case 6:
		val = r6532_read_timer(n, 1);
		break;
	case 7:
		val = r6532_read_flags(n);
		break;
	}

	return val;
}


WRITE_HANDLER( r6532_0_w ) { r6532_write(0, offset, data); }
WRITE_HANDLER( r6532_1_w ) { r6532_write(1, offset, data); }


READ_HANDLER( r6532_0_r ) { return r6532_read(0, offset); }
READ_HANDLER( r6532_1_r ) { return r6532_read(1, offset); }


void r6532_init(int n, const struct R6532interface* intf)
{
	r6532[n] = auto_malloc(sizeof(struct R6532));

	r6532[n]->intf = *intf;

	r6532[n]->DRA = 0;
	r6532[n]->DRB = 0;
	r6532[n]->DDRA = 0;
	r6532[n]->DDRB = 0;

	r6532[n]->shift = 0;
	r6532[n]->cleared = 0;

	r6532[n]->target = 0;
}
