#include "driver.h"
#include "vidhrdw/generic.h"


data8_t *mappy_videoram;
data8_t *mappy_spriteram;

static data8_t mappy_scroll;
static struct tilemap *bg_tilemap;

static struct mame_bitmap *sprite_bitmap;


/***************************************************************************

  Convert the color PROMs.

  All games except Phozon have one 32x8 palette PROM and two 256x4 color
  lookup table PROMs (one for characters, one for sprites), except todruaga
  which has a larger 1024x4 PROM for sprites.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The way how the lookup tables are mapped to palette colors, and priority
  handling, are controlled by a PAL (SPV-5 in Super Pacman, MPI-4 in Mappy),
  so the two hardwares work differently.
  Super Pacman has a special "super priority" for sprite colors, allowing
  one pen to be over high priority tiles (used by Pac & Pal for ghost eyes),
  which isn't present in Mappy.

***************************************************************************/

PALETTE_INIT( superpac )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* characters */
	for (i = 0; i < 64*4; i++)
		colortable[i] = (color_prom[i] & 0x0f) ^ 0x1f;

	/* sprites */
	for (i = 64*4; i < 128*4; i++)
		colortable[i] = color_prom[i] & 0x0f;
}

PALETTE_INIT( mappy )
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* characters */
	for (i = 0*4;i < 64*4;i++)
		colortable[i] = (color_prom[i^3] & 0x0f) + 0x10;

	/* sprites */
	for (i = 64*4;i < Machine->drv->color_table_len;i++)
		colortable[i] = color_prom[i] & 0x0f;
}


/***************************************************************************

  In Phozon, the palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( phozon )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0; i < Machine->drv->total_colors; i++){
		int bit0,bit1,bit2,bit3,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[i + 0x100] >> 0) & 0x01;
		bit1 = (color_prom[i + 0x100] >> 1) & 0x01;
		bit2 = (color_prom[i + 0x100] >> 2) & 0x01;
		bit3 = (color_prom[i + 0x100] >> 3) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[i + 0x200] >> 0) & 0x01;
		bit1 = (color_prom[i + 0x200] >> 1) & 0x01;
		bit2 = (color_prom[i + 0x200] >> 2) & 0x01;
		bit3 = (color_prom[i + 0x200] >> 3) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	color_prom += 0x300;
	/* color_prom now points to the beginning of the lookup table */

	/* characters */
	for (i = 0; i < TOTAL_COLORS(0); i++)
		COLOR(0,i) = (*(color_prom++) & 0x0f);
	/* sprites */
	for (i = 0; i < TOTAL_COLORS(1); i++)
		COLOR(1,i) = (*(color_prom++) & 0x0f) + 0x10;
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/* convert from 32x32 to 36x28 */
static UINT32 superpac_tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	int offs;

	row += 2;
	col -= 2;
	if (col & 0x20)
		offs = row + ((col & 0x1f) << 5);
	else
		offs = col + (row << 5);

	return offs;
}

/* tilemap is a composition of a 32x60 scrolling portion and two 2x28 fixed portions on the sides */
static UINT32 mappy_tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	int offs;

	col -= 2;
	if (col & 0x20)
	{
		/* in the following code, note the +2 followed by & 0x0f. This causes unintuitive
		   mapping from logical to hardware coordinates, which is true to the hardware.
		   Not doing it that way would cause missing tiles in motos and todruaga */
		if (row & 0x20)
			offs = 0x7ff;	/* outside visible area */
		else
			offs = ((row + 2) & 0x0f) + (row & 0x10) + ((col & 3) << 5) + 0x780;
	}
	else
		offs = col + (row << 5);

	return offs;
}

static void superpac_get_tile_info(int tile_index)
{
	unsigned char attr = mappy_videoram[tile_index + 0x400];
	tile_info.priority = (attr & 0x40) >> 6;
	SET_TILE_INFO(
			0,
			mappy_videoram[tile_index],
			attr & 0x3f,
			0)
}

static void phozon_get_tile_info(int tile_index)
{
	unsigned char attr = mappy_videoram[tile_index + 0x400];
	tile_info.priority = (attr & 0x40) >> 6;
	SET_TILE_INFO(
			0,
			mappy_videoram[tile_index] + ((attr & 0x80) << 1),
			attr & 0x3f,
			0)
}

