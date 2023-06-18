/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of Astro Fighter hardware games.

  Lee Taylor 28/11/1997

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *astrof_color;
unsigned char *tomahawk_protection;

static int do_modify_palette = 0;
static int palette_bank = -1, red_on = -1;
static const unsigned char *prom;


/* Just save the colorprom pointer */
PALETTE_INIT( astrof )
{
	prom = color_prom;
}

/***************************************************************************

  Convert the color PROMs into a more useable format.

  The palette PROMs are connected to the RGB output this way:

  bit 0 -- RED
        -- RED
        -- GREEN
	  	-- GREEN
        -- BLUE
  bit 5 -- BLUE

  I couldn't really determine the resistances, (too many resistors and capacitors)
  so the value below might be off a tad. But since there is also a variable
  resistor for each color gun, this is one of the concievable settings

***************************************************************************/
static void modify_palette(void)
{
	int i, col_index;

	col_index = (palette_bank ? 16 : 0);

	for (i = 0;i < Machine->drv->total_colors; i++)
	{
		int bit0,bit1,r,g,b;

		bit0 = ((prom[col_index] >> 0) & 0x01) | (red_on >> 3);
		bit1 = ((prom[col_index] >> 1) & 0x01) | (red_on >> 3);
		r = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( prom[col_index] >> 2) & 0x01;
		bit1 = ( prom[col_index] >> 3) & 0x01;
		g = 0xc0 * bit0 + 0x3f * bit1;

		bit0 = ( prom[col_index] >> 4) & 0x01;
		bit1 = ( prom[col_index] >> 5) & 0x01;
		b = 0xc0 * bit0 + 0x3f * bit1;

		col_index++;

		palette_set_color(i,r,g,b);
	}
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( astrof )
{
	if ((colorram = auto_malloc(videoram_size)) == 0)
		return 1;
	if (video_start_generic())
		return 1;

	do_modify_palette = 0;
	palette_bank = -1;
	red_on = -1;

	return 0;
}



static void common_videoram_w(int offset, int data, int color)
{
	/* DO NOT try to optimize this by comparing if the value actually changed.
	   The games write the same data with a different color. For example, the
	   fuel meter in Astro Fighter doesn't work with that 'optimization' */

	int i,x,y,fore,back;
	int dx = 1;

	videoram[offset] = data;
	colorram[offset] = color;

	fore = Machine->pens[color | 1];
	back = Machine->pens[color    ];

	x = (offset >> 8) << 3;
	y = 255 - (offset & 0xff);

	if (flip_screen)
	{
		x = 255 - x;
		y = 255 - y;
		dx = -1;
	}

	for (i = 0; i < 8; i++)
	{
		plot_pixel(tmpbitmap, x, y, (data & 1) ? fore : back);

		x += dx;
		data >>= 1;
	}
}

WRITE_HANDLER( astrof_videoram_w )
{
	/* Astro Fighter's palette is set in astrof_video_control2_w, D0 is unused*/
	common_videoram_w(offset, data, *astrof_color & 0x0e);
}

WRITE_HANDLER( tomahawk_videoram_w )
{
	/* Tomahawk's palette is set per byte*/
	common_videoram_w(offset, data, (*astrof_color & 0x0e) | ((*astrof_color & 0x01) << 4));
}


WRITE_HANDLER( astrof_video_control1_w )
{
	/* Video control register 1*/
	/**/
	/* Bit 0     = Flip screen*/
	/* Bit 1     = Shown in schematics as what appears to be a screen clear*/
	/*             bit, but it's always zero in Astro Fighter*/
	/* Bit 2     = Not hooked up in the schematics, but at one point the game*/
	/*			   sets it to 1.*/
	/* Bit 3-7   = Not hooked up*/

	if (input_port_2_r(0) & 0x02) /* Cocktail mode */
	{
		flip_screen_set(data & 0x01);
	}
}


/* Video control register 2*/
/**/
/* Bit 0     = Hooked up to a connector called OUT0, don't know what it does*/
/* Bit 1     = Hooked up to a connector called OUT1, don't know what it does*/
/* Bit 2     = Palette select in Astro Fighter, unused in Tomahawk*/
/* Bit 3     = Turns on RED color gun regardless of what the value is*/
/* 			   in the color PROM*/
/* Bit 4-7   = Not hooked up*/

WRITE_HANDLER( astrof_video_control2_w )
{
	if (palette_bank != (data & 0x04))
	{
		palette_bank = (data & 0x04);
		do_modify_palette = 1;
	}

	if (red_on != (data & 0x08))
	{
		red_on = data & 0x08;
		do_modify_palette = 1;
	}

	/* Defer changing the colors to avoid flicker */
}

WRITE_HANDLER( tomahawk_video_control2_w )
{
	if (palette_bank == -1)
	{
		palette_bank = 0;
		do_modify_palette = 1;
	}

	if (red_on != (data & 0x08))
	{
		red_on = data & 0x08;
		do_modify_palette = 1;
	}

	/* Defer changing the colors to avoid flicker */
}


READ_HANDLER( tomahawk_protection_r )
{
	/* flip the byte */

	int res = ((*tomahawk_protection & 0x01) << 7) |
			  ((*tomahawk_protection & 0x02) << 5) |
			  ((*tomahawk_protection & 0x04) << 3) |
			  ((*tomahawk_protection & 0x08) << 1) |
			  ((*tomahawk_protection & 0x10) >> 1) |
			  ((*tomahawk_protection & 0x20) >> 3) |
			  ((*tomahawk_protection & 0x40) >> 5) |
			  ((*tomahawk_protection & 0x80) >> 7);

	return res;
}
/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( astrof )
{
	if (do_modify_palette)
	{
		modify_palette();

		do_modify_palette = 0;
	}

	if (get_vh_global_attribute_changed())
	{
		int offs;

		/* redraw bitmap */
		for (offs = 0; offs < videoram_size; offs++)
		{
			common_videoram_w(offs, videoram[offs], colorram[offs]);
		}
	}
	
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}
