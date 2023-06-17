/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *route16_sharedram;
unsigned char *route16_videoram1;
unsigned char *route16_videoram2;
size_t route16_videoram_size;
int route16_hardware;

static struct mame_bitmap *tmpbitmap1;
static struct mame_bitmap *tmpbitmap2;

static int video_flip;
static int video_color_select_1;
static int video_color_select_2;
static int video_disable_1 = 0;
static int video_disable_2 = 0;
static int video_remap_1;
static int video_remap_2;
static const unsigned char *route16_color_prom;

/* Local functions */
static void modify_pen(int pen, int colorindex);
static void common_videoram_w(int offset,int data,
                              int coloroffset, struct mame_bitmap *bitmap);



PALETTE_INIT( route16 )
{
	route16_color_prom = color_prom;	/* we'll need this later */
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( route16 )
{
	if ((tmpbitmap1 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	video_flip = 0;
	video_color_select_1 = 0;
	video_color_select_2 = 0;
	video_disable_1 = 0;
	video_disable_2 = 0;
	video_remap_1 = 1;
	video_remap_2 = 1;

	return 0;
}


/***************************************************************************
  route16_out0_w
***************************************************************************/
WRITE_HANDLER( route16_out0_w )
{
	static int last_write = 0;

	if (data == last_write) return;

	video_disable_1 = ((data & 0x02) << 6) && route16_hardware;
	video_color_select_1 = ((data & 0x1f) << 2);

	/* Bit 5 is the coin counter. */
	coin_counter_w(0, data & 0x20);

	video_remap_1 = 1;
	last_write = data;
}

/***************************************************************************
  route16_out1_w
***************************************************************************/
WRITE_HANDLER( route16_out1_w )
{
	static int last_write = 0;

	if (data == last_write) return;

	video_disable_2 = ((data & 0x02) << 6 ) && route16_hardware;
	video_color_select_2 = ((data & 0x1f) << 2);

	if (video_flip != ((data & 0x20) >> 5))
	{
		video_flip = (data & 0x20) >> 5;
	}

	video_remap_2 = 1;
	last_write = data;
}

/***************************************************************************

  Handle Stratovox's extra sound effects.

***************************************************************************/
WRITE_HANDLER( stratvox_sn76477_w )
{
	/* get out for Route 16 */
	if (route16_hardware) return;

    /***************************************************************
     * AY8910 output bits are connected to...
     * 7    - direct: 5V * 30k/(100+30k) = 1.15V - via DAC??
     * 6    - SN76477 mixer a
     * 5    - SN76477 mixer b
     * 4    - SN76477 mixer c
     * 3    - SN76477 envelope 1
	 * 2	- SN76477 envelope 2
     * 1    - SN76477 vco
     * 0    - SN76477 enable
     ***************************************************************/
    SN76477_mixer_w(0,(data >> 4) & 7);
	SN76477_envelope_w(0,(data >> 2) & 3);
    SN76477_vco_w(0,(data >> 1) & 1);
    SN76477_enable_w(0,data & 1);
}

/***************************************************************************
  route16_sharedram_r
***************************************************************************/
READ_HANDLER( route16_sharedram_r )
{
	return route16_sharedram[offset];
}

/***************************************************************************
  route16_sharedram_w
***************************************************************************/
WRITE_HANDLER( route16_sharedram_w )
{
	route16_sharedram[offset] = data;

	/* 4313-4319 are used in Route 16 as triggers to wake the other CPU*/
	if (offset >= 0x0313 && offset <= 0x0319 && data == 0xff && route16_hardware)
	{
		/* Let the other CPU run*/
		cpu_yield();
	}
}

/***************************************************************************
  guessing that the unconnected IN3 and OUT2 on the stratvox schematic
  are hooked up for speakres and spacecho to somehow read the variable
  resistors (eg a voltage ramp), using a write to OUT2 as a trigger
  and then bits 0-2 of IN3 going low when each pot "matches". the VRx
  values can be seen when IN0=0x55 and p1b1 is held during power on.
  this would then be checking that the sounds are mixed correctly.
***************************************************************************/
static int speakres_vrx;

READ_HANDLER ( speakres_in3_r )
{
	int bit2=4, bit1=2, bit0=1;

	/* just using a counter, the constants are the number of reads
	   before going low, each read is 40 cycles apart. the constants
	   were chosen based on the startup tests and for vr0=vr2 */
	speakres_vrx++;
	if(speakres_vrx>0x300) bit0=0;		/* VR0 100k ohm - speech */
	if(speakres_vrx>0x200) bit1=0;		/* VR1  50k ohm - main volume */
	if(speakres_vrx>0x300) bit2=0;		/* VR2 100k ohm - explosion */

	return 0xf8|bit2|bit1|bit0;
}

WRITE_HANDLER ( speakres_out2_w )
{
	speakres_vrx=0;
}


/***************************************************************************
  route16_videoram1_r
***************************************************************************/
READ_HANDLER( route16_videoram1_r )
{
	return route16_videoram1[offset];
}

/***************************************************************************
  route16_videoram2_r
***************************************************************************/
READ_HANDLER( route16_videoram2_r )
{
	return route16_videoram1[offset];
}

/***************************************************************************
  route16_videoram1_w
***************************************************************************/
WRITE_HANDLER( route16_videoram1_w )
{
	route16_videoram1[offset] = data;

	common_videoram_w(offset, data, 0, tmpbitmap1);
}

/***************************************************************************
  route16_videoram2_w
***************************************************************************/
WRITE_HANDLER( route16_videoram2_w )
{
	route16_videoram2[offset] = data;

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
VIDEO_UPDATE( route16 )
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
		for (offs = 0; offs < route16_videoram_size; offs++)
		{
			route16_videoram1_w(offs, route16_videoram1[offs]);
			route16_videoram2_w(offs, route16_videoram2[offs]);
		}
	}

	video_remap_1 = 0;
	video_remap_2 = 0;


	if (!video_disable_2)
	{
		copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	if (!video_disable_1)
	{
		if (video_disable_2)
			copybitmap(bitmap,tmpbitmap1,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
		else
			copybitmap(bitmap,tmpbitmap1,0,0,0,0,&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}
}

/***************************************************************************
  mofify_pen
 ***************************************************************************/
static void modify_pen(int pen, int colorindex)
{
	int r,g,b,color;

	color = route16_color_prom[colorindex];

	r = ((color & 1) ? 0xff : 0x00);
	g = ((color & 2) ? 0xff : 0x00);
	b = ((color & 4) ? 0xff : 0x00);

	palette_set_color(pen,r,g,b);
}
