#include "driver.h"
#include "vidhrdw/generic.h"



data16_t *blockout_videoram;
data16_t *blockout_frontvideoram;



static void setcolor(int color,int rgb)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b;


	/* red component */
	bit0 = (rgb >> 0) & 0x01;
	bit1 = (rgb >> 1) & 0x01;
	bit2 = (rgb >> 2) & 0x01;
	bit3 = (rgb >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	bit0 = (rgb >> 4) & 0x01;
	bit1 = (rgb >> 5) & 0x01;
	bit2 = (rgb >> 6) & 0x01;
	bit3 = (rgb >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	bit0 = (rgb >> 8) & 0x01;
	bit1 = (rgb >> 9) & 0x01;
	bit2 = (rgb >> 10) & 0x01;
	bit3 = (rgb >> 11) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	palette_set_color(color,r,g,b);
}

WRITE16_HANDLER( blockout_paletteram_w )
{
	COMBINE_DATA(&paletteram16[offset]);
	setcolor(offset,paletteram16[offset]);
}

WRITE16_HANDLER( blockout_frontcolor_w )
{
	static data16_t color;

	COMBINE_DATA(&color);
	setcolor(512,color);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( blockout )
{
	/* Allocate temporary bitmaps */
	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



static void updatepixels(int x,int y)
{
	data16_t front,back;
	int color;


	if (x < Machine->visible_area.min_x ||
			x > Machine->visible_area.max_x ||
			y < Machine->visible_area.min_y ||
			y > Machine->visible_area.max_y)
		return;

	front = blockout_videoram[y*256+x/2];
	back = blockout_videoram[0x10000 + y*256+x/2];

	if (front>>8) color = front>>8;
	else color = (back>>8) + 256;
	plot_pixel(tmpbitmap, x, y, Machine->pens[color]);

	if (front&0xff) color = front&0xff;
	else color = (back&0xff) + 256;
	plot_pixel(tmpbitmap, x+1, y, Machine->pens[color]);
}



WRITE16_HANDLER( blockout_videoram_w )
{
	data16_t oldword = blockout_videoram[offset];
	COMBINE_DATA(&blockout_videoram[offset]);

	if (oldword != blockout_videoram[offset])
	{
		updatepixels((offset % 256)*2,(offset / 256) % 256);
	}
}



VIDEO_UPDATE( blockout )
{
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	{
		int x,y,color;


		color = Machine->pens[512];

		for (y = 0;y < 256;y++)
		{
			for (x = 0;x < 320;x+=8)
			{
				int d;


				d = blockout_frontvideoram[y*64+(x/8)];

				if (d)
				{
					if (d&0x80) plot_pixel(bitmap, x  , y, color);
					if (d&0x40) plot_pixel(bitmap, x+1, y, color);
					if (d&0x20) plot_pixel(bitmap, x+2, y, color);
					if (d&0x10) plot_pixel(bitmap, x+3, y, color);
					if (d&0x08) plot_pixel(bitmap, x+4, y, color);
					if (d&0x04) plot_pixel(bitmap, x+5, y, color);
					if (d&0x02) plot_pixel(bitmap, x+6, y, color);
					if (d&0x01) plot_pixel(bitmap, x+7, y, color);
				}
			}
		}
	}
}
