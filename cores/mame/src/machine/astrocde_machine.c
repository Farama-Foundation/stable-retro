/**************************************************************************

	Interrupt System Hardware for Bally/Midway games

 	Mike@Dissfulfils.co.uk

**************************************************************************/

#include "driver.h"


READ_HANDLER( gorf_timer_r )
{
	static int Skip=0;
	unsigned char *RAM = memory_region(REGION_CPU1);


	if ((RAM[0x5A93]==160) || (RAM[0x5A93]==4)) 	/* INVADERS AND    */
	{												/* GALAXIAN SCREEN */
		if (activecpu_get_pc()==0x3086)
		{
		    if(--Skip==-1)
			{
				Skip=2;
			}
		}

	   	return Skip;
	}
	else
	{
		return RAM[0xD0A5];
	}

}


/****************************************************************************
 * Seawolf Controllers
 ****************************************************************************/

/*
 * Seawolf2 uses rotary controllers on input ports 10 + 11
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button.
 *
 * The controllers look like they returns Grays binary,
 * so I use a table to translate my simple counter into it!
 */

static const int ControllerTable[64] = {
	0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
	12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
	24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
	20 , 21 , 23 , 22 , 18 , 19 , 17 , 16 ,
	48 , 49 , 51 , 50 , 54 , 55 , 53 , 52 ,
	60 , 61 , 63 , 62 , 58 , 59 , 57 , 56 ,
	40 , 41 , 43 , 42 , 46 , 47 , 45 , 44 ,
	36 , 37 , 39 , 38 , 34 , 35 , 33 , 32
};

READ_HANDLER( seawolf2_controller1_r )
{
	return (input_port_0_r(0) & 0xc0) + ControllerTable[input_port_0_r(0) & 0x3f];
}

READ_HANDLER( seawolf2_controller2_r )
{
	return (input_port_1_r(0) & 0xc0) + ControllerTable[input_port_1_r(0) & 0x3f];
}


static int ebases_trackball_select = 0;

WRITE_HANDLER( ebases_trackball_select_w )
{
	ebases_trackball_select = data;
}

READ_HANDLER( ebases_trackball_r )
{
	int ret = readinputport(3 + ebases_trackball_select);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Port %d = %d\n", ebases_trackball_select, ret);
	return ret;
}
