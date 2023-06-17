#include "driver.h"
#include "state.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1


/**********************************************************/

int asuka_core_video_start(int x_offs,int buffering)
{
	if (PC090OJ_vh_start(0,0,8,buffering))	/* gfxset, x offset, y offset, buffering */
		return 1;

	if (TC0100SCN_vh_start(1,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,0))
		return 1;

	if (TC0110PCR_vh_start())
		return 1;

	return 0;
}

VIDEO_START( asuka )
{
	return (asuka_core_video_start(0,0));
}

VIDEO_START( galmedes )
{
	return (asuka_core_video_start(1,0));
}

VIDEO_START( cadash )
{
	return (asuka_core_video_start(1,1));
}

/**************************************************************
                 SPRITE READ AND WRITE HANDLERS
**************************************************************/

WRITE16_HANDLER( asuka_spritectrl_w )
{
	/* Bits 2-5 are color bank; in asuka games bit 0 is global priority */
	PC090OJ_sprite_ctrl = ((data & 0x3c) >> 2) | ((data & 0x1) << 15);
}


/**************************************************************
                        SCREEN REFRESH
**************************************************************/

VIDEO_UPDATE( asuka )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	/* Sprites may be over or under top bg layer */
	PC090OJ_draw_sprites(bitmap,cliprect,2);
}


VIDEO_UPDATE( bonzeadv )
{
	UINT8 layer[3];

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	fillbitmap(priority_bitmap,0,cliprect);

	/* Ensure screen blanked even when bottom layer not drawn due to disable bit */
	fillbitmap(bitmap, Machine->pens[0], cliprect);

	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,1);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,2);
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,4);

	/* Sprites are always over both bg layers */
	PC090OJ_draw_sprites(bitmap,cliprect,0);
}
