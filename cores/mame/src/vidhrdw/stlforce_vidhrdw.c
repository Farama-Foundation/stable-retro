/* vidhrdw/stlforce.c - see main driver for other notes */

#include "driver.h"

static struct tilemap *stlforce_bg_tilemap, *stlforce_mlow_tilemap, *stlforce_mhigh_tilemap, *stlforce_tx_tilemap;

extern data16_t *stlforce_bg_videoram, *stlforce_mlow_videoram, *stlforce_mhigh_videoram, *stlforce_tx_videoram;
extern data16_t *stlforce_bg_scrollram, *stlforce_mlow_scrollram, *stlforce_mhigh_scrollram, *stlforce_vidattrram;

extern data16_t *stlforce_spriteram;

/* background, appears to be the bottom layer */

static void get_stlforce_bg_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_bg_videoram[tile_index] & 0x0fff;
	colour = stlforce_bg_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_bg_videoram_w )
{
	if (stlforce_bg_videoram[offset] != data)
	{
		stlforce_bg_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_bg_tilemap,offset);
	}
}

/* middle layer, low */

static void get_stlforce_mlow_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_mlow_videoram[tile_index] & 0x0fff;
	colour = stlforce_mlow_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	colour += 8;
	tileno += 0x1000;

	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_mlow_videoram_w )
{
	if (stlforce_mlow_videoram[offset] != data)
	{
		stlforce_mlow_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_mlow_tilemap,offset);
	}
}

/* middle layer, high */

static void get_stlforce_mhigh_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_mhigh_videoram[tile_index] & 0x0fff;
	colour = stlforce_mhigh_videoram[tile_index] & 0xe000;
	colour = colour >> 13;
	colour += 16;
	tileno += 0x2000;

	SET_TILE_INFO(0,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_mhigh_videoram_w )
{
	if (stlforce_mhigh_videoram[offset] != data)
	{
		stlforce_mhigh_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_mhigh_tilemap,offset);
	}
}

/* text layer, appears to be the top layer */

static void get_stlforce_tx_tile_info(int tile_index)
{
	int tileno,colour;

	tileno = stlforce_tx_videoram[tile_index] & 0x0fff;
	colour = stlforce_tx_videoram[tile_index] & 0xe000;
	colour = colour >> 13;

	tileno += 0xc000;

	colour += 24;
	SET_TILE_INFO(1,tileno,colour,0)
}

WRITE16_HANDLER( stlforce_tx_videoram_w )
{
	if (stlforce_tx_videoram[offset] != data)
	{
		stlforce_tx_videoram[offset] = data;
		tilemap_mark_tile_dirty(stlforce_tx_tilemap,offset);
	}
}

/* sprites - quite a bit still needs doing .. */

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{

	const UINT16 *source = stlforce_spriteram+0x0;
	const UINT16 *finish = stlforce_spriteram+0x800;
	const struct GfxElement *gfx = Machine->gfx[2];

	while( source<finish )
	{
		int ypos = source[0]& 0x01ff;
		int attr = source[1]& 0x000f;
		int xpos = source[3]& 0x03ff;
		int num = (source[2] & 0x1fff);

		ypos = 512-ypos;

		drawgfx(
				bitmap,
				gfx,
				num,
				64+attr,
				0,0,
				xpos,ypos,
				cliprect,
				TRANSPARENCY_PEN,0
				);
		source += 0x4;
	}
}

VIDEO_UPDATE( stlforce )
{

	tilemap_set_scrollx( stlforce_bg_tilemap, 0, stlforce_bg_scrollram[0]+8);
	tilemap_set_scrolly( stlforce_bg_tilemap, 0, stlforce_vidattrram[1] );

	tilemap_set_scrollx( stlforce_mlow_tilemap, 0, stlforce_mlow_scrollram[0]+8 );
	tilemap_set_scrolly( stlforce_mlow_tilemap, 0, stlforce_vidattrram[2] );

	tilemap_set_scrollx( stlforce_mhigh_tilemap, 0, stlforce_mhigh_scrollram[0]+8);
	tilemap_set_scrolly( stlforce_mhigh_tilemap, 0, stlforce_vidattrram[3] );


	tilemap_draw(bitmap,cliprect,stlforce_bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,stlforce_mlow_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,stlforce_mhigh_tilemap,0,0);
	draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap,cliprect,stlforce_tx_tilemap,0,0);

/*usrintf_showmessage	("Regs %04x, %04x, %04x, %04x, %04", stlforce_vidattrram[0tlforce_tx_scrollram], stlforce_vidattrram[4],stlforce_vidattrram[5],stlforce_vidattrram[6],stlforce_vidattrram[7] );*/
}

VIDEO_START( stlforce )
{
	stlforce_bg_tilemap = tilemap_create(get_stlforce_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE, 16, 16,64,16);

	stlforce_mlow_tilemap = tilemap_create(get_stlforce_mlow_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT, 16, 16,64,16);
	tilemap_set_transparent_pen(stlforce_mlow_tilemap,0);

	stlforce_mhigh_tilemap = tilemap_create(get_stlforce_mhigh_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT, 16, 16,64,16);
	tilemap_set_transparent_pen(stlforce_mhigh_tilemap,0);

	stlforce_tx_tilemap = tilemap_create(get_stlforce_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	tilemap_set_transparent_pen(stlforce_tx_tilemap,0);

	return 0;
}
