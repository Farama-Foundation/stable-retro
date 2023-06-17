/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *rallyx_videoram2,*rallyx_colorram2;
unsigned char *rallyx_radarx,*rallyx_radary,*rallyx_radarattr;
size_t rallyx_radarram_size;
unsigned char *rallyx_scrollx,*rallyx_scrolly;
static unsigned char *dirtybuffer2;	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct mame_bitmap *tmpbitmap1;



static struct rectangle radarvisiblearea =
{
	28*8, 36*8-1,
	0*8, 28*8-1
};

static struct rectangle radarvisibleareaflip =
{
	0*8, 8*8-1,
	0*8, 28*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Rally X has one 32x8 palette PROM and one 256x4 color lookup table PROM.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  In Rally-X there is a 1 kohm pull-down on B only, in Locomotion the
  1 kohm pull-down is an all three RGB outputs.

***************************************************************************/
PALETTE_INIT( rallyx )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);

		color_prom++;
	}

	/* color_prom now points to the beginning of the lookup table */

	/* character lookup table */
	/* sprites use the same color lookup table as characters */
	/* characters use colors 0-15 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0x0f;

	/* radar dots lookup table */
	/* they use colors 16-19 */
	for (i = 0;i < 4;i++)
		COLOR(2,i) = 16 + i;
}

PALETTE_INIT( locomotn )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 6) & 0x01;
		bit1 = (*color_prom >> 7) & 0x01;
		b = 0x50 * bit0 + 0xab * bit1;

		palette_set_color(i,r,g,b);

		color_prom++;
	}

	/* color_prom now points to the beginning of the lookup table */

	/* character lookup table */
	/* sprites use the same color lookup table as characters */
	/* characters use colors 0-15 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) & 0x0f;

	/* radar dots lookup table */
	/* they use colors 16-19 */
	for (i = 0;i < 4;i++)
		COLOR(2,i) = 16 + i;
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( rallyx )
{
	if (video_start_generic() != 0)
		return 1;

	if ((dirtybuffer2 = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap1 = auto_bitmap_alloc(32*8,32*8)) == 0)
		return 1;

	return 0;
}



WRITE_HANDLER( rallyx_videoram2_w )
{
	if (rallyx_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		rallyx_videoram2[offset] = data;
	}
}


WRITE_HANDLER( rallyx_colorram2_w )
{
	if (rallyx_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		rallyx_colorram2[offset] = data;
	}
}



