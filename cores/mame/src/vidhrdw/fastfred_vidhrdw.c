/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "fastfred.h"

extern UINT8 galaxian_stars_on;
extern void galaxian_init_stars(int colors_offset);
extern void galaxian_draw_stars(struct mame_bitmap *bitmap);

data8_t *fastfred_videoram;
data8_t *fastfred_spriteram;
size_t fastfred_spriteram_size;
data8_t *fastfred_attributesram;
data8_t *imago_fg_videoram;


static struct rectangle spritevisiblearea =
{
      2*8, 32*8-1,
      2*8, 30*8-1
};

static struct rectangle spritevisibleareaflipx =
{
        0*8, 30*8-1,
        2*8, 30*8-1
};

static data16_t charbank;
static data8_t colorbank;
static int flip_screen_x;
static int flip_screen_y;
int fastfred_hardware_type;
static const UINT8 *fastfred_color_prom;
static struct tilemap *bg_tilemap, *fg_tilemap, *web_tilemap;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  bit 0 -- 1  kohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 220 ohm resistor  -- RED/GREEN/BLUE
  bit 3 -- 100 ohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

static void set_color(pen_t pen, int i)
{
	UINT8 r,g,b;
	int bit0, bit1, bit2, bit3;

	bit0 = (fastfred_color_prom[i] >> 0) & 0x01;
	bit1 = (fastfred_color_prom[i] >> 1) & 0x01;
	bit2 = (fastfred_color_prom[i] >> 2) & 0x01;
	bit3 = (fastfred_color_prom[i] >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	bit0 = (fastfred_color_prom[i + 0x100] >> 0) & 0x01;
	bit1 = (fastfred_color_prom[i + 0x100] >> 1) & 0x01;
	bit2 = (fastfred_color_prom[i + 0x100] >> 2) & 0x01;
	bit3 = (fastfred_color_prom[i + 0x100] >> 3) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	bit0 = (fastfred_color_prom[i + 0x200] >> 0) & 0x01;
	bit1 = (fastfred_color_prom[i + 0x200] >> 1) & 0x01;
	bit2 = (fastfred_color_prom[i + 0x200] >> 2) & 0x01;
	bit3 = (fastfred_color_prom[i + 0x200] >> 3) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	palette_set_color(pen,r,g,b);
}

PALETTE_INIT( fastfred )
{
	pen_t i;
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	fastfred_color_prom = color_prom;	/* we'll need this later */

	for (i = 0;i < 256;i++)
	{
		set_color(i, i);
	}


	/* characters and sprites use the same palette */
	for (i = 0; i < 256; i++)
	{
		pen_t color;

		if ((i & 0x07) == 0)
			color = 0;
		else
			color = i;

		COLOR(0,i) = COLOR(1,i) = color;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	data8_t x = tile_index & 0x1f;

	data16_t code = charbank | fastfred_videoram[tile_index];
	data8_t color = colorbank | (fastfred_attributesram[2 * x + 1] & 0x07);

	SET_TILE_INFO(0, code, color, 0)
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( fastfred )
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	if (!bg_tilemap)
		return 1;

	tilemap_set_scroll_cols(bg_tilemap, 32);

	return 0;
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

WRITE_HANDLER( fastfred_videoram_w )
{
	if (fastfred_videoram[offset] != data)
	{
		fastfred_videoram[offset] = data;

		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}


WRITE_HANDLER( fastfred_attributes_w )
{
	if (fastfred_attributesram[offset] != data)
	{
		if (offset & 0x01)
		{
			/* color change */
			int i;

			for (i = offset / 2; i < 0x0400; i += 32)
				tilemap_mark_tile_dirty(bg_tilemap, i);
		}
		else
		{
			/* coloumn scroll */
			tilemap_set_scrolly(bg_tilemap, offset / 2, data);
		}

		fastfred_attributesram[offset] = data;
	}
}


WRITE_HANDLER( fastfred_charbank1_w )
{
	data16_t new_data = (charbank & 0x0200) | ((data & 0x01) << 8);

	if (new_data != charbank)
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	
		charbank = new_data;
	}
}

WRITE_HANDLER( fastfred_charbank2_w )
{
	data16_t new_data = (charbank & 0x0100) | ((data & 0x01) << 9);

	if (new_data != charbank)
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	
		charbank = new_data;
	}
}


WRITE_HANDLER( fastfred_colorbank1_w )
{
	data8_t new_data = (colorbank & 0x10) | ((data & 0x01) << 3);

	if (new_data != colorbank)
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap);

		colorbank = new_data;
	}
}

