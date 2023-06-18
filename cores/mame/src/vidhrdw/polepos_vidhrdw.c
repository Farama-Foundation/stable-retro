#include "driver.h"

data16_t *polepos_view16_memory;
data16_t *polepos_road16_memory;
data16_t *polepos_sprite16_memory;
data16_t *polepos_alpha16_memory;

/* modified vertical position built from three nibbles (12 bit)
 * of ROMs 136014-142, 136014-143, 136014-144
 * The value RVP (road vertical position, lower 12 bits) is added
 * to this value and the upper 10 bits of the result are used to
 * address the playfield video memory (AB0 - AB9).
 */
static data16_t polepos_vertical_position_modifier[256];

static data16_t road16_vscroll;

static struct tilemap *bg_tilemap,*tx_tilemap;
static int polepos_chacl;


/***************************************************************************

  Convert the color PROMs.

  Pole Position has three 256x4 palette PROMs (one per gun)
  and a lot ;-) of 256x4 lookup table PROMs.
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
		-- 470 ohm resistor  -- RED/GREEN/BLUE
		-- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( polepos )
{
	int i, j;

	/*******************************************************
	 * Color PROMs
	 * Sheet 15B: middle, 136014-137,138,139
	 * Inputs: MUX0 ... MUX3, ALPHA/BACK, SPRITE/BACK, 128V, COMPBLANK
	 *
	 * Note that we only decode the lower 128 colors because
	 * the upper 128 are all black and used during the
	 * horizontal and vertical blanking periods.
	 * The purpose of the 128V input is to use a different palette for the
	 * background and for the road; it is irrelevant for alpha and
	 * sprites because their palette is the same in both halves.
	 * Anyway, we emulate that to a certain extent, using different
	 * colortables for the two halves of the screen. We don't support the
	 * palette change in the middle of a sprite, however.
	 * Also, note that priority encoding is done is such a way that alpha
	 * will use palette bank 2 or 3 depending on whether there is a sprite
	 * below the pixel or not. That would be tricky to emulate, and it's
	 * not needed because of course the two banks are the same.
	 *******************************************************/
	for (i = 0; i < 128; i++)
	{
		int bit0,bit1,bit2,bit3,r,g,b;

		/* Sheet 15B: 136014-0137 red component */
		bit0 = (color_prom[0x000 + i] >> 0) & 1;
		bit1 = (color_prom[0x000 + i] >> 1) & 1;
		bit2 = (color_prom[0x000 + i] >> 2) & 1;
		bit3 = (color_prom[0x000 + i] >> 3) & 1;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* Sheet 15B: 136014-0138 green component */
		bit0 = (color_prom[0x100 + i] >> 0) & 1;
		bit1 = (color_prom[0x100 + i] >> 1) & 1;
		bit2 = (color_prom[0x100 + i] >> 2) & 1;
		bit3 = (color_prom[0x100 + i] >> 3) & 1;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		/* Sheet 15B: 136014-0139 blue component */
		bit0 = (color_prom[0x200 + i] >> 0) & 1;
		bit1 = (color_prom[0x200 + i] >> 1) & 1;
		bit2 = (color_prom[0x200 + i] >> 2) & 1;
		bit3 = (color_prom[0x200 + i] >> 3) & 1;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		palette_set_color(i,r,g,b);
	}

	/*******************************************************
	 * Alpha colors (colors 0x000-0x1ff)
	 * Sheet 15B: top left, 136014-140
	 * Inputs: SHFT0, SHFT1 and CHA8* ... CHA13*
	 *******************************************************/
	for (i = 0; i < 64*4; i++)
	{
		int color = color_prom[0x300 + i];
		colortable[0x0000 + i] = (color != 15) ? (0x020 + color) : 0x2f;
		colortable[0x0100 + i] = (color != 15) ? (0x060 + color) : 0x2f;
	}

	/*******************************************************
	 * Background colors (colors 0x200-0x2ff)
	 * Sheet 13A: left, 136014-141
	 * Inputs: SHFT2, SHFT3 and CHA8 ... CHA13
     * The background is only in the top half of the screen
	 *******************************************************/
	for (i = 0; i < 64*4; i++)
	{
		int color = color_prom[0x400 + i];
		colortable[0x0200 + i] = 0x000 + color;
	}

	/*******************************************************
	 * Sprite colors (colors 0x300-0xaff)
	 * Sheet 14B: right, 136014-146
	 * Inputs: CUSTOM0 ... CUSTOM3 and DATA0 ... DATA5
	 *******************************************************/
	for (i = 0; i < 64*16; i++)
	{
		int color = color_prom[0xc00 + i];
		colortable[0x0300 + i] = (color != 15) ? (0x010 + color) : 0x1f;
		colortable[0x0700 + i] = (color != 15) ? (0x050 + color) : 0x1f;
	}

	/*******************************************************
	 * Road colors (colors 0xb00-0x0eff)
	 * Sheet 13A: bottom left, 136014-145
	 * Inputs: R1 ... R6 and CHA0 ... CHA3
     * The road is only in the bottom half of the screen
	 *******************************************************/
	for (i = 0; i < 64*16; i++)
	{
		int color = color_prom[0x800 + i];
		colortable[0x0b00 + i] = 0x040 + color;
	}

	/* 136014-142, 136014-143, 136014-144 Vertical position modifiers */
	for (i = 0; i < 256; i++)
	{
		j = color_prom[0x500 + i] + (color_prom[0x600 + i] << 4) + (color_prom[0x700 + i] << 8);
		polepos_vertical_position_modifier[i] = j;
	}
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void bg_get_tile_info(int tile_index)
{
	data16_t word = polepos_view16_memory[tile_index];
	int code = (word & 0xff) | ((word & 0x4000) >> 6);
	int color = (word & 0x3f00) >> 8;
	SET_TILE_INFO(
			1,
			code,
			color,
			0)
}

