#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"

#define TC0100SCN_GFX_NUM 1

struct tempsprite
{
	int gfx;
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int primask;
};
static struct tempsprite *spritelist;



/**********************************************************/

static int warriorb_core_vh_start (int x_offs,int multiscrn_xoffs)
{
	int chips;

	spritelist = auto_malloc(0x800 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	chips = number_of_TC0100SCN();

	if (chips <= 0)	/* we have an erroneous TC0100SCN configuration */
		return 1;

	if (TC0100SCN_vh_start(chips,TC0100SCN_GFX_NUM,x_offs,0,0,0,0,0,multiscrn_xoffs))
		return 1;

	if (has_TC0110PCR())
		if (TC0110PCR_vh_start())
			return 1;

	if (has_second_TC0110PCR())
		if (TC0110PCR_1_vh_start())
			return 1;

	/* Ensure palette from correct TC0110PCR used for each screen */
	TC0100SCN_set_chip_colbanks(0,0x100,0x0);

	return 0;
}

VIDEO_START( darius2d )
{
	return (warriorb_core_vh_start(4,0));
}

VIDEO_START( warriorb )
{
	return (warriorb_core_vh_start(4,1));
}


/************************************************************
			SPRITE DRAW ROUTINE
************************************************************/

static void warriorb_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int primask,int y_offs)
{
	int offs, data, data2, tilenum, color, flipx, flipy;
	int x, y, priority, curx, cury;
	int code;

#ifdef MAME_DEBUG
	int unknown=0;
#endif

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = (spriteram_size/2)-4;offs >=0;offs -= 4)
	{
		data = spriteram16[offs+1];
		tilenum = data & 0x7fff;

		if (!tilenum) continue;

		data = spriteram16[offs+0];
		y = (-(data &0x1ff) - 24) & 0x1ff;	/* (inverted y adjusted for vis area) */
		flipy = (data & 0x200) >> 9;

		data2 = spriteram16[offs+2];
		/* 8,4 also seen in msbyte */
		priority = (data2 & 0x0100) >> 8; /* 1 = low*/
		if (priority != primask) continue;
		color    = (data2 & 0x7f);

		data = spriteram16[offs+3];
		x = (data & 0x3ff);
		flipx = (data & 0x400) >> 10;


#ifdef MAME_DEBUG
		if (data2 & 0xf280)   unknown |= (data2 &0xf280);
#endif

		y += y_offs;

		/* sprite wrap: coords become negative at high values */
		if (x>0x3c0) x -= 0x400;
		if (y>0x180) y -= 0x200;

		curx = x;
		cury = y;
		code = tilenum;

		sprite_ptr->code = code;
		sprite_ptr->color = color;
		sprite_ptr->flipx = flipx;
		sprite_ptr->flipy = flipy;
		sprite_ptr->x = curx;
		sprite_ptr->y = cury;

		drawgfx(bitmap,Machine->gfx[0],
				sprite_ptr->code,
				sprite_ptr->color,
				sprite_ptr->flipx,sprite_ptr->flipy,
				sprite_ptr->x,sprite_ptr->y,
				cliprect,TRANSPARENCY_PEN,0);
	}

#ifdef MAME_DEBUG
	if (unknown)
		usrintf_showmessage("unknown sprite bits: %04x",unknown);
#endif
}


/**************************************************************
				SCREEN REFRESH
**************************************************************/

VIDEO_UPDATE( warriorb )
{
	UINT8 layer[3], nodraw;

	TC0100SCN_tilemap_update();

	layer[0] = TC0100SCN_bottomlayer(0);
	layer[1] = layer[0]^1;
	layer[2] = 2;

	/* chip 0 does tilemaps on the left, chip 1 does the ones on the right */
	/* draw bottom layer*/
	nodraw  = TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);	/* left */
	nodraw |= TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[0],TILEMAP_IGNORE_TRANSPARENCY,0);	/* right */

	/* Ensure screen blanked even when bottom layers not drawn due to disable bit */
	if (nodraw) fillbitmap(bitmap, get_black_pen(), cliprect);

	/* Sprites can be under/over the layer below text layer */
	warriorb_draw_sprites(bitmap,cliprect,1,8); /* draw sprites with priority 1 which are under the mid layer*/

	/* draw middle layer*/
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[1],0,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[1],0,0);

	warriorb_draw_sprites(bitmap,cliprect,0,8); /* draw sprites with priority 0 which are over the mid layer*/

	/* draw top(text) layer*/
	TC0100SCN_tilemap_draw(bitmap,cliprect,0,layer[2],0,0);
	TC0100SCN_tilemap_draw(bitmap,cliprect,1,layer[2],0,0);
}
