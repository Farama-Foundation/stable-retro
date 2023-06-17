/***************************************************************************

	Midway MCR-68k system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


#define LOW_BYTE(x) ((x) & 0xff)


UINT8 mcr68_sprite_clip;
INT8 mcr68_sprite_xoffset;

static struct tilemap *bg_tilemap;
static struct tilemap *fg_tilemap;



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_bg_tile_info(int tile_index)
{
	int data = LOW_BYTE(videoram16[tile_index * 2]) | (LOW_BYTE(videoram16[tile_index * 2 + 1]) << 8);
	int code = (data & 0x3ff) | ((data >> 4) & 0xc00);
	int color = (~data >> 12) & 3;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((data >> 10) & 3));
	if (Machine->gfx[0]->total_elements < 0x1000)
		tile_info.priority = (data >> 15) & 1;
}


static void zwackery_get_bg_tile_info(int tile_index)
{
	int data = videoram16[tile_index];
	int color = (data >> 13) & 7;
	SET_TILE_INFO(0, data & 0x3ff, color, TILE_FLIPYX((data >> 11) & 3));
}


static void zwackery_get_fg_tile_info(int tile_index)
{
	int data = videoram16[tile_index];
	int color = (data >> 13) & 7;
	SET_TILE_INFO(2, data & 0x3ff, color, TILE_FLIPYX((data >> 11) & 3));
	tile_info.priority = (color != 0);
}



/*************************************
 *
 *	Video startup
 *
 *************************************/

VIDEO_START( mcr68 )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(get_bg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16, 32,32);
	if (!bg_tilemap)
		return 1;
	tilemap_set_transparent_pen(bg_tilemap, 0);
	return 0;
}


VIDEO_START( zwackery )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(zwackery_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,32);
	if (!bg_tilemap)
		return 1;

	/* initialize the foreground tilemap */
	fg_tilemap = tilemap_create(zwackery_get_fg_tile_info, tilemap_scan_rows, TILEMAP_TRANSPARENT, 16,16, 32,32);
	if (!fg_tilemap)
		return 1;
	tilemap_set_transparent_pen(fg_tilemap, 0);

	return 0;
}



/*************************************
 *
 *	Zwackery color data conversion
 *
 *************************************/

PALETTE_INIT( zwackery )
{
	const UINT8 *colordatabase = (const UINT8 *)memory_region(REGION_GFX3);
	struct GfxElement *gfx0 = Machine->gfx[0];
	struct GfxElement *gfx2 = Machine->gfx[2];
	int code, y, x;

	/* "colorize" each code */
	for (code = 0; code < gfx0->total_elements; code++)
	{
		const UINT8 *coldata = colordatabase + code * 32;
		UINT8 *gfxdata0 = gfx0->gfxdata + code * gfx0->char_modulo;
		UINT8 *gfxdata2 = gfx2->gfxdata + code * gfx2->char_modulo;

		/* assume 16 rows */
		for (y = 0; y < 16; y++)
		{
			UINT8 *gd0 = gfxdata0;
			UINT8 *gd2 = gfxdata2;

			/* 16 columns */
			for (x = 0; x < 16; x++, gd0++, gd2++)
			{
				int coloffs = (y & 0x0c) | ((x >> 2) & 0x03);
				int pen0 = coldata[coloffs * 2 + 0];
				int pen1 = coldata[coloffs * 2 + 1];
				int tp0, tp1;

				/* every 4 pixels gets its own foreground/background colors */
				*gd0 = *gd0 ? pen1 : pen0;

				/* for gfx 2, we convert all low-priority pens to 0 */
				tp0 = (pen0 & 0x80) ? pen0 : 0;
				tp1 = (pen1 & 0x80) ? pen1 : 0;
				*gd2 = *gd2 ? tp1 : tp0;
			}

			/* advance */
			gfxdata0 += gfx0->line_modulo;
			gfxdata2 += gfx2->line_modulo;
		}
	}
}



/*************************************
 *
 *	Palette RAM writes
 *
 *************************************/

WRITE16_HANDLER( mcr68_paletteram_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = (newword >> 6) & 7;
	b = (newword >> 3) & 7;
	g = (newword >> 0) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_set_color(offset, r, g, b);
}


WRITE16_HANDLER( zwackery_paletteram_w )
{
	int newword, r, g, b;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	r = (~newword >> 10) & 31;
	b = (~newword >> 5) & 31;
	g = (~newword >> 0) & 31;

	/* up to 8 bits */
	r = (r << 3) | (r >> 2);
	g = (g << 3) | (g >> 2);
	b = (b << 3) | (b >> 2);

	palette_set_color(offset, r, g, b);
}



/*************************************
 *
 *	Video RAM writes
 *
 *************************************/

WRITE16_HANDLER( mcr68_videoram_w )
{
	COMBINE_DATA(&videoram16[offset]);
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);
}


