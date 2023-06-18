
#include "driver.h"
/*#include <stdio.h>*/

#include "machine/8254pit.h"



void pit8254_init (pit8254_interface *intf)
{

}

void pit8254_w (int which, int offset, int data)
{
	switch (offset)
	{
		case 0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d write %d to timer1\n",which, data);
			break;
		case 1:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d write %d to timer2\n",which, data);
			break;
		case 2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d write %d to timer3\n",which, data);
			break;
		case 3:
			{
				int sc=(data>>6)&3;
				int rw=(data>>4)&3;
				int mode=(data>>1)&0x07;
				int bcd=data&0x01;
				log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d write %02x to control : ", which, data);
				log_cb(RETRO_LOG_DEBUG, LOGPRE "*** SC=%d RW=%d MODE=%d BCD=%d\n",sc, rw, mode, bcd);
			}

			break;
	}
}

int pit8254_r (int which, int offset)
{
	switch (offset)
	{
		case 0:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d read from timer1\n", which);

			break;
		case 1:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d read from timer2\n", which);
			break;
		case 2:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d read from timer3\n", which);
			break;
		case 3:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "PIT8254#%d read from control\n", which);
			break;
	}

	return 0;
}

/*
Port handler wrappers.
*/

WRITE_HANDLER( pit8254_0_w )
{
	pit8254_w(0, offset, data);
}

WRITE_HANDLER( pit8254_0_counter1_w )
{
	pit8254_w(0, 0, data);
}

WRITE_HANDLER( pit8254_0_counter2_w )
{
	pit8254_w(0, 1, data);
}

WRITE_HANDLER( pit8254_0_counter3_w )
{
	pit8254_w(0, 2, data);
}

WRITE_HANDLER( pit8254_0_control_w )
{
	pit8254_w(0, 3, data);
}

WRITE_HANDLER( pit8254_1_w )
{
	pit8254_w(1, 0, data);
}

WRITE_HANDLER( pit8254_1_counter1_w )
{
	pit8254_w(1, 0, data);
}

WRITE_HANDLER( pit8254_1_counter2_w )
{
	pit8254_w(1, 1, data);
}

WRITE_HANDLER( pit8254_1_counter3_w )
{
	pit8254_w(1, 2, data);
}

WRITE_HANDLER( pit8254_1_control_w )
{
	pit8254_w(1, 3, data);
}


READ_HANDLER( pit8254_0_r )
{
	return pit8254_r(0, offset);
}

READ_HANDLER( pit8254_0_counter1_r )
{
	return pit8254_r(0, 0);
}

READ_HANDLER( pit8254_0_counter2_r )
{
	return pit8254_r(0, 1);
}

READ_HANDLER( pit8254_0_counter3_r )
{
	return pit8254_r(0, 2);
}

READ_HANDLER( pit8254_0_control_r )
{
	return pit8254_r(0, 3);
}

READ_HANDLER( pit8254_1_r )
{
	return pit8254_r(1, offset);
}

READ_HANDLER( pit8254_1_counter1_r )
{
	return pit8254_r(1, 0);
}

READ_HANDLER( pit8254_1_counter2_r )
{
	return pit8254_r(1, 1);
}

READ_HANDLER( pit8254_1_counter3_r )
{
	return pit8254_r(1, 2);
}
READ_HANDLER( pit8254_1_control_r )
{
	return pit8254_r(1, 3);
}
