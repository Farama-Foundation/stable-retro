/***************************************************************************

	Legionnaire / Heated Barrel video hardware (derived from D-Con)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

data16_t *legionna_back_data,*legionna_fore_data,*legionna_mid_data,*legionna_scrollram16,*legionna_textram;

static struct tilemap *background_layer,*foreground_layer,*midground_layer,*text_layer;
/*static int legionna_enable;*/

/******************************************************************************/

static UINT16 gfx_bank = 0;

void heatbrl_setgfxbank(UINT16 data)
{
	gfx_bank = (data &0x4000) >> 2;
}


WRITE16_HANDLER( legionna_control_w )
{
#if 0
	if (ACCESSING_LSB)
	{
		legionna_enable=data;
		if ((legionna_enable&4)==4)
			tilemap_set_enable(foreground_layer,0);
		else
			tilemap_set_enable(foreground_layer,1);

		if ((legionna_enable&2)==2)
			tilemap_set_enable(midground_layer,0);
		else
			tilemap_set_enable(midground_layer,1);

		if ((legionna_enable&1)==1)
			tilemap_set_enable(background_layer,0);
		else
			tilemap_set_enable(background_layer,1);
	}
#endif
}

WRITE16_HANDLER( legionna_background_w )
{
	int oldword = legionna_back_data[offset];
	COMBINE_DATA(&legionna_back_data[offset]);
	if (oldword != legionna_back_data[offset])
		tilemap_mark_tile_dirty(background_layer,offset);
}

WRITE16_HANDLER( legionna_midground_w )
{
	int oldword = legionna_mid_data[offset];
	COMBINE_DATA(&legionna_mid_data[offset]);
	if (oldword != legionna_mid_data[offset])
		tilemap_mark_tile_dirty(midground_layer,offset);
}

WRITE16_HANDLER( legionna_foreground_w )
{
	int oldword = legionna_fore_data[offset];
	COMBINE_DATA(&legionna_fore_data[offset]);
	if (oldword != legionna_fore_data[offset])
		tilemap_mark_tile_dirty(foreground_layer,offset);
}

WRITE16_HANDLER( legionna_text_w )
{
	int oldword = legionna_textram[offset];
	COMBINE_DATA(&legionna_textram[offset]);
	if (oldword != legionna_textram[offset])
		tilemap_mark_tile_dirty(text_layer,offset);
}

static void get_back_tile_info(int tile_index)
{
	int tile=legionna_back_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;
	tile |= gfx_bank;		/* Heatbrl uses banking */

	SET_TILE_INFO(1,tile,color,0)
}

static void get_mid_tile_info(int tile_index)
{
	int tile=legionna_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	SET_TILE_INFO(5,tile,color,0)
}

static void get_mid_tile_info_cupsoc(int tile_index)
{
	int tile=legionna_mid_data[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	tile |= 0x1000;

	SET_TILE_INFO(1,tile,color,0)
}

static void get_fore_tile_info(int tile_index)	/* this is giving bad tiles... */
{
	int tile=legionna_fore_data[tile_index];
	int color=(tile>>12)&0xf;

	/* legionnaire tile numbers / gfx set wrong, see screen after coin insertion*/
	tile &= 0xfff;

	SET_TILE_INFO(4,tile,color,0)
}

static void get_text_tile_info(int tile_index)
{
	int tile = legionna_textram[tile_index];
	int color=(tile>>12)&0xf;

	tile &= 0xfff;

	SET_TILE_INFO(0,tile,color,0)
}

VIDEO_START( legionna )
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	legionna_scrollram16 = auto_malloc(0x60);

	if (!legionna_scrollram16)	return 1;

	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}

VIDEO_START( cupsoc )
{
	background_layer = tilemap_create(get_back_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	foreground_layer = tilemap_create(get_fore_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	midground_layer =  tilemap_create(get_mid_tile_info_cupsoc, tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	text_layer =       tilemap_create(get_text_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,  8,8,64,32);

	if (!background_layer || !foreground_layer || !midground_layer || !text_layer)
		return 1;

	legionna_scrollram16 = auto_malloc(0x60);

	if (!legionna_scrollram16)	return 1;

	tilemap_set_transparent_pen(background_layer,15);
	tilemap_set_transparent_pen(midground_layer,15);
	tilemap_set_transparent_pen(foreground_layer,15);
	tilemap_set_transparent_pen(text_layer,15);

	return 0;
}


/*************************************************************************

	Legionnaire Spriteram (similar to Dcon)
	---------------------

	It has "big sprites" created by setting width or height >0. Tile
	numbers are read consecutively.

    +0   x....... ........  Sprite enable
	+0   .x...... ........  Flip x
	+0   ..x..... ........  Flip y ???
	+0   ...xxx.. ........  Width: do this many tiles horizontally
	+0   ......xx x.......  Height: do this many tiles vertically
	+0   ........ .?......  unused ?
 	+0   ........ ..xxxxxx  Color bank

	+1   .x...... ........  Priority? (1=high?)
	+1   ..xxxxxx xxxxxxxx  Tile number
	+2   xxxxxxxx xxxxxxxx  X coordinate (signed)
	+3   xxxxxxxx xxxxxxxx  Y coordinate (signed)

*************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int pri)
{
	int offs,fx,fy,x,y,color,sprite;
	int dx,dy,ax,ay;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		UINT16 data = spriteram16[offs];
		if (!(data &0x8000)) continue;

		sprite = spriteram16[offs+1];
		if ((sprite>>14)!=pri) continue;

		sprite &= 0x3fff;

		y = spriteram16[offs+3];
		x = spriteram16[offs+2];

		if (x &0x8000)	x = -(0x200-(x &0x1ff));
		else	x &= 0x1ff;
		if (y &0x8000)	y = -(0x200-(y &0x1ff));
		else	y &= 0x1ff;

		color = (data &0x3f) + 0x40;
		fx =  (data &0x4000) >> 14;
		fy =  (data &0x2000) >> 13;	/* ??? */
		dy = ((data &0x0380) >> 7)  + 1;
		dx = ((data &0x1c00) >> 10) + 1;

		if (!fx)
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[3],
					sprite++,
					color,fx,fy,x+ax*16,y+ay*16,
					cliprect,TRANSPARENCY_PEN,15);
				}
		}
		else
		{
			for (ax=0; ax<dx; ax++)
				for (ay=0; ay<dy; ay++)
				{
					drawgfx(bitmap,Machine->gfx[3],
					sprite++,
					color,fx,fy,x+(dx-ax-1)*16,y+ay*16,
					cliprect,TRANSPARENCY_PEN,15);
				}
		}
	}
}


