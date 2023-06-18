/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



data8_t *citycon_videoram;
data8_t *citycon_linecolor;
data8_t *citycon_scroll;

static int bg_image;
static struct tilemap *bg_tilemap,*fg_tilemap;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static UINT32 citycon_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return (col & 0x1f) + ((row & 0x1f) << 5) + ((col & 0x60) << 5);
}

static void get_fg_tile_info(int tile_index)
{
	SET_TILE_INFO(
			0,
			citycon_videoram[tile_index],
			(tile_index & 0x03e0) >> 5,	/* color depends on scanline only */
			0)
}

static void get_bg_tile_info(int tile_index)
{
	data8_t *rom = memory_region(REGION_GFX4);
	int code = rom[0x1000 * bg_image + tile_index];
	SET_TILE_INFO(
			3 + bg_image,
			code,
			rom[0xc000 + 0x100 * bg_image + code],
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( citycon )
{
	fg_tilemap = tilemap_create(get_fg_tile_info,citycon_scan,TILEMAP_TRANSPARENT,8,8,128,32);
	bg_tilemap = tilemap_create(get_bg_tile_info,citycon_scan,TILEMAP_OPAQUE,     8,8,128,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_scroll_rows(fg_tilemap,32);

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( citycon_videoram_w )
{
	if (citycon_videoram[offset] != data)
	{
		citycon_videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}


WRITE_HANDLER( citycon_linecolor_w )
{
	citycon_linecolor[offset] = data;
}


WRITE_HANDLER( citycon_background_w )
{
	/* bits 4-7 control the background image */
	if (bg_image != (data >> 4))
	{
		bg_image = (data >> 4);
		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	/* bit 0 flips screen */
	/* it is also used to multiplex player 1 and player 2 controls */
	flip_screen_set(data & 0x01);

	/* bits 1-3 are unknown */
/*	if ((data & 0x0e) != 0) log_cb(RETRO_LOG_DEBUG, LOGPRE "background register = %02x\n",data);*/
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int offs;

	for (offs = spriteram_size-4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx;


		sx = spriteram[offs + 3];
		sy = 239 - spriteram[offs];
		flipx = ~spriteram[offs + 2] & 0x10;
		if (flip_screen)
		{
			sx = 240 - sx;
			sy = 238 - sy;
			flipx = !flipx;
		}

		drawgfx(bitmap,Machine->gfx[spriteram[offs + 1] & 0x80 ? 2 : 1],
				spriteram[offs + 1] & 0x7f,
				spriteram[offs + 2] & 0x0f,
				flipx,flip_screen,
				sx,sy,
				cliprect,TRANSPARENCY_PEN,0);
	}
}


static INLINE void changecolor_RRRRGGGGBBBBxxxx(int color,int indx)
{
	int r,g,b;
	int data = paletteram[2*indx | 1] | (paletteram[2*indx] << 8);

	r = (data >> 12) & 0x0f;
	g = (data >>  8) & 0x0f;
	b = (data >>  4) & 0x0f;

	r = (r << 4) | r;
	g = (g << 4) | g;
	b = (b << 4) | b;

	palette_set_color(color,r,g,b);
}

VIDEO_UPDATE( citycon )
{
	int offs,scroll;


	/* Update the virtual palette to support text color code changing on every scanline. */
	for (offs = 0;offs < 256;offs++)
	{
		int indx = citycon_linecolor[offs];
		int i;

		for (i = 0;i < 4;i++)
			changecolor_RRRRGGGGBBBBxxxx(640 + 4*offs + i,512 + 4*indx + i);
	}


	scroll = citycon_scroll[0]*256 + citycon_scroll[1];
	tilemap_set_scrollx(bg_tilemap,0,scroll >> 1);
	for (offs = 6;offs < 32;offs++)
		tilemap_set_scrollx(fg_tilemap,offs,scroll);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
	draw_sprites(bitmap,cliprect);
}
