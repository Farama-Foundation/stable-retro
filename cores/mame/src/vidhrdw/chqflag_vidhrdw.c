/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "cpu/z80/z80.h"

#define SPRITEROM_MEM_REGION REGION_GFX1
#define ZOOMROM0_MEM_REGION REGION_GFX2
#define ZOOMROM1_MEM_REGION REGION_GFX3

int sprite_colorbase,zoom_colorbase[2];

/***************************************************************************

  Callbacks for the K051960

***************************************************************************/

static void sprite_callback(int *code,int *color,int *priority,int *shadow)
{
	*priority = (*color & 0x10) >> 4;
	*color = sprite_colorbase + (*color & 0x0f);
}


/***************************************************************************

  Callbacks for the K051316

***************************************************************************/

static void zoom_callback_0(int *code,int *color)
{
	*code |= ((*color & 0x03) << 8);
	*color = zoom_colorbase[0] + ((*color & 0x3c) >> 2);
}

static void zoom_callback_1(int *code,int *color)
{
	tile_info.flags = TILE_FLIPYX((*color & 0xc0) >> 6);
	*code |= ((*color & 0x0f) << 8);
	*color = zoom_colorbase[1] + ((*color & 0x10) >> 4);
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

VIDEO_START( chqflag )
{
	sprite_colorbase = 0;
	zoom_colorbase[0] = 0x10;
	zoom_colorbase[1] = 0x02;

	if (K051960_vh_start(SPRITEROM_MEM_REGION,NORMAL_PLANE_ORDER,sprite_callback))
		return 1;

	if (K051316_vh_start_0(ZOOMROM0_MEM_REGION,4,TILEMAP_TRANSPARENT,0,zoom_callback_0))
		return 1;

	if (K051316_vh_start_1(ZOOMROM1_MEM_REGION,8,TILEMAP_SPLIT_PENBIT,0xc0,zoom_callback_1))
		return 1;

	K051316_set_offset(0,7,0);
	K051316_wraparound_enable(1,1);

	return 0;
}

/***************************************************************************

	Display Refresh

***************************************************************************/

VIDEO_UPDATE( chqflag )
{
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	K051316_zoom_draw_1(bitmap,cliprect,TILEMAP_BACK,0);
	K051960_sprites_draw(bitmap,cliprect,0,0);
	K051316_zoom_draw_1(bitmap,cliprect,TILEMAP_FRONT,0);
	K051960_sprites_draw(bitmap,cliprect,1,1);
	K051316_zoom_draw_0(bitmap,cliprect,0,0);
}
