/***************************************************************************

  vidhrdw/rpunch.c

  Functions to emulate the video hardware of the machine.

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define BITMAP_WIDTH	304
#define BITMAP_HEIGHT	224
#define BITMAP_XOFFSET	4


/*************************************
 *
 *	Statics
 *
 *************************************/

data16_t *rpunch_bitmapram;
size_t rpunch_bitmapram_size;
static UINT32 *rpunch_bitmapsum;

int rpunch_sprite_palette;

static struct tilemap *background[2];

static data16_t videoflags;
static UINT8 crtc_register;
static void *crtc_timer;
static UINT8 bins, gins;


/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_bg0_tile_info(int tile_index)
{
	int data = videoram16[tile_index];
	int code;
	if (videoflags & 0x0400)	code = (data & 0x0fff) | 0x2000;
	else						code = (data & 0x1fff);

	SET_TILE_INFO(
			0,
			code,
			((videoflags & 0x0010) >> 1) | ((data >> 13) & 7),
			0)
}

static void get_bg1_tile_info(int tile_index)
{
	int data = videoram16[videoram_size / 4 + tile_index];
	int code;
	if (videoflags & 0x0800)	code = (data & 0x0fff) | 0x2000;
	else						code = (data & 0x1fff);

	SET_TILE_INFO(
			1,
			code,
			((videoflags & 0x0020) >> 2) | ((data >> 13) & 7),
			0)
}


/*************************************
 *
 *	Video system start
 *
 *************************************/

static void crtc_interrupt_gen(int param)
{
	cpu_set_irq_line(0, 1, HOLD_LINE);
	if (param != 0)
		timer_adjust(crtc_timer, TIME_IN_HZ(Machine->drv->frames_per_second * param), 0, TIME_IN_HZ(Machine->drv->frames_per_second * param));
}


VIDEO_START( rpunch )
{
	int i;

	/* allocate tilemaps for the backgrounds and a bitmap for the direct-mapped bitmap */
	background[0] = tilemap_create(get_bg0_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,     8,8,64,64);
	background[1] = tilemap_create(get_bg1_tile_info,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,64);

	/* allocate a bitmap sum */
	rpunch_bitmapsum = auto_malloc(BITMAP_HEIGHT * sizeof(UINT32));

	/* if anything failed, clean up and return an error */
	if (!background[0] || !background[1] || !rpunch_bitmapsum)
		return 1;

	/* configure the tilemaps */
	tilemap_set_transparent_pen(background[1],15);

	/* reset the sums and bitmap */
	for (i = 0; i < BITMAP_HEIGHT; i++)
		rpunch_bitmapsum[i] = (BITMAP_WIDTH/4) * 0xffff;
	if (rpunch_bitmapram)
		memset(rpunch_bitmapram, 0xff, rpunch_bitmapram_size);

	/* reset the timer */
	crtc_timer = timer_alloc(crtc_interrupt_gen);
	return 0;
}



/*************************************
 *
 *	Write handlers
 *
 *************************************/

WRITE16_HANDLER( rpunch_bitmap_w )
{
	if (rpunch_bitmapram)
	{
		int oldword = rpunch_bitmapram[offset];
		int newword = oldword;
		COMBINE_DATA(&newword);

		if (oldword != newword)
		{
			int row = offset / 128;
			int col = 4 * (offset % 128) - BITMAP_XOFFSET;

			rpunch_bitmapram[offset] = data;
			if (row < BITMAP_HEIGHT && col >= 0 && col < BITMAP_WIDTH)
				rpunch_bitmapsum[row] += newword - oldword;
		}
	}
}


WRITE16_HANDLER( rpunch_videoram_w )
{
	int oldword = videoram16[offset];
	int newword = oldword;
	COMBINE_DATA(&newword);

	if (oldword != newword)
	{
		int tilemap = offset >> 12;
		int tile_index = offset & 0xfff;

		videoram16[offset] = newword;
		tilemap_mark_tile_dirty(background[tilemap],tile_index);
	}
}


WRITE16_HANDLER( rpunch_videoreg_w )
{
	int oldword = videoflags;
	COMBINE_DATA(&videoflags);

	if (videoflags != oldword)
	{
		/* invalidate tilemaps */
		if ((oldword ^ videoflags) & 0x0410)
			tilemap_mark_all_tiles_dirty(background[0]);
		if ((oldword ^ videoflags) & 0x0820)
			tilemap_mark_all_tiles_dirty(background[1]);
	}
}


