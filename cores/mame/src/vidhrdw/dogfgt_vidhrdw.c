#include "driver.h"
#include "vidhrdw/generic.h"
#include "dogfgt.h"


data8_t *dogfgt_bgvideoram;

static data8_t *bitmapram;
static int bm_plane;
static struct mame_bitmap *pixbitmap;
static int pixcolor;
static struct tilemap *bg_tilemap;

#define PIXMAP_COLOR_BASE (16+32)

#define BITMAPRAM_SIZE 0x6000


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Dog-Fight has both palette RAM and PROMs. The PROMs are used for tiles &
  pixmap, RAM for sprites.

***************************************************************************/

PALETTE_INIT( dogfgt )
{
	int i;

	/* first 16 colors are RAM */

	for (i = 0;i < 64;i++)
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

		palette_set_color(i+16,r,g,b);
		color_prom++;
	}
}


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info(int tile_index)
{
	SET_TILE_INFO(
			0,
			dogfgt_bgvideoram[tile_index],
			dogfgt_bgvideoram[tile_index + 0x400] & 0x03,
			0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( dogfgt )
{
	bg_tilemap = tilemap_create(get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,32,32);

	bitmapram = auto_malloc(BITMAPRAM_SIZE);

	pixbitmap = bitmap_alloc(256,256);

	if (!bg_tilemap || !bitmapram || !pixbitmap)
		return 1;

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE_HANDLER( dogfgt_plane_select_w )
{
	bm_plane = data;
}

READ_HANDLER( dogfgt_bitmapram_r )
{
	if (bm_plane > 2)
	{
		usrintf_showmessage("bitmapram_r offs %04x plane %d\n",offset,bm_plane);
		return 0;
	}

	return bitmapram[offset + BITMAPRAM_SIZE/3 * bm_plane];
}

static WRITE_HANDLER( internal_bitmapram_w )
{
	int x,y,subx;


	bitmapram[offset] = data;

	offset &= (BITMAPRAM_SIZE/3-1);
	x = 8 * (offset / 256);
	y = offset % 256;

	for (subx = 0;subx < 8;subx++)
	{
		int i,color = 0;

		for (i = 0;i < 3;i++)
			color |= ((bitmapram[offset + BITMAPRAM_SIZE/3 * i] >> subx) & 1) << i;
		if (flip_screen)
			plot_pixel(pixbitmap,(x+subx)^0xff,y^0xff,PIXMAP_COLOR_BASE + 8*pixcolor + color);
		else
			plot_pixel(pixbitmap,x+subx,y,PIXMAP_COLOR_BASE + 8*pixcolor + color);
	}
}

WRITE_HANDLER( dogfgt_bitmapram_w )
{
	if (bm_plane > 2)
	{
		usrintf_showmessage("bitmapram_w offs %04x plane %d\n",offset,bm_plane);
		return;
	}

	internal_bitmapram_w(offset + BITMAPRAM_SIZE/3 * bm_plane,data);
}

WRITE_HANDLER( dogfgt_bgvideoram_w )
{
	dogfgt_bgvideoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
}

WRITE_HANDLER( dogfgt_scroll_w )
{
	static int scroll[4];

	scroll[offset] = data;

	tilemap_set_scrollx(bg_tilemap,0,scroll[0] + 256 * scroll[1] + 256);
	tilemap_set_scrolly(bg_tilemap,0,scroll[2] + 256 * scroll[3]);
}

WRITE_HANDLER( dogfgt_1800_w )
{
	/* bits 0 and 1 are probably text color (not verified because PROM is missing) */
	pixcolor = ((data & 0x01) << 1) | ((data & 0x02) >> 1);

	/* bits 4 and 5 are coin counters */
	coin_counter_w(0,data & 0x10);
	coin_counter_w(1,data & 0x20);

	/* bit 7 is screen flip */
	flip_screen_set(data & 0x80);

	/* other bits unused? */
	log_cb(RETRO_LOG_DEBUG, LOGPRE "PC %04x: 1800 = %02x\n",activecpu_get_pc(),data);
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] & 0x01)
		{
			int sx,sy,flipx,flipy;

			sx = spriteram[offs+3];
			sy = (240 - spriteram[offs+2]) & 0xff;
			flipx = spriteram[offs] & 0x04;
			flipy = spriteram[offs] & 0x02;
			if (flip_screen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[1],
					spriteram[offs+1] + ((spriteram[offs] & 0x30) << 4),
					(spriteram[offs] & 0x08) >> 3,
					flipx,flipy,
					sx,sy,
					cliprect,TRANSPARENCY_PEN,0);
		}
	}
}


VIDEO_UPDATE( dogfgt )
{
	static int lastflip,lastpixcolor;
	int offs;


	if (lastflip != flip_screen || lastpixcolor != pixcolor)
	{
		lastflip = flip_screen;
		lastpixcolor = pixcolor;

		for (offs = 0;offs < BITMAPRAM_SIZE;offs++)
			internal_bitmapram_w(offs,bitmapram[offs]);
	}


	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);

	draw_sprites(bitmap,cliprect);

	copybitmap(bitmap,pixbitmap,0,0,0,0,cliprect,TRANSPARENCY_COLOR,PIXMAP_COLOR_BASE + 8*pixcolor);
}
