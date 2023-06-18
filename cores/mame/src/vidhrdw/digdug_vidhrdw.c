#include "driver.h"
#include "vidhrdw/generic.h"


data8_t *digdug_videoram,*digdug_objram, *digdug_posram, *digdug_flpram;


static struct tilemap *bg_tilemap,*tx_tilemap;


static int bg_select, tx_color_mode, bg_disable, bg_color_bank;

unsigned char *digdug_sharedram;
READ_HANDLER( digdug_sharedram0_r );
WRITE_HANDLER( digdug_sharedram0_w );
READ_HANDLER( digdug_sharedram1_r );
WRITE_HANDLER( digdug_sharedram1_w );
READ_HANDLER( digdug_sharedram2_r );
WRITE_HANDLER( digdug_sharedram2_w );
READ_HANDLER( digdug_sharedram3_r );
WRITE_HANDLER( digdug_sharedram3_w );

READ_HANDLER( digdug_sharedram0_r )
{
	return digdug_sharedram[offset];
}


WRITE_HANDLER( digdug_sharedram0_w )
{

	digdug_sharedram[offset] = data;
}


READ_HANDLER( digdug_sharedram1_r )
{
	return digdug_objram[offset];
}


WRITE_HANDLER( digdug_sharedram1_w )
{

	digdug_objram[offset] = data;
}

READ_HANDLER( digdug_sharedram2_r )
{
	return digdug_posram[offset];
}


WRITE_HANDLER( digdug_sharedram2_w )
{

	digdug_posram[offset] = data;
}

READ_HANDLER( digdug_sharedram3_r )
{
	return digdug_flpram[offset];
}


WRITE_HANDLER( digdug_sharedram3_w )
{

	digdug_flpram[offset] = data;
}



/***************************************************************************

  Convert the color PROMs.

  digdug has one 32x8 palette PROM and two 256x4 color lookup table PROMs
  (one for characters, one for sprites).
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

PALETTE_INIT( digdug )
{
	int i;

	for (i = 0;i < 32;i++)
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

	/* characters - direct mapping */
	for (i = 0; i < 16; i++)
	{
		colortable[0] = 0;
		colortable[1] = i;
		colortable += 2;
	}
	/* sprites */
	for (i = 0;i < 0x100;i++)
		*(colortable++) = (*(color_prom++) & 0x0f) + 0x10;
	/* bg_select */
	for (i = 0;i < 0x100;i++)
		*(colortable++) = (*(color_prom++) & 0x0f);
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


static void bg_get_tile_info(int tile_index)
{
	data8_t *rom = memory_region(REGION_GFX4);
	int code = rom[tile_index | (bg_select << 10)];
	/* when the background is "disabled", it is actually still drawn, but using
	   a color code that makes all pixels black. There are pullups setting the
	   code to 0xf, but also solder pads that optionally connect the lines with
	   tilemap RAM, therefore allowing to pick some bits of the color code from
	   the top 4 bits of alpha code. This feature is not used by Dig Dug. */
	int color = bg_disable ? 0xf : (code >> 4);
	SET_TILE_INFO(
			2,
			code,
			color | bg_color_bank,
			0)
}

