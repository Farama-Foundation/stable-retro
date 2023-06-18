/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int charbank;
static struct tilemap *bg_tilemap;

WRITE_HANDLER( hexa_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE_HANDLER( hexa_d008_w )
{
	unsigned char *RAM = memory_region(REGION_CPU1);
	int bankaddress;

	/* bit 0 = flipx (or y?) */
	if (flip_screen_x != (data & 0x01))
	{
		flip_screen_x_set(data & 0x01);
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	/* bit 1 = flipy (or x?) */
	if (flip_screen_y != (data & 0x02))
	{
		flip_screen_y_set(data & 0x02);
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	/* bit 2 - 3 unknown */

	/* bit 4 could be the ROM bank selector for 8000-bfff (not sure) */
	bankaddress = 0x10000 + ((data & 0x10) >> 4) * 0x4000;
	cpu_setbank(1, &RAM[bankaddress]);

	/* bit 5 = char bank */
	if (charbank != ((data & 0x20) >> 5))
	{
		charbank = (data & 0x20) >> 5;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	/* bit 6 - 7 unknown */
}

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int tile = videoram[offs + 1] + ((videoram[offs] & 0x07) << 8) + (charbank << 11);
	int color = (videoram[offs] & 0xf8) >> 3;

	SET_TILE_INFO(0, tile, color, 0)
}

VIDEO_START( hexa )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap)
		return 1;

	return 0;
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( hexa )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
}
