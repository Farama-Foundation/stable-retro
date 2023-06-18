/***************************************************************************

	Atari Night Driver hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *nitedrvr_hvc;

static struct tilemap *bg_tilemap;

WRITE_HANDLER( nitedrvr_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( nitedrvr_hvc_w )
{
	nitedrvr_hvc[offset & 0x3f] = data;

/*	if ((offset & 0x30) == 0x30)*/
/*		;		// Watchdog called here */
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] & 0x3f;

	SET_TILE_INFO(0, code, 0, 0)
}

VIDEO_START( nitedrvr )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void nitedrvr_draw_block(struct mame_bitmap *bitmap, int bx, int by, int ex, int ey)
{
	int x,y;

	for (y=by; y<ey; y++)
	{
		for (x=bx; x<ex; x++)
		{
			if ((y<256) && (x<256))
				plot_pixel(bitmap, x, y, Machine->pens[1]);
		}
	}

	return;
}

static void nitedrvr_draw_road( struct mame_bitmap *bitmap )
{
	int roadway;

	for (roadway=0; roadway < 16; roadway++)
	{
		int bx, by, ex, ey;

		bx = nitedrvr_hvc[roadway];
		by = nitedrvr_hvc[roadway + 16];
		ex = bx + ((nitedrvr_hvc[roadway + 32] & 0xf0) >> 4);
		ey = by + (16 - (nitedrvr_hvc[roadway + 32] & 0x0f));

		nitedrvr_draw_block(bitmap,bx,by,ex,ey);
	}
}

static void nitedrvr_draw_hacks( struct mame_bitmap *bitmap )
{
	int offs;

	extern int nitedrvr_gear;
	extern int nitedrvr_track;

	char gear_buf[] =  {0x07,0x05,0x01,0x12,0x00,0x00}; /* "GEAR  " */
	char track_buf[] = {0x0e,0x0f,0x16,0x09,0x03,0x05, /* "NOVICE" */
						0x05,0x18,0x10,0x05,0x12,0x14, /* "EXPERT" */
						0x00,0x00,0x00,0x10,0x12,0x0f};/* "   PRO" */

	/* gear shift indicator - not a part of the original game!!! */
	gear_buf[5]=0x30 + nitedrvr_gear;
	for (offs = 0; offs < 6; offs++)
		drawgfx(bitmap,Machine->gfx[0],
				gear_buf[offs],0,
				0,0,(offs)*8,31*8,
				&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* track indicator - not a part of the original game!!! */
	for (offs = 0; offs < 6; offs++)
		drawgfx(bitmap,Machine->gfx[0],
				track_buf[offs + 6*nitedrvr_track],0,
				0,0,(offs+26)*8,31*8,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
}

VIDEO_UPDATE( nitedrvr )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
	nitedrvr_draw_road(bitmap);
	/*nitedrvr_draw_hacks(bitmap);*/
}
