/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int background_image;

static struct tilemap *fg_tilemap, *bg_tilemap;

WRITE_HANDLER( bombjack_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( bombjack_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( bombjack_background_w )
{
	if (background_image != data)
	{
		background_image = data;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

WRITE_HANDLER( bombjack_flipscreen_w )
{
	if (flip_screen != (data & 0x01))
	{
		flip_screen_set(data & 0x01);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX4);

	int offs = (background_image & 0x07) * 0x200 + tile_index;
	int code = (background_image & 0x10) ? tilerom[offs] : 0;
	int attr = tilerom[offs + 0x100];
	int color = attr & 0x0f;
	int flags = (attr & 0x80) ? TILE_FLIPY : 0;

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + 16 * (colorram[tile_index] & 0x10);
	int color = colorram[tile_index] & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( bombjack )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 16, 16, 16, 16);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}

static void bombjack_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

	for (offs = spriteram_size - 4; offs >= 0; offs -= 4)
	{

/*
 abbbbbbb cdefgggg hhhhhhhh iiiiiiii

 a        use big sprites (32x32 instead of 16x16)
 bbbbbbb  sprite code
 c        x flip
 d        y flip (used only in death sequence?)
 e        ? (set when big sprites are selected)
 f        ? (set only when the bonus (B) materializes?)
 gggg     color
 hhhhhhhh x position
 iiiiiiii y position
*/
		int sx,sy,flipx,flipy;


		sx = spriteram[offs+3];
		if (spriteram[offs] & 0x80)
			sy = 225-spriteram[offs+2];
		else
			sy = 241-spriteram[offs+2];
		flipx = spriteram[offs+1] & 0x40;
		flipy =	spriteram[offs+1] & 0x80;
		if (flip_screen)
		{
			if (spriteram[offs+1] & 0x20)
			{
				sx = 224 - sx;
				sy = 224 - sy;
			}
			else
			{
				sx = 240 - sx;
				sy = 240 - sy;
			}
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x80) ? 3 : 2],
				spriteram[offs] & 0x7f,
				spriteram[offs+1] & 0x0f,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

VIDEO_UPDATE( bombjack )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, &Machine->visible_area, fg_tilemap, 0, 0);
	bombjack_draw_sprites(bitmap);
}