WRITE16_HANDLER( rpunch_scrollreg_w )
{
	if (ACCESSING_LSB && ACCESSING_MSB)
		switch (offset)
		{
			case 0:
				tilemap_set_scrolly(background[0], 0, data & 0x1ff);
				break;

			case 1:
				tilemap_set_scrollx(background[0], 0, data & 0x1ff);
				break;

			case 2:
				tilemap_set_scrolly(background[1], 0, data & 0x1ff);
				break;

			case 3:
				tilemap_set_scrollx(background[1], 0, data & 0x1ff);
				break;
		}
}


WRITE16_HANDLER( rpunch_crtc_data_w )
{
	if (ACCESSING_LSB)
	{
		data &= 0xff;
		switch (crtc_register)
		{
			/* only register we know about.... */
			case 0x0b:
				timer_adjust(crtc_timer, cpu_getscanlinetime(Machine->visible_area.max_y + 1), (data == 0xc0) ? 2 : 1, 0);
				break;

			default:
				log_cb(RETRO_LOG_DEBUG, LOGPRE "CRTC register %02X = %02X\n", crtc_register, data & 0xff);
				break;
		}
	}
}


WRITE16_HANDLER( rpunch_crtc_register_w )
{
	if (ACCESSING_LSB)
		crtc_register = data & 0xff;
}


WRITE16_HANDLER( rpunch_ins_w )
{
	if (ACCESSING_LSB)
	{
		if (offset == 0)
		{
			gins = data & 0x3f;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "GINS = %02X\n", data & 0x3f);
		}
		else
		{
			bins = data & 0x3f;
			log_cb(RETRO_LOG_DEBUG, LOGPRE "BINS = %02X\n", data & 0x3f);
		}
	}
}


/*************************************
 *
 *	Sprite routines
 *
 *************************************/

static void draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int start, int stop)
{
	int offs;

	start *= 4;
	stop *= 4;

	/* draw the sprites */
	for (offs = start; offs < stop; offs += 4)
	{
		int data1 = spriteram16[offs + 1];
		int code = data1 & 0x7ff;

		if (code < 0x600 && code != 0)
		{
			int data0 = spriteram16[offs + 0];
			int data2 = spriteram16[offs + 2];
			int x = (data2 & 0x1ff) + 8;
			int y = 513 - (data0 & 0x1ff);
			int xflip = data1 & 0x1000;
			int yflip = data1 & 0x0800;
			int color = ((data1 >> 13) & 7) | ((videoflags & 0x0040) >> 3);

			if (x >= BITMAP_WIDTH) x -= 512;
			if (y >= BITMAP_HEIGHT) y -= 512;

			drawgfx(bitmap, Machine->gfx[2],
					code, color + (rpunch_sprite_palette / 16), xflip, yflip, x, y, cliprect, TRANSPARENCY_PEN, 15);
		}
	}
}


/*************************************
 *
 *	Bitmap routines
 *
 *************************************/

static void draw_bitmap(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	pen_t *pens = &Machine->pens[512 + (videoflags & 15) * 16];
	int x, y;

	/* draw any non-transparent scanlines from the VRAM directly */
	for (y = 0; y < BITMAP_HEIGHT; y++)
		if (y >= cliprect->min_y && y <= cliprect->max_y)
			if (rpunch_bitmapsum[y] != (BITMAP_WIDTH/4) * 0xffff)
			{
				data16_t *src = &rpunch_bitmapram[y * 128 + BITMAP_XOFFSET/4];
				UINT8 scanline[BITMAP_WIDTH], *dst = scanline;

				/* extract the scanline */
				for (x = 0; x < BITMAP_WIDTH/4; x++)
				{
					int data = *src++;

					dst[0] = data >> 12;
					dst[1] = (data >> 8) & 15;
					dst[2] = (data >> 4) & 15;
					dst[3] = data & 15;
					dst += 4;
				}
				draw_scanline8(bitmap, 0, y, BITMAP_WIDTH, scanline, pens, 15);
			}
}


/*************************************
 *
 *	Main screen refresh
 *
 *************************************/

VIDEO_UPDATE( rpunch )
{
	int effbins;

	/* this seems like the most plausible explanation */
	effbins = (bins > gins) ? gins : bins;

	tilemap_draw(bitmap,cliprect, background[0], 0,0);
	draw_sprites(bitmap,cliprect, 0, effbins);
	tilemap_draw(bitmap,cliprect, background[1], 0,0);
	draw_sprites(bitmap,cliprect, effbins, gins);
	if (rpunch_bitmapram)
		draw_bitmap(bitmap,cliprect);
}