static void mappy_get_tile_info(int tile_index)
{
	unsigned char attr = mappy_videoram[tile_index + 0x800];
	tile_info.priority = (attr & 0x40) >> 6;
	SET_TILE_INFO(
			0,
			mappy_videoram[tile_index],
			attr & 0x3f,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( superpac )
{
	bg_tilemap = tilemap_create(superpac_get_tile_info,superpac_tilemap_scan,TILEMAP_TRANSPARENT_COLOR,8,8,36,28);
	sprite_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);

	if (!bg_tilemap || !sprite_bitmap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 31);

	spriteram = mappy_spriteram + 0x780;
	spriteram_2 = spriteram + 0x800;
	spriteram_3 = spriteram_2 + 0x800;

	return 0;
}

VIDEO_START( phozon )
{
	bg_tilemap = tilemap_create(phozon_get_tile_info,superpac_tilemap_scan,TILEMAP_TRANSPARENT_COLOR,8,8,36,28);

	if (!bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 15);

	spriteram = mappy_spriteram + 0x780;
	spriteram_2 = spriteram + 0x800;
	spriteram_3 = spriteram_2 + 0x800;

	return 0;
}

VIDEO_START( mappy )
{
	bg_tilemap = tilemap_create(mappy_get_tile_info,mappy_tilemap_scan,TILEMAP_TRANSPARENT_COLOR,8,8,36,60);

	if (!bg_tilemap)
		return 1;

	tilemap_set_transparent_pen(bg_tilemap, 31);
	tilemap_set_scroll_cols(bg_tilemap, 36);

	spriteram = mappy_spriteram + 0x780;
	spriteram_2 = spriteram + 0x800;
	spriteram_3 = spriteram_2 + 0x800;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( superpac_videoram_r )
{
	return mappy_videoram[offset];
}

WRITE_HANDLER( superpac_videoram_w )
{
	if (mappy_videoram[offset] != data)
	{
		mappy_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
	}
}

READ_HANDLER( mappy_videoram_r )
{
	return mappy_videoram[offset];
}

WRITE_HANDLER( mappy_videoram_w )
{
	if (mappy_videoram[offset] != data)
	{
		mappy_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset & 0x7ff);
	}
}

READ_HANDLER( mappy_spriteram_r )
{
    return mappy_spriteram[offset];
}

WRITE_HANDLER( mappy_spriteram_w )
{
	mappy_spriteram[offset] = data;
}

WRITE_HANDLER( superpac_flipscreen_w )
{
	flip_screen_set(data & 1);
}

READ_HANDLER( superpac_flipscreen_r )
{
	flip_screen_set(1);
	return 0xff;
}

WRITE_HANDLER( mappy_scroll_w )
{
	mappy_scroll = offset >> 3;
}



/***************************************************************************

  Display refresh

***************************************************************************/

/* also used by toypop.c */
void mappy_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect, int xoffs, int yoffs, int trans_color )
{
	int offs;

	for (offs = 0;offs < 0x80;offs += 2)
	{
		/* is it on? */
		if ((spriteram_3[offs+1] & 2) == 0)
		{
			static int gfx_offs[2][2] =
			{
				{ 0, 1 },
				{ 2, 3 }
			};
			int sprite = spriteram[offs];
			int color = spriteram[offs+1];
			int sx = spriteram_2[offs+1] + 0x100 * (spriteram_3[offs+1] & 1) - 40 + xoffs;
			int sy = 256 - spriteram_2[offs] + yoffs + 1;	/* sprites are buffered and delayed by one scanline */
			int flipx = (spriteram_3[offs] & 0x01);
			int flipy = (spriteram_3[offs] & 0x02) >> 1;
			int sizex = (spriteram_3[offs] & 0x04) >> 2;
			int sizey = (spriteram_3[offs] & 0x08) >> 3;
			int x,y;

			sprite &= ~sizex;
			sprite &= ~(sizey << 1);

			if (flip_screen)
			{
				flipx ^= 1;
				flipy ^= 1;
			}

			sy -= 16 * sizey;
			sy = (sy & 0xff) - 32;	/* fix wraparound */

			for (y = 0;y <= sizey;y++)
			{
				for (x = 0;x <= sizex;x++)
				{
					drawgfx(bitmap,Machine->gfx[1],
						sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
						color,
						flipx,flipy,
						sx + 16*x,sy + 16*y,
						cliprect,TRANSPARENCY_COLOR,trans_color);
				}
			}
		}
	}
}


