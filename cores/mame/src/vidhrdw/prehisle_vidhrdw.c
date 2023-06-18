/***************************************************************************

	Prehistoric Isle video routines

	Emulation by Bryan McPhail, mish@tendril.co.uk

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


UINT16 *prehisle_bg_videoram16;

static int invert_controls;

static struct tilemap *bg2_tilemap, *bg_tilemap, *fg_tilemap;


WRITE16_HANDLER( prehisle_bg_videoram16_w )
{
	UINT16 oldword = prehisle_bg_videoram16[offset];
	COMBINE_DATA(&prehisle_bg_videoram16[offset]);
	if (oldword != prehisle_bg_videoram16[offset])
	{
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE16_HANDLER( prehisle_fg_videoram16_w )
{
	UINT16 oldword = videoram16[offset];
	COMBINE_DATA(&videoram16[offset]);
	if (oldword != videoram16[offset])
	{
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

READ16_HANDLER( prehisle_control16_r )
{
	switch (offset)
	{
	case 0x08: return readinputport(1);						/* Player 2*/
	case 0x10: return readinputport(2);						/* Coins, Tilt, Service*/
	case 0x20: return readinputport(0) ^ invert_controls;	/* Player 1*/
	case 0x21: return readinputport(3);						/* DIPs*/
	case 0x22: return readinputport(4);						/* DIPs + VBLANK*/
	default: return 0;
	}
}

WRITE16_HANDLER( prehisle_control16_w )
{
	int scroll;

	COMBINE_DATA(&scroll);

	switch (offset)
	{
	case 0x00: tilemap_set_scrolly(bg_tilemap, 0, scroll); break;
	case 0x08: tilemap_set_scrollx(bg_tilemap, 0, scroll); break;
	case 0x10: tilemap_set_scrolly(bg2_tilemap, 0, scroll); break;
	case 0x18: tilemap_set_scrollx(bg2_tilemap, 0, scroll); break;
	case 0x23: invert_controls = data ? 0xff : 0x00; break;
	case 0x30: flip_screen_set(data & 0x01); break;
	}
}

static void get_bg2_tile_info(int tile_index)
{
	UINT8 *tilerom = memory_region(REGION_GFX5);

	int offs = tile_index * 2;
	int attr = tilerom[offs + 1] + (tilerom[offs] << 8);
	int code = (attr & 0x7ff) | 0x800;
	int color = attr >> 12;
	int flags = (attr & 0x800) ? TILE_FLIPX : 0;

	SET_TILE_INFO(1, code, color, flags)
}

static void get_bg_tile_info(int tile_index)
{
	int attr = prehisle_bg_videoram16[tile_index];
	int code = attr & 0x7ff;
	int color = attr >> 12;
	int flags = (attr & 0x800) ? TILE_FLIPY : 0;

	SET_TILE_INFO(2, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int attr = videoram16[tile_index];
	int code = attr & 0xfff;
	int color = attr >> 12;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( prehisle )
{
	bg2_tilemap = tilemap_create(get_bg2_tile_info, tilemap_scan_cols, 
		TILEMAP_OPAQUE, 16, 16, 1024, 32);

	if ( !bg2_tilemap )
		return 1;

	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_cols, 
		TILEMAP_TRANSPARENT, 16, 16, 256, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 15);
	tilemap_set_transparent_pen(fg_tilemap, 15);

	return 0;
}

static void prehisle_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;

	for (offs = 0; offs < 1024; offs += 4)
	{
		int attr = spriteram16[offs + 2];
		int code = attr & 0x1fff;
		int color = spriteram16[offs + 3] >> 12;
		int flipx = attr & 0x4000;
		int flipy = attr & 0x8000;
		int sx = spriteram16[offs + 1];
		int sy = spriteram16[offs];

		if (sx & 0x200) sx = -(0xff - (sx & 0xff));	/* wraparound*/

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[3], code, color, flipx, flipy, sx, sy,
			cliprect, TRANSPARENCY_PEN, 15);
	}
}

VIDEO_UPDATE( prehisle )
{
	tilemap_draw(bitmap, cliprect, bg2_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	prehisle_draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}
