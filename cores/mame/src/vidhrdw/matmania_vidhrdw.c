/***************************************************************************

	vidhrdw.c

	Functions to emulate the video hardware of the machine.

	There are only a few differences between the video hardware of Mysterious
	Stones and Mat Mania. The tile bank select bit is different and the sprite
	selection seems to be different as well. Additionally, the palette is stored
	differently. I'm also not sure that the 2nd tile page is really used in
	Mysterious Stones.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *matmania_videoram2,*matmania_colorram2;
size_t matmania_videoram2_size;
unsigned char *matmania_videoram3,*matmania_colorram3;
size_t matmania_videoram3_size;
unsigned char *matmania_scroll;
static struct mame_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;

unsigned char *matmania_pageselect;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Mat Mania is unusual in that it has both PROMs and RAM to control the
  palette. PROMs are used for characters and background tiles, RAM for
  sprites.
  I don't know for sure how the PROMs are connected to the RGB output,
  but it's probably the usual:

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
PALETTE_INIT( matmania )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 64;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[64] >> 0) & 0x01;
		bit1 = (color_prom[64] >> 1) & 0x01;
		bit2 = (color_prom[64] >> 2) & 0x01;
		bit3 = (color_prom[64] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}
}



WRITE_HANDLER( matmania_paletteram_w )
{
	int bit0,bit1,bit2,bit3,val;
	int r,g,b;
	int offs2;


	paletteram[offset] = data;
	offs2 = offset & 0x0f;

	val = paletteram[offs2];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	val = paletteram[offs2 | 0x10];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	val = paletteram[offs2 | 0x20];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	palette_set_color(offs2 + 64,r,g,b);
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( matmania )
{
	if ((dirtybuffer = auto_malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((dirtybuffer2 = auto_malloc(matmania_videoram3_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,matmania_videoram3_size);

	/* Mat Mania has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap = auto_bitmap_alloc(Machine->drv->screen_width,2* Machine->drv->screen_height)) == 0)
		return 1;

	/* Mat Mania has a virtual screen twice as large as the visible screen */
	if ((tmpbitmap2 = auto_bitmap_alloc(Machine->drv->screen_width,2 * Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}



WRITE_HANDLER( matmania_videoram3_w )
{
	if (matmania_videoram3[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		matmania_videoram3[offset] = data;
	}
}



WRITE_HANDLER( matmania_colorram3_w )
{
	if (matmania_colorram3[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		matmania_colorram3[offset] = data;
	}
}


VIDEO_UPDATE( matmania )
{
	int offs;


	/* Update the tiles in the left tile ram bank */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 15 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs] + ((colorram[offs] & 0x08) << 5),
					(colorram[offs] & 0x30) >> 4,
					0,sy >= 16,	/* flip horizontally tiles on the right half of the bitmap */
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* Update the tiles in the right tile ram bank */
	for (offs = matmania_videoram3_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = 15 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					matmania_videoram3[offs] + ((matmania_colorram3[offs] & 0x08) << 5),
					(matmania_colorram3[offs] & 0x30) >> 4,
					0,sy >= 16,	/* flip horizontally tiles on the right half of the bitmap */
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrolly;


		scrolly = -*matmania_scroll;
		if (matmania_pageselect[0]&0x01) /* maniach sets 0x20 sometimes, which must have a different meaning */
			copyscrollbitmap(bitmap,tmpbitmap2,0,0,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
		else
			copyscrollbitmap(bitmap,tmpbitmap,0,0,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs+1] + ((spriteram[offs] & 0xf0) << 4),
					(spriteram[offs] & 0x08) >> 3,
					spriteram[offs] & 0x04,spriteram[offs] & 0x02,
					239 - spriteram[offs+3],(240 - spriteram[offs+2]) & 0xff,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = matmania_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = 31 - offs / 32;
		sy = offs % 32;

		drawgfx(bitmap,Machine->gfx[0],
				matmania_videoram2[offs] + 256 * (matmania_colorram2[offs] & 0x07),
				(matmania_colorram2[offs] & 0x30) >> 4,
				0,0,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( maniach )
{
	int offs;


	/* Update the tiles in the left tile ram bank */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 15 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[1],
					videoram[offs] + ((colorram[offs] & 0x03) << 8),
					(colorram[offs] & 0x30) >> 4,
					0,sy >= 16,	/* flip horizontally tiles on the right half of the bitmap */
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* Update the tiles in the right tile ram bank */
	for (offs = matmania_videoram3_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = 15 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					matmania_videoram3[offs] + ((matmania_colorram3[offs] & 0x03) << 8),
					(matmania_colorram3[offs] & 0x30) >> 4,
					0,sy >= 16,	/* flip horizontally tiles on the right half of the bitmap */
					16*sx,16*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrolly;


		scrolly = -*matmania_scroll;
		if (matmania_pageselect[0]&0x01) /* this sets 0x20 sometimes, which must have a different meaning */
			copyscrollbitmap(bitmap,tmpbitmap2,0,0,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
		else
			copyscrollbitmap(bitmap,tmpbitmap,0,0,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs+1] + ((spriteram[offs] & 0xf0) << 4),
					(spriteram[offs] & 0x08) >> 3,
					spriteram[offs] & 0x04,spriteram[offs] & 0x02,
					239 - spriteram[offs+3],(240 - spriteram[offs+2]) & 0xff,
					&Machine->visible_area,TRANSPARENCY_PEN,0);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = matmania_videoram2_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = 31 - offs / 32;
		sy = offs % 32;

		drawgfx(bitmap,Machine->gfx[0],
				matmania_videoram2[offs] + 256 * (matmania_colorram2[offs] & 0x07),
				(matmania_colorram2[offs] & 0x30) >> 4,
				0,0,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
