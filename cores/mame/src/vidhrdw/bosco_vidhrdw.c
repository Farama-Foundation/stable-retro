/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define MAX_STARS 250
#define STARS_COLOR_BASE 32

static unsigned int stars_scrollx;
static unsigned int stars_scrolly;
static int bosco_starcontrol,bosco_starblink[2];

static struct tilemap *bg_tilemap,*fg_tilemap;
data8_t *bosco_radarattr;

unsigned char *bosco_sharedram;
READ_HANDLER( bosco_sharedram1_r );
WRITE_HANDLER( bosco_sharedram1_w );
READ_HANDLER( bosco_sharedram2_r );
WRITE_HANDLER( bosco_sharedram2_w );

READ_HANDLER( bosco_sharedram1_r )
{
	return bosco_sharedram[offset];
}

WRITE_HANDLER( bosco_sharedram1_w )
{
	bosco_sharedram[offset] = data;
}

READ_HANDLER( bosco_sharedram2_r )
{
	return bosco_radarattr[offset];
}

WRITE_HANDLER( bosco_sharedram2_w )
{
	bosco_radarattr[offset] = data;
}



struct star
{
	int x,y,col,set;
};
static struct star stars[MAX_STARS];
static int total_stars;

#define VIDEO_RAM_SIZE 0x400

data8_t *bosco_videoram;
static data8_t *bosco_radarx,*bosco_radary;


PALETTE_INIT( bosco )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;


		bit0 = ((*color_prom) >> 0) & 0x01;
		bit1 = ((*color_prom) >> 1) & 0x01;
		bit2 = ((*color_prom) >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = ((*color_prom) >> 3) & 0x01;
		bit1 = ((*color_prom) >> 4) & 0x01;
		bit2 = ((*color_prom) >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = ((*color_prom) >> 6) & 0x01;
		bit2 = ((*color_prom) >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette_set_color(i,r,g,b);
		color_prom++;
	}

	/* characters / sprites */
	for (i = 0;i < 64*4;i++)
	{
		colortable[i] = (color_prom[i] & 0x0f) + 0x10;	/* chars */
		colortable[i+64*4] = (color_prom[i] & 0x0f);	/* sprites */
	}

	/* bullets lookup table */
	/* they use colors 28-31, I think - PAL 5A controls it */
	for (i = 0;i < 4;i++)
	{
		COLOR(2,i) = 31-i;
		COLOR(2,i+4) = 0;	/* transparent */
	}

	/* now the stars */
	for (i = 0;i < 64;i++)
	{
		int bits,r,g,b;
		int map[4] = { 0x00, 0x47, 0x97, 0xde };

		bits = (i >> 0) & 0x03;
		r = map[bits];
		bits = (i >> 2) & 0x03;
		g = map[bits];
		bits = (i >> 4) & 0x03;
		b = map[bits];

		palette_set_color(i + 32,r,g,b);
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

/* the video RAM has space for 32x32 tiles and is only partially used for the radar */
static UINT32 fg_tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return col + (row << 5);
}


static void get_tile_info(int ram_offs,int tile_index)
{
	UINT8 attr = bosco_videoram[ram_offs + tile_index + 0x800];
	tile_info.priority = (attr & 0x20) >> 5;
	SET_TILE_INFO(
			0,
			bosco_videoram[ram_offs + tile_index],
			attr & 0x3f,
			TILE_FLIPYX(attr >> 6) ^ TILE_FLIPX)
}

static void bg_get_tile_info(int tile_index)
{
	get_tile_info(0x400,tile_index);
}

static void fg_get_tile_info(int tile_index)
{
	get_tile_info(0x000,tile_index);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( bosco )
{
	int generator;
	int x,y;
	int set = 0;

	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);
	fg_tilemap = tilemap_create(fg_get_tile_info,fg_tilemap_scan,  TILEMAP_OPAQUE,8,8, 8,32);

	if (!bg_tilemap || !fg_tilemap)
		return 1;

	tilemap_set_scrolldx(bg_tilemap,3,3);

	spriteram_size = 0x0c;
	spriteram = bosco_videoram + 0x03d4;
	spriteram_2 = spriteram + 0x0800;
	bosco_radarx = bosco_videoram + 0x03f0;
	bosco_radary = bosco_radarx + 0x0800;

	/* precalculate the star background */
	/* this comes from the Galaxian hardware, Bosconian is probably different */
	total_stars = 0;
	generator = 0;

	for (x = 255;x >= 0;x--)
	{
		for (y = 511;y >= 0;y--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (x >= Machine->visible_area.min_x &&
					x <= Machine->visible_area.max_x &&
					((~generator >> 16) & 1) &&
					(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].col = Machine->pens[color + STARS_COLOR_BASE];
					stars[total_stars].set = set;
					if (++set > 3)
						set = 0;

					total_stars++;
				}
			}
		}
	}

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( bosco_videoram_r )
{
	return bosco_videoram[offset];
}

WRITE_HANDLER( bosco_videoram_w )
{
	if (bosco_videoram[offset] != data)
	{
		bosco_videoram[offset] = data;
		if (offset & 0x400)
			tilemap_mark_tile_dirty(bg_tilemap,offset & 0x3ff);
		else
			tilemap_mark_tile_dirty(fg_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( bosco_scrollx_w )
{
	tilemap_set_scrollx(bg_tilemap,0,data);
}

WRITE_HANDLER( bosco_scrolly_w )
{
	tilemap_set_scrolly(bg_tilemap,0,data);
}

WRITE_HANDLER( bosco_starcontrol_w )
{
	bosco_starcontrol = data;
}

WRITE_HANDLER( bosco_starblink_w )
{
	bosco_starblink[offset] = data & 1;
}

WRITE_HANDLER( bosco_starclr_w )
{
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;

	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		int sx = spriteram[offs + 1] - 1;
		int sy = 240 - spriteram_2[offs];
		int flipx = spriteram[offs] & 1;
		int flipy = spriteram[offs] & 2;
		if (flip_screen) sx += 32-2;

		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] & 0xfc) >> 2,
				spriteram_2[offs + 1] & 0x3f,
				flipx,flipy,
				sx,sy,
				cliprect,TRANSPARENCY_COLOR,0x0f);
	}
}


static void draw_bullets( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;

	for (offs = 4; offs < 0x10;offs++)
	{
		int x,y;

		x = bosco_radarx[offs] + ((~bosco_radarattr[offs] & 0x01) << 8);
		y = 253 - bosco_radary[offs];
		if (flip_screen) x -= 3;

		drawgfx(bitmap,Machine->gfx[2],
				((bosco_radarattr[offs] & 0x0e) >> 1) ^ 0x07,
				0,
				0,0,
				x,y,
				cliprect,TRANSPARENCY_COLOR,0);
	}
}


static void draw_stars( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;

	/* draw the stars */
	if (1)
	{
		int bpen;

		bpen = Machine->pens[0x1f];
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;
			int set;
			int starset[4][2] = {{0,3},{0,1},{2,3},{2,1}};

			set = bosco_starblink[0] + (bosco_starblink[1] << 1);

			if (((stars[offs].set == starset[set][0]) ||
				 (stars[offs].set == starset[set][1])))
			{
				x = (stars[offs].x + stars_scrollx) % 224;
				y = (stars[offs].y + stars_scrolly) % 224 + 16;

				if (flip_screen) x += 64;

				if (read_pixel(bitmap, x, y) == bpen)
				{
					plot_pixel(bitmap, x, y, stars[offs].col);
				}
			}
		}
	}
}


