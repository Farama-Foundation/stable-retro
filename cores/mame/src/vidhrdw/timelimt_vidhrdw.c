#include "vidhrdw/generic.h"
#include "res_net.h"

/* globals */
data8_t *timelimt_bg_videoram;
size_t timelimt_bg_videoram_size;

/* locals */
static int scrollx, scrolly;

static struct tilemap *bg_tilemap, *fg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Limit has three 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( timelimt )
{
	int i;
	const int resistances_rg[3] = { 1000, 470, 220 };
	const int resistances_b [2] = { 470, 220 };

	double weights_r[3], weights_g[3], weights_b[2];
	compute_resistor_weights(0, 255,    -1.0,
			3,  resistances_rg, weights_r,  0,  0,
			3,  resistances_rg, weights_g,  0,  0,
			2,  resistances_b,  weights_b,  0,  0);

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */
		bit0 = BIT(color_prom[i], 0);
		bit1 = BIT(color_prom[i], 1);
		bit2 = BIT(color_prom[i], 2);
		r = combine_3_weights(weights_r, bit0, bit1, bit2);

		/* green component */
		bit0 = BIT(color_prom[i], 3);
		bit1 = BIT(color_prom[i], 4);
		bit2 = BIT(color_prom[i], 5);
		g = combine_3_weights(weights_g, bit0, bit1, bit2);

		/* blue component */
		bit0 = BIT(color_prom[i], 6);
		bit1 = BIT(color_prom[i], 7);
		b = combine_2_weights(weights_b, bit0, bit1);

		palette_set_color(i, r, g, b);
	}
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(1, timelimt_bg_videoram[tile_index], 0, 0);
}

static void get_fg_tile_info(int tile_index)
{
	SET_TILE_INFO(0, videoram[tile_index], 0, 0);
}

VIDEO_START( timelimt )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if (!bg_tilemap)
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if (!fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

/***************************************************************************/

WRITE_HANDLER( timelimt_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( timelimt_bg_videoram_w )
{
	if (timelimt_bg_videoram[offset] != data)
	{
		timelimt_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( timelimt_scroll_x_lsb_w )
{
	scrollx &= 0x100;
	scrollx |= data & 0xff;
}

WRITE_HANDLER( timelimt_scroll_x_msb_w )
{
	scrollx &= 0xff;
	scrollx |= ( data & 1 ) << 8;
}

WRITE_HANDLER( timelimt_scroll_y_w )
{
	scrolly = data;
}

/***************************************************************************

	Draw the sprites

***************************************************************************/
static void drawsprites( struct mame_bitmap *bitmap )
{
	int offs;

	for( offs = spriteram_size; offs >= 0; offs -= 4 )
	{
		int sy = 240 - spriteram[offs];
		int sx = spriteram[offs+3];
		int code = spriteram[offs+1] & 0x3f;
		int attr = spriteram[offs+2];
		int flipy = spriteram[offs+1] & 0x80;
		int flipx = spriteram[offs+1] & 0x40;

		code += ( attr & 0x80 ) ? 0x40 : 0x00;
		code += ( attr & 0x40 ) ? 0x80 : 0x00;

		drawgfx( bitmap, Machine->gfx[2],
				code,
				attr & 3, /* was & 7, wrong for 3bpp and 32 colors */
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

VIDEO_UPDATE( timelimt )
{
	tilemap_set_scrollx(bg_tilemap, 0, scrollx);
	tilemap_set_scrolly(bg_tilemap, 0, scrolly);
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);

	drawsprites( bitmap );

	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}
