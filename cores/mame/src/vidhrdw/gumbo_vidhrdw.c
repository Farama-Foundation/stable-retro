/* Gumbo Vidhrdw */

#include "driver.h"

extern data16_t *gumbo_bg_videoram;
extern data16_t *gumbo_fg_videoram;

static struct tilemap *gumbo_bg_tilemap;
static struct tilemap *gumbo_fg_tilemap;

WRITE16_HANDLER( gumbo_bg_videoram_w )
{
	if (gumbo_bg_videoram[offset] != data)
	{
		COMBINE_DATA(&gumbo_bg_videoram[offset]);
		tilemap_mark_tile_dirty(gumbo_bg_tilemap,offset);
	}
}

static void get_gumbo_bg_tile_info(int tile_index)
{
	int tileno;
	tileno = gumbo_bg_videoram[tile_index];
	SET_TILE_INFO(0,tileno,0,0)
}


WRITE16_HANDLER( gumbo_fg_videoram_w )
{
	if (gumbo_fg_videoram[offset] != data)
	{
		COMBINE_DATA(&gumbo_fg_videoram[offset]);
		tilemap_mark_tile_dirty(gumbo_fg_tilemap,offset);
	}
}

static void get_gumbo_fg_tile_info(int tile_index)
{
	int tileno;
	tileno = gumbo_fg_videoram[tile_index];
	SET_TILE_INFO(1,tileno,1,0)
}


VIDEO_START( gumbo )
{
	gumbo_bg_tilemap = tilemap_create(get_gumbo_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,      8, 8, 64,32);
	gumbo_fg_tilemap = tilemap_create(get_gumbo_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 4, 4,128,64);
	tilemap_set_transparent_pen(gumbo_fg_tilemap,0xff);

	return 0;
}

VIDEO_UPDATE( gumbo )
{
	tilemap_draw(bitmap,cliprect,gumbo_bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,gumbo_fg_tilemap,0,0);
}
