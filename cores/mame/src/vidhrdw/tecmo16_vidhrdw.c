/******************************************************************************
  Ganbare Ginkun  (Japan)  (c)1995 TECMO
  Final StarForce (US)     (c)1992 TECMO
  Based on sprite drivers from vidhrdw/wc90.c by Ernesto Corvi (ernesto@imagina.com)
******************************************************************************/

#include "vidhrdw/generic.h"


data16_t *tecmo16_videoram;
data16_t *tecmo16_colorram;
data16_t *tecmo16_videoram2;
data16_t *tecmo16_colorram2;
data16_t *tecmo16_charram;

static struct tilemap *fg_tilemap,*bg_tilemap,*tx_tilemap;
static int flipscreen,game_is_riot;

/******************************************************************************/

static void fg_get_tile_info(int tile_index)
{
	int tile = tecmo16_videoram[tile_index] & 0x1fff;
	int color = tecmo16_colorram[tile_index] & 0x0f;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void bg_get_tile_info(int tile_index)
{
	int tile = tecmo16_videoram2[tile_index] & 0x1fff;
	int color = (tecmo16_colorram2[tile_index] & 0x0f)+0x10;

	SET_TILE_INFO(
			1,
			tile,
			color,
			0)
}

static void tx_get_tile_info(int tile_index)
{
	int tile = tecmo16_charram[tile_index];
	SET_TILE_INFO(
			0,
			tile & 0x0fff,
			tile >> 12,
			0)
}

/******************************************************************************/

VIDEO_START( fstarfrc )
{
	fg_tilemap = tilemap_create(fg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	tx_tilemap = tilemap_create(tx_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);

	if (!fg_tilemap || !bg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_transparent_pen(tx_tilemap,0);

	tilemap_set_scrolly(tx_tilemap,0,-16);
	flipscreen = 0;
	game_is_riot = 0;
	return 0;
}

VIDEO_START( ginkun )
{
	fg_tilemap = tilemap_create(fg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(tx_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);

	if (!fg_tilemap || !bg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_transparent_pen(tx_tilemap,0);
	flipscreen = 0;
	game_is_riot = 0;

	return 0;
}

VIDEO_START( riot )
{
	fg_tilemap = tilemap_create(fg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	bg_tilemap = tilemap_create(bg_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
	tx_tilemap = tilemap_create(tx_get_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);

	if (!fg_tilemap || !bg_tilemap || !tx_tilemap)
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);
	tilemap_set_transparent_pen(bg_tilemap,0);
	tilemap_set_transparent_pen(tx_tilemap,0);

	tilemap_set_scrolldy(tx_tilemap,-16,-16);

	flipscreen = 0;
	game_is_riot = 1;

	return 0;
}


/******************************************************************************/

WRITE16_HANDLER( tecmo16_videoram_w )
{
	int oldword = tecmo16_videoram[offset];
	COMBINE_DATA(&tecmo16_videoram[offset]);
	if (oldword != tecmo16_videoram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( tecmo16_colorram_w )
{
	int oldword = tecmo16_colorram[offset];
	COMBINE_DATA(&tecmo16_colorram[offset]);
	if (oldword != tecmo16_colorram[offset])
		tilemap_mark_tile_dirty(fg_tilemap,offset);
}

WRITE16_HANDLER( tecmo16_videoram2_w )
{
	int oldword = tecmo16_videoram2[offset];
	COMBINE_DATA(&tecmo16_videoram2[offset]);
	if (oldword != tecmo16_videoram2[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE16_HANDLER( tecmo16_colorram2_w )
{
	int oldword = tecmo16_colorram2[offset];
	COMBINE_DATA(&tecmo16_colorram2[offset]);
	if (oldword != tecmo16_colorram2[offset])
		tilemap_mark_tile_dirty(bg_tilemap,offset);
}


WRITE16_HANDLER( tecmo16_charram_w )
{
	int oldword = tecmo16_charram[offset];
	COMBINE_DATA(&tecmo16_charram[offset]);
	if (oldword != tecmo16_charram[offset])
		tilemap_mark_tile_dirty(tx_tilemap,offset);
}

WRITE16_HANDLER( tecmo16_flipscreen_w )
{
	flipscreen = data & 0x01;
	flip_screen_set(flipscreen);
}

/******************************************************************************/

WRITE16_HANDLER( tecmo16_scroll_x_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(fg_tilemap,0,scroll);
}

WRITE16_HANDLER( tecmo16_scroll_y_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(fg_tilemap,0,scroll);
}

WRITE16_HANDLER( tecmo16_scroll2_x_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( tecmo16_scroll2_y_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(bg_tilemap,0,scroll);
}

WRITE16_HANDLER( tecmo16_scroll_char_x_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrollx(tx_tilemap,0,scroll);
}

WRITE16_HANDLER( tecmo16_scroll_char_y_w )
{
	static data16_t scroll;
	COMBINE_DATA(&scroll);
	tilemap_set_scrolly(tx_tilemap,0,scroll-16);
}

/******************************************************************************/

static void draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;
	const UINT8 layout[8][8] =
	{
		{0,1,4,5,16,17,20,21},
		{2,3,6,7,18,19,22,23},
		{8,9,12,13,24,25,28,29},
		{10,11,14,15,26,27,30,31},
		{32,33,36,37,48,49,52,53},
		{34,35,38,39,50,51,54,55},
		{40,41,44,45,56,57,60,61},
		{42,43,46,47,58,59,62,63}
	};

	for (offs = spriteram_size/2 - 8;offs >= 0;offs -= 8)
	{
		if (spriteram16[offs] & 0x04)	/* enable */
		{
			int code,color,sizex,sizey,flipx,flipy,xpos,ypos;
			int x,y,priority,priority_mask;

			code = spriteram16[offs+1];
			color = (spriteram16[offs+2] & 0xf0) >> 4;
			sizex = 1 << ((spriteram16[offs+2] & 0x03) >> 0);
			sizey = 1 << ((spriteram16[offs+2] & 0x0c) >> 2);

			if(game_is_riot)
				sizey = sizex;
			else
				sizey = 1 << ((spriteram16[offs+2] & 0x0c) >> 2);


			if (sizex >= 2) code &= ~0x01;
			if (sizey >= 2) code &= ~0x02;
			if (sizex >= 4) code &= ~0x04;
			if (sizey >= 4) code &= ~0x08;
			if (sizex >= 8) code &= ~0x10;
			if (sizey >= 8) code &= ~0x20;
			flipx = spriteram16[offs] & 0x01;
			flipy = spriteram16[offs] & 0x02;
			xpos = spriteram16[offs+4];
			if (xpos >= 0x8000) xpos -= 0x10000;
			ypos = spriteram16[offs+3];
			if (ypos >= 0x8000) ypos -= 0x10000;
			priority = (spriteram16[offs] & 0xc0) >> 6;

			/* bg: 1; fg:2; text: 4 */
			switch (priority)
			{
				default:
				case 0x0: priority_mask = 0; break;
				case 0x1: priority_mask = 0xf0; break; /* obscured by text layer */
				case 0x2: priority_mask = 0xf0|0xcc; break;	/* obscured by foreground */
				case 0x3: priority_mask = 0xf0|0xcc|0xaa; break; /* obscured by bg and fg */
			}

			if (flipscreen)
			{
				flipx = !flipx;
				flipy = !flipy;
			}

			for (y = 0;y < sizey;y++)
			{
				for (x = 0;x < sizex;x++)
				{
					int sx,sy;

					if (!flipscreen)
					{
						sx = xpos + 8*(flipx?(sizex-1-x):x);
						sy = ypos + 8*(flipy?(sizey-1-y):y);
					} else {
						sx = 256 - (xpos + 8*(!flipx?(sizex-1-x):x) + 8);
						sy = 256 - (ypos + 8*(!flipy?(sizey-1-y):y) + 8);
					}
					pdrawgfx(bitmap,Machine->gfx[2],
							code + layout[y][x],
							color,
							flipx,flipy,
							sx,sy,
							cliprect,TRANSPARENCY_PEN,0,
							priority_mask);
				}
			}
		}
	}
}

/******************************************************************************/

VIDEO_UPDATE( tecmo16 )
{
	fillbitmap(priority_bitmap,0,cliprect);
	fillbitmap(bitmap,Machine->pens[0x300],cliprect);
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,1);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,2);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,4);

	draw_sprites(bitmap,cliprect);
}
