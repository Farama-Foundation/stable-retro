/*************************************************************************

	Atari Super Breakout hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"

UINT8 *sbrkout_horiz_ram;
UINT8 *sbrkout_vert_ram;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( sbrkout_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = (videoram[tile_index] & 0x80) ? videoram[tile_index] : 0;

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( sbrkout )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void sbrkout_draw_balls( struct mame_bitmap *bitmap )
{
	int ball;

	for (ball=2; ball>=0; ball--)
	{
		int code = ((sbrkout_vert_ram[ball * 2 + 1] & 0x80) >> 7);
		int sx = 31 * 8 - sbrkout_horiz_ram[ball * 2];
		int sy = 30 * 8 - sbrkout_vert_ram[ball * 2];

		drawgfx(bitmap, Machine->gfx[1],
			code, 0,
			0, 0,
			sx, sy,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0);
	}
}

VIDEO_UPDATE( sbrkout )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	sbrkout_draw_balls(bitmap);
}
