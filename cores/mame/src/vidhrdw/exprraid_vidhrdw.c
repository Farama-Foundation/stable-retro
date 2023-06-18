#include "driver.h"
#include "vidhrdw/generic.h"

static int bg_index[4];

static struct tilemap *bg_tilemap, *fg_tilemap;

WRITE_HANDLER( exprraid_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( exprraid_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( exprraid_flipscreen_w )
{
	if (flip_screen != (data & 0x01))
	{
		flip_screen_set(data & 0x01);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

WRITE_HANDLER( exprraid_bgselect_w )
{
	if (bg_index[offset] != data)
	{
		bg_index[offset] = data;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( exprraid_scrollx_w )
{
	tilemap_set_scrollx(bg_tilemap, offset, data);
}

WRITE_HANDLER( exprraid_scrolly_w )
{
	tilemap_set_scrolly(bg_tilemap, 0, data);
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX4);

	int data, attr, bank, code, color, flags;
	int quadrant = 0, offs;

	int sx = tile_index % 32;
	int sy = tile_index / 32;

	if (sx >= 16) quadrant++;
	if (sy >= 16) quadrant += 2;

	offs = (sy % 16) * 16 + (sx % 16) + (bg_index[quadrant] & 0x3f) * 0x100;

	data = tilerom[offs];
	attr = tilerom[offs + 0x4000];
	bank = (2 * (attr & 0x03) + ((data & 0x80) >> 7)) + 2;
	code = data & 0x7f;
	color = (attr & 0x18) >> 3;
	flags = (attr & 0x04) ? TILE_FLIPX : 0;

	tile_info.priority = ((attr & 0x80) ? 1 : 0);

	SET_TILE_INFO(bank, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0x07) << 8);
	int color = (attr & 0x10) >> 4;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( exprraid )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 16, 16, 32, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_scroll_rows(bg_tilemap, 2);
	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void exprraid_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int attr = spriteram[offs + 1];
		int code = spriteram[offs + 3] + ((attr & 0xe0) << 3);
		int color = (attr & 0x03) + ((attr & 0x08) >> 1);
		int flipx = (attr & 0x04);
		int flipy = 0;
		int sx = ((248 - spriteram[offs + 2]) & 0xff) - 8;
		int sy = spriteram[offs];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[1],
			code, color,
			flipx, flipy,
			sx, sy,
			0, TRANSPARENCY_PEN, 0);

		/* double height */

		if (attr & 0x10)
		{
			drawgfx(bitmap,Machine->gfx[1],
				code + 1, color,
				flipx, flipy,
				sx, sy + (flip_screen ? -16 : 16),
				0, TRANSPARENCY_PEN, 0);
		}
	}
}

VIDEO_UPDATE( exprraid )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	exprraid_draw_sprites(bitmap);
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 1, 0);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
}