static void tx_get_tile_info(int tile_index)
{
	data16_t word = polepos_alpha16_memory[tile_index];
	int code = (word & 0xff) | ((word & 0x4000) >> 6);
	int color = (word & 0x3f00) >> 8;

	/* I assume the purpose of CHACL is to allow the Z80 to control
	   the display (therefore using only the bottom 8 bits of tilemap RAM)
	   in case the Z8002 is not working. */
	if (polepos_chacl == 0)
	{
		code &= 0xff;
		color = 0;
	}

	/* 128V input to the palette PROM */
	if (tile_index >= 32*16) color |= 0x40;

	SET_TILE_INFO(
			0,
			code,
			color,
			0)
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( polepos )
{
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE,8,8,64,16);
	tx_tilemap = tilemap_create(tx_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT_COLOR,8,8,32,32);

	if (!bg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(tx_tilemap, 0x2f);

	return 0;
}


/***************************************************************************

  Sprite memory

***************************************************************************/

READ16_HANDLER( polepos_sprite16_r )
{
	return polepos_sprite16_memory[offset];
}

WRITE16_HANDLER( polepos_sprite16_w )
{
	COMBINE_DATA(&polepos_sprite16_memory[offset]);
}

READ_HANDLER( polepos_sprite_r )
{
	return polepos_sprite16_memory[offset] & 0xff;
}

WRITE_HANDLER( polepos_sprite_w )
{
	polepos_sprite16_memory[offset] = (polepos_sprite16_memory[offset] & 0xff00) | data;
}


/***************************************************************************

  Road memory

***************************************************************************/

READ16_HANDLER( polepos_road16_r )
{
	return polepos_road16_memory[offset];
}

WRITE16_HANDLER( polepos_road16_w )
{
	COMBINE_DATA(&polepos_road16_memory[offset]);
}

READ_HANDLER( polepos_road_r )
{
	return polepos_road16_memory[offset] & 0xff;
}

WRITE_HANDLER( polepos_road_w )
{
	polepos_road16_memory[offset] = (polepos_road16_memory[offset] & 0xff00) | data;
}

WRITE16_HANDLER( polepos_road16_vscroll_w )
{
	COMBINE_DATA(&road16_vscroll);
}


/***************************************************************************

  View memory

***************************************************************************/

READ16_HANDLER( polepos_view16_r )
{
	return polepos_view16_memory[offset];
}

WRITE16_HANDLER( polepos_view16_w )
{
	data16_t oldword = polepos_view16_memory[offset];
	COMBINE_DATA(&polepos_view16_memory[offset]);
	if (oldword != polepos_view16_memory[offset])
	{
		if (offset < 0x400)
			tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

READ_HANDLER( polepos_view_r )
{
	return polepos_view16_memory[offset] & 0xff;
}

WRITE_HANDLER( polepos_view_w )
{
	data16_t oldword = polepos_view16_memory[offset];
	polepos_view16_memory[offset] = (polepos_view16_memory[offset] & 0xff00) | data;
	if (oldword != polepos_view16_memory[offset])
	{
		if (offset < 0x400)
			tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

WRITE16_HANDLER( polepos_view16_hscroll_w )
{
	static data16_t scroll;

	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll);
}

WRITE_HANDLER( polepos_chacl_w )
{
	if (polepos_chacl != (data & 1))
	{
		polepos_chacl = data & 1;
		tilemap_mark_all_tiles_dirty(tx_tilemap);
	}
}


/***************************************************************************

  Alpha memory

***************************************************************************/

READ16_HANDLER( polepos_alpha16_r )
{
	return polepos_alpha16_memory[offset];
}

WRITE16_HANDLER( polepos_alpha16_w )
{
	data16_t oldword = polepos_alpha16_memory[offset];
	COMBINE_DATA(&polepos_alpha16_memory[offset]);
	if (oldword != polepos_alpha16_memory[offset])
	{
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}

READ_HANDLER( polepos_alpha_r )
{
	return polepos_alpha16_memory[offset] & 0xff;
}

WRITE_HANDLER( polepos_alpha_w )
{
	data16_t oldword = polepos_alpha16_memory[offset];
	polepos_alpha16_memory[offset] = (polepos_alpha16_memory[offset] & 0xff00) | data;
	if (oldword != polepos_alpha16_memory[offset])
	{
		tilemap_mark_tile_dirty(tx_tilemap,offset);
	}
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_road(struct mame_bitmap *bitmap)
{
	const UINT8 *road_control = memory_region(REGION_GFX5);
	const UINT8 *road_bits1 = memory_region(REGION_GFX5) + 0x2000;
	const UINT8 *road_bits2 = memory_region(REGION_GFX5) + 0x4000;
	int x, y, i;


	/* loop over the lower half of the screen */
	for (y = 128; y < 256; y++)
	{
		int xoffs, yoffs, xscroll, roadpal;
		UINT8 scanline[256 + 8];
		UINT8 *dest = scanline;
		pen_t *colortable;

		/* first add the vertical position modifier and the vertical scroll */
		yoffs = ((polepos_vertical_position_modifier[y] + road16_vscroll) >> 3) & 0x1ff;

		/* then use that as a lookup into the road memory */
		roadpal = polepos_road16_memory[yoffs] & 15;

		/* this becomes the palette base for the scanline */
		colortable = &Machine->remapped_colortable[0x0b00 + (roadpal << 6)];

		/* now fetch the horizontal scroll offset for this scanline */
		xoffs = polepos_road16_memory[0x380 + (y & 0x7f)] & 0x3ff;

		/* the road is drawn in 8-pixel chunks, so round downward and adjust the base */
		/* note that we assume there is at least 8 pixels of slop on the left/right */
		xscroll = xoffs & 7;
		xoffs &= ~7;

		/* loop over 8-pixel chunks */
		for (x = 0; x < 256 / 8 + 1; x++, xoffs += 8)
		{
			/* if the 0x200 bit of the xoffset is set, a special pin on the custom */
			/* chip is set and the /CE and /OE for the road chips is disabled */
			if (xoffs & 0x200)
			{
				/* in this case, it looks like we just fill with 0 */
				for (i = 0; i < 8; i++)
					*dest++ = 0;
			}

			/* otherwise, we clock in the bits and compute the road value */
			else
			{
				/* the road ROM offset comes from the current scanline and the X offset */
				int romoffs = ((y & 0x07f) << 6) + ((xoffs & 0x1f8) >> 3);

				/* fetch the current data from the road ROMs */
				int control = road_control[romoffs];
				int bits1 = road_bits1[romoffs];
				int bits2 = road_bits2[(romoffs & 0xfff) | ((romoffs & 0x1000) >> 1)];

				/* extract the road value and the carry-in bit */
				int roadval = control & 0x3f;
				int carin = control >> 7;

				/* draw this 8-pixel chunk */
				for (i = 8; i > 0; i--)
				{
					int bits = BIT(bits1,i) + (BIT(bits2,i) << 1);
					if (!carin && bits) bits++;
					*dest++ = roadval & 0x3f;
					roadval += bits;
				}
			}
		}

		/* draw the scanline */
		draw_scanline8(bitmap, 0, y, 256, &scanline[xscroll], colortable, -1);
	}
}

static void zoom_sprite( struct mame_bitmap *bitmap,int big,
		unsigned int code,unsigned int color,int flipx,int sx,int sy,
		int sizex,int sizey)
{
	const struct GfxElement *gfx = Machine->gfx[big ? 3 : 2];
	UINT8 *gfxdata = gfx->gfxdata + (code % gfx->total_elements) * gfx->char_modulo;
	UINT8 *scaling_rom = memory_region(REGION_GFX6);
	pen_t *colortable = gfx->colortable + color * gfx->color_granularity;
	int x,y;

	if (flipx) flipx = big ? 0x1f : 0x0f;

	for (y = 0;y <= sizey;y++)
	{
		int yy = (sy + y) & 0x1ff;

		/* the following should be a reasonable reproduction of how the real hardware works */
		if (yy >= 0x10 && yy < 0xf0)
		{
			int dy = scaling_rom[(y << 6) + sizey] & 0x1f;
			int xx = sx & 0x3ff;
			int siz = 0;
			int offs = 0;
			UINT8 *src;

			if (!big) dy >>= 1;
			src = gfxdata + dy * gfx->line_modulo;

			for (x = (big ? 0x40 : 0x20);x > 0;x--)
			{
				if (xx < 0x100)
				{
					int pen = colortable[src[offs/2 ^ flipx]];

					if (pen != 0x1f)
						plot_pixel(bitmap,xx,yy,pen);
				}
				offs++;

				siz = siz+1+sizex;
				if (siz & 0x40)
				{
					siz &= 0x3f;
					xx = (xx+1) & 0x3ff;
				}
			}
		}
	}
}

static void draw_sprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	data16_t *posmem = &polepos_sprite16_memory[0x380];
	data16_t *sizmem = &polepos_sprite16_memory[0x780];
	int i;

	for (i = 0; i < 64; i++, posmem += 2, sizmem += 2)
	{
		int sx = (posmem[1] & 0x3ff) - 0x40 + 4;
		int sy = 512 - (posmem[0] & 0x1ff) + 1;	/* sprites are buffered and delayed by one scanline */
		int sizex = (sizmem[1] & 0x3f00) >> 8;
		int sizey = (sizmem[0] & 0x3f00) >> 8;
		int code = sizmem[0] & 0x7f;
		int flipx = sizmem[0] & 0x80;
		int color = sizmem[1] & 0x3f;

		/* 128V input to the palette PROM */
		if (sy >= 128) color |= 0x40;

		zoom_sprite(bitmap, (sizmem[0] & 0x8000) ? 1 : 0,
				 code,
				 color,
				 flipx,
				 sx, sy,
				 sizex,sizey);
	}
}


VIDEO_UPDATE( polepos )
{
	struct rectangle clip = *cliprect;
	clip.max_y = 127;
	tilemap_draw(bitmap,&clip,bg_tilemap,0,0);
	draw_road(bitmap);
	draw_sprites(bitmap,cliprect);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);

	{
		int in = readinputport( 0 );
		static int lastin;

		if ((in ^ lastin) & 2)
			usrintf_showmessage((in & 2) ? "LO" : "HI");
		lastin = in;
	}
}
