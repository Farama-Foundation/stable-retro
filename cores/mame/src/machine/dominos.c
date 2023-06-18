/***************************************************************************

	Atari Dominos hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "dominos.h"

static int dominos_attract = 0;
static int ac_line = 0x00;

/***************************************************************************
Read Ports

Dominos looks for the following:
	AAAAAAAA		D				D
	76543210		6				7
	00011000 ($18)	n/a				Player 1 Up
	00011001 ($19)	n/a				Player 1 Right
	00011010 ($1A)	n/a				Player 1 Down
	00011011 ($1B)	n/a				Player 1 Left

	00101000 ($28)	n/a				Player 2 Up
	00101001 ($29)	n/a				Player 2 Right
	00101010 ($2A)	n/a				Player 2 Down
	00101011 ($2B)	n/a				Player 2 Left
	00101100 ($2C)	n/a				Start 1
	00101101 ($2D)	n/a				Start 2
	00101110 ($2E)	n/a				Self Test

	00x10x00		Points0 (DIP)	Points1 (DIP)
	00x10x01		Mode 0 $ (DIP)	Mode 1 $ (DIP)

	01xxxxxx		Coin1			Coin2

We remap our input ports because if we didn't, we'd use a bunch of ports.
***************************************************************************/

READ_HANDLER( dominos_port_r )
{
	switch (offset)
	{
		/* IN0 */
		case 0x18:		return ((input_port_1_r(0) & 0x01) << 7);
		case 0x19:		return ((input_port_1_r(0) & 0x02) << 6);
		case 0x1A:		return ((input_port_1_r(0) & 0x04) << 5);
		case 0x1B:		return ((input_port_1_r(0) & 0x08) << 4);
		case 0x1C:		return ((input_port_1_r(0) & 0x10) << 3);
		case 0x1D:		return ((input_port_1_r(0) & 0x20) << 2);
		case 0x1E:		return ((input_port_1_r(0) & 0x40) << 1);
		case 0x1F:		return ((input_port_1_r(0) & 0x80) << 0);
		/* IN1 */
		case 0x28:		return ((input_port_2_r(0) & 0x01) << 7);
		case 0x29:		return ((input_port_2_r(0) & 0x02) << 6);
		case 0x2A:		return ((input_port_2_r(0) & 0x04) << 5);
		case 0x2B:		return ((input_port_2_r(0) & 0x08) << 4);
		case 0x2C:		return ((input_port_2_r(0) & 0x10) << 3);
		case 0x2D:		return ((input_port_2_r(0) & 0x20) << 2);
		case 0x2E:		return ((input_port_2_r(0) & 0x40) << 1);
		case 0x2F:		return ((input_port_2_r(0) & 0x80) << 0);
		/* DSW */
		case 0x10:
		case 0x14:
		case 0x30:
		case 0x34:		return ((input_port_0_r(0) & 0x03) << 6);
		case 0x11:
		case 0x15:
		case 0x31:
		case 0x35:		return ((input_port_0_r(0) & 0x0C) << 4);
		case 0x12:
		case 0x16:
		case 0x32:
		case 0x36:		return ((input_port_0_r(0) & 0x30) << 2);
		case 0x13:
		case 0x17:
		case 0x33:
		case 0x37:		return ((input_port_0_r(0) & 0xC0) << 0);
		/* Just in case */
		default:		return 0xFF;
	}
}

void dominos_ac_signal_flip(int dummy)
{
	ac_line = ac_line ^ 0x80;
}

/***************************************************************************
Sync

When reading from SYNC:
   D4 = ATTRACT
   D5 = VRESET
   D6 = VBLANK*
   D7 = 60Hz AC line reference

The only one of these I really understand is the VBLANK...
***************************************************************************/
READ_HANDLER( dominos_sync_r )
{
	return ((input_port_4_r(0) & 0x60) | dominos_attract | ac_line);
}



/***************************************************************************
Attract
***************************************************************************/
WRITE_HANDLER( dominos_attract_w )
{
	dominos_attract = (offset & 0x01) << 4;
	discrete_sound_w(3, !(offset & 0x01));
}

/***************************************************************************
Lamps
***************************************************************************/
WRITE_HANDLER( dominos_lamp1_w )
{
	/* Address Line 0 is the data passed to LAMP1 */
	set_led_status(0,offset & 0x01);
}

WRITE_HANDLER( dominos_lamp2_w )
{
	/* Address Line 0 is the data passed to LAMP2 */
	set_led_status(1,offset & 0x01);
}

/***************************************************************************
Sound function
***************************************************************************/
WRITE_HANDLER( dominos_tumble_w )
{
	discrete_sound_w(2, offset & 0x01);
}
