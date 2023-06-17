/***************************************************************************

  vidhrdw.c

  10 Yard Fight

L Taylor
J Clegg

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *yard_scroll_x_low;
unsigned char *yard_scroll_x_high;
unsigned char *yard_scroll_y_low;
unsigned char *yard_score_panel_disabled;
static struct mame_bitmap *scroll_panel_bitmap;

#define SCROLL_PANEL_WIDTH  (14*4)

#define RADAR_PALETTE_BASE (256+16)

static struct rectangle panelvisiblearea =
{
	26*8, 32*8-1,
	1*8, 31*8-1
};

static struct rectangle panelvisibleareaflip =
{
	0*8, 6*8-1,
	1*8, 31*8-1
};


/***************************************************************************

  Convert the color PROMs into a more useable format.

  10 Yard Fight has two 256x4 character palette PROMs, one 32x8 sprite
  palette PROM, one 256x4 sprite color lookup table PROM, and two 256x4
  radar palette PROMs.

  I don't know for sure how the palette PROMs are connected to the RGB
  output, but it's probably something like this; note that RED and BLUE
  are swapped wrt the usual configuration.

  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
  bit 0 -- 1  kohm resistor  -- BLUE

***************************************************************************/
PALETTE_INIT( yard )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* character palette */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = 0;
		bit1 = (color_prom[256] >> 2) & 0x01;
		bit2 = (color_prom[256] >> 3) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[256] >> 0) & 0x01;
		bit2 = (color_prom[256] >> 1) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);

		color_prom++;
	}

	color_prom += 256;
	/* color_prom now points to the beginning of the sprite palette */


	/* sprite palette */
	for (i = 0;i < 16;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i+256,r,g,b);

		color_prom++;
	}

	color_prom += 16;
	/* color_prom now points to the beginning of the sprite lookup table */


	/* sprite lookup table */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = 256 + (*(color_prom++) & 0x0f);

	/* color_prom now points to the beginning of the radar palette */


	/* radar palette */
	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = 0;
		bit1 = (color_prom[256] >> 2) & 0x01;
		bit2 = (color_prom[256] >> 3) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[256] >> 0) & 0x01;
		bit2 = (color_prom[256] >> 1) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i+256+16,r,g,b);

		color_prom++;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( yard )
{
	if ((dirtybuffer = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width*2,Machine->drv->screen_height)) == 0)
		return 1;

	if ((scroll_panel_bitmap = auto_bitmap_alloc(SCROLL_PANEL_WIDTH,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}




WRITE_HANDLER( yard_flipscreen_w )
{
	/* screen flip is handled both by software and hardware */
	data ^= ~readinputport(4) & 1;

	flip_screen_set(data & 1);

	coin_counter_w(0,data & 0x02);
	coin_counter_w(1,data & 0x20);
}


WRITE_HANDLER( yard_scroll_panel_w )
{
	int sx,sy,i;

	sx = ( offset % 16 );
	sy = ( offset / 16 );

	if (sx < 1 || sx > 14)  return;

	sx = 4 * (sx - 1);

	for (i = 0;i < 4;i++)
	{
		int col;

		col = (data >> i) & 0x11;
		col = ((col >> 3) | col) & 3;

		plot_pixel(scroll_panel_bitmap, sx + i, sy, Machine->pens[RADAR_PALETTE_BASE + (sy & 0xfc) + col]);
	}
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( yard )
{
	int offs;


	if (get_vh_global_attribute_changed())
	{
		memset(dirtybuffer,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if (dirtybuffer[offs] || dirtybuffer[offs+1])
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = (offs/2) % 32;
			sy = (offs/2) / 32;
			flipx = videoram[offs+1] & 0x20;

			if (sy >= 32)
			{
				sy -= 32;
				sx += 32;
			}

			if (flip_screen)
			{
				sx = 63 - sx;
				sy = 31 - sy;
				flipx = !flipx;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2),
					videoram[offs+1] & 0x1f,
					flipx,flip_screen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll_x,scroll_y;

		scroll_x = (*yard_scroll_x_high * 0x100) + *yard_scroll_x_low;

		if (flip_screen)
		{
			scroll_x += 256;
			scroll_y = *yard_scroll_y_low ;
		}
		else
		{
			scroll_x = -scroll_x;
			scroll_y = -*yard_scroll_y_low ;
		}

		copyscrollbitmap(bitmap,tmpbitmap,1,&scroll_x,1,&scroll_y,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int code1,code2,bank,sx,sy1,sy2,flipx,flipy;


		bank  = (spriteram[offs + 1] & 0x20) >> 5;
		code1 =  spriteram[offs + 2] & 0xbf;
		sx    =  spriteram[offs + 3];
		sy1   =  241 - spriteram[offs];
		flipx =  spriteram[offs + 1] & 0x40;
		flipy =  spriteram[offs + 1] & 0x80;

		if (flipy)
		{
			code2 = code1;
			code1 += 0x40;
		}
		else
		{
			code2 = code1 + 0x40;
		}

		if (flip_screen)
		{
			flipx = !flipx;
			flipy = !flipy;
			sx  = 240 - sx;
			sy2 = 224 - sy1;
			sy1 = sy2 + 0x10;
		}
		else
		{
			sy2 = sy1 + 0x10;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code1 + 256 * bank,
				spriteram[offs + 1] & 0x1f,
				flipx,flipy,
				sx, sy1,
				&Machine->visible_area,TRANSPARENCY_COLOR,256);

		drawgfx(bitmap,Machine->gfx[1],
				code2 + 256 * bank,
				spriteram[offs + 1] & 0x1f,
				flipx,flipy,
				sx, sy2,
				&Machine->visible_area,TRANSPARENCY_COLOR,256);
	}


	/* draw the static bitmapped area to screen */
	if (! *yard_score_panel_disabled)
	{
		int xpos;

		xpos = flip_screen ? Machine->visible_area.min_x - 8 :
		                     Machine->visible_area.max_x + 1 - SCROLL_PANEL_WIDTH;

		copybitmap(bitmap,scroll_panel_bitmap,flip_screen,flip_screen,
		           xpos,0,
				   flip_screen ? &panelvisibleareaflip : &panelvisiblearea,TRANSPARENCY_NONE,0);
	}
}
