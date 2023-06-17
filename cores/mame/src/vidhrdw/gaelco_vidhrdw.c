/***************************************************************************

  Gaelco Type 1 Video Hardware

  Functions to emulate the video hardware of the machine

***************************************************************************/

#include "driver.h"
#include "tilemap.h"
#include "vidhrdw/generic.h"

data16_t *gaelco_vregs;
data16_t *gaelco_videoram;
data16_t *gaelco_spriteram;

int sprite_count[5];
int *sprite_table[5];
static struct tilemap *gaelco_tilemap[2];

#define pant	gaelco_tilemap

/* lazy*/
void gaelco_mark_offset_dirty(unsigned int offset)
{
	tilemap_mark_tile_dirty(pant[offset >> 11],((offset << 1) & 0x0fff) >> 2);
}

/***************************************************************************

	Callbacks for the TileMap code

***************************************************************************/

/*
	Tile format
	-----------

	Screen 0 & 1: (32*32, 16x16 tiles)

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- -------x | flip x
	  0  | -------- ------x- | flip y
	  0  | xxxxxxxx xxxxxx-- | code
	  1  | -------- --xxxxxx | color
	  1	 | -------- xx------ | priority
	  1  | xxxxxxxx -------- | not used
*/

static void get_tile_info_gaelco_screen0(int tile_index)
{
	int data = gaelco_videoram[tile_index << 1];
	int data2 = gaelco_videoram[(tile_index << 1) + 1];
	int code = ((data & 0xfffc) >> 2);

	tile_info.priority = (data2 >> 6) & 0x03;

	SET_TILE_INFO(1, 0x4000 + code, data2 & 0x3f, TILE_FLIPYX(data & 0x03))
}


static void get_tile_info_gaelco_screen1(int tile_index)
{
	int data = gaelco_videoram[(0x1000/2) + (tile_index << 1)];
	int data2 = gaelco_videoram[(0x1000/2) + (tile_index << 1) + 1];
	int code = ((data & 0xfffc) >> 2);

	tile_info.priority = (data2 >> 6) & 0x03;

	SET_TILE_INFO(1, 0x4000 + code, data2 & 0x3f, TILE_FLIPYX(data & 0x03))
}

/***************************************************************************

	Memory Handlers

***************************************************************************/

WRITE16_HANDLER( gaelco_vram_w )
{
	int oldword = gaelco_videoram[offset];
	COMBINE_DATA(&gaelco_videoram[offset]);

	if (oldword != gaelco_videoram[offset])
		tilemap_mark_tile_dirty(pant[offset >> 11],((offset << 1) & 0x0fff) >> 2);
}

/***************************************************************************

	Start/Stop the video hardware emulation.

***************************************************************************/

VIDEO_START( bigkarnk )
{
	int i;

	pant[0] = tilemap_create(get_tile_info_gaelco_screen0,tilemap_scan_rows,TILEMAP_SPLIT,16,16,32,32);
	pant[1] = tilemap_create(get_tile_info_gaelco_screen1,tilemap_scan_rows,TILEMAP_SPLIT,16,16,32,32);

	if (!pant[0] || !pant[1])
		return 1;

	tilemap_set_transmask(pant[0],0,0xff01,0x00ff); /* pens 1-7 opaque, pens 0, 8-15 transparent */
	tilemap_set_transmask(pant[1],0,0xff01,0x00ff); /* pens 1-7 opaque, pens 0, 8-15 transparent */

	for (i = 0; i < 5; i++){
		sprite_table[i] = auto_malloc(512*sizeof(int));
		if (!sprite_table[i]){
			return 1;
		}
	}

	return 0;
}

