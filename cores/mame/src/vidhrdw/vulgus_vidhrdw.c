/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *vulgus_fgvideoram,*vulgus_bgvideoram;
unsigned char *vulgus_scroll_low,*vulgus_scroll_high;

static int vulgus_palette_bank;
static struct tilemap *fg_tilemap, *bg_tilemap;


/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/

PALETTE_INIT( vulgus )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* characters use colors 32-47 (?) */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = *(color_prom++) + 32;

	/* sprites use colors 16-31 */
	for (i = 0;i < TOTAL_COLORS(2);i++)
	{
		COLOR(2,i) = *(color_prom++) + 16;
	}

	/* background tiles use colors 0-15, 64-79, 128-143, 192-207 in four banks */
	for (i = 0;i < TOTAL_COLORS(1)/4;i++)
	{
		COLOR(1,i) = *color_prom;
		COLOR(1,i+32*8) = *color_prom + 64;
		COLOR(1,i+2*32*8) = *color_prom + 128;
		COLOR(1,i+3*32*8) = *color_prom + 192;
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code, color;

	code = vulgus_fgvideoram[tile_index];
	color = vulgus_fgvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			0,
			code + ((color & 0x80) << 1),
			color & 0x3f,
			0)
}

static void get_bg_tile_info(int tile_index)
{
	int code, color;

	code = vulgus_bgvideoram[tile_index];
	color = vulgus_bgvideoram[tile_index + 0x400];
	SET_TILE_INFO(
			1,
			code + ((color & 0x80) << 1),
			(color & 0x1f) + (0x20 * vulgus_palette_bank),
			TILE_FLIPYX((color & 0x60) >> 5))
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( vulgus )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR, 8, 8,32,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE           ,16,16,32,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,47);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( vulgus_fgvideoram_w )
{
	vulgus_fgvideoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( vulgus_bgvideoram_w )
{
	vulgus_bgvideoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
}


WRITE_HANDLER( vulgus_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0, data & 0x01);
	coin_counter_w(1, data & 0x02);

	/* bit 7 flips screen */
	flip_screen_set(data & 0x80);
}


WRITE_HANDLER( vulgus_palette_bank_w )
{
	if (vulgus_palette_bank != (data & 3))
	{
		vulgus_palette_bank = data & 3;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;


	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int code,i,col,sx,sy,dir;


		code = spriteram[offs];
		col = spriteram[offs + 1] & 0x0f;
		sx = spriteram[offs + 3];
		sy = spriteram[offs + 2];
		dir = 1;
		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			dir = -1;
		}

		i = (spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

		do
		{
			drawgfx(bitmap,Machine->gfx[2],
					code + i,
					col,
					flip_screen,flip_screen,
					sx, sy + 16 * i * dir,
					cliprect,TRANSPARENCY_PEN,15);

			/* draw again with wraparound */
			drawgfx(bitmap,Machine->gfx[2],
					code + i,
					col,
					flip_screen,flip_screen,
					sx, sy + 16 * i * dir -  dir * 256,
					cliprect,TRANSPARENCY_PEN,15);
			i--;
		} while (i >= 0);
	}
}

VIDEO_UPDATE( vulgus )
{
	tilemap_set_scrollx(bg_tilemap, 0, vulgus_scroll_low[1] + 256 * vulgus_scroll_high[1]);
	tilemap_set_scrolly(bg_tilemap, 0, vulgus_scroll_low[0] + 256 * vulgus_scroll_high[0]);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
