#include "driver.h"
#include "vidhrdw/konamiic.h"



static int layer_colorbase[3],sprite_colorbase;

/***************************************************************************

  Callbacks for the K052109

***************************************************************************/

static void tile_callback(int layer,int bank,int *code,int *color)
{
	*code |= ((*color & 0x0f) << 8);
	*color = layer_colorbase[layer] + ((*color & 0xe0) >> 5);
}

/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority,int *shadow)
{
	*priority = (*color & 0x10) >> 4;
	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( blockhl )
{
	layer_colorbase[0] = 0;
	layer_colorbase[1] = 16;
	layer_colorbase[2] = 32;
	sprite_colorbase = 48;

	if (K052109_vh_start(REGION_GFX1,NORMAL_PLANE_ORDER,tile_callback))
		return 1;
	if (K051960_vh_start(REGION_GFX2,NORMAL_PLANE_ORDER,sprite_callback))
		return 1;

	return 0;
}


VIDEO_UPDATE( blockhl )
{
	K052109_tilemap_update();

	tilemap_draw(bitmap,cliprect,K052109_tilemap[2],TILEMAP_IGNORE_TRANSPARENCY,0);
	K051960_sprites_draw(bitmap,cliprect,1,1);
	tilemap_draw(bitmap,cliprect,K052109_tilemap[1],0,0);
	K051960_sprites_draw(bitmap,cliprect,0,0);
	tilemap_draw(bitmap,cliprect,K052109_tilemap[0],0,0);
}
