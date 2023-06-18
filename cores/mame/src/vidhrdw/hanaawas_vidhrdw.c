/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/

PALETTE_INIT( hanaawas )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 0x10;
	/* color_prom now points to the beginning of the lookup table */


	/* character lookup table.  The 1bpp tiles really only use colors 0-0x0f and the
	   3bpp ones 0x10-0x1f */

	for (i = 0;i < TOTAL_COLORS(0)/8 ;i++)
	{
		COLOR(0,i*8+0) = color_prom[i*4+0x00] & 0x0f;
		COLOR(0,i*8+1) = color_prom[i*4+0x01] & 0x0f;
		COLOR(0,i*8+2) = color_prom[i*4+0x02] & 0x0f;
		COLOR(0,i*8+3) = color_prom[i*4+0x03] & 0x0f;
		COLOR(0,i*8+4) = color_prom[i*4+0x80] & 0x0f;
		COLOR(0,i*8+5) = color_prom[i*4+0x81] & 0x0f;
		COLOR(0,i*8+6) = color_prom[i*4+0x82] & 0x0f;
		COLOR(0,i*8+7) = color_prom[i*4+0x83] & 0x0f;
	}
}

WRITE_HANDLER( hanaawas_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( hanaawas_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;

		/* dirty both current and next offsets */
		tilemap_mark_tile_dirty(bg_tilemap, offset);
		tilemap_mark_tile_dirty(bg_tilemap, (offset + (flip_screen ? -1 : 1)) & 0x03ff);
	}
}

WRITE_HANDLER( hanaawas_portB_w )
{
	/* bit 7 is flip screen */
	if (flip_screen != (~data & 0x80))
	{
		flip_screen_set(~data & 0x80);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
}

static void get_bg_tile_info(int tile_index)
{
	/* the color is determined by the current color byte, but the bank is via the previous one!!! */
	int offset = (tile_index + (flip_screen ? 1 : -1)) & 0x3ff;
	int attr = colorram[offset];
	int gfxbank = (attr & 0x40) >> 6;
	int code = videoram[tile_index] + ((attr & 0x20) << 3);
	int color = colorram[tile_index] & 0x1f;
	
	SET_TILE_INFO(gfxbank, code, color, 0)
}

VIDEO_START( hanaawas )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 32, 32);

	if ( !bg_tilemap )
		return 1;
	
	return 0;
}

VIDEO_UPDATE( hanaawas )
{
	tilemap_draw(bitmap, &Machine->visible_area, bg_tilemap, 0, 0);
}
