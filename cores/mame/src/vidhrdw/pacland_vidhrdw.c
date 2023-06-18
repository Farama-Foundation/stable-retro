#include "driver.h"
#include "vidhrdw/generic.h"

UINT8 *pacland_videoram2;

static int palette_bank;
static const UINT8 *pacland_color_prom;

static struct rectangle spritevisiblearea =
{
	3*8, 39*8-1,
	5*8, 29*8-1
};

static struct tilemap *bg_tilemap, *fg_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Pacland has one 1024x8 and one 1024x4 palette PROM; and three 1024x8 lookup
  table PROMs (sprites, bg tiles, fg tiles).
  The palette has 1024 colors, but it is bank switched (4 banks) and only 256
  colors are visible at a time. So, instead of creating a static palette, we
  modify it when the bank switching takes place.
  The color PROMs are connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- GREEN
		-- 470 ohm resistor  -- GREEN
		-- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
		-- 470 ohm resistor  -- RED
		-- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 3 -- 220 ohm resistor  -- BLUE
		-- 470 ohm resistor  -- BLUE
		-- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
PALETTE_INIT( pacland )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	pacland_color_prom = color_prom;	/* we'll need this later */
	/* skip the palette data, it will be initialized later */
	color_prom += 2 * 1024;
	/* color_prom now points to the beginning of the lookup table */

	/* Sprites */
	for (i = 0;i < TOTAL_COLORS(2)/3;i++)
	{
		COLOR(2,i) = *(color_prom++);

		/* color 0x7f is special, it makes the foreground tiles it overlaps */
		/* transparent (used in round 19) */
		if (COLOR(2,i) == 0x7f) COLOR(2,i + 2*TOTAL_COLORS(2)/3) = COLOR(2,i);
		else COLOR(2,i + 2*TOTAL_COLORS(2)/3) = 0xff;

		/* transparent colors are 0x7f and 0xff - map all to 0xff */
		if (COLOR(2,i) == 0x7f) COLOR(2,i) = 0xff;

		/* high priority colors which appear over the foreground even when */
		/* the foreground has priority over sprites */
		if (COLOR(2,i) >= 0xf0) COLOR(2,i + TOTAL_COLORS(2)/3) = COLOR(2,i);
		else COLOR(2,i + TOTAL_COLORS(2)/3) = 0xff;
	}

	/* Foreground */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		COLOR(0,i) = *(color_prom++);
		/* transparent colors are 0x7f and 0xff - map all to 0xff */
		if (COLOR(0,i) == 0x7f) COLOR(0,i) = 0xff;
	}

	/* Background */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = *(color_prom++);
	}
}

WRITE_HANDLER( pacland_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset / 2);
	}
}

WRITE_HANDLER( pacland_videoram2_w )
{
	if (pacland_videoram2[offset] != data)
	{
		pacland_videoram2[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
	}
}

WRITE_HANDLER( pacland_scroll0_w )
{
	int row;

	for (row = 5; row < 29; row++)
	{
		tilemap_set_scrollx(fg_tilemap, row, data + 256 * offset);
	}
}

WRITE_HANDLER( pacland_scroll1_w )
{
	tilemap_set_scrollx(bg_tilemap, 0, data + 256 * offset);
}

WRITE_HANDLER( pacland_bankswitch_w )
{
	int bankaddress;
	UINT8 *RAM = memory_region(REGION_CPU1);

	bankaddress = 0x10000 + ((data & 0x07) << 13);
	cpu_setbank(1,&RAM[bankaddress]);

/*	pbc = data & 0x20;*/

	if (palette_bank != ((data & 0x18) >> 3))
	{
		int i;
		const UINT8 *color_prom;

		palette_bank = (data & 0x18) >> 3;
		color_prom = pacland_color_prom + 256 * palette_bank;

		for (i = 0;i < 256;i++)
		{
			int bit0,bit1,bit2,bit3;
			int r,g,b;

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
			bit0 = (color_prom[1024] >> 0) & 0x01;
			bit1 = (color_prom[1024] >> 1) & 0x01;
			bit2 = (color_prom[1024] >> 2) & 0x01;
			bit3 = (color_prom[1024] >> 3) & 0x01;
			b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

			color_prom++;

			palette_set_color(i,r,g,b);
		}
	}
}

WRITE_HANDLER( pacland_flipscreen_w )
{
	flip_screen_set(~data & 0xa0);
}

static void get_bg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int attr = pacland_videoram2[offs + 1];
	int code = pacland_videoram2[offs] + ((attr & 0x01) << 8);
	int color = ((attr & 0x3e) >> 1) + ((code & 0x1c0) >> 1);
	int flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);

	SET_TILE_INFO(1, code, color, flags)
}

