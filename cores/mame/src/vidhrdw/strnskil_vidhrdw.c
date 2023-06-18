/******************************************************************************

Strength & Skill (c) 1984 Sun Electronics

Video hardware driver by Uki

	19/Jun/2001 -

******************************************************************************/

#include "vidhrdw/generic.h"

static UINT8 strnskil_scrl_ctrl;
static UINT8 strnskil_xscroll[2];

static struct tilemap *bg_tilemap;

PALETTE_INIT( strnskil )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int r = color_prom[0]*0x11;
		int g = color_prom[Machine->drv->total_colors]*0x11;
		int b = color_prom[2*Machine->drv->total_colors]*0x11;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;

	/* color_prom now points to the beginning of the lookup table */

	/* sprites lookup table */
	for (i=0; i<512; i++)
		*(colortable++) = *(color_prom++);

	/* bg lookup table */
	for (i=0; i<512; i++)
		*(colortable++) = *(color_prom++);

}

WRITE_HANDLER( strnskil_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE_HANDLER( strnskil_scroll_x_w )
{
	strnskil_xscroll[offset] = data;
}

WRITE_HANDLER( strnskil_scrl_ctrl_w )
{
	strnskil_scrl_ctrl = data >> 5;

	if (flip_screen != (data & 0x08))
	{
		flip_screen_set(data & 0x08);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = videoram[tile_index * 2];
	int code = videoram[(tile_index * 2) + 1] + ((attr & 0x60) << 3);
	int color = (attr & 0x1f) | ((attr & 0x80) >> 2);

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( strnskil )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	tilemap_set_scroll_rows(bg_tilemap, 32);

	return 0;
}

static void strnskil_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 0x60; offs < 0x100; offs += 4)
	{
		int code = spriteram[offs + 1];
		int color = spriteram[offs + 2] & 0x3f;
		int flipx = flip_screen_x;
		int flipy = flip_screen_y;

		int sx = spriteram[offs + 3];
		int sy = spriteram[offs];
		int px, py;

		if (flip_screen)
		{
			px = 240 - sx + 0; /* +2 or +0 ? */
			py = sy;
		}
		else
		{
			px = sx - 2;
			py = 240 - sy;
		}

		sx = sx & 0xff;

		if (sx > 248)
			sx = sx - 256;

		drawgfx(bitmap, Machine->gfx[1],
			code, color,
			flipx, flipy,
			px, py,
			&Machine->visible_area,
			TRANSPARENCY_COLOR, 0);
	}
}

VIDEO_UPDATE( strnskil )
{
	int row;

	for (row = 0; row < 32; row++)
	{
		if (strnskil_scrl_ctrl != 0x07)
		{
			switch (memory_region(REGION_USER1)[strnskil_scrl_ctrl * 32 + row])
			{
			case 2:
				tilemap_set_scrollx(bg_tilemap, row, -~strnskil_xscroll[1]);
				break;
			case 4:
				tilemap_set_scrollx(bg_tilemap, row, -~strnskil_xscroll[0]);
				break;
			}
		}
	}

	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	strnskil_draw_sprites(bitmap);
}
