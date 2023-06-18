/***************************************************************************

	Atari Food Fight hardware

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/atarigen.h"
#include "foodf.h"


static UINT8 playfield_flip;



/*************************************
 *
 *	Tilemap callbacks
 *
 *************************************/

static void get_playfield_tile_info(int tile_index)
{
	UINT16 data = atarigen_playfield[tile_index];
	int code = (data & 0xff) | ((data >> 7) & 0x100);
	int color = (data >> 8) & 0x3f;
	SET_TILE_INFO(0, code, color, playfield_flip ? (TILE_FLIPX | TILE_FLIPY) : 0);
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

VIDEO_START( foodf )
{
	/* initialize the playfield */
	atarigen_playfield_tilemap = tilemap_create(get_playfield_tile_info, tilemap_scan_cols, TILEMAP_OPAQUE, 8,8, 32,32);
	if (!atarigen_playfield_tilemap)
		return 1;

	/* adjust the playfield for the 8 pixel offset */
	tilemap_set_scrollx(atarigen_playfield_tilemap, 0, -8);
	playfield_flip = 0;
	return 0;
}



/*************************************
 *
 *	Cocktail flip
 *
 *************************************/

void foodf_set_flip(int flip)
{
	if (flip != playfield_flip)
	{
		playfield_flip = flip;
		tilemap_mark_all_tiles_dirty(atarigen_playfield_tilemap);
	}
}



/*************************************
 *
 *	Palette RAM write
 *
 *************************************/

WRITE16_HANDLER( foodf_paletteram_w )
{
	int newword, r, g, b, bit0, bit1, bit2;

	COMBINE_DATA(&paletteram16[offset]);
	newword = paletteram16[offset];

	/* only the bottom 8 bits are used */
	/* red component */
	bit0 = (newword >> 0) & 0x01;
	bit1 = (newword >> 1) & 0x01;
	bit2 = (newword >> 2) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* green component */
	bit0 = (newword >> 3) & 0x01;
	bit1 = (newword >> 4) & 0x01;
	bit2 = (newword >> 5) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	/* blue component */
	bit0 = 0;
	bit1 = (newword >> 6) & 0x01;
	bit2 = (newword >> 7) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_set_color(offset, r, g, b);
}



/*************************************
 *
 *	Main refresh
 *
 *************************************/

VIDEO_UPDATE( foodf )
{
	int offs;

	/* draw the playfield */
	tilemap_draw(bitmap, cliprect, atarigen_playfield_tilemap, 0,0);

	/* walk the motion object list. */
	for (offs = 0; offs < spriteram_size / 4; offs += 2)
	{
		int data1 = spriteram16[offs];
		int data2 = spriteram16[offs+1];

		int pict = data1 & 0xff;
		int color = (data1 >> 8) & 0x1f;
		int xpos = (data2 >> 8) & 0xff;
		int ypos = (0xff - data2 - 16) & 0xff;
		int hflip = (data1 >> 15) & 1;
		int vflip = (data1 >> 14) & 1;

		drawgfx(bitmap, Machine->gfx[1], pict, color, hflip, vflip,
				xpos, ypos, cliprect, TRANSPARENCY_PEN, 0);

		/* draw again with wraparound (needed to get the end of level animation right) */
		drawgfx(bitmap, Machine->gfx[1], pict, color, hflip, vflip,
				xpos - 256, ypos, cliprect, TRANSPARENCY_PEN, 0);
	}
}
