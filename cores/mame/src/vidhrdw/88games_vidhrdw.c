#include "driver.h"
#include "vidhrdw/konamiic.h"


int k88games_priority;

static int layer_colorbase[3],sprite_colorbase,zoom_colorbase;



/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x0f) << 8) | (bank << 12);
	*color = layer_colorbase[layer] + ((*color & 0xf0) >> 4);
}


/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority,int *shadow)
{
	*priority = (*color & 0x20) >> 5;	/* ??? */
	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

static void zoom_callback(int *code,int *color)
{
	tile_info.flags = (*color & 0x40) ? TILE_FLIPX : 0;
	*code |= ((*color & 0x07) << 8);
	*color = zoom_colorbase + ((*color & 0x38) >> 3) + ((*color & 0x80) >> 4);
}


/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( 88games )
{
	layer_colorbase[0] = 64;
	layer_colorbase[1] = 0;
	layer_colorbase[2] = 16;
	sprite_colorbase = 32;
	zoom_colorbase = 48;
	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,sprite_callback))
		return 1;
	if (K051316_vh_start_0(REGION_GFX3,4,TILEMAP_TRANSPARENT,0,zoom_callback))
		return 1;

	return 0;
}



/***************************************************************************

  Display refresh

***************************************************************************/

VIDEO_UPDATE( 88games )
{
	K052109_tilemap_update();

	if (k88games_priority)
	{
		tilemap_draw(bitmap,cliprect,K052109_tilemap[0],TILEMAP_IGNORE_TRANSPARENCY,0);
		K051960_sprites_draw(bitmap,cliprect,1,1);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[2],0,0);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[1],0,0);
		K051960_sprites_draw(bitmap,cliprect,0,0);
		K051316_zoom_draw_0(bitmap,cliprect,0,0);
	}
	else
	{
		tilemap_draw(bitmap,cliprect,K052109_tilemap[2],TILEMAP_IGNORE_TRANSPARENCY,0);
		K051316_zoom_draw_0(bitmap,cliprect,0,0);
		K051960_sprites_draw(bitmap,cliprect,0,0);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[1],0,0);
		K051960_sprites_draw(bitmap,cliprect,1,1);
		tilemap_draw(bitmap,cliprect,K052109_tilemap[0],0,0);
	}
}
