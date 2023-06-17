#include "driver.h"
#include "vidhrdw/generic.h"
#include "tilemap.h"

data16_t *xorworld_videoram;
data16_t *xorworld_spriteram;

static struct tilemap *screen;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Xor World has three 256x4 palette PROMs (one per gun).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 460 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/

PALETTE_INIT( xorworld )
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++){
		int bit0,bit1,bit2,bit3;
		int r,g,b;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		r = 0x0e*bit0 + 0x1e*bit1 + 0x44*bit2 + 0x8f*bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		g = 0x0e*bit0 + 0x1e*bit1 + 0x44*bit2 + 0x8f*bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		b = 0x0e*bit0 + 0x1e * bit1 + 0x44*bit2 + 0x8f*bit3;
		palette_set_color(i,r,g,b);

		color_prom++;
	}
}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

/*
	Tile format
	-----------

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | ----xxxx xxxxxxxx | code
	  0	 | xxxx---- -------- | color
*/

static void get_tile_info_xorworld_screen(int tile_index)
{
	int data = xorworld_videoram[tile_index];
	int code = data & 0x0fff;

	SET_TILE_INFO(0, code, data >> 12, 0);
}


/***************************************************************************

	Memory Handlers

***************************************************************************/

READ16_HANDLER( xorworld_vram_r )
{
	return xorworld_videoram[offset];
}

WRITE16_HANDLER( xorworld_vram_w )
{
	COMBINE_DATA(&xorworld_videoram[offset]);
	tilemap_mark_tile_dirty(screen, offset);
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( xorworld )
{
	screen = tilemap_create(get_tile_info_xorworld_screen,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	if (!screen)
		return 1;

	return 0;
}

/*
	Sprite Format
	-------------

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | x position
	  0  | xxxxxxxx -------- | y position
	  1  | -------- ------xx | flipxy? (not used)
	  1  | ----xxxx xxxxxx-- | sprite number
	  1  | xxxx---- -------- | sprite color
*/

static void xorworld_draw_sprites(struct mame_bitmap *bitmap)
{
	int i;
	const struct GfxElement *gfx = Machine->gfx[1];

	for (i = 0; i < 0x40; i += 2){		
		int sx = xorworld_spriteram[i] & 0x00ff;
		int sy = 240 - (((xorworld_spriteram[i] & 0xff00) >> 8) & 0xff);
		int number = (xorworld_spriteram[i+1] & 0x0ffc) >> 2;
		int color = (xorworld_spriteram[i+1] & 0xf000) >> 12;

		drawgfx(bitmap,gfx,number,
						color,0,0,
						sx,sy,
						&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

VIDEO_UPDATE( xorworld )
{	
	tilemap_draw(bitmap, cliprect, screen, 0, 0);

	xorworld_draw_sprites(bitmap);
}