static void tx_get_tile_info(int tile_index)
{
	unsigned char code = digdug_videoram[tile_index];
	int color;

	/* the hardware has two ways to pick the color, either straight from the
	   bottom 4 bits of the character code, or from the top 4 bits through a
	   formula. The former method isnot used by Dig Dug and seems kind of
	   useless (I don't know what use they were thinking of when they added
	   it), anyway here it is reproduced faithfully. */
	if (tx_color_mode)
		color = code & 0x0f;
	else
		color = ((code >> 4) & 0x0e) | ((code >> 3) & 2);

	/* the hardware has two character sets, one normal and one x-flipped. When
	   screen is flipped, character y flip is done by the hardware inverting the
	   timing signals, while x flip is done by selecting the 2nd character set.
	   We reproduce this here, but since the tilemap system automatically flips
	   characters when screen is flipped, we have to flip them back. */
	SET_TILE_INFO(
			0,
			(code & 0x7f) | (flip_screen ? 0x80 : 0),
			color,
			flip_screen ? TILE_FLIPX : 0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( digdug )
{
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan,TILEMAP_OPAQUE,     8,8,36,28);
	tx_tilemap = tilemap_create(tx_get_tile_info,tilemap_scan,TILEMAP_TRANSPARENT,8,8,36,28);

	if (!bg_tilemap | !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0);

	spriteram   = digdug_objram + 0x380;
	spriteram_2 = digdug_posram + 0x380;
	spriteram_3 = digdug_flpram + 0x380;

	return 0;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

READ_HANDLER( digdug_videoram_r )
{
	return digdug_videoram[offset];
}

WRITE_HANDLER( digdug_videoram_w )
{
	if (digdug_videoram[offset] != data)
	{
		digdug_videoram[offset] = data;
		tilemap_mark_tile_dirty(tx_tilemap,offset & 0x3ff);
	}
}

WRITE_HANDLER( digdug_PORT_w )
{
	switch (offset)
	{
		case 0:	/* select background picture */
		case 1:
			{
				int shift = offset;
				int mask = 1 << shift;

				if ((bg_select & mask) != ((data & 1) << shift))
				{
					bg_select = (bg_select & ~mask) | ((data & 1) << shift);
					tilemap_mark_all_tiles_dirty(bg_tilemap);
				}
			}
			break;

		case 2:	/* select alpha layer color mode (see tx_get_tile_info) */
			if (tx_color_mode != (data & 1))
			{
				tx_color_mode = data & 1;
				tilemap_mark_all_tiles_dirty(tx_tilemap);
			}
			break;

		case 3:	/* "disable" background (see bg_get_tile_info) */
			if (bg_disable != (data & 1))
			{
				bg_disable = data & 1;
				tilemap_mark_all_tiles_dirty(bg_tilemap);
			}
			break;

		case 4:	/* background color bank */
		case 5:
			{
				int shift = offset;
				int mask = 1 << shift;

				if ((bg_color_bank & mask) != ((data & 1) << shift))
				{
					bg_color_bank = (bg_color_bank & ~mask) | ((data & 1) << shift);
					tilemap_mark_all_tiles_dirty(bg_tilemap);
				}
			}
			break;

		case 6:	/* n.c. */
			break;

		case 7:	/* FLIP */
			flip_screen_set(data & 1);
			break;
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static struct rectangle spritevisiblearea =
{
	2*8, 34*8-1,
	0*8, 28*8-1
};

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
		int sprite = spriteram[offs];
		int color = spriteram[offs+1] & 0x3f;
		int sx = spriteram_2[offs+1] - 40+1;
		int sy = 256 - spriteram_2[offs] + 1;	/* sprites are buffered and delayed by one scanline */
		int flipx = (spriteram_3[offs] & 0x01);
		int flipy = (spriteram_3[offs] & 0x02) >> 1;
		int size  = (sprite & 0x80) >> 7;
		int x,y;

		if (size)
			sprite = (sprite & 0xc0) | ((sprite & ~0xc0) << 2);

		if (flip_screen)
		{
			flipx ^= 1;
			flipy ^= 1;
		}

		sy -= 16 * size;
		sy = (sy & 0xff) - 32;	/* fix wraparound */

		for (y = 0;y <= size;y++)
		{
			for (x = 0;x <= size;x++)
			{
				drawgfx(bitmap,Machine->gfx[1],
					sprite + gfx_offs[y ^ (size * flipy)][x ^ (size * flipx)],
					color,
					flipx,flipy,
					((sx + 16*x) & 0xff), sy + 16*y,
					&spritevisiblearea,TRANSPARENCY_COLOR,0x1f);
				/* wraparound */
				drawgfx(bitmap,Machine->gfx[1],
					sprite + gfx_offs[y ^ (size * flipy)][x ^ (size * flipx)],
					color,
					flipx,flipy,
					((sx + 16*x) & 0xff) + 0x100, sy + 16*y,
					&spritevisiblearea,TRANSPARENCY_COLOR,0x1f);
			}
		}
	}
}


VIDEO_UPDATE( digdug )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);

	draw_sprites(bitmap,cliprect);
}
