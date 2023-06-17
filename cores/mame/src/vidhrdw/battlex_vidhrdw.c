#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/battlex.h"

static struct tilemap *bg_tilemap;

WRITE_HANDLER( battlex_palette_w )
{
	int palette_num = offset / 8;
	int color_num = offset & 7;

	palette_set_color(offset, pal1bit(data >> 0), pal1bit(data >> 2), pal1bit(data >> 1));
	/* set darker colors */
	palette_set_color(64+palette_num*16+color_num, pal1bit(data >> 0), pal1bit(data >> 2), pal1bit(data >> 1));
	palette_set_color(64+palette_num*16+color_num+8, pal2bit((data >> 0)&1), pal2bit((data >> 2)&1), pal2bit( (data >> 1) &1));
}

WRITE_HANDLER( battlex_scroll_x_lsb_w )
{
	battlex_scroll_lsb = data;
}

WRITE_HANDLER( battlex_scroll_x_msb_w )
{
	battlex_scroll_msb = data;
}

WRITE_HANDLER( battlex_scroll_starfield_w )
{
}

WRITE_HANDLER( battlex_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
}

WRITE_HANDLER( battlex_flipscreen_w )
{
	battlex_starfield_enabled = data & 0x10;

	if (flip_screen != (data >> 7))
	{
		flip_screen_set(data & 0x80);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int tile = videoram[tile_index*2] | (((videoram[tile_index*2+1] & 0x01)) << 8);
	int color = (videoram[tile_index*2+1] & 0x0e) >> 1;

	SET_TILE_INFO(0,tile,color,0)
}

static void get_dodgeman_bg_tile_info(int tile_index)
{
	int tile = videoram[tile_index*2] | (((videoram[tile_index*2+1] & 0x03)) << 8);
	int color = (videoram[tile_index*2+1] & 0x0c) >> 2;

	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( battlex )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

VIDEO_START( dodgeman )
{
	bg_tilemap = tilemap_create(get_dodgeman_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void battlex_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	UINT8 *source = spriteram;
	UINT8 *finish = spriteram + 0x200;

	while( source<finish )
	{
		int sx = (source[0] & 0x7f) * 2 - (source[0] & 0x80) * 2;
		int sy = source[3];
		int tile = source[2]; /* dodgeman has 0x100 sprites */
		int color = source[1] & 0x07;	/* bits 3,4,5 also used during explosions */
		int flipy = source[1] & 0x80;
		int flipx = source[1] & 0x40;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap,gfx,tile,color,flipx,flipy,sx,sy,cliprect,TRANSPARENCY_PEN,0);

		source += 4;
	}

}

VIDEO_UPDATE(battlex)
{
	if (!flip_screen)
		tilemap_set_scrollx(bg_tilemap, 0, battlex_scroll_lsb | (battlex_scroll_msb << 8));
	else
		tilemap_set_scrollx(bg_tilemap, 0, battlex_scroll_lsb | (battlex_scroll_msb << 3));
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	battlex_drawsprites(bitmap, cliprect);
}
