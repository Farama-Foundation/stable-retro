#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *gotya_scroll;
UINT8 *gotya_videoram2;

static int scroll_bit_8;

static struct tilemap *bg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  I'm using Pac Man resistor values

***************************************************************************/
PALETTE_INIT( gotya )
{
	int i;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		int bit0, bit1, bit2, r, g, b;

		/* red component */

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;

		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */

		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;

		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */

		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;

		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i, r, g, b);

		color_prom++;
	}

	color_prom += 0x18;
	/* color_prom now points to the beginning of the lookup table */

	/* character lookup table */
	/* sprites use the same color lookup table as characters */

	for (i = 0; i < TOTAL_COLORS(0); i++)
	{
		COLOR(0, i) = *(color_prom++) & 0x07;
	}
}

WRITE_HANDLER( gotya_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( gotya_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( gotya_video_control_w )
{
	/* bit 0 - scroll bit 8
	   bit 1 - flip screen
	   bit 2 - sound disable ??? */

	scroll_bit_8 = data & 0x01;

	if (flip_screen != (data & 0x02))
	{
		flip_screen_set(data & 0x02);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index];
	int color = colorram[tile_index] & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( gotya )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows_flip_xy,
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void gotya_draw_status_row( struct mame_bitmap *bitmap, int sx, int col )
{
	int row;

	if (flip_screen)
	{
		sx = 35 - sx;
	}

	for (row = 29; row >= 0; row--)
	{
		int sy;

		if (flip_screen)
		{
			sy = row;
		}
		else
		{
			sy = 31 - row;
		}

		drawgfx(bitmap,Machine->gfx[0],
			gotya_videoram2[row * 32 + col],
			gotya_videoram2[row * 32 + col + 0x10] & 0x0f,
			flip_screen_x, flip_screen_y,
			8 * sx, 8 * sy,
			&Machine->visible_area,
			TRANSPARENCY_NONE, 0);
	}
}

static void gotya_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = 2; offs < 0x0e; offs += 2)
	{
		int code = spriteram[offs + 0x01] >> 2;
		int color = spriteram[offs + 0x11] & 0x0f;
		int sx = 256 - spriteram[offs + 0x10] + (spriteram[offs + 0x01] & 0x01) * 256;
		int sy = spriteram[offs + 0x00];

		if (flip_screen)
		{
			sy = 240 - sy;
		}

		drawgfx(bitmap,Machine->gfx[1],
			code, color,
			flip_screen_x, flip_screen_y,
			sx, sy,
			&Machine->visible_area,
			TRANSPARENCY_PEN, 0);
	}
}

static void gotya_draw_status( struct mame_bitmap *bitmap )
{
	gotya_draw_status_row(bitmap, 0,  1);
	gotya_draw_status_row(bitmap, 1,  0);
	gotya_draw_status_row(bitmap, 2,  2);	/* these two are blank, but I dont' know if the data comes */
	gotya_draw_status_row(bitmap, 33, 13);	/* from RAM or 'hardcoded' into the hardware. Likely the latter */
	gotya_draw_status_row(bitmap, 35, 14);
	gotya_draw_status_row(bitmap, 34, 15);
}

VIDEO_UPDATE( gotya )
{
	tilemap_set_scrollx(bg_tilemap, 0, -(*gotya_scroll + (scroll_bit_8 * 256)) - 2 * 8);
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	gotya_draw_sprites(bitmap);
	gotya_draw_status(bitmap);
}
