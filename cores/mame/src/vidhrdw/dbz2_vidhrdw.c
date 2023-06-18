/*
  Dragonball Z
  (c) 1993 Banpresto
  Dragonball Z 2 Super Battle
  (c) 1994 Banpresto

  Video hardware emulation.
*/


#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"

static struct tilemap *dbz2_bg_tilemap, *dbz2_bg2_tilemap;
extern data16_t *dbz2_bg_videoram, *dbz2_bg2_videoram;

static int scrolld[2][4][2] = {
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}},
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};
/*
static int scrolld1[2][4][2] = {
 	{{ 0, 0 }, {2, 0}, {4, 0}, {5, 0}},	// default '157 offsets
 	{{ 0, 0 }, {0, 0}, {0, 0}, {0, 0}}
};
*/
static int layer_colorbase;
static int sprite_colorbase;

static void dbz2_tile_callback(int layer, int *code, int *color)
{
	tile_info.flags = TILE_FLIPYX((*color) & 3);

	if (layer == 3)
		*color = layer_colorbase | (((*color & 0xf0) + 0xe00 ) >> 4);
	else
		*color = layer_colorbase | (((*color & 0xf0) + 0xc00 ) >> 4);
}

static void dbz2_sprite_callback(int *code, int *color, int *priority_mask)
{
	*priority_mask = 0;
	*color = sprite_colorbase | (*color & 0x001f);
}

/* Background Tilemaps */

WRITE16_HANDLER( dbz2_bg2_videoram_w )
{
	COMBINE_DATA(&dbz2_bg2_videoram[offset]);
	tilemap_mark_tile_dirty(dbz2_bg2_tilemap,offset/2);
}

static void get_dbz2_bg2_tile_info(int tile_index)
{
	int tileno, colour, flipx;

	tileno = dbz2_bg2_videoram[tile_index*2+1] & 0x7fff;

/*	tileno = (tileno & 0x3f1f) | (tileno & 0x40) | ((tileno & 0x80) >> 2) | ((tileno & 0x20) << 2) | ((tileno & 0x4000) << 1); // not right? some bg's are still bad*/
	colour = (dbz2_bg2_videoram[tile_index*2] & 0x000f);  /* more bits?*/

	flipx = (dbz2_bg2_videoram[tile_index*2] & 0x0080) >> 7;

	SET_TILE_INFO(0,tileno,colour,TILE_FLIPYX(flipx))
}


WRITE16_HANDLER( dbz2_bg_videoram_w )
{
	COMBINE_DATA(&dbz2_bg_videoram[offset]);
	tilemap_mark_tile_dirty(dbz2_bg_tilemap,offset/2);
}


static void get_dbz2_bg_tile_info(int tile_index)
{
	int tileno, colour, flipx;

	tileno = dbz2_bg_videoram[tile_index*2+1] & 0x7fff;

/*	tileno = (tileno & 0x3f1f) | (tileno & 0x40) | ((tileno & 0x80) >> 2) | ((tileno & 0x20) << 2) | ((tileno & 0x4000) << 1); // not right? some bg's are still bad*/
	colour = (dbz2_bg_videoram[tile_index*2] & 0x000f);  /* more bits?*/

	flipx = (dbz2_bg_videoram[tile_index*2] & 0x0080) >> 7;

	SET_TILE_INFO(1,tileno,colour,TILE_FLIPYX(flipx))
}

VIDEO_START(dbz2)
{
	K053251_vh_start();

	if (K054157_vh_start(REGION_GFX1, 0, scrolld, NORMAL_PLANE_ORDER, dbz2_tile_callback))
	{
		return 1;
	}

	if (K053247_vh_start(REGION_GFX2, -87, 32, NORMAL_PLANE_ORDER, dbz2_sprite_callback))
	{
		return 1;
	}

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -46, -16);

	K053936_wraparound_enable(1, 1);
	K053936_set_offset(1, -46, -16);

	dbz2_bg_tilemap = tilemap_create(get_dbz2_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,64,32);
	if (!dbz2_bg_tilemap) return 1;

	tilemap_set_transparent_pen(dbz2_bg_tilemap,0);

	dbz2_bg2_tilemap = tilemap_create(get_dbz2_bg2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 16, 16,64,32);
	if (!dbz2_bg2_tilemap) return 1;

	return 0;
}

VIDEO_START(dbz)
{
	K053251_vh_start();

	if (K054157_vh_start(REGION_GFX1, 0, scrolld, NORMAL_PLANE_ORDER, dbz2_tile_callback))
	{
		return 1;
	}

	if (K053247_vh_start(REGION_GFX2, -52, 16, NORMAL_PLANE_ORDER, dbz2_sprite_callback))
	{
		return 1;
	}

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -46+34, -16);

	K053936_wraparound_enable(1, 1);
	K053936_set_offset(1, -46+34, -16);

	dbz2_bg_tilemap = tilemap_create(get_dbz2_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 16, 16,64,32);
	if (!dbz2_bg_tilemap) return 1;

	tilemap_set_transparent_pen(dbz2_bg_tilemap,0);

	dbz2_bg2_tilemap = tilemap_create(get_dbz2_bg2_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 16, 16,64,32);
	if (!dbz2_bg2_tilemap) return 1;

	return 0;
}

VIDEO_UPDATE(dbz2)
{
	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);

	K053936_0_zoom_draw(bitmap,cliprect,dbz2_bg2_tilemap,0,0);
	K053936_1_zoom_draw(bitmap,cliprect,dbz2_bg_tilemap,0,0);

	K054157_tilemap_update();

	K053247_sprites_draw(bitmap, cliprect);

	K054157_tilemap_draw(bitmap, cliprect, 3, 0, 1<<0);
}

VIDEO_UPDATE(dbz)
{
	fillbitmap(priority_bitmap, 0, NULL);
	fillbitmap(bitmap, get_black_pen(), &Machine->visible_area);


	K054157_tilemap_update();

	K054157_tilemap_draw(bitmap, cliprect, 3, 0, 1<<3);
	K053936_0_zoom_draw(bitmap,cliprect,dbz2_bg2_tilemap,0,1<<2);
	K053936_1_zoom_draw(bitmap,cliprect,dbz2_bg_tilemap,0,1<<1);
	K054157_tilemap_draw(bitmap, cliprect, 0, 0, 1<<0);

	K053247_sprites_draw(bitmap, cliprect);
}
