/***************************************************************************

Minivader (Space Invaders's mini game)
(c)1990 Taito Corporation

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/19 -

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/*******************************************************************

	Palette Setting.

*******************************************************************/

PALETTE_INIT( minivadr )
{
	palette_set_color(0,0x00,0x00,0x00);
	palette_set_color(1,0xff,0xff,0xff);
}


/*******************************************************************

	Draw Pixel.

*******************************************************************/
WRITE_HANDLER( minivadr_videoram_w )
{
	int i;
	int x, y;
	int color;


	videoram[offset] = data;

	x = (offset % 32) * 8;
	y = (offset / 32);

	if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x &&
			y >= Machine->visible_area.min_y &&
			y <= Machine->visible_area.max_y)
	{
		for (i = 0; i < 8; i++)
		{
			color = Machine->pens[((data >> i) & 0x01)];

			plot_pixel(tmpbitmap, x + (7 - i), y, color);
		}
	}
}


VIDEO_UPDATE( minivadr )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
			minivadr_videoram_w(offs,videoram[offs]);
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
