/*************************************************************************

	Namco Pac Man

**************************************************************************

	This file is used by the Pac Man, Pengo & Jr Pac Man drivers.

	Pengo & Pac Man are almost identical, the only differences being the
	extra gfx bank in Pengo, and the need to compensate for an hardware
	sprite positioning "bug" in Pac Man.

	Jr Pac Man has the same sprite hardware as Pac Man, the extra bank
	from Pengo and a scrolling playfield at the expense of one color per row
	for the playfield so it can fit in the same amount of ram.

**************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *tilemap;
static int charbank;
static int spritebank;
static int palettebank;
static int colortablebank;
static int flipscreen;
static int bgpriority;

static int xoffsethack;
data8_t *sprite_bank;
data8_t *tiles_bankram;

static struct rectangle spritevisiblearea =
{
	2*8, 34*8-1,
	0*8, 28*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.


  Pac Man has a 32x8 palette PROM and a 256x4 color lookup table PROM.

  Pengo has a 32x8 palette PROM and a 1024x4 color lookup table PROM.

  The palette PROM is connected to the RGB output this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED


  Jr. Pac Man has two 256x4 palette PROMs (the three msb of the address are
  grounded, so the effective colors are only 32) and one 256x4 color lookup
  table PROM.

  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- BLUE
     -- 470 ohm resistor  -- BLUE
     -- 220 ohm resistor  -- GREEN
  bit 0 -- 470 ohm resistor  -- GREEN

  bit 3 -- 1  kohm resistor  -- GREEN
     -- 220 ohm resistor  -- RED
     -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

PALETTE_INIT( pacman )
{
	int i;
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		palette_set_color(i,r,g,b);
	}

	/* color_prom now points to the beginning of the lookup table */
	color_prom += 32;

	/* character lookup table */
	/* sprites use the same color lookup table as characters */
	for (i = 0;i < 64*4;i++)
	{
		/* first palette bank */
		colortable[i] = color_prom[i] & 0x0f;

		/* second palette bank */
		if( colortable[i] != 0 )
		{
			colortable[i + 64*4] = colortable[i] + 0x10;
		}
		else
		{
			colortable[i + 64*4] = 0;
		}
	}
}

UINT32 pacman_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
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

static void pacman_get_tile_info(int tile_index)
{
	int code = videoram[tile_index] | (charbank << 8);
	int attr = (colorram[tile_index] & 0x1f) | (colortablebank << 5) | (palettebank << 6 );

	SET_TILE_INFO(0,code,attr,0)
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( pacman )
{
	charbank = 0;
	spritebank = 0;
	palettebank = 0;
	colortablebank = 0;
	flipscreen = 0;
	bgpriority = 0;

	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	xoffsethack = 1;

	tilemap = tilemap_create( pacman_get_tile_info, pacman_scan_rows, TILEMAP_OPAQUE, 8, 8, 36, 28 );

	if( !tilemap )
		return 1;

	return 0;
}

WRITE_HANDLER( pacman_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( tilemap, offset );
	}
}

WRITE_HANDLER( mspactwin_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty( tilemap, offset );
		force_partial_update(cpu_getscanline() + 29);
	}
}

WRITE_HANDLER( pacman_colorram_w )
{
	if (colorram[offset] != data)
	{
		colorram[offset] = data;
		tilemap_mark_tile_dirty( tilemap, offset );
	}
}