WRITE16_HANDLER( zwackery_videoram_w )
{
	COMBINE_DATA(&videoram16[offset]);
	tilemap_mark_tile_dirty(bg_tilemap, offset);
	tilemap_mark_tile_dirty(fg_tilemap, offset);
}


WRITE16_HANDLER( zwackery_spriteram_w )
{
	/* yech -- Zwackery relies on the upper 8 bits of a spriteram read being $ff! */
	/* to make this happen we always write $ff in the upper 8 bits */
	COMBINE_DATA(&spriteram16[offset]);
	spriteram16[offset] |= 0xff00;
}



/*************************************
 *
 *	Sprite update
 *
 *************************************/

static void mcr68_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	struct rectangle sprite_clip = Machine->visible_area;
	int offs;

	/* adjust for clipping */
	sprite_clip.min_x += mcr68_sprite_clip;
	sprite_clip.max_x -= mcr68_sprite_clip;
	sect_rect(&sprite_clip, cliprect);

	fillbitmap(priority_bitmap,1,&sprite_clip);

	/* loop over sprite RAM */
	for (offs = spriteram_size / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, flags;

		flags = LOW_BYTE(spriteram16[offs + 1]);
		code = LOW_BYTE(spriteram16[offs + 2]) + 256 * ((flags >> 3) & 0x01) + 512 * ((flags >> 6) & 0x03);

		/* skip if zero */
		if (code == 0)
			continue;

		/* also skip if this isn't the priority we're drawing right now */
		if (((flags >> 2) & 1) != priority)
			continue;

		/* extract the bits of information */
		color = ~flags & 0x03;
		flipx = flags & 0x10;
		flipy = flags & 0x20;
		x = LOW_BYTE(spriteram16[offs + 3]) * 2 + mcr68_sprite_xoffset;
		y = (241 - LOW_BYTE(spriteram16[offs])) * 2;

		/* allow sprites to clip off the left side */
		if (x > 0x1f0) x -= 0x200;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				&sprite_clip, TRANSPARENCY_PENS, 0x0101, 0x00);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				&sprite_clip, TRANSPARENCY_PENS, 0xfeff, 0x02);
	}
}


static void zwackery_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int priority)
{
	int offs;

	fillbitmap(priority_bitmap,1,cliprect);

	/* loop over sprite RAM */
	for (offs = spriteram_size / 2 - 4;offs >= 0;offs -= 4)
	{
		int code, color, flipx, flipy, x, y, flags;

		/* get the code and skip if zero */
		code = LOW_BYTE(spriteram16[offs + 2]);
		if (code == 0)
			continue;

		/* extract the flag bits and determine the color */
		flags = LOW_BYTE(spriteram16[offs + 1]);
		color = ((~flags >> 2) & 0x0f) | ((flags & 0x02) << 3);

		/* for low priority, draw everything but color 7 */
		if (!priority)
		{
			if (color == 7)
				continue;
		}

		/* for high priority, only draw color 7 */
		else
		{
			if (color != 7)
				continue;
		}

		/* determine flipping and coordinates */
		flipx = ~flags & 0x40;
		flipy = flags & 0x80;
		x = (231 - LOW_BYTE(spriteram16[offs + 3])) * 2;
		y = (241 - LOW_BYTE(spriteram16[offs])) * 2;

		if (x <= -32) x += 512;

		/* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites. */

		/* first draw the sprite, visible */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				cliprect, TRANSPARENCY_PENS, 0x0101, 0x00);

		/* then draw the mask, behind the background but obscuring following sprites */
		pdrawgfx(bitmap, Machine->gfx[1], code, color, flipx, flipy, x, y,
				cliprect, TRANSPARENCY_PENS, 0xfeff, 0x02);
	}
}



/*************************************
 *
 *	General MCR/68k update
 *
 *************************************/

VIDEO_UPDATE( mcr68 )
{
	/* draw the background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, TILEMAP_IGNORE_TRANSPARENCY | 0, 0);
	tilemap_draw(bitmap, cliprect, bg_tilemap, TILEMAP_IGNORE_TRANSPARENCY | 1, 0);

	/* draw the low-priority sprites */
	mcr68_update_sprites(bitmap, cliprect, 0);

    /* redraw tiles with priority over sprites */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 1, 0);

	/* draw the high-priority sprites */
	mcr68_update_sprites(bitmap, cliprect, 1);
}


VIDEO_UPDATE( zwackery )
{
	/* draw the background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw the low-priority sprites */
	zwackery_update_sprites(bitmap, cliprect, 0);

    /* redraw tiles with priority over sprites */
	tilemap_draw(bitmap, cliprect, fg_tilemap, 1, 0);

	/* draw the high-priority sprites */
	zwackery_update_sprites(bitmap, cliprect, 1);
}