static void get_fg_tile_info(int tile_index)
{
	int offs = tile_index * 2;
	int attr = videoram[offs + 1];
	int code = videoram[offs] + ((attr & 0x01) << 8);
	int color = ((attr & 0x1e) >> 1) + ((code & 0x1e0) >> 1);
	int flags = ((attr & 0x40) ? TILE_FLIPX : 0) | ((attr & 0x80) ? TILE_FLIPY : 0);

	tile_info.priority = (attr & 0x20) ? 1 : 0;

	SET_TILE_INFO(0, code, color, flags)
}

VIDEO_START( pacland )
{
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, 
		TILEMAP_OPAQUE, 8, 8, 64, 32);

	if ( !bg_tilemap )
		return 1;

	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, 
		TILEMAP_TRANSPARENT_COLOR, 8, 8, 64, 32);

	if ( !fg_tilemap )
		return 1;

	tilemap_set_scrolldx(bg_tilemap, 0, -22*8);
	tilemap_set_scrolldx(fg_tilemap, 0, -22*8);
	tilemap_set_scroll_rows(fg_tilemap, 32);
	tilemap_set_transparent_pen(fg_tilemap, 0xff);

	palette_bank = -1;

	return 0;
}

#define DRAW_SPRITE( code, sx, sy ) \
		{ drawgfx( bitmap, Machine->gfx[ 2+gfx ], code, color, flipx, flipy, sx, sy, \
		&spritevisiblearea, TRANSPARENCY_COLOR,0xff); }

static void pacland_draw_sprites( struct mame_bitmap *bitmap,int priority)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		int sprite = spriteram[offs];
		int gfx = ( spriteram_3[offs] >> 7 ) & 1;
		int color = ( spriteram[offs+1] & 0x3f ) + 64 * priority;
		int x = (spriteram_2[offs+1]) + 0x100*(spriteram_3[offs+1] & 1) - 48;
		int y = 256 - spriteram_2[offs] - 23;
		int flipy = spriteram_3[offs] & 2;
		int flipx = spriteram_3[offs] & 1;

		if (flip_screen)
		{
			x += 8;
			flipx = !flipx;
			flipy = !flipy;
		}

		switch ( spriteram_3[offs] & 0x0c )
		{
			case 0:		/* normal size */
				DRAW_SPRITE( sprite, x, y )
			break;

			case 4:		/* 2x horizontal */
				sprite &= ~1;
				if (!flipx)
				{
					DRAW_SPRITE( sprite, x, y )
					DRAW_SPRITE( 1+sprite, x+16, y )
				} else {
					DRAW_SPRITE( 1+sprite, x, y )
					DRAW_SPRITE( sprite, x+16, y )
				}
			break;

			case 8:		/* 2x vertical */
				sprite &= ~2;
				if (!flipy)
				{
					DRAW_SPRITE( sprite, x, y-16 )
					DRAW_SPRITE( 2+sprite, x, y )
				} else {
					DRAW_SPRITE( 2+sprite, x, y-16 )
					DRAW_SPRITE( sprite, x, y )
				}
			break;

			case 12:		/* 2x both ways */
				sprite &= ~3;
				if ( !flipy && !flipx )
				{
					DRAW_SPRITE( sprite, x, y-16 )
					DRAW_SPRITE( 1+sprite, x+16, y-16 )
					DRAW_SPRITE( 2+sprite, x, y )
					DRAW_SPRITE( 3+sprite, x+16, y )
				} else if ( flipy && flipx ) {
					DRAW_SPRITE( 3+sprite, x, y-16 )
					DRAW_SPRITE( 2+sprite, x+16, y-16 )
					DRAW_SPRITE( 1+sprite, x, y )
					DRAW_SPRITE( sprite, x+16, y )
				} else if ( flipx ) {
					DRAW_SPRITE( 1+sprite, x, y-16 )
					DRAW_SPRITE( sprite, x+16, y-16 )
					DRAW_SPRITE( 3+sprite, x, y )
					DRAW_SPRITE( 2+sprite, x+16, y )
				} else /* flipy */ {
					DRAW_SPRITE( 2+sprite, x, y-16 )
					DRAW_SPRITE( 3+sprite, x+16, y-16 )
					DRAW_SPRITE( sprite, x, y )
					DRAW_SPRITE( 1+sprite, x+16, y )
				}
				break;
		}
	}
}

VIDEO_UPDATE( pacland )
{
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
	pacland_draw_sprites(bitmap, 2);
	pacland_draw_sprites(bitmap, 0);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 1, 0);
	pacland_draw_sprites(bitmap, 2);
	pacland_draw_sprites(bitmap, 1);
}
