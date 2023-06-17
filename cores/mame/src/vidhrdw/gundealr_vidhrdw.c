/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"



unsigned char *gundealr_bg_videoram,*gundealr_fg_videoram;

static struct tilemap *bg_tilemap,*fg_tilemap;
static int flipscreen;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	unsigned char attr = gundealr_bg_videoram[2*tile_index+1];
	SET_TILE_INFO(
			0,
			gundealr_bg_videoram[2*tile_index] + ((attr & 0x07) << 8),
			(attr & 0xf0) >> 4,
			0)
}

static UINT32 gundealr_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (row & 0x0f) + ((col & 0x3f) << 4) + ((row & 0x10) << 6);
}

static void get_fg_tile_info(int tile_index)
{
	unsigned char attr = gundealr_fg_videoram[2*tile_index+1];
	SET_TILE_INFO(
			1,
			gundealr_fg_videoram[2*tile_index] + ((attr & 0x03) << 8),
			(attr & 0xf0) >> 4,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( gundealr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,      8, 8,32,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,gundealr_scan,    TILEMAP_TRANSPARENT,16,16,64,32);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,15);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( gundealr_bg_videoram_w )
{
	if (gundealr_bg_videoram[offset] != data)
	{
		gundealr_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
	}
}

WRITE_HANDLER( gundealr_fg_videoram_w )
{
	if (gundealr_fg_videoram[offset] != data)
	{
		gundealr_fg_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset/2);
	}
}

WRITE_HANDLER( gundealr_paletteram_w )
{
	int r,g,b,val;


	paletteram[offset] = data;

	val = paletteram[offset & ~1];
	r = (val >> 4) & 0x0f;
	g = (val >> 0) & 0x0f;

	val = paletteram[offset | 1];
	b = (val >> 4) & 0x0f;
	/* TODO: the bottom 4 bits are used as well, but I'm not sure about the meaning */

	r = 0x11 * r;
	g = 0x11 * g;
	b = 0x11 * b;

	palette_set_color(offset / 2,r,g,b);
}

WRITE_HANDLER( gundealr_fg_scroll_w )
{
	static unsigned char scroll[4];

	scroll[offset] = data;
	tilemap_set_scrollx(fg_tilemap,0,scroll[1] | ((scroll[0] & 0x03) << 8));
	tilemap_set_scrolly(fg_tilemap,0,scroll[3] | ((scroll[2] & 0x03) << 8));
}

WRITE_HANDLER( yamyam_fg_scroll_w )
{
	static unsigned char scroll[4];

	scroll[offset] = data;
	tilemap_set_scrollx(fg_tilemap,0,scroll[0] | ((scroll[1] & 0x03) << 8));
	tilemap_set_scrolly(fg_tilemap,0,scroll[2] | ((scroll[3] & 0x03) << 8));
}

WRITE_HANDLER( gundealr_flipscreen_w )
{
	flipscreen = data;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}



/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( gundealr )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
