/***************************************************************************

	Atari Sky Diver hardware

***************************************************************************/

#include "driver.h"
#include "artwork.h"
#include "skydiver.h"


data8_t *skydiver_videoram;

static struct tilemap *bg_tilemap;
static int width = 0;


MACHINE_INIT( skydiver )
{
	/* reset all latches */
	skydiver_start_lamp_1_w(0, 0);
	skydiver_start_lamp_2_w(0, 0);
	skydiver_lamp_s_w(0, 0);
	skydiver_lamp_k_w(0, 0);
	skydiver_lamp_y_w(0, 0);
	skydiver_lamp_d_w(0, 0);
	skydiver_lamp_i_w(0, 0);
	skydiver_lamp_v_w(0, 0);
	skydiver_lamp_e_w(0, 0);
	skydiver_lamp_r_w(0, 0);
	skydiver_width_w(0, 0);
	skydiver_coin_lockout_w(0, 0);
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	data8_t code = skydiver_videoram[tile_index];
	SET_TILE_INFO(0, code & 0x3f, code >> 6, 0)
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( skydiver )
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	return !bg_tilemap;
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

WRITE_HANDLER( skydiver_videoram_w )
{
	if (skydiver_videoram[offset] != data)
	{
		skydiver_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap, offset);
	}
}


READ_HANDLER( skydiver_wram_r )
{
	return skydiver_videoram[offset | 0x380];
}

WRITE_HANDLER( skydiver_wram_w )
{
	skydiver_videoram[offset | 0x0380] = data;
}


WRITE_HANDLER( skydiver_width_w )
{
	width = offset;
}


WRITE_HANDLER( skydiver_coin_lockout_w )
{
	coin_lockout_global_w(!offset);
}


WRITE_HANDLER( skydiver_start_lamp_1_w )
{
	set_led_status(0, offset);
}

WRITE_HANDLER( skydiver_start_lamp_2_w )
{
	set_led_status(1, offset);
}


WRITE_HANDLER( skydiver_lamp_s_w )
{
	artwork_show("lamps", offset);
}

WRITE_HANDLER( skydiver_lamp_k_w )
{
	artwork_show("lampk", offset);
}

WRITE_HANDLER( skydiver_lamp_y_w )
{
	artwork_show("lampy", offset);
}

WRITE_HANDLER( skydiver_lamp_d_w )
{
	artwork_show("lampd", offset);
}

WRITE_HANDLER( skydiver_lamp_i_w )
{
	artwork_show("lampi", offset);
}

WRITE_HANDLER( skydiver_lamp_v_w )
{
	artwork_show("lampv", offset);
}

WRITE_HANDLER( skydiver_lamp_e_w )
{
	artwork_show("lampe", offset);
}

WRITE_HANDLER( skydiver_lamp_r_w )
{
	artwork_show("lampr", offset);
}


/*************************************
 *
 *	Video update
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int pic;


	/* draw each one of our four motion objects, the two PLANE sprites
	   can be drawn double width */
	for (pic = 3; pic >= 0; pic--)
	{
		int sx,sy;
		int charcode;
		int xflip, yflip;
		int color;
		int wide;

		sx = 29*8 - skydiver_videoram[pic + 0x0390];
		sy = 30*8 - skydiver_videoram[pic*2 + 0x0398];
		charcode = skydiver_videoram[pic*2 + 0x0399];
		xflip = charcode & 0x10;
		yflip = charcode & 0x08;
		wide = (~pic & 0x02) && width;
		charcode = (charcode & 0x07) | ((charcode & 0x60) >> 2);
		color = pic & 0x01;

		if (wide)
		{
			sx -= 8;
		}

		drawgfxzoom(bitmap,Machine->gfx[1],
			charcode, color,
			xflip,yflip,sx,sy,
			cliprect,TRANSPARENCY_PEN,0,
			wide ? 0x20000 : 0x10000, 0x10000);
	}
}


VIDEO_UPDATE( skydiver )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	draw_sprites(bitmap, cliprect);
}
