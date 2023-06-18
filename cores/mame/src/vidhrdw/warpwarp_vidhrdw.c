/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *warpwarp_bulletsram;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Warp Warp doesn't use PROMs - the 8-bit code is directly converted into a
  color.

  The color RAM is connected to the RGB output this way (I think - schematics
  are fuzzy):

  bit 7 -- 300 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 300 ohm resistor  -- GREEN
        -- 820 ohm resistor  -- GREEN
        -- 1.6kohm resistor  -- GREEN
        -- 300 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
  bit 0 -- 1.6kohm resistor  -- RED

  Moreover, the bullet is pure white, obtained with three 220 ohm resistors.

***************************************************************************/
PALETTE_INIT( warpwarp )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		r = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;
		/* green component */
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		g = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		b = 0x1f * bit0 + 0x3c * bit1 + 0xa4 * bit2;

		palette_set_color(i,r,g,b);
	}

	for (i = 0;i < TOTAL_COLORS(0);i += 2)
	{
		COLOR(0,i) = 0;			/* black background */
		COLOR(0,i + 1) = i / 2;	/* colored foreground */
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( warpwarp )
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int mx,my,sx,sy;

			mx = offs % 32;
			my = offs / 32;

			if (my == 0)
			{
				sx = 33;
				sy = mx;
			}
			else if (my == 1)
			{
				sx = 0;
				sy = mx;
			}
			else
			{
				sx = mx + 1;
				sy = my;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs],
					0,0,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);

			dirtybuffer[offs] = 0;
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,flip_screen,flip_screen,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	if (warpwarp_bulletsram[0] > 1)
	{
		int x,y;


		x = 260 - warpwarp_bulletsram[0];
		y = 252 - warpwarp_bulletsram[1];
		if (x >= Machine->visible_area.min_x && x+3 <= Machine->visible_area.max_x &&
			y >= Machine->visible_area.min_y && y+3 <= Machine->visible_area.max_y)
		{
			int colour;
			int i,j;


			colour = Machine->pens[0xf6];	/* white */

			for (i = 0;i < 4;i++)
			{
				for (j = 0;j < 4;j++)
				{
					plot_pixel(bitmap, x+j, y+i, colour);
				}
			}
		}
	}
}


WRITE_HANDLER( warpwarp_flip_screen_w )
{
	flip_screen_set(data & 1);
}
