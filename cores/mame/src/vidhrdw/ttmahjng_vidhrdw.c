/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *ttmahjng_sharedram;
unsigned char *ttmahjng_videoram1;
unsigned char *ttmahjng_videoram2;
size_t ttmahjng_videoram_size;

static struct mame_bitmap *tmpbitmap1;
static struct mame_bitmap *tmpbitmap2;

static int video_flip;
static int video_color_select_1;
static int video_color_select_2;
static int video_remap_1;
static int video_remap_2;
static const unsigned char *ttmahjng_color_prom;

/* Local functions */
static void modify_pen(int pen, int colorindex);
static void common_videoram_w(int offset,int data,
                              int coloroffset, struct mame_bitmap *bitmap);



PALETTE_INIT( ttmahjng )
{
	ttmahjng_color_prom = color_prom;	/* we'll need this later */
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( ttmahjng )
{
	if ((tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	video_flip = 0;
	video_color_select_1 = 0;
	video_color_select_2 = 0;
	video_remap_1 = 1;
	video_remap_2 = 1;

	return 0;
}

/***************************************************************************
  ttmahjng_out0_w
***************************************************************************/
WRITE_HANDLER( ttmahjng_out0_w )
{
	static int last_write = 0;

	if (data == last_write) return;

	video_color_select_1 = ((data & 0x1f) << 2);

	/* Bit 5 is the coin counter. */
	coin_counter_w(0, data & 0x20);

	video_remap_1 = 1;
	last_write = data;
}

/***************************************************************************
  ttmahjng_out1_w
***************************************************************************/
WRITE_HANDLER( ttmahjng_out1_w )
{
	static int last_write = 0;

	if (data == last_write) return;

	video_color_select_2 = ((data & 0x1f) << 2);

	if (video_flip != ((data & 0x20) >> 5))
	{
		video_flip = (data & 0x20) >> 5;
	}

	video_remap_2 = 1;
	last_write = data;
}

/***************************************************************************
  ttmahjng_sharedram_r
***************************************************************************/
READ_HANDLER( ttmahjng_sharedram_r )
{
	return ttmahjng_sharedram[offset];
}

/***************************************************************************
  ttmahjng_sharedram_w
***************************************************************************/
WRITE_HANDLER( ttmahjng_sharedram_w )
{
	ttmahjng_sharedram[offset] = data;
}

/***************************************************************************
  ttmahjng_videoram1_r
***************************************************************************/
READ_HANDLER( ttmahjng_videoram1_r )
{
	return ttmahjng_videoram1[offset];
}

/***************************************************************************
  ttmahjng_videoram2_r
***************************************************************************/
READ_HANDLER( ttmahjng_videoram2_r )
{
	return ttmahjng_videoram1[offset];
}

/***************************************************************************
  ttmahjng_videoram1_w
***************************************************************************/
WRITE_HANDLER( ttmahjng_videoram1_w )
{
	ttmahjng_videoram1[offset] = data;

	common_videoram_w(offset, data, 0, tmpbitmap1);
}

/***************************************************************************
  ttmahjng_videoram2_w
***************************************************************************/
WRITE_HANDLER( ttmahjng_videoram2_w )
{
	ttmahjng_videoram2[offset] = data;

	common_videoram_w(offset, data, 4, tmpbitmap2);
}

/***************************************************************************
  common_videoram_w
***************************************************************************/
static void common_videoram_w(int offset,int data,
                              int coloroffset, struct mame_bitmap *bitmap)
{
	int x, y, color1, color2, color3, color4;

	x = ((offset & 0x3f) << 2);
	y = (offset & 0xffc0) >> 6;

	if (video_flip)
	{
		x = 255 - x;
		y = 255 - y;
	}

	color4 = ((data & 0x80) >> 6) | ((data & 0x08) >> 3);
	color3 = ((data & 0x40) >> 5) | ((data & 0x04) >> 2);
	color2 = ((data & 0x20) >> 4) | ((data & 0x02) >> 1);
	color1 = ((data & 0x10) >> 3) | ((data & 0x01)     );

	if (video_flip)
	{
		plot_pixel(bitmap, x  , y, Machine->pens[color1 | coloroffset]);
		plot_pixel(bitmap, x-1, y, Machine->pens[color2 | coloroffset]);
		plot_pixel(bitmap, x-2, y, Machine->pens[color3 | coloroffset]);
		plot_pixel(bitmap, x-3, y, Machine->pens[color4 | coloroffset]);
	}
	else
	{
		plot_pixel(bitmap, x  , y, Machine->pens[color1 | coloroffset]);
		plot_pixel(bitmap, x+1, y, Machine->pens[color2 | coloroffset]);
		plot_pixel(bitmap, x+2, y, Machine->pens[color3 | coloroffset]);
		plot_pixel(bitmap, x+3, y, Machine->pens[color4 | coloroffset]);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( ttmahjng )
{
	if (video_remap_1)
	{
		modify_pen(0, video_color_select_1 + 0);
		modify_pen(1, video_color_select_1 + 1);
		modify_pen(2, video_color_select_1 + 2);
		modify_pen(3, video_color_select_1 + 3);
	}

	if (video_remap_2)
	{
		modify_pen(4, video_color_select_2 + 0);
		modify_pen(5, video_color_select_2 + 1);
		modify_pen(6, video_color_select_2 + 2);
		modify_pen(7, video_color_select_2 + 3);
	}


	if (get_vh_global_attribute_changed() || video_remap_1 || video_remap_2)
	{
		int offs;

		/* redraw bitmaps*/
		for (offs = 0; offs < ttmahjng_videoram_size; offs++)
		{
			ttmahjng_videoram1_w(offs, ttmahjng_videoram1[offs]);
			ttmahjng_videoram2_w(offs, ttmahjng_videoram2[offs]);
		}
	}

	video_remap_1 = 0;
	video_remap_2 = 0;


	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap1,0,0,0,0,&Machine->visible_area,TRANSPARENCY_COLOR,0);
}

/***************************************************************************
  mofify_pen
 ***************************************************************************/
static void modify_pen(int pen, int colorindex)
{
	int r,g,b,color;

	color = ttmahjng_color_prom[colorindex];

	b = ((color & 1) ? 0xff : 0x00);
	g = ((color & 2) ? 0xff : 0x00);
	r = ((color & 4) ? 0xff : 0x00);

	palette_set_color(pen,r,g,b);
}
