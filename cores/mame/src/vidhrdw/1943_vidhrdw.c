/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *c1943_scrollx;
unsigned char *c1943_scrolly;
unsigned char *c1943_bgscrolly;
static int chon,objon,sc1on,sc2on;
static int flipscreen;

static struct mame_bitmap *sc2bitmap;
static struct mame_bitmap *sc1bitmap;
static unsigned char sc2map[9][8][2];
static unsigned char sc1map[9][9][2];



/***************************************************************************

  Convert the color PROMs into a more useable format.

  1943 has three 256x4 palette PROMs (one per gun) and a lot ;-) of 256x4
  lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
PALETTE_INIT( 1943 )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i + 2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[i + 2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[i + 2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[i + 2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	color_prom += 3*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */

	/* characters use colors 64-79 */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 64;
	color_prom += 128;	/* skip the bottom half of the PROM - not used */

	/* foreground tiles use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		/* color 0 MUST map to pen 0 in order for transparency to work */
		if (i % Machine->gfx[1]->color_granularity == 0)
			COLOR(1,i) = 0;
		else
			COLOR(1,i) = color_prom[0] + 16 * (color_prom[256] & 0x03);
		color_prom++;
	}
	color_prom += TOTAL_COLORS(1);

	/* background tiles use colors 0-63 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = color_prom[0] + 16 * (color_prom[256] & 0x03);
		color_prom++;
	}
	color_prom += TOTAL_COLORS(2);

	/* sprites use colors 128-255 */
	/* bit 3 of BMPROM.07 selects priority over the background, but we handle */
	/* it differently for speed reasons */
	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		COLOR(3,i) = color_prom[0] + 16 * (color_prom[256] & 0x07) + 128;
		color_prom++;
	}
	color_prom += TOTAL_COLORS(3);
}



VIDEO_START( 1943 )
{
	if ((sc2bitmap = auto_bitmap_alloc(9*32,8*32)) == 0)
		return 1;

	if ((sc1bitmap = auto_bitmap_alloc(9*32,9*32)) == 0)
		return 1;

	if (video_start_generic())
		return 1;

	memset (sc2map, 0xff, sizeof (sc2map));
	memset (sc1map, 0xff, sizeof (sc1map));

	return 0;
}



WRITE_HANDLER( c1943_c804_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);


	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 1);
	coin_counter_w(1,data & 2);

	/* bits 2, 3 and 4 select the ROM bank */
	bankaddress = 0x10000 + (data & 0x1c) * 0x1000;
	cpu_setbank(1,&RAM[bankaddress]);

	/* bit 5 resets the sound CPU - we ignore it */

	/* bit 6 flips screen */
	if (flipscreen != (data & 0x40))
	{
		flipscreen = data & 0x40;
/*		memset(dirtybuffer,1,c1942_backgroundram_size);*/
	}

	/* bit 7 enables characters */
	chon = data & 0x80;
}