VIDEO_UPDATE( legionna )
{
#ifdef MAME_DEBUG
	static int dislayer[5];	/* Layer toggles to help get the layers correct */
#endif

#ifdef MAME_DEBUG
	if (keyboard_pressed_memory (KEYCODE_Z))
	{
		dislayer[0] ^= 1;
		usrintf_showmessage("bg0: %01x",dislayer[0]);
	}

	if (keyboard_pressed_memory (KEYCODE_X))
	{
		dislayer[1] ^= 1;
		usrintf_showmessage("bg1: %01x",dislayer[1]);
	}

	if (keyboard_pressed_memory (KEYCODE_C))
	{
		dislayer[2] ^= 1;
		usrintf_showmessage("bg2: %01x",dislayer[2]);
	}

	if (keyboard_pressed_memory (KEYCODE_V))
	{
		dislayer[3] ^= 1;
		usrintf_showmessage("sprites: %01x",dislayer[3]);
	}

	if (keyboard_pressed_memory (KEYCODE_B))
	{
		dislayer[4] ^= 1;
		usrintf_showmessage("text: %01x",dislayer[4]);
	}
#endif

	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer, 0, legionna_scrollram16[0] );
	tilemap_set_scrolly( background_layer, 0, legionna_scrollram16[1] );
	tilemap_set_scrollx( midground_layer,  0, legionna_scrollram16[2] );
	tilemap_set_scrolly( midground_layer,  0, legionna_scrollram16[3] );
	tilemap_set_scrollx( foreground_layer, 0, legionna_scrollram16[4] );
	tilemap_set_scrolly( foreground_layer, 0, legionna_scrollram16[5] );

/*	if ((legionna_enable&1)!=1)*/

	fillbitmap(bitmap,get_black_pen(),cliprect);	/* wrong color? */

#ifdef MAME_DEBUG
	if (dislayer[2]==0)
#endif
	tilemap_draw(bitmap,cliprect,foreground_layer,TILEMAP_IGNORE_TRANSPARENCY,0);

#ifdef MAME_DEBUG
	if (dislayer[1]==0)
#endif
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);

#ifdef MAME_DEBUG
	if (dislayer[0]==0)
#endif
	tilemap_draw(bitmap,cliprect,background_layer,0,0);

	draw_sprites(bitmap,cliprect,2);
	draw_sprites(bitmap,cliprect,1);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);

#ifdef MAME_DEBUG
	if (dislayer[4]==0)
#endif
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}

VIDEO_UPDATE( godzilla )
{
	tilemap_set_scrollx( text_layer, 0, 0 );
	tilemap_set_scrolly( text_layer, 0, 112 );

	fillbitmap(bitmap,get_black_pen(),cliprect);

	tilemap_draw(bitmap,cliprect,background_layer,0,0);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);
	draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,foreground_layer,0,0);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}

VIDEO_UPDATE( sdgndmrb )
{
	/* Setup the tilemaps */
	tilemap_set_scrollx( background_layer, 0, legionna_scrollram16[0] );
	tilemap_set_scrolly( background_layer, 0, legionna_scrollram16[1] );
	tilemap_set_scrollx( midground_layer,  0, legionna_scrollram16[2] );
	tilemap_set_scrolly( midground_layer,  0, legionna_scrollram16[3] );
	tilemap_set_scrollx( foreground_layer, 0, legionna_scrollram16[4] );
	tilemap_set_scrolly( foreground_layer, 0, legionna_scrollram16[5] );
/*	tilemap_set_scrollx( text_layer,       0, 128                     ); // set to 128 instead of legionna_scrollram16[6] */
/*	tilemap_set_scrolly( text_layer,       0, 0                       ); // set to 0 instead of legionna_scrollram16[7] */

	fillbitmap(bitmap,get_black_pen(),cliprect);

	tilemap_draw(bitmap,cliprect,background_layer,0,0);
	draw_sprites(bitmap,cliprect,2);
	tilemap_draw(bitmap,cliprect,midground_layer,0,0);
	draw_sprites(bitmap,cliprect,1);
	tilemap_draw(bitmap,cliprect,foreground_layer,0,0);
	draw_sprites(bitmap,cliprect,0);
	draw_sprites(bitmap,cliprect,3);
	tilemap_draw(bitmap,cliprect,text_layer,0,0);
}
