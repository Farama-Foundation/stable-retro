/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap;

PALETTE_INIT( kchamp )
{
	int i, red, green, blue;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		red = color_prom[i];
		green = color_prom[Machine->drv->total_colors+i];
		blue = color_prom[2*Machine->drv->total_colors+i];

		palette_set_color(i,red*0x11,green*0x11,blue*0x11);

		*(colortable++) = i;
	}
}

WRITE_HANDLER( kchamp_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( kchamp_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( kchamp_flipscreen_w )
{
	flip_screen_set(data & 0x01);
}

static void get_bg_tile_info(int tile_index)
{
	int code = videoram[tile_index] + ((colorram[tile_index] & 7) << 8);
	int color = (colorram[tile_index] >> 3) & 0x1f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( kchamp )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

/*
        Sprites
        -------
        Offset          Encoding
            0             YYYYYYYY
            1             TTTTTTTT
            2             FGGTCCCC
            3             XXXXXXXX
*/

static void kchamp_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

    for (offs = 0; offs < 0x100; offs += 4)
	{
		int attr = spriteram[offs + 2];
        int bank = 1 + ((attr & 0x60) >> 5);
        int code = spriteram[offs + 1] + ((attr & 0x10) << 4);
        int color = attr & 0x0f;
		int flipx = 0;
        int flipy = attr & 0x80;
        int sx = spriteram[offs + 3] - 8;
        int sy = 247 - spriteram[offs];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

        drawgfx(bitmap, Machine->gfx[bank], code, color, flipx, flipy, sx, sy,
            &Machine->visible_area, TRANSPARENCY_PEN, 0);
	}
}

static void kchampvs_draw_sprites( struct mame_bitmap *bitmap )
{
	int offs;

    for (offs = 0; offs < 0x100; offs += 4)
	{
		int attr = spriteram[offs + 2];
        int bank = 1 + ((attr & 0x60) >> 5);
        int code = spriteram[offs + 1] + ((attr & 0x10) << 4);
        int color = attr & 0x0f;
		int flipx = 0;
        int flipy = attr & 0x80;
        int sx = spriteram[offs + 3];
        int sy = 240 - spriteram[offs];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

        drawgfx(bitmap, Machine->gfx[bank], code, color, flipx, flipy, sx, sy,
            &Machine->visible_area, TRANSPARENCY_PEN, 0);
	}
}


VIDEO_UPDATE( kchamp )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	kchamp_draw_sprites(bitmap);
}

VIDEO_UPDATE( kchampvs )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	kchampvs_draw_sprites(bitmap);
}
