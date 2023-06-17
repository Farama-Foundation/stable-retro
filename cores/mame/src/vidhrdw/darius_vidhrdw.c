#include "driver.h"
#include "state.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/taitoic.h"


static struct tilemap *fg_tilemap;
data16_t *darius_fg_ram;

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


/***************************************************************************/

static void actual_get_fg_tile_info(data16_t *ram,int gfxnum,int tile_index)
{
	UINT16 code = (ram[tile_index + 0x2000] & 0x7ff);
	UINT16 attr = ram[tile_index];

	SET_TILE_INFO(
			gfxnum,
			code,
			((attr & 0xff) << 2),
			TILE_FLIPYX((attr & 0xc000) >> 14))
}

static void get_fg_tile_info(int tile_index)
{
	actual_get_fg_tile_info(darius_fg_ram,2,tile_index);
}

static void (*darius_fg_get_tile_info[1])(int tile_index) =
{
	get_fg_tile_info
};

static void dirty_fg_tilemap(void)
{
	tilemap_mark_all_tiles_dirty(fg_tilemap);
}

/***************************************************************************/

VIDEO_START( darius )
{
	fg_tilemap = tilemap_create(darius_fg_get_tile_info[0],tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,128,64);
	if (!fg_tilemap)
		return 1;

	spritelist = auto_malloc(0x800 * sizeof(*spritelist));
	if (!spritelist)
		return 1;

	/* (chips, gfxnum, x_offs, y_offs, y_invert, opaque, dblwidth) */
	if ( PC080SN_vh_start(1,1,-16,8,0,1,1) )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	/* colors from saved states are often screwy (and this doesn't help...) */
	state_save_register_func_postload(dirty_fg_tilemap);

	return 0;
}

/***************************************************************************/

READ16_HANDLER( darius_fg_layer_r )
{
	return darius_fg_ram[offset];
}

WRITE16_HANDLER( darius_fg_layer_w )
{
	int oldword = darius_fg_ram[offset];

	COMBINE_DATA(&darius_fg_ram[offset]);
	if (oldword != darius_fg_ram[offset])
	{
		if (offset < 0x4000)
			tilemap_mark_tile_dirty(fg_tilemap,(offset & 0x1fff));
	}
}

/***************************************************************************/

void darius_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect,int primask, int y_offs)
{
	int offs,curx,cury;
	UINT16 code,data,sx,sy;
	UINT8 flipx,flipy,color,priority;

	/* pdrawgfx() needs us to draw sprites front to back, so we have to build a list
	   while processing sprite ram and then draw them all at the end */
	struct tempsprite *sprite_ptr = spritelist;

	for (offs = spriteram_size/2-4; offs >= 0; offs -= 4)
	{
		code = spriteram16[offs+2] &0x1fff;

		if (code)
		{
			data = spriteram16[offs];
			sy = (256-data) & 0x1ff;

			data = spriteram16[offs+1];
			sx = data & 0x3ff;

			data = spriteram16[offs+2];
			flipx = ((data & 0x4000) >> 14);
			flipy = ((data & 0x8000) >> 15);

			data = spriteram16[offs+3];
			priority = (data &0x80) >> 7;  /* 0 = low*/
			if (priority != primask) continue;
			color = (data & 0x7f);

			curx = sx;
			cury = sy + y_offs;

			if (curx > 900) curx -= 1024;
 			if (cury > 400) cury -= 512;

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
	}
}



VIDEO_UPDATE( darius )
{
	PC080SN_tilemap_update();

	/* draw bottom layer(always active)*/
 	PC080SN_tilemap_draw(bitmap,cliprect,0,0,TILEMAP_IGNORE_TRANSPARENCY,0);

	/* Sprites can be under/over the layer below text layer */
	darius_draw_sprites(bitmap,cliprect,0,-8); /* draw sprites with priority 0 which are under the mid layer*/

	/* draw middle layer*/
	PC080SN_tilemap_draw(bitmap,cliprect,0,1,0,0);

	darius_draw_sprites(bitmap,cliprect,1,-8); /* draw sprites with priority 1 which are over the mid layer*/

	/* top(text) layer is in fixed position */
	tilemap_set_scrollx(fg_tilemap,0,0);
	tilemap_set_scrolly(fg_tilemap,0,-8);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}
