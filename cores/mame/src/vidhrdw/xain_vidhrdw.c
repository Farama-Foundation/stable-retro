/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *xain_charram, *xain_bgram0, *xain_bgram1;

static struct tilemap *char_tilemap, *bgram0_tilemap, *bgram1_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 back_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x0f) + ((row & 0x0f) << 4) + ((col & 0x10) << 4) + ((row & 0x10) << 5);
}

static void get_bgram0_tile_info(int tile_index)
{
	int attr = xain_bgram0[tile_index | 0x400];
	SET_TILE_INFO(
			2,
			xain_bgram0[tile_index] | ((attr & 7) << 8),
			(attr & 0x70) >> 4,
			(attr & 0x80) ? TILE_FLIPX : 0)
}

static void get_bgram1_tile_info(int tile_index)
{
	int attr = xain_bgram1[tile_index | 0x400];
	SET_TILE_INFO(
			1,
			xain_bgram1[tile_index] | ((attr & 7) << 8),
			(attr & 0x70) >> 4,
			(attr & 0x80) ? TILE_FLIPX : 0)
}

static void get_char_tile_info(int tile_index)
{
	int attr = xain_charram[tile_index | 0x400];
	SET_TILE_INFO(
			0,
			xain_charram[tile_index] | ((attr & 3) << 8),
			(attr & 0xe0) >> 5,
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( xain )
{
	bgram0_tilemap = tilemap_create(get_bgram0_tile_info,back_scan,    TILEMAP_OPAQUE,     16,16,32,32);
	bgram1_tilemap = tilemap_create(get_bgram1_tile_info,back_scan,    TILEMAP_TRANSPARENT,16,16,32,32);
	char_tilemap = tilemap_create(get_char_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);

	if (!bgram0_tilemap || !bgram1_tilemap || !char_tilemap)
		return 1;

	tilemap_set_transparent_pen(bgram1_tilemap,0);
	tilemap_set_transparent_pen(char_tilemap,0);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( xain_bgram0_w )
{
	if (xain_bgram0[offset] != data)
	{
		xain_bgram0[offset] = data;
		tilemap_mark_tile_dirty(bgram0_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( xain_bgram1_w )
{
	if (xain_bgram1[offset] != data)
	{
		xain_bgram1[offset] = data;
		tilemap_mark_tile_dirty(bgram1_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( xain_charram_w )
{
	if (xain_charram[offset] != data)
	{
		xain_charram[offset] = data;
		tilemap_mark_tile_dirty(char_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( xain_scrollxP0_w )
{
	static unsigned char xain_scrollxP0[2];

	xain_scrollxP0[offset] = data;
	tilemap_set_scrollx(bgram0_tilemap, 0, xain_scrollxP0[0]|(xain_scrollxP0[1]<<8));
}

WRITE_HANDLER( xain_scrollyP0_w )
{
	static unsigned char xain_scrollyP0[2];

	xain_scrollyP0[offset] = data;
	tilemap_set_scrolly(bgram0_tilemap, 0, xain_scrollyP0[0]|(xain_scrollyP0[1]<<8));
}

WRITE_HANDLER( xain_scrollxP1_w )
{
	static unsigned char xain_scrollxP1[2];

	xain_scrollxP1[offset] = data;
	tilemap_set_scrollx(bgram1_tilemap, 0, xain_scrollxP1[0]|(xain_scrollxP1[1]<<8));
}

WRITE_HANDLER( xain_scrollyP1_w )
{
	static unsigned char xain_scrollyP1[2];

	xain_scrollyP1[offset] = data;
	tilemap_set_scrolly(bgram1_tilemap, 0, xain_scrollyP1[0]|(xain_scrollyP1[1]<<8));
}


WRITE_HANDLER( xain_flipscreen_w )
{
	flip_screen_set(data & 1);
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0; offs < spriteram_size;offs += 4)
	{
		int sx,sy,flipx;
		int attr = spriteram[offs+1];
		int numtile = spriteram[offs+2] | ((attr & 7) << 8);
		int color = (attr & 0x38) >> 3;

		sx = 239 - spriteram[offs+3];
		if (sx <= -7) sx += 256;
		sy = 240 - spriteram[offs];
		if (sy <= -7) sy += 256;
		flipx = attr & 0x40;
		if (flip_screen)
		{
			sx = 239 - sx;
			sy = 240 - sy;
			flipx = !flipx;
		}

		if (attr & 0x80)	/* double height */
		{
			drawgfx(bitmap,Machine->gfx[3],
					numtile,
					color,
					flipx,flip_screen,
					sx-1,flip_screen?sy+16:sy-16,
					cliprect,TRANSPARENCY_PEN,0);
			drawgfx(bitmap,Machine->gfx[3],
					numtile+1,
					color,
					flipx,flip_screen,
					sx-1,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
		else
		{
			drawgfx(bitmap,Machine->gfx[3],
					numtile,
					color,
					flipx,flip_screen,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
}

VIDEO_UPDATE( xain )
{
	tilemap_draw(bitmap,cliprect,bgram0_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,bgram1_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,char_tilemap,0,0);
}
