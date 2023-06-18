/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *srumbler_backgroundram,*srumbler_foregroundram;
static struct tilemap *bg_tilemap,*fg_tilemap;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	unsigned char attr = srumbler_foregroundram[2*tile_index];
	SET_TILE_INFO(
			0,
			srumbler_foregroundram[2*tile_index + 1] + ((attr & 0x03) << 8),
			(attr & 0x3c) >> 2,
			(attr & 0x40) ? TILE_IGNORE_TRANSPARENCY : 0)
}

static void get_bg_tile_info(int tile_index)
{
	unsigned char attr = srumbler_backgroundram[2*tile_index];
	SET_TILE_INFO(
			1,
			srumbler_backgroundram[2*tile_index + 1] + ((attr & 0x07) << 8),
			(attr & 0xe0) >> 5,
			TILE_SPLIT((attr & 0x10) >> 4) | ((attr & 0x08) ? TILE_FLIPY : 0))
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( srumbler )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_SPLIT,    16,16,64,64);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,3);

	tilemap_set_transmask(bg_tilemap,0,0xffff,0x0000); /* split type 0 is totally transparent in front half */
	tilemap_set_transmask(bg_tilemap,1,0x07ff,0xf800); /* split type 1 has pens 0-10 transparent in front half */

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( srumbler_foreground_w )
{
	if (srumbler_foregroundram[offset] != data)
	{
		srumbler_foregroundram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset/2);
	}
}

WRITE_HANDLER( srumbler_background_w )
{
	if (srumbler_backgroundram[offset] != data)
	{
		srumbler_backgroundram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
	}
}


WRITE_HANDLER( srumbler_4009_w )
{
	/* bit 0 flips screen */
	flip_screen_set(data & 1);

	/* bits 4-5 used during attract mode, unknown */

	/* bits 6-7 coin counters */
	coin_counter_w(0,data & 0x40);
	coin_counter_w(1,data & 0x80);
}


WRITE_HANDLER( srumbler_scroll_w )
{
	static int scroll[4];

	scroll[offset] = data;

	tilemap_set_scrollx(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
	tilemap_set_scrolly(bg_tilemap,0,scroll[2] | (scroll[3] << 8));
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	/* Draw the sprites. */
	for (offs = spriteram_size-4; offs>=0;offs -= 4)
	{
		/* SPRITES
		=====
		Attribute
		0x80 Code MSB
		0x40 Code MSB
		0x20 Code MSB
		0x10 Colour
		0x08 Colour
		0x04 Colour
		0x02 y Flip
		0x01 X MSB
		*/


		int code,colour,sx,sy,flipy;
		int attr = buffered_spriteram[offs+1];
		code = buffered_spriteram[offs];
		code += ( (attr&0xe0) << 3 );
		colour = (attr & 0x1c)>>2;
		sy = buffered_spriteram[offs + 2];
		sx = buffered_spriteram[offs + 3] + 0x100 * ( attr & 0x01);
		flipy = attr & 0x02;

		if (flip_screen)
		{
			sx = 496 - sx;
			sy = 240 - sy;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[2],
				code,
				colour,
				flip_screen,flipy,
				sx, sy,
				cliprect,TRANSPARENCY_PEN,15);
	}
}


VIDEO_UPDATE( srumbler )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_BACK,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,TILEMAP_FRONT,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

VIDEO_EOF( srumbler )
{
	buffer_spriteram_w(0,0);
}
