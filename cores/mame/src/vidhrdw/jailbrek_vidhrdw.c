#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *jailbrek_scroll_x;
UINT8 *jailbrek_scroll_dir;

static struct tilemap *bg_tilemap;

PALETTE_INIT( jailbrek )
{
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])
	int i;

	for ( i = 0; i < Machine->drv->total_colors; i++ )
	{
		int bit0,bit1,bit2,bit3,r,g,b;


		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	color_prom += Machine->drv->total_colors;

	for ( i = 0; i < TOTAL_COLORS(0); i++ )
		COLOR(0,i) = ( *color_prom++ ) + 0x10;

	for ( i = 0; i < TOTAL_COLORS(1); i++ )
		COLOR(1,i) = *color_prom++;
}

WRITE_HANDLER( jailbrek_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

WRITE_HANDLER( jailbrek_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}

static void get_bg_tile_info(int tile_index)
{
	int attr = colorram[tile_index];
	int code = videoram[tile_index] + ((attr & 0xc0) << 2);
	int color = attr & 0x0f;

	SET_TILE_INFO(0, code, color, 0)
}

VIDEO_START( jailbrek )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows,
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	return 0;
}

static void jailbrek_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int i;

	for (i = 0; i < spriteram_size; i += 4)
	{
		int attr = spriteram[i + 1];	/* attributes = ?tyxcccc*/
		int code = spriteram[i] + ((attr & 0x40) << 2);
		int color = attr & 0x0f;
		int flipx = attr & 0x10;
		int flipy = attr & 0x20;
		int sx = spriteram[i + 2] - ((attr & 0x80) << 1);
		int sy = spriteram[i + 3];

		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 240 - sy;
			flipx = !flipx;
			flipy = !flipy;
		}

		drawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy,
			sx, sy, cliprect, TRANSPARENCY_COLOR, 0);
	}
}

VIDEO_UPDATE( jailbrek )
{
	int i;

	/* added support for vertical scrolling (credits).  23/1/2002  -BR*/
	/* bit 2 appears to be horizontal/vertical scroll control*/
	if (jailbrek_scroll_dir[0] & 0x04)
	{
		tilemap_set_scroll_cols(bg_tilemap, 32);
		tilemap_set_scroll_rows(bg_tilemap, 1);
		tilemap_set_scrollx(bg_tilemap, 0, 0);

		for (i = 0; i < 32; i++)
		{
			tilemap_set_scrolly(bg_tilemap, i, ((jailbrek_scroll_x[i + 32] << 8) + jailbrek_scroll_x[i]));
		}
	}
	else
	{
		tilemap_set_scroll_rows(bg_tilemap, 32);
		tilemap_set_scroll_cols(bg_tilemap, 1);
		tilemap_set_scrolly(bg_tilemap, 0, 0);

		for (i = 0; i < 32; i++)
		{
			tilemap_set_scrollx(bg_tilemap, i, ((jailbrek_scroll_x[i + 32] << 8) + jailbrek_scroll_x[i]));
		}
	}

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	jailbrek_draw_sprites(bitmap, cliprect);
}