WRITE_HANDLER( rallyx_flipscreen_w )
{
	if (flip_screen != (data & 1))
	{
		flip_screen_set(data & 1);
		memset(dirtybuffer,1,videoram_size);
		memset(dirtybuffer2,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( rallyx )
{
	int offs,sx,sy;
	int scrollx,scrolly;
const int displacement = 1;


	if (flip_screen)
	{
		scrollx = (*rallyx_scrollx - displacement) + 32;
		scrolly = (*rallyx_scrolly + 16) - 32;
	}
	else
	{
		scrollx = -(*rallyx_scrollx - 3*displacement);
		scrolly = -(*rallyx_scrolly + 16);
	}


	/* draw the below sprite priority characters */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (rallyx_colorram2[offs] & 0x20)  continue;

		if (dirtybuffer2[offs])
		{
			int flipx,flipy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			flipx = ~rallyx_colorram2[offs] & 0x40;
			flipy = rallyx_colorram2[offs] & 0x80;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap1,Machine->gfx[0],
					rallyx_videoram2[offs],
					rallyx_colorram2[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = (offs % 32) ^ 4;
			sy = offs / 32 - 2;
			flipx = ~colorram[offs] & 0x40;
			flipy = colorram[offs] & 0x80;
			if (flip_screen)
			{
				sx = 7 - sx;
				sy = 27 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					&radarvisibleareaflip,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		sx = spriteram[offs + 1] + ((spriteram_2[offs + 1] & 0x80) << 1) - displacement;
		sy = 225 - spriteram_2[offs] - displacement;

		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] & 0xfc) >> 2,
				spriteram_2[offs + 1] & 0x3f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* draw the above sprite priority characters */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int flipx,flipy;


		if (!(rallyx_colorram2[offs] & 0x20))  continue;

		sx = offs % 32;
		sy = offs / 32;
		flipx = ~rallyx_colorram2[offs] & 0x40;
		flipy = rallyx_colorram2[offs] & 0x80;
		if (flip_screen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				rallyx_videoram2[offs],
				rallyx_colorram2[offs] & 0x3f,
				flipx,flipy,
				(8*sx + scrollx) & 0xff,(8*sy + scrolly) & 0xff,
				0,TRANSPARENCY_NONE,0);
		drawgfx(bitmap,Machine->gfx[0],
				rallyx_videoram2[offs],
				rallyx_colorram2[offs] & 0x3f,
				flipx,flipy,
				((8*sx + scrollx) & 0xff) - 256,(8*sy + scrolly) & 0xff,
				0,TRANSPARENCY_NONE,0);
	}


	/* radar */
	if (flip_screen)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisibleareaflip,TRANSPARENCY_NONE,0);
	else
		copybitmap(bitmap,tmpbitmap,0,0,28*8,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the cars on the radar */
	for (offs = 0; offs < rallyx_radarram_size;offs++)
	{
		int x,y;

		x = rallyx_radarx[offs] + ((~rallyx_radarattr[offs] & 0x01) << 8);
		y = 237 - rallyx_radary[offs];
		if (flip_screen) x -= 3;

		drawgfx(bitmap,Machine->gfx[2],
				((rallyx_radarattr[offs] & 0x0e) >> 1) ^ 0x07,
				0,
				0,0,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,3);
	}
}



VIDEO_UPDATE( jungler )
{
	int offs,sx,sy;
	int scrollx,scrolly;
const int displacement = 0;


	if (flip_screen)
	{
		scrollx = (*rallyx_scrollx - displacement) + 32;
		scrolly = (*rallyx_scrolly + 16) - 32;
	}
	else
	{
		scrollx = -(*rallyx_scrollx - 3*displacement);
		scrolly = -(*rallyx_scrolly + 16);
	}


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int flipx,flipy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			flipx = ~rallyx_colorram2[offs] & 0x40;
			flipy = rallyx_colorram2[offs] & 0x80;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap1,Machine->gfx[0],
					rallyx_videoram2[offs],
					rallyx_colorram2[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = (offs % 32) ^ 4;
			sy = offs / 32 - 2;
			flipx = ~colorram[offs] & 0x40;
			flipy = colorram[offs] & 0x80;
			if (flip_screen)
			{
				sx = 7 - sx;
				sy = 27 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					&radarvisibleareaflip,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		sx = spriteram[offs + 1] + ((spriteram_2[offs + 1] & 0x80) << 1) - displacement;
		sy = 225 - spriteram_2[offs] - displacement;

		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] & 0xfc) >> 2,
				spriteram_2[offs + 1] & 0x3f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* radar */
	if (flip_screen)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisibleareaflip,TRANSPARENCY_NONE,0);
	else
		copybitmap(bitmap,tmpbitmap,0,0,28*8,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the cars on the radar */
	for (offs = 0; offs < rallyx_radarram_size;offs++)
	{
		int x,y;

		x = rallyx_radarx[offs] + ((~rallyx_radarattr[offs] & 0x08) << 5);
		y = 237 - rallyx_radary[offs];

		drawgfx(bitmap,Machine->gfx[2],
				(rallyx_radarattr[offs] & 0x07) ^ 0x07,
				0,
				0,0,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}



VIDEO_UPDATE( locomotn )
{
	int offs,sx,sy;
const int displacement = 0;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int flipx,flipy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			/* not a mistake, one bit selects both  flips */
			flipx = rallyx_colorram2[offs] & 0x80;
			flipy = rallyx_colorram2[offs] & 0x80;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap1,Machine->gfx[0],
					(rallyx_videoram2[offs]&0x7f) + 2*(rallyx_colorram2[offs]&0x40) + 2*(rallyx_videoram2[offs]&0x80),
					rallyx_colorram2[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = (offs % 32) ^ 4;
			sy = offs / 32 - 2;
			/* not a mistake, one bit selects both  flips */
			flipx = colorram[offs] & 0x80;
			flipy = colorram[offs] & 0x80;
			if (flip_screen)
			{
				sx = 7 - sx;
				sy = 27 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs]&0x7f) + 2*(colorram[offs]&0x40) + 2*(videoram[offs]&0x80),
					colorram[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					&radarvisibleareaflip,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		if (flip_screen)
		{
			scrollx = (*rallyx_scrollx) + 32;
			scrolly = (*rallyx_scrolly + 16) - 32;
		}
		else
		{
			scrollx = -(*rallyx_scrollx);
			scrolly = -(*rallyx_scrolly + 16);
		}

		copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* radar */
	if (flip_screen)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisibleareaflip,TRANSPARENCY_NONE,0);
	else
		copybitmap(bitmap,tmpbitmap,0,0,28*8,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		sx = spriteram[offs + 1] + ((spriteram_2[offs + 1] & 0x80) << 1) - displacement;
		sy = 225 - spriteram_2[offs] - displacement;

		/* handle reduced visible area in some games */
		if (flip_screen && Machine->drv->default_visible_area.max_x == 32*8-1) sx += 32;

		drawgfx(bitmap,Machine->gfx[1],
				((spriteram[offs] & 0x7c) >> 2) + 0x20*(spriteram[offs] & 0x01) + ((spriteram[offs] & 0x80) >> 1),
				spriteram_2[offs + 1] & 0x3f,
				spriteram[offs] & 2,spriteram[offs] & 2,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* draw the cars on the radar */
	for (offs = 0; offs < rallyx_radarram_size;offs++)
	{
		int x,y;

		x = rallyx_radarx[offs] + ((~rallyx_radarattr[offs] & 0x08) << 5);
		y = 237 - rallyx_radary[offs];
		if (flip_screen) x -= 3;

		/* handle reduced visible area in some games */
		if (flip_screen && Machine->drv->default_visible_area.max_x == 32*8-1) x += 32;

		drawgfx(bitmap,Machine->gfx[2],
				(rallyx_radarattr[offs & 0x0f] & 0x07) ^ 0x07,
				0,
				0,0,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,3);
	}
}



VIDEO_UPDATE( commsega )
{
	int offs,sx,sy;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int flipx,flipy;


			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;
			/* not a mistake, one bit selects both  flips */
			flipx = rallyx_colorram2[offs] & 0x80;
			flipy = rallyx_colorram2[offs] & 0x80;
			if (flip_screen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap1,Machine->gfx[0],
					(rallyx_videoram2[offs]&0x7f) + 2*(rallyx_colorram2[offs]&0x40) + 2*(rallyx_videoram2[offs]&0x80),
					rallyx_colorram2[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int flipx,flipy;


			dirtybuffer[offs] = 0;

			sx = (offs % 32) ^ 4;
			sy = offs / 32 - 2;
			/* not a mistake, one bit selects both  flips */
			flipx = colorram[offs] & 0x80;
			flipy = colorram[offs] & 0x80;
			if (flip_screen)
			{
				sx = 7 - sx;
				sy = 27 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs]&0x7f) + 2*(colorram[offs]&0x40) + 2*(videoram[offs]&0x80),
					colorram[offs] & 0x3f,
					flipx,flipy,
					8*sx,8*sy,
					&radarvisibleareaflip,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		if (flip_screen)
		{
			scrollx = (*rallyx_scrollx) + 32;
			scrolly = (*rallyx_scrolly + 16) - 32;
		}
		else
		{
			scrollx = -(*rallyx_scrollx);
			scrolly = -(*rallyx_scrolly + 16);
		}

		copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* radar */
	if (flip_screen)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisibleareaflip,TRANSPARENCY_NONE,0);
	else
		copybitmap(bitmap,tmpbitmap,0,0,28*8,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		int flipx,flipy;


		sx = spriteram[offs + 1] - 1;
		sy = 224 - spriteram_2[offs];
if (flip_screen) sx += 32;
		flipx = ~spriteram[offs] & 1;
		flipy = ~spriteram[offs] & 2;
		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
		}

		if (spriteram[offs] & 0x01)	/* ??? */
			drawgfx(bitmap,Machine->gfx[1],
					((spriteram[offs] & 0x7c) >> 2) + 0x20*(spriteram[offs] & 0x01) + ((spriteram[offs] & 0x80) >> 1),
					spriteram_2[offs + 1] & 0x3f,
					flipx,flipy,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_COLOR,0);
	}


	/* draw the cars on the radar */
	for (offs = 0; offs < rallyx_radarram_size;offs++)
	{
		int x,y;


		/* it looks like the addresses used are
		   a000-a003  a004-a00f
		   8020-8023  8034-803f
		   8820-8823  8834-883f
		   so 8024-8033 and 8824-8833 are not used
		*/

		x = rallyx_radarx[offs] + ((~rallyx_radarattr[offs & 0x0f] & 0x08) << 5);
		if (flip_screen) x += 32;
		y = 237 - rallyx_radary[offs];


		drawgfx(bitmap,Machine->gfx[2],
				(rallyx_radarattr[offs & 0x0f] & 0x07) ^ 0x07,
				0,
				0,0,
				x,y,
				&Machine->visible_area,TRANSPARENCY_PEN,3);
	}
}
