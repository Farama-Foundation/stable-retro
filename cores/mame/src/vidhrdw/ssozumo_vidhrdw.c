/***************************************************************************

Syusse Oozumou
(c) 1984 Technos Japan (Licensed by Data East)

Driver by Takahiro Nogi (nogi@kt.rim.or.jp) 1999/10/04

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *ssozumo_videoram2;
UINT8 *ssozumo_colorram2;

static struct tilemap *bg_tilemap, *fg_tilemap;

#define TOTAL_COLORS(gfxn)	(Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
#define COLOR(gfxn,offs)	(colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

/**************************************************************************/

PALETTE_INIT( ssozumo )
{
	int	bit0, bit1, bit2, bit3, r, g, b;
	int	i;

	for (i = 0 ; i < 64 ; i++)
	{
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

WRITE_HANDLER( ssozumo_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( ssozumo_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( ssozumo_videoram2_w )
{
	if (ssozumo_videoram2[offset] != data)
	{
		ssozumo_videoram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( ssozumo_colorram2_w )
{
	if (ssozumo_colorram2[offset] != data)
	{
		ssozumo_colorram2[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( ssozumo_paletteram_w )
{
	int	bit0, bit1, bit2, bit3, val;
	int	r, g, b;
	int	offs2;

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

	palette_set_color(offs2 + 64, r, g, b);
}

WRITE_HANDLER( ssozumo_scroll_w )
{
	tilemap_set_scrolly(bg_tilemap, 0, data);
}

WRITE_HANDLER( ssozumo_flipscreen_w )
{
	flip_screen_set(data & 0x80);
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((colorram[tile_index] & 0x08) << 5);
	int color = (colorram[tile_index] & 0x30) >> 4;
	int flags = ((tile_index % 32) >= 16) ? TILE_FLIPY : 0;

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int code = ssozumo_videoram2[tile_index] + 256 * (ssozumo_colorram2[tile_index] & 0x07);
	int color = (ssozumo_colorram2[tile_index] & 0x30) >> 4;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( ssozumo )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols_flip_x, 
		TILEMAP_OPAQUE, 16, 16, 16, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_cols_flip_x, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void ssozumo_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			int code = spriteram[offs + 1] + ((spriteram[offs] & 0xf0) << 4);
			int color = (spriteram[offs] & 0x08) >> 3;
			int flipx = spriteram[offs] & 0x04;
			int flipy = spriteram[offs] & 0x02;
			int sx = 239 - spriteram[offs + 3];
			int sy = (240 - spriteram[offs + 2]) & 0xff;

			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap, Machine->gfx[2],
				code, color,
				flipx, flipy,
				sx, sy,
				&Machine->visible_area,
				TRANSPARENCY_PEN, 0);
		}
	}
}

VIDEO_UPDATE( ssozumo )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
	ssozumo_draw_sprites(bitmap);
}