WRITE_HANDLER( pacman_flipscreen_w )
{
	if (flipscreen != (data & 1))
	{
		flipscreen = data & 1;
		tilemap_set_flip( tilemap, flipscreen * ( TILEMAP_FLIPX + TILEMAP_FLIPY ) );
	}
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( pacman )
{
	if (bgpriority != 0)
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);
	else
		tilemap_draw(bitmap,cliprect,tilemap,TILEMAP_IGNORE_TRANSPARENCY,0);

	if( spriteram_size )
	{
		int offs;

		struct rectangle spriteclip = spritevisiblearea;
		sect_rect(&spriteclip, cliprect);

		/* Draw the sprites. Note that it is important to draw them exactly in this */
		/* order, to have the correct priorities. */
		for (offs = spriteram_size - 2;offs > 2*2;offs -= 2)
		{
			int sx,sy;

			sx = 272 - spriteram_2[offs + 1];
			sy = spriteram_2[offs] - 31;

			drawgfx(bitmap,Machine->gfx[1],
					( spriteram[offs] >> 2 ) | (spritebank << 6),
					( spriteram[offs + 1] & 0x1f ) | (colortablebank << 5) | (palettebank << 6 ),
					spriteram[offs] & 1,spriteram[offs] & 2,
					sx,sy,
					&spriteclip,TRANSPARENCY_COLOR,0);

			/* also plot the sprite with wraparound (tunnel in Crush Roller) */
			drawgfx(bitmap,Machine->gfx[1],
					( spriteram[offs] >> 2 ) | (spritebank << 6),
					( spriteram[offs + 1] & 0x1f ) | (colortablebank << 5) | (palettebank << 6 ),
					spriteram[offs] & 1,spriteram[offs] & 2,
					sx - 256,sy,
					&spriteclip,TRANSPARENCY_COLOR,0);
		}
		/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
		/* one pixel to the left to get a more correct placement */
		for (offs = 2*2;offs >= 0;offs -= 2)
		{
			int sx,sy;

			sx = 272 - spriteram_2[offs + 1];
			sy = spriteram_2[offs] - 31;

			drawgfx(bitmap,Machine->gfx[1],
					( spriteram[offs] >> 2 ) | (spritebank << 6),
					( spriteram[offs + 1] & 0x1f ) | (colortablebank << 5) | (palettebank << 6 ),
					spriteram[offs] & 1,spriteram[offs] & 2,
					sx,sy + xoffsethack,
					&spriteclip,TRANSPARENCY_COLOR,0);

			/* also plot the sprite with wraparound (tunnel in Crush Roller) */
			drawgfx(bitmap,Machine->gfx[1],
					( spriteram[offs] >> 2 ) | (spritebank << 6),
					( spriteram[offs + 1] & 0x1f ) | (colortablebank << 5) | (palettebank << 6 ),
					spriteram[offs] & 2,spriteram[offs] & 1,
					sx - 256,sy + xoffsethack,
					&spriteclip,TRANSPARENCY_COLOR,0);
		}
	}

	if (bgpriority != 0)
		tilemap_draw(bitmap,cliprect,tilemap,0,0);
}


/*************************************************************************

	Sega Pengo

**************************************************************************/

VIDEO_START( pengo )
{
	charbank = 0;
	spritebank = 0;
	palettebank = 0;
	colortablebank = 0;
	flipscreen = 0;
	bgpriority = 0;

	xoffsethack = 0;

	tilemap = tilemap_create( pacman_get_tile_info, pacman_scan_rows, TILEMAP_OPAQUE, 8, 8, 36, 28 );

	if( !tilemap )
		return 1;

    return 0;
}

WRITE_HANDLER( pengo_palettebank_w )
{
	if (palettebank != data)
	{
		palettebank = data;
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

WRITE_HANDLER( pengo_colortablebank_w )
{
	if (colortablebank != data)
	{
		colortablebank = data;
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}

WRITE_HANDLER( pengo_gfxbank_w )
{
	if (charbank != (data & 1))
	{
		spritebank = data & 1;
		charbank = data & 1;
		tilemap_mark_all_tiles_dirty( tilemap );
	}
}


/*************************************************************************

Van Van

**************************************************************************/

WRITE_HANDLER( vanvan_bgcolor_w )
{
	int c = 0xaa * (data & 1);
	palette_set_color(0,c,c,c);
}


/*************************************************************************

S2650 Games

**************************************************************************/

static void s2650_get_tile_info(int tile_index)
{
	int colbank, code, attr;

	colbank = tiles_bankram[tile_index & 0x1f] & 0x3;

	code = videoram[tile_index] + (colbank << 8);
	attr = colorram[tile_index & 0x1f];

	SET_TILE_INFO(0,code,attr & 0x1f,0)
}

VIDEO_START( s2650games )
{
	xoffsethack = 1;

	tilemap = tilemap_create( s2650_get_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32 );

	if( !tilemap )
		return 1;

	tilemap_set_scroll_cols(tilemap, 32);

	return 0;
}

VIDEO_UPDATE( s2650games )
{
	int offs;

	tilemap_draw(bitmap,cliprect,tilemap,0,0);

	for (offs = spriteram_size - 2;offs > 2*2;offs -= 2)
	{
		int sx,sy;


		sx = 255 - spriteram_2[offs + 1];
		sy = spriteram_2[offs] - 15;

		/* TODO: ?? */
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] >> 2) | ((sprite_bank[offs] & 3) << 6),
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				sx,sy,
				cliprect,TRANSPARENCY_COLOR,0);
	}
	/* In the Pac Man based games (NOT Pengo) the first two sprites must be offset */
	/* one pixel to the left to get a more correct placement */
	for (offs = 2*2;offs >= 0;offs -= 2)
	{
		int sx,sy;


		sx = 255 - spriteram_2[offs + 1];
		sy = spriteram_2[offs] - 15;

		/* TODO: ?? */
		drawgfx(bitmap,Machine->gfx[1],
				(spriteram[offs] >> 2) | ((sprite_bank[offs] & 3)<<6),
				spriteram[offs + 1] & 0x1f,
				spriteram[offs] & 1,spriteram[offs] & 2,
				sx,sy + xoffsethack,
				cliprect,TRANSPARENCY_COLOR,0);
	}
}