VIDEO_UPDATE( bosco )
{
	/* the radar tilemap is just 8x32. We rely on the tilemap code to repeat it across
	   the screen, and clip it to only the position where it is supposed to be shown */
	struct rectangle fg_clip = *cliprect;
	struct rectangle bg_clip = *cliprect;
	if (flip_screen)
	{
		bg_clip.min_x = 8*8;
		fg_clip.max_x = 8*8-1;
	}
	else
	{
		bg_clip.max_x = 28*8-1;
		fg_clip.min_x = 28*8;
	}

	tilemap_draw(bitmap,&bg_clip,bg_tilemap,0,0);
	tilemap_draw(bitmap,&fg_clip,fg_tilemap,0,0);

	draw_sprites(bitmap,cliprect);

	/* draw the high priority characters */
	tilemap_draw(bitmap,&bg_clip,bg_tilemap,1,0);
	tilemap_draw(bitmap,&fg_clip,fg_tilemap,1,0);

	draw_bullets(bitmap,cliprect);

	draw_stars(bitmap,cliprect);
}


VIDEO_EOF( bosco )
{
	int speedsx[8] = { -1, -2, -3, 0, 3, 2, 1, 0 };
	int speedsy[8] = { 0, -1, -2, -3, 0, 3, 2, 1 };

	stars_scrollx += speedsx[bosco_starcontrol & 0x07];
	stars_scrolly += speedsy[(bosco_starcontrol & 0x38) >> 3];
}
