/******************************************************************************

	Video Hardware for Video System Mahjong series and Pipe Dream.

	Driver by Takahiro Nogi <nogi@kt.rim.or.jp> 2001/02/04 -
	and Bryan McPhail, Nicola Salmoria, Aaron Giles

******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "fromance.h"


static UINT8 selected_videoram;
static data8_t *local_videoram[2];

static UINT8 selected_paletteram;
static data8_t *local_paletteram;

static int scrollx[2], scrolly[2];
static UINT8 gfxreg;
static UINT8 flipscreen;

static UINT8 crtc_register;
static UINT8 crtc_data[0x10];
static void *crtc_timer;

static struct tilemap *bg_tilemap, *fg_tilemap;


static void crtc_interrupt_gen(int param);


/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static INLINE void get_fromance_tile_info(int tile_index,int layer)
{
	int tile = ((local_videoram[layer][0x0000 + tile_index] & 0x80) << 9) |
				(local_videoram[layer][0x1000 + tile_index] << 8) |
				local_videoram[layer][0x2000 + tile_index];
	int color = local_videoram[layer][tile_index] & 0x7f;

	SET_TILE_INFO(layer, tile, color, 0);
}

static void get_fromance_bg_tile_info(int tile_index) { get_fromance_tile_info(tile_index, 0); }
static void get_fromance_fg_tile_info(int tile_index) { get_fromance_tile_info(tile_index, 1); }

static INLINE void get_nekkyoku_tile_info(int tile_index,int layer)
{
	int tile = (local_videoram[layer][0x0000 + tile_index] << 8) |
				local_videoram[layer][0x1000 + tile_index];
	int color = local_videoram[layer][tile_index + 0x2000] & 0x3f;

	SET_TILE_INFO(layer, tile, color, 0);
}

static void get_nekkyoku_bg_tile_info(int tile_index) { get_nekkyoku_tile_info(tile_index, 0); }
static void get_nekkyoku_fg_tile_info(int tile_index) { get_nekkyoku_tile_info(tile_index, 1); }



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( fromance )
{
	/* allocate tilemaps */
	bg_tilemap = tilemap_create(get_fromance_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      8,4, 64,64);
	fg_tilemap = tilemap_create(get_fromance_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,4, 64,64);

	/* allocate local videoram */
	local_videoram[0] = auto_malloc(0x1000 * 3);
	local_videoram[1] = auto_malloc(0x1000 * 3);

	/* allocate local palette RAM */
	local_paletteram = auto_malloc(0x800 * 2);

	/* handle failure */
	if (!bg_tilemap || !fg_tilemap || !local_videoram[0] || !local_videoram[1] || !local_paletteram)
		return 1;

	/* configure tilemaps */
	tilemap_set_transparent_pen(fg_tilemap,15);

	/* reset the timer */
	crtc_timer = timer_alloc(crtc_interrupt_gen);
	return 0;
}

VIDEO_START( nekkyoku )
{
	/* allocate tilemaps */
	bg_tilemap = tilemap_create(get_nekkyoku_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE,      8,4, 64,64);
	fg_tilemap = tilemap_create(get_nekkyoku_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 8,4, 64,64);

	/* allocate local videoram */
	local_videoram[0] = auto_malloc(0x1000 * 3);
	local_videoram[1] = auto_malloc(0x1000 * 3);

	/* allocate local palette RAM */
	local_paletteram = auto_malloc(0x800 * 2);

	/* handle failure */
	if (!bg_tilemap || !fg_tilemap || !local_videoram[0] || !local_videoram[1] || !local_paletteram)
		return 1;

	/* configure tilemaps */
	tilemap_set_transparent_pen(fg_tilemap,15);

	/* reset the timer */
	crtc_timer = timer_alloc(crtc_interrupt_gen);
	return 0;
}



/*************************************
 *
 *	Graphics control register
 *
 *************************************/

