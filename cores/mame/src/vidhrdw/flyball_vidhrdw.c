/***************************************************************************

Atari Flyball video emulation

***************************************************************************/

#include "driver.h"

static struct tilemap* flyball_tilemap;

UINT8 flyball_pitcher_vert;
UINT8 flyball_pitcher_horz;
UINT8 flyball_pitcher_pic;
UINT8 flyball_ball_vert;
UINT8 flyball_ball_horz;

UINT8* flyball_playfield_ram;


static UINT32 flyball_get_memory_offset(UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows)
{
	if (col == 0)
	{
		col = num_cols;
	}

	return num_cols * (num_rows - row) - col;
}


static void flyball_get_tile_info(int tile_index)
{
	UINT8 data = flyball_playfield_ram[tile_index];

	int flags =
		((data & 0x40) ? TILE_FLIPX : 0) |
		((data & 0x80) ? TILE_FLIPY : 0);

	int code = data & 63;

	if ((flags & TILE_FLIPX) && (flags & TILE_FLIPY))
	{
		code += 64;
	}

	SET_TILE_INFO(0, code, 0, flags)
}


VIDEO_START( flyball )
{
	flyball_tilemap = tilemap_create(flyball_get_tile_info,
		flyball_get_memory_offset, TILEMAP_OPAQUE, 8, 16, 32, 16);

	return 0;
}


VIDEO_UPDATE( flyball )
{
	int pitcherx = flyball_pitcher_horz;
	int pitchery = flyball_pitcher_vert - 31;

	int ballx = flyball_ball_horz - 1;
	int bally = flyball_ball_vert - 17;

	int x;
	int y;

	tilemap_mark_all_tiles_dirty(flyball_tilemap);

	/* draw playfield */

	tilemap_draw(bitmap, cliprect, flyball_tilemap, 0, 0);

	/* draw pitcher */

	drawgfx(bitmap, Machine->gfx[1], flyball_pitcher_pic ^ 0xf,
		0, 1, 0, pitcherx, pitchery, &Machine->visible_area, TRANSPARENCY_PEN, 1);

	/* draw ball */

	for (y = bally; y < bally + 2; y++)
	{
		for (x = ballx; x < ballx + 2; x++)
		{
			if (x >= Machine->visible_area.min_x &&
			    x <= Machine->visible_area.max_x &&
			    y >= Machine->visible_area.min_y &&
			    y <= Machine->visible_area.max_y)
			{
				plot_pixel(bitmap, x, y, Machine->pens[1]);
			}
		}
	}
}
