/***************************************************************************

	Atari Clay Shoot hardware

	driver by Zsolt Vasvari

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/*************************************
 *
 *	Palette generation
 *
 *************************************/

PALETTE_INIT( clayshoo )
{
	palette_set_color(0,0x00,0x00,0x00); /* black */
	palette_set_color(1,0xff,0xff,0xff);  /* white */
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

WRITE_HANDLER( clayshoo_videoram_w )
{
	UINT8 x,y;
	int i;


	x = ((offset & 0x1f) << 3);
	y = 191 - (offset >> 5);

	for (i = 0; i < 8; i++)
	{
		plot_pixel(tmpbitmap, x, y, (data & 0x80) ? Machine->pens[1] : Machine->pens[0]);

		x++;
		data <<= 1;
	}
}