WRITE_HANDLER( fromance_gfxreg_w )
{
	static int flipscreen_old = -1;

	gfxreg = data;
	flipscreen = (data & 0x01);
	selected_videoram = (~data >> 1) & 1;
	selected_paletteram = (data >> 6) & 1;

	if (flipscreen != flipscreen_old)
	{
		flipscreen_old = flipscreen;
		tilemap_set_flip(ALL_TILEMAPS, flipscreen ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);
	}
}



/*************************************
 *
 *	Banked palette RAM
 *
 *************************************/

READ_HANDLER( fromance_paletteram_r )
{
	/* adjust for banking and read */
	offset |= selected_paletteram << 11;
	return local_paletteram[offset];
}


WRITE_HANDLER( fromance_paletteram_w )
{
	int palword;
	int r, g, b;

	/* adjust for banking and modify */
	offset |= selected_paletteram << 11;
	local_paletteram[offset] = data;

	/* compute R,G,B */
	palword = (local_paletteram[offset | 1] << 8) | local_paletteram[offset & ~1];
	r = (palword >> 10) & 0x1f;
	g = (palword >>  5) & 0x1f;
	b = (palword >>  0) & 0x1f;

	/* up to 8 bits */
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);
	palette_set_color(offset / 2, r, g, b);
}



/*************************************
 *
 *	Video RAM read/write
 *
 *************************************/

READ_HANDLER( fromance_videoram_r )
{
	return local_videoram[selected_videoram][offset];
}


WRITE_HANDLER( fromance_videoram_w )
{
	local_videoram[selected_videoram][offset] = data;
	tilemap_mark_tile_dirty(selected_videoram ? fg_tilemap : bg_tilemap, offset & 0x0fff);
}



/*************************************
 *
 *	Scroll registers
 *
 *************************************/

WRITE_HANDLER( fromance_scroll_w )
{
	if (flipscreen)
	{
		switch (offset)
		{
			case 0:
				scrollx[1] = (data + (((gfxreg & 0x08) >> 3) * 0x100) - 0x159);
				break;
			case 1:
				scrolly[1] = (data + (((gfxreg & 0x04) >> 2) * 0x100) - 0x10);
				break;
			case 2:
				scrollx[0] = (data + (((gfxreg & 0x20) >> 5) * 0x100) - 0x159);
				break;
			case 3:
				scrolly[0] = (data + (((gfxreg & 0x10) >> 4) * 0x100) - 0x10);
				break;
		}
	}
	else
	{
		switch (offset)
		{
			case 0:
				scrollx[1] = (data + (((gfxreg & 0x08) >> 3) * 0x100) - 0x1f7);
				break;
			case 1:
				scrolly[1] = (data + (((gfxreg & 0x04) >> 2) * 0x100) - 0xfa);
				break;
			case 2:
				scrollx[0] = (data + (((gfxreg & 0x20) >> 5) * 0x100) - 0x1f7);
				break;
			case 3:
				scrolly[0] = (data + (((gfxreg & 0x10) >> 4) * 0x100) - 0xfa);
				break;
		}
	}
}



/*************************************
 *
 *	Fake video controller
 *
 *************************************/

static void crtc_interrupt_gen(int param)
{
	cpu_set_irq_line(1, 0, HOLD_LINE);
	if (param != 0)
		timer_adjust(crtc_timer, TIME_IN_HZ(Machine->drv->frames_per_second * param), 0, TIME_IN_HZ(Machine->drv->frames_per_second * param));
}


WRITE_HANDLER( fromance_crtc_data_w )
{
	crtc_data[crtc_register] = data;

	switch (crtc_register)
	{
		/* only register we know about.... */
		case 0x0b:
			timer_adjust(crtc_timer, cpu_getscanlinetime(Machine->visible_area.max_y + 1), (data > 0x80) ? 2 : 1, 0);
			break;

		default:
			log_cb(RETRO_LOG_DEBUG, LOGPRE "CRTC register %02X = %02X\n", crtc_register, data & 0xff);
			break;
	}
}


WRITE_HANDLER( fromance_crtc_register_w )
{
	crtc_register = data;
}



