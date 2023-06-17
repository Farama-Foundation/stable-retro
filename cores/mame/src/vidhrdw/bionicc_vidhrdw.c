/***************************************************************************

Bionic Commando Video Hardware

This board handles tile/tile and tile/sprite priority with a PROM. Its
working is complicated and hardcoded in the driver.

The PROM is a 256x4 chip, with address inputs wired as follows:

A0 bg opaque
A1 \
A2 |  fg pen
A3 |
A4 /
A5 fg has priority over sprites (bit 5 of tile attribute)
A6 fg has not priority over bg (bits 6 & 7 of tile attribute both set)
A7 sprite opaque

The output selects the active layer, it can be:
0  bg
1  fg
2  sprite

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *bionicc_fgvideoram;
data16_t *bionicc_bgvideoram;
data16_t *bionicc_txvideoram;

static struct tilemap *tx_tilemap, *bg_tilemap, *fg_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int tile_index)
{
	int attr = bionicc_bgvideoram[2*tile_index+1];
	SET_TILE_INFO(
			1,
			(bionicc_bgvideoram[2*tile_index] & 0xff) + ((attr & 0x07) << 8),
			(attr & 0x18) >> 3,
			TILE_FLIPXY((attr & 0xc0) >> 6))
}

static void get_fg_tile_info(int tile_index)
{
	int attr = bionicc_fgvideoram[2*tile_index+1];
	int flags;

	if ((attr & 0xc0) == 0xc0)
	{
		tile_info.priority = 1;
		flags = TILE_SPLIT(0);
	}
	else
	{
		tile_info.priority = 0;
		flags = TILE_SPLIT((attr & 0x20) >> 5) | TILE_FLIPXY((attr & 0xc0) >> 6);
	}

	SET_TILE_INFO(
			2,
			(bionicc_fgvideoram[2*tile_index] & 0xff) + ((attr & 0x07) << 8),
			(attr & 0x18) >> 3,
			flags)
}

static void get_tx_tile_info(int tile_index)
{
	int attr = bionicc_txvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			0,
			(bionicc_txvideoram[tile_index] & 0xff) + ((attr & 0x00c0) << 2),
			attr & 0x3f,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( bionicc )
{
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,32,32);
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,      16,16,64,64);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,64);

	if (!fg_tilemap || !bg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap,3);
	tilemap_set_transmask(fg_tilemap,0,0xffff,0x8000); /* split type 0 is completely transparent in front half */
	tilemap_set_transmask(fg_tilemap,1,0xffc1,0x803e); /* split type 1 has pens 1-5 opaque in front half */
	tilemap_set_transparent_pen(bg_tilemap,15);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( bionicc_bgvideoram_w )
{
	int oldword = bionicc_bgvideoram[offset];
	COMBINE_DATA(&bionicc_bgvideoram[offset]);
	if (oldword != bionicc_bgvideoram[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
}

WRITE16_HANDLER( bionicc_fgvideoram_w )
{
	int oldword = bionicc_fgvideoram[offset];
	COMBINE_DATA(&bionicc_fgvideoram[offset]);
	if (oldword != bionicc_fgvideoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset/2);
}

WRITE16_HANDLER( bionicc_txvideoram_w )
{
	int oldword = bionicc_txvideoram[offset];
	COMBINE_DATA(&bionicc_txvideoram[offset]);
	if (oldword != bionicc_txvideoram[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset&0x3ff);
}

WRITE16_HANDLER( bionicc_paletteram_w )
{
  	/* The bottom bits are 'intensity' here, but level 2 of Top Secret shows that even
    when intensity is zero the colour is not reduced to pure black, (the sky should stay
    at dark blue rather than absolute black) */
	data = COMBINE_DATA(&paletteram16[offset]);
	paletteram16_RRRRGGGGBBBBIIII_word_w(offset,(data & 0xfff0) | ((data & 0x0007) << 1) | 1, 0);
}

WRITE16_HANDLER( bionicc_scroll_w )
{
	static data16_t scroll[4];

	data = COMBINE_DATA(&scroll[offset]);

	switch (offset)
	{
		case 0:
			tilemap_set_scrollx(fg_tilemap,0,data);
			break;
		case 1:
			tilemap_set_scrolly(fg_tilemap,0,data);
			break;
		case 2:
			tilemap_set_scrollx(bg_tilemap,0,data);
			break;
		case 3:
			tilemap_set_scrolly(bg_tilemap,0,data);
			break;
	}
}

WRITE16_HANDLER( bionicc_gfxctrl_w )
{
	if (ACCESSING_MSB)
	{
		flip_screen_set(data & 0x0100);

		tilemap_set_enable(bg_tilemap,data & 0x2000);	/* guess */
		tilemap_set_enable(fg_tilemap,data & 0x1000);	/* guess */

		coin_counter_w(0,data & 0x8000);
		coin_counter_w(1,data & 0x4000);
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void bionicc_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;
	const struct GfxElement *gfx = Machine->gfx[3];

	for (offs = (spriteram_size-8)/2;offs >= 0;offs -= 4)
	{
		int tile_number = buffered_spriteram16[offs] & 0x7ff;
		if( tile_number!=0x7FF ){
			int attr = buffered_spriteram16[offs+1];
			int color = (attr&0x3C)>>2;
			int flipx = attr&0x02;
			int flipy = 0;
			int sx = (INT16)buffered_spriteram16[offs+3];	/* signed */
			int sy = (INT16)buffered_spriteram16[offs+2];	/* signed */
			if(sy>512-16) sy-=512;
			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx( bitmap, gfx,
				tile_number,
				color,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,15);
		}
	}
}

VIDEO_UPDATE( bionicc )
{
	fillbitmap(bitmap,Machine->pens[0],cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,1|TILEMAP_BACK,0);	/* nothing in FRONT */
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0|TILEMAP_BACK,0);
	bionicc_draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0|TILEMAP_FRONT,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

VIDEO_EOF( bionicc )
{
	buffer_spriteram16_w(0,0,0);
}
