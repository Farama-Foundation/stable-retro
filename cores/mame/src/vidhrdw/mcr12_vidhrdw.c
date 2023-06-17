/***************************************************************************

	Midway MCR-I/II system

	Journey is an MCR-II game with a MCR-III sprite board so it has it's
	own routines.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "mcr.h"


INT8 mcr12_sprite_xoffs;
INT8 mcr12_sprite_xoffs_flip;

static UINT8 *spritebitmap;
static UINT32 spritebitmap_width;
static UINT32 spritebitmap_height;

static UINT8 xtiles, ytiles;
static struct tilemap *bg_tilemap;



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void mcr1_get_bg_tile_info(int tile_index)
{
	SET_TILE_INFO(0, videoram[tile_index], 0, 0);
}


static void mcr2_get_bg_tile_info(int tile_index)
{
	int data = videoram[tile_index * 2] | (videoram[tile_index * 2 + 1] << 8);
	int code = data & 0x1ff;
	int color = (data >> 11) & 3;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((data >> 9) & 3));
}


static void twotigra_get_bg_tile_info(int tile_index)
{
	int data = videoram[tile_index] | (videoram[tile_index + 0x400] << 8);
	int code = data & 0x1ff;
	int color = (data >> 11) & 3;
	SET_TILE_INFO(0, code, color, TILE_FLIPYX((data >> 9) & 3));
}



/*************************************
 *
 *	Common video startup/shutdown
 *
 *************************************/

static int video_start_common(void)
{
	const struct GfxElement *gfx = Machine->gfx[1];

	/* allocate a dirty buffer */
	dirtybuffer = auto_malloc(videoram_size);
	if (!dirtybuffer)
		return 1;

	/* allocate a temporary bitmap for the sprite rendering */
	spritebitmap_width = Machine->drv->screen_width + 2 * 32;
	spritebitmap_height = Machine->drv->screen_height + 2 * 32;
	spritebitmap = auto_malloc(spritebitmap_width * spritebitmap_height);
	if (!spritebitmap)
		return 1;
	memset(spritebitmap, 0, spritebitmap_width * spritebitmap_height);

	/* if we're swapped in X/Y, the sprite data will be swapped */
	/* but that's not what we want, so we swap it back here */
	if (gfx && (Machine->orientation & ORIENTATION_SWAP_XY) && !(gfx->flags & GFX_SWAPXY))
	{
		UINT8 *base = gfx->gfxdata;
		int c, x, y;
		for (c = 0; c < gfx->total_elements; c++)
		{
			for (y = 0; y < gfx->height; y++)
				for (x = y; x < gfx->width; x++)
				{
					int temp = base[y * gfx->line_modulo + x];
					base[y * gfx->line_modulo + x] = base[x * gfx->line_modulo + y];
					base[x * gfx->line_modulo + y] = temp;
				}
			base += gfx->char_modulo;
		}
	}

	/* compute tile counts */
	xtiles = Machine->drv->screen_width / 16;
	ytiles = Machine->drv->screen_height / 16;
	return 0;
}


VIDEO_START( mcr1 )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(mcr1_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
	if (!bg_tilemap)
		return 1;
	
	/* handle the rest */
	return video_start_common();
}


VIDEO_START( mcr2 )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(mcr2_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
	if (!bg_tilemap)
		return 1;
	
	/* handle the rest */
	return video_start_common();
}


VIDEO_START( twotigra )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(twotigra_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
	if (!bg_tilemap)
		return 1;
	
	/* handle the rest */
	return video_start_common();
}


VIDEO_START( journey )
{
	/* initialize the background tilemap */
	bg_tilemap = tilemap_create(mcr2_get_bg_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 16,16, 32,30);
	if (!bg_tilemap)
		return 1;
	return 0;
}



/*************************************
 *
 *	Videoram writes
 *
 *************************************/

WRITE_HANDLER( mcr1_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset);
}


WRITE_HANDLER( mcr2_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset / 2);

	/* palette RAM is mapped into the upper 0x80 bytes here */
	if ((offset & 0x780) == 0x780)
	{
		/* bit 2 of the red component is taken from bit 0 of the address */
		int idx = (offset >> 1) & 0x3f;
		int r = ((offset & 1) << 2) + (data >> 6);
		int g = (data >> 0) & 7;
		int b = (data >> 3) & 7;

		/* up to 8 bits */
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 5) | (b << 2) | (b >> 1);

		palette_set_color(idx, r, g, b);
	}
}


WRITE_HANDLER( twotigra_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap, offset & 0x3ff);

	/* palette RAM is mapped into the upper 0x40 bytes of each bank */
	if ((offset & 0x3c0) == 0x3c0)
	{
		/* bit 2 of the red component is taken from bit 0 of the address */
		int idx = ((offset & 0x400) >> 5) | ((offset >> 1) & 0x1f);
		int r = ((offset & 1) << 2) + (data >> 6);
		int g = (data >> 0) & 7;
		int b = (data >> 3) & 7;

		/* up to 8 bits */
		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 5) | (b << 2) | (b >> 1);

		palette_set_color(idx, r, g, b);
	}
}



/*************************************
 *
 *	Sprite drawing
 *
 *************************************/

static void render_one_sprite(int code, int sx, int sy, int hflip, int vflip)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	UINT8 *src = gfx->gfxdata + gfx->char_modulo * code;
	int y, x;
	
	/* offset for the extra top/left area */
	sx += 32;
	sy += 32;

	/* adjust for vflip */
	if (vflip)
		src += 31 * gfx->line_modulo;

	/* loop over lines in the sprite */
	for (y = 0; y < 32; y++, sy++)
	{
		UINT8 *dst = spritebitmap + spritebitmap_width * sy + sx;

		/* redraw the line */
		if (!hflip)
		{
			for (x = 0; x < 32; x++)
				*dst++ |= *src++;
		}
		else
		{
			src += 32;
			for (x = 0; x < 32; x++)
				*dst++ |= *--src;
			src += 32;
		}

		/* adjust for vflip */
		if (vflip)
			src -= 2 * gfx->line_modulo;
	}
}



