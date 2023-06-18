/***************************************************************************

  Target Hits Video Hardware

  Functions to emulate the video hardware of the machine

***************************************************************************/

#include "driver.h"
#include "tilemap.h"
#include "vidhrdw/generic.h"

data16_t *targeth_spriteram;
data16_t *targeth_vregs;
data16_t *targeth_videoram;

static struct tilemap *pant[2];


/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

/*
	Tile format
	-----------

	Screen 0 & 1: (64*32, 16x16 tiles)

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | --xxxxxx xxxxxxxx | code
	  0  | xx------ -------- | not used?
	  1  | -------- ---xxxxx | color (uses 1st half of the palette)
	  1  | -------- --x----- | flip y
	  1  | -------- -x------ | flip x
	  1  | xxxxxxxx x------- | not used?
*/

static void get_tile_info_targeth_screen0(int tile_index)
{
	int data = targeth_videoram[tile_index << 1];
	int data2 = targeth_videoram[(tile_index << 1) + 1];
	int code = data & 0x3fff;

	SET_TILE_INFO(0, code, data2 & 0x1f, TILE_FLIPXY((data2 >> 5) & 0x03))
}

static void get_tile_info_targeth_screen1(int tile_index)
{
	int data = targeth_videoram[(0x2000/2) + (tile_index << 1)];
	int data2 = targeth_videoram[(0x2000/2) + (tile_index << 1) + 1];
	int code = data & 0x3fff;

	SET_TILE_INFO(0, code, data2 & 0x1f, TILE_FLIPXY((data2 >> 5) & 0x03))
}

/***************************************************************************

	Memory Handlers

***************************************************************************/

WRITE16_HANDLER( targeth_vram_w )
{
	targeth_videoram[offset] = data;

	tilemap_mark_tile_dirty(pant[(offset & 0x1fff) >> 12], ((offset << 1) & 0x1fff) >> 2);
}


/***************************************************************************

	Start/Stop the video hardware emulation.

***************************************************************************/

VIDEO_START( targeth )
{
	pant[0] = tilemap_create(get_tile_info_targeth_screen0,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	pant[1] = tilemap_create(get_tile_info_targeth_screen1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);

	if (!pant[0] || !pant[1])
		return 1;

	tilemap_set_transparent_pen(pant[0],0);

	return 0;
}


/***************************************************************************

	Sprites

***************************************************************************/

/*
	Sprite Format
	-------------

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | y position
	  0  | --xxxxxx -------- | not used?
	  0  | -x------ -------- | flipx
	  0  | x------- -------- | flipy
	  1  | xxxxxxxx xxxxxxxx | not used?
	  2  | ------xx xxxxxxxx | x position
	  2  | -xxxxx-- -------- | sprite color (uses 2nd half of the palette)
	  3  | --xxxxxx xxxxxxxx | sprite code
	  3	 | xx------ -------- | not used?
*/

static void targeth_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect)
{
	int i;
	const struct GfxElement *gfx = Machine->gfx[0];

	for (i = 3; i < (0x1000 - 6)/2; i += 4){
		int sx = targeth_spriteram[i+2] & 0x03ff;
		int sy = (240 - (targeth_spriteram[i] & 0x00ff)) & 0x00ff;
		int number = targeth_spriteram[i+3] & 0x3fff;
		int color = (targeth_spriteram[i+2] & 0x7c00) >> 10;
		int attr = (targeth_spriteram[i] & 0xfe00) >> 9;

		int xflip = attr & 0x20;
		int yflip = attr & 0x40;

		drawgfx(bitmap,gfx,number,
				0x20 + color,xflip,yflip,
				sx - 0x0f,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

VIDEO_UPDATE( targeth )
{
	/* set scroll registers */
	tilemap_set_scrolly(pant[0], 0, targeth_vregs[0]);
	tilemap_set_scrollx(pant[0], 0, targeth_vregs[1] + 0x04);
	tilemap_set_scrolly(pant[1], 0, targeth_vregs[2]);
	tilemap_set_scrollx(pant[1], 0, targeth_vregs[3]);

	tilemap_draw(bitmap,cliprect,pant[1],0,0);
	tilemap_draw(bitmap,cliprect,pant[0],0,0);
	targeth_draw_sprites(bitmap,cliprect);

	/* draw crosshairs */
	{
		int posx, posy;

		/* 1P Gun */
		posx = readinputport(0) & 0x1ff;
		posy = readinputport(1) & 0x0ff;
		draw_crosshair(1, bitmap, posx - 0x17, posy + 1, cliprect);

		/* 2P Gun */
		posx = readinputport(2) & 0x1ff;
		posy = readinputport(3) & 0x0ff;
		draw_crosshair(2, bitmap, posx - 0x17, posy + 1, cliprect);
	}
}
