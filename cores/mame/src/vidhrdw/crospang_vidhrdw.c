/*

  Cross Pang
  video hardware emulation

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_layer,*fg_layer;
data16_t *crospang_bg_videoram,*crospang_fg_videoram;

WRITE16_HANDLER ( crospang_fg_scrolly_w )
{
	tilemap_set_scrolly(fg_layer,0,data+8);
}

WRITE16_HANDLER ( crospang_bg_scrolly_w )
{
	tilemap_set_scrolly(bg_layer,0,data+8);
}

WRITE16_HANDLER ( crospang_fg_scrollx_w )
{
	tilemap_set_scrollx(fg_layer,0,data);
}

WRITE16_HANDLER ( crospang_bg_scrollx_w )
{
	tilemap_set_scrollx(bg_layer,0,data+4);
}

WRITE16_HANDLER ( crospang_fg_videoram_w )
{
	COMBINE_DATA(&crospang_fg_videoram[offset]);
	tilemap_mark_tile_dirty(fg_layer,offset);
}

WRITE16_HANDLER ( crospang_bg_videoram_w )
{
	COMBINE_DATA(&crospang_bg_videoram[offset]);
	tilemap_mark_tile_dirty(bg_layer,offset);
}

static void get_bg_tile_info(int tile_index)
{
	int data  = crospang_bg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1,tile,color + 0x20,0)
}

static void get_fg_tile_info(int tile_index)
{
	int data  = crospang_fg_videoram[tile_index];
	int tile  = data & 0xfff;
	int color = (data >> 12) & 0x0f;

	SET_TILE_INFO(1,tile,color + 0x10,0)
}

/*

 offset

	  0		-------yyyyyyyyy  y offset
			-----hh---------  sprite height
			---a------------  alpha blending enable
			f---------------  flip x
			-??-?-----------  unused

	  1		--ssssssssssssss  sprite code
			??--------------  unused

	  2		-------xxxxxxxxx  x offset
			---cccc---------  colors
			???-------------  unused

	  3		----------------  unused

*/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs,fx,fy,x,y,color,sprite,attr,dy,ay,flag;

	for (offs = 0; offs < spriteram_size/2; offs += 4)
	{
		y = spriteram16[offs+0];
		x = spriteram16[offs+2];
		sprite = spriteram16[offs+1];
		attr = spriteram16[offs+3];

		fy = 0;

		dy = 1 << ((y & 0x0600) >> 9);

		color = (x & 0x1e00) >> 9;
		fx = (y & 0x8000) >> 15;

		if (y & 0x1000)
			flag = TRANSPARENCY_ALPHA;
		else
			flag = TRANSPARENCY_PEN;

		x &= 0x1ff;
		y &= 0x1ff;

		if (x & 0x100)
		{
			x -= 0x200;
		
			if(x < -32) 
				x += 512;
		}

		if (y & 0x100)
			y -= 0x200;

		x -= 44;
		y += 8;

		sprite &= 0x3fff;

		y += dy*16;

		for (ay=0; ay<dy; ay++)
		{
			drawgfx(bitmap,Machine->gfx[0],
				sprite++,
				color,(1-fx),fy,0x100-x, (0x100-(y - ay * 16)),
				cliprect,flag,0);
		}
	}
}

VIDEO_START( crospang )
{
	bg_layer = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,16,16,32,32);
	fg_layer = tilemap_create(get_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

	if(!bg_layer || !fg_layer)
		return 1;

	tilemap_set_transparent_pen(fg_layer,0);

	alpha_set_level(0x80);

	return 0;
}

VIDEO_UPDATE( crospang )
{
	tilemap_draw(bitmap,cliprect,bg_layer,0,0);
	tilemap_draw(bitmap,cliprect,fg_layer,0,0);
	draw_sprites(bitmap,cliprect);
}
