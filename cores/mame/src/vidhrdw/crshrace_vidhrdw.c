#include "driver.h"
#include "vidhrdw/generic.h"
#include "vidhrdw/konamiic.h"
#include "crshrace.h"


data16_t *crshrace_videoram1,*crshrace_videoram2;
data16_t *crshrace_roz_ctrl1,*crshrace_roz_ctrl2;

static int roz_bank,gfxctrl,flipscreen;

static struct tilemap *tilemap1,*tilemap2;



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info1(int tile_index)
{
	int code = crshrace_videoram1[tile_index];

	SET_TILE_INFO(1,(code & 0xfff) + (roz_bank << 12),code >> 12,0)
}

static void get_tile_info2(int tile_index)
{
	int code = crshrace_videoram2[tile_index];

	SET_TILE_INFO(0,code,0,0)
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( crshrace )
{
	tilemap1 = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,64);
	tilemap2 = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,64);

	if (!tilemap1 || !tilemap2)
		return 1;

	K053936_wraparound_enable(0, 1);
	K053936_set_offset(0, -48, -21);

	tilemap_set_transparent_pen(tilemap1,0x0f);
	tilemap_set_transparent_pen(tilemap2,0xff);

	return 0;
}


/***************************************************************************

  Memory handlers

***************************************************************************/

WRITE16_HANDLER( crshrace_videoram1_w )
{
	int oldword = crshrace_videoram1[offset];
	COMBINE_DATA(&crshrace_videoram1[offset]);
	if (oldword != crshrace_videoram1[offset])
		tilemap_mark_tile_dirty(tilemap1,offset);
}

WRITE16_HANDLER( crshrace_videoram2_w )
{
	int oldword = crshrace_videoram2[offset];
	COMBINE_DATA(&crshrace_videoram2[offset]);
	if (oldword != crshrace_videoram2[offset])
		tilemap_mark_tile_dirty(tilemap2,offset);
}

WRITE16_HANDLER( crshrace_roz_bank_w )
{
	if (ACCESSING_LSB)
	{
		if (roz_bank != (data & 0xff))
		{
			roz_bank = data & 0xff;
			tilemap_mark_all_tiles_dirty(tilemap1);
		}
	}
}


WRITE16_HANDLER( crshrace_gfxctrl_w )
{
	if (ACCESSING_LSB)
	{
		gfxctrl = data & 0xdf;
		flipscreen = data & 0x20;
	}
}


/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;


	offs = 0;
	while (offs < 0x0400 && (buffered_spriteram16[offs] & 0x4000) == 0)
	{
		int attr_start;
		int map_start;
		int ox,oy,x,y,xsize,ysize,zoomx,zoomy,flipx,flipy,color;
		/* table hand made by looking at the ship explosion in aerofgt attract mode */
		/* it's almost a logarithmic scale but not exactly */
		int zoomtable[16] = { 0,7,14,20,25,30,34,38,42,46,49,52,54,57,59,61 };

		attr_start = 4 * (buffered_spriteram16[offs++] & 0x03ff);

		ox = buffered_spriteram16[attr_start + 1] & 0x01ff;
		xsize = (buffered_spriteram16[attr_start + 1] & 0x0e00) >> 9;
		zoomx = (buffered_spriteram16[attr_start + 1] & 0xf000) >> 12;
		oy = buffered_spriteram16[attr_start + 0] & 0x01ff;
		ysize = (buffered_spriteram16[attr_start + 0] & 0x0e00) >> 9;
		zoomy = (buffered_spriteram16[attr_start + 0] & 0xf000) >> 12;
		flipx = buffered_spriteram16[attr_start + 2] & 0x4000;
		flipy = buffered_spriteram16[attr_start + 2] & 0x8000;
		color = (buffered_spriteram16[attr_start + 2] & 0x1f00) >> 8;
		map_start = buffered_spriteram16[attr_start + 3] & 0x7fff;

		zoomx = 16 - zoomtable[zoomx]/8;
		zoomy = 16 - zoomtable[zoomy]/8;

		if (buffered_spriteram16[attr_start + 2] & 0x20ff) color = rand();

		for (y = 0;y <= ysize;y++)
		{
			int sx,sy;

			if (flipy) sy = ((oy + zoomy * (ysize - y) + 16) & 0x1ff) - 16;
			else sy = ((oy + zoomy * y + 16) & 0x1ff) - 16;

			for (x = 0;x <= xsize;x++)
			{
				int code;

				if (flipx) sx = ((ox + zoomx * (xsize - x) + 16) & 0x1ff) - 16;
				else sx = ((ox + zoomx * x + 16) & 0x1ff) - 16;

				code = buffered_spriteram16_2[map_start & 0x7fff];
				map_start++;

				if (flipscreen)
					drawgfxzoom(bitmap,Machine->gfx[2],
							code,
							color,
							!flipx,!flipy,
							304-sx,208-sy,
							cliprect,TRANSPARENCY_PEN,15,
							0x1000 * zoomx,0x1000 * zoomy);
				else
					drawgfxzoom(bitmap,Machine->gfx[2],
							code,
							color,
							flipx,flipy,
							sx,sy,
							cliprect,TRANSPARENCY_PEN,15,
							0x1000 * zoomx,0x1000 * zoomy);
			}
		}
	}
}


static void draw_bg(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	tilemap_draw(bitmap,cliprect,tilemap2,0,0);
}


static void draw_fg(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	K053936_0_zoom_draw(bitmap,cliprect,tilemap1,0,0);
}


VIDEO_UPDATE( crshrace )
{
	if (gfxctrl & 0x04)	/* display disable? */
	{
		fillbitmap(bitmap,get_black_pen(),cliprect);
		return;
	}

	fillbitmap(bitmap,Machine->pens[0x1ff],cliprect);

	switch (gfxctrl & 0xfb)
	{
		case 0x00:	/* high score screen */
			draw_sprites(bitmap,cliprect);
			draw_bg(bitmap,cliprect);
			draw_fg(bitmap,cliprect);
			break;
		case 0x01:
		case 0x02:
			draw_bg(bitmap,cliprect);
			draw_fg(bitmap,cliprect);
			draw_sprites(bitmap,cliprect);
			break;
		default:
usrintf_showmessage("gfxctrl = %02x",gfxctrl);
			break;
	}
}

VIDEO_EOF( crshrace )
{
	buffer_spriteram16_w(0,0,0);
	buffer_spriteram16_2_w(0,0,0);
}
