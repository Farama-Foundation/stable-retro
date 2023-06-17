/***************************************************************************

Dottori Kun (Head On's mini game)
(c)1990 SEGA

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/12/15 -

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



/*******************************************************************

	Palette Setting.

*******************************************************************/
WRITE_HANDLER( dotrikun_color_w )
{
	int r, g, b;

	r = ((data & 0x08) ? 0xff : 0x00);
	g = ((data & 0x10) ? 0xff : 0x00);
	b = ((data & 0x20) ? 0xff : 0x00);
	palette_set_color(0, r, g, b);		/* BG color*/

	r = ((data & 0x01) ? 0xff : 0x00);
	g = ((data & 0x02) ? 0xff : 0x00);
	b = ((data & 0x04) ? 0xff : 0x00);
	palette_set_color(1, r, g, b);		/* DOT color*/
}


/*******************************************************************

	Draw Pixel.

*******************************************************************/
WRITE_HANDLER( dotrikun_videoram_w )
{
	int i;
	int x, y;
	int color;


	videoram[offset] = data;

	x = 2 * (((offset % 16) * 8));
	y = 2 * ((offset / 16));

	if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x &&
			y >= Machine->visible_area.min_y &&
			y <= Machine->visible_area.max_y)
	{
		for (i = 0; i < 8; i++)
		{
			color = Machine->pens[((data >> i) & 0x01)];

			/* I think the video hardware doubles pixels, screen would be too small otherwise */
			plot_pixel(tmpbitmap, x + 2*(7 - i),   y,   color);
			plot_pixel(tmpbitmap, x + 2*(7 - i)+1, y,   color);
			plot_pixel(tmpbitmap, x + 2*(7 - i),   y+1, color);
			plot_pixel(tmpbitmap, x + 2*(7 - i)+1, y+1, color);
		}
	}
}


VIDEO_UPDATE( dotrikun )
{
	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */

		for (offs = 0; offs < videoram_size; offs++)
			dotrikun_videoram_w(offs,videoram[offs]);
	}
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
