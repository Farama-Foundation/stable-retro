/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *commando_fgvideoram,*commando_bgvideoram;

static struct tilemap *fg_tilemap, *bg_tilemap;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = commando_fgvideoram[tile_index];
	color = commando_fgvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			0,
			code + ((color & 0xc0) << 2),
			color & 0x0f,
			TILE_FLIPYX((color & 0x30) >> 4))
}

static void get_bg_tile_info(int tile_index)
{
	int code, color;

	code = commando_bgvideoram[tile_index];
	color = commando_bgvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			1,
			code + ((color & 0xc0) << 2),
			color & 0x0f,
			TILE_FLIPYX((color & 0x30) >> 4))
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( commando )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     16,16,32,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,3);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( commando_fgvideoram_w )
{
	commando_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( commando_bgvideoram_w )
{
	commando_bgvideoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
}


WRITE_HANDLER( commando_scrollx_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrollx(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
}

WRITE_HANDLER( commando_scrolly_w )
{
	static unsigned char scroll[2];

	scroll[offset] = data;
	tilemap_set_scrolly(bg_tilemap,0,scroll[0] | (scroll[1] << 8));
}


WRITE_HANDLER( commando_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 4 resets the sound CPU */
	cpu_set_reset_line(1,(data & 0x10) ? ASSERT_LINE : CLEAR_LINE);

	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy,bank,attr;


		/* bit 1 of attr is not used */
		attr = buffered_spriteram[offs + 1];
		sx = buffered_spriteram[offs + 3] - ((attr & 0x01) << 8);
		sy = buffered_spriteram[offs + 2];
		flipx = attr & 0x04;
		flipy = attr & 0x08;
		bank = (attr & 0xc0) >> 6;

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[2],
					buffered_spriteram[offs] + 256 * bank,
					(attr & 0x30) >> 4,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,15);
	}
}

VIDEO_UPDATE( commando )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

VIDEO_EOF( commando )
{
	buffer_spriteram_w(0,0);
}
