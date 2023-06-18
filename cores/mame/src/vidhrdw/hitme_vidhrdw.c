
#include "driver.h"

extern data8_t *hitme_vidram;


/* Palette Init */

PALETTE_INIT( hitme )
{
	palette_set_color(0,0x00,0x00,0x00);	/* black */
	palette_set_color(1,0xff,0xff,0xff);	/* white */
}

/* Tilemap */

static struct tilemap *hitme_tilemap;

static void get_hitme_tile_info(int tile_index)
{
	int code = hitme_vidram[tile_index] & 0x3f;
	SET_TILE_INFO(
			0,
			code,
			0,
			0)
}

/* the old vidhrdw/hitme.c had some invert thing, need to change it to work with this I guess .. */

/*

	for (offs = 0; offs < videoram_size; offs++)
	{
		charcode = videoram[offs] & 0x3F;

		if (videoram[offs] & 0x80) hitme_invert_count = 4;
		if (hitme_invert_count) {
			invert = 1;
			hitme_invert_count--;
		}
		else {
			invert = 0;
		}
		sx = 10 * (offs % 40);
		sy = 11 * (offs / 40);
		drawgfx(tmpbitmap,Machine->gfx[0],
        charcode, invert,
		0,0,sx,sy,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

*/

WRITE_HANDLER( hitme_vidram_w )
{
	hitme_vidram[offset] = data;
	tilemap_mark_tile_dirty(hitme_tilemap,offset);
}

/* Video Start / Update */

VIDEO_START (hitme)
{
	hitme_tilemap = tilemap_create(get_hitme_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,40, 19);

	return 0;
}

VIDEO_START (brickyrd)
{
	hitme_tilemap = tilemap_create(get_hitme_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32, 24);

	return 0;
}

VIDEO_UPDATE (hitme)
{
	tilemap_draw(bitmap,cliprect,hitme_tilemap,0,0);
}
