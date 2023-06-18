/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *blueprnt_scrollram;

static int gfx_bank,flipscreen;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Blue Print doesn't have color PROMs. For sprites, the ROM data is directly
  converted into colors; for characters, it is converted through the color
  code (bits 0-2 = RBG for 01 pixels, bits 3-5 = RBG for 10 pixels, 00 pixels
  always black, 11 pixels use the OR of bits 0-2 and 3-5. Bit 6 is intensity
  control)

***************************************************************************/

PALETTE_INIT( blueprnt )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 16;i++)
	{
		int r = ((i >> 0) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		int g = ((i >> 2) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		int b = ((i >> 1) & 1) * ((i & 0x08) ? 0xbf : 0xff);
		palette_set_color(i,r,g,b);
	}


	/* chars */
	for (i = 0;i < 128;i++)
	{
		int base =  (i & 0x40) ? 8 : 0;
		COLOR(0,4*i+0) = base + 0;
		COLOR(0,4*i+1) = base + ((i >> 0) & 7);
		COLOR(0,4*i+2) = base + ((i >> 3) & 7);
		COLOR(0,4*i+3) = base + (((i >> 0) & 7) | ((i >> 3) & 7));
	}

	/* sprites */
	for (i = 0;i < 8;i++)
		COLOR(1,i) = i;
}



WRITE_HANDLER( blueprnt_flipscreen_w )
{
	if (flipscreen != (~data & 2))
	{
		flipscreen = ~data & 2;
		memset(dirtybuffer,1,videoram_size);
	}

	if (gfx_bank != ((data & 4) >> 2))
	{
		gfx_bank = ((data & 4) >> 2);
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( blueprnt )
{
	int offs;
	int scroll[32];


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 31 - offs / 32;
			sy = offs % 32;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 256 * gfx_bank,
					colorram[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int i;


		if (flipscreen)
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = blueprnt_scrollram[32-i];	/* mmm... */
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -blueprnt_scrollram[30-i];	/* mmm... */
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sx,sy,flipx,flipy;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs + 0];
		flipx = spriteram[offs + 2] & 0x40;
		flipy = spriteram[offs + 2 - 4] & 0x80;	/* -4? Awkward, isn't it? */
		if (flipscreen)
		{
			sx = 248 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 1],
				0,
				flipx,flipy,
				2+sx,sy-1,	/* sprites are slightly misplaced, regardless of the screen flip */
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}


	/* redraw the characters which have priority over sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (colorram[offs] & 0x80)
		{
			int sx,sy;


			sx = 31 - offs / 32;
			sy = offs % 32;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + 256 * gfx_bank,
					colorram[offs] & 0x7f,
					flipscreen,flipscreen,
					8*sx,(8*sy+scroll[sx]) & 0xff,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