WRITE_HANDLER( fastfred_colorbank2_w )
{
	data8_t new_data = (colorbank & 0x08) | ((data & 0x01) << 4);

	if (new_data != colorbank)
	{
		tilemap_mark_all_tiles_dirty(bg_tilemap);

		colorbank = new_data;
	}
}


WRITE_HANDLER( fastfred_background_color_w )
{
	set_color(0, data);
}


WRITE_HANDLER( fastfred_flip_screen_x_w )
{
	if (flip_screen_x != (data & 0x01))
	{
		flip_screen_x = data & 0x01;

		tilemap_set_flip(bg_tilemap, (flip_screen_x ? TILEMAP_FLIPX : 0) | (flip_screen_y ? TILEMAP_FLIPY : 0));
	}
}

WRITE_HANDLER( fastfred_flip_screen_y_w )
{
	if (flip_screen_y != (data & 0x01))
	{
		flip_screen_y = data & 0x01;

		tilemap_set_flip(bg_tilemap, (flip_screen_x ? TILEMAP_FLIPX : 0) | (flip_screen_y ? TILEMAP_FLIPY : 0));
	}
}



/*************************************
 *
 *	Video update
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	for (offs = fastfred_spriteram_size - 4; offs >= 0; offs -= 4)
	{
		UINT8 code,sx,sy;
		int flipx,flipy;

		sx = fastfred_spriteram[offs + 3];
		sy = 240 - fastfred_spriteram[offs];

		if (fastfred_hardware_type == 3)
		{
			/* Imago*/
			code  = (fastfred_spriteram[offs + 1]) & 0x3f;

			flipx = 0;
			flipy = 0;
		}
		else if (fastfred_hardware_type == 2)
		{
			/* Boggy 84*/
			code  =  fastfred_spriteram[offs + 1] & 0x7f;
			flipx =  0;
			flipy =  fastfred_spriteram[offs + 1] & 0x80;
		}
		else if (fastfred_hardware_type == 1)
		{
			/* Fly-Boy/Fast Freddie/Red Robin*/
			code  =  fastfred_spriteram[offs + 1] & 0x7f;
			flipx =  0;
			flipy = ~fastfred_spriteram[offs + 1] & 0x80;
		}
		else
		{
			/* Jump Coaster*/
			code  = (fastfred_spriteram[offs + 1] & 0x3f) | 0x40;
			flipx = ~fastfred_spriteram[offs + 1] & 0x40;
			flipy =  fastfred_spriteram[offs + 1] & 0x80;
		}


		if (flip_screen_x)
		{
			sx = 240 - sx;
			flipx = !flipx;
		}
		if (flip_screen_y)
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[1],
				code,
				colorbank | (fastfred_spriteram[offs + 2] & 0x07),
				flipx,flipy,
				sx,sy,
				flip_screen_x ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}


VIDEO_UPDATE( fastfred )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	draw_sprites(bitmap, cliprect);
}


static void imago_get_tile_info_bg(int tile_index)
{
	data8_t x = tile_index & 0x1f;

	data16_t code = charbank * 0x100 + fastfred_videoram[tile_index];
	data8_t color = colorbank | (fastfred_attributesram[2 * x + 1] & 0x07);

	SET_TILE_INFO(0, code, color, 0)
}

static void imago_get_tile_info_fg(int tile_index)
{
	int code = imago_fg_videoram[tile_index];
	SET_TILE_INFO(2, code, 2, 0)
}

static void imago_get_tile_info_web(int tile_index)
{
	SET_TILE_INFO(3, tile_index & 0x1ff, 0, 0);
}

WRITE_HANDLER( imago_fg_videoram_w )
{
	if( imago_fg_videoram[offset] != data)
	{
		imago_fg_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap, offset);
	}
}

WRITE_HANDLER( imago_charbank_w )
{
	if( charbank != data )
	{
		charbank = data;
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}
}

VIDEO_START( imago )
{
	web_tilemap = tilemap_create(imago_get_tile_info_web,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	bg_tilemap  = tilemap_create(imago_get_tile_info_bg, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	fg_tilemap  = tilemap_create(imago_get_tile_info_fg, tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if( !web_tilemap || !bg_tilemap || !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 0);
	tilemap_set_transparent_pen(fg_tilemap, 0);

	/* the game has a galaxian starfield */
	galaxian_init_stars(256);
	galaxian_stars_on = 1;

	/* web colors */
	palette_set_color(256+64+0, 0x50, 0x00, 0x00);
	palette_set_color(256+64+1, 0x00, 0x00, 0x00);

	return 0;
}

VIDEO_UPDATE( imago )
{
	tilemap_draw(bitmap,cliprect,web_tilemap,0,0);

	galaxian_draw_stars(bitmap);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	draw_sprites(bitmap, cliprect);
	
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