/*************************************
 *
 *	Common sprite update
 *
 *************************************/

static void mcr12_update_sprites(void)
{
	int offs;

	/* render the sprites into the bitmap, ORing together */
	for (offs = 0; offs < spriteram_size; offs += 4)
	{
		int code, x, y, sx, sy, xcount, ycount, xtile, ytile, hflip, vflip;

		/* skip if zero */
		if (spriteram[offs] == 0)
			continue;

		/* extract the bits of information */
		code = spriteram[offs + 1] & 0x3f;
		hflip = spriteram[offs + 1] & 0x40;
		vflip = spriteram[offs + 1] & 0x80;
		x = (spriteram[offs + 2] - 4) * 2;
		y = (240 - spriteram[offs]) * 2;

		/* apply cocktail mode */
		if (mcr_cocktail_flip)
		{
			hflip = !hflip;
			vflip = !vflip;
			x = 466 - x + mcr12_sprite_xoffs_flip;
			y = 450 - y;
		}
		else
			x += mcr12_sprite_xoffs;

		/* wrap and clip */
		if (x > Machine->visible_area.max_x)
			x -= 512;
		if (y > Machine->visible_area.max_y)
			y -= 512;
		if (x <= -32 || y <= -32)
			continue;

		/* draw the sprite into the sprite bitmap */
		render_one_sprite(code, x, y, hflip, vflip);

		/* determine which tiles we will overdraw with this sprite */
		sx = x / 16;
		sy = y / 16;
		xcount = (x & 15) ? 3 : 2;
		ycount = (y & 15) ? 3 : 2;

		/* loop over dirty tiles and set the sprite bit */
		for (ytile = sy; ytile < sy + ycount; ytile++)
			for (xtile = sx; xtile < sx + xcount; xtile++)
				if (xtile >= 0 && xtile < xtiles && ytile >= 0 && ytile < ytiles)
					dirtybuffer[32 * ytile + xtile] = 1;
	}
}



/*************************************
 *
 *	Sprite bitmap drawing
 *
 *************************************/

static void render_sprite_tile(struct mame_bitmap *bitmap, pen_t *pens, int tile_index)
{
	int sx = tile_index % 32;
	int sy = tile_index / 32;
	int x, y;
	
	/* skip if out of range */
	if (sx >= xtiles || sy >= ytiles)
		return;
	
	/* convert to pixel coordinates */
	sx *= 16;
	sy *= 16;

	/* draw any dirty scanlines from the VRAM directly */
	for (y = 0; y < 16; y++, sy++)
	{
		UINT8 *src = &spritebitmap[(sy + 32) * spritebitmap_width + (sx + 32)];

		/* redraw the sprite scanline, erasing as we go */
		for (x = 0; x < 16; x++)
		{
			int pixel = *src;
			if (pixel & 7)
				plot_pixel(bitmap, sx + x, sy, pens[pixel]);
			*src++ = 0;
		}
	}
}



/*************************************
 *
 *	Sprite rendering
 *
 *************************************/

static void mcr1_render_sprites(struct mame_bitmap *bitmap)
{
	int offs;

	/* first render them raw */
	mcr12_update_sprites();

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1; offs >= 0; offs--)
		if (dirtybuffer[offs])
		{
			render_sprite_tile(bitmap, &Machine->pens[16], offs);
			dirtybuffer[offs] = 0;
		}
}


static void mcr2_render_sprites(struct mame_bitmap *bitmap)
{
	int offs;

	/* first render them raw */
	mcr12_update_sprites();

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size / 2 - 1; offs >= 0; offs--)
		if (dirtybuffer[offs])
		{
			int tx = offs % 32;
			int ty = offs / 32;
			int attr;
			
			/* adjust for cocktail flip */
			if (mcr_cocktail_flip)
			{
				tx = xtiles - 1 - tx;
				ty = ytiles - 1 - ty;
			}

			/* lookup the attributes for the tile underneath to get the color */
			attr = videoram[(ty * 32 + tx) * 2 + 1];
			render_sprite_tile(bitmap, &Machine->pens[(attr & 0xc0) >> 2], offs);
			dirtybuffer[offs] = 0;
		}
}



/*************************************
 *
 *	Main refresh routines
 *
 *************************************/

VIDEO_UPDATE( mcr1 )
{
	/* update the flip state */
	tilemap_set_flip(bg_tilemap, mcr_cocktail_flip ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

	/* draw the background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* update the sprites and render them */
	mcr1_render_sprites(bitmap);
}


VIDEO_UPDATE( mcr2 )
{
	/* update the flip state */
	tilemap_set_flip(bg_tilemap, mcr_cocktail_flip ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

	/* draw the background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* update the sprites and render them */
	mcr2_render_sprites(bitmap);
}


VIDEO_UPDATE( journey )
{
	/* update the flip state */
	tilemap_set_flip(bg_tilemap, mcr_cocktail_flip ? (TILEMAP_FLIPX | TILEMAP_FLIPY) : 0);

	/* draw the background */
	tilemap_draw(bitmap, cliprect, bg_tilemap, 0, 0);

	/* draw the sprites */
	mcr3_update_sprites(bitmap, cliprect, 0x03, 0, 0, 0);
}