static void phozon_draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;

	for (offs = 0;offs < 0x80;offs += 2)
	{
		/* is it on? */
		if ((spriteram_3[offs+1] & 2) == 0)
		{
			static int size[4] = { 1, 0, 3, 0 };	/* 16, 8, 32 pixels; fourth combination unused? */
			static int gfx_offs[4][4] =
			{
				{ 0, 1, 4, 5 },
				{ 2, 3, 6, 7 },
				{ 8, 9,12,13 },
				{10,11,14,15 }
			};
			int sprite = (spriteram[offs] << 2) | ((spriteram_3[offs] & 0xc0) >> 6);
			int color = spriteram[offs+1] & 0x3f;
			int sx = spriteram_2[offs+1] + 0x100 * (spriteram_3[offs+1] & 1) - 69;
			int sy = 256 - spriteram_2[offs];
			int flipx = (spriteram_3[offs] & 0x01);
			int flipy = (spriteram_3[offs] & 0x02) >> 1;
			int sizex = size[(spriteram_3[offs] & 0x0c) >> 2];
			int sizey = size[(spriteram_3[offs] & 0x30) >> 4];
			int x,y;

			if (flip_screen)
			{
				flipx ^= 1;
				flipy ^= 1;
			}

			sy -= 8 * sizey;
			sy = (sy & 0xff) - 32;	/* fix wraparound */

			for (y = 0;y <= sizey;y++)
			{
				for (x = 0;x <= sizex;x++)
				{
					drawgfx(bitmap,Machine->gfx[1],
						sprite + gfx_offs[y ^ (sizey * flipy)][x ^ (sizex * flipx)],
						color,
						flipx,flipy,
						sx + 8*x,sy + 8*y,
						cliprect,TRANSPARENCY_COLOR,31);
				}
			}
		}
	}
}


VIDEO_UPDATE( superpac )
{
	int x,y;

	tilemap_draw(bitmap,cliprect,bg_tilemap,0|TILEMAP_IGNORE_TRANSPARENCY,0);
	tilemap_draw(bitmap,cliprect,bg_tilemap,1|TILEMAP_IGNORE_TRANSPARENCY,0);

	fillbitmap(sprite_bitmap,15,cliprect);
	mappy_draw_sprites(sprite_bitmap,cliprect,0,0,15);
	copybitmap(bitmap,sprite_bitmap,0,0,0,0,cliprect,TRANSPARENCY_PEN,15);

	/* Redraw the high priority characters */
	tilemap_draw(bitmap,cliprect,bg_tilemap,1,0);

	/* sprite color 0 still has priority over that (ghost eyes in Pac 'n Pal) */
	for (y = 0;y < sprite_bitmap->height;y++)
	{
		for (x = 0;x < sprite_bitmap->width;x++)
		{
			if (read_pixel(sprite_bitmap,x,y) == 0)
				plot_pixel(bitmap,x,y,0);
		}
	}
}

VIDEO_UPDATE( phozon )
{
	/* flip screen control is embedded in RAM */
	flip_screen_set(mappy_spriteram[0x1f7f-0x800] & 1);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0|TILEMAP_IGNORE_TRANSPARENCY,0);
	tilemap_draw(bitmap,cliprect,bg_tilemap,1|TILEMAP_IGNORE_TRANSPARENCY,0);

	phozon_draw_sprites(bitmap,cliprect);

	/* Redraw the high priority characters */
	tilemap_draw(bitmap,cliprect,bg_tilemap,1,0);
}

VIDEO_UPDATE( mappy )
{
	int offs;

	for (offs = 2;offs < 34;offs++)
		tilemap_set_scrolly(bg_tilemap,offs,mappy_scroll);

	tilemap_draw(bitmap,cliprect,bg_tilemap,0|TILEMAP_IGNORE_TRANSPARENCY,0);
	tilemap_draw(bitmap,cliprect,bg_tilemap,1|TILEMAP_IGNORE_TRANSPARENCY,0);

	mappy_draw_sprites(bitmap,cliprect,0,0,15);

	/* Redraw the high priority characters */
	tilemap_draw(bitmap,cliprect,bg_tilemap,1,0);
}