VIDEO_START( maniacsq )
{
	int i;

	pant[0] = tilemap_create(get_tile_info_gaelco_screen0,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	pant[1] = tilemap_create(get_tile_info_gaelco_screen1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if (!pant[0] || !pant[1])
		return 1;

	tilemap_set_transparent_pen(pant[0],0);
	tilemap_set_transparent_pen(pant[1],0);

	for (i = 0; i < 5; i++){
		sprite_table[i] = auto_malloc(512*sizeof(int));
		if (!sprite_table[i]){
			return 1;
		}
	}

	return 0;
}


/***************************************************************************

	Sprites

***************************************************************************/

static void gaelco_sort_sprites(void)
{
	int i;

	sprite_count[0] = 0;
	sprite_count[1] = 0;
	sprite_count[2] = 0;
	sprite_count[3] = 0;
	sprite_count[4] = 0;

	for (i = 3; i < (0x1000 - 6)/2; i += 4){
		int color = (gaelco_spriteram[i+2] & 0x7e00) >> 9;
		int priority = (gaelco_spriteram[i] & 0x3000) >> 12;

		/* palettes 0x38-0x3f are used for high priority sprites in Big Karnak */
		if (color >= 0x38){
			sprite_table[4][sprite_count[4]] = i;
			sprite_count[4]++;
		}

		/* save sprite number in the proper array for later */
		sprite_table[priority][sprite_count[priority]] = i;
		sprite_count[priority]++;
	}
}

/*
	Sprite Format
	-------------

	Word | Bit(s)			 | Description
	-----+-FEDCBA98-76543210-+--------------------------
	  0  | -------- xxxxxxxx | y position
	  0  | -----xxx -------- | not used
	  0  | ----x--- -------- | sprite size
	  0  | --xx---- -------- | sprite priority
	  0  | -x------ -------- | flipx
	  0  | x------- -------- | flipy
	  1  | xxxxxxxx xxxxxxxx | not used
	  2  | -------x xxxxxxxx | x position
	  2  | -xxxxxx- -------- | sprite color
	  3	 | -------- ------xx | sprite code (8x8 cuadrant)
	  3  | xxxxxxxx xxxxxx-- | sprite code
*/

static void gaelco_draw_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int pri)
{
	int j, x, y, ex, ey;
	const struct GfxElement *gfx = Machine->gfx[0];

	static int x_offset[2] = {0x0,0x2};
	static int y_offset[2] = {0x0,0x1};

	for (j = 0; j < sprite_count[pri]; j++){
		int i = sprite_table[pri][j];
		int sx = gaelco_spriteram[i+2] & 0x01ff;
		int sy = (240 - (gaelco_spriteram[i] & 0x00ff)) & 0x00ff;
		int number = gaelco_spriteram[i+3];
		int color = (gaelco_spriteram[i+2] & 0x7e00) >> 9;
		int attr = (gaelco_spriteram[i] & 0xfe00) >> 9;

		int xflip = attr & 0x20;
		int yflip = attr & 0x40;
		int spr_size;

		if (attr & 0x04){
			spr_size = 1;
		}
		else{
			spr_size = 2;
			number &= (~3);
		}

		for (y = 0; y < spr_size; y++){
			for (x = 0; x < spr_size; x++){

				ex = xflip ? (spr_size-1-x) : x;
				ey = yflip ? (spr_size-1-y) : y;

				drawgfx(bitmap,gfx,number + x_offset[ex] + y_offset[ey],
						color,xflip,yflip,
						sx-0x0f+x*8,sy+y*8,
						cliprect,TRANSPARENCY_PEN,0);
			}
		}
	}
}

/***************************************************************************

	Display Refresh

***************************************************************************/

VIDEO_UPDATE( maniacsq )
{
	/* set scroll registers */
	tilemap_set_scrolly(pant[0], 0, gaelco_vregs[0]);
	tilemap_set_scrollx(pant[0], 0, gaelco_vregs[1]+4);
	tilemap_set_scrolly(pant[1], 0, gaelco_vregs[2]);
	tilemap_set_scrollx(pant[1], 0, gaelco_vregs[3]);

	gaelco_sort_sprites();


	fillbitmap( bitmap, Machine->pens[0], cliprect );

	tilemap_draw(bitmap,cliprect,pant[1],3,0);
	tilemap_draw(bitmap,cliprect,pant[0],3,0);
	gaelco_draw_sprites(bitmap,cliprect,3);

	tilemap_draw(bitmap,cliprect,pant[1],2,0);
	tilemap_draw(bitmap,cliprect,pant[0],2,0);
	gaelco_draw_sprites(bitmap,cliprect,2);

	tilemap_draw(bitmap,cliprect,pant[1],1,0);
	tilemap_draw(bitmap,cliprect,pant[0],1,0);
	gaelco_draw_sprites(bitmap,cliprect,1);

	tilemap_draw(bitmap,cliprect,pant[1],0,0);
	tilemap_draw(bitmap,cliprect,pant[0],0,0);
	gaelco_draw_sprites(bitmap,cliprect,0);
}

VIDEO_UPDATE( bigkarnk )
{
	/* set scroll registers */
	tilemap_set_scrolly(pant[0], 0, gaelco_vregs[0]);
	tilemap_set_scrollx(pant[0], 0, gaelco_vregs[1]+4);
	tilemap_set_scrolly(pant[1], 0, gaelco_vregs[2]);
	tilemap_set_scrollx(pant[1], 0, gaelco_vregs[3]);

	gaelco_sort_sprites();


	fillbitmap( bitmap, Machine->pens[0], cliprect );

	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_BACK | 3,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_BACK | 3,0);
	gaelco_draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_FRONT | 3,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_FRONT | 3,0);

	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_BACK | 2,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_BACK | 2,0);
	gaelco_draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_FRONT | 2,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_FRONT | 2,0);

	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_BACK | 1,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_BACK | 1,0);
	gaelco_draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_FRONT | 1,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_FRONT | 1,0);

	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_BACK | 0,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_BACK | 0,0);
	gaelco_draw_sprites(bitmap,cliprect,0);
	tilemap_draw(bitmap,cliprect,pant[1],TILEMAP_FRONT | 0,0);
	tilemap_draw(bitmap,cliprect,pant[0],TILEMAP_FRONT | 0,0);

	gaelco_draw_sprites(bitmap,cliprect,4);
}