WRITE_HANDLER( c1943_d806_w )
{
	/* bit 4 enables bg 1 */
	sc1on = data & 0x10;

	/* bit 5 enables bg 2 */
	sc2on = data & 0x20;

	/* bit 6 enables sprites */
	objon = data & 0x40;
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( 1943 )
{
	int offs,sx,sy;
	int bg_scrolly, bg_scrollx;
	unsigned char *p;
	int top,left,xscroll,yscroll;

/* TODO: support flipscreen */
	if (sc2on)
	{
		p=memory_region(REGION_GFX5)+0x8000;
		bg_scrolly = c1943_bgscrolly[0] + 256 * c1943_bgscrolly[1];
		offs = 16 * ((bg_scrolly>>5)+8);

		top = 8 - (bg_scrolly>>5) % 9;

		bg_scrolly&=0x1f;

		for (sy = 0;sy <9;sy++)
		{
			int ty = (sy + top) % 9;
			unsigned char *map = &sc2map[ty][0][0];
			offs &= 0x7fff; /* Enforce limits (for top of scroll) */

			for (sx = 0;sx < 8;sx++)
			{
				int tile, attr, offset;
				offset=offs+2*sx;

				tile=p[offset];
				attr=p[offset+1];

				if (tile != map[0] || attr != map[1])
				{
					map[0] = tile;
					map[1] = attr;
					drawgfx(sc2bitmap,Machine->gfx[2],
							tile,
							(attr & 0x3c) >> 2,
							attr&0x40, attr&0x80,
							(8-ty)*32, sx*32,
							0,
							TRANSPARENCY_NONE,0);
				}
				map += 2;
			}
			offs-=0x10;
		}

		xscroll = (top*32-bg_scrolly);
		yscroll = 0;
		copyscrollbitmap(bitmap,sc2bitmap,
			1,&xscroll,
			1,&yscroll,
			cliprect,
			TRANSPARENCY_NONE,0);
	}
	else fillbitmap(bitmap,get_black_pen(),cliprect);


	if (objon)
	{
		/* Draw the sprites which don't have priority over the foreground. */
		for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
		{
			int color;


			color = spriteram[offs + 1] & 0x0f;
			if (color == 0x0a || color == 0x0b)	/* the priority is actually selected by */
												/* bit 3 of BMPROM.07 */
			{
				sx = spriteram[offs + 3] - ((spriteram[offs + 1] & 0x10) << 4);
				sy = spriteram[offs + 2];
				if (flipscreen)
				{
					sx = 240 - sx;
					sy = 240 - sy;
				}

				drawgfx(bitmap,Machine->gfx[3],
						spriteram[offs] + ((spriteram[offs + 1] & 0xe0) << 3),
						color,
						flipscreen,flipscreen,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
	}


/* TODO: support flipscreen */
	if (sc1on)
	{
		p=memory_region(REGION_GFX5);

		bg_scrolly = c1943_scrolly[0] + 256 * c1943_scrolly[1];
		bg_scrollx = c1943_scrollx[0];
		offs = 16 * ((bg_scrolly>>5)+8)+2*(bg_scrollx>>5) ;
		if (bg_scrollx & 0x80) offs -= 0x10;

		top = 8 - (bg_scrolly>>5) % 9;
		left = (bg_scrollx>>5) % 9;

		bg_scrolly&=0x1f;
		bg_scrollx&=0x1f;

		for (sy = 0;sy <9;sy++)
		{
			int ty = (sy + top) % 9;
			offs &= 0x7fff; /* Enforce limits (for top of scroll) */

			for (sx = 0;sx < 9;sx++)
			{
				int tile, attr, offset;
				int tx = (sx + left) % 9;
				unsigned char *map = &sc1map[ty][tx][0];
				offset=offs+(sx*2);

				tile=p[offset];
				attr=p[offset+1];

				if (tile != map[0] || attr != map[1])
				{
					map[0] = tile;
					map[1] = attr;
					tile+=256*(attr&0x01);
					drawgfx(sc1bitmap,Machine->gfx[1],
							tile,
							(attr & 0x3c) >> 2,
							attr & 0x40,attr & 0x80,
							(8-ty)*32, tx*32,
							0,
							TRANSPARENCY_NONE,0);
				}
			}
			offs-=0x10;
		}

		xscroll = (top*32-bg_scrolly);
		yscroll = -(left*32+bg_scrollx);
		copyscrollbitmap(bitmap,sc1bitmap,
			1,&xscroll,
			1,&yscroll,
			cliprect,
			TRANSPARENCY_COLOR,0);
	}


	if (objon)
	{
		/* Draw the sprites which have priority over the foreground. */
		for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
		{
			int color;


			color = spriteram[offs + 1] & 0x0f;
			if (color != 0x0a && color != 0x0b)	/* the priority is actually selected by */
												/* bit 3 of BMPROM.07 */
			{
				sx = spriteram[offs + 3] - ((spriteram[offs + 1] & 0x10) << 4);
				sy = spriteram[offs + 2];
				if (flipscreen)
				{
					sx = 240 - sx;
					sy = 240 - sy;
				}

				drawgfx(bitmap,Machine->gfx[3],
						spriteram[offs] + ((spriteram[offs + 1] & 0xe0) << 3),
						color,
						flipscreen,flipscreen,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
	}


	if (chon)
	{
		/* draw the frontmost playfield. They are characters, but draw them as sprites */
		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			sx = offs % 32;
			sy = offs / 32;
			if (flipscreen)
			{
				sx = 31 - sx;
				sy = 31 - sy;
			}

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + ((colorram[offs] & 0xe0) << 3),
					colorram[offs] & 0x1f,
					flipscreen,flipscreen,
					8*sx,8*sy,
					cliprect,TRANSPARENCY_COLOR,79);
		}
	}
}