/*************************************
 *
 *	Sprite routines (Pipe Dream)
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int draw_priority)
{
	UINT8 zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };
	int offs;

	/* draw the sprites */
	for (offs = 0; offs < spriteram_size; offs += 8)
	{
		int data2 = spriteram[offs + 4] | (spriteram[offs + 5] << 8);
		int priority = (data2 >> 4) & 1;

		/* turns out the sprites are the same as in aerofgt.c */
		if ((data2 & 0x80) && priority == draw_priority)
		{
			int data0 = spriteram[offs + 0] | (spriteram[offs + 1] << 8);
			int data1 = spriteram[offs + 2] | (spriteram[offs + 3] << 8);
			int data3 = spriteram[offs + 6] | (spriteram[offs + 7] << 8);
			int code = data3 & 0xfff;
			int color = data2 & 0x0f;
			int y = (data0 & 0x1ff) - 6;
			int x = (data1 & 0x1ff) - 13;
			int yzoom = (data0 >> 12) & 15;
			int xzoom = (data1 >> 12) & 15;
			int zoomed = (xzoom | yzoom);
			int ytiles = ((data2 >> 12) & 7) + 1;
			int xtiles = ((data2 >> 8) & 7) + 1;
			int yflip = (data2 >> 15) & 1;
			int xflip = (data2 >> 11) & 1;
			int xt, yt;

			/* compute the zoom factor -- stolen from aerofgt.c */
			xzoom = 16 - zoomtable[xzoom] / 8;
			yzoom = 16 - zoomtable[yzoom] / 8;

			/* wrap around */
			if (x > Machine->visible_area.max_x) x -= 0x200;
			if (y > Machine->visible_area.max_y) y -= 0x200;

			/* normal case */
			if (!xflip && !yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[2], code, color, 0, 0,
									x + xt * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[2], code, color, 0, 0,
									x + xt * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* xflipped case */
			else if (xflip && !yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[2], code, color, 1, 0,
									x + (xtiles - 1 - xt) * 16, y + yt * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[2], code, color, 1, 0,
									x + (xtiles - 1 - xt) * xzoom, y + yt * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* yflipped case */
			else if (!xflip && yflip)
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[2], code, color, 0, 1,
									x + xt * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[2], code, color, 0, 1,
									x + xt * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}

			/* x & yflipped case */
			else
			{
				for (yt = 0; yt < ytiles; yt++)
					for (xt = 0; xt < xtiles; xt++, code++)
						if (!zoomed)
							drawgfx(bitmap, Machine->gfx[2], code, color, 1, 1,
									x + (xtiles - 1 - xt) * 16, y + (ytiles - 1 - yt) * 16, cliprect, TRANSPARENCY_PEN, 15);
						else
							drawgfxzoom(bitmap, Machine->gfx[2], code, color, 1, 1,
									x + (xtiles - 1 - xt) * xzoom, y + (ytiles - 1 - yt) * yzoom, cliprect, TRANSPARENCY_PEN, 15,
									0x1000 * xzoom, 0x1000 * yzoom);
			}
		}
	}
}



/*************************************
 *
 *	Main screen refresh
 *
 *************************************/

VIDEO_UPDATE( fromance )
{
	tilemap_set_scrollx(bg_tilemap, 0, scrollx[0]);
	tilemap_set_scrolly(bg_tilemap, 0, scrolly[0]);
	tilemap_set_scrollx(fg_tilemap, 0, scrollx[1]);
	tilemap_set_scrolly(fg_tilemap, 0, scrolly[1]);

	tilemap_draw(bitmap,cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap,cliprect, fg_tilemap, 0, 0);
}


VIDEO_UPDATE( pipedrm )
{
	/* there seems to be no logical mapping for the X scroll register -- maybe it's gone */
	tilemap_set_scrolly(bg_tilemap, 0, scrolly[1]);
	tilemap_set_scrolly(fg_tilemap, 0, scrolly[0]);

	tilemap_draw(bitmap,cliprect, bg_tilemap, 0, 0);
	tilemap_draw(bitmap,cliprect, fg_tilemap, 0, 0);

	draw_sprites(bitmap,cliprect, 0);
	draw_sprites(bitmap,cliprect, 1);
}
