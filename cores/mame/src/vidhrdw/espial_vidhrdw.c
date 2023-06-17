/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


data8_t *espial_videoram;
data8_t *espial_colorram;
data8_t *espial_attributeram;
data8_t *espial_scrollram;
data8_t *espial_spriteram_1;
data8_t *espial_spriteram_2;
data8_t *espial_spriteram_3;

static int flipscreen;
static struct tilemap *tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Espial has two 256x4 palette PROMs.

  I don't know for sure how the palette PROMs are connected to the RGB
  output, but it's probably the usual:

  bit 3 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
  bit 0 -- 470 ohm resistor  -- GREEN
  bit 3 -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
PALETTE_INIT( espial )
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 0) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 1) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i + Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[i + Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	data8_t code = espial_videoram[tile_index];
	data8_t col = espial_colorram[tile_index];
	data8_t attr = espial_attributeram[tile_index];
	SET_TILE_INFO(0,
				  code | ((attr & 0x03) << 8),
				  col & 0x3f,
				  TILE_FLIPYX(attr >> 2))
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( espial )
{
	tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	if (!tilemap)
		return 1;

	tilemap_set_scroll_cols(tilemap, 32);

	return 0;
}

VIDEO_START( netwars )
{
	/* Net Wars has a tile map that's twice as big as Espial's */
	tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,64);

	if (!tilemap)
		return 1;

	tilemap_set_scroll_cols(tilemap, 32);
	tilemap_set_scrolldy(tilemap, 0, 0x100);

	return 0;
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

WRITE_HANDLER( espial_videoram_w )
{
	if (espial_videoram[offset] != data)
	{
		espial_videoram[offset] = data;
		tilemap_mark_tile_dirty(tilemap, offset);
	}
}


WRITE_HANDLER( espial_colorram_w )
{
	if (espial_colorram[offset] != data)
	{
		espial_colorram[offset] = data;
		tilemap_mark_tile_dirty(tilemap, offset);
	}
}


WRITE_HANDLER( espial_attributeram_w )
{
	if (espial_attributeram[offset] != data)
	{
		espial_attributeram[offset] = data;
		tilemap_mark_tile_dirty(tilemap, offset);
	}
}


WRITE_HANDLER( espial_scrollram_w )
{
	espial_scrollram[offset] = data;
	tilemap_set_scrolly(tilemap, offset, data);
}


WRITE_HANDLER( espial_flipscreen_w )
{
	flipscreen = data;

	tilemap_set_flip(0, flipscreen ? TILEMAP_FLIPX | TILEMAP_FLIPY : 0);
}


/*************************************
 *
 *	Video update
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;


	/* Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs < 16;offs++)
	{
		int sx,sy,code,color,flipx,flipy;


		sx = espial_spriteram_1[offs + 16];
		sy = espial_spriteram_2[offs];
		code = espial_spriteram_1[offs] >> 1;
		color = espial_spriteram_2[offs + 16];
		flipx = espial_spriteram_3[offs] & 0x04;
		flipy = espial_spriteram_3[offs] & 0x08;

		if (flipscreen)
		{
			flipx = !flipx;
			flipy = !flipy;
		}
		else
		{
			sy = 240 - sy;
		}

		if (espial_spriteram_1[offs] & 1)	/* double height */
		{
			if (flipscreen)
			{
				drawgfx(bitmap,Machine->gfx[1],
						code,color,
						flipx,flipy,
						sx,sy + 16,
						cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],
						code + 1,
						color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0);
			}
			else
			{
				drawgfx(bitmap,Machine->gfx[1],
						code,color,
						flipx,flipy,
						sx,sy - 16,
						cliprect,TRANSPARENCY_PEN,0);
				drawgfx(bitmap,Machine->gfx[1],
						code + 1,color,
						flipx,flipy,
						sx,sy,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[1],
					code,color,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
}


VIDEO_UPDATE( espial )
{
	tilemap_draw(bitmap,cliprect,tilemap,0,0);

	draw_sprites(bitmap, cliprect);
}