WRITE_HANDLER( s2650games_videoram_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(tilemap,offset);
}

WRITE_HANDLER( s2650games_colorram_w )
{
	int i;
	colorram[offset & 0x1f] = data;
	for (i = offset; i < 0x0400; i += 32)
		tilemap_mark_tile_dirty(tilemap, i);
}

WRITE_HANDLER( s2650games_scroll_w )
{
	tilemap_set_scrolly(tilemap, offset, data);
}

WRITE_HANDLER( s2650games_tilesbank_w )
{
	tiles_bankram[offset] = data;
	tilemap_mark_all_tiles_dirty(tilemap);
}


/*************************************************************************

Jr. Pac-Man

**************************************************************************/

/*
   0 -   31 = column 2 - 33 attr (used for all 54 rows)
  64 - 1791 = column 2 - 33 code (54 rows)
1794 - 1821 = column 34 code (28 rows)
1826 - 1853 = column 35 code (28 rows)
1858 - 1885 = column 0 code (28 rows)
1890 - 1917 = column 1 code (28 rows)
1922 - 1949 = column 34 attr (28 rows)
1954 - 1981 = column 35 attr (28 rows)
1986 - 2013 = column 0 attr (28 rows)
2018 - 2045 = column 1 attr (28 rows)
*/

UINT32 jrpacman_scan_rows( UINT32 col, UINT32 row, UINT32 num_cols, UINT32 num_rows )
{
	int offs;

	row += 2;
	col -= 2;
	if ((col & 0x20) && (row & 0x20))
		offs = 0;
	else if (col & 0x20)
		offs = row + (((col&0x3) | 0x38)<< 5);
	else
		offs = col + (row << 5);
	return offs;
}

static void jrpacman_get_tile_info(int tile_index)
{
	int color_index, code, attr;
	if( tile_index < 1792 )
	{
		color_index = tile_index & 0x1f;
	}
	else
	{
		color_index = tile_index + 0x80;
	}

	code = videoram[tile_index] | (charbank << 8);
	attr = (videoram[color_index] & 0x1f) | (colortablebank << 5) | (palettebank << 6 );

	SET_TILE_INFO(0,code,attr,0)
}

static void jrpacman_mark_tile_dirty( int offset )
{
	if( offset < 0x20 )
	{
		/* line color - mark whole line as dirty */
		int i;
		for( i = 2 * 0x20; i < 56 * 0x20; i += 0x20 )
		{
			tilemap_mark_tile_dirty( tilemap, offset + i );
		}
	}
	else if (offset < 1792)
	{
		/* tiles for playfield */
		tilemap_mark_tile_dirty( tilemap, offset );
	}
	else
	{
		/* tiles & colors for top and bottom two rows */
		tilemap_mark_tile_dirty( tilemap, offset & ~0x80 );
	}
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
VIDEO_START( jrpacman )
{
	charbank = 0;
	spritebank = 0;
	palettebank = 0;
	colortablebank = 0;
	flipscreen = 0;
	bgpriority = 0;

	xoffsethack = 1;

	tilemap = tilemap_create( jrpacman_get_tile_info,jrpacman_scan_rows,TILEMAP_TRANSPARENT,8,8,36,54 );

	if( !tilemap )
		return 1;

	tilemap_set_transparent_pen( tilemap, 0 );
	tilemap_set_scroll_cols( tilemap, 36 );
	return 0;
}

WRITE_HANDLER( jrpacman_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
		jrpacman_mark_tile_dirty(offset);
	}
}

WRITE_HANDLER( jrpacman_charbank_w )
{
	if (charbank != (data & 1))
	{
		charbank = data & 1;
		tilemap_mark_all_tiles_dirty(tilemap);
	}
}

WRITE_HANDLER( jrpacman_spritebank_w )
{
	spritebank = (data & 1);
}

WRITE_HANDLER( jrpacman_scroll_w )
{
	int i;
	for( i = 2; i < 34; i++ )
	{
		tilemap_set_scrolly( tilemap, i, data );
	}
}

WRITE_HANDLER( jrpacman_bgpriority_w )
{
	bgpriority = (data & 1);
}
