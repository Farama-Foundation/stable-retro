/***************************************************************************

	Kyugo hardware games

***************************************************************************/

#include "driver.h"
#include "kyugo.h"


data8_t *kyugo_fgvideoram;
data8_t *kyugo_bgvideoram;
data8_t *kyugo_bgattribram;
data8_t *kyugo_spriteram_1;
data8_t *kyugo_spriteram_2;


static data8_t scroll_x_lo;
static data8_t scroll_x_hi;
static data8_t scroll_y;
static struct tilemap *fg_tilemap;
static struct tilemap *bg_tilemap;
static int bgpalbank,fgcolor;
static int flipscreen;
static const UINT8 *color_codes;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_fg_tile_info(int tile_index)
{
	int code = kyugo_fgvideoram[tile_index];
	SET_TILE_INFO(0,
				  code,
				  2*color_codes[code >> 3] + fgcolor,
				  0)
}


static void get_bg_tile_info(int tile_index)
{
	int code = kyugo_bgvideoram[tile_index];
	int attr = kyugo_bgattribram[tile_index];
	SET_TILE_INFO(1,
				  code | ((attr & 0x03) << 8),
				  (attr >> 4) | (bgpalbank << 4),
				  TILE_FLIPYX((attr & 0x0c) >> 2))
}


/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( kyugo )
{
	color_codes = memory_region(REGION_PROMS) + 0x300;


	fg_tilemap = tilemap_create(get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,8, 64,32);
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      8,8, 64,32);

	if (!fg_tilemap || !bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	tilemap_set_scrolldx(fg_tilemap,   0, 224);
	tilemap_set_scrolldx(bg_tilemap, -32, 32);

	return 0;
}


/*************************************
 *
 *	Memory handlers
 *
 *************************************/

WRITE_HANDLER( kyugo_fgvideoram_w )
{
	if (kyugo_fgvideoram[offset] != data)
	{
		kyugo_fgvideoram[offset] = data;
		tilemap_mark_tile_dirty( fg_tilemap, offset );
	}
}


WRITE_HANDLER( kyugo_bgvideoram_w )
{
	if (kyugo_bgvideoram[offset] != data)
	{
		kyugo_bgvideoram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset );
	}
}


WRITE_HANDLER( kyugo_bgattribram_w )
{
	if (kyugo_bgattribram[offset] != data)
	{
		kyugo_bgattribram[offset] = data;
		tilemap_mark_tile_dirty( bg_tilemap, offset );
	}
}


READ_HANDLER( kyugo_spriteram_2_r )
{
	/* only the lower nibble is connected*/
	return kyugo_spriteram_2[offset] | 0xf0;
}


WRITE_HANDLER( kyugo_scroll_x_lo_w )
{
	scroll_x_lo = data;
}


WRITE_HANDLER( kyugo_gfxctrl_w )
{
	/* bit 0 is scroll MSB */
	scroll_x_hi = data & 0x01;

	/* bit 5 is front layer color (Son of Phoenix only) */
	if (fgcolor != ((data & 0x20) >> 5))
	{
		fgcolor = (data & 0x20) >> 5;

		tilemap_mark_all_tiles_dirty(fg_tilemap);
	}

	/* bit 6 is background palette bank */
	if (bgpalbank != ((data & 0x40) >> 6))
	{
		bgpalbank = (data & 0x40) >> 6;

		tilemap_mark_all_tiles_dirty(bg_tilemap);
	}

	if (data & 0x9e)
		usrintf_showmessage("%02x",data);
}


WRITE_HANDLER( kyugo_scroll_y_w )
{
	scroll_y = data;
}


WRITE_HANDLER( kyugo_flipscreen_w )
{
	if (flipscreen != (data & 0x01))
	{
		flipscreen = (data & 0x01);

		tilemap_set_flip(ALL_TILEMAPS, (flipscreen ? (TILEMAP_FLIPX | TILEMAP_FLIPY): 0));
	}
}


/*************************************
 *
 *	Video update
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	/* sprite information is scattered through memory */
	/* and uses a portion of the text layer memory (outside the visible area) */
	data8_t *spriteram_area1 = &kyugo_spriteram_1[0x28];
	data8_t *spriteram_area2 = &kyugo_spriteram_2[0x28];
	data8_t *spriteram_area3 = &kyugo_fgvideoram[0x28];

	int n;

	for( n = 0; n < 12*2; n++ )
	{
		int offs,y,sy,sx,color;

		offs = 2*(n % 12) + 64*(n / 12);

		sx = spriteram_area3[offs+1] + 256 * (spriteram_area2[offs+1] & 1);
		if (sx > 320) sx -= 512;

		sy = 255 - spriteram_area1[offs];
		if (flipscreen) sy = 240 - sy;

		color = spriteram_area1[offs+1] & 0x1f;

		for (y = 0;y < 16;y++)
		{
			int code,attr,flipx,flipy;

			code = spriteram_area3[offs + 128*y];
			attr = spriteram_area2[offs + 128*y];

			code = code | ((attr & 0x01) << 9) | ((attr & 0x02) << 7);

			flipx =  attr & 0x08;
			flipy =  attr & 0x04;

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}


			drawgfx( bitmap, Machine->gfx[2],
					 code,
					 color,
					 flipx,flipy,
					 sx,flipscreen ? sy - 16*y : sy + 16*y,
					 cliprect,TRANSPARENCY_PEN, 0 );
		 }
	}
}


VIDEO_UPDATE( kyugo )
{
	if (flipscreen)
		tilemap_set_scrollx(bg_tilemap,0,-(scroll_x_lo + (scroll_x_hi*256)));
	else
		tilemap_set_scrollx(bg_tilemap,0,  scroll_x_lo + (scroll_x_hi*256));

	tilemap_set_scrolly(bg_tilemap,0,scroll_y);

	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);
	draw_sprites(bitmap, cliprect);
	tilemap_draw(bitmap, cliprect, fg_tilemap, 0, 0);
}
