/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define MAX_STARS 250
#define STARS_COLOR_BASE 32

data8_t *galaga_videoram;
data8_t *galaga_ram1,*galaga_ram2,*galaga_ram3;
data8_t galaga_starcontrol[6];
static unsigned int stars_scrollx,stars_scrolly;

READ_HANDLER( galaga_sharedram1_r );
WRITE_HANDLER( galaga_sharedram1_w );
READ_HANDLER( galaga_sharedram2_r );
WRITE_HANDLER( galaga_sharedram2_w );
READ_HANDLER( galaga_sharedram3_r );
WRITE_HANDLER( galaga_sharedram3_w );

READ_HANDLER( galaga_sharedram1_r )
{
	return galaga_ram1[offset];
}


WRITE_HANDLER( galaga_sharedram1_w )
{

	galaga_ram1[offset] = data;
}

READ_HANDLER( galaga_sharedram2_r )
{
	return galaga_ram2[offset];
}


WRITE_HANDLER( galaga_sharedram2_w )
{

	galaga_ram2[offset] = data;
}

READ_HANDLER( galaga_sharedram3_r )
{
	return galaga_ram3[offset];
}


WRITE_HANDLER( galaga_sharedram3_w )
{

	galaga_ram3[offset] = data;
}

struct star
{
	int x,y,col,set;
};
static struct star stars[MAX_STARS];
static int total_stars;
static int galaga_gfxbank; /* used by catsbee */

static struct tilemap *tx_tilemap;



/***************************************************************************

  Convert the color PROMs.

  Galaga has one 32x8 palette PROM and two 256x4 color lookup table PROMs
  (one for characters, one for sprites). Only the first 128 bytes of the
  lookup tables seem to be used.
  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( galaga )
{
	int i;


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

	/* characters */
	for (i = 0;i < 64*4;i++)
		*(colortable++) = (*(color_prom++) & 0x0f) + 0x10;	/* chars */

	/* sprites */
	for (i = 0;i < 64*4;i++)
		*(colortable++) = (*(color_prom++) & 0x0f);	/* sprites */


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

/* convert from 32x32 to 36x28 */
static UINT32 tilemap_scan(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
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


static void get_tile_info(int tile_index)
{
	/* the hardware has two character sets, one normal and one x-flipped. When
	   screen is flipped, character y flip is done by the hardware inverting the
	   timing signals, while x flip is done by selecting the 2nd character set.
	   We reproduce this here, but since the tilemap system automatically flips
	   characters when screen is flipped, we have to flip them back. */
	SET_TILE_INFO(
			0,
			(galaga_videoram[tile_index] & 0x7f) | (flip_screen ? 0x80 : 0) | (galaga_gfxbank << 8),
			galaga_videoram[tile_index + 0x400] & 0x3f,
			flip_screen ? TILE_FLIPX : 0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( galaga )
{
	int generator;
	int x,y;
	int set = 0;

	tx_tilemap = tilemap_create(get_tile_info,tilemap_scan,TILEMAP_TRANSPARENT_COLOR,8,8,36,28);

	if (!tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0x1f);

	galaga_gfxbank = 0;

	spriteram   = galaga_ram1 + 0x380;
	spriteram_2 = galaga_ram2 + 0x380;
	spriteram_3 = galaga_ram3 + 0x380;

	/* precalculate the star background */
	/* this comes from the Galaxian hardware, Galaga is different */
	total_stars = 0;
	generator = 0;

	for (y = 0;y <= 255;y++)
	{
		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (((~generator >> 16) & 1) && (generator & 0xff) == 0xff)
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

READ_HANDLER( galaga_videoram_r )
{
	return galaga_videoram[offset];
}

WRITE_HANDLER( galaga_videoram_w )
{
	if (galaga_videoram[offset] != data)
	{
		galaga_videoram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( galaga_starcontrol_w )
{
	galaga_starcontrol[offset] = data & 1;
}

WRITE_HANDLER ( gatsbee_bank_w )
{
	galaga_gfxbank = data & 0x1;
	tilemap_mark_all_tiles_dirty(tx_tilemap);
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	int offs;


	for (offs = 0;offs < 0x80;offs += 2)
	{
		static int gfx_offs[2][2] =
		{
			{ 0, 1 },
			{ 2, 3 }
		};
		int sprite = spriteram[offs] & 0x7f;
		int color = spriteram[offs+1] & 0x3f;
		int sx = spriteram_2[offs+1] - 40 + 0x100*(spriteram_3[offs+1] & 3);
		int sy = 256 - spriteram_2[offs] + 1;	/* sprites are buffered and delayed by one scanline */
		int flipx = (spriteram_3[offs] & 0x01);
		int flipy = (spriteram_3[offs] & 0x02) >> 1;
		int sizex = (spriteram_3[offs] & 0x04) >> 2;
		int sizey = (spriteram_3[offs] & 0x08) >> 3;
		int x,y;

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
					sx + 16*x, sy + 16*y,
					cliprect,TRANSPARENCY_COLOR,0x0f);
			}
		}
	}
}


static void draw_stars( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	/* draw the stars */
	if (1)
	{
		int bpen,offs;

		bpen = Machine->pens[0x1f];
		for (offs = 0;offs < total_stars;offs++)
		{
			int x,y;
			int set;
			int starset[4][2] = {{0,3},{0,1},{2,3},{2,1}};

			set = galaga_starcontrol[3] + (galaga_starcontrol[4] << 1);

			if ((stars[offs].set == starset[set][0]) ||
				(stars[offs].set == starset[set][1]))
			{
				x = (stars[offs].x + stars_scrollx) % 256 + 16;
				y = (stars[offs].y + stars_scrolly) % 256;

				if (y >= Machine->visible_area.min_y &&
					y <= Machine->visible_area.max_y)
				{
					if (read_pixel(bitmap, x, y) == bpen)
						plot_pixel(bitmap, x, y, stars[offs].col);
				}
			}
		}
	}
}


VIDEO_UPDATE( galaga )
{
	fillbitmap(bitmap,Machine->pens[0x1f],cliprect);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);

	draw_stars(bitmap,cliprect);
}



VIDEO_EOF( galaga )
{
	/* this function is called by galaga_interrupt_1() */
	int s0,s1,s2;
	int speeds[8] = { -1, -2, -3, 0, 3, 2, 1, 0 };


	s0 = galaga_starcontrol[0];
	s1 = galaga_starcontrol[1];
	s2 = galaga_starcontrol[2];

	stars_scrollx += speeds[s0 + s1*2 + s2*4];
}
